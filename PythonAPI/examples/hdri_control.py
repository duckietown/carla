#!/usr/bin/env python

# Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma de
# Barcelona (UAB).
#
# This work is licensed under the terms of the MIT license.
# For a copy, see <https://opensource.org/licenses/MIT>.

"""
CARLA HDRI control:

Connect to a CARLA Simulator instance and control image-based (HDRI) lighting
for the current map.

When HDRI is enabled, the map's HDRIBackdrop drives all scene lighting and the
regular sky/weather actor (BP_Sky) is hidden so that only the HDRI lights the
scene. Calling ``world.set_weather(...)`` automatically disables HDRI again and
restores the regular sky.

HDRI is only available on maps that contain an HDRI controller actor. On any
other map the HDRI API raises a ``RuntimeError``; this example reports that and
exits cleanly.

Examples:

    # Enable HDRI with the default neutral cubemap
    python hdri_control.py --enable

    # Enable HDRI with a specific cubemap and intensity
    python hdri_control.py --enable --asset HDRi_Neutral --intensity 1.5

    # Disable HDRI again (restores the regular sky)
    python hdri_control.py --disable

    # Show the HDRI state currently active on the map
    python hdri_control.py --status

    # Demonstrate the "weather wins" behaviour: enabling HDRI, then changing
    # the weather, which automatically turns HDRI back off.
    python hdri_control.py --demo
"""

import carla

import argparse
import sys


def print_status(world):
    """Print the HDRI state currently active in the world."""
    hdri = world.get_hdri()
    print("Current HDRI state:")
    print("  enabled           : {}".format(hdri.enabled))
    print("  asset             : '{}'".format(hdri.asset))
    print("  intensity         : {}".format(hdri.intensity))
    print("  size              : {}".format(hdri.size))
    print("  projection_center : ({:.1f}, {:.1f}, {:.1f})".format(
        hdri.projection_center.x,
        hdri.projection_center.y,
        hdri.projection_center.z))


def main():
    argparser = argparse.ArgumentParser(description=__doc__)
    argparser.add_argument(
        '--host',
        metavar='H',
        default='127.0.0.1',
        help='IP of the host server (default: 127.0.0.1)')
    argparser.add_argument(
        '-p', '--port',
        metavar='P',
        default=2000,
        type=int,
        help='TCP port to listen to (default: 2000)')
    argparser.add_argument(
        '--enable',
        action='store_true',
        help='enable HDRI lighting (hides the regular sky)')
    argparser.add_argument(
        '--disable',
        action='store_true',
        help='disable HDRI lighting (restores the regular sky)')
    argparser.add_argument(
        '--status',
        action='store_true',
        help='print the HDRI state currently active and exit')
    argparser.add_argument(
        '--demo',
        action='store_true',
        help='enable HDRI, then change the weather to show that it auto-disables HDRI')
    argparser.add_argument(
        '--asset',
        metavar='NAME',
        default='HDRi_Neutral',
        help="cubemap asset name in the default HDRI directory (default: HDRi_Neutral)")
    argparser.add_argument(
        '--intensity',
        metavar='F',
        default=1.0,
        type=float,
        help='HDRI lighting intensity (default: 1.0)')
    argparser.add_argument(
        '--size',
        metavar='F',
        default=1000.0,
        type=float,
        help='HDRIBackdrop dome size/radius (default: 1000.0)')
    argparser.add_argument(
        '--projection',
        metavar=('X', 'Y', 'Z'),
        nargs=3,
        default=[0.0, 0.0, 0.0],
        type=float,
        help='HDRIBackdrop projection center, 3 floats (default: 0 0 0)')
    argparser.add_argument(
        '--location',
        metavar=('X', 'Y', 'Z'),
        nargs=3,
        default=[0.0, 0.0, 0.0],
        type=float,
        help='World location (Unreal coords, cm) of the HDRIBackdrop actor, 3 floats (default: 0 0 0)')
    args = argparser.parse_args()

    projection_center = carla.Vector3D(args.projection[0], args.projection[1], args.projection[2])
    location = carla.Vector3D(args.location[0], args.location[1], args.location[2])

    client = carla.Client(args.host, args.port)
    client.set_timeout(10.0)
    world = client.get_world()

    try:
        if args.status:
            print_status(world)
            return

        if args.disable:
            world.set_hdri(carla.HDRIParameters(enabled=False))
            print("HDRI disabled. Regular sky / weather restored.")
            return

        if args.demo:
            print("Enabling HDRI with asset '{}'...".format(args.asset))
            world.set_hdri(carla.HDRIParameters(
                enabled=True,
                asset=args.asset,
                intensity=args.intensity,
                size=args.size,
                projection_center=projection_center,
                location=location))
            print_status(world)
            print("\nNow changing the weather to ClearNoon — this should "
                  "automatically disable HDRI and restore the sky...")
            world.set_weather(carla.WeatherParameters.ClearNoon)
            print_status(world)
            return

        # Default action (and --enable) enables HDRI.
        world.set_hdri(carla.HDRIParameters(
            enabled=True,
            asset=args.asset,
            intensity=args.intensity,
            size=args.size,
            projection_center=projection_center,
            location=location))
        print("HDRI enabled with asset '{}' (intensity={}, size={}).".format(
            args.asset, args.intensity, args.size))
        print_status(world)

    except RuntimeError as error:
        # Raised when the current map does not support HDRI (no HDRI controller),
        # or when the requested cubemap asset could not be found.
        print("HDRI request failed: {}".format(error), file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
