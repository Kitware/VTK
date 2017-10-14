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

import binascii
import hashlib
import base64
import struct
import random
import os
import pickle
import copy
import json
import six

from pprint import pformat
from collections import deque

from autobahn import __version__

from autobahn.websocket.interfaces import IWebSocketChannel, \
    IWebSocketChannelFrameApi, \
    IWebSocketChannelStreamingApi

from autobahn.websocket.types import ConnectionRequest, ConnectionResponse, ConnectionDeny

from autobahn.util import Stopwatch, newid, wildcards2patterns, encode_truncate
from autobahn.util import _LazyHexFormatter
from autobahn.websocket.utf8validator import Utf8Validator
from autobahn.websocket.xormasker import XorMaskerNull, create_xor_masker
from autobahn.websocket.compress import PERMESSAGE_COMPRESSION_EXTENSION
from autobahn.websocket.util import parse_url

from six.moves import urllib
import txaio

if six.PY3:
    # Python 3
    # noinspection PyShadowingBuiltins
    xrange = range

__all__ = ("WebSocketProtocol",
           "WebSocketFactory",
           "WebSocketServerProtocol",
           "WebSocketServerFactory",
           "WebSocketClientProtocol",
           "WebSocketClientFactory")


def _url_to_origin(url):
    """
    Given an RFC6455 Origin URL, this returns the (scheme, host, port)
    triple. If there's no port, and the scheme isn't http or https
    then port will be None
    """
    if url.lower() == 'null':
        return 'null'

    res = urllib.parse.urlsplit(url)
    scheme = res.scheme.lower()
    if scheme == 'file':
        # when browsing local files, Chrome sends file:// URLs,
        # Firefox sends 'null'
        return 'null'

    host = res.hostname
    port = res.port
    if port is None:
        try:
            port = {'https': 443, 'http': 80}[scheme]
        except KeyError:
            port = None

    if not host:
        raise ValueError("No host part in Origin '{}'".format(url))
    return scheme, host, port


def _is_same_origin(websocket_origin, host_scheme, host_port, host_policy):
    """
    Internal helper. Returns True if the provided websocket_origin
    triple should be considered valid given the provided policy and
    expected host_port.

    Currently, the policy is just the list of allowedOriginsPatterns
    from the WebSocketProtocol instance. Schemes and ports are matched
    first, and only if there is not a mismatch do we compare each
    allowed-origin pattern against the host.
    """

    if websocket_origin == 'null':
        # nothing is the same as the null origin
        return False

    if not isinstance(websocket_origin, tuple) or not len(websocket_origin) == 3:
        raise ValueError("'websocket_origin' must be a 3-tuple")

    (origin_scheme, origin_host, origin_port) = websocket_origin

    # so, theoretically we should match on the 3-tuple of (scheme,
    # origin, port) to follow the RFC. However, the existing API just
    # allows you to pass a list of regular expressions that match
    # against the Origin header -- so to keep that API working, we
    # just match a reconstituted/sanitized Origin line against the
    # regular expressions. We *do* explicitly match the scheme first,
    # however!

    # therefore, the default of "*" will still match everything (even
    # if things are on weird ports). To be "actually secure" and pass
    # explicit ports, you can put it in your matcher
    # (e.g. "https://*.example.com:1234")

    template = '{scheme}://{host}:{port}'
    origin_header = template.format(
        scheme=origin_scheme,
        host=origin_host,
        port=origin_port,
    )
    # so, this will be matching against e.g. "http://example.com:8080"
    for origin_pattern in host_policy:
        if origin_pattern.match(origin_header):
            return True

    return False


class TrafficStats(object):

    def __init__(self):
        self.reset()

    def reset(self):
        # all of the following only tracks data messages, not control frames, not handshaking
        #
        self.outgoingOctetsWireLevel = 0
        self.outgoingOctetsWebSocketLevel = 0
        self.outgoingOctetsAppLevel = 0
        self.outgoingWebSocketFrames = 0
        self.outgoingWebSocketMessages = 0

        self.incomingOctetsWireLevel = 0
        self.incomingOctetsWebSocketLevel = 0
        self.incomingOctetsAppLevel = 0
        self.incomingWebSocketFrames = 0
        self.incomingWebSocketMessages = 0

        # the following track any traffic before the WebSocket connection
        # reaches STATE_OPEN (this includes WebSocket opening handshake
        # proxy handling and such)
        self.preopenOutgoingOctetsWireLevel = 0
        self.preopenIncomingOctetsWireLevel = 0

    def __json__(self):

        # compression ratio = compressed size / uncompressed size
        #
        if self.outgoingOctetsAppLevel > 0:
            outgoingCompressionRatio = float(self.outgoingOctetsWebSocketLevel) / float(self.outgoingOctetsAppLevel)
        else:
            outgoingCompressionRatio = None
        if self.incomingOctetsAppLevel > 0:
            incomingCompressionRatio = float(self.incomingOctetsWebSocketLevel) / float(self.incomingOctetsAppLevel)
        else:
            incomingCompressionRatio = None

        # protocol overhead = non-payload size / payload size
        #
        if self.outgoingOctetsWebSocketLevel > 0:
            outgoingWebSocketOverhead = float(self.outgoingOctetsWireLevel - self.outgoingOctetsWebSocketLevel) / float(self.outgoingOctetsWebSocketLevel)
        else:
            outgoingWebSocketOverhead = None
        if self.incomingOctetsWebSocketLevel > 0:
            incomingWebSocketOverhead = float(self.incomingOctetsWireLevel - self.incomingOctetsWebSocketLevel) / float(self.incomingOctetsWebSocketLevel)
        else:
            incomingWebSocketOverhead = None

        return {'outgoingOctetsWireLevel': self.outgoingOctetsWireLevel,
                'outgoingOctetsWebSocketLevel': self.outgoingOctetsWebSocketLevel,
                'outgoingOctetsAppLevel': self.outgoingOctetsAppLevel,
                'outgoingCompressionRatio': outgoingCompressionRatio,
                'outgoingWebSocketOverhead': outgoingWebSocketOverhead,
                'outgoingWebSocketFrames': self.outgoingWebSocketFrames,
                'outgoingWebSocketMessages': self.outgoingWebSocketMessages,
                'preopenOutgoingOctetsWireLevel': self.preopenOutgoingOctetsWireLevel,

                'incomingOctetsWireLevel': self.incomingOctetsWireLevel,
                'incomingOctetsWebSocketLevel': self.incomingOctetsWebSocketLevel,
                'incomingOctetsAppLevel': self.incomingOctetsAppLevel,
                'incomingCompressionRatio': incomingCompressionRatio,
                'incomingWebSocketOverhead': incomingWebSocketOverhead,
                'incomingWebSocketFrames': self.incomingWebSocketFrames,
                'incomingWebSocketMessages': self.incomingWebSocketMessages,
                'preopenIncomingOctetsWireLevel': self.preopenIncomingOctetsWireLevel}

    def __str__(self):
        return json.dumps(self.__json__())


class FrameHeader(object):
    """
    Thin-wrapper for storing WebSocket frame metadata.

    FOR INTERNAL USE ONLY!
    """

    def __init__(self, opcode, fin, rsv, length, mask):
        """
        Constructor.

        :param opcode: Frame opcode (0-15).
        :type opcode: int
        :param fin: Frame FIN flag.
        :type fin: bool
        :param rsv: Frame reserved flags (0-7).
        :type rsv: int
        :param length: Frame payload length.
        :type length: int
        :param mask: Frame mask (binary string) or None.
        :type mask: str
        """
        self.opcode = opcode
        self.fin = fin
        self.rsv = rsv
        self.length = length
        self.mask = mask


def parseHttpHeader(data):
    """
    Parses the beginning of a HTTP request header (the data up to the \n\n line) into a pair
    of status line and HTTP headers dictionary.
    Header keys are normalized to all-lower-case.

    FOR INTERNAL USE ONLY!

    :param data: The HTTP header data up to the \n\n line.
    :type data: bytes

    :returns: tuple -- Tuple of HTTP status line, headers and headers count.
    """
    # By default, message header field parameters in Hypertext Transfer
    # Protocol (HTTP) messages cannot carry characters outside the ISO-
    # 8859-1 character set.
    #
    # See:
    #   - http://tools.ietf.org/html/rfc5987
    #   - https://github.com/crossbario/autobahn-python/issues/533
    #
    raw = data.decode('iso-8859-1').splitlines()
    http_status_line = raw[0].strip()
    http_headers = {}
    http_headers_cnt = {}
    for h in raw[1:]:
        i = h.find(":")
        if i > 0:
            # HTTP header keys are case-insensitive
            key = h[:i].strip().lower()
            value = h[i + 1:].strip()

            # handle HTTP headers split across multiple lines
            if key in http_headers:
                http_headers[key] += ", %s" % value
                http_headers_cnt[key] += 1
            else:
                http_headers[key] = value
                http_headers_cnt[key] = 1
        else:
            # skip bad HTTP header
            pass
    return http_status_line, http_headers, http_headers_cnt


class Timings(object):
    """
    Helper class to track timings by key. This class also supports item access,
    iteration and conversion to string.
    """

    def __init__(self):
        self._stopwatch = Stopwatch()
        self._timings = {}

    def track(self, key):
        """
        Track elapsed for key.

        :param key: Key under which to track the timing.
        :type key: str
        """
        self._timings[key] = self._stopwatch.elapsed()

    def diff(self, startKey, endKey, formatted=True):
        """
        Get elapsed difference between two previously tracked keys.

        :param startKey: First key for interval (older timestamp).
        :type startKey: str
        :param endKey: Second key for interval (younger timestamp).
        :type endKey: str
        :param formatted: If ``True``, format computed time period and return string.
        :type formatted: bool

        :returns: float or str -- Computed time period in seconds (or formatted string).
        """
        if endKey in self._timings and startKey in self._timings:
            d = self._timings[endKey] - self._timings[startKey]
            if formatted:
                if d < 0.00001:  # 10us
                    s = "%d ns" % round(d * 1000000000.)
                elif d < 0.01:  # 10ms
                    s = "%d us" % round(d * 1000000.)
                elif d < 10:  # 10s
                    s = "%d ms" % round(d * 1000.)
                else:
                    s = "%d s" % round(d)
                return s.rjust(8)
            else:
                return d
        else:
            if formatted:
                return "n.a.".rjust(8)
            else:
                return None

    def __getitem__(self, key):
        return self._timings.get(key, None)

    def __iter__(self):
        return self._timings.__iter__()

    def __str__(self):
        return pformat(self._timings)


class WebSocketProtocol(object):
    """
    Protocol base class for WebSocket.

    This class implements:

      * :class:`autobahn.websocket.interfaces.IWebSocketChannel`
      * :class:`autobahn.websocket.interfaces.IWebSocketChannelFrameApi`
      * :class:`autobahn.websocket.interfaces.IWebSocketChannelStreamingApi`
    """

    peer = u'<never connected>'

    SUPPORTED_SPEC_VERSIONS = [10, 11, 12, 13, 14, 15, 16, 17, 18]
    """
    WebSocket protocol spec (draft) versions supported by this implementation.
    Use of version 18 indicates RFC6455. Use of versions < 18 indicate actual
    draft spec versions (Hybi-Drafts).
    """

    SUPPORTED_PROTOCOL_VERSIONS = [8, 13]
    """
    WebSocket protocol versions supported by this implementation.
    """

    SPEC_TO_PROTOCOL_VERSION = {10: 8, 11: 8, 12: 8, 13: 13, 14: 13, 15: 13, 16: 13, 17: 13, 18: 13}
    """
    Mapping from protocol spec (draft) version to protocol version.
    """

    PROTOCOL_TO_SPEC_VERSION = {8: 12, 13: 18}
    """
    Mapping from protocol version to the latest protocol spec (draft) version
    using that protocol version.
    """

    DEFAULT_SPEC_VERSION = 18
    """
    Default WebSocket protocol spec version this implementation speaks: final
    RFC6455.
    """

    _WS_MAGIC = b"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
    """
    Protocol defined magic used during WebSocket handshake (used in Hybi-drafts
    and final RFC6455.
    """

    _QUEUED_WRITE_DELAY = 0.00001
    """
    For synched/chopped writes, this is the reactor reentry delay in seconds.
    """

    MESSAGE_TYPE_TEXT = 1
    """
    WebSocket text message type (UTF-8 payload).
    """

    MESSAGE_TYPE_BINARY = 2
    """
    WebSocket binary message type (arbitrary binary payload).
    """

    # WebSocket protocol state:
    # (STATE_PROXY_CONNECTING) => STATE_CONNECTING => STATE_OPEN => STATE_CLOSING => STATE_CLOSED
    #
    STATE_CLOSED = 0
    STATE_CONNECTING = 1
    STATE_CLOSING = 2
    STATE_OPEN = 3
    STATE_PROXY_CONNECTING = 4

    # Streaming Send State
    SEND_STATE_GROUND = 0
    SEND_STATE_MESSAGE_BEGIN = 1
    SEND_STATE_INSIDE_MESSAGE = 2
    SEND_STATE_INSIDE_MESSAGE_FRAME = 3

    # WebSocket protocol close codes
    # See:https://www.iana.org/assignments/websocket/websocket.xml#close-code-number-rules
    #
    CLOSE_STATUS_CODE_NORMAL = 1000
    """Normal close of connection."""

    CLOSE_STATUS_CODE_GOING_AWAY = 1001
    """Going away."""

    CLOSE_STATUS_CODE_PROTOCOL_ERROR = 1002
    """Protocol error."""

    CLOSE_STATUS_CODE_UNSUPPORTED_DATA = 1003
    """Unsupported data."""

    CLOSE_STATUS_CODE_RESERVED1 = 1004
    """RESERVED"""

    CLOSE_STATUS_CODE_NULL = 1005  # MUST NOT be set in close frame!
    """No status received. (MUST NOT be used as status code when sending a close)."""

    CLOSE_STATUS_CODE_ABNORMAL_CLOSE = 1006  # MUST NOT be set in close frame!
    """Abnormal close of connection. (MUST NOT be used as status code when sending a close)."""

    CLOSE_STATUS_CODE_INVALID_PAYLOAD = 1007
    """Invalid frame payload data."""

    CLOSE_STATUS_CODE_POLICY_VIOLATION = 1008
    """Policy violation."""

    CLOSE_STATUS_CODE_MESSAGE_TOO_BIG = 1009
    """Message too big."""

    CLOSE_STATUS_CODE_MANDATORY_EXTENSION = 1010
    """Mandatory extension."""

    CLOSE_STATUS_CODE_INTERNAL_ERROR = 1011
    """The peer encountered an unexpected condition or internal error."""

    CLOSE_STATUS_CODE_SERVICE_RESTART = 1012
    """Service restart."""

    CLOSE_STATUS_CODE_TRY_AGAIN_LATER = 1013
    """Try again later."""

    CLOSE_STATUS_CODE_UNASSIGNED1 = 1014
    """Unassiged."""

    CLOSE_STATUS_CODE_TLS_HANDSHAKE_FAILED = 1015  # MUST NOT be set in close frame!
    """TLS handshake failed, i.e. server certificate could not be verified. (MUST NOT be used as status code when sending a close)."""

    CLOSE_STATUS_CODES_ALLOWED = [CLOSE_STATUS_CODE_NORMAL,
                                  CLOSE_STATUS_CODE_GOING_AWAY,
                                  CLOSE_STATUS_CODE_PROTOCOL_ERROR,
                                  CLOSE_STATUS_CODE_UNSUPPORTED_DATA,
                                  CLOSE_STATUS_CODE_INVALID_PAYLOAD,
                                  CLOSE_STATUS_CODE_POLICY_VIOLATION,
                                  CLOSE_STATUS_CODE_MESSAGE_TOO_BIG,
                                  CLOSE_STATUS_CODE_MANDATORY_EXTENSION,
                                  CLOSE_STATUS_CODE_INTERNAL_ERROR,
                                  CLOSE_STATUS_CODE_SERVICE_RESTART,
                                  CLOSE_STATUS_CODE_TRY_AGAIN_LATER]
    """Status codes allowed to send in close."""

    CONFIG_ATTRS_COMMON = ['logOctets',
                           'logFrames',
                           'trackTimings',
                           'utf8validateIncoming',
                           'applyMask',
                           'maxFramePayloadSize',
                           'maxMessagePayloadSize',
                           'autoFragmentSize',
                           'failByDrop',
                           'echoCloseCodeReason',
                           'openHandshakeTimeout',
                           'closeHandshakeTimeout',
                           'tcpNoDelay',
                           'autoPingInterval',
                           'autoPingTimeout',
                           'autoPingSize']
    """
    Configuration attributes common to servers and clients.
    """

    CONFIG_ATTRS_SERVER = ['versions',
                           'webStatus',
                           'requireMaskedClientFrames',
                           'maskServerFrames',
                           'perMessageCompressionAccept',
                           'serveFlashSocketPolicy',
                           'flashSocketPolicy',
                           'allowedOrigins',
                           'allowedOriginsPatterns',
                           'allowNullOrigin',
                           'maxConnections',
                           'trustXForwardedFor']
    """
    Configuration attributes specific to servers.
    """

    CONFIG_ATTRS_CLIENT = ['version',
                           'acceptMaskedServerFrames',
                           'maskClientFrames',
                           'serverConnectionDropTimeout',
                           'perMessageCompressionOffers',
                           'perMessageCompressionAccept']
    """
    Configuration attributes specific to clients.
    """

    def __init__(self):
        #: a Future/Deferred that fires when we hit STATE_CLOSED
        self.is_closed = txaio.create_future()

    def onOpen(self):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.onOpen`
        """
        self.log.debug("WebSocketProtocol.onOpen")

    def onMessageBegin(self, isBinary):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.onMessageBegin`
        """
        self.message_is_binary = isBinary
        self.message_data = []
        self.message_data_total_length = 0

    def onMessageFrameBegin(self, length):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.onMessageFrameBegin`
        """
        self.frame_length = length
        self.frame_data = []
        self.message_data_total_length += length
        if not self.failedByMe:
            if 0 < self.maxMessagePayloadSize < self.message_data_total_length:
                self.wasMaxMessagePayloadSizeExceeded = True
                self._fail_connection(
                    WebSocketProtocol.CLOSE_STATUS_CODE_MESSAGE_TOO_BIG,
                    u'message exceeds payload limit of {} octets'.format(self.maxMessagePayloadSize)
                )
            elif 0 < self.maxFramePayloadSize < length:
                self.wasMaxFramePayloadSizeExceeded = True
                self._fail_connection(
                    WebSocketProtocol.CLOSE_STATUS_CODE_POLICY_VIOLATION,
                    u'frame exceeds payload limit of {} octets'.format(self.maxFramePayloadSize)
                )

    def onMessageFrameData(self, payload):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.onMessageFrameData`
        """
        if not self.failedByMe:
            if self.websocket_version == 0:
                self.message_data_total_length += len(payload)
                if 0 < self.maxMessagePayloadSize < self.message_data_total_length:
                    self.wasMaxMessagePayloadSizeExceeded = True
                    self._fail_connection(
                        WebSocketProtocol.CLOSE_STATUS_CODE_MESSAGE_TOO_BIG,
                        u'message exceeds payload limit of {} octets'.format(self.maxMessagePayloadSize)
                    )
                self.message_data.append(payload)
            else:
                self.frame_data.append(payload)

    def onMessageFrameEnd(self):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.onMessageFrameEnd`
        """
        if not self.failedByMe:
            self._onMessageFrame(self.frame_data)

        self.frame_data = None

    def onMessageFrame(self, payload):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.onMessageFrame`
        """
        if not self.failedByMe:
            self.message_data.extend(payload)

    def onMessageEnd(self):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.onMessageEnd`
        """
        if not self.failedByMe:
            payload = b''.join(self.message_data)
            if self.trackedTimings:
                self.trackedTimings.track("onMessage")
            self._onMessage(payload, self.message_is_binary)

        self.message_data = None

    def onMessage(self, payload, isBinary):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.onMessage`
        """
        self.log.debug(
            "WebSocketProtocol.onMessage(payload=<{payload_len} bytes)>, isBinary={isBinary}",
            payload_len=(len(payload) if payload else 0),
            isBinary=isBinary,
        )

    def onPing(self, payload):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.onPing`
        """
        self.log.debug(
            "WebSocketProtocol.onPing(payload=<{payload_len} bytes>)",
            payload_len=(len(payload) if payload else 0),
        )
        if self.state == WebSocketProtocol.STATE_OPEN:
            self.sendPong(payload)

    def onPong(self, payload):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.onPong`
        """
        self.log.debug(
            "WebSocketProtocol.onPong(payload=<{payload_len} bytes>)",
            payload_len=(len(payload) if payload else 0),
        )

    def onClose(self, wasClean, code, reason):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.onClose`
        """
        self.log.debug(
            "WebSocketProtocol.onClose(wasClean={wasClean}, code={code}, reason={reason})",
            wasClean=wasClean,
            code=code,
            reason=reason,
        )

    def onCloseFrame(self, code, reasonRaw):
        """
        Callback when a Close frame was received. The default implementation answers by
        sending a Close when no Close was sent before. Otherwise it drops
        the TCP connection either immediately (when we are a server) or after a timeout
        (when we are a client and expect the server to drop the TCP).

        :param code: Close status code, if there was one (:class:`WebSocketProtocol`.CLOSE_STATUS_CODE_*).
        :type code: int
        :param reasonRaw: Close reason (when present, a status code MUST have been also be present).
        :type reasonRaw: bytes
        """
        self.remoteCloseCode = None
        self.remoteCloseReason = None

        # reserved close codes: 0-999, 1004, 1005, 1006, 1011-2999, >= 5000
        #
        if code is not None and (code < 1000 or (1000 <= code <= 2999 and code not in WebSocketProtocol.CLOSE_STATUS_CODES_ALLOWED) or code >= 5000):
            if self._protocol_violation(u'invalid close code {}'.format(code)):
                return True
            else:
                self.remoteCloseCode = WebSocketProtocol.CLOSE_STATUS_CODE_NORMAL
        else:
            self.remoteCloseCode = code

        # closing reason
        #
        if reasonRaw is not None:
            # we use our own UTF-8 validator to get consistent and fully conformant
            # UTF-8 validation behavior
            u = Utf8Validator()
            val = u.validate(reasonRaw)

            # the UTF8 must be valid _and_ end on a Unicode code point
            if not (val[0] and val[1]):
                if self._invalid_payload(u'invalid close reason (non-UTF8 payload)'):
                    return True
            else:
                self.remoteCloseReason = reasonRaw.decode('utf8')

        # handle receive of close frame depending on protocol state
        #
        if self.state == WebSocketProtocol.STATE_CLOSING:
            # We already initiated the closing handshake, so this
            # is the peer's reply to our close frame.

            # cancel any closing HS timer if present
            #
            if self.closeHandshakeTimeoutCall is not None:
                self.log.debug("connection closed properly: canceling closing handshake timeout")
                self.closeHandshakeTimeoutCall.cancel()
                self.closeHandshakeTimeoutCall = None

            self.wasClean = True

            if self.factory.isServer:
                # When we are a server, we immediately drop the TCP.
                self.dropConnection(abort=True)
            else:
                # When we are a client, the server should drop the TCP
                # If that doesn't happen, we do. And that will set wasClean = False.
                if self.serverConnectionDropTimeout > 0:
                    self.serverConnectionDropTimeoutCall = txaio.call_later(
                        self.serverConnectionDropTimeout,
                        self.onServerConnectionDropTimeout,
                    )

        elif self.state == WebSocketProtocol.STATE_OPEN:
            # The peer initiates a closing handshake, so we reply
            # by sending close frame.

            self.wasClean = True

            if self.websocket_version == 0:
                self.sendCloseFrame(isReply=True)
            else:
                # Either reply with same code/reason, or code == NORMAL/reason=None
                if self.echoCloseCodeReason:
                    self.sendCloseFrame(code=self.remoteCloseCode, reasonUtf8=encode_truncate(self.remoteCloseReason, 123), isReply=True)
                else:
                    self.sendCloseFrame(code=WebSocketProtocol.CLOSE_STATUS_CODE_NORMAL, isReply=True)

            if self.factory.isServer:
                # When we are a server, we immediately drop the TCP.
                self.dropConnection(abort=False)
            else:
                # When we are a client, we expect the server to drop the TCP,
                # and when the server fails to do so, a timeout in sendCloseFrame()
                # will set wasClean = False back again.
                pass

        elif self.state == WebSocketProtocol.STATE_CLOSED:
            # The peer initiated a closing handshake but dropped the TCP immediately.
            self.wasClean = False

        else:
            # STATE_PROXY_CONNECTING, STATE_CONNECTING
            raise Exception("logic error")

    def onServerConnectionDropTimeout(self):
        """
        We (a client) expected the peer (a server) to drop the connection,
        but it didn't (in time self.serverConnectionDropTimeout).
        So we drop the connection, but set self.wasClean = False.
        """
        self.serverConnectionDropTimeoutCall = None

        if self.state != WebSocketProtocol.STATE_CLOSED:
            self.wasClean = False
            self.wasNotCleanReason = u'WebSocket closing handshake timeout (server did not drop TCP connection in time)'
            self.wasServerConnectionDropTimeout = True
            self.dropConnection(abort=True)
        else:
            self.log.debug("skipping closing handshake timeout: server did indeed drop the connection in time")

    def onOpenHandshakeTimeout(self):
        """
        We expected the peer to complete the opening handshake with to us.
        It didn't do so (in time self.openHandshakeTimeout).
        So we drop the connection, but set self.wasClean = False.
        """
        self.openHandshakeTimeoutCall = None

        if self.state in [WebSocketProtocol.STATE_CONNECTING, WebSocketProtocol.STATE_PROXY_CONNECTING]:
            self.wasClean = False
            self.wasNotCleanReason = u'WebSocket opening handshake timeout (peer did not finish the opening handshake in time)'
            self.wasOpenHandshakeTimeout = True
            self.dropConnection(abort=True)
        elif self.state == WebSocketProtocol.STATE_OPEN:
            self.log.debug("skipping opening handshake timeout: WebSocket connection is open (opening handshake already finished)")
        elif self.state == WebSocketProtocol.STATE_CLOSING:
            self.log.debug("skipping opening handshake timeout: WebSocket connection is already closing ..")
        elif self.state == WebSocketProtocol.STATE_CLOSED:
            self.log.debug("skipping opening handshake timeout: WebSocket connection is already closed")
        else:
            # should not arrive here
            raise Exception("logic error")

    def onCloseHandshakeTimeout(self):
        """
        We expected the peer to respond to us initiating a close handshake. It didn't
        respond (in time self.closeHandshakeTimeout) with a close response frame though.
        So we drop the connection, but set self.wasClean = False.
        """
        self.closeHandshakeTimeoutCall = None

        if self.state != WebSocketProtocol.STATE_CLOSED:
            self.wasClean = False
            self.wasNotCleanReason = u'WebSocket closing handshake timeout (peer did not finish the opening handshake in time)'
            self.wasCloseHandshakeTimeout = True
            self.dropConnection(abort=True)
        else:
            self.log.debug('skipping closing handshake timeout: WebSocket connection is already closed')

    def onAutoPingTimeout(self):
        """
        When doing automatic ping/pongs to detect broken connection, the peer
        did not reply in time to our ping. We drop the connection.
        """
        self.wasClean = False
        self.wasNotCleanReason = u'WebSocket ping timeout (peer did not respond with pong in time)'
        self.autoPingTimeoutCall = None
        self.dropConnection(abort=True)

    def dropConnection(self, abort=False):
        """
        Drop the underlying TCP connection.
        """
        if self.state != WebSocketProtocol.STATE_CLOSED:

            if self.wasClean:
                self.log.debug('dropping connection to peer {peer} with abort={abort}', peer=self.peer, abort=abort)
            else:
                self.log.warn('dropping connection to peer {peer} with abort={abort}: {reason}', peer=self.peer, abort=abort, reason=self.wasNotCleanReason)

            self.droppedByMe = True

            # this code-path will be hit (*without* hitting
            # _connectionLost) in some timeout scenarios (unit-tests
            # cover these). However, sometimes we hit both.
            self.state = WebSocketProtocol.STATE_CLOSED
            txaio.resolve(self.is_closed, self)

            self._closeConnection(abort)
        else:
            self.log.debug('dropping connection to peer {peer} skipped - connection already closed', peer=self.peer)

    def _fail_connection(self, code=CLOSE_STATUS_CODE_GOING_AWAY, reason=u'going away'):
        """
        Fails the WebSocket connection.
        """
        if self.state != WebSocketProtocol.STATE_CLOSED:
            self.log.debug("failing connection: {code}: {reason}", code=code, reason=reason)

            self.failedByMe = True

            if self.failByDrop:
                # brutally drop the TCP connection
                self.wasClean = False
                self.wasNotCleanReason = u'I dropped the WebSocket TCP connection: {0}'.format(reason)
                self.dropConnection(abort=True)

            else:
                if self.state != WebSocketProtocol.STATE_CLOSING:
                    # perform WebSocket closing handshake
                    self.sendCloseFrame(code=code, reasonUtf8=encode_truncate(reason, 123), isReply=False)
                else:
                    # already performing closing handshake .. we now drop the TCP
                    # (this can happen e.g. if we encounter a 2nd protocol violation during closing HS)
                    self.dropConnection(abort=False)

        else:
            self.log.debug("skip failing of connection since connection is already closed")

    def _protocol_violation(self, reason):
        """
        Fired when a WebSocket protocol violation/error occurs.

        :param reason: Protocol violation that was encountered (human readable).
        :type reason: str

        :returns: bool -- True, when any further processing should be discontinued.
        """
        self.log.debug("Protocol violation: {reason}", reason=reason)

        self._fail_connection(WebSocketProtocol.CLOSE_STATUS_CODE_PROTOCOL_ERROR, reason)

        if self.failByDrop:
            return True
        else:
            # if we don't immediately drop the TCP, we need to skip the invalid frame
            # to continue to later receive the closing handshake reply
            return False

    def _invalid_payload(self, reason):
        """
        Fired when invalid payload is encountered. Currently, this only happens
        for text message when payload is invalid UTF-8 or close frames with
        close reason that is invalid UTF-8.

        :param reason: What was invalid for the payload (human readable).
        :type reason: str

        :returns: bool -- True, when any further processing should be discontinued.
        """
        self.log.debug("Invalid payload: {reason}", reason=reason)

        self._fail_connection(WebSocketProtocol.CLOSE_STATUS_CODE_INVALID_PAYLOAD, reason)

        if self.failByDrop:
            return True
        else:
            # if we don't immediately drop the TCP, we need to skip the invalid frame
            # to continue to later receive the closing handshake reply
            return False

    def setTrackTimings(self, enable):
        """
        Enable/disable tracking of detailed timings.

        :param enable: Turn time tracking on/off.
        :type enable: bool
        """
        if not hasattr(self, 'trackTimings') or self.trackTimings != enable:
            self.trackTimings = enable
            if self.trackTimings:
                self.trackedTimings = Timings()
            else:
                self.trackedTimings = None

    def _connectionMade(self):
        """
        This is called by network framework when a new TCP connection has been established
        and handed over to a Protocol instance (an instance of this class).
        """
        # copy default options from factory (so we are not affected by changed on
        # those), but only copy if not already set on protocol instance (allow
        # to set configuration individually)
        #
        configAttrLog = []
        for configAttr in self.CONFIG_ATTRS:
            if not hasattr(self, configAttr):
                setattr(self, configAttr, getattr(self.factory, configAttr))
                configAttrSource = self.factory.__class__.__name__
            else:
                configAttrSource = self.__class__.__name__
            configAttrLog.append((configAttr, getattr(self, configAttr), configAttrSource))

        self.log.debug("\n{attrs}", attrs=pformat(configAttrLog))

        # permessage-compress extension
        self._perMessageCompress = None

        # Time tracking
        self.trackedTimings = None
        self.setTrackTimings(self.trackTimings)

        # Traffic stats
        self.trafficStats = TrafficStats()

        # initial state
        if not self.factory.isServer and self.factory.proxy is not None:
            self.state = WebSocketProtocol.STATE_PROXY_CONNECTING
        else:
            self.state = WebSocketProtocol.STATE_CONNECTING
        self.send_state = WebSocketProtocol.SEND_STATE_GROUND
        self.data = b''

        # for chopped/synched sends, we need to queue to maintain
        # ordering when recalling the reactor to actually "force"
        # the octets to wire (see test/trickling in the repo)
        self.send_queue = deque()
        self.triggered = False

        # incremental UTF8 validator
        self.utf8validator = Utf8Validator()

        # track when frame/message payload sizes (incoming) were exceeded
        self.wasMaxFramePayloadSizeExceeded = False
        self.wasMaxMessagePayloadSizeExceeded = False

        # the following vars are related to connection close handling/tracking

        # True, iff I have initiated closing HS (that is, did send close first)
        self.closedByMe = False

        # True, iff I have failed the WS connection (i.e. due to protocol error)
        # Failing can be either by initiating close HS or brutal drop (this is
        # controlled by failByDrop option)
        self.failedByMe = False

        # True, iff I dropped the TCP connection (called transport.loseConnection())
        self.droppedByMe = False

        # True, iff full WebSocket closing handshake was performed (close frame sent
        # and received) _and_ the server dropped the TCP (which is its responsibility)
        self.wasClean = False

        # When self.wasClean = False, the reason (what happened)
        self.wasNotCleanReason = None

        # When we are a client, and we expected the server to drop the TCP, but that
        # didn't happen in time, this gets True
        self.wasServerConnectionDropTimeout = False

        # When the initial WebSocket opening handshake times out, this gets True
        self.wasOpenHandshakeTimeout = False

        # When we initiated a closing handshake, but the peer did not respond in
        # time, this gets True
        self.wasCloseHandshakeTimeout = False

        # True, iff I dropped the TCP connection because we fully served the
        # Flash Socket Policy File after a policy file request.
        self.wasServingFlashSocketPolicyFile = False

        # The close code I sent in close frame (if any)
        self.localCloseCode = None

        # The close reason I sent in close frame (if any)
        self.localCloseReason = None

        # The close code the peer sent me in close frame (if any)
        self.remoteCloseCode = None

        # The close reason the peer sent me in close frame (if any)
        self.remoteCloseReason = None

        # timers, which might get set up later, and remembered here to get canceled
        # when appropriate
        if not self.factory.isServer:
            self.serverConnectionDropTimeoutCall = None
        self.openHandshakeTimeoutCall = None
        self.closeHandshakeTimeoutCall = None

        self.autoPingTimeoutCall = None
        self.autoPingPending = None
        self.autoPingPendingCall = None

        # set opening handshake timeout handler
        if self.openHandshakeTimeout > 0:
            self.openHandshakeTimeoutCall = self.factory._batched_timer.call_later(
                self.openHandshakeTimeout,
                self.onOpenHandshakeTimeout,
            )

    def _connectionLost(self, reason):
        """
        This is called by network framework when a transport connection was
        lost.
        """
        # cancel any server connection drop timer if present
        #
        self.log.debug('_connectionLost: {reason}', reason=reason)

        if not self.factory.isServer and self.serverConnectionDropTimeoutCall is not None:
            self.log.debug("serverConnectionDropTimeoutCall.cancel")
            self.serverConnectionDropTimeoutCall.cancel()
            self.serverConnectionDropTimeoutCall = None

        # cleanup auto ping/pong timers
        #
        if self.autoPingPendingCall:
            self.log.debug("Auto ping/pong: canceling autoPingPendingCall upon lost connection")
            self.autoPingPendingCall.cancel()
            self.autoPingPendingCall = None

        if self.autoPingTimeoutCall:
            self.log.debug("Auto ping/pong: canceling autoPingTimeoutCall upon lost connection")
            self.autoPingTimeoutCall.cancel()
            self.autoPingTimeoutCall = None

        # check required here because in some scenarios dropConnection
        # will already have resolved the Future/Deferred.
        if self.state != WebSocketProtocol.STATE_CLOSED:
            self.state = WebSocketProtocol.STATE_CLOSED
            txaio.resolve(self.is_closed, self)

        if self.wasServingFlashSocketPolicyFile:
            self.log.debug("connection dropped after serving Flash Socket Policy File")
        else:
            if not self.wasClean:
                if not self.droppedByMe and self.wasNotCleanReason is None:
                    self.wasNotCleanReason = u'peer dropped the TCP connection without previous WebSocket closing handshake'
                self._onClose(self.wasClean, WebSocketProtocol.CLOSE_STATUS_CODE_ABNORMAL_CLOSE, "connection was closed uncleanly (%s)" % self.wasNotCleanReason)
            else:
                self._onClose(self.wasClean, self.remoteCloseCode, self.remoteCloseReason)

    def logRxOctets(self, data):
        """
        Hook fired right after raw octets have been received, but only when
        self.logOctets == True.
        """
        self.log.debug(
            "RX Octets from {peer} : octets = {octets}",
            peer=self.peer,
            octets=_LazyHexFormatter(data),
        )

    def logTxOctets(self, data, sync):
        """
        Hook fired right after raw octets have been sent, but only when
        self.logOctets == True.
        """
        self.log.debug(
            "TX Octets to {peer} : sync = {sync}, octets = {octets}",
            peer=self.peer,
            sync=sync,
            octets=_LazyHexFormatter(data),
        )

    def logRxFrame(self, frameHeader, payload):
        """
        Hook fired right after WebSocket frame has been received and decoded,
        but only when self.logFrames == True.
        """
        data = b''.join(payload)
        self.log.debug(
            "RX Frame from {peer} : fin = {fin}, rsv = {rsv}, opcode = {opcode}"
            ", mask = {mask}, length = {length}, payload = {payload}",
            peer=self.peer,
            fin=frameHeader.fin,
            rsv=frameHeader.rsv,
            opcode=frameHeader.opcode,
            mask=binascii.b2a_hex(frameHeader.mask) if frameHeader.mask else "-",
            length=frameHeader.length,
            payload=repr(data) if frameHeader.opcode == 1 else _LazyHexFormatter(data),
        )

    def logTxFrame(self, frameHeader, payload, repeatLength, chopsize, sync):
        """
        Hook fired right after WebSocket frame has been encoded and sent, but
        only when self.logFrames == True.
        """
        self.log.debug(
            "TX Frame to {peer} : fin = {fin}, rsv = {rsv}, opcode = {opcode}, "
            "mask = {mask}, length = {length}, repeat_length = {repeat_length},"
            " chopsize = {chopsize}, sync = {sync}, payload = {payload}",
            peer=self.peer,
            fin=frameHeader.fin,
            rsv=frameHeader.rsv,
            opcode=frameHeader.opcode,
            mask=binascii.b2a_hex(frameHeader.mask) if frameHeader.mask else "-",
            length=frameHeader.length,
            repeat_length=repeatLength,
            chopsize=chopsize,
            sync=sync,
            payload=repr(payload) if frameHeader.opcode == 1 else _LazyHexFormatter(payload),
        )

    def _dataReceived(self, data):
        """
        This is called by network framework upon receiving data on transport
        connection.
        """
        if self.state == WebSocketProtocol.STATE_OPEN:
            self.trafficStats.incomingOctetsWireLevel += len(data)
        elif self.state == WebSocketProtocol.STATE_CONNECTING or self.state == WebSocketProtocol.STATE_PROXY_CONNECTING:
            self.trafficStats.preopenIncomingOctetsWireLevel += len(data)

        if self.logOctets:
            self.logRxOctets(data)
        self.data += data
        self.consumeData()

    def consumeData(self):
        """
        Consume buffered (incoming) data.
        """
        # WebSocket is open (handshake was completed) or close was sent
        #
        if self.state == WebSocketProtocol.STATE_OPEN or self.state == WebSocketProtocol.STATE_CLOSING:

            # process until no more buffered data left or WS was closed
            #
            while self.processData() and self.state != WebSocketProtocol.STATE_CLOSED:
                pass

        # need to establish proxy connection
        #
        elif self.state == WebSocketProtocol.STATE_PROXY_CONNECTING:

            self.processProxyConnect()

        # WebSocket needs handshake
        #
        elif self.state == WebSocketProtocol.STATE_CONNECTING:

            # the implementation of processHandshake() in derived
            # class needs to perform client or server handshake
            # from other party here ..
            #
            self.processHandshake()

        # we failed the connection .. don't process any more data!
        #
        elif self.state == WebSocketProtocol.STATE_CLOSED:

            # ignore any data received after WS was closed
            #
            self.log.debug("received data in STATE_CLOSED")

        # should not arrive here (invalid state)
        #
        else:
            raise Exception("invalid state")

    def processProxyConnect(self):
        """
        Process proxy connect.
        """
        raise Exception("must implement proxy connect (client or server) in derived class")

    def processHandshake(self):
        """
        Process WebSocket handshake.
        """
        raise Exception("must implement handshake (client or server) in derived class")

    def _trigger(self):
        """
        Trigger sending stuff from send queue (which is only used for
        chopped/synched writes).
        """
        if not self.triggered:
            self.triggered = True
            self._send()

    def _send(self):
        """
        Send out stuff from send queue. For details how this works, see
        test/trickling in the repo.
        """
        if len(self.send_queue) > 0:
            e = self.send_queue.popleft()

            if self.state != WebSocketProtocol.STATE_CLOSED:

                self.transport.write(e[0])

                if self.state == WebSocketProtocol.STATE_OPEN:
                    self.trafficStats.outgoingOctetsWireLevel += len(e[0])
                elif self.state == WebSocketProtocol.STATE_CONNECTING or self.state == WebSocketProtocol.STATE_PROXY_CONNECTING:
                    self.trafficStats.preopenOutgoingOctetsWireLevel += len(e[0])

                if self.logOctets:
                    self.logTxOctets(e[0], e[1])
            else:
                self.log.debug("skipped delayed write, since connection is closed")

            # we need to reenter the reactor to make the latter
            # reenter the OS network stack, so that octets
            # can get on the wire. Note: this is a "heuristic",
            # since there is no (easy) way to really force out
            # octets from the OS network stack to wire.
            txaio.call_later(WebSocketProtocol._QUEUED_WRITE_DELAY, self._send)
        else:
            self.triggered = False

    def sendData(self, data, sync=False, chopsize=None):
        """
        Wrapper for self.transport.write which allows to give a chopsize.
        When asked to chop up writing to TCP stream, we write only chopsize
        octets and then give up control to select() in underlying reactor so
        that bytes get onto wire immediately. Note that this is different from
        and unrelated to WebSocket data message fragmentation. Note that this
        is also different from the TcpNoDelay option which can be set on the
        socket.
        """
        if chopsize and chopsize > 0:
            i = 0
            n = len(data)
            done = False
            while not done:
                j = i + chopsize
                if j >= n:
                    done = True
                    j = n
                self.send_queue.append((data[i:j], True))
                i += chopsize
            self._trigger()
        else:
            if sync or len(self.send_queue) > 0:
                self.send_queue.append((data, sync))
                self._trigger()
            else:
                self.transport.write(data)

                if self.state == WebSocketProtocol.STATE_OPEN:
                    self.trafficStats.outgoingOctetsWireLevel += len(data)
                elif self.state == WebSocketProtocol.STATE_CONNECTING or self.state == WebSocketProtocol.STATE_PROXY_CONNECTING:
                    self.trafficStats.preopenOutgoingOctetsWireLevel += len(data)

                if self.logOctets:
                    self.logTxOctets(data, False)

    def sendPreparedMessage(self, preparedMsg):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.sendPreparedMessage`
        """
        if self._perMessageCompress is None or preparedMsg.doNotCompress:
            self.sendData(preparedMsg.payloadHybi)
        else:
            self.sendMessage(preparedMsg.payload, preparedMsg.binary)

    def processData(self):
        """
        After WebSocket handshake has been completed, this procedure will do
        all subsequent processing of incoming bytes.
        """
        buffered_len = len(self.data)

        # outside a frame, that is we are awaiting data which starts a new frame
        #
        if self.current_frame is None:

            # need minimum of 2 octets to for new frame
            #
            if buffered_len >= 2:

                # FIN, RSV, OPCODE
                #
                if six.PY3:
                    b = self.data[0]
                else:
                    b = ord(self.data[0])
                frame_fin = (b & 0x80) != 0
                frame_rsv = (b & 0x70) >> 4
                frame_opcode = b & 0x0f

                # MASK, PAYLOAD LEN 1
                #
                if six.PY3:
                    b = self.data[1]
                else:
                    b = ord(self.data[1])
                frame_masked = (b & 0x80) != 0
                frame_payload_len1 = b & 0x7f

                # MUST be 0 when no extension defining
                # the semantics of RSV has been negotiated
                #
                if frame_rsv != 0:
                    if self._perMessageCompress is not None and frame_rsv == 4:
                        pass
                    else:
                        if self._protocol_violation(u'RSV = {} and no extension negotiated'.format(frame_rsv)):
                            return False

                # all client-to-server frames MUST be masked
                #
                if self.factory.isServer and self.requireMaskedClientFrames and not frame_masked:
                    if self._protocol_violation(u'unmasked client-to-server frame'):
                        return False

                # all server-to-client frames MUST NOT be masked
                #
                if not self.factory.isServer and not self.acceptMaskedServerFrames and frame_masked:
                    if self._protocol_violation(u'masked server-to-client frame'):
                        return False

                # check frame
                #
                if frame_opcode > 7:  # control frame (have MSB in opcode set)

                    # control frames MUST NOT be fragmented
                    #
                    if not frame_fin:
                        if self._protocol_violation(u'fragmented control frame'):
                            return False

                    # control frames MUST have payload 125 octets or less
                    #
                    if frame_payload_len1 > 125:
                        if self._protocol_violation(u'control frame with payload length > 125 octets'):
                            return False

                    # check for reserved control frame opcodes
                    #
                    if frame_opcode not in [8, 9, 10]:
                        if self._protocol_violation(u'control frame using reserved opcode {}'.format(frame_opcode)):
                            return False

                    # close frame : if there is a body, the first two bytes of the body MUST be a 2-byte
                    # unsigned integer (in network byte order) representing a status code
                    #
                    if frame_opcode == 8 and frame_payload_len1 == 1:
                        if self._protocol_violation(u'received close control frame with payload len 1'):
                            return False

                    # control frames MUST NOT be compressed
                    #
                    if self._perMessageCompress is not None and frame_rsv == 4:
                        if self._protocol_violation(u'received compressed control frame [{}]'.format(self._perMessageCompress.EXTENSION_NAME)):
                            return False

                else:  # data frame

                    # check for reserved data frame opcodes
                    #
                    if frame_opcode not in [0, 1, 2]:
                        if self._protocol_violation(u'data frame using reserved opcode {}'.format(frame_opcode)):
                            return False

                    # check opcode vs message fragmentation state 1/2
                    #
                    if not self.inside_message and frame_opcode == 0:
                        if self._protocol_violation(u'received continuation data frame outside fragmented message'):
                            return False

                    # check opcode vs message fragmentation state 2/2
                    #
                    if self.inside_message and frame_opcode != 0:
                        if self._protocol_violation(u'received non-continuation data frame while inside fragmented message'):
                            return False

                    # continuation data frames MUST NOT have the compressed bit set
                    #
                    if self._perMessageCompress is not None and frame_rsv == 4 and self.inside_message:
                        if self._protocol_violation(u'received continuation data frame with compress bit set [{}]'.format(self._perMessageCompress.EXTENSION_NAME)):
                            return False

                # compute complete header length
                #
                if frame_masked:
                    mask_len = 4
                else:
                    mask_len = 0

                if frame_payload_len1 < 126:
                    frame_header_len = 2 + mask_len
                elif frame_payload_len1 == 126:
                    frame_header_len = 2 + 2 + mask_len
                elif frame_payload_len1 == 127:
                    frame_header_len = 2 + 8 + mask_len
                else:
                    raise Exception("logic error")

                # only proceed when we have enough data buffered for complete
                # frame header (which includes extended payload len + mask)
                #
                if buffered_len >= frame_header_len:

                    # minimum frame header length (already consumed)
                    #
                    i = 2

                    # extract extended payload length
                    #
                    if frame_payload_len1 == 126:
                        frame_payload_len = struct.unpack("!H", self.data[i:i + 2])[0]
                        if frame_payload_len < 126:
                            if self._protocol_violation(u'invalid data frame length (not using minimal length encoding)'):
                                return False
                        i += 2
                    elif frame_payload_len1 == 127:
                        frame_payload_len = struct.unpack("!Q", self.data[i:i + 8])[0]
                        if frame_payload_len > 0x7FFFFFFFFFFFFFFF:  # 2**63
                            if self._protocol_violation(u'invalid data frame length (>2^63)'):
                                return False
                        if frame_payload_len < 65536:
                            if self._protocol_violation(u'invalid data frame length (not using minimal length encoding)'):
                                return False
                        i += 8
                    else:
                        frame_payload_len = frame_payload_len1

                    # when payload is masked, extract frame mask
                    #
                    frame_mask = None
                    if frame_masked:
                        frame_mask = self.data[i:i + 4]
                        i += 4

                    if frame_masked and frame_payload_len > 0 and self.applyMask:
                        self.current_frame_masker = create_xor_masker(frame_mask, frame_payload_len)
                    else:
                        self.current_frame_masker = XorMaskerNull()

                    # remember rest (payload of current frame after header and everything thereafter)
                    #
                    self.data = self.data[i:]

                    # ok, got complete frame header
                    #
                    self.current_frame = FrameHeader(frame_opcode,
                                                     frame_fin,
                                                     frame_rsv,
                                                     frame_payload_len,
                                                     frame_mask)

                    # process begin on new frame
                    #
                    self.onFrameBegin()

                    # reprocess when frame has no payload or and buffered data left
                    #
                    return frame_payload_len == 0 or len(self.data) > 0

                else:
                    return False  # need more data
            else:
                return False  # need more data

        # inside a started frame
        #
        else:

            # cut out rest of frame payload
            #
            rest = self.current_frame.length - self.current_frame_masker.pointer()
            if buffered_len >= rest:
                data = self.data[:rest]
                length = rest
                self.data = self.data[rest:]
            else:
                data = self.data
                length = buffered_len
                self.data = b''

            if length > 0:
                # unmask payload
                #
                payload = self.current_frame_masker.process(data)
            else:
                # we also process empty payloads, since we need to fire
                # our hooks (at least for streaming processing, this is
                # necessary for correct protocol state transitioning)
                #
                payload = b''

            # process frame data
            #
            fr = self.onFrameData(payload)
            # noinspection PySimplifyBooleanCheck
            if fr is False:
                return False

            # fire frame end handler when frame payload is complete
            #
            if self.current_frame_masker.pointer() == self.current_frame.length:
                fr = self.onFrameEnd()
                # noinspection PySimplifyBooleanCheck
                if fr is False:
                    return False

            # reprocess when no error occurred and buffered data left
            #
            return len(self.data) > 0

    def onFrameBegin(self):
        """
        Begin of receive new frame.
        """
        if self.current_frame.opcode > 7:
            self.control_frame_data = []
        else:
            # new message started
            #
            if not self.inside_message:

                self.inside_message = True

                # setup decompressor
                #
                if self._perMessageCompress is not None and self.current_frame.rsv == 4:
                    self._isMessageCompressed = True
                    self._perMessageCompress.start_decompress_message()
                else:
                    self._isMessageCompressed = False

                # setup UTF8 validator
                #
                if self.current_frame.opcode == WebSocketProtocol.MESSAGE_TYPE_TEXT and self.utf8validateIncoming:
                    self.utf8validator.reset()
                    self.utf8validateIncomingCurrentMessage = True
                    self.utf8validateLast = (True, True, 0, 0)
                else:
                    self.utf8validateIncomingCurrentMessage = False

                # track timings
                #
                if self.trackedTimings:
                    self.trackedTimings.track("onMessageBegin")

                # fire onMessageBegin
                #
                self._onMessageBegin(self.current_frame.opcode == WebSocketProtocol.MESSAGE_TYPE_BINARY)

            self._onMessageFrameBegin(self.current_frame.length)

    def onFrameData(self, payload):
        """
        New data received within frame.
        """
        if self.current_frame.opcode > 7:
            self.control_frame_data.append(payload)
        else:
            # decompress frame payload
            #
            if self._isMessageCompressed:
                compressedLen = len(payload)
                self.log.debug(
                    "RX compressed [length]: octets",
                    legnth=compressedLen,
                    octets=_LazyHexFormatter(payload),
                )

                payload = self._perMessageCompress.decompress_message_data(payload)
                uncompressedLen = len(payload)
            else:
                l = len(payload)
                compressedLen = l
                uncompressedLen = l

            if self.state == WebSocketProtocol.STATE_OPEN:
                self.trafficStats.incomingOctetsWebSocketLevel += compressedLen
                self.trafficStats.incomingOctetsAppLevel += uncompressedLen

            # incrementally validate UTF-8 payload
            #
            if self.utf8validateIncomingCurrentMessage:
                self.utf8validateLast = self.utf8validator.validate(payload)
                if not self.utf8validateLast[0]:
                    if self._invalid_payload(u'encountered invalid UTF-8 while processing text message at payload octet index {}'.format(self.utf8validateLast[3])):
                        return False

            self._onMessageFrameData(payload)

    def onFrameEnd(self):
        """
        End of frame received.
        """
        if self.current_frame.opcode > 7:
            if self.logFrames:
                self.logRxFrame(self.current_frame, self.control_frame_data)
            self.processControlFrame()
        else:
            if self.state == WebSocketProtocol.STATE_OPEN:
                self.trafficStats.incomingWebSocketFrames += 1
            if self.logFrames:
                self.logRxFrame(self.current_frame, self.frame_data)

            self._onMessageFrameEnd()

            if self.current_frame.fin:

                # handle end of compressed message
                #
                if self._isMessageCompressed:
                    self._perMessageCompress.end_decompress_message()

                # verify UTF8 has actually ended
                #
                if self.utf8validateIncomingCurrentMessage:
                    if not self.utf8validateLast[1]:
                        if self._invalid_payload(u'UTF-8 text message payload ended within Unicode code point at payload octet index {}'.format(self.utf8validateLast[3])):
                            return False

                if self.state == WebSocketProtocol.STATE_OPEN:
                    self.trafficStats.incomingWebSocketMessages += 1

                self._onMessageEnd()
                self.inside_message = False

        self.current_frame = None

    def processControlFrame(self):
        """
        Process a completely received control frame.
        """
        payload = b''.join(self.control_frame_data)
        self.control_frame_data = None

        # CLOSE frame
        #
        if self.current_frame.opcode == 8:

            code = None
            reasonRaw = None
            ll = len(payload)
            if ll > 1:
                code = struct.unpack("!H", payload[0:2])[0]
                if ll > 2:
                    reasonRaw = payload[2:]

            if self.onCloseFrame(code, reasonRaw):
                return False

        # PING frame
        #
        elif self.current_frame.opcode == 9:
            self._onPing(payload)

        # PONG frame
        #
        elif self.current_frame.opcode == 10:

            # auto ping/pong processing
            #
            if self.autoPingPending:
                try:
                    if payload == self.autoPingPending:
                        self.log.debug("Auto ping/pong: received pending pong for auto-ping/pong")

                        if self.autoPingTimeoutCall:
                            self.autoPingTimeoutCall.cancel()

                        self.autoPingPending = None
                        self.autoPingTimeoutCall = None

                        if self.autoPingInterval:
                            self.autoPingPendingCall = self.factory._batched_timer.call_later(
                                self.autoPingInterval,
                                self._sendAutoPing,
                            )
                    else:
                        self.log.debug("Auto ping/pong: received non-pending pong")
                except:
                    self.log.debug("Auto ping/pong: received non-pending pong")

            # fire app-level callback
            #
            self._onPong(payload)

        else:
            # we might arrive here, when protocolViolation
            # wants us to continue anyway
            pass

        return True

    def sendFrame(self,
                  opcode,
                  payload=b'',
                  fin=True,
                  rsv=0,
                  mask=None,
                  payload_len=None,
                  chopsize=None,
                  sync=False):
        """
        Send out frame. Normally only used internally via sendMessage(),
        sendPing(), sendPong() and sendClose().

        This method deliberately allows to send invalid frames (that is frames
        invalid per-se, or frames invalid because of protocol state). Other
        than in fuzzing servers, calling methods will ensure that no invalid
        frames are sent.

        In addition, this method supports explicit specification of payload
        length. When payload_len is given, it will always write that many
        octets to the stream. It'll wrap within payload, resending parts of
        that when more octets were requested The use case is again for fuzzing
        server which want to sent increasing amounts of payload data to peers
        without having to construct potentially large messages themselves.
        """
        if payload_len is not None:
            if len(payload) < 1:
                raise Exception("cannot construct repeated payload with length %d from payload of length %d" % (payload_len, len(payload)))
            l = payload_len
            pl = b''.join([payload for _ in range(payload_len / len(payload))]) + payload[:payload_len % len(payload)]
        else:
            l = len(payload)
            pl = payload

        # first byte
        #
        b0 = 0
        if fin:
            b0 |= (1 << 7)
        b0 |= (rsv % 8) << 4
        b0 |= opcode % 128

        # second byte, payload len bytes and mask
        #
        b1 = 0
        if mask or (not self.factory.isServer and self.maskClientFrames) or (self.factory.isServer and self.maskServerFrames):
            b1 |= 1 << 7
            if not mask:
                # note: the RFC mentions "cryptographic randomness"
                # for the masks, which *does* make sense for browser
                # implementations, but not in this case -- for
                # example, a user of this library could just
                # monkey-patch os.urandom (or getrandbits) and predict
                # the masks easily. See issue 758 for more.
                mask = struct.pack("!I", random.getrandbits(32))
                mv = mask
            else:
                mv = b''

            # mask frame payload
            #
            if l > 0 and self.applyMask:
                masker = create_xor_masker(mask, l)
                plm = masker.process(pl)
            else:
                plm = pl

        else:
            mv = b''
            plm = pl

        el = b''
        if l <= 125:
            b1 |= l
        elif l <= 0xFFFF:
            b1 |= 126
            el = struct.pack("!H", l)
        elif l <= 0x7FFFFFFFFFFFFFFF:
            b1 |= 127
            el = struct.pack("!Q", l)
        else:
            raise Exception("invalid payload length")

        if six.PY3:
            raw = b''.join([b0.to_bytes(1, 'big'), b1.to_bytes(1, 'big'), el, mv, plm])
        else:
            raw = b''.join([chr(b0), chr(b1), el, mv, plm])

        if opcode in [0, 1, 2]:
            self.trafficStats.outgoingWebSocketFrames += 1

        if self.logFrames:
            frameHeader = FrameHeader(opcode, fin, rsv, l, mask)
            self.logTxFrame(frameHeader, payload, payload_len, chopsize, sync)

        # send frame octets
        #
        self.sendData(raw, sync, chopsize)

    def sendPing(self, payload=None):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.sendPing`
        """
        if self.state != WebSocketProtocol.STATE_OPEN:
            return
        if payload:
            l = len(payload)
            if l > 125:
                raise Exception("invalid payload for PING (payload length must be <= 125, was %d)" % l)
            self.sendFrame(opcode=9, payload=payload)
        else:
            self.sendFrame(opcode=9)

    def _sendAutoPing(self):
        # Sends an automatic ping and sets up a timeout.
        self.log.debug("Auto ping/pong: sending ping auto-ping/pong")

        self.autoPingPendingCall = None

        self.autoPingPending = newid(self.autoPingSize).encode('utf8')

        self.sendPing(self.autoPingPending)

        if self.autoPingTimeout:
            self.log.debug(
                "Expecting ping in {seconds} seconds for auto-ping/pong",
                seconds=self.autoPingTimeout,
            )
            self.autoPingTimeoutCall = self.factory._batched_timer.call_later(
                self.autoPingTimeout,
                self.onAutoPingTimeout,
            )

    def sendPong(self, payload=None):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.sendPong`
        """
        if self.state != WebSocketProtocol.STATE_OPEN:
            return
        if payload:
            l = len(payload)
            if l > 125:
                raise Exception("invalid payload for PONG (payload length must be <= 125, was %d)" % l)
            self.sendFrame(opcode=10, payload=payload)
        else:
            self.sendFrame(opcode=10)

    def sendCloseFrame(self, code=None, reasonUtf8=None, isReply=False):
        """
        Send a close frame and update protocol state. Note, that this is
        an internal method which deliberately allows not send close
        frame with invalid payload.
        """
        if self.state == WebSocketProtocol.STATE_CLOSING:
            self.log.debug("ignoring sendCloseFrame since connection is closing")

        elif self.state == WebSocketProtocol.STATE_CLOSED:
            self.log.debug("ignoring sendCloseFrame since connection already closed")

        elif self.state in [WebSocketProtocol.STATE_PROXY_CONNECTING, WebSocketProtocol.STATE_CONNECTING]:
            raise Exception("cannot close a connection not yet connected")

        elif self.state == WebSocketProtocol.STATE_OPEN:

            # construct Hybi close frame payload and send frame
            payload = b''
            if code is not None:
                payload += struct.pack("!H", code)
            if reasonUtf8 is not None:
                payload += reasonUtf8
            self.sendFrame(opcode=8, payload=payload)

            # update state
            self.state = WebSocketProtocol.STATE_CLOSING
            self.closedByMe = not isReply

            # remember payload of close frame we sent
            self.localCloseCode = code
            self.localCloseReason = reasonUtf8

            # drop connection when timeout on receiving close handshake reply
            if self.closedByMe and self.closeHandshakeTimeout > 0:
                self.closeHandshakeTimeoutCall = self.factory._batched_timer.call_later(
                    self.closeHandshakeTimeout,
                    self.onCloseHandshakeTimeout,
                )

        else:
            raise Exception("logic error")

    def sendClose(self, code=None, reason=None):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.sendClose`
        """
        if code is not None:
            if type(code) not in six.integer_types:
                raise Exception("invalid type '{}' for close code (must be an integer)".format(type(code)))

            # 1000 Normal Closure
            # 3000-3999 First Come First Served
            # 4000-4999 Reserved for Private Use
            # See: https://www.iana.org/assignments/websocket/websocket.xml#close-code-number-rules
            #
            if code != 1000 and not (3000 <= code <= 4999):
                raise Exception("invalid close code {} (must be 1000 or from [3000, 4999])".format(code))

        if reason is not None:
            if code is None:
                raise Exception("close reason without close code")

            if type(reason) != six.text_type:
                raise Exception("reason must be of type unicode (was '{}')".format(type(reason)))

            reasonUtf8 = encode_truncate(reason, 123)
        else:
            reasonUtf8 = None

        self.sendCloseFrame(code=code, reasonUtf8=reasonUtf8, isReply=False)

    def beginMessage(self, isBinary=False, doNotCompress=False):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.beginMessage`
        """
        if self.state != WebSocketProtocol.STATE_OPEN:
            return

        # check if sending state is valid for this method
        #
        if self.send_state != WebSocketProtocol.SEND_STATE_GROUND:
            raise Exception("WebSocketProtocol.beginMessage invalid in current sending state")

        self.send_message_opcode = WebSocketProtocol.MESSAGE_TYPE_BINARY if isBinary else WebSocketProtocol.MESSAGE_TYPE_TEXT
        self.send_state = WebSocketProtocol.SEND_STATE_MESSAGE_BEGIN

        # setup compressor
        #
        if self._perMessageCompress is not None and not doNotCompress:
            self.send_compressed = True
            self._perMessageCompress.start_compress_message()
        else:
            self.send_compressed = False

        self.trafficStats.outgoingWebSocketMessages += 1

    def beginMessageFrame(self, length):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.beginMessageFrame`
        """
        if self.state != WebSocketProtocol.STATE_OPEN:
            return

        # check if sending state is valid for this method
        #
        if self.send_state not in [WebSocketProtocol.SEND_STATE_MESSAGE_BEGIN, WebSocketProtocol.SEND_STATE_INSIDE_MESSAGE]:
            raise Exception("WebSocketProtocol.beginMessageFrame invalid in current sending state [%d]" % self.send_state)

        if type(length) != int or length < 0 or length > 0x7FFFFFFFFFFFFFFF:  # 2**63
            raise Exception("invalid value for message frame length")

        self.send_message_frame_length = length

        self.trafficStats.outgoingWebSocketFrames += 1

        if (not self.factory.isServer and self.maskClientFrames) or (self.factory.isServer and self.maskServerFrames):
            # automatic mask:
            # - client-to-server masking (if not deactivated)
            # - server-to-client masking (if activated)
            #
            # see note above about getrandbits
            self.send_message_frame_mask = struct.pack("!I", random.getrandbits(32))

        else:
            # no mask
            #
            self.send_message_frame_mask = None

        # payload masker
        #
        if self.send_message_frame_mask and length > 0 and self.applyMask:
            self.send_message_frame_masker = create_xor_masker(self.send_message_frame_mask, length)
        else:
            self.send_message_frame_masker = XorMaskerNull()

        # first byte
        #
        # FIN = false .. since with streaming, we don't know when message ends
        b0 = 0
        if self.send_state == WebSocketProtocol.SEND_STATE_MESSAGE_BEGIN:

            b0 |= self.send_message_opcode % 128

            if self.send_compressed:
                b0 |= (4 % 8) << 4

            self.send_state = WebSocketProtocol.SEND_STATE_INSIDE_MESSAGE
        else:
            pass  # message continuation frame

        # second byte, payload len bytes and mask
        #
        b1 = 0
        if self.send_message_frame_mask:
            b1 |= 1 << 7
            mv = self.send_message_frame_mask
        else:
            mv = b''

        el = b''
        if length <= 125:
            b1 |= length
        elif length <= 0xFFFF:
            b1 |= 126
            el = struct.pack("!H", length)
        elif length <= 0x7FFFFFFFFFFFFFFF:
            b1 |= 127
            el = struct.pack("!Q", length)
        else:
            raise Exception("invalid payload length")

        # write message frame header
        #
        if six.PY3:
            header = b''.join([b0.to_bytes(1, 'big'), b1.to_bytes(1, 'big'), el, mv])
        else:
            header = b''.join([chr(b0), chr(b1), el, mv])

        self.sendData(header)

        # now we are inside message frame ..
        #
        self.send_state = WebSocketProtocol.SEND_STATE_INSIDE_MESSAGE_FRAME

    def sendMessageFrameData(self, payload, sync=False):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.sendMessageFrameData`
        """
        if self.state != WebSocketProtocol.STATE_OPEN:
            return

        if not self.send_compressed:
            self.trafficStats.outgoingOctetsAppLevel += len(payload)
        self.trafficStats.outgoingOctetsWebSocketLevel += len(payload)

        if self.send_state != WebSocketProtocol.SEND_STATE_INSIDE_MESSAGE_FRAME:
            raise Exception("WebSocketProtocol.sendMessageFrameData invalid in current sending state")

        rl = len(payload)
        if self.send_message_frame_masker.pointer() + rl > self.send_message_frame_length:
            l = self.send_message_frame_length - self.send_message_frame_masker.pointer()
            rest = -(rl - l)
            pl = payload[:l]
        else:
            l = rl
            rest = self.send_message_frame_length - self.send_message_frame_masker.pointer() - l
            pl = payload

        # mask frame payload
        #
        plm = self.send_message_frame_masker.process(pl)

        # send frame payload
        #
        self.sendData(plm, sync=sync)

        # if we are done with frame, move back into "inside message" state
        #
        if self.send_message_frame_masker.pointer() >= self.send_message_frame_length:
            self.send_state = WebSocketProtocol.SEND_STATE_INSIDE_MESSAGE

        # when =0 : frame was completed exactly
        # when >0 : frame is still uncomplete and that much amount is still left to complete the frame
        # when <0 : frame was completed and there was this much unconsumed data in payload argument
        #
        return rest

    def endMessage(self):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.endMessage`
        """
        if self.state != WebSocketProtocol.STATE_OPEN:
            return

        # check if sending state is valid for this method
        #
        # if self.send_state != WebSocketProtocol.SEND_STATE_INSIDE_MESSAGE:
        #   raise Exception("WebSocketProtocol.endMessage invalid in current sending state [%d]" % self.send_state)

        if self.send_compressed:
            payload = self._perMessageCompress.end_compress_message()
            self.trafficStats.outgoingOctetsWebSocketLevel += len(payload)
        else:
            # send continuation frame with empty payload and FIN set to end message
            payload = b''
        self.sendFrame(opcode=0, payload=payload, fin=True)

        self.send_state = WebSocketProtocol.SEND_STATE_GROUND

    def sendMessageFrame(self, payload, sync=False):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.sendMessageFrame`
        """
        if self.state != WebSocketProtocol.STATE_OPEN:
            return

        if self.send_compressed:
            self.trafficStats.outgoingOctetsAppLevel += len(payload)
            payload = self._perMessageCompress.compress_message_data(payload)

        self.beginMessageFrame(len(payload))
        self.sendMessageFrameData(payload, sync)

    def sendMessage(self,
                    payload,
                    isBinary=False,
                    fragmentSize=None,
                    sync=False,
                    doNotCompress=False):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketChannel.sendMessage`
        """
        assert(type(payload) == bytes)

        if self.state != WebSocketProtocol.STATE_OPEN:
            return

        if self.trackedTimings:
            self.trackedTimings.track("sendMessage")

        # (initial) frame opcode
        #
        if isBinary:
            opcode = 2
        else:
            opcode = 1

        self.trafficStats.outgoingWebSocketMessages += 1

        # setup compressor
        #
        if self._perMessageCompress is not None and not doNotCompress:
            sendCompressed = True

            self._perMessageCompress.start_compress_message()

            self.trafficStats.outgoingOctetsAppLevel += len(payload)

            payload1 = self._perMessageCompress.compress_message_data(payload)
            payload2 = self._perMessageCompress.end_compress_message()
            payload = b''.join([payload1, payload2])

            self.trafficStats.outgoingOctetsWebSocketLevel += len(payload)

        else:
            sendCompressed = False
            l = len(payload)
            self.trafficStats.outgoingOctetsAppLevel += l
            self.trafficStats.outgoingOctetsWebSocketLevel += l

        # explicit fragmentSize arguments overrides autoFragmentSize setting
        #
        if fragmentSize is not None:
            pfs = fragmentSize
        else:
            if self.autoFragmentSize > 0:
                pfs = self.autoFragmentSize
            else:
                pfs = None

        # send unfragmented
        #
        if pfs is None or len(payload) <= pfs:
            self.sendFrame(opcode=opcode, payload=payload, sync=sync, rsv=4 if sendCompressed else 0)

        # send data message in fragments
        #
        else:
            if pfs < 1:
                raise Exception("payload fragment size must be at least 1 (was %d)" % pfs)
            n = len(payload)
            i = 0
            done = False
            first = True
            while not done:
                j = i + pfs
                if j > n:
                    done = True
                    j = n
                if first:
                    self.sendFrame(opcode=opcode, payload=payload[i:j], fin=done, sync=sync, rsv=4 if sendCompressed else 0)
                    first = False
                else:
                    self.sendFrame(opcode=0, payload=payload[i:j], fin=done, sync=sync)
                i += pfs

    def _parseExtensionsHeader(self, header, removeQuotes=True):
        """
        Parse the Sec-WebSocket-Extensions header.
        """
        extensions = []
        exts = [str(x.strip()) for x in header.split(',')]
        for e in exts:
            if e != "":
                ext = [x.strip() for x in e.split(";")]
                if len(ext) > 0:
                    extension = ext[0].lower()
                    params = {}
                    for p in ext[1:]:
                        p = [x.strip() for x in p.split("=")]
                        key = p[0].lower()
                        if len(p) > 1:
                            value = "=".join(p[1:])
                            if removeQuotes:
                                if len(value) > 0 and value[0] == '"':
                                    value = value[1:]
                                if len(value) > 0 and value[-1] == '"':
                                    value = value[:-1]
                        else:
                            value = True
                        if key not in params:
                            params[key] = []
                        params[key].append(value)
                    extensions.append((extension, params))
                else:
                    pass  # should not arrive here
        return extensions


IWebSocketChannel.register(WebSocketProtocol)
IWebSocketChannelFrameApi.register(WebSocketProtocol)
IWebSocketChannelStreamingApi.register(WebSocketProtocol)


class PreparedMessage(object):
    """
    Encapsulates a prepared message to be sent later once or multiple
    times on one or more WebSocket connections.
    This can be used for optimizing Broadcast/PubSub.
    """

    def __init__(self, payload, isBinary, applyMask, doNotCompress):
        """
        Ctor for a prepared message.

        :param payload: The message payload.
        :type payload: str
        :param isBinary: Provide `True` for binary payload.
        :type isBinary: bool
        :param applyMask: Provide `True` if WebSocket message is to be masked (required for client to server WebSocket messages).
        :type applyMask: bool
        :param doNotCompress: Iff `True`, never compress this message. This
            only applies when WebSocket compression has been negotiated on the
            WebSocket connection. Use when you know the payload incompressible
            (e.g. encrypted or already compressed).
        :type doNotCompress: bool
        """
        if not doNotCompress:
            # we need to store original payload for compressed WS
            # connections (cannot compress/frame in advanced when
            # compression is on, and context takeover is off)
            self.payload = payload
            self.binary = isBinary
        self.doNotCompress = doNotCompress

        l = len(payload)

        # first byte
        #
        b0 = ((1 << 7) | 2) if isBinary else ((1 << 7) | 1)

        # second byte, payload len bytes and mask
        #
        if applyMask:
            b1 = 1 << 7
            # see note above about getrandbits
            mask = struct.pack("!I", random.getrandbits(32))
            if l == 0:
                plm = payload
            else:
                plm = create_xor_masker(mask, l).process(payload)
        else:
            b1 = 0
            mask = b''
            plm = payload

        # payload extended length
        #
        el = b''
        if l <= 125:
            b1 |= l
        elif l <= 0xFFFF:
            b1 |= 126
            el = struct.pack("!H", l)
        elif l <= 0x7FFFFFFFFFFFFFFF:
            b1 |= 127
            el = struct.pack("!Q", l)
        else:
            raise Exception("invalid payload length")

        # raw WS message (single frame)
        #
        if six.PY3:
            self.payloadHybi = b''.join([b0.to_bytes(1, 'big'), b1.to_bytes(1, 'big'), el, mask, plm])
        else:
            self.payloadHybi = b''.join([chr(b0), chr(b1), el, mask, plm])


class WebSocketFactory(object):
    """
    Mixin for
    :class:`autobahn.websocket.protocol.WebSocketClientFactory` and
    :class:`autobahn.websocket.protocol.WebSocketServerFactory`.
    """

    def prepareMessage(self, payload, isBinary=False, doNotCompress=False):
        """
        Prepare a WebSocket message. This can be later sent on multiple
        instances of :class:`autobahn.websocket.WebSocketProtocol` using
        :meth:`autobahn.websocket.WebSocketProtocol.sendPreparedMessage`.

        By doing so, you can avoid the (small) overhead of framing the
        *same* payload into WebSocket messages multiple times when that
        same payload is to be sent out on multiple connections.

        :param payload: The message payload.
        :type payload: bytes
        :param isBinary: `True` iff payload is binary, else the payload must be
            UTF-8 encoded text.
        :type isBinary: bool
        :param doNotCompress: Iff `True`, never compress this message. This
            only applies when WebSocket compression has been negotiated on the
            WebSocket connection. Use when you know the payload incompressible
            (e.g. encrypted or already compressed).
        :type doNotCompress: bool

        :returns: obj -- An instance of :class:`autobahn.websocket.protocol.PreparedMessage`.
        """
        applyMask = not self.isServer
        return PreparedMessage(payload, isBinary, applyMask, doNotCompress)


_SERVER_STATUS_TEMPLATE = """<!DOCTYPE html>
<html>
   <head>
      %s
      <style>
         body {
            color: #fff;
            background-color: #027eae;
            font-family: "Segoe UI", "Lucida Grande", "Helvetica Neue", Helvetica, Arial, sans-serif;
            font-size: 16px;
         }

         a, a:visited, a:hover {
            color: #fff;
         }
      </style>
   </head>
   <body>
      <h1>AutobahnPython %s</h1>
      <p>
         I am not Web server, but a <b>WebSocket Endpoint</b>.
      </p>
      <p>
         You can talk to me using the <a href="http://tools.ietf.org/html/rfc6455">WebSocket</a> protocol.
      </p>
      <p>
         For more information, please see:
         <ul>
            <li><a href="http://crossbar.io/autobahn">Autobahn</a></li>
         </ul>
      </p>
   </body>
</html>
"""


class WebSocketServerProtocol(WebSocketProtocol):
    """
    Protocol base class for WebSocket servers.
    """

    log = txaio.make_logger()

    CONFIG_ATTRS = WebSocketProtocol.CONFIG_ATTRS_COMMON + WebSocketProtocol.CONFIG_ATTRS_SERVER

    def onConnect(self, request):
        """
        Callback fired during WebSocket opening handshake when new WebSocket client
        connection is about to be established.

        When you want to accept the connection, return the accepted protocol
        from list of WebSocket (sub)protocols provided by client or `None` to
        speak no specific one or when the client protocol list was empty.

        You may also return a pair of `(protocol, headers)` to send additional
        HTTP headers, with `headers` being a dictionary of key-values.

        Throw :class:`autobahn.websocket.types.ConnectionDeny` when you don't want
        to accept the WebSocket connection request.

        :param request: WebSocket connection request information.
        :type request: instance of :class:`autobahn.websocket.protocol.ConnectionRequest`
        """
        return None

    def _connectionMade(self):
        """
        Called by network framework when new transport connection from client was
        accepted. Default implementation will prepare for initial WebSocket opening
        handshake. When overriding in derived class, make sure to call this base class
        implementation *before* your code.
        """
        WebSocketProtocol._connectionMade(self)
        self.factory.countConnections += 1
        self.log.debug("connection accepted from peer {peer}", peer=self.peer)

    def _connectionLost(self, reason):
        """
        Called by network framework when established transport connection from client
        was lost. Default implementation will tear down all state properly.
        When overriding in derived class, make sure to call this base class
        implementation *after* your code.
        """
        WebSocketProtocol._connectionLost(self, reason)
        self.factory.countConnections -= 1

    def processProxyConnect(self):
        raise Exception("Autobahn isn't a proxy server")

    def processHandshake(self):
        """
        Process WebSocket opening handshake request from client.
        """
        # only proceed when we have fully received the HTTP request line and all headers
        #
        end_of_header = self.data.find(b"\x0d\x0a\x0d\x0a")
        if end_of_header >= 0:

            self.http_request_data = self.data[:end_of_header + 4]
            self.log.debug(
                "received HTTP request:\n\n{data}\n\n",
                data=self.http_request_data,
            )

            # extract HTTP status line and headers
            #
            try:
                self.http_status_line, self.http_headers, http_headers_cnt = parseHttpHeader(self.http_request_data)
            except Exception as e:
                return self.failHandshake("Error during parsing of HTTP status line / request headers : {0}".format(e))

            # replace self.peer if the x-forwarded-for header is present and trusted
            #
            if 'x-forwarded-for' in self.http_headers and self.trustXForwardedFor:
                addresses = [x.strip() for x in self.http_headers['x-forwarded-for'].split(',')]
                trusted_addresses = addresses[-self.trustXForwardedFor:]
                self.peer = trusted_addresses[0]

            # validate WebSocket opening handshake client request
            #
            self.log.debug(
                "received HTTP status line in opening handshake : {status}",
                status=self.http_status_line,
            )
            self.log.debug(
                "received HTTP headers in opening handshake : {headers}",
                headers=self.http_headers,
            )

            # HTTP Request line : METHOD, VERSION
            #
            rl = self.http_status_line.split()
            if len(rl) != 3:
                return self.failHandshake("Bad HTTP request status line '%s'" % self.http_status_line)
            if rl[0].strip() != "GET":
                return self.failHandshake("HTTP method '%s' not allowed" % rl[0], 405)
            vs = rl[2].strip().split("/")
            if len(vs) != 2 or vs[0] != "HTTP" or vs[1] not in ["1.1"]:
                return self.failHandshake("Unsupported HTTP version '%s'" % rl[2], 505)

            # HTTP Request line : REQUEST-URI
            #
            self.http_request_uri = rl[1].strip()
            try:
                (scheme, netloc, path, params, query, fragment) = urllib.parse.urlparse(self.http_request_uri)

                # FIXME: check that if absolute resource URI is given,
                # the scheme/netloc matches the server
                if scheme != "" or netloc != "":
                    pass

                # Fragment identifiers are meaningless in the context of WebSocket
                # URIs, and MUST NOT be used on these URIs.
                if fragment != "":
                    return self.failHandshake("HTTP requested resource contains a fragment identifier '%s'" % fragment)

                # resource path and query parameters .. this will get forwarded
                # to onConnect()
                self.http_request_path = path
                self.http_request_params = urllib.parse.parse_qs(query)
            except:
                return self.failHandshake("Bad HTTP request resource - could not parse '%s'" % rl[1].strip())

            # Host
            #
            if 'host' not in self.http_headers:
                return self.failHandshake("HTTP Host header missing in opening handshake request")

            if http_headers_cnt["host"] > 1:
                return self.failHandshake("HTTP Host header appears more than once in opening handshake request")

            self.http_request_host = self.http_headers["host"].strip()

            if self.http_request_host.find(":") >= 0 and not self.http_request_host.endswith(']'):
                (h, p) = self.http_request_host.rsplit(":", 1)
                try:
                    port = int(str(p.strip()))
                except ValueError:
                    return self.failHandshake("invalid port '%s' in HTTP Host header '%s'" % (str(p.strip()), str(self.http_request_host)))

                # do port checking only if externalPort or URL was set
                if self.factory.externalPort:
                    if port != self.factory.externalPort:
                        return self.failHandshake("port %d in HTTP Host header '%s' does not match server listening port %s" % (port, str(self.http_request_host), self.factory.externalPort))
                else:
                    self.log.debug("skipping opening handshake port checking - neither WS URL nor external port set")

                self.http_request_host = h

            else:
                # do port checking only if externalPort or URL was set
                if self.factory.externalPort:
                    if not ((self.factory.isSecure and self.factory.externalPort == 443) or (not self.factory.isSecure and self.factory.externalPort == 80)):
                        return self.failHandshake("missing port in HTTP Host header '%s' and server runs on non-standard port %d (wss = %s)" % (str(self.http_request_host), self.factory.externalPort, self.factory.isSecure))
                else:
                    self.log.debug("skipping opening handshake port checking - neither WS URL nor external port set")

            # Upgrade
            #
            if 'upgrade' not in self.http_headers:
                # When no WS upgrade, render HTML server status page
                #
                if self.webStatus:
                    if 'redirect' in self.http_request_params and len(self.http_request_params['redirect']) > 0:
                        # To specify an URL for redirection, encode the URL, i.e. from JavaScript:
                        #
                        # var url = encodeURIComponent("http://crossbar.io/autobahn");
                        #
                        # and append the encoded string as a query parameter 'redirect'
                        #
                        # http://localhost:9000?redirect=http%3A%2F%2Fcrossbar.io%2Fautobahn
                        # https://localhost:9000?redirect=https%3A%2F%2Ftwitter.com%2F
                        #
                        # This will perform an immediate HTTP-303 redirection. If you provide
                        # an additional parameter 'after' (int >= 0), the redirection happens
                        # via Meta-Refresh in the rendered HTML status page, i.e.
                        #
                        # https://localhost:9000/?redirect=https%3A%2F%2Ftwitter.com%2F&after=3
                        #
                        url = self.http_request_params['redirect'][0]
                        if 'after' in self.http_request_params and len(self.http_request_params['after']) > 0:
                            after = int(self.http_request_params['after'][0])
                            self.log.debug(
                                "HTTP Upgrade header missing : render server status page and "
                                "meta-refresh-redirecting to {url} after {duration} seconds",
                                url=url,
                                duration=after,
                            )
                            self.sendServerStatus(url, after)
                        else:
                            self.log.debug(
                                "HTTP Upgrade header missing : 303-redirecting to {url}",
                                url=url,
                            )
                            self.sendRedirect(url)
                    else:
                        self.log.debug("HTTP Upgrade header missing : render server status page")
                        self.sendServerStatus()
                    self.dropConnection(abort=False)
                    return
                else:
                    return self.failHandshake("HTTP Upgrade header missing", 426)  # Upgrade Required
            upgradeWebSocket = False
            for u in self.http_headers["upgrade"].split(","):
                if u.strip().lower() == "websocket":
                    upgradeWebSocket = True
                    break
            if not upgradeWebSocket:
                return self.failHandshake("HTTP Upgrade headers do not include 'websocket' value (case-insensitive) : %s" % self.http_headers["upgrade"])

            # Connection
            #
            if 'connection' not in self.http_headers:
                return self.failHandshake("HTTP Connection header missing")
            connectionUpgrade = False
            for c in self.http_headers["connection"].split(","):
                if c.strip().lower() == "upgrade":
                    connectionUpgrade = True
                    break
            if not connectionUpgrade:
                return self.failHandshake("HTTP Connection headers do not include 'upgrade' value (case-insensitive) : %s" % self.http_headers["connection"])

            # Sec-WebSocket-Version PLUS determine mode: Hybi or Hixie
            #
            if 'sec-websocket-version' not in self.http_headers:
                self.log.debug("Hixie76 protocol detected")
                return self.failHandshake("WebSocket connection denied - Hixie76 protocol not supported.")
            else:
                self.log.debug("Hybi protocol detected")
                if http_headers_cnt["sec-websocket-version"] > 1:
                    return self.failHandshake("HTTP Sec-WebSocket-Version header appears more than once in opening handshake request")
                try:
                    version = int(self.http_headers["sec-websocket-version"])
                except ValueError:
                    return self.failHandshake("could not parse HTTP Sec-WebSocket-Version header '%s' in opening handshake request" % self.http_headers["sec-websocket-version"])

            if version not in self.versions:

                # respond with list of supported versions (descending order)
                #
                sv = sorted(self.versions)
                sv.reverse()
                svs = ','.join([str(x) for x in sv])
                return self.failHandshake("WebSocket version %d not supported (supported versions: %s)" % (version, svs),
                                          400,  # Bad Request
                                          [("Sec-WebSocket-Version", svs)])
            else:
                # store the protocol version we are supposed to talk
                self.websocket_version = version

            # Sec-WebSocket-Protocol
            #
            if 'sec-websocket-protocol' in self.http_headers:
                protocols = [str(x.strip()) for x in self.http_headers["sec-websocket-protocol"].split(",")]
                # check for duplicates in protocol header
                pp = {}
                for p in protocols:
                    if p in pp:
                        return self.failHandshake("duplicate protocol '%s' specified in HTTP Sec-WebSocket-Protocol header" % p)
                    else:
                        pp[p] = 1
                # ok, no duplicates, save list in order the client sent it
                self.websocket_protocols = protocols
            else:
                self.websocket_protocols = []

            # Origin / Sec-WebSocket-Origin
            # http://tools.ietf.org/html/draft-ietf-websec-origin-02
            #
            if self.websocket_version < 13:
                # Hybi, but only < Hybi-13
                websocket_origin_header_key = 'sec-websocket-origin'
            else:
                # RFC6455, >= Hybi-13
                websocket_origin_header_key = "origin"

            self.websocket_origin = ""
            if websocket_origin_header_key in self.http_headers:
                if http_headers_cnt[websocket_origin_header_key] > 1:
                    return self.failHandshake("HTTP Origin header appears more than once in opening handshake request")
                self.websocket_origin = self.http_headers[websocket_origin_header_key].strip()
                try:
                    origin_tuple = _url_to_origin(self.websocket_origin)
                except ValueError as e:
                    return self.failHandshake(
                        "HTTP Origin header invalid: {}".format(e)
                    )
                have_origin = True
            else:
                # non-browser clients are allowed to omit this header
                have_origin = False

            if have_origin:
                if origin_tuple == 'null' and self.factory.allowNullOrigin:
                    origin_is_allowed = True
                else:
                    origin_is_allowed = _is_same_origin(
                        origin_tuple,
                        'https' if self.factory.isSecure else 'http',
                        self.factory.externalPort or self.factory.port,
                        self.allowedOriginsPatterns,
                    )
                if not origin_is_allowed:
                    return self.failHandshake(
                        "WebSocket connection denied: origin '{0}' "
                        "not allowed".format(self.websocket_origin)
                    )

            # Sec-WebSocket-Key
            #
            if 'sec-websocket-key' not in self.http_headers:
                return self.failHandshake("HTTP Sec-WebSocket-Key header missing")
            if http_headers_cnt["sec-websocket-key"] > 1:
                return self.failHandshake("HTTP Sec-WebSocket-Key header appears more than once in opening handshake request")
            key = self.http_headers["sec-websocket-key"].strip()
            if len(key) != 24:  # 16 bytes => (ceil(128/24)*24)/6 == 24
                return self.failHandshake("bad Sec-WebSocket-Key (length must be 24 ASCII chars) '%s'" % key)
            if key[-2:] != "==":  # 24 - ceil(128/6) == 2
                return self.failHandshake("bad Sec-WebSocket-Key (invalid base64 encoding) '%s'" % key)
            for c in key[:-2]:
                if c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+/":
                    return self.failHandshake("bad character '%s' in Sec-WebSocket-Key (invalid base64 encoding) '%s'" % (c, key))

            # Sec-WebSocket-Extensions
            #
            self.websocket_extensions = []
            if 'sec-websocket-extensions' in self.http_headers:
                if http_headers_cnt["sec-websocket-extensions"] > 1:
                    return self.failHandshake("HTTP Sec-WebSocket-Extensions header appears more than once in opening handshake request")
                else:
                    # extensions requested/offered by client
                    #
                    self.websocket_extensions = self._parseExtensionsHeader(self.http_headers["sec-websocket-extensions"])

            # Ok, got complete HS input, remember rest (if any)
            #
            self.data = self.data[end_of_header + 4:]

            # store WS key
            #
            # noinspection PyUnboundLocalVariable
            self._wskey = key

            # DoS protection
            #
            if self.maxConnections > 0 and self.factory.countConnections > self.maxConnections:

                # maximum number of concurrent connections reached
                #
                self.failHandshake("maximum number of connections reached", code=503)  # Service Unavailable

            else:
                # WebSocket handshake validated => produce opening handshake response
                #
                request = ConnectionRequest(self.peer,
                                            self.http_headers,
                                            self.http_request_host,
                                            self.http_request_path,
                                            self.http_request_params,
                                            self.websocket_version,
                                            self.websocket_origin,
                                            self.websocket_protocols,
                                            self.websocket_extensions)

                # The user's onConnect() handler must do one of the following:
                #   - return the subprotocol to be spoken
                #   - return None to continue with no subprotocol
                #   - return a pair (subprotocol, headers)
                #   - raise a ConnectionDeny to dismiss the client
                f = txaio.as_future(self.onConnect, request)

                def forward_error(err):
                    if isinstance(err.value, ConnectionDeny):
                        # the user handler explicitly denies the connection
                        self.failHandshake(err.value.reason, err.value.code)
                    else:
                        # the user handler ran into an unexpected error (and hence, user code needs fixing!)
                        self.log.warn("Unexpected exception in onConnect ['{err.value}']", err=err)
                        self.log.warn("{tb}", tb=txaio.failure_format_traceback(err))
                        return self.failHandshake("Internal server error: {}".format(err.value), ConnectionDeny.INTERNAL_SERVER_ERROR)

                txaio.add_callbacks(f, self.succeedHandshake, forward_error)

        elif self.serveFlashSocketPolicy:
            flash_policy_file_request = self.data.find(b"<policy-file-request/>\x00")
            if flash_policy_file_request >= 0:
                self.log.debug("received Flash Socket Policy File request")

                if self.serveFlashSocketPolicy:
                    self.log.debug(
                        "sending Flash Socket Policy File :\n{policy}",
                        policy=self.flashSocketPolicy,
                    )

                    self.sendData(self.flashSocketPolicy.encode('utf8'))

                    self.wasServingFlashSocketPolicyFile = True

                    self.dropConnection()
                else:
                    self.log.debug(
                        "No Flash Policy File served. You might want to serve a"
                        " Flask Socket Policy file on the destination port "
                        "since you received a request for it. See "
                        "WebSocketServerFactory.serveFlashSocketPolicy and "
                        "WebSocketServerFactory.flashSocketPolicy"
                    )

    def succeedHandshake(self, res):
        """
        Callback after onConnect() returns successfully. Generates the response for the handshake.
        """
        protocol = None
        headers = {}
        if type(res) == tuple:
            if len(res) > 0:
                protocol = res[0]
            if len(res) > 1:
                headers = res[1]
        else:
            protocol = res

        if protocol is not None and not (protocol in self.websocket_protocols):
            raise Exception("protocol accepted must be from the list client sent or None")

        self.websocket_protocol_in_use = protocol
        key = self._wskey

        # extensions effectively in use for this connection
        #
        self.websocket_extensions_in_use = []

        extensionResponse = []

        # gets filled with permessage-compress offers from the client
        #
        pmceOffers = []

        # handle WebSocket extensions
        #
        for (extension, params) in self.websocket_extensions:

            self.log.debug(
                "parsed WebSocket extension '{extension}' with params '{params}'",
                extension=extension,
                params=params,
            )

            # process permessage-compress extension
            #
            if extension in PERMESSAGE_COMPRESSION_EXTENSION:

                PMCE = PERMESSAGE_COMPRESSION_EXTENSION[extension]

                try:
                    offer = PMCE['Offer'].parse(params)
                    pmceOffers.append(offer)
                except Exception as e:
                    return self.failHandshake(str(e))

            else:
                self.log.debug(
                    "client requested '{extension}' extension we don't support "
                    "or which is not activated",
                    extension=extension,
                )

        # handle permessage-compress offers by the client
        #
        if len(pmceOffers) > 0:
            accept = self.perMessageCompressionAccept(pmceOffers)
            if accept is not None:
                PMCE = PERMESSAGE_COMPRESSION_EXTENSION[accept.EXTENSION_NAME]
                self._perMessageCompress = PMCE['PMCE'].create_from_offer_accept(self.factory.isServer, accept)
                self.websocket_extensions_in_use.append(self._perMessageCompress)
                extensionResponse.append(accept.get_extension_string())
            else:
                self.log.debug(
                    "client request permessage-compress extension, but we did "
                    "not accept any offer [{offers}]",
                    offers=pmceOffers,
                )

        # build response to complete WebSocket handshake
        #
        response = "HTTP/1.1 101 Switching Protocols\x0d\x0a"

        if self.factory.server:
            response += "Server: %s\x0d\x0a" % self.factory.server

        response += "Upgrade: WebSocket\x0d\x0a"
        response += "Connection: Upgrade\x0d\x0a"

        # optional, user supplied additional HTTP headers
        #
        # headers from factory, headers from onConnect
        for headers_source in (self.factory.headers.items(), headers.items()):
            for uh in headers_source:
                if isinstance(uh[1], six.string_types):
                    header_values = [uh[1]]
                else:
                    try:
                        header_values = iter(uh[1])
                    except TypeError:
                        header_values = [uh[1]]

                for header_value in header_values:
                    response += "%s: %s\x0d\x0a" % (uh[0], header_value)

        if self.websocket_protocol_in_use is not None:
            response += "Sec-WebSocket-Protocol: %s\x0d\x0a" % str(self.websocket_protocol_in_use)

        # compute Sec-WebSocket-Accept
        #
        sha1 = hashlib.sha1()
        # noinspection PyUnboundLocalVariable
        sha1.update(key.encode('utf8') + WebSocketProtocol._WS_MAGIC)
        sec_websocket_accept = base64.b64encode(sha1.digest())

        response += "Sec-WebSocket-Accept: %s\x0d\x0a" % sec_websocket_accept.decode()

        # agreed extensions
        #
        if len(extensionResponse) > 0:
            response += "Sec-WebSocket-Extensions: %s\x0d\x0a" % ', '.join(extensionResponse)

        # end of HTTP response headers
        response += "\x0d\x0a"
        response_body = None

        # send out opening handshake response
        #
        self.log.debug("sending HTTP response:\n\n{response}", response=response)
        self.sendData(response.encode('utf8'))

        if response_body:
            self.log.debug(
                "sending HTTP response body:\n\n{octets}",
                octets=_LazyHexFormatter(response_body),
            )
            self.sendData(response_body)

        # save response for testsuite
        #
        self.http_response_data = response

        # opening handshake completed, move WebSocket connection into OPEN state
        #
        self.state = WebSocketProtocol.STATE_OPEN

        # cancel any opening HS timer if present
        #
        if self.openHandshakeTimeoutCall is not None:
            self.log.debug("openHandshakeTimeoutCall.cancel")
            self.openHandshakeTimeoutCall.cancel()
            self.openHandshakeTimeoutCall = None

        # init state
        #
        self.inside_message = False
        self.current_frame = None

        # automatic ping/pong
        #
        if self.autoPingInterval:
            self.autoPingPendingCall = self.factory._batched_timer.call_later(
                self.autoPingInterval,
                self._sendAutoPing,
            )

        # fire handler on derived class
        #
        if self.trackedTimings:
            self.trackedTimings.track("onOpen")
        self._onOpen()

        # process rest, if any
        #
        if len(self.data) > 0:
            self.consumeData()

    def failHandshake(self, reason, code=400, responseHeaders=None):
        """
        During opening handshake the client request was invalid, we send a HTTP
        error response and then drop the connection.
        """
        self.wasNotCleanReason = reason
        self.log.info("failing WebSocket opening handshake ('{reason}')", reason=reason)
        self.sendHttpErrorResponse(code, reason, responseHeaders)
        self.dropConnection(abort=False)

    def sendHttpErrorResponse(self, code, reason, responseHeaders=None):
        """
        Send out HTTP error response.
        """
        response = "HTTP/1.1 {0} {1}\x0d\x0a".format(code, reason)
        if responseHeaders:
            for h in responseHeaders:
                response += "{0}: {1}\x0d\x0a".format(h[0], h[1])
        response += "\x0d\x0a"
        self.sendData(response.encode('utf8'))

    def sendHtml(self, html):
        """
        Send HTML page HTTP response.
        """
        responseBody = html.encode('utf8')
        response = "HTTP/1.1 200 OK\x0d\x0a"
        if self.factory.server is not None and self.factory.server != "":
            response += "Server: %s\x0d\x0a" % self.factory.server
        response += "Content-Type: text/html; charset=UTF-8\x0d\x0a"
        response += "Content-Length: %d\x0d\x0a" % len(responseBody)
        response += "\x0d\x0a"
        self.sendData(response.encode('utf8'))
        self.sendData(responseBody)

    def sendRedirect(self, url):
        """
        Send HTTP Redirect (303) response.
        """
        response = "HTTP/1.1 303\x0d\x0a"
        if self.factory.server is not None and self.factory.server != "":
            response += "Server: %s\x0d\x0a" % self.factory.server
        response += "Location: %s\x0d\x0a" % url
        response += "\x0d\x0a"
        self.sendData(response.encode('utf8'))

    def sendServerStatus(self, redirectUrl=None, redirectAfter=0):
        """
        Used to send out server status/version upon receiving a HTTP/GET without
        upgrade to WebSocket header (and option serverStatus is True).
        """
        if redirectUrl:
            redirect = """<meta http-equiv="refresh" content="%d;URL='%s'">""" % (redirectAfter, redirectUrl)
        else:
            redirect = ""
        self.sendHtml(_SERVER_STATUS_TEMPLATE % (redirect, __version__))


class WebSocketServerFactory(WebSocketFactory):
    """
    A protocol factory for WebSocket servers.

    Implements :func:`autobahn.websocket.interfaces.IWebSocketServerChannelFactory`
    """

    log = txaio.make_logger()

    protocol = WebSocketServerProtocol
    """
    The protocol to be spoken. Must be derived from :class:`autobahn.websocket.protocol.WebSocketServerProtocol`.
    """

    isServer = True
    """
    Flag indicating if this factory is client- or server-side.
    """

    def __init__(self,
                 url=None,
                 protocols=None,
                 server="AutobahnPython/%s" % __version__,
                 headers=None,
                 externalPort=None):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketServerChannelFactory.__init__`
        """
        self.logOctets = False
        self.logFrames = False
        self.trackTimings = False

        # batch up and chunk timers ("call_later")
        self._batched_timer = txaio.make_batched_timer(
            bucket_seconds=0.200,
            chunk_size=1000,
        )

        # seed RNG which is used for WS frame masks generation
        random.seed()

        # default WS session parameters
        #
        self.setSessionParameters(url, protocols, server, headers, externalPort)

        # default WebSocket protocol options
        #
        self.resetProtocolOptions()

        # number of currently connected clients
        #
        self.countConnections = 0

    def setSessionParameters(self,
                             url=None,
                             protocols=None,
                             server=None,
                             headers=None,
                             externalPort=None):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketServerChannelFactory.setSessionParameters`
        """

        # parse WebSocket URI into components
        (isSecure, host, port, resource, path, params) = parse_url(url or "ws://localhost")
        if len(params) > 0:
            raise Exception("query parameters specified for server WebSocket URL")
        self.url = url
        self.isSecure = isSecure
        self.host = host
        self.port = port
        self.resource = resource
        self.path = path
        self.params = params

        self.protocols = protocols or []
        self.server = server
        self.headers = headers or {}

        if externalPort:
            self.externalPort = externalPort
        elif url:
            self.externalPort = self.port
        else:
            self.externalPort = None

    def resetProtocolOptions(self):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketServerChannelFactory.resetProtocolOptions`
        """
        self.versions = WebSocketProtocol.SUPPORTED_PROTOCOL_VERSIONS
        self.webStatus = True
        self.utf8validateIncoming = True
        self.requireMaskedClientFrames = True
        self.maskServerFrames = False
        self.applyMask = True
        self.maxFramePayloadSize = 0
        self.maxMessagePayloadSize = 0
        self.autoFragmentSize = 0
        self.failByDrop = True
        self.echoCloseCodeReason = False
        self.openHandshakeTimeout = 5
        self.closeHandshakeTimeout = 1
        self.tcpNoDelay = True
        self.serveFlashSocketPolicy = False
        self.flashSocketPolicy = u'''<cross-domain-policy>
     <allow-access-from domain="*" to-ports="*" />
</cross-domain-policy>\x00'''

        # permessage-XXX extension
        #
        self.perMessageCompressionAccept = lambda _: None

        # automatic ping/pong ("heartbeating")
        #
        self.autoPingInterval = 0
        self.autoPingTimeout = 0
        self.autoPingSize = 4

        # check WebSocket origin against this list
        self.allowedOrigins = ["*"]
        self.allowedOriginsPatterns = wildcards2patterns(self.allowedOrigins)
        self.allowNullOrigin = True

        # maximum number of concurrent connections
        self.maxConnections = 0

        # number of trusted web servers in front of this server
        self.trustXForwardedFor = 0

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
                           trustXForwardedFor=None):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketServerChannelFactory.setProtocolOptions`
        """
        if versions is not None:
            for v in versions:
                if v not in WebSocketProtocol.SUPPORTED_PROTOCOL_VERSIONS:
                    raise Exception("invalid WebSocket protocol version %s (allowed values: %s)" % (v, str(WebSocketProtocol.SUPPORTED_PROTOCOL_VERSIONS)))
            if set(versions) != set(self.versions):
                self.versions = versions

        if webStatus is not None and webStatus != self.webStatus:
            self.webStatus = webStatus

        if utf8validateIncoming is not None and utf8validateIncoming != self.utf8validateIncoming:
            self.utf8validateIncoming = utf8validateIncoming

        if requireMaskedClientFrames is not None and requireMaskedClientFrames != self.requireMaskedClientFrames:
            self.requireMaskedClientFrames = requireMaskedClientFrames

        if maskServerFrames is not None and maskServerFrames != self.maskServerFrames:
            self.maskServerFrames = maskServerFrames

        if applyMask is not None and applyMask != self.applyMask:
            self.applyMask = applyMask

        if maxFramePayloadSize is not None and maxFramePayloadSize != self.maxFramePayloadSize:
            self.maxFramePayloadSize = maxFramePayloadSize

        if maxMessagePayloadSize is not None and maxMessagePayloadSize != self.maxMessagePayloadSize:
            self.maxMessagePayloadSize = maxMessagePayloadSize

        if autoFragmentSize is not None and autoFragmentSize != self.autoFragmentSize:
            self.autoFragmentSize = autoFragmentSize

        if failByDrop is not None and failByDrop != self.failByDrop:
            self.failByDrop = failByDrop

        if echoCloseCodeReason is not None and echoCloseCodeReason != self.echoCloseCodeReason:
            self.echoCloseCodeReason = echoCloseCodeReason

        if openHandshakeTimeout is not None and openHandshakeTimeout != self.openHandshakeTimeout:
            self.openHandshakeTimeout = openHandshakeTimeout

        if closeHandshakeTimeout is not None and closeHandshakeTimeout != self.closeHandshakeTimeout:
            self.closeHandshakeTimeout = closeHandshakeTimeout

        if tcpNoDelay is not None and tcpNoDelay != self.tcpNoDelay:
            self.tcpNoDelay = tcpNoDelay

        if perMessageCompressionAccept is not None and perMessageCompressionAccept != self.perMessageCompressionAccept:
            self.perMessageCompressionAccept = perMessageCompressionAccept

        if autoPingInterval is not None and autoPingInterval != self.autoPingInterval:
            self.autoPingInterval = autoPingInterval

        if autoPingTimeout is not None and autoPingTimeout != self.autoPingTimeout:
            self.autoPingTimeout = autoPingTimeout

        if autoPingSize is not None and autoPingSize != self.autoPingSize:
            assert(type(autoPingSize) == float or type(autoPingSize) in six.integer_types)
            assert(4 <= autoPingSize <= 125)
            self.autoPingSize = autoPingSize

        if serveFlashSocketPolicy is not None and serveFlashSocketPolicy != self.serveFlashSocketPolicy:
            self.serveFlashSocketPolicy = serveFlashSocketPolicy

        if flashSocketPolicy is not None and flashSocketPolicy != self.flashSocketPolicy:
            self.flashSocketPolicy = flashSocketPolicy

        if allowedOrigins is not None and allowedOrigins != self.allowedOrigins:
            self.allowedOrigins = allowedOrigins
            self.allowedOriginsPatterns = wildcards2patterns(self.allowedOrigins)

        if allowNullOrigin not in [True, False]:
            raise ValueError('allowNullOrigin must be a bool')
        self.allowNullOrigin = allowNullOrigin

        if maxConnections is not None and maxConnections != self.maxConnections:
            assert(type(maxConnections) in six.integer_types)
            assert(maxConnections >= 0)
            self.maxConnections = maxConnections

        if trustXForwardedFor is not None and trustXForwardedFor != self.trustXForwardedFor:
            assert(type(trustXForwardedFor) in six.integer_types)
            assert(trustXForwardedFor >= 0)
            self.trustXForwardedFor = trustXForwardedFor

    def getConnectionCount(self):
        """
        Get number of currently connected clients.

        :returns: int -- Number of currently connected clients.
        """
        return self.countConnections


class WebSocketClientProtocol(WebSocketProtocol):
    """
    Protocol base class for WebSocket clients.
    """

    log = txaio.make_logger()

    CONFIG_ATTRS = WebSocketProtocol.CONFIG_ATTRS_COMMON + WebSocketProtocol.CONFIG_ATTRS_CLIENT

    def onConnect(self, response):
        """
        Callback fired directly after WebSocket opening handshake when new WebSocket server
        connection was established.

        :param response: WebSocket connection response information.
        :type response: instance of :class:`autobahn.websocket.protocol.ConnectionResponse`
        """
        pass

    def _connectionMade(self):
        """
        Called by network framework when new transport connection to server was established. Default
        implementation will start the initial WebSocket opening handshake (or proxy connect).
        When overriding in derived class, make sure to call this base class
        implementation _before_ your code.
        """
        WebSocketProtocol._connectionMade(self)
        self.log.debug("connection to {peer} established", peer=self.peer)

        if not self.factory.isServer and self.factory.proxy is not None:
            # start by doing a HTTP/CONNECT for explicit proxies
            self.startProxyConnect()
        else:
            # immediately start with the WebSocket opening handshake
            self.startHandshake()

    def _connectionLost(self, reason):
        """
        Called by network framework when established transport connection to server was lost. Default
        implementation will tear down all state properly.
        When overriding in derived class, make sure to call this base class
        implementation _after_ your code.
        """
        WebSocketProtocol._connectionLost(self, reason)

    def startProxyConnect(self):
        """
        Connect to explicit proxy.
        """
        # construct proxy connect HTTP request
        #
        request = "CONNECT %s:%d HTTP/1.1\x0d\x0a" % (self.factory.host.encode("utf-8"), self.factory.port)
        request += "Host: %s:%d\x0d\x0a" % (self.factory.host.encode("utf-8"), self.factory.port)
        request += "\x0d\x0a"

        self.log.debug("{request}", request=request)

        self.sendData(request)

    def processProxyConnect(self):
        """
        Process HTTP/CONNECT response from server.
        """
        # only proceed when we have fully received the HTTP request line and all headers
        #
        end_of_header = self.data.find(b"\x0d\x0a\x0d\x0a")
        if end_of_header >= 0:

            http_response_data = self.data[:end_of_header + 4]
            self.log.debug(
                "received HTTP response:\n\n{response}\n\n",
                response=http_response_data,
            )

            # extract HTTP status line and headers
            #
            (http_status_line, http_headers, http_headers_cnt) = parseHttpHeader(http_response_data)

            # validate proxy connect response
            #
            self.log.debug(
                "received HTTP status line for proxy connect request : {status}",
                status=http_status_line,
            )
            self.log.debug(
                "received HTTP headers for proxy connect request : {headers}",
                headers=http_headers,
            )

            # Response Line
            #
            sl = http_status_line.split()
            if len(sl) < 2:
                return self.failProxyConnect("Bad HTTP response status line '%s'" % http_status_line)

            # HTTP version
            #
            http_version = sl[0].strip()
            if http_version not in ("HTTP/1.1", "HTTP/1.0"):
                return self.failProxyConnect("Unsupported HTTP version ('%s')" % http_version)

            # HTTP status code
            #
            try:
                status_code = int(sl[1].strip())
            except ValueError:
                return self.failProxyConnect("Bad HTTP status code ('%s')" % sl[1].strip())

            if not (200 <= status_code < 300):

                # FIXME: handle redirects
                # FIXME: handle authentication required

                if len(sl) > 2:
                    reason = " - %s" % ''.join(sl[2:])
                else:
                    reason = ""
                return self.failProxyConnect("HTTP proxy connect failed (%d%s)" % (status_code, reason))

            # Ok, got complete response for HTTP/CONNECT, remember rest (if any)
            #
            self.data = self.data[end_of_header + 4:]

            # opening handshake completed, move WebSocket connection into OPEN state
            #
            self.state = WebSocketProtocol.STATE_CONNECTING

            # process rest of buffered data, if any
            #
            if len(self.data) > 0:
                self.consumeData()

            # now start WebSocket opening handshake
            #
            if self.factory.isSecure:
                self.startTLS()
            self.startHandshake()

    def failProxyConnect(self, reason):
        """
        During initial explicit proxy connect, the server response indicates some failure and we drop the
        connection.
        """
        self.log.debug("failing proxy connect ('{reason}')", reason=reason)
        self.dropConnection(abort=True)

    def startHandshake(self):
        """
        Start WebSocket opening handshake.
        """

        # construct WS opening handshake HTTP header
        #
        request = "GET %s HTTP/1.1\x0d\x0a" % self.factory.resource

        if self.factory.useragent is not None and self.factory.useragent != "":
            request += "User-Agent: %s\x0d\x0a" % self.factory.useragent

        request += "Host: %s:%d\x0d\x0a" % (self.factory.host, self.factory.port)
        request += "Upgrade: WebSocket\x0d\x0a"
        request += "Connection: Upgrade\x0d\x0a"

        # this seems to prohibit some non-compliant proxies from removing the
        # connection "Upgrade" header
        # See also:
        # http://www.ietf.org/mail-archive/web/hybi/current/msg09841.html
        # http://code.google.com/p/chromium/issues/detail?id=148908
        #
        request += "Pragma: no-cache\x0d\x0a"
        request += "Cache-Control: no-cache\x0d\x0a"

        # optional, user supplied additional HTTP headers
        #
        for uh in self.factory.headers.items():
            request += "%s: %s\x0d\x0a" % (uh[0], uh[1])

        # handshake random key
        #
        self.websocket_key = base64.b64encode(os.urandom(16))
        request += "Sec-WebSocket-Key: %s\x0d\x0a" % self.websocket_key.decode()

        # optional origin announced
        #
        if self.factory.origin:
            if self.version > 10:
                request += "Origin: %s\x0d\x0a" % self.factory.origin
            else:
                request += "Sec-WebSocket-Origin: %s\x0d\x0a" % self.factory.origin

        # optional list of WS subprotocols announced
        #
        if len(self.factory.protocols) > 0:
            request += "Sec-WebSocket-Protocol: %s\x0d\x0a" % ','.join(self.factory.protocols)

        # extensions
        #
        extensions = []

        # permessage-compress offers
        #
        for offer in self.perMessageCompressionOffers:
            extensions.append(offer.get_extension_string())

        if len(extensions) > 0:
            request += "Sec-WebSocket-Extensions: %s\x0d\x0a" % ', '.join(extensions)

        # set WS protocol version
        #
        request += "Sec-WebSocket-Version: %d\x0d\x0a" % WebSocketProtocol.SPEC_TO_PROTOCOL_VERSION[self.version]

        request += "\x0d\x0a"

        self.http_request_data = request.encode('utf8')
        self.sendData(self.http_request_data)

        self.log.debug("{request}", request=request)

    def processHandshake(self):
        """
        Process WebSocket opening handshake response from server.
        """
        # only proceed when we have fully received the HTTP request line and all headers
        #
        end_of_header = self.data.find(b"\x0d\x0a\x0d\x0a")
        if end_of_header >= 0:

            self.http_response_data = self.data[:end_of_header + 4]
            self.log.debug(
                "received HTTP response:\n\n{response}\n\n",
                response=self.http_response_data,
            )

            # extract HTTP status line and headers
            #
            (self.http_status_line, self.http_headers, http_headers_cnt) = parseHttpHeader(self.http_response_data)

            # validate WebSocket opening handshake server response
            #
            self.log.debug(
                "received HTTP status line in opening handshake : {status}",
                status=self.http_status_line,
            )
            self.log.debug(
                "received HTTP headers in opening handshake : {headers}",
                headers=self.http_headers,
            )

            # Response Line
            #
            sl = self.http_status_line.split()
            if len(sl) < 2:
                return self.failHandshake("Bad HTTP response status line '%s'" % self.http_status_line)

            # HTTP version
            #
            http_version = sl[0].strip()
            if http_version != "HTTP/1.1":
                return self.failHandshake("Unsupported HTTP version ('%s')" % http_version)

            # HTTP status code
            #
            try:
                status_code = int(sl[1].strip())
            except ValueError:
                return self.failHandshake("Bad HTTP status code ('%s')" % sl[1].strip())
            if status_code != 101:  # Switching Protocols

                # FIXME: handle redirects
                # FIXME: handle authentication required

                if len(sl) > 2:
                    reason = " - %s" % ''.join(sl[2:])
                else:
                    reason = ""
                return self.failHandshake("WebSocket connection upgrade failed (%d%s)" % (status_code, reason))

            # Upgrade
            #
            if 'upgrade' not in self.http_headers:
                return self.failHandshake("HTTP Upgrade header missing")
            if self.http_headers["upgrade"].strip().lower() != "websocket":
                return self.failHandshake("HTTP Upgrade header different from 'websocket' (case-insensitive) : %s" % self.http_headers["upgrade"])

            # Connection
            #
            if 'connection' not in self.http_headers:
                return self.failHandshake("HTTP Connection header missing")
            connectionUpgrade = False
            for c in self.http_headers["connection"].split(","):
                if c.strip().lower() == "upgrade":
                    connectionUpgrade = True
                    break
            if not connectionUpgrade:
                return self.failHandshake("HTTP Connection header does not include 'upgrade' value (case-insensitive) : %s" % self.http_headers["connection"])

            # compute Sec-WebSocket-Accept
            #
            if 'sec-websocket-accept' not in self.http_headers:
                return self.failHandshake("HTTP Sec-WebSocket-Accept header missing in opening handshake reply")
            else:
                if http_headers_cnt["sec-websocket-accept"] > 1:
                    return self.failHandshake("HTTP Sec-WebSocket-Accept header appears more than once in opening handshake reply")
                sec_websocket_accept_got = self.http_headers["sec-websocket-accept"].strip()

                sha1 = hashlib.sha1()
                sha1.update(self.websocket_key + WebSocketProtocol._WS_MAGIC)
                sec_websocket_accept = base64.b64encode(sha1.digest()).decode()

                if sec_websocket_accept_got != sec_websocket_accept:
                    return self.failHandshake("HTTP Sec-WebSocket-Accept bogus value : expected %s / got %s" % (sec_websocket_accept, sec_websocket_accept_got))

            # Sec-WebSocket-Extensions
            #

            # extensions effectively in use for this connection
            #
            self.websocket_extensions_in_use = []

            if 'sec-websocket-extensions' in self.http_headers:

                if http_headers_cnt["sec-websocket-extensions"] > 1:
                    return self.failHandshake("HTTP Sec-WebSocket-Extensions header appears more than once in opening handshake reply")
                else:
                    # extensions select by server
                    #
                    websocket_extensions = self._parseExtensionsHeader(self.http_headers["sec-websocket-extensions"])

                # process extensions selected by server
                #
                for (extension, params) in websocket_extensions:

                    self.log.debug(
                        "parsed WebSocket extension '{extension}' with params '{params}'",
                        extension=extension,
                        params=params,
                    )

                    # process permessage-compress extension
                    #
                    if extension in PERMESSAGE_COMPRESSION_EXTENSION:

                        # check that server only responded with 1 configuration ("PMCE")
                        #
                        if self._perMessageCompress is not None:
                            return self.failHandshake("multiple occurrence of a permessage-compress extension")

                        PMCE = PERMESSAGE_COMPRESSION_EXTENSION[extension]

                        try:
                            pmceResponse = PMCE['Response'].parse(params)
                        except Exception as e:
                            return self.failHandshake(str(e))

                        accept = self.perMessageCompressionAccept(pmceResponse)

                        if accept is None:
                            return self.failHandshake("WebSocket permessage-compress extension response from server denied by client")

                        self._perMessageCompress = PMCE['PMCE'].create_from_response_accept(self.factory.isServer, accept)

                        self.websocket_extensions_in_use.append(self._perMessageCompress)

                    else:
                        return self.failHandshake("server wants to use extension '%s' we did not request, haven't implemented or did not enable" % extension)

            # handle "subprotocol in use" - if any
            #
            self.websocket_protocol_in_use = None
            if 'sec-websocket-protocol' in self.http_headers:
                if http_headers_cnt["sec-websocket-protocol"] > 1:
                    return self.failHandshake("HTTP Sec-WebSocket-Protocol header appears more than once in opening handshake reply")
                sp = str(self.http_headers["sec-websocket-protocol"].strip())
                if sp != "":
                    if sp not in self.factory.protocols:
                        return self.failHandshake("subprotocol selected by server (%s) not in subprotocol list requested by client (%s)" % (sp, str(self.factory.protocols)))
                    else:
                        # ok, subprotocol in use
                        #
                        self.websocket_protocol_in_use = sp

            # Ok, got complete HS input, remember rest (if any)
            #
            self.data = self.data[end_of_header + 4:]

            # opening handshake completed, move WebSocket connection into OPEN state
            #
            self.state = WebSocketProtocol.STATE_OPEN

            # cancel any opening HS timer if present
            #
            if self.openHandshakeTimeoutCall is not None:
                self.log.debug("openHandshakeTimeoutCall.cancel")
                self.openHandshakeTimeoutCall.cancel()
                self.openHandshakeTimeoutCall = None

            # init state
            #
            self.inside_message = False
            self.current_frame = None
            self.websocket_version = self.version

            # automatic ping/pong
            #
            if self.autoPingInterval:
                self.autoPingPendingCall = self.factory._batched_timer.call_later(
                    self.autoPingInterval,
                    self._sendAutoPing,
                )

            # we handle this symmetrical to server-side .. that is, give the
            # client a chance to bail out .. i.e. on no subprotocol selected
            # by server
            try:
                response = ConnectionResponse(self.peer,
                                              self.http_headers,
                                              self.websocket_version,
                                              self.websocket_protocol_in_use,
                                              self.websocket_extensions_in_use)

                self._onConnect(response)

            except Exception as e:
                # immediately close the WS connection
                #
                self._fail_connection(1000, u'{}'.format(e))
            else:
                # fire handler on derived class
                #
                if self.trackedTimings:
                    self.trackedTimings.track("onOpen")
                self._onOpen()

            # process rest, if any
            #
            if len(self.data) > 0:
                self.consumeData()

    def failHandshake(self, reason):
        """
        During opening handshake the server response is invalid and we drop the
        connection.
        """
        self.wasNotCleanReason = reason
        self.log.info(
            "failing WebSocket opening handshake ('{reason}')",
            reason=reason,
        )
        self.dropConnection(abort=True)


class WebSocketClientFactory(WebSocketFactory):
    """
    A protocol factory for WebSocket clients.

    Implements :func:`autobahn.websocket.interfaces.IWebSocketClientChannelFactory`
    """

    log = txaio.make_logger()

    protocol = WebSocketClientProtocol
    """
    The protocol to be spoken. Must be derived from :class:`autobahn.websocket.protocol.WebSocketClientProtocol`.
    """

    isServer = False
    """
    Flag indicating if this factory is client- or server-side.
    """

    def __init__(self,
                 url=None,
                 origin=None,
                 protocols=None,
                 useragent="AutobahnPython/%s" % __version__,
                 headers=None,
                 proxy=None):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketClientChannelFactory.__init__`
        """
        self.logOctets = False
        self.logFrames = False
        self.trackTimings = False

        # batch up and chunk timers ("call_later")
        self._batched_timer = txaio.make_batched_timer(
            bucket_seconds=0.200,
            chunk_size=1000,
        )

        # seed RNG which is used for WS opening handshake key and WS frame masks generation
        random.seed()

        # default WS session parameters
        #
        self.setSessionParameters(url, origin, protocols, useragent, headers, proxy)

        # default WebSocket protocol options
        #
        self.resetProtocolOptions()

    def setSessionParameters(self,
                             url=None,
                             origin=None,
                             protocols=None,
                             useragent=None,
                             headers=None,
                             proxy=None):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketClientChannelFactory.setSessionParameters`
        """
        # parse WebSocket URI into components
        (isSecure, host, port, resource, path, params) = parse_url(url or "ws://localhost")
        self.url = url
        self.isSecure = isSecure
        self.host = host
        self.port = port
        self.resource = resource
        self.path = path
        self.params = params

        self.origin = origin
        self.protocols = protocols or []
        self.useragent = useragent
        self.headers = headers or {}

        self.proxy = proxy

    def resetProtocolOptions(self):
        """
        Implements :func:`autobahn.websocket.interfaces.IWebSocketClientChannelFactory.resetProtocolOptions`
        """
        self.version = WebSocketProtocol.DEFAULT_SPEC_VERSION
        self.utf8validateIncoming = True
        self.acceptMaskedServerFrames = False
        self.maskClientFrames = True
        self.applyMask = True
        self.maxFramePayloadSize = 0
        self.maxMessagePayloadSize = 0
        self.autoFragmentSize = 0
        self.failByDrop = True
        self.echoCloseCodeReason = False
        self.serverConnectionDropTimeout = 1
        self.openHandshakeTimeout = 5
        self.closeHandshakeTimeout = 1
        self.tcpNoDelay = True

        # permessage-XXX extensions
        #
        self.perMessageCompressionOffers = []
        self.perMessageCompressionAccept = lambda _: None

        # automatic ping/pong ("heartbeating")
        #
        self.autoPingInterval = 0
        self.autoPingTimeout = 0
        self.autoPingSize = 4

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
        Implements :func:`autobahn.websocket.interfaces.IWebSocketClientChannelFactory.setProtocolOptions`
        """
        if version is not None:
            if version not in WebSocketProtocol.SUPPORTED_SPEC_VERSIONS:
                raise Exception("invalid WebSocket draft version %s (allowed values: %s)" % (version, str(WebSocketProtocol.SUPPORTED_SPEC_VERSIONS)))
            if version != self.version:
                self.version = version

        if utf8validateIncoming is not None and utf8validateIncoming != self.utf8validateIncoming:
            self.utf8validateIncoming = utf8validateIncoming

        if acceptMaskedServerFrames is not None and acceptMaskedServerFrames != self.acceptMaskedServerFrames:
            self.acceptMaskedServerFrames = acceptMaskedServerFrames

        if maskClientFrames is not None and maskClientFrames != self.maskClientFrames:
            self.maskClientFrames = maskClientFrames

        if applyMask is not None and applyMask != self.applyMask:
            self.applyMask = applyMask

        if maxFramePayloadSize is not None and maxFramePayloadSize != self.maxFramePayloadSize:
            self.maxFramePayloadSize = maxFramePayloadSize

        if maxMessagePayloadSize is not None and maxMessagePayloadSize != self.maxMessagePayloadSize:
            self.maxMessagePayloadSize = maxMessagePayloadSize

        if autoFragmentSize is not None and autoFragmentSize != self.autoFragmentSize:
            self.autoFragmentSize = autoFragmentSize

        if failByDrop is not None and failByDrop != self.failByDrop:
            self.failByDrop = failByDrop

        if echoCloseCodeReason is not None and echoCloseCodeReason != self.echoCloseCodeReason:
            self.echoCloseCodeReason = echoCloseCodeReason

        if serverConnectionDropTimeout is not None and serverConnectionDropTimeout != self.serverConnectionDropTimeout:
            self.serverConnectionDropTimeout = serverConnectionDropTimeout

        if openHandshakeTimeout is not None and openHandshakeTimeout != self.openHandshakeTimeout:
            self.openHandshakeTimeout = openHandshakeTimeout

        if closeHandshakeTimeout is not None and closeHandshakeTimeout != self.closeHandshakeTimeout:
            self.closeHandshakeTimeout = closeHandshakeTimeout

        if tcpNoDelay is not None and tcpNoDelay != self.tcpNoDelay:
            self.tcpNoDelay = tcpNoDelay

        if perMessageCompressionOffers is not None and pickle.dumps(perMessageCompressionOffers) != pickle.dumps(self.perMessageCompressionOffers):
            if type(perMessageCompressionOffers) == list:
                #
                # FIXME: more rigorous verification of passed argument
                #
                self.perMessageCompressionOffers = copy.deepcopy(perMessageCompressionOffers)
            else:
                raise Exception("invalid type %s for perMessageCompressionOffers - expected list" % type(perMessageCompressionOffers))

        if perMessageCompressionAccept is not None and perMessageCompressionAccept != self.perMessageCompressionAccept:
            self.perMessageCompressionAccept = perMessageCompressionAccept

        if autoPingInterval is not None and autoPingInterval != self.autoPingInterval:
            self.autoPingInterval = autoPingInterval

        if autoPingTimeout is not None and autoPingTimeout != self.autoPingTimeout:
            self.autoPingTimeout = autoPingTimeout

        if autoPingSize is not None and autoPingSize != self.autoPingSize:
            assert(type(autoPingSize) == float or type(autoPingSize) in six.integer_types)
            assert(4 <= autoPingSize <= 125)
            self.autoPingSize = autoPingSize
