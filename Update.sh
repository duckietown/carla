#! /bin/bash

################################################################################
# Updates CARLA and Duckietown content.
################################################################################

set -e

DOC_STRING="Update CARLA content to the latest version, to be run after 'git pull'."

USAGE_STRING="Usage: $0 [-h|--help] [-s|--skip-download] [-d|--skip-duckietown]"

# ==============================================================================
# -- Parse arguments -----------------------------------------------------------
# ==============================================================================

SKIP_DOWNLOAD=false
SKIP_DUCKIETOWN=false

OPTS=`getopt -o hsd --long help,skip-download,skip-duckietown -n 'parse-options' -- "$@"`

if [ $? != 0 ] ; then echo "$USAGE_STRING" ; exit 2 ; fi

eval set -- "$OPTS"

while true; do
  case "$1" in
    -s | --skip-download )
      SKIP_DOWNLOAD=true;
      shift ;;
    -d | --skip-duckietown )
      SKIP_DUCKIETOWN=true;
      shift ;;
    -h | --help )
      echo "$DOC_STRING"
      echo "$USAGE_STRING"
      exit 1
      ;;
    * )
      break ;;
  esac
done

# ==============================================================================
# -- Set up environment --------------------------------------------------------
# ==============================================================================

MAX_PARALLELL_DOWNLOADS=16
MAX_CONNECTIONS_PER_SERVER=16

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd "$SCRIPT_DIR" >/dev/null

CONTENT_FOLDER="${SCRIPT_DIR}/Unreal/CarlaUE4/Content/Carla"

CONTENT_ID=$(tac $SCRIPT_DIR/Util/ContentVersions.txt | egrep -m 1 . | rev | cut -d' ' -f1 | rev)
CONTENT_LINK=https://carla-assets.s3.us-east-005.backblazeb2.com/${CONTENT_ID}.tar.gz

VERSION_FILE="${CONTENT_FOLDER}/.version"

DUCKIETOWN_CONTENT_FOLDER="${SCRIPT_DIR}/Unreal/CarlaUE4/Content/Duckietown"
DUCKIETOWN_CONTENT_ID=$(tac $SCRIPT_DIR/Util/DuckietownContentVersions.txt | egrep -m 1 . | rev | cut -d' ' -f1 | rev)
DUCKIETOWN_VERSION_FILE="${DUCKIETOWN_CONTENT_FOLDER}/.version"

function download_content {
  if [[ -d "$CONTENT_FOLDER" ]]; then
    echo "Backing up existing Content..."
    mv -v "$CONTENT_FOLDER" "${CONTENT_FOLDER}_$(date +%Y%m%d%H%M%S)"
  fi
  mkdir -p "$CONTENT_FOLDER"
  mkdir -p Content
  if hash aria2c 2>/dev/null; then
    echo -e "${CONTENT_LINK}\n\tout=Content.tar.gz" > .aria2c.input
    aria2c -j${MAX_PARALLELL_DOWNLOADS} -x${MAX_CONNECTIONS_PER_SERVER} --input-file=.aria2c.input
    rm -f .aria2c.input
  else
    wget -c ${CONTENT_LINK} -O Content.tar.gz
  fi
  tar -xvzf Content.tar.gz -C Content
  rm Content.tar.gz
  mv Content/* "$CONTENT_FOLDER"
  rm -rf Content
  echo "$CONTENT_ID" > "$VERSION_FILE"
  echo "Content updated successfully."
}

# ==============================================================================
# -- Download Content if necessary ---------------------------------------------
# ==============================================================================

if $SKIP_DOWNLOAD ; then
  echo "Skipping 'Content' update. Please manually download the package from"
  echo
  echo "  ${CONTENT_LINK}"
  echo
  echo "and extract it under Unreal/CarlaUE4/Content/Carla."
elif [[ -d "$CONTENT_FOLDER/.git" ]]; then
  echo "Using git version of 'Content', skipping update."
elif [[ -f "$CONTENT_FOLDER/.version" ]]; then
  if [ "$CONTENT_ID" == `cat $VERSION_FILE` ]; then
    echo "Content is up-to-date."
  else
    download_content
  fi
else
  download_content
fi

# ==============================================================================
# -- Download Duckietown Content if necessary ----------------------------------
# ==============================================================================

function download_duckietown_content {
  if [[ -d "$DUCKIETOWN_CONTENT_FOLDER" ]]; then
    echo "Backing up existing Duckietown Content..."
    mv -v "$DUCKIETOWN_CONTENT_FOLDER" "${DUCKIETOWN_CONTENT_FOLDER}_$(date +%Y%m%d%H%M%S)"
  fi
  mkdir -p "$DUCKIETOWN_CONTENT_FOLDER"
  echo "Downloading Duckietown content from Google Drive (id: ${DUCKIETOWN_CONTENT_ID})..."
  python3 ${SCRIPT_DIR}/Util/download_from_gdrive.py ${DUCKIETOWN_CONTENT_ID} DuckietownContent.tar.gz
  echo "Extracting Duckietown content..."
  tar -xzf DuckietownContent.tar.gz -C Unreal/CarlaUE4/Content
  rm DuckietownContent.tar.gz
  echo "$DUCKIETOWN_CONTENT_ID" > "$DUCKIETOWN_VERSION_FILE"
  echo "Duckietown content updated successfully."
}

if $SKIP_DUCKIETOWN ; then
  echo "Skipping Duckietown content update."
elif [[ -f "$DUCKIETOWN_VERSION_FILE" ]]; then
  if [ "$DUCKIETOWN_CONTENT_ID" == `cat $DUCKIETOWN_VERSION_FILE` ]; then
    echo "Duckietown content is up-to-date."
  else
    download_duckietown_content
  fi
else
  download_duckietown_content
fi

popd >/dev/null
