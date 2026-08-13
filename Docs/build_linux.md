# Linux build

This guide details how to build CARLA Duckietown from source on Linux. It follows the upstream CARLA build closely; the parts that differ are the repository you clone, the content download, and the scripts you run once the simulator is up.

The build process is long — 3 to 4 hours — and most of that time is spent compiling Unreal Engine. It is highly recommended to read through the guide fully before starting.

!!! note
    If you do not need the Unreal Editor or to modify the C++ code, you do not need to build at all. The [prebuilt release](https://github.com/duckietown/carla/releases) ships the simulator with all six Duckietown maps and a matching Python API wheel.

If you come across errors or difficulties then have a look at the **[F.A.Q.](build_faq.md)** page, which offers solutions for the most common complications. The [upstream CARLA discussions](https://github.com/carla-simulator/carla/discussions) are still the best place to ask about problems that are not specific to Duckietown.

- [__Prerequisites__](#part-one-prerequisites)
    - [System requirements](#system-requirements)
    - [Software requirements](#software-requirements)
- [__Building Unreal Engine__](#building-unreal-engine)
- [__Building CARLA Duckietown__](#building-carla-duckietown)
    - [Clone the repository](#clone-the-repository)
    - [Set up the CARLA_UE4_ROOT environment variable](#set-up-the-carla_ue4_root-environment-variable)
    - [Download the content](#download-the-content)
    - [Build with Make](#build-with-make)
        - [Compile the Python API client](#1-compile-the-python-api-client)
        - [Compile the server](#2-compile-the-server)
        - [Start the simulation](#3-start-the-simulation)
    - [Additional Make options](#additional-make-options)
    - [Running tests](#running-tests)

---
## Part One: Prerequisites

### System requirements

* __Ubuntu 22.04 or 26.04__: the supported versions. 22.04 is the recommended minimum — earlier releases are no longer supported, in part because a client wheel built here requires glibc 2.34 or newer. Pick the matching package list in [Software requirements](#software-requirements) below; the two differ.
* __130 GB of disk space__: roughly 91 GB for Unreal Engine and 31 GB for CARLA and its content. Note that `Update.sh` *backs up* any content folder it replaces instead of deleting it, so repeated content updates will consume more space until you remove the timestamped backups under `Unreal/CarlaUE4/Content`.
* __A dedicated GPU__: CARLA places a high demand on the GPU. An NVIDIA RTX 2070 or better with at least 6 GB of VRAM is the minimum; 8 GB or more is recommended, and working in the Unreal Editor benefits from more still.
* __A high-performance CPU__: an Intel Core i7 with 4 or more cores, or equivalent.
* __At least 32 GB of RAM__ to build and run the simulator comfortably.
* __Two TCP ports and a good internet connection__: 2000 and 2001 by default. Make sure they are not blocked by a firewall or another application.
* __Python 3.10 or higher__. Note that the client is a compiled extension, so a wheel only works with the exact Python version it was built against; `make PythonAPI` targets whichever `python3` is on your `PATH`.

### Software requirements

CARLA requires numerous software tools for compilation. Some are built during the CARLA build process itself, such as *Boost.Python*. Others are binaries that must be installed before starting the build.

#### Ubuntu 22.04
```sh
sudo apt-get update
sudo apt-get install build-essential g++-12 cmake ninja-build libvulkan1 python3 python3-dev python3-pip python3-venv python3-requests autoconf wget curl rsync unzip git git-lfs libpng-dev libtiff5-dev libjpeg-dev aria2
```

#### Ubuntu 26.04
```sh
sudo apt-get update
sudo apt-get install build-essential g++-12 cmake ninja-build lld libvulkan1 python3 python3-dev python3-pip python3-venv python3-requests autoconf wget curl rsync unzip git git-lfs libpng-dev libtiff-dev libjpeg-dev aria2
```

!!! important
    Three things differ on newer releases, all handled above or automatically:

    * __`libtiff-dev` instead of `libtiff5-dev`__. The `5` variant was dropped after 22.04, so the older command fails outright. `libtiff-dev` resolves to the same package on 22.04, so it is safe everywhere.
    * __`lld` is required.__ The linker bundled with Unreal Engine cannot read the `.relr.dyn` sections present in glibc 2.36 and newer. `Ubuntu24Compat.sh` detects this and wraps the compiler to use `ld.lld` instead, but it exits with an error if `lld` is not installed.
    * __The system Python is [externally managed](https://packaging.python.org/en/latest/specifications/externally-managed-environments/)__ (PEP 668), so a bare `pip install` into it is refused. This does not break the build: `Ubuntu24Compat.sh` detects it, the wheel is still built into `PythonAPI/carla/dist`, and the build prints the commands to install it manually. Using a [virtual environment](#1-compile-the-python-api-client) avoids the issue entirely.

!!! note
    `g++-12` is named explicitly because that is the version the build expects, but the host compiler does less work than it appears to: `Setup.sh` switches to the clang toolchain bundled with Unreal Engine for the bulk of the compilation. If `g++-12` is unavailable from the default repositories on your release, the [Toolchain PPA](https://launchpad.net/~ubuntu-toolchain-r/+archive/ubuntu/test) provides it.

!!! important
    `python3-requests` is required by [`Update.sh`](#download-the-content) — the Duckietown content is fetched from Google Drive by `Util/download_from_gdrive.py`, which imports `requests`. Without it the content download fails with a `ModuleNotFoundError` before anything is compiled.

`aria2` is optional but recommended: `Update.sh` will use `aria2c` to fetch the CARLA content archive over multiple connections, falling back to `wget` when it is missing. The Duckietown archive is always a single-stream download.

## Building Unreal Engine

This version of CARLA uses a modified fork of Unreal Engine 4.26. This fork contains patches specific to CARLA, so a build from the Epic Games Launcher will not work.

Be aware that to download this fork of Unreal Engine, __you need to have a GitHub account linked to the Epic Games organization__. If you don't have this link already set up, please follow [this guide](https://www.unrealengine.com/en-US/ue4-on-github) before going any further — without it the clone below fails with a 404.

__1.__ **Clone the content for CARLA's fork of Unreal Engine 4.26 to your local computer**:

```sh
git clone --depth 1 -b carla https://github.com/CarlaUnreal/UnrealEngine.git ~/UnrealEngine_4.26
```
!!! Note
    Since github doesn't allow authentication with usename/password anymore, a personal authentication token can be used to clone the UnrealEngine repository. Here's the command to clone with OAuth.

```sh
git clone --depth 1 -b carla https://oauth2:TOKEN@github.com/CarlaUnreal/UnrealEngine.git ~/UnrealEngine_4.26
```

__2.__ **Navigate into the directory where you cloned the Unreal Engine repository**:
```sh
cd ~/UnrealEngine_4.26
```

__3.__ **Set up and build with `make`. `Setup.sh` pulls around 10 GB of binary dependencies, and the build itself may take an hour or two depending on your system**:
```sh
./Setup.sh && ./GenerateProjectFiles.sh && make
```
!!! Warning
    Do not use `-j` tag to use all processor cores, e.g., `make -j$(nproc)`. This will cause the build to fail. Clang will use all available cores anyway.  

__4.__ **Open the Editor to check that Unreal Engine has been installed properly**:
```sh
cd ~/UnrealEngine_4.26/Engine/Binaries/Linux && ./UE4Editor
```

__5.__ **Set the Unreal Engine environment variable**:

For CARLA to locate the correct installation of Unreal Engine, an environment variable is needed.

To set the variable for this session only:

```sh
export UE4_ROOT=~/UnrealEngine_4.26
```

You will want to set the environment variable in your `.bashrc` or `.profile` (or the equivalent for your shell) so that it is always set:

```sh
echo 'export UE4_ROOT=~/UnrealEngine_4.26' >> ~/.bashrc
source ~/.bashrc
```

---

## Building CARLA Duckietown

### Clone the repository

Clone the `ue4-dev` branch of the Duckietown CARLA repository:

```sh
git clone -b ue4-dev git@github.com:duckietown/carla.git CarlaSource
cd CarlaSource
```

If you have not set up an SSH key with GitHub, use the HTTPS remote instead:

```sh
git clone -b ue4-dev https://github.com/duckietown/carla.git CarlaSource
```

!!! note
    `ue4-dev` is the working branch for this Unreal Engine 4.26 build and is the branch you want. Always check which branch you are on with `git branch`. The `ue5-dev` branch targets Unreal Engine 5 and is built differently.

### Set up the CARLA_UE4_ROOT environment variable

For the following build commands it is convenient to create a `CARLA_UE4_ROOT` environment variable pointing at the root of the repository you just cloned. Run the following in your shell (you may also want to add it to `.bashrc` or `.profile` for future sessions):

```sh
export CARLA_UE4_ROOT=/path/to/CarlaSource
```

If you choose not to use an environment variable, replace `${CARLA_UE4_ROOT}` in the following commands with the appropriate directory. Every `make` command must be run from that directory.

### Download the content

The repository holds code only. Maps, meshes and textures are distributed as two separate archives:

* __CARLA's own content__, pinned in [`Util/ContentVersions.txt`](https://github.com/duckietown/carla/blob/ue4-dev/Util/ContentVersions.txt), extracted into `${CARLA_UE4_ROOT}/Unreal/CarlaUE4/Content/Carla`.
* __The Duckietown package__ — the six Duckietown maps, the Duckiebot, the signs, the duckie props and the HDRI presets — pinned in [`Util/DuckietownContentVersions.txt`](https://github.com/duckietown/carla/blob/ue4-dev/Util/DuckietownContentVersions.txt), extracted into `${CARLA_UE4_ROOT}/Unreal/CarlaUE4/Content/Duckietown`.

One script fetches and extracts both:

```sh
./Update.sh
```

Each half is tracked by a `.version` file inside its content folder, so re-running `Update.sh` after a `git pull` only downloads what actually changed. If a folder does need replacing, the existing one is renamed with a timestamp suffix rather than deleted — remove those backups yourself once you are satisfied with the new content.

Two flags let you fetch just one half:

| Flag | Effect |
| ---- | ------ |
| `-s`, `--skip-download` | Skip the CARLA content, download only the Duckietown package. |
| `-d`, `--skip-duckietown` | Skip the Duckietown package, download only the CARLA content. |

!!! note
    Passing `-s` prints the direct link to the CARLA content archive so you can download and extract it manually into `Unreal/CarlaUE4/Content/Carla`.

#### Using Git for the CARLA content

If you intend to commit changes to the *upstream* CARLA content, you can keep that half as a git repository instead. `Update.sh` detects a `.git` directory in the content folder and leaves it alone:

```sh
git clone -b master https://bitbucket.org/carla-simulator/carla-content ${CARLA_UE4_ROOT}/Unreal/CarlaUE4/Content/Carla
```

The Duckietown package is not distributed as a git repository; use `Update.sh` for it.

#### Downloading the content for a specific version

To pin the content to an older version, take the relevant ID from `Util/ContentVersions.txt` or `Util/DuckietownContentVersions.txt`, download that archive, and extract it into the matching folder:

```sh
tar -xvzf <assets_archive>.tar.gz -C ${CARLA_UE4_ROOT}/Unreal/CarlaUE4/Content/Carla
```

Write the ID you used into the folder's `.version` file so that the next `Update.sh` run does not immediately replace it.

---

### Build with Make

The following commands should be run from the root folder of the repository. There are two parts to the build process, compiling the client and compiling the server.

#### 1. Compile the Python API client

The Python API client grants control over the simulation. Compilation of the client is required the first time you build and again after you perform any updates. After the client is compiled, you will be able to run scripts to interact with the simulation.

Install the Python prerequisites:

```sh
python3 -m pip install --upgrade -r ${CARLA_UE4_ROOT}/PythonAPI/carla/requirements.txt
```

Then build the Python API with the following command. The first run takes 20 to 30 minutes as it compiles LibCarla and its dependencies; later runs are incremental:

```sh
make PythonAPI
``` 

!!! note
    **NumPy 2 error**: If the Python installation or environment that you are using to build CARLA has *numpy>=2.0.0* installed, this will cause an error during the build process due to conflicting dependencies. This should be the first thing to check when encountering errors related to Boost. Check your NumPy version using `python3 -m pip show numpy`.

**Building the Python API for a specific Python version**

The above command will compile CARLA with the system default Python version, which is called when you run `python3` on the command line. If you wish to build CARLA for other Python versions, we recommend you use virtual environments.

For some Python versions, the Deadsnakes repository may be needed:

```sh
# The Deadsnakes PPA may be needed for some Python versions
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt update
```

For the version of Python you are targeting, install Python along with the development headers and the Python `venv` package:

```sh
sudo apt-get install python3.X python3.X-dev python3.X-venv # Replace X with correct version number
```

Create a new virtual environment for your target Python version:

```sh
#Replace X with the appropriate Python version number and "myenv" with a name of your choice
python3.X -m venv myenv
```

Activate the new virtual environment, then install the CARLA requirements:

```sh
source myenv/bin/activate
(myenv): python3 -m pip install --upgrade -r ${CARLA_UE4_ROOT}/PythonAPI/carla/requirements.txt
```

Finally, run `make PythonAPI` with the Python virtual environment activated:

```sh
(myenv): make PythonAPI
```

The CARLA Python API wheel will be generated in `${CARLA_UE4_ROOT}/PythonAPI/carla/dist`. The name of the wheel depends on the current CARLA version and the chosen Python version. Install the wheel with PIP:

```sh
# CARLA 0.9.16, Python 3.10
python3 -m pip install ${CARLA_UE4_ROOT}/PythonAPI/carla/dist/carla_duckietown-1.0-cp310-cp310-linux_x86_64.whl

# Or let the shell pick whichever wheel was just built
python3 -m pip install ${CARLA_UE4_ROOT}/PythonAPI/carla/dist/carla_duckietown-*.whl
```

!!! Warning
    Issues can arise through the use of different methods to install the CARLA client library and having different versions of CARLA on your system. It is recommended to use virtual environments when installing the `.whl` and to [uninstall](build_faq.md#how-do-i-uninstall-the-carla-client-library) any previously installed client libraries before installing new ones.

#### 2. Compile the server

The following command compiles and launches the Unreal Engine editor. Run this command each time you want to launch the server or use the Unreal Engine editor:

```sh
make launch
```

During the first launch the editor compiles shaders and mesh distance fields for every Duckietown map, which can take 20 minutes or more. The maps will not show properly until it finishes. Subsequent launches of the editor will be quicker.

![ue4_editor_open](img/ue4_editor_open.png)

!!! note
    **NumPy 2 error**: `make launch` can be affected by the NumPy 2 conflict, check the NumPy version in your Python installation using PIP: `python3 -m pip show numpy`. If it is version *2.0.0* or later, you will need to downgrade to *numpy<2.0.0*.

#### 3. Start the simulation

Press **Play** to start the server simulation. The camera can be moved with `WASD` keys and rotated by clicking the scene while moving the mouse around.  

Test the simulator using the Duckietown scripts in `PythonAPI/duckietown`. With the simulator running, open a new terminal for each script:

```sh
# Terminal A — drive a Duckiebot, and switch maps, lighting and sensors live
cd ${CARLA_UE4_ROOT}/PythonAPI/duckietown
python3 manual_control.py

# Terminal B — populate the map with autopiloted Duckiebots
cd ${CARLA_UE4_ROOT}/PythonAPI/duckietown
python3 generate_traffic.py
```

`manual_control.py` always spawns `vehicle.duckietown.duckiebot`. Press `H` for the full key list; `U` cycles the Duckietown maps and `J` cycles the map's HDRI lighting presets. The other two scripts are `place_duckies.py`, which scatters duckie props along the road (`--cleanup` removes them again), and `hdri_control.py`, which lists, applies and disables HDRI presets from the command line.

The upstream CARLA example scripts in `PythonAPI/examples` also work, but they default to CARLA's own towns and vehicles rather than Duckietown content.

!!! Important
    If the simulation is running at a very low FPS rate, go to `Edit -> Editor preferences -> Performance` in the Unreal Engine editor and disable `Use less CPU when in background`.

---

### Additional Make options

There are more `make` commands that you may find useful. Find them in the table below:  

| Command | Description |
| ------- | ------- |
| `make help`                                                           | Prints all available commands.                                        |
| `make launch`                                                         | Launches CARLA server in Editor window.                               |
| `make launch-only`                                                    | Launches the Editor without recompiling first.                        |
| `make PythonAPI`                                                      | Builds the CARLA client.                                              |
| `make LibCarla`                                                       | Prepares the CARLA library to be imported anywhere.                   |
| `make package`                                                        | Builds CARLA and creates a packaged version for distribution in `Dist`. |
| `make clean`                                                          | Deletes all the binaries and temporals generated by the build system. |
| `make rebuild`                                                        | Removes intermediate build files and rebuilds the whole project. Does not launch the Editor. |
| `make hard-clean`                                                     | A more thorough `make clean`; prints how to force recompiling dependencies. |

---

### Running tests

CARLA's code comes with a suite of tests designed to detect regressions in fundamental functionality that might be introduced by new code changes. If you are managing your own build we recommend that you run the test suite at least periodically to detect breaking changes.

First, create a CARLA package from your latest changes:

```sh
make package
```

The package will be created in the `Dist` folder with a name dependent on the last commit. Run the simulator from the newly built package, substituting the appropriate package ID:

```sh
./Dist/CARLA_<package_id>/LinuxNoEditor/CarlaUE4.sh --ros2 -RenderOffScreen --carla-rpc-port=<port> --carla-streaming-port=0 -nosound
```

Once the simulator is running, run the smoke tests:

```sh
make smoke_tests ARGS="--xml --python-version=<python_version>"
```

Then, finally, run the examples:

```sh
make run-examples ARGS="localhost <port>"
```

You will be alerted on the command line if any tests fail. You can find the smoke tests in `${CARLA_UE4_ROOT}/PythonAPI/test/smoke`. 

---

Read the **[F.A.Q.](build_faq.md)** page or post in the [upstream CARLA discussions](https://github.com/carla-simulator/carla/discussions) for any issues regarding this guide.  

Up next, learn how to update the build or take your first steps in the simulation, and learn some core concepts.  
<div class="build-buttons">

<p>
<a href="../core_concepts" target="_blank" class="btn btn-neutral" title="Learn about CARLA core concepts">
First steps</a>
</p>

</div>
