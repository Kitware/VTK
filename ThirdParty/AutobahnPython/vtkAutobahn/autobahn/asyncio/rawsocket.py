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

try:
    import asyncio
except ImportError:
    # trollious for py2 support - however it has been deprecated
    import trollius as asyncio
import struct
import math

from autobahn.util import public, _LazyHexFormatter
from autobahn.wamp.exception import ProtocolError, SerializationError, TransportLost
from autobahn.asyncio.util import peer2str, get_serializers
import txaio

__all__ = (
    'WampRawSocketServerProtocol',
    'WampRawSocketClientProtocol',
    'WampRawSocketServerFactory',
    'WampRawSocketClientFactory'
)

txaio.use_asyncio()

FRAME_TYPE_DATA = 0
FRAME_TYPE_PING = 1
FRAME_TYPE_PONG = 2

MAGIC_BYTE = 0x7F


class PrefixProtocol(asyncio.Protocol):

    prefix_format = '!L'
    prefix_length = struct.calcsize(prefix_format)
    max_length = 16 * 1024 * 1024
    max_length_send = max_length
    log = txaio.make_logger()  # @UndefinedVariable

    def connection_made(self, transport):
        self.transport = transport
        peer = transport.get_extra_info('peername')
        self.peer = peer2str(peer)
        self.log.debug('RawSocker Asyncio: Connection made with peer {peer}', peer=self.peer)
        self._buffer = b''
        self._header = None
        self._wait_closed = txaio.create_future()

    @property
    def is_closed(self):
        if hasattr(self, '_wait_closed'):
            return self._wait_closed
        else:
            f = txaio.create_future()
            f.set_result(True)
            return f

    def connection_lost(self, exc):
        self.log.debug('RawSocker Asyncio: Connection lost')
        self.transport = None
        self._wait_closed.set_result(True)
        self._on_connection_lost(exc)

    def _on_connection_lost(self, exc):
        pass

    def protocol_error(self, msg):
        self.log.error(msg)
        self.transport.close()

    def sendString(self, data):
        l = len(data)
        if l > self.max_length_send:
            raise ValueError('Data too big')
        header = struct.pack(self.prefix_format, len(data))
        self.transport.write(header)
        self.transport.write(data)

    def ping(self, data):
        raise NotImplementedError()

    def pong(self, data):
        raise NotImplementedError()

    def data_received(self, data):
        self._buffer += data
        pos = 0
        remaining = len(self._buffer)
        while remaining >= self.prefix_length:
            # do not recalculate header if available from previous call
            if self._header:
                frame_type, frame_length = self._header
            else:
                header = self._buffer[pos:pos + self.prefix_length]
                frame_type = ord(header[0:1]) & 0b00000111
                if frame_type > FRAME_TYPE_PONG:
                    self.protocol_error('Invalid frame type')
                    return
                frame_length = struct.unpack(self.prefix_format, b'\0' + header[1:])[0]
                if frame_length > self.max_length:
                    self.protocol_error('Frame too big')
                    return

            if remaining - self.prefix_length >= frame_length:
                self._header = None
                pos += self.prefix_length
                remaining -= self.prefix_length
                data = self._buffer[pos:pos + frame_length]
                pos += frame_length
                remaining -= frame_length

                if frame_type == FRAME_TYPE_DATA:
                    self.stringReceived(data)
                elif frame_type == FRAME_TYPE_PING:
                    self.ping(data)
                elif frame_type == FRAME_TYPE_PONG:
                    self.pong(data)
            else:
                # save heaader
                self._header = frame_type, frame_length
                break

        self._buffer = self._buffer[:remaining]

    def stringReceived(self, data):
        raise NotImplementedError()


class RawSocketProtocol(PrefixProtocol):

    def __init__(self):
        max_size = None
        if max_size:
            exp = int(math.ceil(math.log(max_size, 2))) - 9
            if exp > 15:
                raise ValueError('Maximum length is 16M')
            self.max_length = 2**(exp + 9)
            self._length_exp = exp
        else:
            self._length_exp = 15
            self.max_length = 2**24

    def connection_made(self, transport):
        PrefixProtocol.connection_made(self, transport)
        self._handshake_done = False

    def _on_handshake_complete(self):
        raise NotImplementedError()

    def parse_handshake(self):
        buf = bytearray(self._buffer[:4])
        if buf[0] != MAGIC_BYTE:
            raise HandshakeError('Invalid magic byte in handshake')
            return
        ser = buf[1] & 0x0F
        lexp = buf[1] >> 4
        self.max_length_send = 2**(lexp + 9)
        if buf[2] != 0 or buf[3] != 0:
            raise HandshakeError('Reserved bytes must be zero')
        return ser, lexp

    def process_handshake(self):
        raise NotImplementedError()

    def data_received(self, data):
        self.log.debug('RawSocker Asyncio: data received {data}', data=_LazyHexFormatter(data))
        if self._handshake_done:
            return PrefixProtocol.data_received(self, data)
        else:
            self._buffer += data
            if len(self._buffer) >= 4:
                try:
                    self.process_handshake()
                except HandshakeError as e:
                    self.protocol_error('Handshake error : {err}'.format(err=e))
                    return
                self._handshake_done = True
                self._on_handshake_complete()
                data = self._buffer[4:]
                self._buffer = b''
                if data:
                    PrefixProtocol.data_received(self, data)


ERR_SERIALIZER_UNSUPPORTED = 1

ERRMAP = {
    0: "illegal (must not be used)",
    1: "serializer unsupported",
    2: "maximum message length unacceptable",
    3: "use of reserved bits (unsupported feature)",
    4: "maximum connection count reached"
}


class HandshakeError(Exception):
    def __init__(self, msg, code=0):
        Exception.__init__(self, msg if not code else msg + ' : %s' % ERRMAP.get(code))


class RawSocketClientProtocol(RawSocketProtocol):

    def check_serializer(self, ser_id):
        return True

    def process_handshake(self):
        ser_id, err = self.parse_handshake()
        if ser_id == 0:
            raise HandshakeError('Server returned handshake error', err)
        if self.serializer_id != ser_id:
            raise HandshakeError('Server returned different serializer {0} then requested {1}'
                                 .format(ser_id, self.serializer_id))

    @property
    def serializer_id(self):
        raise NotImplementedError()

    def connection_made(self, transport):
        RawSocketProtocol.connection_made(self, transport)
        # start handshake
        hs = bytes(bytearray([MAGIC_BYTE,
                              self._length_exp << 4 | self.serializer_id,
                              0, 0]))
        transport.write(hs)
        self.log.debug('RawSocket Asyncio: Client handshake sent')


class RawSocketServerProtocol(RawSocketProtocol):

    def supports_serializer(self, ser_id):
        raise NotImplementedError()

    def process_handshake(self):
        def send_response(lexp, ser_id):
            b2 = lexp << 4 | (ser_id & 0x0f)
            self.transport.write(bytes(bytearray([MAGIC_BYTE, b2, 0, 0])))
        ser_id, _lexp = self.parse_handshake()
        if not self.supports_serializer(ser_id):
            send_response(ERR_SERIALIZER_UNSUPPORTED, 0)
            raise HandshakeError('Serializer unsupported : {ser_id}'.format(ser_id=ser_id))
        send_response(self._length_exp, ser_id)


# this is transport independent part of WAMP protocol
class WampRawSocketMixinGeneral(object):

    def _on_handshake_complete(self):
        self.log.debug("WampRawSocketProtocol: Handshake complete")
        try:
            self._session = self.factory._factory()
            self._session.onOpen(self)
        except Exception as e:
            # Exceptions raised in onOpen are fatal ..
            self.log.warn("WampRawSocketProtocol: ApplicationSession constructor / onOpen raised ({err})", err=e)
            self.abort()
        else:
            self.log.info("ApplicationSession started.")

    def stringReceived(self, payload):
        self.log.debug("WampRawSocketProtocol: RX octets: {octets}", octets=_LazyHexFormatter(payload))
        try:
            for msg in self._serializer.unserialize(payload):
                self.log.debug("WampRawSocketProtocol: RX WAMP message: {msg}", msg=msg)
                self._session.onMessage(msg)

        except ProtocolError as e:
            self.log.warn("WampRawSocketProtocol: WAMP Protocol Error ({err}) - aborting connection", err=e)
            self.abort()

        except Exception as e:
            self.log.warn("WampRawSocketProtocol: WAMP Internal Error ({err}) - aborting connection", err=e)
            self.abort()

    def send(self, msg):
        """
        Implements :func:`autobahn.wamp.interfaces.ITransport.send`
        """
        if self.isOpen():
            self.log.debug("WampRawSocketProtocol: TX WAMP message: {msg}", msg=msg)
            try:
                payload, _ = self._serializer.serialize(msg)
            except Exception as e:
                # all exceptions raised from above should be serialization errors ..
                raise SerializationError("WampRawSocketProtocol: unable to serialize WAMP application payload ({0})"
                                         .format(e))
            else:
                self.sendString(payload)
                self.log.debug("WampRawSocketProtocol: TX octets: {octets}", octets=_LazyHexFormatter(payload))
        else:
            raise TransportLost()

    def isOpen(self):
        """
        Implements :func:`autobahn.wamp.interfaces.ITransport.isOpen`
        """
        return hasattr(self, '_session') and self._session is not None


# this is asyncio dependent part of WAMP protocol
class WampRawSocketMixinAsyncio(object):
    """
    Base class for asyncio-based WAMP-over-RawSocket protocols.
    """

    def _on_connection_lost(self, exc):
        try:
            wasClean = exc is None
            self._session.onClose(wasClean)
        except Exception as e:
            # silently ignore exceptions raised here ..
            self.log.warn("WampRawSocketProtocol: ApplicationSession.onClose raised ({err})", err=e)
        self._session = None

    def close(self):
        """
        Implements :func:`autobahn.wamp.interfaces.ITransport.close`
        """
        if self.isOpen():
            self.transport.close()
        else:
            raise TransportLost()

    def abort(self):
        """
        Implements :func:`autobahn.wamp.interfaces.ITransport.abort`
        """
        if self.isOpen():
            if hasattr(self.transport, 'abort'):
                # ProcessProtocol lacks abortConnection()
                self.transport.abort()
            else:
                self.transport.close()
        else:
            raise TransportLost()


@public
class WampRawSocketServerProtocol(WampRawSocketMixinGeneral, WampRawSocketMixinAsyncio, RawSocketServerProtocol):
    """
    asyncio-based WAMP-over-RawSocket server protocol.

    Implements:

        * :class:`autobahn.wamp.interfaces.ITransport`
    """

    def supports_serializer(self, ser_id):
        if ser_id in self.factory._serializers:
            self._serializer = self.factory._serializers[ser_id]()
            self.log.debug(
                "WampRawSocketProtocol: client wants to use serializer '{serializer}'",
                serializer=ser_id,
            )
            return True
        else:
            self.log.debug(
                "WampRawSocketProtocol: opening handshake - no suitable serializer found (client requested {serializer}, and we have {serializers}",
                serializer=ser_id,
                serializers=self.factory._serializers.keys(),
            )
            self.abort()
            return False

    def get_channel_id(self, channel_id_type=u'tls-unique'):
        """
        Implements :func:`autobahn.wamp.interfaces.ITransport.get_channel_id`
        """
        return None
        # return transport_channel_id(self.transport, is_server=True, channel_id_type=channel_id_type)


@public
class WampRawSocketClientProtocol(WampRawSocketMixinGeneral, WampRawSocketMixinAsyncio, RawSocketClientProtocol):
    """
    asyncio-based WAMP-over-RawSocket client protocol.

    Implements:

        * :class:`autobahn.wamp.interfaces.ITransport`
    """

    @property
    def serializer_id(self):
        if not hasattr(self, '_serializer'):
            self._serializer = self.factory._serializer()
        return self._serializer.RAWSOCKET_SERIALIZER_ID

    def get_channel_id(self, channel_id_type=u'tls-unique'):
        """
        Implements :func:`autobahn.wamp.interfaces.ITransport.get_channel_id`
        """
        return None
        # return transport_channel_id(self.transport, is_server=False, channel_id_type=channel_id_type)


class WampRawSocketFactory(object):
    """
    Adapter class for asyncio-based WebSocket client and server factories.def dataReceived(self, data):
    """

    log = txaio.make_logger()

    @public
    def __call__(self):
        proto = self.protocol()
        proto.factory = self
        return proto


@public
class WampRawSocketServerFactory(WampRawSocketFactory):
    """
    asyncio-based WAMP-over-RawSocket server protocol factory.
    """
    protocol = WampRawSocketServerProtocol

    def __init__(self, factory, serializers=None):
        """

        :param factory: A callable that produces instances that implement
            :class:`autobahn.wamp.interfaces.ITransportHandler`
        :type factory: callable

        :param serializers: A list of WAMP serializers to use (or ``None``
            for all available serializers).
        :type serializers: list of objects implementing
            :class:`autobahn.wamp.interfaces.ISerializer`
        """
        if callable(factory):
            self._factory = factory
        else:
            self._factory = lambda: factory

        # when no serializers were requested specifically, then support
        # all that are available
        if serializers is None:
            serializers = get_serializers()

            if not serializers:
                raise Exception("could not import any WAMP serializers")

        self._serializers = {ser.RAWSOCKET_SERIALIZER_ID: ser for ser in serializers}


@public
class WampRawSocketClientFactory(WampRawSocketFactory):
    """
    asyncio-based WAMP-over-RawSocket client factory.
    """
    protocol = WampRawSocketClientProtocol

    def __init__(self, factory, serializer=None):
        """

        :param factory: A callable that produces instances that implement
            :class:`autobahn.wamp.interfaces.ITransportHandler`
        :type factory: callable

        :param serializer: The WAMP serializer to use (or ``None`` for
           "best" serializer, chosen as the first serializer available from
           this list: CBOR, MessagePack, UBJSON, JSON).
        :type serializer: object implementing :class:`autobahn.wamp.interfaces.ISerializer`
        """
        if callable(factory):
            self._factory = factory
        else:
            self._factory = lambda: factory

        # when no serializer was requested specifically, use the first
        # one available
        if serializer is None:
            serializers = get_serializers()
            if serializers:
                serializer = serializers[0]

        if serializer is None:
            raise Exception("could not import any WAMP serializer")

        self._serializer = serializer
