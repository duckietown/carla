#! /bin/bash
DOC_STRING="Provision the additional UE4 plugins CARLA Duckietown needs: StreetMap
(downloaded from GitHub) and a content-only copy of HDRIBackdrop (taken from the
engine at \${UE4_ROOT})."

USAGE_STRING=$(cat <<- END
Usage: $0 [-h|--help]

commands

    [--clean]    Clean intermediate files.
    [--rebuild]  Clean and rebuild both configurations.
END
)

REMOVE_INTERMEDIATE=false
BUILD_STREETMAP=false
BUILD_HDRIBACKDROP=false
GIT_PULL=true
CURRENT_STREETMAP_COMMIT=260273d6b7c3f28988cda31fd33441de7e272958
STREETMAP_BRANCH=master
STREETMAP_REPO=https://github.com/carla-simulator/StreetMap.git

OPTS=`getopt -o h --long build,rebuild,clean,chrono,chrono-path: -n 'parse-options' -- "$@"`

eval set -- "$OPTS"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --rebuild )
      REMOVE_INTERMEDIATE=true;
      BUILD_STREETMAP=true;
      BUILD_HDRIBACKDROP=true;
      shift ;;
    --build )
      BUILD_STREETMAP=true;
      BUILD_HDRIBACKDROP=true;
      shift ;;
    --no-pull )
      GIT_PULL=false
      shift ;;
    --clean )
      REMOVE_INTERMEDIATE=true;
      shift ;;
    --chrono )
      shift ;;
    --chrono-path )
      shift 2 ;;
    -h | --help )
      echo "$DOC_STRING"
      echo "$USAGE_STRING"
      exit 1
      ;;
    * )
      shift ;;
  esac
done

source $(dirname "$0")/Environment.sh

if ! { ${REMOVE_INTERMEDIATE} || ${BUILD_STREETMAP} || ${BUILD_HDRIBACKDROP}; }; then
  fatal_error "Nothing selected to be done."
fi

# ==============================================================================
# -- Clean intermediate files --------------------------------------------------
# ==============================================================================

if ${REMOVE_INTERMEDIATE} ; then

  log "Cleaning intermediate files and folders."

  UE4_INTERMEDIATE_FOLDERS="Binaries Build Intermediate DerivedDataCache"

  pushd "${CARLAUE4_STREETMAP_FOLDER}" >/dev/null

  rm -Rf ${UE4_INTERMEDIATE_FOLDERS}

  popd >/dev/null

fi

# ==============================================================================
# -- Build library -------------------------------------------------------------
# ==============================================================================

if ${BUILD_STREETMAP} ; then
  log "Downloading STREETMAP plugin."
  if ${GIT_PULL} ; then
    if [ ! -d ${CARLAUE4_STREETMAP_FOLDER} ] ; then
      git clone -b ${STREETMAP_BRANCH} ${STREETMAP_REPO} ${CARLAUE4_STREETMAP_FOLDER}
    fi
    cd ${CARLAUE4_STREETMAP_FOLDER}
    git fetch
    git checkout ${CURRENT_STREETMAP_COMMIT}
  fi
fi

log "StreetMap Success!"

# ==============================================================================
# -- Provision the HDRIBackdrop plugin -----------------------------------------
# ==============================================================================
#
# AHDRIController drives Epic's HDRIBackdrop actor, loading
# /HDRIBackdrop/Blueprints/HDRIBackdrop.HDRIBackdrop_C by path at runtime. The
# engine ships that plugin, but its only module is Editor-type and links
# UnrealEd, so it is not staged into a packaged (non-editor) build and the class
# fails to load there.
#
# The fix is a content-only copy inside the project: project plugins are staged,
# and UE4 lets a project plugin replace an engine plugin of the same name (see
# FPluginManager::CreatePluginObject), so the /HDRIBackdrop/ mount point and the
# hardcoded asset path stay valid. Dropping the module costs nothing at runtime
# -- it only registered a Place Actors category and an editor icon style.
#
# The copy is generated here rather than committed: the assets are Epic's, and
# anyone building CARLA already has an engine licence.

if ${BUILD_HDRIBACKDROP} ; then

  HDRIBACKDROP_SRC="${UE4_ROOT}/Engine/Plugins/Runtime/HDRIBackdrop"
  HDRIBACKDROP_STAMP="${CARLAUE4_HDRIBACKDROP_FOLDER}/.provisioned-from"

  # The backdrop Blueprint hard-references this one cubemap as its default. The
  # engine plugin ships five 4k samples (~114 MB); the other four are unused
  # here because the lighting comes from Content/Duckietown/HDRI.
  HDRIBACKDROP_DEFAULT_CUBEMAP=approaching_storm_4k.uasset

  if [[ -f "${HDRIBACKDROP_STAMP}" ]] && \
     [[ "$(cat "${HDRIBACKDROP_STAMP}")" == "${HDRIBACKDROP_SRC}" ]] ; then

    log "HDRIBackdrop plugin already provisioned."

  else

    if [[ -z "${UE4_ROOT}" ]] ; then
      fatal_error "UE4_ROOT is not set, cannot provision the HDRIBackdrop plugin."
    fi

    if [[ ! -d "${HDRIBACKDROP_SRC}" ]] ; then
      fatal_error "Cannot find the engine's HDRIBackdrop plugin at
    ${HDRIBACKDROP_SRC}
Expected it to ship with Unreal Engine 4.26. If your engine tree differs, copy
Content/{Blueprints,Materials,Meshes} plus one cubemap into
    ${CARLAUE4_HDRIBACKDROP_FOLDER}/Content
alongside ${CARLA_BUILD_TOOLS_FOLDER}/HDRIBackdrop.uplugin and re-run."
    fi

    if [[ ! -f "${HDRIBACKDROP_SRC}/Content/Textures/${HDRIBACKDROP_DEFAULT_CUBEMAP}" ]] ; then
      fatal_error "Cannot find ${HDRIBACKDROP_DEFAULT_CUBEMAP} in the engine's
HDRIBackdrop plugin. The backdrop Blueprint references it as its default
cubemap, so it must be copied. Check whether the engine renamed it and update
HDRIBACKDROP_DEFAULT_CUBEMAP in this script."
    fi

    log "Provisioning content-only HDRIBackdrop plugin from ${HDRIBACKDROP_SRC}."

    rm -Rf "${CARLAUE4_HDRIBACKDROP_FOLDER}"
    mkdir -p "${CARLAUE4_HDRIBACKDROP_FOLDER}/Content/Textures"
    mkdir -p "${CARLAUE4_HDRIBACKDROP_FOLDER}/Resources"

    cp -r "${HDRIBACKDROP_SRC}/Content/Blueprints" \
          "${HDRIBACKDROP_SRC}/Content/Materials" \
          "${HDRIBACKDROP_SRC}/Content/Meshes" \
          "${CARLAUE4_HDRIBACKDROP_FOLDER}/Content/"

    cp "${HDRIBACKDROP_SRC}/Content/Textures/${HDRIBACKDROP_DEFAULT_CUBEMAP}" \
       "${CARLAUE4_HDRIBACKDROP_FOLDER}/Content/Textures/"

    # Third-party attribution for the cubemaps, kept with the asset.
    if [[ -f "${HDRIBACKDROP_SRC}/Content/Textures/HDRIBackdrop.tps" ]] ; then
      cp "${HDRIBACKDROP_SRC}/Content/Textures/HDRIBackdrop.tps" \
         "${CARLAUE4_HDRIBACKDROP_FOLDER}/Content/Textures/"
    fi

    cp "${HDRIBACKDROP_SRC}/Resources/"*.png "${CARLAUE4_HDRIBACKDROP_FOLDER}/Resources/"

    # Our own descriptor: same plugin name, no Modules section.
    cp "${CARLA_BUILD_TOOLS_FOLDER}/HDRIBackdrop.uplugin" \
       "${CARLAUE4_HDRIBACKDROP_FOLDER}/HDRIBackdrop.uplugin"

    echo "${HDRIBACKDROP_SRC}" > "${HDRIBACKDROP_STAMP}"

  fi

  log "HDRIBackdrop Success!"

fi
