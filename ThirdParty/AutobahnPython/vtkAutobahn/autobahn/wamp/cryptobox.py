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

import six

from autobahn.util import public
from autobahn.wamp.interfaces import IPayloadCodec
from autobahn.wamp.types import EncodedPayload
from autobahn.wamp.serializer import _dumps as _json_dumps
from autobahn.wamp.serializer import _loads as _json_loads

__all__ = [
    'HAS_CRYPTOBOX',
    'EncodedPayload'
]

try:
    # try to import everything we need for WAMP-cryptobox
    from nacl.encoding import Base64Encoder, RawEncoder
    from nacl.public import PrivateKey, PublicKey, Box
    from nacl.utils import random
    from pytrie import StringTrie
except ImportError:
    HAS_CRYPTOBOX = False
else:
    HAS_CRYPTOBOX = True
    __all__.extend(['Key', 'KeyRing'])


if HAS_CRYPTOBOX:

    @public
    class Key(object):
        """
        Holds originator and responder keys for an URI.

        The originator is either a caller or a publisher. The responder is either a callee or subscriber.
        """

        def __init__(self, originator_priv=None, originator_pub=None, responder_priv=None, responder_pub=None):
            # the originator private and public keys, as available
            if originator_priv:
                self.originator_priv = PrivateKey(originator_priv, encoder=Base64Encoder)
            else:
                self.originator_priv = None

            if self.originator_priv:
                self.originator_pub = self.originator_priv.public_key
                assert(originator_pub is None or originator_pub == self.originator_pub)
            else:
                self.originator_pub = PublicKey(originator_pub, encoder=Base64Encoder)

            # the responder private and public keys, as available
            if responder_priv:
                self.responder_priv = PrivateKey(responder_priv, encoder=Base64Encoder)
            else:
                self.responder_priv = None

            if self.responder_priv:
                self.responder_pub = self.responder_priv.public_key
                assert(responder_pub is None or responder_pub == self.responder_pub)
            else:
                self.responder_pub = PublicKey(responder_pub, encoder=Base64Encoder)

            # this crypto box is for originators (callers, publishers):
            #
            #  1. _encrypting_ WAMP messages outgoing from originators: CALL*, PUBLISH*
            #  2. _decrypting_ WAMP messages incoming to originators: RESULT*, ERROR
            #
            if self.originator_priv and self.responder_pub:
                self.originator_box = Box(self.originator_priv, self.responder_pub)
            else:
                self.originator_box = None

            # this crypto box is for responders (callees, subscribers):
            #
            #  1. _decrypting_ WAMP messages incoming to responders: INVOCATION*, EVENT*
            #  2. _encrypting_ WAMP messages outgoing from responders: YIELD*, ERROR
            #
            if self.responder_priv and self.originator_pub:
                self.responder_box = Box(self.responder_priv, self.originator_pub)
            else:
                self.responder_box = None

            if not (self.originator_box or self.responder_box):
                raise Exception("insufficient keys provided for at least originator or responder role")

    @public
    class KeyRing(object):
        """
        A keyring holds (cryptobox) public-private key pairs for use with WAMP-cryptobox payload
        encryption. The keyring can be set on a WAMP session and then transparently will get used
        for encrypting and decrypting WAMP message payloads.
        """

        @public
        def __init__(self, default_key=None):
            """

            Create a new key ring to hold public and private keys mapped from an URI space.
            """
            assert(default_key is None or isinstance(default_key, Key) or type(default_key == six.text_type))
            self._uri_to_key = StringTrie()
            if type(default_key) == six.text_type:
                default_key = Key(originator_priv=default_key, responder_priv=default_key)
            self._default_key = default_key

        @public
        def generate_key(self):
            """
            Generate a new private key and return a pair with the base64 encodings
            of (priv_key, pub_key).
            """
            key = PrivateKey.generate()
            priv_key = key.encode(encoder=Base64Encoder)
            pub_key = key.public_key.encode(encoder=Base64Encoder)
            return (u'{}'.format(priv_key), u''.format(pub_key))

        @public
        def set_key(self, uri, key):
            """
            Add a key set for a given URI.
            """
            assert(type(uri) == six.text_type)
            assert(key is None or isinstance(key, Key) or type(key) == six.text_type)
            if type(key) == six.text_type:
                key = Key(originator_priv=key, responder_priv=key)
            if uri == u'':
                self._default_key = key
            else:
                if key is None:
                    if uri in self._uri_to_key:
                        del self._uri_to_key[uri]
                else:
                    self._uri_to_key[uri] = key

        def _get_box(self, is_originating, uri, match_exact=False):
            try:
                if match_exact:
                    key = self._uri_to_key[uri]
                else:
                    key = self._uri_to_key.longest_prefix_value(uri)
            except KeyError:
                if self._default_key:
                    key = self._default_key
                else:
                    return None

            if is_originating:
                return key.originator_box
            else:
                return key.responder_box

        @public
        def encode(self, is_originating, uri, args=None, kwargs=None):
            """
            Encrypt the given WAMP URI, args and kwargs into an EncodedPayload instance, or None
            if the URI should not be encrypted.
            """
            assert(type(is_originating) == bool)
            assert(type(uri) == six.text_type)
            assert(args is None or type(args) in (list, tuple))
            assert(kwargs is None or type(kwargs) == dict)

            box = self._get_box(is_originating, uri)

            if not box:
                # if we didn't find a crypto box, then return None, which
                # signals that the payload travel unencrypted (normal)
                return None

            payload = {
                u'uri': uri,
                u'args': args,
                u'kwargs': kwargs
            }
            nonce = random(Box.NONCE_SIZE)
            payload_ser = _json_dumps(payload).encode('utf8')

            payload_encr = box.encrypt(payload_ser, nonce, encoder=RawEncoder)

            # above returns an instance of http://pynacl.readthedocs.io/en/latest/utils/#nacl.utils.EncryptedMessage
            # which is a bytes _subclass_! hence we apply bytes() to get at the underlying plain
            # bytes "scalar", which is the concatenation of `payload_encr.nonce + payload_encr.ciphertext`
            payload_bytes = bytes(payload_encr)
            payload_key = None

            return EncodedPayload(payload_bytes, u'cryptobox', u'json', enc_key=payload_key)

        @public
        def decode(self, is_originating, uri, encoded_payload):
            """
            Decrypt the given WAMP URI and EncodedPayload into a tuple ``(uri, args, kwargs)``.
            """
            assert(type(uri) == six.text_type)
            assert(isinstance(encoded_payload, EncodedPayload))
            assert(encoded_payload.enc_algo == u'cryptobox')

            box = self._get_box(is_originating, uri)

            if not box:
                raise Exception("received encrypted payload, but can't find key!")

            payload_ser = box.decrypt(encoded_payload.payload, encoder=RawEncoder)

            if encoded_payload.enc_serializer != u'json':
                raise Exception("received encrypted payload, but don't know how to process serializer '{}'".format(encoded_payload.enc_serializer))

            payload = _json_loads(payload_ser)

            uri = payload.get(u'uri', None)
            args = payload.get(u'args', None)
            kwargs = payload.get(u'kwargs', None)

            return uri, args, kwargs

    # A WAMP-cryptobox keyring can work as a codec for
    # payload transparency
    IPayloadCodec.register(KeyRing)
