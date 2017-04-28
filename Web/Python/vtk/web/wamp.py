r"""wamp is a module that provide classes that extend any
WAMP related class for the purpose of vtkWeb.

"""

from __future__ import absolute_import, division, print_function

import inspect, types, string, random, logging, six, json, re, base64, time

from threading import Timer

from twisted.web            import resource
from twisted.python         import log
from twisted.internet       import reactor
from twisted.internet       import defer
from twisted.internet.defer import Deferred, returnValue

from autobahn               import wamp
from autobahn               import util
from autobahn.wamp          import types
from autobahn.wamp          import auth
from autobahn.wamp          import register as exportRpc

from autobahn.twisted.wamp import ApplicationSession, RouterSession
from autobahn.twisted.websocket import WampWebSocketServerFactory
from autobahn.twisted.websocket import WampWebSocketServerProtocol
from autobahn.twisted.websocket import WebSocketServerProtocol

from vtk.web import protocols
from vtk.vtkWebCore import vtkWebApplication

# =============================================================================
salt = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(32))
application = None
imageCapture = None

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

    def __init__(self, config):
        ApplicationSession.__init__(self, config)
        self.vtkWebProtocols = []
        self.authdb = None
        self.secret = None
        self.Application = self.initApplication()
        self.initialize()

        # Init Binary WebSocket image renderer
        global imageCapture
        imageCapture = protocols.vtkWebViewPortImageDelivery()
        imageCapture.setApplication(self.Application)

    def setAuthDB(self, db):
        self.authdb = db
        if self.secret:
            self.authdb.updateKey('vtkweb', self.secret)

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

        # Init Binary WebSocket image renderer
        global imageCapture
        imageCapture.setApplication(self.Application)

    def registerVtkWebProtocol(self, protocol):
        protocol.coreServer = self
        protocol.setApplication(self.Application)
        self.vtkWebProtocols.append(protocol)

    def getVtkWebProtocols(self):
        return self.vtkWebProtocols

    def updateSecret(self, newSecret):
        self.secret = newSecret
        if self.authdb:
            self.authdb.updateKey('vtkweb', self.secret)

    @exportRpc("application.exit")
    def exit(self):
        """RPC callback to exit"""
        reactor.stop()

    @exportRpc("application.exit.later")
    def exitLater(self, secondsLater=60):
        """RPC callback to exit after a short delay"""
        reactor.callLater(secondsLater, reactor.stop)

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

# =============================================================================

class AuthDb:
    """
    An in-memory-only user database of a single user.
    """

    AUTHEXTRA = {'salt': 'salt123', 'keylen': 32, 'iterations': 1000}

    def __init__(self):
        self._creds = {'vtkweb': auth.derive_key("vtkweb-secret", self.AUTHEXTRA['salt'])}

    def get(self, authid):
        ## we return a deferred to simulate an asynchronous lookup
        return defer.succeed(self._creds.get(authid, None))

    def updateKey(self, id, newKey):
        self._creds[id] = auth.derive_key(newKey, self.AUTHEXTRA['salt'])

# =============================================================================

class PendingAuth:
   """
   Used for tracking pending authentications.
   """

   def __init__(self, key, session, authid, authrole, authmethod, authprovider):
      self.authid = authid
      self.authrole = authrole
      self.authmethod = authmethod
      self.authprovider = authprovider

      self.session = session
      self.timestamp = util.utcnow()
      self.nonce = util.newid()

      challenge_obj = {
         'authid': self.authid,
         'authrole': self.authrole,
         'authmethod': self.authmethod,
         'authprovider': self.authprovider,
         'session': self.session,
         'nonce': self.nonce,
         'timestamp': self.timestamp
      }
      self.challenge = json.dumps(challenge_obj)
      self.signature = auth.compute_wcs(key, self.challenge)

# =============================================================================

class CustomWampCraRouterSession(RouterSession):
    """
    A custom router session that authenticates via WAMP-CRA.
    """

    def __init__(self, routerFactory):
      """
      Constructor.
      """
      RouterSession.__init__(self, routerFactory)

    @defer.inlineCallbacks
    def onHello(self, realm, details):
        """
        Callback fired when client wants to attach session.
        """

        self._pending_auth = None

        if details.authmethods:
            for authmethod in details.authmethods:
                if authmethod == u"wampcra":

                    authdb = self.factory.authdb

                    ## lookup user in user DB
                    key = yield authdb.get(details.authid)

                    ## if user found ..
                    if key:

                        ## setup pending auth
                        self._pending_auth = PendingAuth(key, details.pending_session,
                            details.authid, "user", authmethod, "authdb")

                        ## send challenge to client
                        extra = { 'challenge': self._pending_auth.challenge }

                        ## when using salted passwords, provide the client with
                        ## the salt and then PBKDF2 parameters used
                        extra['salt'] = authdb.AUTHEXTRA['salt']
                        extra['iterations'] = 1000
                        extra['keylen'] = 32

                        defer.returnValue(types.Challenge('wampcra', extra))

        ## deny client
        defer.returnValue(types.Deny())


    def onAuthenticate(self, signature, extra):
        """
        Callback fired when a client responds to an authentication challenge.
        """

        ## if there is a pending auth, and the signature provided by client matches ..
        if self._pending_auth and signature.encode('utf-8') == self._pending_auth.signature:

            ## accept the client
            return types.Accept(authid = self._pending_auth.authid,
                authrole = self._pending_auth.authrole,
                authmethod = self._pending_auth.authmethod,
                authprovider = self._pending_auth.authprovider)

        ## deny client
        return types.Deny()


# =============================================================================
# Simple web server endpoint handling POST requests to execute rpc methods
# =============================================================================

class HttpRpcResource(resource.Resource, object):
    def __init__(self, serverProtocol, endpointRootPath):
        super(HttpRpcResource, self).__init__()

        self.functionMap = {}
        self.urlMatcher = re.compile(endpointRootPath.strip('/') + '/([^/]+)')

        # Build the rpc method dictionary
        protocolList = serverProtocol.getVtkWebProtocols()
        protocolList.append(serverProtocol)    # so the exit methods get "registered"
        for protocolObject in protocolList:
            test = lambda x: inspect.ismethod(x) or inspect.isfunction(x)
            for k in inspect.getmembers(protocolObject.__class__, test):
                proc = k[1]
                if "_wampuris" in proc.__dict__:
                    pat = proc.__dict__["_wampuris"][0]
                    if pat.is_endpoint():
                        uri = pat.uri()
                        self.functionMap[uri] = (protocolObject, proc)

    def extractRpcMethod(self, path):
        m = self.urlMatcher.search(path)
        if m:
            return m.group(1)
        else:
            return None

    def getChild(self, path, request):
        return self

    def render_POST(self, request):
        payload = json.loads(request.content.getvalue())
        args = payload['args']
        methodName = self.extractRpcMethod(request.path)
        obj,func = self.functionMap[methodName]
        results = func(obj, *args)
        return json.dumps(results)

# =============================================================================
# Binary WebSocket image push protocol
# =============================================================================

class ImagePushBinaryWebSocketServerProtocol(WebSocketServerProtocol):

    def onOpen(self):
        global imageCapture
        self.helper = imageCapture
        self.app = imageCapture.getApplication()
        self.viewToCapture = {}
        self.lastStaleTime = 0
        self.staleHandlerCount = 0
        self.deltaStaleTimeBeforeRender = 0.5 # 0.5s
        self.subscription = self.app.AddObserver('PushRender', lambda obj, event: reactor.callLater(0.0, lambda: self.render()))
        self.subscriptionReset = self.app.AddObserver('ResetActiveView', lambda obj, event: reactor.callLater(0.0, lambda: self.resetActiveView()))

    def onMessage(self, msg, isBinary):
        request = json.loads(msg)
        if 'view_id' in request:
            viewId = str(request['view_id'])
            if viewId not in self.viewToCapture:
                self.viewToCapture[viewId] = { 'quality': 100, 'enabled': True, 'view': self.helper.getView(viewId), 'view_id': viewId, 'mtime': 0 }

            if 'invalidate_cache' in request:
                if self.viewToCapture[viewId]['view']:
                    self.app.InvalidateCache(self.viewToCapture[viewId]['view'].SMProxy)
                    self.render()
            else:
                # Update fields
                objToUpdate = self.viewToCapture[viewId]
                for key in request:
                    objToUpdate[key] = request[key]

    def onClose(self, wasClean, code, reason):
        self.viewToCapture = {}
        self.app.RemoveObserver(self.subscription)
        self.app.RemoveObserver(self.subscriptionReset)

    def connectionLost(self, reason):
        self.viewToCapture = {}
        self.app.RemoveObserver(self.subscription)
        self.app.RemoveObserver(self.subscriptionReset)

    def renderStaleImage(self):
        self.staleHandlerCount -= 1

        if self.lastStaleTime != 0:
            delta = (time.time() - self.lastStaleTime)
            if delta >= self.deltaStaleTimeBeforeRender:
                self.render()
            else:
                self.staleHandlerCount += 1
                reactor.callLater(self.deltaStaleTimeBeforeRender - delta + 0.001, lambda: self.renderStaleImage())

    def resetActiveView(self):
        if '-1' in self.viewToCapture and self.viewToCapture['-1']:
            activeViewReq = self.viewToCapture['-1']
            previousSize = tuple(activeViewReq['view'].ViewSize)
            activeViewReq['view'] = self.helper.getView('-1')
            activeViewReq['view'].ViewSize = previousSize

    def render(self):
        keepGoing = False
        for k in self.viewToCapture:
            v = self.viewToCapture[k];
            if v['enabled']:
                keepGoing = True
                view = v['view']
                if hasattr(view,'SMProxy'):
                    view = view.SMProxy
                quality = v['quality']
                mtime = v['mtime']
                base64Image = self.app.StillRenderToString(view, mtime, quality)
                stale = self.app.GetHasImagesBeingProcessed(view)
                if base64Image:
                    v['mtime'] = self.app.GetLastStillRenderToStringMTime()
                    meta = {
                        'size': self.app.GetLastStillRenderImageSize(),
                        'id': k
                    }
                    self.sendMessage(json.dumps(meta, ensure_ascii = False).encode('utf8'), False)
                    self.sendMessage(base64.standard_b64decode(base64Image), True)
                if stale:
                    self.lastStaleTime = time.time()
                    if self.staleHandlerCount == 0:
                        self.staleHandlerCount += 1
                        reactor.callLater(self.deltaStaleTimeBeforeRender, lambda: self.renderStaleImage())
                else:
                    self.lastStaleTime = 0

        return keepGoing
