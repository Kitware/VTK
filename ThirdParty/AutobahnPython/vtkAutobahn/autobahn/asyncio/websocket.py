###############################################################################
#
# The MIT License (MIT)
#
# Copyright (c) Crossbar.io Technologies GmbH
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
###############################################################################

from __future__ import absolute_import

from collections import deque

import txaio
txaio.use_asyncio()

from autobahn.util import public
from autobahn.wamp import websocket
from autobahn.websocket import protocol

try:
    import asyncio
    from asyncio import iscoroutine
    from asyncio import Future
except ImportError:
    # Trollius >= 0.3 was renamed
    # noinspection PyUnresolvedReferences
    import trollius as asyncio
    from trollius import iscoroutine
    from trollius import Future

if hasattr(asyncio, 'ensure_future'):
    ensure_future = asyncio.ensure_future
else:  # Deprecated since Python 3.4.4
    ensure_future = asyncio.async

__all__ = (
    'WebSocketServerProtocol',
    'WebSocketClientProtocol',
    'WebSocketServerFactory',
    'WebSocketClientFactory',
    'WampWebSocketServerProtocol',
    'WampWebSocketClientProtocol',
    'WampWebSocketServerFactory',
    'WampWebSocketClientFactory',
)


def yields(value):
    """
    Returns ``True`` iff the value yields.

    .. seealso:: http://stackoverflow.com/questions/20730248/maybedeferred-analog-with-asyncio
    """
    return isinstance(value, Future) or iscoroutine(value)


class WebSocketAdapterProtocol(asyncio.Protocol):
    """
    Adapter class for asyncio-based WebSocket client and server protocols.
    """

    def connection_made(self, transport):
        self.transport = transport

        self.receive_queue = deque()
        self._consume()

        try:
            peer = transport.get_extra_info('peername')
            try:
                # FIXME: tcp4 vs tcp6
                self.peer = u"tcp:%s:%d" % (peer[0], peer[1])
            except:
                # e.g. Unix Domain sockets don't have host/port
                self.peer = u"unix:{0}".format(peer)
        except:
            self.peer = u"?"

        self._connectionMade()

    def connection_lost(self, exc):
        self._connectionLost(exc)
        # according to asyncio docs, connection_lost(None) is called
        # if something else called transport.close()
        if exc is not None:
            self.transport.close()
        self.transport = None

    def _consume(self):
        self.waiter = Future(loop=self.factory.loop or txaio.config.loop)

        def process(_):
            while len(self.receive_queue):
                data = self.receive_queue.popleft()
                if self.transport:
                    self._dataReceived(data)
            self._consume()

        self.waiter.add_done_callback(process)

    def data_received(self, data):
        self.receive_queue.append(data)
        if not self.waiter.done():
            self.waiter.set_result(None)

    # noinspection PyUnusedLocal
    def _closeConnection(self, abort=False):
        self.transport.close()

    def _onOpen(self):
        res = self.onOpen()
        if yields(res):
            ensure_future(res)

    def _onMessageBegin(self, isBinary):
        res = self.onMessageBegin(isBinary)
        if yields(res):
            ensure_future(res)

    def _onMessageFrameBegin(self, length):
        res = self.onMessageFrameBegin(length)
        if yields(res):
            ensure_future(res)

    def _onMessageFrameData(self, payload):
        res = self.onMessageFrameData(payload)
        if yields(res):
            ensure_future(res)

    def _onMessageFrameEnd(self):
        res = self.onMessageFrameEnd()
        if yields(res):
            ensure_future(res)

    def _onMessageFrame(self, payload):
        res = self.onMessageFrame(payload)
        if yields(res):
            ensure_future(res)

    def _onMessageEnd(self):
        res = self.onMessageEnd()
        if yields(res):
            ensure_future(res)

    def _onMessage(self, payload, isBinary):
        res = self.onMessage(payload, isBinary)
        if yields(res):
            ensure_future(res)

    def _onPing(self, payload):
        res = self.onPing(payload)
        if yields(res):
            ensure_future(res)

    def _onPong(self, payload):
        res = self.onPong(payload)
        if yields(res):
            ensure_future(res)

    def _onClose(self, wasClean, code, reason):
        res = self.onClose(wasClean, code, reason)
        if yields(res):
            ensure_future(res)

    def get_channel_id(self):
        """
        Implements :func:`autobahn.wamp.interfaces.ITransport.get_channel_id`
        """
        self.log.debug('FIXME: transport channel binding not implemented for asyncio (autobahn-python issue #729)')
        return None

    def registerProducer(self, producer, streaming):
        raise Exception("not implemented")


@public
class WebSocketServerProtocol(WebSocketAdapterProtocol, protocol.WebSocketServerProtocol):
    """
    Base class for asyncio-based WebSocket server protocols.

    Implements:

    * :class:`autobahn.websocket.interfaces.IWebSocketChannel`
    """

    log = txaio.make_logger()


@public
class WebSocketClientProtocol(WebSocketAdapterProtocol, protocol.WebSocketClientProtocol):
    """
    Base class for asyncio-based WebSocket client protocols.

    Implements:

    * :class:`autobahn.websocket.interfaces.IWebSocketChannel`
    """

    log = txaio.make_logger()

    def _onConnect(self, response):
        res = self.onConnect(response)
        if yields(res):
            ensure_future(res)

    def startTLS(self):
        raise Exception("WSS over explicit proxies not implemented")


class WebSocketAdapterFactory(object):
    """
    Adapter class for asyncio-based WebSocket client and server factories.
    """
    log = txaio.make_logger()

    def __call__(self):
        proto = self.protocol()
        proto.factory = self
        return proto


@public
class WebSocketServerFactory(WebSocketAdapterFactory, protocol.WebSocketServerFactory):
    """
    Base class for asyncio-based WebSocket server factories.

    Implements:

    * :class:`autobahn.websocket.interfaces.IWebSocketServerChannelFactory`
    """

    protocol = WebSocketServerProtocol

    def __init__(self, *args, **kwargs):
        """
        .. note::
            In addition to all arguments to the constructor of
            :meth:`autobahn.websocket.interfaces.IWebSocketServerChannelFactory`,
            you can supply a ``loop`` keyword argument to specify the
            asyncio event loop to be used.
        """
        loop = kwargs.pop('loop', None)
        self.loop = loop or asyncio.get_event_loop()

        protocol.WebSocketServerFactory.__init__(self, *args, **kwargs)


@public
class WebSocketClientFactory(WebSocketAdapterFactory, protocol.WebSocketClientFactory):
    """
    Base class for asyncio-based WebSocket client factories.

    Implements:

    * :class:`autobahn.websocket.interfaces.IWebSocketClientChannelFactory`
    """

    def __init__(self, *args, **kwargs):
        """

        .. note::
            In addition to all arguments to the constructor of
            :meth:`autobahn.websocket.interfaces.IWebSocketClientChannelFactory`,
            you can supply a ``loop`` keyword argument to specify the
            asyncio event loop to be used.
        """
        loop = kwargs.pop('loop', None)
        self.loop = loop or asyncio.get_event_loop()

        protocol.WebSocketClientFactory.__init__(self, *args, **kwargs)


@public
class WampWebSocketServerProtocol(websocket.WampWebSocketServerProtocol, WebSocketServerProtocol):
    """
    asyncio-based WAMP-over-WebSocket server protocol.

    Implements:

    * :class:`autobahn.wamp.interfaces.ITransport`
    """


@public
class WampWebSocketServerFactory(websocket.WampWebSocketServerFactory, WebSocketServerFactory):
    """
    asyncio-based WAMP-over-WebSocket server factory.
    """

    protocol = WampWebSocketServerProtocol

    def __init__(self, factory, *args, **kwargs):
        """

        :param factory: A callable that produces instances that implement
            :class:`autobahn.wamp.interfaces.ITransportHandler`
        :type factory: callable

        :param serializers: A list of WAMP serializers to use (or ``None``
            for all available serializers).
        :type serializers: list of objects implementing
            :class:`autobahn.wamp.interfaces.ISerializer`
        """

        serializers = kwargs.pop('serializers', None)

        websocket.WampWebSocketServerFactory.__init__(self, factory, serializers)

        kwargs['protocols'] = self._protocols

        # noinspection PyCallByClass
        WebSocketServerFactory.__init__(self, *args, **kwargs)


@public
class WampWebSocketClientProtocol(websocket.WampWebSocketClientProtocol, WebSocketClientProtocol):
    """
    asyncio-based WAMP-over-WebSocket client protocols.

    Implements:

    * :class:`autobahn.wamp.interfaces.ITransport`
    """


@public
class WampWebSocketClientFactory(websocket.WampWebSocketClientFactory, WebSocketClientFactory):
    """
    asyncio-based WAMP-over-WebSocket client factory.
    """

    protocol = WampWebSocketClientProtocol

    def __init__(self, factory, *args, **kwargs):
        """

        :param factory: A callable that produces instances that implement
            :class:`autobahn.wamp.interfaces.ITransportHandler`
        :type factory: callable

        :param serializer: The WAMP serializer to use (or ``None`` for
           "best" serializer, chosen as the first serializer available from
           this list: CBOR, MessagePack, UBJSON, JSON).
        :type serializer: object implementing :class:`autobahn.wamp.interfaces.ISerializer`
        """

        serializers = kwargs.pop('serializers', None)

        websocket.WampWebSocketClientFactory.__init__(self, factory, serializers)

        kwargs['protocols'] = self._protocols

        WebSocketClientFactory.__init__(self, *args, **kwargs)
