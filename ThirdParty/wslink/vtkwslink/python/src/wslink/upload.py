r"""
    This module provides a server side mechanism to support uploading files
    to the server.

    $ python file_upload_handler.py --upload-directory .../path-to-upload-directory  --upload-port port

    --upload-directory
         Path to directory where uploaded files should go.  If this argument is not
         specified, then uploaded files will end up in the directory from which python
         is run.
    --port
         Port on which upload server should listen.
"""

from __future__ import absolute_import, division, print_function

# import os to concatenate paths in a system independent way
import os

# import twisted as the webserver
from twisted.web.server import Site
from twisted.web.resource import Resource
from twisted.internet import reactor

# import to process command line arguments
try:
    import argparse
except ImportError:
    # since  Python 2.6 and earlier don't have argparse, we simply provide
    # the source for the same as _argparse and we use it instead.
    from vtk.util import _argparse as argparse


def add_arguments(parser) :
    """
    Extract arguments needed by the upload server.

      --upload-directory => Full path to location where files should be uploaded
    """
    parser.add_argument("--upload-directory",
                        default=os.getcwd(),
                        help="path to root upload directory",
                        dest="uploadPath")


class UploadPage(Resource) :
    isLeaf = True
    uploadDirectory = os.getcwd()

    def __init__(self, uploadDir) :
        self.uploadDirectory = uploadDir
        Resource.__init__(self)

    def render_POST(self, request):
        for key in request.args :
            filename = os.path.join(self.uploadDirectory, key)
            with open(filename, 'w') as nfd:
                for lineText in request.args[key] :
                    nfd.write(lineText)

        request.setHeader('Access-Control-Allow-Origin', '*')
        return '<html><body>Your post was received</body></html>'


if __name__ == "__main__":
    # Create argument parser
    parser = argparse.ArgumentParser(description="File Upload Server")
    add_arguments(parser)
    parser.add_argument("--port",
                        default=8081,
                        help="port for upload server to listen on",
                        dest="port")
    args = parser.parse_args()

    # Now start the twisted server
    resource = UploadPage(args.uploadPath)
    factory = Site(resource)
    reactor.listenTCP(args.port, factory)
    reactor.run()
