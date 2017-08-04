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

__all__ = (
    'PerMessageCompressOffer',
    'PerMessageCompressOfferAccept',
    'PerMessageCompressResponse',
    'PerMessageCompressResponseAccept',
    'PerMessageCompress',
)


class PerMessageCompressOffer(object):
    """
    Base class for WebSocket compression parameter client offers.
    """


class PerMessageCompressOfferAccept(object):
    """
    Base class for WebSocket compression parameter client offer accepts by the server.
    """


class PerMessageCompressResponse(object):
    """
    Base class for WebSocket compression parameter server responses.
    """


class PerMessageCompressResponseAccept(object):
    """
    Base class for WebSocket compression parameter server response accepts by client.
    """


class PerMessageCompress(object):
    """
    Base class for WebSocket compression negotiated parameters.
    """
