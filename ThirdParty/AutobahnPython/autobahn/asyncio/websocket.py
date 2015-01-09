###############################################################################
##
##  Copyright (C) 2013-2014 Tavendo GmbH
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

__all__ = [
   'WebSocketAdapterProtocol',
   'WebSocketServerProtocol',
   'WebSocketClientProtocol',
   'WebSocketAdapterFactory',
   'WebSocketServerFactory',
   'WebSocketClientFactory',

   'WampWebSocketServerProtocol',
   'WampWebSocketClientProtocol',
   'WampWebSocketServerFactory',
   'WampWebSocketClientFactory'
]

from collections import deque

try:
   import asyncio
   from asyncio.tasks import iscoroutine
   from asyncio import Future
except ImportError:
   ## Trollius >= 0.3 was renamed
   import trollius as asyncio
   from trollius.tasks import iscoroutine
   from trollius import Future

from autobahn.wamp import websocket
from autobahn.websocket import protocol
from autobahn.websocket import http


def yields(value):
   """
   Return True iff the value yields.

   See: http://stackoverflow.com/questions/20730248/maybedeferred-analog-with-asyncio
   """
   return isinstance(value, Future) or iscoroutine(value)



class WebSocketAdapterProtocol(asyncio.Protocol):
   """
   Adapter class for Asyncio WebSocket client and server protocols.
   """

   def connection_made(self, transport):
      self.transport = transport

      self.receive_queue = deque()
      self._consume()

      peer = transport.get_extra_info('peername')
      try:
         self.peer = "%s:%d" % (peer[0], peer[1])
      except:
         ## eg Unix Domain sockets don't have host/port
         self.peer = str(peer)

      self._connectionMade()


   def connection_lost(self, exc):
      self._connectionLost(exc)
      self.transport = None


   def _consume(self):
      self.waiter = Future()

      def process(_):
         while len(self.receive_queue):
            data = self.receive_queue.popleft()
            if self.transport:
               self._dataReceived(data)
            else:
               print("WebSocketAdapterProtocol._consume: no transport")
         self._consume()

      self.waiter.add_done_callback(process)


   def data_received(self, data):
      self.receive_queue.append(data)
      if not self.waiter.done():
         self.waiter.set_result(None)


   # noinspection PyUnusedLocal
   def _closeConnection(self, abort = False):
      self.transport.close()


   def _onOpen(self):
      res = self.onOpen()
      if yields(res):
         asyncio.async(res)

   def _onMessageBegin(self, isBinary):
      res = self.onMessageBegin(isBinary)
      if yields(res):
         asyncio.async(res)

   def _onMessageFrameBegin(self, length):
      res = self.onMessageFrameBegin(length)
      if yields(res):
         asyncio.async(res)

   def _onMessageFrameData(self, payload):
      res = self.onMessageFrameData(payload)
      if yields(res):
         asyncio.async(res)

   def _onMessageFrameEnd(self):
      res = self.onMessageFrameEnd()
      if yields(res):
         asyncio.async(res)

   def _onMessageFrame(self, payload):
      res = self.onMessageFrame(payload)
      if yields(res):
         asyncio.async(res)

   def _onMessageEnd(self):
      res = self.onMessageEnd()
      if yields(res):
         asyncio.async(res)

   def _onMessage(self, payload, isBinary):
      res = self.onMessage(payload, isBinary)
      if yields(res):
         asyncio.async(res)

   def _onPing(self, payload):
      res = self.onPing(payload)
      if yields(res):
         asyncio.async(res)

   def _onPong(self, payload):
      res = self.onPong(payload)
      if yields(res):
         asyncio.async(res)

   def _onClose(self, wasClean, code, reason):
      res = self.onClose(wasClean, code, reason)
      if yields(res):
         asyncio.async(res)


   def registerProducer(self, producer, streaming):
      raise Exception("not implemented")



class WebSocketServerProtocol(WebSocketAdapterProtocol, protocol.WebSocketServerProtocol):
   """
   Base class for Asyncio WebSocket server protocols.
   """

   def _onConnect(self, request):
      ## onConnect() will return the selected subprotocol or None
      ## or a pair (protocol, headers) or raise an HttpException
      ##
      try:
         res = self.onConnect(request)
         #if yields(res):
         #  res = yield from res
      except http.HttpException as exc:
         self.failHandshake(exc.reason, exc.code)
      except Exception:
         self.failHandshake(http.INTERNAL_SERVER_ERROR[1], http.INTERNAL_SERVER_ERROR[0])
      else:
         self.succeedHandshake(res)



class WebSocketClientProtocol(WebSocketAdapterProtocol, protocol.WebSocketClientProtocol):
   """
   Base class for Asyncio WebSocket client protocols.
   """

   def _onConnect(self, response):
      res = self.onConnect(response)
      if yields(res):
         asyncio.async(res)



class WebSocketAdapterFactory:
   """
   Adapter class for Asyncio WebSocket client and server factories.
   """

   def _log(self, msg):
      print(msg)


   def _callLater(self, delay, fun):
      return self.loop.call_later(delay, fun)


   def __call__(self):
      proto = self.protocol()
      proto.factory = self
      return proto



class WebSocketServerFactory(WebSocketAdapterFactory, protocol.WebSocketServerFactory):
   """
   Base class for Asyncio WebSocket server factories.
   """

   def __init__(self, *args, **kwargs):
      """
      In addition to all arguments to the constructor of
      :class:`autobahn.websocket.protocol.WebSocketServerFactory`,
      you can supply a `loop` keyword argument to specify the
      Asyncio loop to be used.
      """
      if 'loop' in kwargs:
         if kwargs['loop']:
            self.loop = kwargs['loop']
         else:
            self.loop = asyncio.get_event_loop()
         del kwargs['loop']
      else:
         self.loop = asyncio.get_event_loop()

      protocol.WebSocketServerFactory.__init__(self, *args, **kwargs)



class WebSocketClientFactory(WebSocketAdapterFactory, protocol.WebSocketClientFactory):
   """
   Base class for Asyncio WebSocket client factories.
   """

   def __init__(self, *args, **kwargs):
      """
      In addition to all arguments to the constructor of
      :class:`autobahn.websocket.protocol.WebSocketClientFactory`,
      you can supply a `loop` keyword argument to specify the
      Asyncio loop to be used.
      """
      if 'loop' in kwargs:
         if kwargs['loop']:
            self.loop = kwargs['loop']
         else:
            self.loop = asyncio.get_event_loop()
         del kwargs['loop']
      else:
         self.loop = asyncio.get_event_loop()

      protocol.WebSocketClientFactory.__init__(self, *args, **kwargs)



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
