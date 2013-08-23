r"""wamp is a module that provide classes that extend any
WAMP related class for the purpose of vtkWeb.

"""

import string
import random
import types
import logging

from threading import Timer

from twisted.python     import log
from twisted.internet   import reactor

from autobahn.resource  import WebSocketResource
from autobahn.websocket import listenWS
from autobahn.wamp      import exportRpc, WampServerProtocol, WampCraProtocol, \
                               WampCraServerProtocol, WampServerFactory
try:
    from vtkWebCore import vtkWebApplication
except:
    from vtkWebCorePython import vtkWebApplication

# =============================================================================
salt = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(32))
application = None
# =============================================================================
#
# Base class for vtkWeb WampServerProtocol
#
# =============================================================================

class ServerProtocol(WampCraServerProtocol):
    """
    Defines the core server protocol for vtkWeb. Adds support to
    marshall/unmarshall RPC callbacks that involve ServerManager proxies as
    arguments or return values.

    Applications typically don't use this class directly, since it doesn't
    register any RPC callbacks that are required for basic web-applications with
    interactive visualizations. For that, use vtkWebServerProtocol.
    """

    AUTHEXTRA = {'salt': salt, 'keylen': 32, 'iterations': 1000}
    SECRETS   = {'vtkweb': WampCraProtocol.deriveKey("vtkweb-secret", AUTHEXTRA)}

    def __init__(self):
        self.vtkWebProtocols = []
        self.Application = self.initApplication()
        self.initialize()

    def initialize(self):
        """
        Let the sub class define what they need to do to properly initialize
        themselves.
        """
        pass

    def initApplication(self):
        """
        Let subclass optionally initialize a custom application in lieu
        of the default vtkWebApplication.
        """
        global application
        if not application:
            application = vtkWebApplication()
        return application

    def getAuthPermissions(self, authKey, authExtra):
        return {'permissions': 'all', 'authextra': self.AUTHEXTRA}

    def updateSecret(self, newSecret):
        self.SECRETS['vtkweb'] = WampCraProtocol.deriveKey(newSecret, self.AUTHEXTRA)

    def getAuthSecret(self, authKey):
        ## return the auth secret for the given auth key or None when the auth key
        ## does not exist
        secret = self.SECRETS.get(authKey, None)
        return secret

    def onAuthenticated(self, authKey, perms):
        ## register RPC endpoints
        if authKey is not None:
            self.registerForPubSub("http://vtk.org/event#", True)
            self.registerForRpc(self, "http://vtk.org/vtk#")
            for protocol in self.vtkWebProtocols:
                self.registerForRpc(protocol, "http://vtk.org/vtk#")

    def setApplication(self, application):
        self.Application = application

    def registerVtkWebProtocol(self, protocol):
        protocol.setApplication(self.Application)
        self.vtkWebProtocols.append(protocol)

    def getVtkWebProtocols(self):
        return self.vtkWebProtocols

    def onAfterCallSuccess(self, result, callid):
        """
        Callback fired after executing incoming RPC with success.

        The default implementation will just return `result` to the client.

        :param result: Result returned for executing the incoming RPC.
        :type result: Anything returned by the user code for the endpoint.
        :param callid: WAMP call ID for incoming RPC.
        :type callid: str
        :returns obj -- Result send back to client.
        """
        return self.marshall(result)

    def onBeforeCall(self, callid, uri, args, isRegistered):
        """
        Callback fired before executing incoming RPC. This can be used for
        logging, statistics tracking or redirecting RPCs or argument mangling i.e.

        The default implementation just returns the incoming URI/args.

        :param uri: RPC endpoint URI (fully-qualified).
        :type uri: str
        :param args: RPC arguments array.
        :type args: list
        :param isRegistered: True, iff RPC endpoint URI is registered in this session.
        :type isRegistered: bool
        :returns pair -- Must return URI/Args pair.
        """
        return uri, self.unmarshall(args)

    def marshall(self, argument):
        return argument

    def unmarshall(self, argument):
        """
        Demarshalls the "argument".
        """
        if isinstance(argument, types.ListType):
            # for lists, unmarshall each argument in the list.
            result = []
            for arg in argument:
                arg = self.unmarshall(arg)
                result.append(arg)
            return result
        return argument

    def onConnect(self, connection_request):
        """
        Callback  fired during WebSocket opening handshake when new WebSocket
        client connection is about to be established.

        Call the factory to increment the connection count.
        """
        try:
            self.factory.on_connect()
        except AttributeError:
            pass
        return WampCraServerProtocol.onConnect(self, connection_request)

    def connectionLost(self, reason):
        """
        Called by Twisted when established TCP connection from client was lost.

        Call the factory to decrement the connection count and start a reaper if
        necessary.
        """
        try:
            self.factory.connection_lost()
        except AttributeError:
            pass
        WampCraServerProtocol.connectionLost(self, reason)

    @exportRpc("exit")
    def exit(self):
        """RPC callback to exit"""
        reactor.stop()

# =============================================================================
#
# Base class for vtkWeb WampServerFactory
#
# =============================================================================

class ReapingWampServerFactory(WampServerFactory):
    """
    ReapingWampServerFactory is WampServerFactory subclass that adds support to
    close the web-server after a timeout when the last connected client drops.

    Currently, the protocol must call on_connect() and connection_lost() methods
    to notify the factory that the connection was started/closed.

    If the connection count drops to zero, then the reap timer
    is started which will end the process if no other connections are made in
    the timeout interval.
    """

    def __init__(self, url, debugWamp, timeout):
        self._reaper = None
        self._connection_count = 0
        self._timeout = timeout
        WampServerFactory.__init__(self, url, debugWamp)

    def startFactory(self):
        if not self._reaper:
            self._reaper = reactor.callLater(self._timeout, lambda: reactor.stop())
        WampServerFactory.startFactory(self)

    def on_connect(self):
        """
        Called when a new connection is made.
        """
        if self._reaper:
            log.msg("Client has reconnected, cancelling reaper",
                logLevel=logging.DEBUG)
            self._reaper.cancel()
            self._reaper = None

        self._connection_count += 1
        log.msg("on_connect: connection count = %s" % self._connection_count,
            logLevel=logging.DEBUG)

    def connection_lost(self):
        """
        Called when a connection is lost.
        """
        if self._connection_count > 0:
            self._connection_count -= 1
        log.msg("connection_lost: connection count = %s" % self._connection_count,
            logLevel=logging.DEBUG)

        if self._connection_count == 0 and not self._reaper:
            log.msg("Starting timer, process will terminate in: %ssec" % self._timeout,
                logLevel=logging.DEBUG)
            self._reaper = reactor.callLater(self._timeout, lambda: reactor.stop())
