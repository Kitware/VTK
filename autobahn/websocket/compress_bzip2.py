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

import bz2

from autobahn.websocket.compress_base import PerMessageCompressOffer, \
    PerMessageCompressOfferAccept, \
    PerMessageCompressResponse, \
    PerMessageCompressResponseAccept, \
    PerMessageCompress

__all__ = (
    'PerMessageBzip2Mixin',
    'PerMessageBzip2Offer',
    'PerMessageBzip2OfferAccept',
    'PerMessageBzip2Response',
    'PerMessageBzip2ResponseAccept',
    'PerMessageBzip2',
)


class PerMessageBzip2Mixin(object):
    """
    Mixin class for this extension.
    """

    EXTENSION_NAME = "permessage-bzip2"
    """
    Name of this WebSocket extension.
    """

    COMPRESS_LEVEL_PERMISSIBLE_VALUES = [1, 2, 3, 4, 5, 6, 7, 8, 9]
    """
    Permissible value for compression level parameter.
    """


class PerMessageBzip2Offer(PerMessageCompressOffer, PerMessageBzip2Mixin):
    """
    Set of extension parameters for `permessage-bzip2` WebSocket extension
    offered by a client to a server.
    """

    @classmethod
    def parse(cls, params):
        """
        Parses a WebSocket extension offer for `permessage-bzip2` provided by a client to a server.

        :param params: Output from :func:`autobahn.websocket.WebSocketProtocol._parseExtensionsHeader`.
        :type params: list

        :returns: object -- A new instance of :class:`autobahn.compress.PerMessageBzip2Offer`.
        """
        # extension parameter defaults
        accept_max_compress_level = False
        request_max_compress_level = 0

        # verify/parse client ("client-to-server direction") parameters of permessage-bzip2 offer
        for p in params:

            if len(params[p]) > 1:
                raise Exception("multiple occurrence of extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

            val = params[p][0]

            if p == 'client_max_compress_level':
                # noinspection PySimplifyBooleanCheck
                if val is not True:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    accept_max_compress_level = True

            elif p == 'server_max_compress_level':
                try:
                    val = int(val)
                except:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                if val not in PerMessageBzip2Mixin.COMPRESS_LEVEL_PERMISSIBLE_VALUES:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    request_max_compress_level = val

            else:
                raise Exception("illegal extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

        offer = cls(accept_max_compress_level,
                    request_max_compress_level)
        return offer

    def __init__(self,
                 accept_max_compress_level=True,
                 request_max_compress_level=0):
        """
        Constructor.

        :param accept_max_compress_level: Iff true, client accepts "maximum compression level" parameter.
        :type accept_max_compress_level: bool
        :param request_max_compress_level: Iff non-zero, client requests given "maximum compression level" - must be 1-9.
        :type request_max_compress_level: int
        """
        if type(accept_max_compress_level) != bool:
            raise Exception("invalid type %s for accept_max_compress_level" % type(accept_max_compress_level))

        self.accept_max_compress_level = accept_max_compress_level

        if request_max_compress_level != 0 and request_max_compress_level not in self.COMPRESS_LEVEL_PERMISSIBLE_VALUES:
            raise Exception("invalid value %s for request_max_compress_level - permissible values %s" % (request_max_compress_level, self.COMPRESS_LEVEL_PERMISSIBLE_VALUES))

        self.request_max_compress_level = request_max_compress_level

    def get_extension_string(self):
        """
        Returns the WebSocket extension configuration string as sent to the server.

        :returns: PMCE configuration string.
        :rtype: str
        """
        pmce_string = self.EXTENSION_NAME
        if self.accept_max_compress_level:
            pmce_string += "; client_max_compress_level"
        if self.request_max_compress_level != 0:
            pmce_string += "; server_max_compress_level=%d" % self.request_max_compress_level
        return pmce_string

    def __json__(self):
        """
        Returns a JSON serializable object representation.

        :returns: JSON serializable representation.
        :rtype: dict
        """
        return {'extension': self.EXTENSION_NAME,
                'accept_max_compress_level': self.accept_max_compress_level,
                'request_max_compress_level': self.request_max_compress_level}

    def __repr__(self):
        """
        Returns Python object representation that can be eval'ed to reconstruct the object.

        :returns: Python string representation.
        :rtype: str
        """
        return "PerMessageBzip2Offer(accept_max_compress_level = %s, request_max_compress_level = %s)" % (self.accept_max_compress_level, self.request_max_compress_level)


class PerMessageBzip2OfferAccept(PerMessageCompressOfferAccept, PerMessageBzip2Mixin):
    """
    Set of parameters with which to accept an `permessage-bzip2` offer
    from a client by a server.
    """

    def __init__(self,
                 offer,
                 request_max_compress_level=0,
                 compress_level=None):
        """
        Constructor.

        :param offer: The offer being accepted.
        :type offer: Instance of :class:`autobahn.compress.PerMessageBzip2Offer`.
        :param request_max_compress_level: Iff non-zero, server requests given "maximum compression level" - must be 1-9.
        :type request_max_compress_level: int
        :param compress_level: Override server ("server-to-client direction") compress level (this must be compatible with offer).
        :type compress_level: int
        """
        if not isinstance(offer, PerMessageBzip2Offer):
            raise Exception("invalid type %s for offer" % type(offer))

        self.offer = offer

        if request_max_compress_level != 0 and request_max_compress_level not in self.COMPRESS_LEVEL_PERMISSIBLE_VALUES:
            raise Exception("invalid value %s for request_max_compress_level - permissible values %s" % (request_max_compress_level, self.COMPRESS_LEVEL_PERMISSIBLE_VALUES))

        if request_max_compress_level != 0 and not offer.accept_max_compress_level:
            raise Exception("invalid value %s for request_max_compress_level - feature unsupported by client" % request_max_compress_level)

        self.request_max_compress_level = request_max_compress_level

        if compress_level is not None:
            if compress_level not in self.COMPRESS_LEVEL_PERMISSIBLE_VALUES:
                raise Exception("invalid value %s for compress_level - permissible values %s" % (compress_level, self.COMPRESS_LEVEL_PERMISSIBLE_VALUES))

            if offer.request_max_compress_level != 0 and compress_level > offer.request_max_compress_level:
                raise Exception("invalid value %s for compress_level - client requested lower maximum value" % compress_level)

        self.compress_level = compress_level

    def get_extension_string(self):
        """
        Returns the WebSocket extension configuration string as sent to the server.

        :returns: PMCE configuration string.
        :rtype: str
        """
        pmce_string = self.EXTENSION_NAME
        if self.offer.request_max_compress_level != 0:
            pmce_string += "; server_max_compress_level=%d" % self.offer.request_max_compress_level
        if self.request_max_compress_level != 0:
            pmce_string += "; client_max_compress_level=%d" % self.request_max_compress_level
        return pmce_string

    def __json__(self):
        """
        Returns a JSON serializable object representation.

        :returns: JSON serializable representation.
        :rtype: dict
        """
        return {'extension': self.EXTENSION_NAME,
                'offer': self.offer.__json__(),
                'request_max_compress_level': self.request_max_compress_level,
                'compress_level': self.compress_level}

    def __repr__(self):
        """
        Returns Python object representation that can be eval'ed to reconstruct the object.

        :returns: Python string representation.
        :rtype: str
        """
        return "PerMessageBzip2Accept(offer = %s, request_max_compress_level = %s, compress_level = %s)" % (self.offer.__repr__(), self.request_max_compress_level, self.compress_level)


class PerMessageBzip2Response(PerMessageCompressResponse, PerMessageBzip2Mixin):
    """
    Set of parameters for `permessage-bzip2` responded by server.
    """

    @classmethod
    def parse(cls, params):
        """
        Parses a WebSocket extension response for `permessage-bzip2` provided by a server to a client.

        :param params: Output from :func:`autobahn.websocket.WebSocketProtocol._parseExtensionsHeader`.
        :type params: list

        :returns: A new instance of :class:`autobahn.compress.PerMessageBzip2Response`.
        :rtype: obj
        """
        client_max_compress_level = 0
        server_max_compress_level = 0

        for p in params:

            if len(params[p]) > 1:
                raise Exception("multiple occurrence of extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

            val = params[p][0]

            if p == 'client_max_compress_level':
                try:
                    val = int(val)
                except:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                if val not in PerMessageBzip2Mixin.COMPRESS_LEVEL_PERMISSIBLE_VALUES:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    client_max_compress_level = val

            elif p == 'server_max_compress_level':
                try:
                    val = int(val)
                except:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                if val not in PerMessageBzip2Mixin.COMPRESS_LEVEL_PERMISSIBLE_VALUES:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    server_max_compress_level = val

            else:
                raise Exception("illegal extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

        response = cls(client_max_compress_level,
                       server_max_compress_level)
        return response

    def __init__(self,
                 client_max_compress_level,
                 server_max_compress_level):
        self.client_max_compress_level = client_max_compress_level
        self.server_max_compress_level = server_max_compress_level

    def __json__(self):
        """
        Returns a JSON serializable object representation.

        :returns: JSON serializable representation.
        :rtype: dict
        """
        return {'extension': self.EXTENSION_NAME,
                'client_max_compress_level': self.client_max_compress_level,
                'server_max_compress_level': self.server_max_compress_level}

    def __repr__(self):
        """
        Returns Python object representation that can be eval'ed to reconstruct the object.

        :returns: Python string representation.
        :rtype: str
        """
        return "PerMessageBzip2Response(client_max_compress_level = %s, server_max_compress_level = %s)" % (self.client_max_compress_level, self.server_max_compress_level)


class PerMessageBzip2ResponseAccept(PerMessageCompressResponseAccept, PerMessageBzip2Mixin):
    """
    Set of parameters with which to accept an `permessage-bzip2` response
    from a server by a client.
    """

    def __init__(self,
                 response,
                 compress_level=None):
        """

        :param response: The response being accepted.
        :type response: Instance of :class:`autobahn.compress.PerMessageBzip2Response`.
        :param compress_level: Override client ("client-to-server direction") compress level (this must be compatible with response).
        :type compress_level: int
        """
        if not isinstance(response, PerMessageBzip2Response):
            raise Exception("invalid type %s for response" % type(response))

        self.response = response

        if compress_level is not None:
            if compress_level not in self.COMPRESS_LEVEL_PERMISSIBLE_VALUES:
                raise Exception("invalid value %s for compress_level - permissible values %s" % (compress_level, self.COMPRESS_LEVEL_PERMISSIBLE_VALUES))

            if response.client_max_compress_level != 0 and compress_level > response.client_max_compress_level:
                raise Exception("invalid value %s for compress_level - server requested lower maximum value" % compress_level)

        self.compress_level = compress_level

    def __json__(self):
        """
        Returns a JSON serializable object representation.

        :returns: JSON serializable representation.
        :rtype: dict
        """
        return {'extension': self.EXTENSION_NAME,
                'response': self.response.__json__(),
                'compress_level': self.compress_level}

    def __repr__(self):
        """
        Returns Python object representation that can be eval'ed to reconstruct the object.

        :returns: Python string representation.
        :rtype: str
        """
        return "PerMessageBzip2ResponseAccept(response = %s, compress_level = %s)" % (self.response.__repr__(), self.compress_level)


class PerMessageBzip2(PerMessageCompress, PerMessageBzip2Mixin):
    """
    `permessage-bzip2` WebSocket extension processor.
    """
    DEFAULT_COMPRESS_LEVEL = 9

    @classmethod
    def create_from_response_accept(cls, is_server, accept):
        pmce = cls(is_server,
                   accept.response.server_max_compress_level,
                   accept.compress_level if accept.compress_level is not None else accept.response.client_max_compress_level)
        return pmce

    @classmethod
    def create_from_offer_accept(cls, is_server, accept):
        pmce = cls(is_server,
                   accept.compress_level if accept.compress_level is not None else accept.offer.request_max_compress_level,
                   accept.request_max_compress_level)
        return pmce

    def __init__(self,
                 is_server,
                 server_max_compress_level,
                 client_max_compress_level):
        self._isServer = is_server
        self._compressor = None
        self._decompressor = None

        self.server_max_compress_level = server_max_compress_level if server_max_compress_level != 0 else self.DEFAULT_COMPRESS_LEVEL
        self.client_max_compress_level = client_max_compress_level if client_max_compress_level != 0 else self.DEFAULT_COMPRESS_LEVEL

    def __json__(self):
        return {'extension': self.EXTENSION_NAME,
                'isServer': self._isServer,
                'server_max_compress_level': self.server_max_compress_level,
                'client_max_compress_level': self.client_max_compress_level}

    def __repr__(self):
        return "PerMessageBzip2(isServer = %s, server_max_compress_level = %s, client_max_compress_level = %s)" % (self._isServer, self.server_max_compress_level, self.client_max_compress_level)

    def start_compress_message(self):
        if self._isServer:
            if self._compressor is None:
                self._compressor = bz2.BZ2Compressor(self.server_max_compress_level)
        else:
            if self._compressor is None:
                self._compressor = bz2.BZ2Compressor(self.client_max_compress_level)

    def compress_message_data(self, data):
        return self._compressor.compress(data)

    def end_compress_message(self):
        data = self._compressor.flush()

        # there seems to be no "flush without close stream", and after
        # full flush, compressor must not be reused
        self._compressor = None

        return data

    def start_decompress_message(self):
        if self._decompressor is None:
            self._decompressor = bz2.BZ2Decompressor()

    def decompress_message_data(self, data):
        return self._decompressor.decompress(data)

    def end_decompress_message(self):
        self._decompressor = None
