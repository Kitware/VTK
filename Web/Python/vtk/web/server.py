r"""server is a module that enables using VTK through a web-server. This
module implments a Wamp v2 Server Protocol that provides the core RPC-API needed to
place interactive visualization in web-pages. Developers can extent
ServerProtocol to provide additional RPC callbacks for their web-applications.

This module can be used as the entry point to the application. In that case, it
sets up a Twisted web-server that can generate visualizations as well as serve
web-pages are determines by the command line arguments passed in.
Use "--help" to list the supported arguments.

"""

import sys, logging

from vtk.web import testing
from vtk.web import upload
from vtk.web import wamp as vtk_wamp

from autobahn.wamp              import types

from autobahn.twisted.resource  import WebSocketResource
from autobahn.twisted.websocket import listenWS
from autobahn.twisted.longpoll  import WampLongPollResource

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
    Add arguments processed know to this module. parser must be
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
    parser.add_argument("-a", "--authKey", default='vtkweb-secret',
        help="Authentication key for clients to connect to the WebSocket.")
    parser.add_argument("-f", "--force-flush", default=False, help="If provided, this option will force additional padding content to the output.  Useful when application is triggered by a session manager.", dest="forceFlush", action='store_true')
    parser.add_argument("-k", "--sslKey", type=str, default="",
        help="SSL key.  Use this and --sslCert to start the server on https.")
    parser.add_argument("-j", "--sslCert", type=str, default="",
        help="SSL certificate.  Use this and --sslKey to start the server on https.")
    parser.add_argument("-ws", "--ws-endpoint", type=str, default="ws", dest='ws',
        help="Specify WebSocket endpoint. (e.g. foo/bar/ws, Default: ws)")
    parser.add_argument("-lp", "--lp-endpoint", type=str, default="lp", dest='lp',
        help="Specify LongPoll endpoint. (e.g. foo/bar/lp, Default: lp)")
    parser.add_argument("-hp", "--http-endpoint", default='hp', dest='hp',
        help="Specify an HTTP endpoint.  (e.g. foo/bar/hp, Default: hp)")
    parser.add_argument("--no-ws-endpoint", action="store_true", dest='nows',
        help="If provided, disables the websocket endpoint")
    parser.add_argument("--no-lp-endpoint", action="store_true", dest='nolp',
        help="If provided, disables the longpoll endpoint")

    # Hook to extract any testing arguments we need
    testing.add_arguments(parser)

    # Extract any necessary upload server arguments
    upload.add_arguments(parser)

    return parser

# =============================================================================
# Parse arguments and start webserver
# =============================================================================

def start(argv=None,
        protocol=vtk_wamp.ServerProtocol,
        description="VTK/Web web-server based on Twisted."):
    """
    Sets up the web-server using with __name__ == '__main__'. This can also be
    called directly. Pass the opational protocol to override the protocol used.
    Default is ServerProtocol.
    """
    try:
        import argparse
    except ImportError:
        # since  Python 2.6 and earlier don't have argparse, we simply provide
        # the source for the same as _argparse and we use it instead.
        import _argparse as argparse

    parser = argparse.ArgumentParser(description=description)
    add_arguments(parser)
    args = parser.parse_args(argv)
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
    # Handle complex endpoint
    fullpath = path.split('/')
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

def start_webserver(options, protocol=vtk_wamp.ServerProtocol, disableLogging=False):
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
        log.startLogging(sys.stdout)

    contextFactory = None

    use_SSL = False
    if options.sslKey and options.sslCert:
      use_SSL = True
      wsProtocol = "wss"
      from twisted.internet import ssl
      contextFactory = ssl.DefaultOpenSSLContextFactory(options.sslKey, options.sslCert)
    else:
      wsProtocol = "ws"

    # Create WAMP router factory
    from autobahn.twisted.wamp import RouterFactory
    router_factory = RouterFactory()

    # create a user DB
    authdb = vtk_wamp.AuthDb()

    # create a WAMP router session factory
    from autobahn.twisted.wamp import RouterSessionFactory
    session_factory = RouterSessionFactory(router_factory)
    session_factory.session = vtk_wamp.CustomWampCraRouterSession
    session_factory.authdb = authdb

    # Create ApplicationSession and register protocols
    appSession = protocol(types.ComponentConfig(realm = "vtkweb"))
    appSession.setAuthDB(authdb)
    session_factory.add(appSession)

    # create a WAMP-over-WebSocket transport server factory
    transport_factory = vtk_wamp.TimeoutWampWebSocketServerFactory(session_factory, \
           url        = "%s://%s:%d" % (wsProtocol, options.host, options.port),    \
           debug      = options.debug,                                              \
           debug_wamp = options.debug,                                              \
           timeout    = options.timeout )

    root = Resource()

    # Do we serve static content or just websocket ?
    if len(options.content) > 0:
        # Static HTTP + WebSocket
        root = File(options.content)

    # Handle possibly complex ws endpoint
    if not options.nows:
        wsResource = WebSocketResource(transport_factory)
        handle_complex_resource_path(options.ws, root, wsResource)

    # Handle possibly complex lp endpoint
    if not options.nolp:
        lpResource = WampLongPollResource(session_factory)
        handle_complex_resource_path(options.lp, root, lpResource)

    if options.uploadPath != None :
        from upload import UploadPage
        uploadResource = UploadPage(options.uploadPath)
        root.putChild("upload", uploadResource)

    site = Site(root)

    if use_SSL:
      reactor.listenSSL(options.port, site, contextFactory)
    else:
      reactor.listenTCP(options.port, site)

    # Work around to force the output buffer to be flushed
    # This allow the process launcher to parse the output and
    # wait for "Start factory" to know that the WebServer
    # is running.
    if options.forceFlush :
        for i in range(200):
            log.msg("+"*80, logLevel=logging.CRITICAL)

    # Initialize testing: checks if we're doing a test and sets it up
    testing.initialize(options, reactor, stop_webserver)

    # Start the reactor
    if options.nosignalhandlers:
        reactor.run(installSignalHandlers=0)
    else:
        reactor.run()

    # Give the testing module a chance to finalize, if necessary
    testing.finalize()


# =============================================================================
# Start httpserver
# =============================================================================

def start_httpserver(options, protocol=vtk_wamp.ServerProtocol, disableLogging=False):
    """
    Starts an http-only server with the given protocol.  The options argument should
    contain 'host', 'port', 'urlRegex', and optionally 'content'.
    """
    from twisted.web import server, http
    from twisted.internet import reactor
    from twisted.web.static import File

    host = options.host
    port = options.port
    contentDir = options.content
    rootPath = options.hp

    # Initialize web resource
    web_resource = File(contentDir) if contentDir else resource.Resource()

    # Add the rpc method server
    handle_complex_resource_path(rootPath, web_resource, vtk_wamp.HttpRpcResource(protocol(None), rootPath))

    site = server.Site(web_resource)
    reactor.listenTCP(port, site, interface=host)

    reactor.run()

if __name__ == "__main__":
    start()
