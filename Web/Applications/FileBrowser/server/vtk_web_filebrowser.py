r"""
    This module is a VTK Web server application.
    The following command line illustrate how to use it::

        $ vtkpython .../vtk_web_filebrowser.py --data-dir /.../server_directory_to_share

    Any VTK Web executable script come with a set of standard arguments that
    can be overriden if need be::
        --host localhost
             Interface on which the HTTP server will listen on.

        --port 8080
             Port number on which the HTTP server will listen to.

        --content /path-to-web-content/
             Directory that you want to server as static web content.
             By default, this variable is empty which mean that we rely on another server
             to deliver the static content and the current process only focus on the
             WebSocket connectivity of clients.

        --authKey vtk-secret
             Secret key that should be provided by the client to allow it to make any
             WebSocket communication. The client will assume if none is given that the
             server expect "vtk-secret" as secret key.
"""

# import to process args
import sys
import os

# import vtk modules.
import vtk
from vtk.web import protocols, server
from vtk.web import wamp as vtk_wamp

try:
    import argparse
except ImportError:
    # since  Python 2.6 and earlier don't have argparse, we simply provide
    # the source for the same as _argparse and we use it instead.
    import _argparse as argparse

# =============================================================================
# Create custom File Opener class to handle clients requests
# =============================================================================

class _WebFileBrowser(vtk_wamp.ServerProtocol):

    # Application configuration
    authKey = "vtkweb-secret"
    basedir = ""

    def initialize(self):
        # Bring used components
        self.registerVtkWebProtocol(protocols.vtkWebFileBrowser(_WebFileBrowser.basedir, "Home"))

        # Update authentication key to use
        self.updateSecret(_WebFileBrowser.authKey)

# =============================================================================
# Main: Parse args and start server
# =============================================================================

if __name__ == "__main__":
    # Create argument parser
    parser = argparse.ArgumentParser(description="VTK/Web FileBrowser web-application")

    # Add default arguments
    server.add_arguments(parser)

    # Add local arguments
    parser.add_argument("--data-dir", help="Base directory to list", dest="basedir", default=".")


    # Exctract arguments
    args = parser.parse_args()

    # Configure our current application
    _WebFileBrowser.authKey = args.authKey
    _WebFileBrowser.basedir = args.basedir

    # Start server
    server.start_webserver(options=args, protocol=_WebFileBrowser)
