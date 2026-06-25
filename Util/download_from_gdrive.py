#!/usr/bin/env python3

# Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma de
# Barcelona (UAB).
#
# This work is licensed under the terms of the MIT license.
# For a copy, see <https://opensource.org/licenses/MIT>.

"""Download big files from Google Drive."""

import argparse
import shutil
import sys

import requests


def sizeof_fmt(num, suffix='B'):
    # https://stackoverflow.com/a/1094933/5308925
    for unit in ['', 'K', 'M', 'G', 'T', 'P', 'E', 'Z']:
        if abs(num) < 1000.0:
            return "%3.2f%s%s" % (num, unit, suffix)
        num /= 1000.0
    return "%.2f%s%s" % (num, 'Yi', suffix)


def print_status(destination, progress):
    message = "Downloading %s...    %s" % (destination, sizeof_fmt(progress))
    empty_space = shutil.get_terminal_size((80, 20)).columns - len(message)
    sys.stdout.write('\r' + message + empty_space * ' ')
    sys.stdout.flush()


def download_file_from_google_drive(id, destination):
    import re

    def save_response_content(response, destination):
        chunk_size = 32768
        written_size = 0

        with open(destination, "wb") as f:
            for chunk in response.iter_content(chunk_size):
                if chunk:  # filter out keep-alive new chunks
                    f.write(chunk)
                    written_size += chunk_size
                    print_status(destination, written_size)
        print('Done.')

    session = requests.Session()

    # Initial request — may redirect to a virus-scan warning page for large files
    url = "https://drive.google.com/uc?export=download"
    response = session.get(url, params={'id': id}, stream=True)

    # Check for the large-file confirmation page (Google's current flow uses
    # drive.usercontent.google.com with a uuid parameter)
    content_type = response.headers.get('Content-Type', '')
    if 'text/html' in content_type:
        html = response.content.decode('utf-8', errors='replace')
        uuid_match = re.search(r'name="uuid"\s+value="([^"]+)"', html)
        if uuid_match:
            uuid = uuid_match.group(1)
            download_url = "https://drive.usercontent.google.com/download"
            response = session.get(
                download_url,
                params={'id': id, 'export': 'download', 'confirm': 't', 'uuid': uuid},
                stream=True)
        else:
            # Fallback: older cookie-based confirmation
            token = next(
                (v for k, v in response.cookies.items() if k.startswith('download_warning')),
                None)
            if token:
                response = session.get(url, params={'id': id, 'confirm': token}, stream=True)

    save_response_content(response, destination)


if __name__ == "__main__":

    try:

        argparser = argparse.ArgumentParser(description=__doc__)
        argparser.add_argument(
            'id',
            help='Google Drive\'s file id')
        argparser.add_argument(
            'destination',
            help='destination file path')
        args = argparser.parse_args()

        download_file_from_google_drive(args.id, args.destination)

    except KeyboardInterrupt:
        print('\nCancelled by user. Bye!')
