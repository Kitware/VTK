r"""server is a module that enables using python through a web-server.

This module can be used as the entry point to the application. In that case, it
sets up a Twisted web-server.
web-pages are determines by the command line arguments passed in.
Use "--help" to list the supported arguments.

"""

from __future__ import absolute_import, division, print_function

import logging

# from vtk.web import testing
from wslink import upload
from wslink import websocket as wsl

from autobahn.twisted.resource  import WebSocketResource
from autobahn.twisted.websocket import listenWS, WebSocketServerFactory

from twisted.web                import resource
from twisted.web.resource       import Resource
from twisted.internet           import reactor
from twisted.internet.defer     import inlineCallbacks
from twisted.internet.endpoints import serverFromString
from twisted.python             import log

# =============================================================================
# Setup default arguments to be parsed
#   -s, --nosignalhandlers
#   -d, --debug
#   -p, --port     8080
#   -t, --timeout  300 (seconds)
#   -c, --content  '/www'  (No content means WebSocket only)
#   -a, --authKey  vtkweb-secret
# =============================================================================

def add_arguments(parser):
    """
    Add arguments known to this module. parser must be
    argparse.ArgumentParser instance.
    """
    import os
    parser.add_argument("-d", "--debug",
        help="log debugging messages to stdout",
        action="store_true")
    parser.add_argument("-s", "--nosignalhandlers",
        help="Prevent Twisted to install the signal handlers so it can be started inside a thread.",
        action="store_true")
    parser.add_argument("-i", "--host", type=str, default='localhost',
        help="the interface for the web-server to listen on (default: localhost)")
    parser.add_argument("-p", "--port", type=int, default=8080,
        help="port number for the web-server to listen on (default: 8080)")
    parser.add_argument("-t", "--timeout", type=int, default=300,
        help="timeout for reaping process on idle in seconds (default: 300s)")
    parser.add_argument("-c", "--content", default='',
        help="root for web-pages to serve (default: none)")
    parser.add_argument("-a", "--authKey", default='wslink-secret',
        help="Authentication key for clients to connect to the WebSocket.")
    parser.add_argument("-f", "--force-flush", default=False, help="If provided, this option will force additional padding content to the output.  Useful when application is triggered by a session manager.", dest="forceFlush", action='store_true')
    parser.add_argument("-k", "--sslKey", type=str, default="",
        help="SSL key.  Use this and --sslCert to start the server on https.")
    parser.add_argument("-j", "--sslCert", type=str, default="",
        help="SSL certificate.  Use this and --sslKey to start the server on https.")
    parser.add_argument("-ws", "--ws-endpoint", type=str, default="ws", dest='ws',
        help="Specify WebSocket endpoint. (e.g. foo/bar/ws, Default: ws)")
    parser.add_argument("--no-ws-endpoint", action="store_true", dest='nows',
        help="If provided, disables the websocket endpoint")
    parser.add_argument("--fs-endpoints", default='', dest='fsEndpoints',
        help="add another fs location to a specific endpoint (i.e: data=/Users/seb/Download|images=/Users/seb/Pictures)")

    # Hook to extract any testing arguments we need
    # testing.add_arguments(parser)

    # Extract any necessary upload server arguments
    upload.add_arguments(parser)

    return parser

# =============================================================================
# Parse arguments and start webserver
# =============================================================================

def start(argv=None,
        protocol=wsl.ServerProtocol,
        description="wslink web-server based on Twisted."):
    """
    Sets up the web-server using with __name__ == '__main__'. This can also be
    called directly. Pass the optional protocol to override the protocol used.
    Default is ServerProtocol.
    """
    try:
        import argparse
    except ImportError:
        # since  Python 2.6 and earlier don't have argparse, we simply provide
        # the source for the same as _argparse and we use it instead.
        from vtk.util import _argparse as argparse

    parser = argparse.ArgumentParser(description=description)
    add_arguments(parser)
    args = parser.parse_args(argv)
    # configure protocol, if available
    try:
        protocol.configure(args)
    except AttributeError:
        pass

    start_webserver(options=args, protocol=protocol)


# =============================================================================
# Stop webserver
# =============================================================================
def stop_webserver() :
    reactor.callFromThread(reactor.stop)

# =============================================================================
# Convenience method to build a resource sub tree to reflect a desired path.
# =============================================================================
def handle_complex_resource_path(path, root, resource):
    # Handle complex endpoint. Twisted expects byte-type URIs.
    fullpath = path.encode('utf-8').split(b'/')
    parent_path_item_resource = root
    for path_item in fullpath:
        if path_item == fullpath[-1]:
            parent_path_item_resource.putChild(path_item, resource)
        else:
            new_resource = Resource()
            parent_path_item_resource.putChild(path_item, new_resource)
            parent_path_item_resource = new_resource


# =============================================================================
# Start webserver
# =============================================================================

def start_webserver(options, protocol=wsl.ServerProtocol, disableLogging=False):
    """
    Starts the web-server with the given protocol. Options must be an object
    with the following members:
        options.host : the interface for the web-server to listen on
        options.port : port number for the web-server to listen on
        options.timeout : timeout for reaping process on idle in seconds
        options.content : root for web-pages to serve.
    """
    from twisted.internet import reactor
    from twisted.web.server import Site
    from twisted.web.static import File
    import sys

    if not disableLogging:
        # redirect twisted logs to python standard logging.
        observer = log.PythonLoggingObserver()
        observer.start()
        # log.startLogging(sys.stdout)
        # Set logging level.
        if (options.debug): logging.basicConfig(level=logging.DEBUG)

    contextFactory = None

    use_SSL = False
    if options.sslKey and options.sslCert:
      use_SSL = True
      wsProtocol = "wss"
      from twisted.internet import ssl
      contextFactory = ssl.DefaultOpenSSLContextFactory(options.sslKey, options.sslCert)
    else:
      wsProtocol = "ws"

    # Create default or custom ServerProtocol
    wslinkServer = protocol()

    # create a wslink-over-WebSocket transport server factory
    transport_factory = wsl.TimeoutWebSocketServerFactory(\
           url        = "%s://%s:%d" % (wsProtocol, options.host, options.port),    \
           timeout    = options.timeout )
    transport_factory.protocol = wsl.WslinkWebSocketServerProtocol
    transport_factory.setServerProtocol(wslinkServer)

    root = Resource()

    # Do we serve static content or just websocket ?
    if len(options.content) > 0:
        # Static HTTP + WebSocket
        root = File(options.content)

    # Handle possibly complex ws endpoint
    if not options.nows:
        wsResource = WebSocketResource(transport_factory)
        handle_complex_resource_path(options.ws, root, wsResource)

    if options.uploadPath != None :
        from wslink.upload import UploadPage
        uploadResource = UploadPage(options.uploadPath)
        root.putChild("upload", uploadResource)

    if len(options.fsEndpoints) > 3:
        for fsResourceInfo in options.fsEndpoints.split('|'):
            infoSplit = fsResourceInfo.split('=')
            handle_complex_resource_path(infoSplit[0], root, File(infoSplit[1]))

    site = Site(root)

    if use_SSL:
      reactor.listenSSL(options.port, site, contextFactory)
    else:
      reactor.listenTCP(options.port, site)

    # flush ready line
    sys.stdout.flush()

    # Work around to force the output buffer to be flushed
    # This allow the process launcher to parse the output and
    # wait for "Start factory" to know that the WebServer
    # is running.
    if options.forceFlush :
        for i in range(200):
            log.msg("+"*80, logLevel=logging.CRITICAL)

    # Initialize testing: checks if we're doing a test and sets it up
    # testing.initialize(options, reactor, stop_webserver)

    # Start the reactor
    if options.nosignalhandlers:
        reactor.run(installSignalHandlers=0)
    else:
        reactor.run()

    # Give the testing module a chance to finalize, if necessary
    # testing.finalize()


if __name__ == "__main__":
    start()
