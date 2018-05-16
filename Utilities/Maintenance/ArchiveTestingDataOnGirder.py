#!/usr/bin/env python

import argparse
import girder_client
from girder_client import GirderClient
import os
import fnmatch
import json
import mimetypes
from distutils.version import StrictVersion

if StrictVersion(girder_client.__version__) < StrictVersion("2.0.0"):
    raise Exception("Girder 2.0.0 or newer is required")

class GirderExternalDataCli(GirderClient):
    """
    A command line Python client for interacting with a Girder instance's
    RESTful api, specifically for performing uploads into a Girder instance.
    """
    def __init__(self, apiKey, objectStore):
        """initialization function to create a GirderCli instance, will attempt
        to authenticate with the designated Girder instance.
        """
        GirderClient.__init__(self,
                              apiUrl='https://data.kitware.com/api/v1')
        self.objectStore = objectStore
        self.authenticate(apiKey=apiKey)

    def content_link_upload(self, localFolder, parentId, ext='.sha512',
            parentType='folder', blacklist=['.git', '.ExternalData'],
            reuseExisting=True, dryRun=False):
        """Upload objects corresponding to CMake ExternalData content links.

        This will recursively walk down the tree and find content links ending
        with the specified extension and create a hierarchy on the server under
        the parentId.

        :param ext: Content link file extension.
        :param parentId: id of the parent in Girder or resource path.
        :param parentType: one of (collection,folder,user), default of folder.
        :param reuseExisting: bool whether to accept an existing item of
            the same name in the same location, or create a new one instead.
        :param dryRun: Do not actually upload any content.
        """
        parentId = self._checkResourcePath(parentId)
        localFolder = os.path.normpath(localFolder)
        for entry in os.listdir(localFolder):
            if entry in blacklist:
                print("Ignoring file %s as it is blacklisted" % entry)
                continue
            full_entry = os.path.join(localFolder, entry)
            if os.path.islink(full_entry):
                # os.walk skips symlinks by default
                print("Skipping file %s as it is a symlink" % entry)
                continue
            if os.path.isdir(full_entry):
                self._uploadFolderRecursive(
                    full_entry, parentId, parentType, ext,
                    reuseExisting=reuseExisting, blacklist=blacklist,
                    dryRun=dryRun)

    def _uploadContentLinkItem(self, name, content_link, folder,
            ext='.sha512', parentType='folder', dryRun=False,
            reuseExisting=False):
        """Upload objects corresponding to CMake ExternalData content links.

        This will upload the file with name, *name*, for the content link
        located at *content_link* to the Girder folder, *folder*.

        :param ext: Content link file extension.
        :param parentType: one of (collection,folder,user), default of folder.
        :param reuseExisting: bool whether to accept an existing item of
            the same name in the same location, or create a new one instead.
        :param dryRun: Do not actually upload any content.
        """
        content_link = os.path.normpath(content_link)
        if os.path.isfile(content_link) and \
                fnmatch.fnmatch(content_link, '*' + ext):
            if parentType != 'folder':
                raise Exception(('Attempting to upload an item under a %s.'
                                % parentType) +
                                ' Items can only be added to folders.')
            else:
                with open(content_link, 'r') as fp:
                    hash_value = fp.readline().strip()
                self._uploadAsItem(
                    name,
                    folder['_id'],
                    os.path.join(self.objectStore, hash_value),
                    reuseExisting=reuseExisting,
                    dryRun=dryRun)

    def _uploadFolderRecursive(self, localFolder, parentId, parentType,
                                 ext='.sha512',
                                 reuseExisting=False,
                                 blacklist=[],
                                 dryRun=False):
        """Function to recursively upload a folder and all of its descendants.
        :param localFolder: full path to local folder to be uploaded
        :param parentId: id of parent in Girder,
            where new folder will be added
        :param parentType: one of (collection, folder, user)
        :param leaf_folders_as_items: whether leaf folders should have all
        files uploaded as single items
        :param reuseExisting: boolean indicating whether to accept an existing
        item
        of the same name in the same location, or create a new one instead
        """
        localFolder = os.path.normpath(localFolder)
        filename = os.path.basename(localFolder)
        if filename in blacklist:
            print("Ignoring file %s as it is blacklisted" % filename)
            return

        # Do not add the folder if it does not contain any content links
        has_content_link = False
        for root, dirnames, filenames in os.walk(localFolder):
                for filename in fnmatch.filter(filenames, '*' + ext):
                        has_content_link = True
                        break
        if not has_content_link:
            return

        print('Creating Folder from %s' % localFolder)
        if dryRun:
            # create a dryRun placeholder
            folder = {'_id': 'dryRun'}
        elif localFolder == '.':
            folder = {'_id': parentId}
        else:
            folder = self.loadOrCreateFolder(
                os.path.basename(localFolder), parentId, parentType)

        for entry in sorted(os.listdir(localFolder)):
            if entry in blacklist:
                print("Ignoring file %s as it is blacklisted" % entry)
                continue
            full_entry = os.path.join(localFolder, entry)
            if os.path.islink(full_entry):
                # os.walk skips symlinks by default
                print("Skipping file %s as it is a symlink" % entry)
                continue
            elif os.path.isdir(full_entry):
                # At this point we should have an actual folder, so can
                # pass that as the parentType
                self._uploadFolderRecursive(
                    full_entry, folder['_id'], 'folder',
                    ext, reuseExisting=reuseExisting,
                    blacklist=blacklist, dryRun=dryRun)
            else:
                name = os.path.splitext(entry)[0]
                self._uploadContentLinkItem(name, full_entry, folder,
                        ext=ext, parentType=parentType, dryRun=dryRun,
                        reuseExisting=reuseExisting)

            if not dryRun:
                for callback in self._folderUploadCallbacks:
                    callback(folder, localFolder)


def main():
    parser = argparse.ArgumentParser(
        description='Upload CMake ExternalData content links to Girder')
    parser.add_argument(
        '--dry-run', action='store_true',
        help='will not write anything to Girder, only report on what would '
        'happen')
    parser.add_argument('--api-key', required=True, default=None)
    parser.add_argument('--local-folder', required=False,
                        default=os.path.join(os.path.dirname(__file__), '..',
                            '..'),
                        help='path to local target folder')
    # Default is ITK/ITKTestingData/Nightly
    parser.add_argument('--parent-id', required=False,
                        default='57b673388d777f10f269651c',
                        help='id of Girder parent target')
    parser.add_argument('--object-store', required=True,
                        help='Path to the CMake ExternalData object store')
    parser.add_argument(
        '--no-reuse', action='store_true',
        help='Don\'t reuse existing items of same name at same location')
    args = parser.parse_args()

    reuseExisting = not args.no_reuse
    gc = GirderExternalDataCli(args.api_key,
        objectStore=os.path.join(args.object_store, 'SHA512'))
    gc.content_link_upload(args.local_folder, args.parent_id,
            reuseExisting=reuseExisting, dryRun=args.dry_run)

if __name__ == '__main__':
    main()
