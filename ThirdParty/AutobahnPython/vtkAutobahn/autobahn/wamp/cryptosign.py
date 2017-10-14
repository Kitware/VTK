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
import struct

import six
import txaio

from autobahn import util
from autobahn.wamp.types import Challenge

__all__ = [
    'HAS_CRYPTOSIGN',
]

try:
    # try to import everything we need for WAMP-cryptosign
    from nacl import public, encoding, signing, bindings
except ImportError:
    HAS_CRYPTOSIGN = False
else:
    HAS_CRYPTOSIGN = True
    __all__.append('SigningKey')


def _unpack(keydata):
    """
    Unpack a SSH agent key blob into parts.

    See: http://blog.oddbit.com/2011/05/08/converting-openssh-public-keys/
    """
    parts = []
    while keydata:
        # read the length of the data
        dlen = struct.unpack('>I', keydata[:4])[0]

        # read in <length> bytes
        data, keydata = keydata[4:dlen + 4], keydata[4 + dlen:]
        parts.append(data)
    return parts


def _pack(keyparts):
    """
    Pack parts into a SSH key blob.
    """
    parts = []
    for part in keyparts:
        parts.append(struct.pack('>I', len(part)))
        parts.append(part)
    return b''.join(parts)


def _read_ssh_ed25519_pubkey(keydata):
    """
    Parse an OpenSSH Ed25519 public key from a string into a raw public key.

    Example input:

        ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIJukDU5fqXv/yVhSirsDWsUFyOodZyCSLxyitPPzWJW9 oberstet@office-corei7

    :param keydata: The OpenSSH Ed25519 public key data to parse.
    :type keydata: str

    :returns: pair of raw public key (32 bytes) and comment
    :rtype: tuple
    """
    if type(keydata) != six.text_type:
        raise Exception("invalid type {} for keydata".format(type(keydata)))

    parts = keydata.strip().split()
    if len(parts) != 3:
        raise Exception('invalid SSH Ed25519 public key')
    algo, keydata, comment = parts

    if algo != u'ssh-ed25519':
        raise Exception('not a Ed25519 SSH public key (but {})'.format(algo))

    blob = binascii.a2b_base64(keydata)

    try:
        key = _unpack(blob)[1]
    except Exception as e:
        raise Exception('could not parse key ({})'.format(e))

    if len(key) != 32:
        raise Exception('invalid length {} for embedded raw key (must be 32 bytes)'.format(len(key)))

    return key, comment


class _SSHPacketReader:
    """
    Read OpenSSH packet format which is used for key material.
    """

    def __init__(self, packet):
        self._packet = packet
        self._idx = 0
        self._len = len(packet)

    def get_remaining_payload(self):
        return self._packet[self._idx:]

    def get_bytes(self, size):
        if self._idx + size > self._len:
            raise Exception('incomplete packet')

        value = self._packet[self._idx:self._idx + size]
        self._idx += size
        return value

    def get_uint32(self):
        return int.from_bytes(self.get_bytes(4), 'big')

    def get_string(self):
        return self.get_bytes(self.get_uint32())


def _read_ssh_ed25519_privkey(keydata):
    """
    Parse an OpenSSH Ed25519 private key from a string into a raw private key.

    Example input:

        -----BEGIN OPENSSH PRIVATE KEY-----
        b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAAMwAAAAtzc2gtZW
        QyNTUxOQAAACCbpA1OX6l7/8lYUoq7A1rFBcjqHWcgki8corTz81iVvQAAAKDWjZ0Y1o2d
        GAAAAAtzc2gtZWQyNTUxOQAAACCbpA1OX6l7/8lYUoq7A1rFBcjqHWcgki8corTz81iVvQ
        AAAEArodzIMjH9MOBz0X+HDvL06rEJOMYFhzGQ5zXPM7b7fZukDU5fqXv/yVhSirsDWsUF
        yOodZyCSLxyitPPzWJW9AAAAFm9iZXJzdGV0QG9mZmljZS1jb3JlaTcBAgMEBQYH
        -----END OPENSSH PRIVATE KEY-----


    :param keydata: The OpenSSH Ed25519 private key data to parse.
    :type keydata: str

    :returns: pair of raw private key (32 bytes) and comment
    :rtype: tuple
    """

    # Some pointers:
    # https://github.com/ronf/asyncssh/blob/master/asyncssh/public_key.py
    # https://github.com/ronf/asyncssh/blob/master/asyncssh/ed25519.py
    # crypto_sign_ed25519_sk_to_seed
    # https://github.com/jedisct1/libsodium/blob/master/src/libsodium/crypto_sign/ed25519/sign_ed25519_api.c#L27
    # https://tools.ietf.org/html/draft-bjh21-ssh-ed25519-02
    # http://blog.oddbit.com/2011/05/08/converting-openssh-public-keys/

    SSH_BEGIN = u'-----BEGIN OPENSSH PRIVATE KEY-----'
    SSH_END = u'-----END OPENSSH PRIVATE KEY-----'
    OPENSSH_KEY_V1 = b'openssh-key-v1\0'

    if not (keydata.startswith(SSH_BEGIN) and keydata.endswith(SSH_END)):
        raise Exception('invalid OpenSSH private key (does not start/end with OPENSSH preamble)')

    ssh_end = keydata.find(SSH_END)
    keydata = keydata[len(SSH_BEGIN):ssh_end]
    keydata = u''.join([x.strip() for x in keydata.split()])
    blob = binascii.a2b_base64(keydata)

    blob = blob[len(OPENSSH_KEY_V1):]
    packet = _SSHPacketReader(blob)

    cipher_name = packet.get_string()
    kdf = packet.get_string()
    packet.get_string()  # kdf_data
    nkeys = packet.get_uint32()
    packet.get_string()  # public_key
    key_data = packet.get_string()
    mac = packet.get_remaining_payload()

    block_size = 8

    if cipher_name != b'none':
        raise Exception('encrypted private keys not supported (please remove the passphrase from your private key or use SSH agent)')

    if kdf != b'none':
        raise Exception('passphrase encrypted private keys not supported')

    if nkeys != 1:
        raise Exception('multiple private keys in a key file not supported (found {} keys)'.format(nkeys))

    if mac:
        raise Exception('invalid OpenSSH private key (found remaining payload for mac)')

    packet = _SSHPacketReader(key_data)

    packet.get_uint32()  # check1
    packet.get_uint32()  # check2

    alg = packet.get_string()

    if alg != b'ssh-ed25519':
        raise Exception('invalid key type: we only support Ed25519 (found "{}")'.format(alg.decode('ascii')))

    vk = packet.get_string()
    sk = packet.get_string()

    if len(vk) != bindings.crypto_sign_PUBLICKEYBYTES:
        raise Exception('invalid public key length')

    if len(sk) != bindings.crypto_sign_SECRETKEYBYTES:
        raise Exception('invalid public key length')

    comment = packet.get_string()                             # comment
    pad = packet.get_remaining_payload()

    if len(pad) >= block_size or pad != bytes(range(1, len(pad) + 1)):
        raise Exception('invalid OpenSSH private key')

    # secret key (64 octets) = 32 octets seed || 32 octets secret key derived of seed
    seed = sk[:bindings.crypto_sign_SEEDBYTES]

    comment = comment.decode('ascii')

    return seed, comment


def _read_signify_ed25519_signature(signature_file):
    """
    Read a Ed25519 signature file created with OpenBSD signify.

    http://man.openbsd.org/OpenBSD-current/man1/signify.1
    """
    with open(signature_file) as f:
        # signature file format: 2nd line is base64 of 'Ed' || 8 random octets || 64 octets Ed25519 signature
        sig = binascii.a2b_base64(f.read().splitlines()[1])[10:]
        if len(sig) != 64:
            raise Exception('bogus Ed25519 signature: raw signature length was {}, but expected 64'.format(len(sig)))
        return sig


def _read_signify_ed25519_pubkey(pubkey_file):
    """
    Read a public key from a Ed25519 key pair created with OpenBSD signify.

    http://man.openbsd.org/OpenBSD-current/man1/signify.1
    """
    with open(pubkey_file) as f:
        # signature file format: 2nd line is base64 of 'Ed' || 8 random octets || 32 octets Ed25519 public key
        pubkey = binascii.a2b_base64(f.read().splitlines()[1])[10:]
        if len(pubkey) != 32:
            raise Exception('bogus Ed25519 public key: raw key length was {}, but expected 32'.format(len(pubkey)))
        return pubkey


def _qrcode_from_signify_ed25519_pubkey(pubkey_file, mode='text'):
    """

    Usage:

    1. Get the OpenBSD 5.7 release public key from here

        http://cvsweb.openbsd.org/cgi-bin/cvsweb/src/etc/signify/Attic/openbsd-57-base.pub?rev=1.1

    2. Generate QR Code and print to terminal

        print(cryptosign._qrcode_from_signify_ed25519_pubkey('openbsd-57-base.pub'))

    3. Compare to (scroll down) QR code here

        https://www.openbsd.org/papers/bsdcan-signify.html
    """
    assert(mode in ['text', 'svg'])

    import pyqrcode

    with open(pubkey_file) as f:
        pubkey = f.read().splitlines()[1]

        qr = pyqrcode.create(pubkey, error='L', mode='binary')

        if mode == 'text':
            return qr.terminal()

        elif mode == 'svg':
            import io
            data_buffer = io.BytesIO()

            qr.svg(data_buffer, omithw=True)

            return data_buffer.getvalue()

        else:
            raise Exception('logic error')


def _verify_signify_ed25519_signature(pubkey_file, signature_file, message):
    """
    Verify a Ed25519 signature created with OpenBSD signify.

    This will raise a `nacl.exceptions.BadSignatureError` if the signature is bad
    and return silently when the signature is good.

    Usage:

    1. Create a signature:

        signify-openbsd -S -s ~/.signify/crossbario-trustroot.sec -m .profile

    2. Verify the signature

        from autobahn.wamp import cryptosign

        with open('.profile', 'rb') as f:
            message = f.read()
            cryptosign._verify_signify_ed25519_signature('.signify/crossbario-trustroot.pub', '.profile.sig', message)

    http://man.openbsd.org/OpenBSD-current/man1/signify.1
    """
    pubkey = _read_signify_ed25519_pubkey(pubkey_file)
    verify_key = signing.VerifyKey(pubkey)
    sig = _read_signify_ed25519_signature(signature_file)
    verify_key.verify(message, sig)


# SigningKey from
#   - raw byte string or file with raw bytes
#   - SSH private key string or key file
#   - SSH agent proxy
#
# VerifyKey from
#   - raw byte string or file with raw bytes
#   - SSH public key string or key file

if HAS_CRYPTOSIGN:

    @util.public
    class SigningKey(object):
        """
        A cryptosign private key for signing, and hence usable for authentication or a
        public key usable for verification (but can't be used for signing).
        """

        def __init__(self, key, comment=None):
            """

            :param key: A Ed25519 private signing key or a Ed25519 public verification key.
            :type key: instance of nacl.public.VerifyKey or instance of nacl.public.SigningKey
            """
            if not (isinstance(key, signing.VerifyKey) or isinstance(key, signing.SigningKey)):
                raise Exception("invalid type {} for key".format(type(key)))

            if not (comment is None or type(comment) == six.text_type):
                raise Exception("invalid type {} for comment".format(type(comment)))

            self._key = key
            self._comment = comment
            self._can_sign = isinstance(key, signing.SigningKey)

        def __str__(self):
            comment = u'"{}"'.format(self.comment()) if self.comment() else None
            return u'Key(can_sign={}, comment={}, public_key={})'.format(self.can_sign(), comment, self.public_key())

        @util.public
        def can_sign(self):
            """
            Check if the key can be used to sign.

            :returns: `True`, iff the key can sign.
            :rtype: bool
            """
            return self._can_sign

        @util.public
        def comment(self):
            """
            Get the key comment (if any).

            :returns: The comment (if any) from the key.
            :rtype: str or None
            """
            return self._comment

        @util.public
        def public_key(self, binary=False):
            """
            Returns the public key part of a signing key or the (public) verification key.

            :returns: The public key in Hex encoding.
            :rtype: str or None
            """
            if isinstance(self._key, signing.SigningKey):
                key = self._key.verify_key
            else:
                key = self._key

            if binary:
                return key.encode()
            else:
                return key.encode(encoder=encoding.HexEncoder).decode('ascii')

        @util.public
        def sign(self, data):
            """
            Sign some data.

            :param data: The data to be signed.
            :type data: bytes

            :returns: The signature.
            :rtype: bytes
            """
            if not self._can_sign:
                raise Exception("a signing key required to sign")

            if type(data) != six.binary_type:
                raise Exception("data to be signed must be binary")

            # sig is a nacl.signing.SignedMessage
            sig = self._key.sign(data)

            # we only return the actual signature! if we return "sig",
            # it get coerced into the concatenation of message + signature
            # not sure which order, but we don't want that. we only want
            # the signature
            return txaio.create_future_success(sig.signature)

        @util.public
        def sign_challenge(self, session, challenge):
            """
            Sign WAMP-cryptosign challenge.

            :param challenge: The WAMP-cryptosign challenge object for which a signature should be computed.
            :type challenge: instance of autobahn.wamp.types.Challenge

            :returns: A Deferred/Future that resolves to the computed signature.
            :rtype: str
            """
            if not isinstance(challenge, Challenge):
                raise Exception("challenge must be instance of autobahn.wamp.types.Challenge, not {}".format(type(challenge)))

            if u'challenge' not in challenge.extra:
                raise Exception("missing challenge value in challenge.extra")

            # the challenge sent by the router (a 32 bytes random value)
            challenge_hex = challenge.extra[u'challenge']

            # the challenge for WAMP-cryptosign is a 32 bytes random value in Hex encoding (that is, a unicode string)
            challenge_raw = binascii.a2b_hex(challenge_hex)

            # if the transport has a channel ID, the message to be signed by the client actually
            # is the XOR of the challenge and the channel ID
            channel_id_raw = session._transport.get_channel_id()
            if channel_id_raw:
                data = util.xor(challenge_raw, channel_id_raw)
            else:
                data = challenge_raw

            # a raw byte string is signed, and the signature is also a raw byte string
            d1 = self.sign(data)

            # asyncio lacks callback chaining (and we cannot use co-routines, since we want
            # to support older Pythons), hence we need d2
            d2 = txaio.create_future()

            def process(signature_raw):
                # convert the raw signature into a hex encode value (unicode string)
                signature_hex = binascii.b2a_hex(signature_raw).decode('ascii')

                # we return the concatenation of the signature and the message signed (96 bytes)
                data_hex = binascii.b2a_hex(data).decode('ascii')

                sig = signature_hex + data_hex
                txaio.resolve(d2, sig)

            txaio.add_callbacks(d1, process, None)

            return d2

        @util.public
        @classmethod
        def from_key_bytes(cls, keydata, comment=None):
            if not (comment is None or type(comment) == six.text_type):
                raise ValueError("invalid type {} for comment".format(type(comment)))

            if len(keydata) != 32:
                raise ValueError("invalid key length {}".format(len(keydata)))

            key = signing.SigningKey(keydata)
            return cls(key, comment)

        @classmethod
        def from_raw_key(cls, filename, comment=None):
            """
            Load an Ed25519 (private) signing key (actually, the seed for the key) from a raw file of 32 bytes length.
            This can be any random byte sequence, such as generated from Python code like

                os.urandom(32)

            or from the shell

                dd if=/dev/urandom of=client02.key bs=1 count=32

            :param filename: Filename of the key.
            :type filename: str
            :param comment: Comment for key (optional).
            :type comment: str or None
            """
            if not (comment is None or type(comment) == six.text_type):
                raise Exception("invalid type {} for comment".format(type(comment)))

            if type(filename) != six.text_type:
                raise Exception("invalid type {} for filename".format(filename))

            with open(filename, 'rb') as f:
                keydata = f.read()

            return cls.from_key_bytes(keydata, comment=comment)

        @util.public
        @classmethod
        def from_ssh_key(cls, filename):
            """
            Load an Ed25519 key from a SSH key file. The key file can be a (private) signing
            key (from a SSH private key file) or a (public) verification key (from a SSH
            public key file). A private key file must be passphrase-less.
            """
            SSH_BEGIN = u'-----BEGIN OPENSSH PRIVATE KEY-----'

            with open(filename, 'r') as f:
                keydata = f.read().strip()

            if keydata.startswith(SSH_BEGIN):
                # OpenSSH private key
                keydata, comment = _read_ssh_ed25519_privkey(keydata)
                key = signing.SigningKey(keydata, encoder=encoding.RawEncoder)
            else:
                # OpenSSH public key
                keydata, comment = _read_ssh_ed25519_pubkey(filename)
                key = public.PublicKey(keydata, encoder=encoding.RawEncoder)

            return cls(key, comment)
