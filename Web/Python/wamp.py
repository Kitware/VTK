r"""wamp is a module that provide classes that extend any
WAMP related class for the purpose of vtkWeb.

"""

import inspect, types, string, random, logging, six

from threading import Timer

from twisted.python         import log
from twisted.internet       import reactor
from twisted.internet.defer import Deferred, returnValue

from autobahn      import wamp
from autobahn.wamp import types
from autobahn.wamp import procedure as exportRpc

from autobahn.twisted.wamp import ApplicationSession
from autobahn.twisted.websocket import WampWebSocketServerFactory
from autobahn.twisted.websocket import WampWebSocketServerProtocol

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

class ServerProtocol(ApplicationSession):
    """
    Defines the core server protocol for vtkWeb. Adds support to
    marshall/unmarshall RPC callbacks that involve ServerManager proxies as
    arguments or return values.

    Applications typically don't use this class directly, since it doesn't
    register any RPC callbacks that are required for basic web-applications with
    interactive visualizations. For that, use vtkWebServerProtocol.
    """

    def __init__(self):
        ApplicationSession.__init__(self, types.ComponentConfig(realm = "vtkweb"))
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

    def onJoin(self, details):
        ApplicationSession.onJoin(self, details)
        self.register(self)
        for protocol in self.vtkWebProtocols:
            self.register(protocol)

    def setApplication(self, application):
        self.Application = application

    def registerVtkWebProtocol(self, protocol):
        protocol.setApplication(self.Application)
        self.vtkWebProtocols.append(protocol)

    def getVtkWebProtocols(self):
        return self.vtkWebProtocols

    def updateSecret(self, newSecret):
        pass

    @exportRpc("application.exit")
    def exit(self):
        """RPC callback to exit"""
        reactor.stop()

# =============================================================================
#
# Base class for vtkWeb WampServerFactory
#
# =============================================================================

class TimeoutWampWebSocketServerFactory(WampWebSocketServerFactory):
    """
    TimeoutWampWebSocketServerFactory is WampWebSocketServerFactory subclass
    that adds support to close the web-server after a timeout when the last
    connected client drops.

    Currently, the protocol must call connectionMade() and connectionLost() methods
    to notify the factory that the connection was started/closed.
    If the connection count drops to zero, then the reap timer
    is started which will end the process if no other connections are made in
    the timeout interval.
    """

    def __init__(self, factory, *args, **kwargs):
        self._connection_count = 0
        self._timeout = kwargs['timeout']
        self._reaper = reactor.callLater(self._timeout, lambda: reactor.stop())

        del kwargs['timeout']
        WampWebSocketServerFactory.__init__(self, factory, *args, **kwargs)
        WampWebSocketServerFactory.protocol = TimeoutWampWebSocketServerProtocol

    def connectionMade(self):
        if self._reaper:
            log.msg("Client has reconnected, cancelling reaper", logLevel=logging.DEBUG)
            self._reaper.cancel()
            self._reaper = None
        self._connection_count += 1
        log.msg("on_connect: connection count = %s" % self._connection_count, logLevel=logging.DEBUG)

    def connectionLost(self, reason):
        if self._connection_count > 0:
            self._connection_count -= 1
        log.msg("connection_lost: connection count = %s" % self._connection_count, logLevel=logging.DEBUG)

        if self._connection_count == 0 and not self._reaper:
            log.msg("Starting timer, process will terminate in: %ssec" % self._timeout, logLevel=logging.DEBUG)
            self._reaper = reactor.callLater(self._timeout, lambda: reactor.stop())

# =============================================================================

class TimeoutWampWebSocketServerProtocol(WampWebSocketServerProtocol):

    def connectionMade(self):
        WampWebSocketServerProtocol.connectionMade(self)
        self.factory.connectionMade()

    def connectionLost(self, reason):
        WampWebSocketServerProtocol.connectionLost(self, reason)
        self.factory.connectionLost(reason)