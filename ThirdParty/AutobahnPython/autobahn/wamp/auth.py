###############################################################################
##
##  Copyright (C) 2014 Tavendo GmbH
##
##  Licensed under the Apache License, Version 2.0 (the "License");
##  you may not use this file except in compliance with the License.
##  You may obtain a copy of the License at
##
##      http://www.apache.org/licenses/LICENSE-2.0
##
##  Unless required by applicable law or agreed to in writing, software
##  distributed under the License is distributed on an "AS IS" BASIS,
##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
##  See the License for the specific language governing permissions and
##  limitations under the License.
##
###############################################################################

from __future__ import absolute_import

__all__ = ['derive_key',
           'generate_totp_secret',
           'compute_totp',
           'generate_wcs',
           'compute_wcs']

import os
import base64
import hashlib
import hmac
import six
import struct
import time
import binascii



def generate_totp_secret(short = False):
   """
   Generates a new base32 encoded, random secret.

   :param short: If `True`, generate 5 bytes entropy, else 10 bytes.
   :type short: bool

   :returns: bytes -- The generated secret.
   """
   assert(type(short) == bool)
   if short:
      return base64.b32encode(os.urandom(5))
   else:
      return base64.b32encode(os.urandom(10))



def compute_totp(secret, offset = 0):
   """
   Computes the current TOTP code.

   :param secret: Base32 encoded secret.
   :type secret: bytes
   :param offset: Time offset for which to compute TOTP.
   :type offset: int

   :retuns: str -- TOTP for current time (+/- offset).
   """
   assert(type(secret) == six.binary_type)
   assert(type(offset) in six.integer_types)
   try:
      key = base64.b32decode(secret)
   except TypeError:
      raise Exception('invalid secret')
   interval = offset + int(time.time()) // 30
   msg = struct.pack('>Q', interval)
   digest = hmac.new(key, msg, hashlib.sha1).digest()
   o = 15 & (digest[19] if six.PY3 else ord(digest[19]))
   token = (struct.unpack('>I', digest[o:o+4])[0] & 0x7fffffff) % 1000000
   return six.b('{:06d}'.format(token))



## pbkdf2_bin() function taken from https://github.com/mitsuhiko/python-pbkdf2
## Copyright 2011 by Armin Ronacher. Licensed under BSD license.
## @see: https://github.com/mitsuhiko/python-pbkdf2/blob/master/LICENSE
##
import sys
PY3 = sys.version_info >= (3,)


import hmac
import hashlib
import random
from struct import Struct
from operator import xor
from itertools import starmap

if PY3:
   izip = zip
else:
   from itertools import izip, starmap

_pack_int = Struct('>I').pack


def pbkdf2_bin(data, salt, iterations = 1000, keylen = 32, hashfunc = None):
   hashfunc = hashfunc or hashlib.sha256
   mac = hmac.new(data, None, hashfunc)
   def _pseudorandom(x, mac=mac):
      h = mac.copy()
      h.update(x)
      return map(ord, h.digest())
   buf = []
   for block in xrange(1, -(-keylen // mac.digest_size) + 1):
      rv = u = _pseudorandom(salt + _pack_int(block))
      for i in xrange(iterations - 1):
         u = _pseudorandom(''.join(map(chr, u)))
         rv = starmap(xor, izip(rv, u))
      buf.extend(rv)
   return ''.join(map(chr, buf))[:keylen]



def derive_key(secret, salt, iterations = 1000, keylen = 32):
   """
   Computes a derived cryptographic key from a password according to PBKDF2.

   @see: http://en.wikipedia.org/wiki/PBKDF2

   :param secret: The secret.
   :type secret: str
   :param salt: The salt to be used.
   :type salt: str
   :param iterations: Number of iterations of derivation algorithm to run.
   :type iterations: int
   :param keylen: Length of the key to derive in bits.
   :type keylen: int

   :returns: str -- The derived key in Base64 encoding.
   """
   key = pbkdf2_bin(secret, salt, iterations, keylen)
   return binascii.b2a_base64(key).strip()



def generate_wcs(short = False):
   """
   Generates a new random secret string.

   :param short: If `True`, generate string of length 6, else 12
   :type short: bool

   :returns: str -- The generated secret.
   """
   if short:
      l = 6
   else:
      l = 12
   return ''.join([random.choice("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_") for _ in range(l)])



def compute_wcs(key, challenge):
   """
   Compute an WAMP-CRA authentication signature from an authentication
   challenge and a (derived) key.

   :param key: The key derived (via PBKDF2) from the secret.
   :type key: str
   :param challenge: The authentication challenge to sign.
   :type challenge: str

   :returns: str -- The authentication signature.
   """
   sig = hmac.new(key, challenge, hashlib.sha256).digest()
   return binascii.b2a_base64(sig).strip()
