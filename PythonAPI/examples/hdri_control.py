#!/usr/bin/env python

# Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma de
# Barcelona (UAB).
#
# This work is licensed under the terms of the MIT license.
# For a copy, see <https://opensource.org/licenses/MIT>.

"""Control image-based (HDRI) lighting for the current CARLA map.

HDRI is configured as named presets on the map's HDRI controller (e.g.
"airport", "workshop", "nature"). Only the preset name is sent; the look
settings and backdrop location come from the map.

Only available on maps that ship an HDRI controller; on other maps the API
raises a RuntimeError, which this example reports before exiting.

    python hdri_control.py --preset airport
    python hdri_control.py --disable
    python hdri_control.py --list
    python hdri_control.py --demo --preset airport
"""

import carla

import argparse
import sys


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
        '--preset',
        metavar='NAME',
        help='name of the HDRI preset to enable (e.g. airport)')
    argparser.add_argument(
        '--disable',
        action='store_true',
        help='disable HDRI lighting (restores the regular sky)')
    argparser.add_argument(
        '--list',
        action='store_true',
        help='list the HDRI presets available on the current map and exit')
    argparser.add_argument(
        '--demo',
        action='store_true',
        help='enable a preset, then change the weather to show that it auto-disables HDRI')
    args = argparser.parse_args()

    client = carla.Client(args.host, args.port)
    client.set_timeout(10.0)
    world = client.get_world()

    try:
        if args.list:
            presets = world.get_hdri_presets()
            print("Available HDRI presets: {}".format(presets if presets else "(none)"))
            return

        if args.disable:
            world.set_hdri_preset(None)
            print("HDRI disabled. Regular sky restored.")
            return

        if not args.preset:
            print("Nothing to do. Use --preset NAME, --disable, or --list.",
                  file=sys.stderr)
            print("Available presets: {}".format(world.get_hdri_presets()))
            sys.exit(1)

        if args.demo:
            print("Enabling HDRI preset '{}'...".format(args.preset))
            world.set_hdri_preset(args.preset)
            print("\nChanging the weather to ClearNoon (auto-disables HDRI)...")
            world.set_weather(carla.WeatherParameters.ClearNoon)
            return

        world.set_hdri_preset(args.preset)
        print("HDRI preset '{}' enabled.".format(args.preset))

    except RuntimeError as error:
        print("HDRI request failed: {}".format(error), file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
