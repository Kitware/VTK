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

import zlib

from autobahn.util import public
from autobahn.websocket.compress_base import PerMessageCompressOffer, \
    PerMessageCompressOfferAccept, \
    PerMessageCompressResponse, \
    PerMessageCompressResponseAccept, \
    PerMessageCompress

__all__ = (
    'PerMessageDeflateMixin',
    'PerMessageDeflateOffer',
    'PerMessageDeflateOfferAccept',
    'PerMessageDeflateResponse',
    'PerMessageDeflateResponseAccept',
    'PerMessageDeflate',
)


class PerMessageDeflateMixin(object):
    """
    Mixin class for this extension.
    """

    EXTENSION_NAME = "permessage-deflate"
    """
    Name of this WebSocket extension.
    """

    WINDOW_SIZE_PERMISSIBLE_VALUES = [8, 9, 10, 11, 12, 13, 14, 15]
    """
    Permissible value for window size parameter.
    Higher values use more memory, but produce smaller output. The default is 15.
    """

    MEM_LEVEL_PERMISSIBLE_VALUES = [1, 2, 3, 4, 5, 6, 7, 8, 9]
    """
    Permissible value for memory level parameter.
    Higher values use more memory, but are faster and produce smaller output. The default is 8.
    """


@public
class PerMessageDeflateOffer(PerMessageCompressOffer, PerMessageDeflateMixin):
    """
    Set of extension parameters for `permessage-deflate` WebSocket extension
    offered by a client to a server.
    """

    @classmethod
    def parse(cls, params):
        """
        Parses a WebSocket extension offer for `permessage-deflate` provided by a client to a server.

        :param params: Output from :func:`autobahn.websocket.WebSocketProtocol._parseExtensionsHeader`.
        :type params: list

        :returns: A new instance of :class:`autobahn.compress.PerMessageDeflateOffer`.
        :rtype: obj
        """

        # extension parameter defaults
        accept_max_window_bits = False
        accept_no_context_takeover = True
        # accept_no_context_takeover = False # FIXME: this may change in draft
        request_max_window_bits = 0
        request_no_context_takeover = False

        # verify/parse client ("client-to-server direction") parameters of permessage-deflate offer
        for p in params:

            if len(params[p]) > 1:
                raise Exception("multiple occurrence of extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

            val = params[p][0]

            if p == 'client_max_window_bits':
                #
                # see: https://tools.ietf.org/html/draft-ietf-hybi-permessage-compression-18
                # 8.1.2.2. client_max_window_bits

                # ".. This parameter has no value or a decimal integer value without
                # leading zeroes between 8 to 15 inclusive ..""

                # noinspection PySimplifyBooleanCheck
                if val is not True:
                    try:
                        val = int(val)
                    except:
                        raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                    else:
                        if val not in PerMessageDeflateMixin.WINDOW_SIZE_PERMISSIBLE_VALUES:
                            raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                        else:
                            # FIXME (maybe): possibly forward/process the client hint!
                            # accept_max_window_bits = val
                            accept_max_window_bits = True
                else:
                    accept_max_window_bits = True

            elif p == 'client_no_context_takeover':
                # noinspection PySimplifyBooleanCheck
                if val is not True:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    accept_no_context_takeover = True

            elif p == 'server_max_window_bits':
                try:
                    val = int(val)
                except:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    if val not in PerMessageDeflateMixin.WINDOW_SIZE_PERMISSIBLE_VALUES:
                        raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                    else:
                        request_max_window_bits = val

            elif p == 'server_no_context_takeover':
                # noinspection PySimplifyBooleanCheck
                if val is not True:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    request_no_context_takeover = True

            else:
                raise Exception("illegal extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

        offer = cls(accept_no_context_takeover,
                    accept_max_window_bits,
                    request_no_context_takeover,
                    request_max_window_bits)
        return offer

    def __init__(self,
                 accept_no_context_takeover=True,
                 accept_max_window_bits=True,
                 request_no_context_takeover=False,
                 request_max_window_bits=0):
        """

        :param accept_no_context_takeover: When ``True``, the client accepts the "no context takeover" feature.
        :type accept_no_context_takeover: bool
        :param accept_max_window_bits: When ``True``, the client accepts setting "max window size".
        :type accept_max_window_bits: bool
        :param request_no_context_takeover: When ``True``, the client request the "no context takeover" feature.
        :type request_no_context_takeover: bool
        :param request_max_window_bits: When non-zero, the client requests the given "max window size" (must be
            and integer from the interval ``[8..15]``).
        :type request_max_window_bits: int
        """
        if type(accept_no_context_takeover) != bool:
            raise Exception("invalid type %s for accept_no_context_takeover" % type(accept_no_context_takeover))

        self.accept_no_context_takeover = accept_no_context_takeover

        if type(accept_max_window_bits) != bool:
            raise Exception("invalid type %s for accept_max_window_bits" % type(accept_max_window_bits))

        self.accept_max_window_bits = accept_max_window_bits

        if type(request_no_context_takeover) != bool:
            raise Exception("invalid type %s for request_no_context_takeover" % type(request_no_context_takeover))

        self.request_no_context_takeover = request_no_context_takeover

        if request_max_window_bits != 0 and request_max_window_bits not in self.WINDOW_SIZE_PERMISSIBLE_VALUES:
            raise Exception("invalid value %s for request_max_window_bits - permissible values %s" % (request_max_window_bits, self.WINDOW_SIZE_PERMISSIBLE_VALUES))

        self.request_max_window_bits = request_max_window_bits

    def get_extension_string(self):
        """
        Returns the WebSocket extension configuration string as sent to the server.

        :returns: PMCE configuration string.
        :rtype: str
        """
        pmce_string = self.EXTENSION_NAME
        if self.accept_no_context_takeover:
            pmce_string += "; client_no_context_takeover"
        if self.accept_max_window_bits:
            pmce_string += "; client_max_window_bits"
        if self.request_no_context_takeover:
            pmce_string += "; server_no_context_takeover"
        if self.request_max_window_bits != 0:
            pmce_string += "; server_max_window_bits=%d" % self.request_max_window_bits
        return pmce_string

    def __json__(self):
        """
        Returns a JSON serializable object representation.

        :returns: JSON serializable representation.
        :rtype: dict
        """
        return {'extension': self.EXTENSION_NAME,
                'accept_no_context_takeover': self.accept_no_context_takeover,
                'accept_max_window_bits': self.accept_max_window_bits,
                'request_no_context_takeover': self.request_no_context_takeover,
                'request_max_window_bits': self.request_max_window_bits}

    def __repr__(self):
        """
        Returns Python object representation that can be eval'ed to reconstruct the object.

        :returns: Python string representation.
        :rtype: str
        """
        return "PerMessageDeflateOffer(accept_no_context_takeover = %s, accept_max_window_bits = %s, request_no_context_takeover = %s, request_max_window_bits = %s)" % (self.accept_no_context_takeover, self.accept_max_window_bits, self.request_no_context_takeover, self.request_max_window_bits)


@public
class PerMessageDeflateOfferAccept(PerMessageCompressOfferAccept, PerMessageDeflateMixin):
    """
    Set of parameters with which to accept an `permessage-deflate` offer
    from a client by a server.
    """

    def __init__(self,
                 offer,
                 request_no_context_takeover=False,
                 request_max_window_bits=0,
                 no_context_takeover=None,
                 window_bits=None,
                 mem_level=None):
        """

        :param offer: The offer being accepted.
        :type offer: Instance of :class:`autobahn.compress.PerMessageDeflateOffer`.
        :param request_no_context_takeover: When ``True``, the server requests the "no context takeover" feature.
        :type request_no_context_takeover: bool
        :param request_max_window_bits: When non-zero, the server requests the given "max window size" (must be
            and integer from the interval ``[8..15]``).
        :param request_max_window_bits: int
        :param no_context_takeover: Override server ("server-to-client direction") context takeover (this must
                be compatible with the offer).
        :type no_context_takeover: bool
        :param window_bits: Override server ("server-to-client direction") window size (this must be
                compatible with the offer).
        :type window_bits: int
        :param mem_level: Set server ("server-to-client direction") memory level.
        :type mem_level: int
        """
        if not isinstance(offer, PerMessageDeflateOffer):
            raise Exception("invalid type %s for offer" % type(offer))

        self.offer = offer

        if type(request_no_context_takeover) != bool:
            raise Exception("invalid type %s for request_no_context_takeover" % type(request_no_context_takeover))

        if request_no_context_takeover and not offer.accept_no_context_takeover:
            raise Exception("invalid value %s for request_no_context_takeover - feature unsupported by client" % request_no_context_takeover)

        self.request_no_context_takeover = request_no_context_takeover

        if request_max_window_bits != 0 and request_max_window_bits not in self.WINDOW_SIZE_PERMISSIBLE_VALUES:
            raise Exception("invalid value %s for request_max_window_bits - permissible values %s" % (request_max_window_bits, self.WINDOW_SIZE_PERMISSIBLE_VALUES))

        if request_max_window_bits != 0 and not offer.accept_max_window_bits:
            raise Exception("invalid value %s for request_max_window_bits - feature unsupported by client" % request_max_window_bits)

        self.request_max_window_bits = request_max_window_bits

        if no_context_takeover is not None:
            if type(no_context_takeover) != bool:
                raise Exception("invalid type %s for no_context_takeover" % type(no_context_takeover))

            if offer.request_no_context_takeover and not no_context_takeover:
                raise Exception("invalid value %s for no_context_takeover - client requested feature" % no_context_takeover)

        self.no_context_takeover = no_context_takeover

        if window_bits is not None:
            if window_bits not in self.WINDOW_SIZE_PERMISSIBLE_VALUES:
                raise Exception("invalid value %s for window_bits - permissible values %s" % (window_bits, self.WINDOW_SIZE_PERMISSIBLE_VALUES))

            if offer.request_max_window_bits != 0 and window_bits > offer.request_max_window_bits:
                raise Exception("invalid value %s for window_bits - client requested lower maximum value" % window_bits)

        self.window_bits = window_bits

        if mem_level is not None:
            if mem_level not in self.MEM_LEVEL_PERMISSIBLE_VALUES:
                raise Exception("invalid value %s for mem_level - permissible values %s" % (mem_level, self.MEM_LEVEL_PERMISSIBLE_VALUES))

        self.mem_level = mem_level

    def get_extension_string(self):
        """
        Returns the WebSocket extension configuration string as sent to the server.

        :returns: PMCE configuration string.
        :rtype: str
        """
        pmce_string = self.EXTENSION_NAME
        if self.offer.request_no_context_takeover:
            pmce_string += "; server_no_context_takeover"
        if self.offer.request_max_window_bits != 0:
            pmce_string += "; server_max_window_bits=%d" % self.offer.request_max_window_bits
        if self.request_no_context_takeover:
            pmce_string += "; client_no_context_takeover"
        if self.request_max_window_bits != 0:
            pmce_string += "; client_max_window_bits=%d" % self.request_max_window_bits
        return pmce_string

    def __json__(self):
        """
        Returns a JSON serializable object representation.

        :returns: JSON serializable representation.
        :rtype: dict
        """
        return {'extension': self.EXTENSION_NAME,
                'offer': self.offer.__json__(),
                'request_no_context_takeover': self.request_no_context_takeover,
                'request_max_window_bits': self.request_max_window_bits,
                'no_context_takeover': self.no_context_takeover,
                'window_bits': self.window_bits,
                'mem_level': self.mem_level}

    def __repr__(self):
        """
        Returns Python object representation that can be eval'ed to reconstruct the object.

        :returns: Python string representation.
        :rtype: str
        """
        return "PerMessageDeflateOfferAccept(offer = %s, request_no_context_takeover = %s, request_max_window_bits = %s, no_context_takeover = %s, window_bits = %s, mem_level = %s)" % (self.offer.__repr__(), self.request_no_context_takeover, self.request_max_window_bits, self.no_context_takeover, self.window_bits, self.mem_level)


@public
class PerMessageDeflateResponse(PerMessageCompressResponse, PerMessageDeflateMixin):
    """
    Set of parameters for `permessage-deflate` responded by server.
    """

    @classmethod
    def parse(cls, params):
        """
        Parses a WebSocket extension response for `permessage-deflate` provided by a server to a client.

        :param params: Output from :func:`autobahn.websocket.WebSocketProtocol._parseExtensionsHeader`.
        :type params: list

        :returns: A new instance of :class:`autobahn.compress.PerMessageDeflateResponse`.
        :rtype: obj
        """
        client_max_window_bits = 0
        client_no_context_takeover = False
        server_max_window_bits = 0
        server_no_context_takeover = False

        for p in params:

            if len(params[p]) > 1:
                raise Exception("multiple occurrence of extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

            val = params[p][0]

            if p == 'client_max_window_bits':
                try:
                    val = int(val)
                except:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    if val not in PerMessageDeflateMixin.WINDOW_SIZE_PERMISSIBLE_VALUES:
                        raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                    else:
                        client_max_window_bits = val

            elif p == 'client_no_context_takeover':
                # noinspection PySimplifyBooleanCheck
                if val is not True:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    client_no_context_takeover = True

            elif p == 'server_max_window_bits':
                try:
                    val = int(val)
                except:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    if val not in PerMessageDeflateMixin.WINDOW_SIZE_PERMISSIBLE_VALUES:
                        raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                    else:
                        server_max_window_bits = val

            elif p == 'server_no_context_takeover':
                # noinspection PySimplifyBooleanCheck
                if val is not True:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    server_no_context_takeover = True

            else:
                raise Exception("illegal extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

        response = cls(client_max_window_bits,
                       client_no_context_takeover,
                       server_max_window_bits,
                       server_no_context_takeover)
        return response

    def __init__(self,
                 client_max_window_bits,
                 client_no_context_takeover,
                 server_max_window_bits,
                 server_no_context_takeover):
        """

        :param client_max_window_bits: FIXME
        :type client_max_window_bits: int
        :param client_no_context_takeover: FIXME
        :type client_no_context_takeover: bool
        :param server_max_window_bits: FIXME
        :type server_max_window_bits: int
        :param server_no_context_takeover: FIXME
        :type server_no_context_takeover: bool
        """
        self.client_max_window_bits = client_max_window_bits
        self.client_no_context_takeover = client_no_context_takeover
        self.server_max_window_bits = server_max_window_bits
        self.server_no_context_takeover = server_no_context_takeover

    def __json__(self):
        """
        Returns a JSON serializable object representation.

        :returns: JSON serializable representation.
        :rtype: dict
        """
        return {'extension': self.EXTENSION_NAME,
                'client_max_window_bits': self.client_max_window_bits,
                'client_no_context_takeover': self.client_no_context_takeover,
                'server_max_window_bits': self.server_max_window_bits,
                'server_no_context_takeover': self.server_no_context_takeover}

    def __repr__(self):
        """
        Returns Python object representation that can be eval'ed to reconstruct the object.

        :returns: Python string representation.
        :rtype: str
        """
        return "PerMessageDeflateResponse(client_max_window_bits = %s, client_no_context_takeover = %s, server_max_window_bits = %s, server_no_context_takeover = %s)" % (self.client_max_window_bits, self.client_no_context_takeover, self.server_max_window_bits, self.server_no_context_takeover)


@public
class PerMessageDeflateResponseAccept(PerMessageCompressResponseAccept, PerMessageDeflateMixin):
    """
    Set of parameters with which to accept an `permessage-deflate` response
    from a server by a client.
    """

    def __init__(self,
                 response,
                 no_context_takeover=None,
                 window_bits=None,
                 mem_level=None):
        """

        :param response: The response being accepted.
        :type response: Instance of :class:`autobahn.compress.PerMessageDeflateResponse`.
        :param no_context_takeover: Override client ("client-to-server direction") context takeover (this must be compatible with response).
        :type no_context_takeover: bool
        :param window_bits: Override client ("client-to-server direction") window size (this must be compatible with response).
        :type window_bits: int
        :param mem_level: Set client ("client-to-server direction") memory level.
        :type mem_level: int
        """
        if not isinstance(response, PerMessageDeflateResponse):
            raise Exception("invalid type %s for response" % type(response))

        self.response = response

        if no_context_takeover is not None:
            if type(no_context_takeover) != bool:
                raise Exception("invalid type %s for no_context_takeover" % type(no_context_takeover))

            if response.client_no_context_takeover and not no_context_takeover:
                raise Exception("invalid value %s for no_context_takeover - server requested feature" % no_context_takeover)

        self.no_context_takeover = no_context_takeover

        if window_bits is not None:
            if window_bits not in self.WINDOW_SIZE_PERMISSIBLE_VALUES:
                raise Exception("invalid value %s for window_bits - permissible values %s" % (window_bits, self.WINDOW_SIZE_PERMISSIBLE_VALUES))

            if response.client_max_window_bits != 0 and window_bits > response.client_max_window_bits:
                raise Exception("invalid value %s for window_bits - server requested lower maximum value" % window_bits)

        self.window_bits = window_bits

        if mem_level is not None:
            if mem_level not in self.MEM_LEVEL_PERMISSIBLE_VALUES:
                raise Exception("invalid value %s for mem_level - permissible values %s" % (mem_level, self.MEM_LEVEL_PERMISSIBLE_VALUES))

        self.mem_level = mem_level

    def __json__(self):
        """
        Returns a JSON serializable object representation.

        :returns: JSON serializable representation.
        :rtype: dict
        """
        return {'extension': self.EXTENSION_NAME,
                'response': self.response.__json__(),
                'no_context_takeover': self.no_context_takeover,
                'window_bits': self.window_bits,
                'mem_level': self.mem_level}

    def __repr__(self):
        """
        Returns Python object representation that can be eval'ed to reconstruct the object.

        :returns: Python string representation.
        :rtype: str
        """
        return "PerMessageDeflateResponseAccept(response = %s, no_context_takeover = %s, window_bits = %s, mem_level = %s)" % (self.response.__repr__(), self.no_context_takeover, self.window_bits, self.mem_level)


# noinspection PyArgumentList
class PerMessageDeflate(PerMessageCompress, PerMessageDeflateMixin):
    """
    `permessage-deflate` WebSocket extension processor.
    """
    DEFAULT_WINDOW_BITS = zlib.MAX_WBITS
    DEFAULT_MEM_LEVEL = 8

    @classmethod
    def create_from_response_accept(cls, is_server, accept):
        # accept: instance of PerMessageDeflateResponseAccept
        pmce = cls(is_server,
                   accept.response.server_no_context_takeover,
                   accept.no_context_takeover if accept.no_context_takeover is not None else accept.response.client_no_context_takeover,
                   accept.response.server_max_window_bits,
                   accept.window_bits if accept.window_bits is not None else accept.response.client_max_window_bits,
                   accept.mem_level)
        return pmce

    @classmethod
    def create_from_offer_accept(cls, is_server, accept):
        # accept: instance of PerMessageDeflateOfferAccept
        pmce = cls(is_server,
                   accept.no_context_takeover if accept.no_context_takeover is not None else accept.offer.request_no_context_takeover,
                   accept.request_no_context_takeover,
                   accept.window_bits if accept.window_bits is not None else accept.offer.request_max_window_bits,
                   accept.request_max_window_bits,
                   accept.mem_level)
        return pmce

    def __init__(self,
                 is_server,
                 server_no_context_takeover,
                 client_no_context_takeover,
                 server_max_window_bits,
                 client_max_window_bits,
                 mem_level):
        self._is_server = is_server

        self.server_no_context_takeover = server_no_context_takeover
        self.client_no_context_takeover = client_no_context_takeover

        self.server_max_window_bits = server_max_window_bits if server_max_window_bits != 0 else self.DEFAULT_WINDOW_BITS
        self.client_max_window_bits = client_max_window_bits if client_max_window_bits != 0 else self.DEFAULT_WINDOW_BITS

        self.mem_level = mem_level if mem_level else self.DEFAULT_MEM_LEVEL

        self._compressor = None
        self._decompressor = None

    def __json__(self):
        return {'extension': self.EXTENSION_NAME,
                'is_server': self._is_server,
                'server_no_context_takeover': self.server_no_context_takeover,
                'client_no_context_takeover': self.client_no_context_takeover,
                'server_max_window_bits': self.server_max_window_bits,
                'client_max_window_bits': self.client_max_window_bits,
                'mem_level': self.mem_level}

    def __repr__(self):
        return "PerMessageDeflate(is_server = %s, server_no_context_takeover = %s, client_no_context_takeover = %s, server_max_window_bits = %s, client_max_window_bits = %s, mem_level = %s)" % (self._is_server, self.server_no_context_takeover, self.client_no_context_takeover, self.server_max_window_bits, self.client_max_window_bits, self.mem_level)

    def start_compress_message(self):
        # compressobj([level[, method[, wbits[, mem_level[, strategy]]]]])
        # http://bugs.python.org/issue19278
        # http://hg.python.org/cpython/rev/c54c8e71b79a
        if self._is_server:
            if self._compressor is None or self.server_no_context_takeover:
                self._compressor = zlib.compressobj(zlib.Z_DEFAULT_COMPRESSION, zlib.DEFLATED, -self.server_max_window_bits, self.mem_level)
        else:
            if self._compressor is None or self.client_no_context_takeover:
                self._compressor = zlib.compressobj(zlib.Z_DEFAULT_COMPRESSION, zlib.DEFLATED, -self.client_max_window_bits, self.mem_level)

    def compress_message_data(self, data):
        return self._compressor.compress(data)

    def end_compress_message(self):
        data = self._compressor.flush(zlib.Z_SYNC_FLUSH)
        return data[:-4]

    def start_decompress_message(self):
        if self._is_server:
            if self._decompressor is None or self.client_no_context_takeover:
                self._decompressor = zlib.decompressobj(-self.client_max_window_bits)
        else:
            if self._decompressor is None or self.server_no_context_takeover:
                self._decompressor = zlib.decompressobj(-self.server_max_window_bits)

    def decompress_message_data(self, data):
        return self._decompressor.decompress(data)

    def end_decompress_message(self):
        # Eat stripped LEN and NLEN field of a non-compressed block added
        # for Z_SYNC_FLUSH.
        self._decompressor.decompress(b'\x00\x00\xff\xff')
