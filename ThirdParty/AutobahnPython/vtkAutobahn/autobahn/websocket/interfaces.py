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

import abc
import six

from autobahn.util import public

__all__ = ('IWebSocketServerChannelFactory',
           'IWebSocketClientChannelFactory',
           'IWebSocketChannel',
           'IWebSocketChannelFrameApi',
           'IWebSocketChannelStreamingApi')


@public
@six.add_metaclass(abc.ABCMeta)
class IWebSocketServerChannelFactory(object):
    """
    WebSocket server protocol factories implement this interface, and create
    protocol instances which in turn implement
    :class:`autobahn.websocket.interfaces.IWebSocketChannel`.
    """

    @abc.abstractmethod
    def __init__(self,
                 url=None,
                 protocols=None,
                 server=None,
                 headers=None,
                 externalPort=None):
        """

        :param url: The WebSocket URL this factory is working for, e.g. ``ws://myhost.com/somepath``.
                    For non-TCP transports like pipes or Unix domain sockets, provide ``None``.
                    This will use an implicit URL of ``ws://localhost``.
        :type url: str

        :param protocols: List of subprotocols the server supports. The subprotocol used is the first from the list of subprotocols announced by the client that is contained in this list.
        :type protocols: list of str

        :param server: Server as announced in HTTP response header during opening handshake.
        :type server: str

        :param headers: An optional mapping of additional HTTP headers to send during the WebSocket opening handshake.
        :type headers: dict

        :param externalPort: Optionally, the external visible port this server will be reachable under (i.e. when running behind a L2/L3 forwarding device).
        :type externalPort: int
        """

    @public
    @abc.abstractmethod
    def setSessionParameters(self,
                             url=None,
                             protocols=None,
                             server=None,
                             headers=None,
                             externalPort=None):
        """
        Set WebSocket session parameters.

        :param url: The WebSocket URL this factory is working for, e.g. ``ws://myhost.com/somepath``.
                    For non-TCP transports like pipes or Unix domain sockets, provide ``None``.
                    This will use an implicit URL of ``ws://localhost``.
        :type url: str

        :param protocols: List of subprotocols the server supports. The subprotocol used is the first from the list of subprotocols announced by the client that is contained in this list.
        :type protocols: list of str

        :param server: Server as announced in HTTP response header during opening handshake.
        :type server: str

        :param headers: An optional mapping of additional HTTP headers to send during the WebSocket opening handshake.
        :type headers: dict

        :param externalPort: Optionally, the external visible port this server will be reachable under (i.e. when running behind a L2/L3 forwarding device).
        :type externalPort: int
        """

    @public
    @abc.abstractmethod
    def setProtocolOptions(self,
                           versions=None,
                           webStatus=None,
                           utf8validateIncoming=None,
                           maskServerFrames=None,
                           requireMaskedClientFrames=None,
                           applyMask=None,
                           maxFramePayloadSize=None,
                           maxMessagePayloadSize=None,
                           autoFragmentSize=None,
                           failByDrop=None,
                           echoCloseCodeReason=None,
                           openHandshakeTimeout=None,
                           closeHandshakeTimeout=None,
                           tcpNoDelay=None,
                           perMessageCompressionAccept=None,
                           autoPingInterval=None,
                           autoPingTimeout=None,
                           autoPingSize=None,
                           serveFlashSocketPolicy=None,
                           flashSocketPolicy=None,
                           allowedOrigins=None,
                           allowNullOrigin=False,
                           maxConnections=None,
                           trustXForwardedFor=0):
        """
        Set WebSocket protocol options used as defaults for new protocol instances.

        :param versions: The WebSocket protocol versions accepted by the server (default: :func:`autobahn.websocket.protocol.WebSocketProtocol.SUPPORTED_PROTOCOL_VERSIONS`).
        :type versions: list of ints or None

        :param webStatus: Return server status/version on HTTP/GET without WebSocket upgrade header (default: `True`).
        :type webStatus: bool or None

        :param utf8validateIncoming: Validate incoming UTF-8 in text message payloads (default: `True`).
        :type utf8validateIncoming: bool or None

        :param maskServerFrames: Mask server-to-client frames (default: `False`).
        :type maskServerFrames: bool or None

        :param requireMaskedClientFrames: Require client-to-server frames to be masked (default: `True`).
        :type requireMaskedClientFrames: bool or None

        :param applyMask: Actually apply mask to payload when mask it present. Applies for outgoing and incoming frames (default: `True`).
        :type applyMask: bool or None

        :param maxFramePayloadSize: Maximum frame payload size that will be accepted when receiving or `0` for unlimited (default: `0`).
        :type maxFramePayloadSize: int or None

        :param maxMessagePayloadSize: Maximum message payload size (after reassembly of fragmented messages) that will be accepted when receiving or `0` for unlimited (default: `0`).
        :type maxMessagePayloadSize: int or None

        :param autoFragmentSize: Automatic fragmentation of outgoing data messages (when using the message-based API) into frames with payload length `<=` this size or `0` for no auto-fragmentation (default: `0`).
        :type autoFragmentSize: int or None

        :param failByDrop: Fail connections by dropping the TCP connection without performing closing handshake (default: `True`).
        :type failbyDrop: bool or None

        :param echoCloseCodeReason: Iff true, when receiving a close, echo back close code/reason. Otherwise reply with `code == 1000, reason = ""` (default: `False`).
        :type echoCloseCodeReason: bool or None

        :param openHandshakeTimeout: Opening WebSocket handshake timeout, timeout in seconds or `0` to deactivate (default: `0`).
        :type openHandshakeTimeout: float or None

        :param closeHandshakeTimeout: When we expect to receive a closing handshake reply, timeout in seconds (default: `1`).
        :type closeHandshakeTimeout: float or None

        :param tcpNoDelay: TCP NODELAY ("Nagle") socket option (default: `True`).
        :type tcpNoDelay: bool or None

        :param perMessageCompressionAccept: Acceptor function for offers.
        :type perMessageCompressionAccept: callable or None

        :param autoPingInterval: Automatically send WebSocket pings every given seconds. When the peer does not respond
           in `autoPingTimeout`, drop the connection. Set to `0` to disable. (default: `0`).
        :type autoPingInterval: float or None

        :param autoPingTimeout: Wait this many seconds for the peer to respond to automatically sent pings. If the
           peer does not respond in time, drop the connection. Set to `0` to disable. (default: `0`).
        :type autoPingTimeout: float or None

        :param autoPingSize: Payload size for automatic pings/pongs. Must be an integer from `[4, 125]`. (default: `4`).
        :type autoPingSize: int or None

        :param serveFlashSocketPolicy: Serve the Flash Socket Policy when we receive a policy file request on this protocol. (default: `False`).
        :type serveFlashSocketPolicy: bool or None

        :param flashSocketPolicy: The flash socket policy to be served when we are serving the Flash Socket Policy on this protocol
           and when Flash tried to connect to the destination port. It must end with a null character.
        :type flashSocketPolicy: str or None

        :param allowedOrigins: A list of allowed WebSocket origins (with '*' as a wildcard character).
        :type allowedOrigins: list or None

        :param allowNullOrigin: if True, allow WebSocket connections whose `Origin:` is `"null"`.
        :type allowNullOrigin: bool

        :param maxConnections: Maximum number of concurrent connections. Set to `0` to disable (default: `0`).
        :type maxConnections: int or None

        :param trustXForwardedFor: Number of trusted web servers in front of this server that add their own X-Forwarded-For header (default: `0`)
        :type trustXForwardedFor: int
        """

    @public
    @abc.abstractmethod
    def resetProtocolOptions(self):
        """
        Reset all WebSocket protocol options to defaults.
        """


@public
@six.add_metaclass(abc.ABCMeta)
class IWebSocketClientChannelFactory(object):
    """
    WebSocket client protocol factories implement this interface, and create
    protocol instances which in turn implement
    :class:`autobahn.websocket.interfaces.IWebSocketChannel`.
    """

    @abc.abstractmethod
    def __init__(self,
                 url=None,
                 origin=None,
                 protocols=None,
                 useragent=None,
                 headers=None,
                 proxy=None):
        """

        Note that you MUST provide URL either here or set using
        :meth:`autobahn.websocket.WebSocketClientFactory.setSessionParameters`
        *before* the factory is started.

        :param url: WebSocket URL this factory will connect to, e.g. ``ws://myhost.com/somepath?param1=23``.
                    For non-TCP transports like pipes or Unix domain sockets, provide ``None``.
                    This will use an implicit URL of ``ws://localhost``.
        :type url: str

        :param origin: The origin to be sent in WebSocket opening handshake or None (default: `None`).
        :type origin: str

        :param protocols: List of subprotocols the client should announce in WebSocket opening handshake (default: `[]`).
        :type protocols: list of strings

        :param useragent: User agent as announced in HTTP request header or None (default: `AutobahnWebSocket/?.?.?`).
        :type useragent: str

        :param headers: An optional mapping of additional HTTP headers to send during the WebSocket opening handshake.
        :type headers: dict

        :param proxy: Explicit proxy server to use; a dict with ``host`` and ``port`` keys
        :type proxy: dict or None
        """

    @public
    @abc.abstractmethod
    def setSessionParameters(self,
                             url=None,
                             origin=None,
                             protocols=None,
                             useragent=None,
                             headers=None,
                             proxy=None):
        """
        Set WebSocket session parameters.

        :param url: WebSocket URL this factory will connect to, e.g. `ws://myhost.com/somepath?param1=23`.
                    For non-TCP transports like pipes or Unix domain sockets, provide `None`.
                    This will use an implicit URL of `ws://localhost`.
        :type url: str

        :param origin: The origin to be sent in opening handshake.
        :type origin: str

        :param protocols: List of WebSocket subprotocols the client should announce in opening handshake.
        :type protocols: list of strings

        :param useragent: User agent as announced in HTTP request header during opening handshake.
        :type useragent: str

        :param headers: An optional mapping of additional HTTP headers to send during the WebSocket opening handshake.
        :type headers: dict

        :param proxy: (Optional) a dict with ``host`` and ``port`` keys specifying a proxy to use
        :type proxy: dict or None
        """

    @public
    @abc.abstractmethod
    def setProtocolOptions(self,
                           version=None,
                           utf8validateIncoming=None,
                           acceptMaskedServerFrames=None,
                           maskClientFrames=None,
                           applyMask=None,
                           maxFramePayloadSize=None,
                           maxMessagePayloadSize=None,
                           autoFragmentSize=None,
                           failByDrop=None,
                           echoCloseCodeReason=None,
                           serverConnectionDropTimeout=None,
                           openHandshakeTimeout=None,
                           closeHandshakeTimeout=None,
                           tcpNoDelay=None,
                           perMessageCompressionOffers=None,
                           perMessageCompressionAccept=None,
                           autoPingInterval=None,
                           autoPingTimeout=None,
                           autoPingSize=None):
        """
        Set WebSocket protocol options used as defaults for _new_ protocol instances.

        :param version: The WebSocket protocol spec (draft) version to be used (default: :func:`autobahn.websocket.protocol.WebSocketProtocol.SUPPORTED_PROTOCOL_VERSIONS`).
        :type version: int

        :param utf8validateIncoming: Validate incoming UTF-8 in text message payloads (default: `True`).
        :type utf8validateIncoming: bool

        :param acceptMaskedServerFrames: Accept masked server-to-client frames (default: `False`).
        :type acceptMaskedServerFrames: bool

        :param maskClientFrames: Mask client-to-server frames (default: `True`).
        :type maskClientFrames: bool

        :param applyMask: Actually apply mask to payload when mask it present. Applies for outgoing and incoming frames (default: `True`).
        :type applyMask: bool

        :param maxFramePayloadSize: Maximum frame payload size that will be accepted when receiving or `0` for unlimited (default: `0`).
        :type maxFramePayloadSize: int

        :param maxMessagePayloadSize: Maximum message payload size (after reassembly of fragmented messages) that will be accepted when receiving or `0` for unlimited (default: `0`).
        :type maxMessagePayloadSize: int

        :param autoFragmentSize: Automatic fragmentation of outgoing data messages (when using the message-based API) into frames with payload length `<=` this size or `0` for no auto-fragmentation (default: `0`).
        :type autoFragmentSize: int

        :param failByDrop: Fail connections by dropping the TCP connection without performing closing handshake (default: `True`).
        :type failbyDrop: bool

        :param echoCloseCodeReason: Iff true, when receiving a close, echo back close code/reason. Otherwise reply with `code == 1000, reason = ""` (default: `False`).
        :type echoCloseCodeReason: bool

        :param serverConnectionDropTimeout: When the client expects the server to drop the TCP, timeout in seconds (default: `1`).
        :type serverConnectionDropTimeout: float

        :param openHandshakeTimeout: Opening WebSocket handshake timeout, timeout in seconds or `0` to deactivate (default: `0`).
        :type openHandshakeTimeout: float

        :param closeHandshakeTimeout: When we expect to receive a closing handshake reply, timeout in seconds (default: `1`).
        :type closeHandshakeTimeout: float

        :param tcpNoDelay: TCP NODELAY ("Nagle"): bool socket option (default: `True`).
        :type tcpNoDelay: bool

        :param perMessageCompressionOffers: A list of offers to provide to the server for the permessage-compress WebSocket extension. Must be a list of instances of subclass of PerMessageCompressOffer.
        :type perMessageCompressionOffers: list of instance of subclass of PerMessageCompressOffer

        :param perMessageCompressionAccept: Acceptor function for responses.
        :type perMessageCompressionAccept: callable

        :param autoPingInterval: Automatically send WebSocket pings every given seconds. When the peer does not respond
           in `autoPingTimeout`, drop the connection. Set to `0` to disable. (default: `0`).
        :type autoPingInterval: float or None

        :param autoPingTimeout: Wait this many seconds for the peer to respond to automatically sent pings. If the
           peer does not respond in time, drop the connection. Set to `0` to disable. (default: `0`).
        :type autoPingTimeout: float or None

        :param autoPingSize: Payload size for automatic pings/pongs. Must be an integer from `[4, 125]`. (default: `4`).
        :type autoPingSize: int
        """

    @public
    @abc.abstractmethod
    def resetProtocolOptions(self):
        """
        Reset all WebSocket protocol options to defaults.
        """


@public
@six.add_metaclass(abc.ABCMeta)
class IWebSocketChannel(object):
    """
    A WebSocket channel is a bidirectional, full-duplex, ordered, reliable message channel
    over a WebSocket connection as specified in RFC6455.

    This interface defines a message-based API to WebSocket plus auxiliary hooks
    and methods.
    """

    @public
    @abc.abstractmethod
    def onConnect(self, request_or_response):
        """
        Callback fired during WebSocket opening handshake when a client connects (to a server with
        request from client) or when server connection established (by a client with response from
        server). This method may run asynchronous code.

        :param request_or_response: Connection request (for servers) or response (for clients).
        :type request_or_response: Instance of :class:`autobahn.websocket.types.ConnectionRequest`
           or :class:`autobahn.websocket.types.ConnectionResponse`.

        :returns:
           When this callback is fired on a WebSocket server, you may return either ``None`` (in
           which case the connection is accepted with no specific WebSocket subprotocol) or
           an str instance with the name of the WebSocket subprotocol accepted.
           When the callback is fired on a WebSocket client, this method must return ``None``.
           To deny a connection, raise an Exception.
           You can also return a Deferred/Future that resolves/rejects to the above.
        """

    @public
    @abc.abstractmethod
    def onOpen(self):
        """
        Callback fired when the initial WebSocket opening handshake was completed.
        You now can send and receive WebSocket messages.
        """

    @public
    @abc.abstractmethod
    def sendMessage(self, payload, isBinary):
        """
        Send a WebSocket message over the connection to the peer.

        :param payload: The WebSocket message to be sent.
        :type payload: bytes

        :param isBinary: Flag indicating whether payload is binary or
            UTF-8 encoded text.
        :type isBinary: bool
        """

    @public
    @abc.abstractmethod
    def onMessage(self, payload, isBinary):
        """
        Callback fired when a complete WebSocket message was received.

        :param payload: The WebSocket message received.
        :type payload: bytes

        :param isBinary: Flag indicating whether payload is binary or
            UTF-8 encoded text.
        :type isBinary: bool
        """

    @public
    @abc.abstractmethod
    def sendClose(self, code=None, reason=None):
        """
        Starts a WebSocket closing handshake tearing down the WebSocket connection.

        :param code: An optional close status code (``1000`` for normal close or ``3000-4999`` for
           application specific close).
        :type code: int
        :param reason: An optional close reason (a string that when present, a status
           code MUST also be present).
        :type reason: str
        """

    @public
    @abc.abstractmethod
    def onClose(self, wasClean, code, reason):
        """
        Callback fired when the WebSocket connection has been closed (WebSocket closing
        handshake has been finished or the connection was closed uncleanly).

        :param wasClean: ``True`` iff the WebSocket connection was closed cleanly.
        :type wasClean: bool

        :param code: Close status code as sent by the WebSocket peer.
        :type code: int or None

        :param reason: Close reason as sent by the WebSocket peer.
        :type reason: str or None
        """

    @abc.abstractmethod
    def sendPing(self, payload=None):
        """
        Send a WebSocket ping to the peer.

        A peer is expected to pong back the payload a soon as "practical". When more than
        one ping is outstanding at a peer, the peer may elect to respond only to the last ping.

        :param payload: An (optional) arbitrary payload of length **less than 126** octets.
        :type payload: bytes or None
        """

    @abc.abstractmethod
    def onPing(self, payload):
        """
        Callback fired when a WebSocket ping was received. A default implementation responds
        by sending a WebSocket pong.

        :param payload: Payload of ping (when there was any). Can be arbitrary, up to `125` octets.
        :type payload: bytes
        """

    @abc.abstractmethod
    def sendPong(self, payload=None):
        """
        Send a WebSocket pong to the peer.

        A WebSocket pong may be sent unsolicited. This serves as a unidirectional heartbeat.
        A response to an unsolicited pong is "not expected".

        :param payload: An (optional) arbitrary payload of length < 126 octets.
        :type payload: bytes
        """

    @abc.abstractmethod
    def onPong(self, payload):
        """
        Callback fired when a WebSocket pong was received. A default implementation does nothing.

        :param payload: Payload of pong (when there was any). Can be arbitrary, up to 125 octets.
        :type payload: bytes
        """


class IWebSocketChannelFrameApi(IWebSocketChannel):
    """
    Frame-based API to a WebSocket channel.
    """

    @abc.abstractmethod
    def onMessageBegin(self, isBinary):
        """
        Callback fired when receiving of a new WebSocket message has begun.

        :param isBinary: ``True`` if payload is binary, else the payload is UTF-8 encoded text.
        :type isBinary: bool
        """

    @abc.abstractmethod
    def onMessageFrame(self, payload):
        """
        Callback fired when a complete WebSocket message frame for a previously begun
        WebSocket message has been received.

        :param payload: Message frame payload (a list of chunks received).
        :type payload: list of bytes
        """

    @abc.abstractmethod
    def onMessageEnd(self):
        """
        Callback fired when a WebSocket message has been completely received (the last
        WebSocket frame for that message has been received).
        """

    @abc.abstractmethod
    def beginMessage(self, isBinary=False, doNotCompress=False):
        """
        Begin sending a new WebSocket message.

        :param isBinary: ``True`` if payload is binary, else the payload must be UTF-8 encoded text.
        :type isBinary: bool
        :param doNotCompress: If ``True``, never compress this message. This only applies to
           Hybi-Mode and only when WebSocket compression has been negotiated on the WebSocket
           connection. Use when you know the payload incompressible (e.g. encrypted or
           already compressed).
        :type doNotCompress: bool
        """

    @abc.abstractmethod
    def sendMessageFrame(self, payload, sync=False):
        """
        When a message has been previously begun, send a complete message frame in one go.

        :param payload: The message frame payload. When sending a text message, the payload must
                        be UTF-8 encoded already.
        :type payload: bytes
        :param sync: If ``True``, try to force data onto the wire immediately.

           .. warning::
              Do NOT use this feature for normal applications.
              Performance likely will suffer significantly.
              This feature is mainly here for use by Autobahn|Testsuite.
        :type sync: bool
        """

    @abc.abstractmethod
    def endMessage(self):
        """
        End a message previously begun message. No more frames may be sent (for that message).
        You have to begin a new message before sending again.
        """


class IWebSocketChannelStreamingApi(IWebSocketChannelFrameApi):
    """
    Streaming API to a WebSocket channel.
    """

    @abc.abstractmethod
    def onMessageFrameBegin(self, length):
        """
        Callback fired when receiving a new message frame has begun.
        A default implementation will prepare to buffer message frame data.

        :param length: Payload length of message frame which is subsequently received.
        :type length: int
        """

    @abc.abstractmethod
    def onMessageFrameData(self, payload):
        """
        Callback fired when receiving data within a previously begun message frame.
        A default implementation will buffer data for frame.

        :param payload: Partial payload for message frame.
        :type payload: bytes
        """

    @abc.abstractmethod
    def onMessageFrameEnd(self):
        """
        Callback fired when a previously begun message frame has been completely received.
        A default implementation will flatten the buffered frame data and
        fire `onMessageFrame`.
        """

    @abc.abstractmethod
    def beginMessageFrame(self, length):
        """
        Begin sending a new message frame.

        :param length: Length of the frame which is to be started. Must be less or equal **2^63**.
        :type length: int
        """

    @abc.abstractmethod
    def sendMessageFrameData(self, payload, sync=False):
        """
        Send out data when within a message frame (message was begun, frame was begun).
        Note that the frame is automatically ended when enough data has been sent.
        In other words, there is no ``endMessageFrame``, since you have begun the frame
        specifying the frame length, which implicitly defined the frame end. This is different
        from messages, which you begin *and* end explicitly , since a message can contain
        an unlimited number of frames.

        :param payload: Frame payload to send.
        :type payload: bytes
        :param sync: If ``True``, try to force data onto the wire immediately.

           .. warning::
              Do NOT use this feature for normal applications.
              Performance likely will suffer significantly.
              This feature is mainly here for use by Autobahn|Testsuite.
        :type sync: bool

        :returns: When the currently sent message frame is still incomplete, returns octets
           remaining to be sent. When the frame is complete, returns **0**. Otherwise the amount
           of unconsumed data in payload argument is returned.
        :rtype: int
        """
