#!/usr/bin/env python3
"""
place_duckies.py  —  Scatter BP_DuckieRubber props along non-junction road
splines with random longitudinal spacing and an optional lateral offset.

Each duckie is oriented using the full waypoint transform so that its
up-axis matches the road-surface normal (handles banked / pitched roads).

Usage examples
--------------
  # defaults: 2 – 8 m spacing, centred on lane
  python place_duckies.py

  # tighter spacing, offset 0.15 m to the right of centre
  python place_duckies.py --min-spacing 1.0 --max-spacing 3.0 \
                          --lateral-offset 0.15

  # remove all previously spawned duckies and exit
  python place_duckies.py --cleanup
"""

import argparse
import random

import numpy as np
import carla

_DUCKIE_BP_ID = 'static.prop.rubberduckie'


# ---------------------------------------------------------------------------
# Road-segment extraction
# ---------------------------------------------------------------------------

def _collect_non_junction_segments(carla_map, sample_distance=0.5):
    """
    Return a list of waypoint lists.  Each inner list is one continuous,
    non-junction road segment sampled every *sample_distance* metres.
    """
    topology = carla_map.get_topology()
    segments = []
    seen_road_lane = set()

    for start_wp, _ in topology:
        if start_wp.is_junction:
            continue

        key = (start_wp.road_id, start_wp.lane_id)
        if key in seen_road_lane:
            continue
        seen_road_lane.add(key)

        waypoints = [start_wp]
        current = start_wp

        while True:
            nexts = current.next(sample_distance)
            if not nexts:
                break
            nxt = nexts[0]
            # Stop when the segment enters a junction or a different road
            if nxt.is_junction or nxt.road_id != start_wp.road_id:
                break
            waypoints.append(nxt)
            current = nxt

        if len(waypoints) >= 2:
            segments.append(waypoints)

    return segments


# ---------------------------------------------------------------------------
# Arc-length helpers
# ---------------------------------------------------------------------------

def _arc_lengths(waypoints):
    """Cumulative distances along a waypoint list (metres)."""
    s = [0.0]
    for i in range(1, len(waypoints)):
        a = waypoints[i - 1].transform.location
        b = waypoints[i].transform.location
        s.append(s[-1] + a.distance(b))
    return s


def _waypoint_at(waypoints, cumulative_s, target_s):
    """Return the waypoint whose arc-length is just below *target_s*."""
    lo, hi = 0, len(cumulative_s) - 1
    while lo < hi - 1:
        mid = (lo + hi) // 2
        if cumulative_s[mid] <= target_s:
            lo = mid
        else:
            hi = mid
    return waypoints[lo]


# ---------------------------------------------------------------------------
# Duckie placement
# ---------------------------------------------------------------------------

def _build_transform(wp, lateral_offset, z_offset):
    rotation = carla.Rotation(
        pitch=wp.transform.rotation.pitch,
        yaw=wp.transform.rotation.yaw + random.uniform(0.0, 360.0),
        roll=wp.transform.rotation.roll,
    )
    transform = carla.Transform(
        carla.Location(
            x=wp.transform.location.x,
            y=wp.transform.location.y,
            z=wp.transform.location.z + z_offset,
        ),
        rotation,
    )
    if lateral_offset != 0.0:
        right = wp.transform.get_right_vector()
        transform.location.x += right.x * lateral_offset
        transform.location.y += right.y * lateral_offset
        transform.location.z += right.z * lateral_offset
    return transform


def _build_spline_array(segments):
    """Flatten all waypoint XY positions into an (N, 2) numpy array."""
    coords = []
    for seg in segments:
        for wp in seg:
            coords.append((wp.transform.location.x, wp.transform.location.y))
    return np.array(coords, dtype=np.float32)


def _min_dist_to_splines_2d(location, spline_xy):
    """Return the minimum 2D distance from *location* to the precomputed spline array."""
    diff = spline_xy - np.array([location.x, location.y], dtype=np.float32)
    return float(np.sqrt((diff ** 2).sum(axis=1)).min())


def place_random_duckies(
    client,
    world,
    *,
    min_spacing: float = 2.0,
    max_spacing: float = 8.0,
    min_lateral_offset: float = 0.0,
    max_lateral_offset: float = 0.0,
    z_offset: float = 0.05,
    sample_distance: float = 0.25,
    spawn_probability: float = 1.0,
    seed: int | None = None,
):
    if seed is not None:
        random.seed(seed)

    carla_map = world.get_map()
    duckie_bp = world.get_blueprint_library().find(_DUCKIE_BP_ID)

    segments = _collect_non_junction_segments(carla_map, sample_distance)
    print(f'[duckies] found {len(segments)} non-junction road segments')

    # --- Pass 1: generate and validate all transforms locally (no server calls) ---
    spline_xy = _build_spline_array(segments)
    transforms = []

    for seg in segments:
        s_vals = _arc_lengths(seg)
        total_len = s_vals[-1]

        if total_len < min_spacing:
            continue

        s = random.uniform(0.0, min_spacing)

        while s < total_len:
            wp = _waypoint_at(seg, s_vals, s)
            offset = random.uniform(min_lateral_offset, max_lateral_offset)
            transform = _build_transform(wp, offset, z_offset)

            if (_min_dist_to_splines_2d(transform.location, spline_xy) >= min_lateral_offset
                    and random.random() < spawn_probability):
                transforms.append(transform)

            s += random.uniform(min_spacing, max_spacing)

    print(f'[duckies] {len(transforms)} positions generated')

    # --- Pass 2: batch spawn in one round-trip ---
    SpawnActor = carla.command.SpawnActor
    responses = client.apply_batch_sync(
        [SpawnActor(duckie_bp, t) for t in transforms],
        True,
    )

    actor_ids = [r.actor_id for r in responses if not r.error]
    failed = sum(1 for r in responses if r.error)
    if failed:
        print(f'[duckies] {failed} spawn failures (collisions / out-of-bounds)')

    spawned = world.get_actors(actor_ids)
    print(f'[duckies] spawned {len(actor_ids)} duckies')
    return list(spawned)


# ---------------------------------------------------------------------------
# Cleanup helper
# ---------------------------------------------------------------------------

def cleanup_duckies(client, world):
    """Destroy all spawned rubber duckie props (not the Duckiebot vehicle)."""
    actors = [a for a in world.get_actors() if a.type_id == _DUCKIE_BP_ID]
    if not actors:
        print('[duckies] no duckie props found')
        return 0
    responses = client.apply_batch_sync(
        [carla.command.DestroyActor(a) for a in actors], True
    )
    destroyed = sum(1 for r in responses if not r.error)
    print(f'[duckies] destroyed {destroyed} duckie props')
    return destroyed


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _parse_args():
    parser = argparse.ArgumentParser(
        description='Place BP_DuckieRubber props along non-junction road splines.'
    )
    parser.add_argument('--host', default='127.0.0.1', help='CARLA server host')
    parser.add_argument('--port', type=int, default=2000, help='CARLA server port')
    parser.add_argument('--timeout', type=float, default=10.0, help='Client timeout (s)')

    parser.add_argument(
        '--min-spacing', type=float, default=8.0,
        help='Minimum longitudinal gap between duckies in metres (default: 2.0)'
    )
    parser.add_argument(
        '--max-spacing', type=float, default=16.0,
        help='Maximum longitudinal gap between duckies in metres (default: 8.0)'
    )
    parser.add_argument(
        '--min-lateral-offset', type=float, default=0.0,
        help='Minimum lateral offset from lane centre in metres (default: 0.0)'
    )
    parser.add_argument(
        '--max-lateral-offset', type=float, default=0.0,
        help='Maximum lateral offset from lane centre in metres (default: 0.0)'
    )
    parser.add_argument(
        '--z-offset', type=float, default=0.05,
        help='Height offset above road surface in metres (default: 0.05)'
    )
    parser.add_argument(
        '--sample-distance', type=float, default=0.25,
        help='Internal waypoint sampling resolution in metres (default: 0.25)'
    )
    parser.add_argument(
        '--spawn-probability', type=float, default=1.0,
        help='Probability [0.0–1.0] that each candidate position spawns a duckie (default: 1.0)'
    )
    parser.add_argument(
        '--seed', type=int, default=None,
        help='RNG seed for reproducible placement (default: random)'
    )
    parser.add_argument(
        '--cleanup', action='store_true',
        help='Destroy all existing duckie actors and exit'
    )
    return parser.parse_args()


def main():
    args = _parse_args()

    client = carla.Client(args.host, args.port)
    client.set_timeout(args.timeout)

    world = client.get_world()
    print(f'[duckies] connected to  {args.host}:{args.port}  '
          f'map={world.get_map().name}')

    if args.cleanup:
        cleanup_duckies(client, world)
        return

    place_random_duckies(
        client,
        world,
        min_spacing=args.min_spacing,
        max_spacing=args.max_spacing,
        min_lateral_offset=args.min_lateral_offset,
        max_lateral_offset=args.max_lateral_offset,
        spawn_probability=args.spawn_probability,
        z_offset=args.z_offset,
        sample_distance=args.sample_distance,
        seed=args.seed,
    )


if __name__ == '__main__':
    main()
