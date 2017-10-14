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

import snappy

from autobahn.websocket.compress_base import PerMessageCompressOffer, \
    PerMessageCompressOfferAccept, \
    PerMessageCompressResponse, \
    PerMessageCompressResponseAccept, \
    PerMessageCompress

__all__ = (
    'PerMessageSnappyMixin',
    'PerMessageSnappyOffer',
    'PerMessageSnappyOfferAccept',
    'PerMessageSnappyResponse',
    'PerMessageSnappyResponseAccept',
    'PerMessageSnappy',
)


class PerMessageSnappyMixin(object):
    """
    Mixin class for this extension.
    """

    EXTENSION_NAME = "permessage-snappy"
    """
    Name of this WebSocket extension.
    """


class PerMessageSnappyOffer(PerMessageCompressOffer, PerMessageSnappyMixin):
    """
    Set of extension parameters for `permessage-snappy` WebSocket extension
    offered by a client to a server.
    """

    @classmethod
    def parse(cls, params):
        """
        Parses a WebSocket extension offer for `permessage-snappy` provided by a client to a server.

        :param params: Output from :func:`autobahn.websocket.WebSocketProtocol._parseExtensionsHeader`.
        :type params: list

        :returns: A new instance of :class:`autobahn.compress.PerMessageSnappyOffer`.
        :rtype: obj
        """
        # extension parameter defaults
        accept_no_context_takeover = False
        request_no_context_takeover = False

        # verify/parse client ("client-to-server direction") parameters of permessage-snappy offer
        for p in params:

            if len(params[p]) > 1:
                raise Exception("multiple occurrence of extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

            val = params[p][0]

            if p == 'client_no_context_takeover':
                # noinspection PySimplifyBooleanCheck
                if val is not True:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    accept_no_context_takeover = True

            elif p == 'server_no_context_takeover':
                # noinspection PySimplifyBooleanCheck
                if val is not True:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    request_no_context_takeover = True

            else:
                raise Exception("illegal extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

        offer = cls(accept_no_context_takeover,
                    request_no_context_takeover)
        return offer

    def __init__(self,
                 accept_no_context_takeover=True,
                 request_no_context_takeover=False):
        """

        :param accept_no_context_takeover: Iff true, client accepts "no context takeover" feature.
        :type accept_no_context_takeover: bool
        :param request_no_context_takeover: Iff true, client request "no context takeover" feature.
        :type request_no_context_takeover: bool
        """
        if type(accept_no_context_takeover) != bool:
            raise Exception("invalid type %s for accept_no_context_takeover" % type(accept_no_context_takeover))

        self.accept_no_context_takeover = accept_no_context_takeover

        if type(request_no_context_takeover) != bool:
            raise Exception("invalid type %s for request_no_context_takeover" % type(request_no_context_takeover))

        self.request_no_context_takeover = request_no_context_takeover

    def get_extension_string(self):
        """
        Returns the WebSocket extension configuration string as sent to the server.

        :returns: PMCE configuration string.
        :rtype: str
        """
        pmce_string = self.EXTENSION_NAME
        if self.accept_no_context_takeover:
            pmce_string += "; client_no_context_takeover"
        if self.request_no_context_takeover:
            pmce_string += "; server_no_context_takeover"
        return pmce_string

    def __json__(self):
        """
        Returns a JSON serializable object representation.

        :returns: JSON serializable representation.
        :rtype: dict
        """
        return {'extension': self.EXTENSION_NAME,
                'accept_no_context_takeover': self.accept_no_context_takeover,
                'request_no_context_takeover': self.request_no_context_takeover}

    def __repr__(self):
        """
        Returns Python object representation that can be eval'ed to reconstruct the object.

        :returns: Python string representation.
        :rtype: str
        """
        return "PerMessageSnappyOffer(accept_no_context_takeover = %s, request_no_context_takeover = %s)" % (self.accept_no_context_takeover, self.request_no_context_takeover)


class PerMessageSnappyOfferAccept(PerMessageCompressOfferAccept, PerMessageSnappyMixin):
    """
    Set of parameters with which to accept an `permessage-snappy` offer
    from a client by a server.
    """

    def __init__(self,
                 offer,
                 request_no_context_takeover=False,
                 no_context_takeover=None):
        """

        :param offer: The offer being accepted.
        :type offer: Instance of :class:`autobahn.compress.PerMessageSnappyOffer`.
        :param request_no_context_takeover: Iff true, server request "no context takeover" feature.
        :type request_no_context_takeover: bool
        :param no_context_takeover: Override server ("server-to-client direction") context takeover (this must be compatible with offer).
        :type no_context_takeover: bool
        """
        if not isinstance(offer, PerMessageSnappyOffer):
            raise Exception("invalid type %s for offer" % type(offer))

        self.offer = offer

        if type(request_no_context_takeover) != bool:
            raise Exception("invalid type %s for request_no_context_takeover" % type(request_no_context_takeover))

        if request_no_context_takeover and not offer.accept_no_context_takeover:
            raise Exception("invalid value %s for request_no_context_takeover - feature unsupported by client" % request_no_context_takeover)

        self.request_no_context_takeover = request_no_context_takeover

        if no_context_takeover is not None:
            if type(no_context_takeover) != bool:
                raise Exception("invalid type %s for no_context_takeover" % type(no_context_takeover))

            if offer.request_no_context_takeover and not no_context_takeover:
                raise Exception("invalid value %s for no_context_takeover - client requested feature" % no_context_takeover)

        self.no_context_takeover = no_context_takeover

    def get_extension_string(self):
        """
        Returns the WebSocket extension configuration string as sent to the server.

        :returns: PMCE configuration string.
        :rtype: str
        """
        pmce_string = self.EXTENSION_NAME
        if self.offer.request_no_context_takeover:
            pmce_string += "; server_no_context_takeover"
        if self.request_no_context_takeover:
            pmce_string += "; client_no_context_takeover"
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
                'no_context_takeover': self.no_context_takeover}

    def __repr__(self):
        """
        Returns Python object representation that can be eval'ed to reconstruct the object.

        :returns: Python string representation.
        :rtype: str
        """
        return "PerMessageSnappyAccept(offer = %s, request_no_context_takeover = %s, no_context_takeover = %s)" % (self.offer.__repr__(), self.request_no_context_takeover, self.no_context_takeover)


class PerMessageSnappyResponse(PerMessageCompressResponse, PerMessageSnappyMixin):
    """
    Set of parameters for `permessage-snappy` responded by server.
    """

    @classmethod
    def parse(cls, params):
        """
        Parses a WebSocket extension response for `permessage-snappy` provided by a server to a client.

        :param params: Output from :func:`autobahn.websocket.WebSocketProtocol._parseExtensionsHeader`.
        :type params: list

        :returns: A new instance of :class:`autobahn.compress.PerMessageSnappyResponse`.
        :rtype: obj
        """
        client_no_context_takeover = False
        server_no_context_takeover = False

        for p in params:

            if len(params[p]) > 1:
                raise Exception("multiple occurrence of extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

            val = params[p][0]

            if p == 'client_no_context_takeover':
                # noinspection PySimplifyBooleanCheck
                if val is not True:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    client_no_context_takeover = True

            elif p == 'server_no_context_takeover':
                # noinspection PySimplifyBooleanCheck
                if val is not True:
                    raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
                else:
                    server_no_context_takeover = True

            else:
                raise Exception("illegal extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

        response = cls(client_no_context_takeover,
                       server_no_context_takeover)
        return response

    def __init__(self,
                 client_no_context_takeover,
                 server_no_context_takeover):
        self.client_no_context_takeover = client_no_context_takeover
        self.server_no_context_takeover = server_no_context_takeover

    def __json__(self):
        """
        Returns a JSON serializable object representation.

        :returns: JSON serializable representation.
        :rtype: dict
        """
        return {'extension': self.EXTENSION_NAME,
                'client_no_context_takeover': self.client_no_context_takeover,
                'server_no_context_takeover': self.server_no_context_takeover}

    def __repr__(self):
        """
        Returns Python object representation that can be eval'ed to reconstruct the object.

        :returns: Python string representation.
        :rtype: str
        """
        return "PerMessageSnappyResponse(client_no_context_takeover = %s, server_no_context_takeover = %s)" % (self.client_no_context_takeover, self.server_no_context_takeover)


class PerMessageSnappyResponseAccept(PerMessageCompressResponseAccept, PerMessageSnappyMixin):
    """
    Set of parameters with which to accept an `permessage-snappy` response
    from a server by a client.
    """

    def __init__(self,
                 response,
                 no_context_takeover=None):
        """

        :param response: The response being accepted.
        :type response: Instance of :class:`autobahn.compress.PerMessageSnappyResponse`.
        :param no_context_takeover: Override client ("client-to-server direction") context takeover (this must be compatible with response).
        :type no_context_takeover: bool
        """
        if not isinstance(response, PerMessageSnappyResponse):
            raise Exception("invalid type %s for response" % type(response))

        self.response = response

        if no_context_takeover is not None:
            if type(no_context_takeover) != bool:
                raise Exception("invalid type %s for no_context_takeover" % type(no_context_takeover))

            if response.client_no_context_takeover and not no_context_takeover:
                raise Exception("invalid value %s for no_context_takeover - server requested feature" % no_context_takeover)

        self.no_context_takeover = no_context_takeover

    def __json__(self):
        """
        Returns a JSON serializable object representation.

        :returns: JSON serializable representation.
        :rtype: dict
        """
        return {'extension': self.EXTENSION_NAME,
                'response': self.response.__json__(),
                'no_context_takeover': self.no_context_takeover}

    def __repr__(self):
        """
        Returns Python object representation that can be eval'ed to reconstruct the object.

        :returns: Python string representation.
        :rtype: str
        """
        return "PerMessageSnappyResponseAccept(response = %s, no_context_takeover = %s)" % (self.response.__repr__(), self.no_context_takeover)


class PerMessageSnappy(PerMessageCompress, PerMessageSnappyMixin):
    """
    `permessage-snappy` WebSocket extension processor.
    """

    @classmethod
    def create_from_response_accept(cls, is_server, accept):
        pmce = cls(is_server,
                   accept.response.server_no_context_takeover,
                   accept.no_context_takeover if accept.no_context_takeover is not None else accept.response.client_no_context_takeover)
        return pmce

    @classmethod
    def create_from_offer_accept(cls, is_server, accept):
        pmce = cls(is_server,
                   accept.no_context_takeover if accept.no_context_takeover is not None else accept.offer.request_no_context_takeover,
                   accept.request_no_context_takeover)
        return pmce

    def __init__(self,
                 is_server,
                 server_no_context_takeover,
                 client_no_context_takeover):
        self._is_server = is_server
        self.server_no_context_takeover = server_no_context_takeover
        self.client_no_context_takeover = client_no_context_takeover

        self._compressor = None
        self._decompressor = None

    def __json__(self):
        return {'extension': self.EXTENSION_NAME,
                'server_no_context_takeover': self.server_no_context_takeover,
                'client_no_context_takeover': self.client_no_context_takeover}

    def __repr__(self):
        return "PerMessageSnappy(is_server = %s, server_no_context_takeover = %s, client_no_context_takeover = %s)" % (self._is_server, self.server_no_context_takeover, self.client_no_context_takeover)

    def start_compress_message(self):
        if self._is_server:
            if self._compressor is None or self.server_no_context_takeover:
                self._compressor = snappy.StreamCompressor()
        else:
            if self._compressor is None or self.client_no_context_takeover:
                self._compressor = snappy.StreamCompressor()

    def compress_message_data(self, data):
        return self._compressor.add_chunk(data)

    def end_compress_message(self):
        return ""

    def start_decompress_message(self):
        if self._is_server:
            if self._decompressor is None or self.client_no_context_takeover:
                self._decompressor = snappy.StreamDecompressor()
        else:
            if self._decompressor is None or self.server_no_context_takeover:
                self._decompressor = snappy.StreamDecompressor()

    def decompress_message_data(self, data):
        return self._decompressor.decompress(data)

    def end_decompress_message(self):
        pass
