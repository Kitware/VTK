###############################################################################
##
##  Copyright (C) 2011-2014 Tavendo GmbH
##
##  Licensed under the Apache License, Version 2.0 (the "License");
##  you may not use this file except in compliance with the License.
##  You may obtain a copy of the License at
##
##      http://www.apache.org/licenses/LICENSE-2.0
##
##  Unless required by applicable law or agreed to in writing, software
##  distributed under the License is distributed on an "AS IS" BASIS,
##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
##  See the License for the specific language governing permissions and
##  limitations under the License.
##
###############################################################################

from __future__ import absolute_import

__all__ = [
   'WebSocketAdapterProtocol',
   'WebSocketServerProtocol',
   'WebSocketClientProtocol',
   'WebSocketAdapterFactory',
   'WebSocketServerFactory',
   'WebSocketClientFactory',

   'WrappingWebSocketAdapter',
   'WrappingWebSocketServerProtocol',
   'WrappingWebSocketClientProtocol',
   'WrappingWebSocketServerFactory',
   'WrappingWebSocketClientFactory',

   'listenWS',
   'connectWS',

   'WampWebSocketServerProtocol',
   'WampWebSocketServerFactory',
   'WampWebSocketClientProtocol',
   'WampWebSocketClientFactory',
]


from base64 import b64encode, b64decode

from zope.interface import implementer

import twisted.internet.protocol
from twisted.internet.defer import maybeDeferred
from twisted.python import log
from twisted.internet.interfaces import ITransport

from autobahn.wamp import websocket
from autobahn.websocket import protocol
from autobahn.websocket import http

from autobahn.websocket.compress import PerMessageDeflateOffer, \
                                        PerMessageDeflateOfferAccept, \
                                        PerMessageDeflateResponse, \
                                        PerMessageDeflateResponseAccept


class WebSocketAdapterProtocol(twisted.internet.protocol.Protocol):
   """
   Adapter class for Twisted WebSocket client and server protocols.
   """

   def connectionMade(self):
      ## the peer we are connected to
      try:
         peer = self.transport.getPeer()
      except:
         ## ProcessProtocols lack getPeer()
         self.peer = "?"
      else:
         try:
            self.peer = "%s:%d" % (peer.host, peer.port)
         except:
            ## eg Unix Domain sockets don't have host/port
            self.peer = str(peer)

      self._connectionMade()

      ## Set "Nagle"
      try:
         self.transport.setTcpNoDelay(self.tcpNoDelay)
      except:
         ## eg Unix Domain sockets throw Errno 22 on this
         pass


   def connectionLost(self, reason):
      self._connectionLost(reason)


   def dataReceived(self, data):
      self._dataReceived(data)


   def _closeConnection(self, abort = False):
      if abort and hasattr(self.transport, 'abortConnection'):
         ## ProcessProtocol lacks abortConnection()
         self.transport.abortConnection()
      else:
         self.transport.loseConnection()


   def _onOpen(self):
      self.onOpen()

   def _onMessageBegin(self, isBinary):
      self.onMessageBegin(isBinary)

   def _onMessageFrameBegin(self, length):
      self.onMessageFrameBegin(length)

   def _onMessageFrameData(self, payload):
      self.onMessageFrameData(payload)

   def _onMessageFrameEnd(self):
      self.onMessageFrameEnd()

   def _onMessageFrame(self, payload):
      self.onMessageFrame(payload)

   def _onMessageEnd(self):
      self.onMessageEnd()

   def _onMessage(self, payload, isBinary):
      self.onMessage(payload, isBinary)

   def _onPing(self, payload):
      self.onPing(payload)

   def _onPong(self, payload):
      self.onPong(payload)

   def _onClose(self, wasClean, code, reason):
      self.onClose(wasClean, code, reason)


   def registerProducer(self, producer, streaming):
      """
      Register a Twisted producer with this protocol.

      Modes: Hybi, Hixie

      :param producer: A Twisted push or pull producer.
      :type producer: object
      :param streaming: Producer type.
      :type streaming: bool
      """
      self.transport.registerProducer(producer, streaming)



class WebSocketServerProtocol(WebSocketAdapterProtocol, protocol.WebSocketServerProtocol):
   """
   Base class for Twisted WebSocket server protocols.
   """

   def _onConnect(self, request):
      ## onConnect() will return the selected subprotocol or None
      ## or a pair (protocol, headers) or raise an HttpException
      ##
      res = maybeDeferred(self.onConnect, request)

      res.addCallback(self.succeedHandshake)

      def forwardError(failure):
         if failure.check(http.HttpException):
            return self.failHandshake(failure.value.reason, failure.value.code)
         else:
            if self.debug:
               self.factory._log("Unexpected exception in onConnect ['%s']" % failure.value)
            return self.failHandshake(http.INTERNAL_SERVER_ERROR[1], http.INTERNAL_SERVER_ERROR[0])

      res.addErrback(forwardError)



class WebSocketClientProtocol(WebSocketAdapterProtocol, protocol.WebSocketClientProtocol):
   """
   Base class for Twisted WebSocket client protocols.
   """

   def _onConnect(self, response):
      self.onConnect(response)



class WebSocketAdapterFactory:
   """
   Adapter class for Twisted WebSocket client and server factories.
   """

   def _log(self, msg):
      log.msg(msg)


   def _callLater(self, delay, fun):
      return self.reactor.callLater(delay, fun)



class WebSocketServerFactory(WebSocketAdapterFactory, protocol.WebSocketServerFactory, twisted.internet.protocol.ServerFactory):
   """
   Base class for Twisted WebSocket server factories.

   .. seealso:: `twisted.internet.protocol.ServerFactory <http://twistedmatrix.com/documents/current/api/twisted.internet.protocol.ServerFactory.html>`_
   """

   def __init__(self, *args, **kwargs):
      """
      In addition to all arguments to the constructor of
      :class:`autobahn.websocket.protocol.WebSocketServerFactory`,
      you can supply a `reactor` keyword argument to specify the
      Twisted reactor to be used.
      """
      ## lazy import to avoid reactor install upon module import
      if 'reactor' in kwargs:
         if kwargs['reactor']:
            self.reactor = kwargs['reactor']
         else:
            from twisted.internet import reactor
            self.reactor = reactor
         del kwargs['reactor']
      else:
         from twisted.internet import reactor
         self.reactor = reactor

      protocol.WebSocketServerFactory.__init__(self, *args, **kwargs)



class WebSocketClientFactory(WebSocketAdapterFactory, protocol.WebSocketClientFactory, twisted.internet.protocol.ClientFactory):
   """
   Base class for Twisted WebSocket client factories.

   .. seealso:: `twisted.internet.protocol.ClientFactory <http://twistedmatrix.com/documents/current/api/twisted.internet.protocol.ClientFactory.html>`_
   """

   def __init__(self, *args, **kwargs):
      """
      In addition to all arguments to the constructor of
      :class:`autobahn.websocket.protocol.WebSocketClientFactory`,
      you can supply a `reactor` keyword argument to specify the
      Twisted reactor to be used.
      """
      ## lazy import to avoid reactor install upon module import
      if 'reactor' in kwargs:
         if kwargs['reactor']:
            self.reactor = kwargs['reactor']
         else:
            from twisted.internet import reactor
            self.reactor = reactor
         del kwargs['reactor']
      else:
         from twisted.internet import reactor
         self.reactor = reactor

      protocol.WebSocketClientFactory.__init__(self, *args, **kwargs)



@implementer(ITransport)
class WrappingWebSocketAdapter:
   """
   An adapter for stream-based transport over WebSocket.

   This follows "websockify" (https://github.com/kanaka/websockify)
   and should be compatible with that.

   It uses WebSocket subprotocol negotiation and 2+ subprotocols:
     - binary (or a compatible subprotocol)
     - base64

   Octets are either transmitted as the payload of WebSocket binary
   messages when using the 'binary' subprotocol (or an alternative
   binary compatible subprotocol), or encoded with Base64
   and then transmitted as the payload of WebSocket text messages when
   using the 'base64' subprotocol.
   """

   def onConnect(self, requestOrResponse):

      ## Negotiate either the 'binary' or the 'base64' WebSocket subprotocol
      ##
      if isinstance(requestOrResponse, protocol.ConnectionRequest):
         request = requestOrResponse
         for p in request.protocols:
            if p in self.factory._subprotocols:
               self._binaryMode = (p != 'base64')
               return p
         raise http.HttpException(http.NOT_ACCEPTABLE[0], "this server only speaks %s WebSocket subprotocols" % self.factory._subprotocols)
      elif isinstance(requestOrResponse, protocol.ConnectionResponse):
         response = requestOrResponse
         if response.protocol not in self.factory._subprotocols:
            self.failConnection(protocol.WebSocketProtocol.CLOSE_STATUS_CODE_PROTOCOL_ERROR, "this client only speaks %s WebSocket subprotocols" % self.factory._subprotocols)
         self._binaryMode = (response.protocol != 'base64')
      else:
         ## should not arrive here
         raise Exception("logic error")

   def onOpen(self):
      self._proto.connectionMade()

   def onMessage(self, payload, isBinary):
      if isBinary != self._binaryMode:
         self.failConnection(protocol.WebSocketProtocol.CLOSE_STATUS_CODE_UNSUPPORTED_DATA, "message payload type does not match the negotiated subprotocol")
      else:
         if not isBinary:
            try:
               payload = b64decode(payload)
            except Exception as e:
               self.failConnection(protocol.WebSocketProtocol.CLOSE_STATUS_CODE_INVALID_PAYLOAD, "message payload base64 decoding error: {}".format(e))
         #print("forwarding payload: {}".format(binascii.hexlify(payload)))
         self._proto.dataReceived(payload)

   def onClose(self, wasClean, code, reason):
      self._proto.connectionLost(None)

   def write(self, data):
      #print("sending payload: {}".format(binascii.hexlify(data)))
      ## part of ITransport
      assert(type(data) == bytes)
      if self._binaryMode:
         self.sendMessage(data, isBinary = True)
      else:
         data = b64encode(data)
         self.sendMessage(data, isBinary = False)

   def writeSequence(self, data):
      ## part of ITransport
      for d in data:
         self.write(d)

   def loseConnection(self):
      ## part of ITransport
      self.sendClose()

   def getPeer(self):
      ## part of ITransport
      return self.transport.getPeer()

   def getHost(self):
      ## part of ITransport
      return self.transport.getHost()



class WrappingWebSocketServerProtocol(WrappingWebSocketAdapter, WebSocketServerProtocol):
   """
   Server protocol for stream-based transport over WebSocket.
   """



class WrappingWebSocketClientProtocol(WrappingWebSocketAdapter, WebSocketClientProtocol):
   """
   Client protocol for stream-based transport over WebSocket.
   """



class WrappingWebSocketServerFactory(WebSocketServerFactory):
   """
   Wrapping server factory for stream-based transport over WebSocket.
   """

   def __init__(self,
                factory,
                url,
                reactor = None,
                enableCompression = True,
                autoFragmentSize = 0,
                subprotocol = None,
                debug = False):
      """
      Constructor.

      :param factory: Stream-based factory to be wrapped.
      :type factory: A subclass of `twisted.internet.protocol.Factory`
      :param url: WebSocket URL of the server this server factory will work for.
      :type url: str
      """
      self._factory = factory
      self._subprotocols = ['binary', 'base64']
      if subprotocol:
          self._subprotocols.append(subprotocol)

      WebSocketServerFactory.__init__(self,
         url = url,
         reactor = reactor,
         protocols = self._subprotocols,
         debug = debug)

      ## automatically fragment outgoing traffic into WebSocket frames
      ## of this size
      self.setProtocolOptions(autoFragmentSize = autoFragmentSize)

      ## play nice and perform WS closing handshake
      self.setProtocolOptions(failByDrop = False)

      if enableCompression:
         ## Enable WebSocket extension "permessage-deflate".
         ##

         ## Function to accept offers from the client ..
         def accept(offers):
            for offer in offers:
               if isinstance(offer, PerMessageDeflateOffer):
                  return PerMessageDeflateOfferAccept(offer)

         self.setProtocolOptions(perMessageCompressionAccept = accept)


   def buildProtocol(self, addr):
      proto = WrappingWebSocketServerProtocol()
      proto.factory = self
      proto._proto = self._factory.buildProtocol(addr)
      proto._proto.transport = proto
      return proto


   def startFactory(self):
      self._factory.startFactory()
      WebSocketServerFactory.startFactory(self)


   def stopFactory(self):
      self._factory.stopFactory()
      WebSocketServerFactory.stopFactory(self)



class WrappingWebSocketClientFactory(WebSocketClientFactory):
   """
   Wrapping client factory for stream-based transport over WebSocket.
   """

   def __init__(self,
                factory,
                url,
                reactor = None,
                enableCompression = True,
                autoFragmentSize = 0,
                subprotocol = None,
                debug = False):
      """
      Constructor.

      :param factory: Stream-based factory to be wrapped.
      :type factory: A subclass of `twisted.internet.protocol.Factory`
      :param url: WebSocket URL of the server this client factory will connect to.
      :type url: str
      """
      self._factory = factory
      self._subprotocols = ['binary', 'base64']
      if subprotocol:
          self._subprotocols.append(subprotocol)

      WebSocketClientFactory.__init__(self,
         url = url,
         reactor = reactor,
         protocols = self._subprotocols,
         debug = debug)

      ## automatically fragment outgoing traffic into WebSocket frames
      ## of this size
      self.setProtocolOptions(autoFragmentSize = autoFragmentSize)

      ## play nice and perform WS closing handshake
      self.setProtocolOptions(failByDrop = False)

      if enableCompression:
         ## Enable WebSocket extension "permessage-deflate".
         ##

         ## The extensions offered to the server ..
         offers = [PerMessageDeflateOffer()]
         self.setProtocolOptions(perMessageCompressionOffers = offers)

         ## Function to accept responses from the server ..
         def accept(response):
            if isinstance(response, PerMessageDeflateResponse):
               return PerMessageDeflateResponseAccept(response)

         self.setProtocolOptions(perMessageCompressionAccept = accept)


   def buildProtocol(self, addr):
      proto = WrappingWebSocketClientProtocol()
      proto.factory = self
      proto._proto = self._factory.buildProtocol(addr)
      proto._proto.transport = proto
      return proto



def connectWS(factory, contextFactory = None, timeout = 30, bindAddress = None):
   """
   Establish WebSocket connection to a server. The connection parameters like target
   host, port, resource and others are provided via the factory.

   :param factory: The WebSocket protocol factory to be used for creating client protocol instances.
   :type factory: An :class:`autobahn.websocket.WebSocketClientFactory` instance.
   :param contextFactory: SSL context factory, required for secure WebSocket connections ("wss").
   :type contextFactory: A `twisted.internet.ssl.ClientContextFactory <http://twistedmatrix.com/documents/current/api/twisted.internet.ssl.ClientContextFactory.html>`_ instance.
   :param timeout: Number of seconds to wait before assuming the connection has failed.
   :type timeout: int
   :param bindAddress: A (host, port) tuple of local address to bind to, or None.
   :type bindAddress: tuple

   :returns: obj -- An object which implements `twisted.interface.IConnector <http://twistedmatrix.com/documents/current/api/twisted.internet.interfaces.IConnector.html>`_.
   """
   ## lazy import to avoid reactor install upon module import
   if hasattr(factory, 'reactor'):
      reactor = factory.reactor
   else:
      from twisted.internet import reactor

   if factory.proxy is not None:
      if factory.isSecure:
         raise Exception("WSS over explicit proxies not implemented")
      else:
         conn = reactor.connectTCP(factory.proxy['host'], factory.proxy['port'], factory, timeout, bindAddress)
   else:
      if factory.isSecure:
         if contextFactory is None:
            # create default client SSL context factory when none given
            from twisted.internet import ssl
            contextFactory = ssl.ClientContextFactory()
         conn = reactor.connectSSL(factory.host, factory.port, factory, contextFactory, timeout, bindAddress)
      else:
         conn = reactor.connectTCP(factory.host, factory.port, factory, timeout, bindAddress)
   return conn



def listenWS(factory, contextFactory = None, backlog = 50, interface = ''):
   """
   Listen for incoming WebSocket connections from clients. The connection parameters like
   listening port and others are provided via the factory.

   :param factory: The WebSocket protocol factory to be used for creating server protocol instances.
   :type factory: An :class:`autobahn.websocket.WebSocketServerFactory` instance.
   :param contextFactory: SSL context factory, required for secure WebSocket connections ("wss").
   :type contextFactory: A twisted.internet.ssl.ContextFactory.
   :param backlog: Size of the listen queue.
   :type backlog: int
   :param interface: The interface (derived from hostname given) to bind to, defaults to '' (all).
   :type interface: str

   :returns: obj -- An object that implements `twisted.interface.IListeningPort <http://twistedmatrix.com/documents/current/api/twisted.internet.interfaces.IListeningPort.html>`_.
   """
   ## lazy import to avoid reactor install upon module import
   if hasattr(factory, 'reactor'):
      reactor = factory.reactor
   else:
      from twisted.internet import reactor

   if factory.isSecure:
      if contextFactory is None:
         raise Exception("Secure WebSocket listen requested, but no SSL context factory given")
      listener = reactor.listenSSL(factory.port, factory, contextFactory, backlog, interface)
   else:
      listener = reactor.listenTCP(factory.port, factory, backlog, interface)
   return listener



class WampWebSocketServerProtocol(websocket.WampWebSocketServerProtocol, WebSocketServerProtocol):
   pass



class WampWebSocketServerFactory(websocket.WampWebSocketServerFactory, WebSocketServerFactory):

   protocol = WampWebSocketServerProtocol

   def __init__(self, factory, *args, **kwargs):

      if 'serializers' in kwargs:
         serializers = kwargs['serializers']
         del kwargs['serializers']
      else:
         serializers = None

      if 'debug_wamp' in kwargs:
         debug_wamp = kwargs['debug_wamp']
         del kwargs['debug_wamp']
      else:
         debug_wamp = False

      websocket.WampWebSocketServerFactory.__init__(self, factory, serializers, debug_wamp = debug_wamp)

      kwargs['protocols'] = self._protocols

      # noinspection PyCallByClass
      WebSocketServerFactory.__init__(self, *args, **kwargs)



class WampWebSocketClientProtocol(websocket.WampWebSocketClientProtocol, WebSocketClientProtocol):
   pass



class WampWebSocketClientFactory(websocket.WampWebSocketClientFactory, WebSocketClientFactory):

   protocol = WampWebSocketClientProtocol

   def __init__(self, factory, *args, **kwargs):

      if 'serializers' in kwargs:
         serializers = kwargs['serializers']
         del kwargs['serializers']
      else:
         serializers = None

      if 'debug_wamp' in kwargs:
         debug_wamp = kwargs['debug_wamp']
         del kwargs['debug_wamp']
      else:
         debug_wamp = False

      websocket.WampWebSocketClientFactory.__init__(self, factory, serializers, debug_wamp = debug_wamp)

      kwargs['protocols'] = self._protocols

      WebSocketClientFactory.__init__(self, *args, **kwargs)
