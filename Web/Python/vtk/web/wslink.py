r"""wslink is a module that provide classes that extend any
wslink related class for the purpose of vtkWeb.

"""

from __future__ import absolute_import, division, print_function

# import inspect, types, string, random, logging, six, json, re, base64, time

# from threading import Timer

# from twisted.web            import resource
# from twisted.python         import log
# from twisted.internet       import reactor
# from twisted.internet       import defer
# from twisted.internet.defer import Deferred, returnValue

# from autobahn               import wamp
# from autobahn               import util
# from autobahn.wamp          import types
# from autobahn.wamp          import auth
# from autobahn.wamp          import register as exportRpc

# from autobahn.twisted.wamp import ApplicationSession, RouterSession
# from autobahn.twisted.websocket import WampWebSocketServerFactory
# from autobahn.twisted.websocket import WampWebSocketServerProtocol
from autobahn.twisted.websocket import WebSocketServerProtocol

from wslink import websocket
from wslink import register as exportRpc

from vtk.web import protocols
from vtk.vtkWebCore import vtkWebApplication

# =============================================================================
application = None
imageCapture = None

# =============================================================================
#
# Base class for vtkWeb ServerProtocol
#
# =============================================================================

class ServerProtocol(websocket.ServerProtocol):
    """
    Defines the core server protocol for vtkWeb. Adds support to
    marshall/unmarshall RPC callbacks that involve ServerManager proxies as
    arguments or return values.

    Applications typically don't use this class directly, since it doesn't
    register any RPC callbacks that are required for basic web-applications with
    interactive visualizations. For that, use vtkWebServerProtocol.
    """

    def __init__(self):
        self.setSharedObject("app", self.initApplication())
        websocket.ServerProtocol.__init__(self)

        # Init Binary WebSocket image renderer
        global imageCapture
        imageCapture = protocols.vtkWebViewPortImageDelivery()
        imageCapture.setApplication(self.getApplication())

    def initApplication(self):
        """
        Let subclass optionally initialize a custom application in lieu
        of the default vtkWebApplication.
        """
        global application
        if not application:
            application = vtkWebApplication()
        return application

    def setApplication(self, application):
        self.setSharedObject("app", application)

        # Init Binary WebSocket image renderer
        global imageCapture
        imageCapture.setApplication(self.getApplication())

    def getApplication(self):
        return self.getSharedObject("app")

    def registerVtkWebProtocol(self, protocol):
        self.registerLinkProtocol(protocol)

    def getVtkWebProtocols(self):
        return self.getLinkProtocols()

# =============================================================================
# Binary WebSocket image push protocol
# TODO remove in favor of handling over main websocket.
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
