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

from autobahn.util import public

import json
import six

__all__ = (
    'ConnectionRequest',
    'ConnectionResponse',
    'ConnectionAccept',
    'ConnectionDeny',
    'Message',
    'IncomingMessage',
    'OutgoingMessage',
)


@public
class ConnectionRequest(object):
    """
    Thin-wrapper for WebSocket connection request information provided in
    :meth:`autobahn.websocket.protocol.WebSocketServerProtocol.onConnect` when
    a WebSocket client want to establish a connection to a WebSocket server.
    """

    __slots__ = (
        'peer',
        'headers',
        'host',
        'path',
        'params',
        'version',
        'origin',
        'protocols',
        'extensions'
    )

    def __init__(self, peer, headers, host, path, params, version, origin, protocols, extensions):
        """

        :param peer: Descriptor of the connecting client (e.g. IP address/port
            in case of TCP transports).
        :type peer: str

        :param headers: HTTP headers from opening handshake request.
        :type headers: dict

        :param host: Host from opening handshake HTTP header.
        :type host: str

        :param path: Path from requested HTTP resource URI. For example, a resource URI of
            ``/myservice?foo=23&foo=66&bar=2`` will be parsed to ``/myservice``.
        :type path: str

        :param params: Query parameters (if any) from requested HTTP resource URI.
            For example, a resource URI of ``/myservice?foo=23&foo=66&bar=2`` will be
            parsed to ``{'foo': ['23', '66'], 'bar': ['2']}``.
        :type params: dict

        :param version: The WebSocket protocol version the client announced (and will be
            spoken, when connection is accepted).
        :type version: int

        :param origin: The WebSocket origin header or None. Note that this only
            a reliable source of information for browser clients!
        :type origin: str

        :param protocols: The WebSocket (sub)protocols the client announced. You must
            select and return one of those (or ``None``) in
            :meth:`autobahn.websocket.WebSocketServerProtocol.onConnect`.
        :type protocols: list

        :param extensions: The WebSocket extensions the client requested and the
            server accepted, and thus will be spoken, once the WebSocket connection
            has been fully established.
        :type extensions: list
        """
        self.peer = peer
        self.headers = headers
        self.host = host
        self.path = path
        self.params = params
        self.version = version
        self.origin = origin
        self.protocols = protocols
        self.extensions = extensions

    def __json__(self):
        return {'peer': self.peer,
                'headers': self.headers,
                'host': self.host,
                'path': self.path,
                'params': self.params,
                'version': self.version,
                'origin': self.origin,
                'protocols': self.protocols,
                'extensions': self.extensions}

    def __str__(self):
        return json.dumps(self.__json__())


@public
class ConnectionResponse(object):
    """
    Thin-wrapper for WebSocket connection response information provided in
    :meth:`autobahn.websocket.protocol.WebSocketClientProtocol.onConnect` when
    a WebSocket server has accepted a connection request by a client.
    """

    __slots__ = (
        'peer',
        'headers',
        'version',
        'protocol',
        'extensions'
    )

    def __init__(self, peer, headers, version, protocol, extensions):
        """
        Constructor.

        :param peer: Descriptor of the connected server (e.g. IP address/port in case of TCP transport).
        :type peer: str

        :param headers: HTTP headers from opening handshake response.
        :type headers: dict

        :param version: The WebSocket protocol version that is spoken.
        :type version: int

        :param protocol: The WebSocket (sub)protocol in use.
        :type protocol: str

        :param extensions: The WebSocket extensions in use.
        :type extensions: list of str
        """
        self.peer = peer
        self.headers = headers
        self.version = version
        self.protocol = protocol
        self.extensions = extensions

    def __json__(self):
        return {'peer': self.peer,
                'headers': self.headers,
                'version': self.version,
                'protocol': self.protocol,
                'extensions': self.extensions}

    def __str__(self):
        return json.dumps(self.__json__())


@public
class ConnectionAccept(object):
    """
    Used by WebSocket servers to accept an incoming WebSocket connection.
    If the client announced one or multiple subprotocols, the server MUST
    select one of the subprotocols announced by the client.
    """

    __slots__ = ('subprotocol', 'headers')

    def __init__(self, subprotocol=None, headers=None):
        """

        :param subprotocol: The WebSocket connection is accepted with the
            this WebSocket subprotocol chosen. The value must be a token
            as defined by RFC 2616.
        :type subprotocol: unicode or None

        :param headers: Additional HTTP headers to send on the WebSocket
            opening handshake reply, e.g. cookies. The keys must be unicode,
            and the values either unicode or tuple/list. In the latter case
            a separate HTTP header line will be sent for each item in
            tuple/list.
        :type headers: dict or None
        """
        assert(subprotocol is None or type(subprotocol) == six.text_type)
        assert(headers is None or type(headers) == dict)
        if headers is not None:
            for k, v in headers.items():
                assert(type(k) == six.text_type)
                assert(type(v) == six.text_type or type(v) == list or type(v) == tuple)
                if type(v) == list or type(v) == tuple:
                    for vv in v:
                        assert(type(vv) == six.text_type)

        self.subprotocol = subprotocol
        self.headers = headers


@public
class ConnectionDeny(Exception):
    """
    Throw an instance of this class to deny a WebSocket connection
    during handshake in :meth:`autobahn.websocket.protocol.WebSocketServerProtocol.onConnect`.
    """

    __slots__ = ('code', 'reason')

    BAD_REQUEST = 400
    """
    Bad Request. The request cannot be fulfilled due to bad syntax.
    """

    FORBIDDEN = 403
    """
    Forbidden. The request was a legal request, but the server is refusing to respond to it.[2] Unlike a 401 Unauthorized response, authenticating will make no difference.
    """

    NOT_FOUND = 404
    """
    Not Found. The requested resource could not be found but may be available again in the future.[2] Subsequent requests by the client are permissible.
    """

    NOT_ACCEPTABLE = 406
    """
    Not Acceptable. The requested resource is only capable of generating content not acceptable according to the Accept headers sent in the request.
    """

    REQUEST_TIMEOUT = 408
    """
    Request Timeout. The server timed out waiting for the request. According to W3 HTTP specifications: 'The client did not produce a request within the time that the server was prepared to wait. The client MAY repeat the request without modifications at any later time.
    """

    INTERNAL_SERVER_ERROR = 500
    """
    Internal Server Error. A generic error message, given when no more specific message is suitable.
    """

    NOT_IMPLEMENTED = 501
    """
    Not Implemented. The server either does not recognize the request method, or it lacks the ability to fulfill the request.
    """

    SERVICE_UNAVAILABLE = 503
    """
    Service Unavailable. The server is currently unavailable (because it is overloaded or down for maintenance). Generally, this is a temporary state.
    """

    def __init__(self, code, reason=None):
        """

        :param code: HTTP error code.
        :type code: int

        :param reason: HTTP error reason.
        :type reason: unicode
        """
        assert(type(code) == int)
        assert(reason is None or type(reason) == six.text_type)

        self.code = code
        self.reason = reason


class Message(object):
    """
    Abstract base class for WebSocket messages.
    """

    __slots__ = ()


class IncomingMessage(Message):
    """
    An incoming WebSocket message.
    """

    __slots__ = ('payload', 'is_binary')

    def __init__(self, payload, is_binary=False):
        """

        :param payload: The WebSocket message payload, which can be UTF-8
            encoded text or a binary string.
        :type payload: bytes

        :param is_binary: ``True`` for binary payload, else the payload
            contains UTF-8 encoded text.
        :type is_binary: bool
        """
        assert(type(payload) == bytes)
        assert(type(is_binary) == bool)

        self.payload = payload
        self.is_binary = is_binary


class OutgoingMessage(Message):
    """
    An outgoing WebSocket message.
    """

    __slots__ = ('payload', 'is_binary', 'skip_compress')

    def __init__(self, payload, is_binary=False, skip_compress=False):
        """

        :param payload: The WebSocket message payload, which can be UTF-8
            encoded text or a binary string.
        :type payload: bytes

        :param is_binary: ``True`` iff payload is binary, else the payload
            contains UTF-8 encoded text.
        :type is_binary: bool

        :param skip_compress: If ``True``, never compress this message.
            This only has an effect when WebSocket compression has been negotiated
            on the WebSocket connection. Use when you know the payload is
            incompressible (e.g. encrypted or already compressed).
        :type skip_compress: bool
        """
        assert(type(payload) == bytes)
        assert(type(is_binary) == bool)
        assert(type(skip_compress) == bool)

        self.payload = payload
        self.is_binary = is_binary
        self.skip_compress = skip_compress


class Ping(object):
    """
    A WebSocket ping message.
    """

    __slots__ = ('payload')

    def __init__(self, payload=None):
        """

        :param payload: The WebSocket ping message payload.
        :type payload: bytes or None
        """
        assert(payload is None or type(payload) == bytes), \
            ("invalid type {} for WebSocket ping payload - must be None or bytes".format(type(payload)))
        if payload is not None:
            assert(len(payload) < 126), \
                ("WebSocket ping payload too long ({} bytes) - must be <= 125 bytes".format(len(payload)))

        self.payload = payload
