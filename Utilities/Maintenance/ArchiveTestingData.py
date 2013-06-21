#!/usr/bin/env python
#==========================================================================
#
#   Copyright Insight Software Consortium
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          http://www.apache.org/licenses/LICENSE-2.0.txt
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#==========================================================================*/

description = """
Upload all the ExternalData files to the Midas server.

The files corresponding to all the ExternalData content links
in the source tree are archived on the Midas server using the
local ExternalData object store.

Requires pydas: https://github.com/midasplatform/pydas
"""

import argparse
import fnmatch
import os
import sys

import pydas

def connect_to_midas(email=None, api_key=None):
    midas_url = 'http://midas3.kitware.com/midas/'
    if not api_key:
        print('Please enter your login information for ' + midas_url)
        pydas.login(url=midas_url, email=email)
    else:
        pydas.login(url=midas_url, email=email, api_key=api_key)
    session = pydas.session
    communicator = session.communicator
    return session, communicator


def upload_to_midas(content_link, externaldata_object_store,
        repository_dir,
        session, communicator):
    # get the MD5 checksum
    print('Uploading ' + content_link + ' ...')
    with open(content_link, 'r') as fp:
        md5hash = fp.readline().strip()
    print('Checksum: ' + md5hash)

    # upload to Midas
    def get_child_folder(parent, child_name):
        children = communicator.folder_children(session.token, parent['folder_id'])
        for folder in children['folders']:
            if folder['name'] == child_name:
                return folder
        return None
    vtk_community = communicator.get_community_by_name('VTK')
    vtk_public = get_child_folder(vtk_community, 'Public')
    vtk_folder = get_child_folder(vtk_public, 'VTK')

    # Where to place the file in Midas.  Mirror the source tree location.
    folders = os.path.dirname(content_link)
    if repository_dir != '':
        folders = folders[folders.find(repository_dir)+len(repository_dir)+1:]
    folders = folders.split(os.path.sep)

    current_folder = vtk_folder
    for folder in folders:
        child_folder = get_child_folder(current_folder, folder)
        if child_folder is None:
            print('Creating folder: ' + folder)
            current_folder = communicator.create_folder(session.token,
                    folder,
                    current_folder['folder_id'])
        else:
            current_folder = child_folder

    # get the existing or create a new item to hold the file
    item_name = os.path.basename(content_link[:-4])
    item_id = None
    current_folder_children = communicator.folder_children(session.token,
            current_folder['folder_id'])
    if current_folder_children.has_key('items'):
        for item in current_folder_children['items']:
            if item['name'] == item_name:
                item_id = item['item_id']
                break

    if item_id is None:
        new_item = communicator.create_item(session.token, item_name,
                current_folder['folder_id'])
        item_id = new_item['item_id']

    object_store = os.path.join(externaldata_object_store, 'MD5', md5hash)
    if not os.path.exists(object_store):
        sys.stderr.write('Could not find the expected object store.\n')
        sys.exit(1)

    upload_token = communicator.generate_upload_token(session.token,
            item_id,
            item_name,
            md5hash)
    if upload_token != "":
        communicator.perform_upload(upload_token,
                item_name,
                item_id=item_id,
                revision='head',
                filepath=object_store)


def run(vtk_source_dir, externaldata_object_store,
        email=None, api_key=None):
    session, communicator = connect_to_midas(email, api_key)

    md5files = []
    for root, dirnames, filenames in os.walk(vtk_source_dir):
        for filename in fnmatch.filter(filenames, '*.md5'):
            md5files.append(os.path.join(root, filename))

    # Find the location of top level directory in the repository.
    repository_dir = None
    example_path = os.path.normpath(os.path.dirname(md5files[0]))
    potential_path = example_path
    previous_path = None
    while previous_path != '':
        dir = os.path.join(potential_path, '.git')
        if os.path.exists(dir):
            repository_dir = potential_path
            break
        previous_path = potential_path
        potential_path = os.path.split(potential_path)[0]
    if repository_dir == None:
        sys.stderr.write('Could not find VTK repository directory.\n')
        sys.exit(1)

    for content_link in md5files:
        src_examples = os.path.join('src', 'Examples')
        if content_link.find(src_examples) == -1:
            uncategorized = True
        else:
            uncategorized = False

        if uncategorized:
            folders = ['Uncategorized']
        else:
            group_module_path = content_link.split(src_examples)[1][1:]
            hiercharchy = group_module_path.split(os.path.sep)
            group = hiercharchy[0]
            module = hiercharchy[1]
            folders = [group, module]

        upload_to_midas(content_link, externaldata_object_store, repository_dir,
                session, communicator)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('--api-key-file', '-k', type=argparse.FileType('r'),
            help="A file that contains your Midas user's API key.")
    parser.add_argument('--email', '-e',
            help="Email address associated with your Midas account.")
    parser.add_argument('vtk_source_dir',
            help='Path to the VTK source tree.')
    parser.add_argument('externaldata_object_store',
            help='Path to the ExternalData object store, e.g. ' \
            + 'ExternalData/Objects/ in a build tree.')
    args = parser.parse_args()

    if args.api_key_file:
        api_key = args.api_key_file.readline()
        api_key = api_key.strip()
    else:
        api_key = None

    run(args.vtk_source_dir, args.externaldata_object_store,
        email=args.email, api_key=api_key)
