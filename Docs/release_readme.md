CARLA Duckietown
================

Thanks for downloading CARLA Duckietown — a modified build of the CARLA
simulator carrying the six Duckietown maps, the Duckiebot, the Duckietown sign
set, rubber duckie props, extra semantic labels and switchable HDRI lighting.

IMPORTANT: this is a modified CARLA, not an asset pack
-----------------------------------------------------

The simulator and the Python client are both changed, so they are not
interchangeable with stock CARLA. Install the client from the wheel shipped in
this package — the `carla` package from PyPI will not work against this server,
and this client will not work against an upstream CARLA server.


Installing the Python client
----------------------------

From this folder, pick the wheel matching your Python version:

```sh
python3 -m pip install --upgrade -r PythonAPI/carla/requirements.txt
python3 -m pip install PythonAPI/carla/dist/carla_duckietown-*.whl
```

If your system Python is externally managed (Ubuntu 24.04 and newer refuse a
bare `pip install`), install into a virtual environment instead:

```sh
python3 -m venv ~/carla-venv
source ~/carla-venv/bin/activate
python3 -m pip install PythonAPI/carla/dist/carla_duckietown-*.whl
```


How to run
----------

Launch a terminal in this folder and start the simulator:

```sh
./CarlaUE4.sh
```

This opens a window looking at a Duckietown map from the "spectator" view. You
can fly around with the mouse and WASD keys, but you cannot interact with the
world from this view — the simulator is now running as a server, waiting for a
client to connect.

Useful launch options:

```sh
./CarlaUE4.sh -RenderOffScreen        # headless, for data generation
./CarlaUE4.sh -quality-level=Low      # cheaper rendering
./CarlaUE4.sh -carla-rpc-port=3000    # non-default port
```

The server listens on ports 2000 and 2001 by default. The first launch is
slower while shaders warm up.


Frame rate
----------

Rendering is capped at 60 FPS. The Duckietown maps are light enough that an
uncapped server idles at several hundred frames per second, which buys nothing
and makes some GPUs audibly whine.

To change it, edit `FrameRateLimit` in
`CarlaUE4/Config/DefaultGameUserSettings.ini`. Once you have launched the
simulator at least once there is also a per-user copy at
`~/.config/Epic/CarlaUE4/Saved/Config/LinuxNoEditor/GameUserSettings.ini`, and
that one takes precedence. Set the value to `0` for uncapped.

The `-benchmark` and `-fps=` command line flags do not work here: that code is
compiled out of packaged builds. And note this is a rendering cap only — for
reproducible, evenly spaced sensor captures, set `fixed_delta_seconds` and
`synchronous_mode` on the world from your client instead.


Driving a Duckiebot
-------------------

With the simulator running, open a second terminal in this folder:

```sh
cd PythonAPI/duckietown
python3 manual_control.py
```

This opens a view of a Duckiebot you can drive with the WASD or arrow keys.
Useful keys:

    W A S D    drive
    U          next Duckietown map (Shift+U for previous)
    J          cycle the map's HDRI lighting presets (Shift+J for previous)
    ` or N     next sensor
    P          toggle autopilot
    H          full key list

The other scripts in that folder:

    generate_traffic.py    populate the map with autopiloted Duckiebots
    place_duckies.py       scatter rubber duckies along the road (--cleanup to remove)
    hdri_control.py        list, apply and disable HDRI lighting presets

The upstream CARLA example scripts in `PythonAPI/examples` also work, but they
default to CARLA's own towns and vehicles rather than Duckietown content.


Documentation
-------------

The Python API reference for this build is included as
`PythonAPI/python_api.md`. It documents the Duckietown additions alongside the
standard CARLA API — notably `World.set_hdri_preset`, `World.get_hdri_presets`
and `Actor.set_hidden_in_game`.

For general CARLA concepts, refer to the upstream documentation:

<http://carla.readthedocs.io>
