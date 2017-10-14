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

from autobahn.websocket.compress_base import \
    PerMessageCompressOffer, \
    PerMessageCompressOfferAccept, \
    PerMessageCompressResponse, \
    PerMessageCompressResponseAccept, \
    PerMessageCompress

from autobahn.websocket.compress_deflate import \
    PerMessageDeflateMixin, \
    PerMessageDeflateOffer, \
    PerMessageDeflateOfferAccept, \
    PerMessageDeflateResponse, \
    PerMessageDeflateResponseAccept, \
    PerMessageDeflate

# this must be a list (not tuple), since we dynamically
# extend it ..
__all__ = [
    'PerMessageCompressOffer',
    'PerMessageCompressOfferAccept',
    'PerMessageCompressResponse',
    'PerMessageCompressResponseAccept',
    'PerMessageCompress',
    'PerMessageDeflateOffer',
    'PerMessageDeflateOfferAccept',
    'PerMessageDeflateResponse',
    'PerMessageDeflateResponseAccept',
    'PerMessageDeflate',
    'PERMESSAGE_COMPRESSION_EXTENSION'
]

# map of available compression extensions
PERMESSAGE_COMPRESSION_EXTENSION = {
    # class for 'permessage-deflate' is always available
    PerMessageDeflateMixin.EXTENSION_NAME: {
        'Offer': PerMessageDeflateOffer,
        'OfferAccept': PerMessageDeflateOfferAccept,
        'Response': PerMessageDeflateResponse,
        'ResponseAccept': PerMessageDeflateResponseAccept,
        'PMCE': PerMessageDeflate
    }
}


# include 'permessage-bzip2' classes if bzip2 is available
try:
    import bz2
except ImportError:
    bz2 = None
else:
    from autobahn.websocket.compress_bzip2 import \
        PerMessageBzip2Mixin, \
        PerMessageBzip2Offer, \
        PerMessageBzip2OfferAccept, \
        PerMessageBzip2Response, \
        PerMessageBzip2ResponseAccept, \
        PerMessageBzip2

    PMCE = {
        'Offer': PerMessageBzip2Offer,
        'OfferAccept': PerMessageBzip2OfferAccept,
        'Response': PerMessageBzip2Response,
        'ResponseAccept': PerMessageBzip2ResponseAccept,
        'PMCE': PerMessageBzip2
    }
    PERMESSAGE_COMPRESSION_EXTENSION[PerMessageBzip2Mixin.EXTENSION_NAME] = PMCE

    __all__.extend(['PerMessageBzip2Offer',
                    'PerMessageBzip2OfferAccept',
                    'PerMessageBzip2Response',
                    'PerMessageBzip2ResponseAccept',
                    'PerMessageBzip2'])


# include 'permessage-snappy' classes if Snappy is available
try:
    # noinspection PyPackageRequirements
    import snappy
except ImportError:
    snappy = None
else:
    from autobahn.websocket.compress_snappy import \
        PerMessageSnappyMixin, \
        PerMessageSnappyOffer, \
        PerMessageSnappyOfferAccept, \
        PerMessageSnappyResponse, \
        PerMessageSnappyResponseAccept, \
        PerMessageSnappy

    PMCE = {
        'Offer': PerMessageSnappyOffer,
        'OfferAccept': PerMessageSnappyOfferAccept,
        'Response': PerMessageSnappyResponse,
        'ResponseAccept': PerMessageSnappyResponseAccept,
        'PMCE': PerMessageSnappy
    }
    PERMESSAGE_COMPRESSION_EXTENSION[PerMessageSnappyMixin.EXTENSION_NAME] = PMCE

    __all__.extend(['PerMessageSnappyOffer',
                    'PerMessageSnappyOfferAccept',
                    'PerMessageSnappyResponse',
                    'PerMessageSnappyResponseAccept',
                    'PerMessageSnappy'])
