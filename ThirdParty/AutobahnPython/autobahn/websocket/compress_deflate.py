###############################################################################
##
##  Copyright 2013 Tavendo GmbH
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


__all__ = ["PerMessageDeflateMixin",
           "PerMessageDeflateOffer",
           "PerMessageDeflateOfferAccept",
           "PerMessageDeflateResponse",
           "PerMessageDeflateResponseAccept",
           "PerMessageDeflate"]


import zlib

from autobahn.websocket.compress_base import PerMessageCompressOffer, \
                                             PerMessageCompressOfferAccept, \
                                             PerMessageCompressResponse, \
                                             PerMessageCompressResponseAccept, \
                                             PerMessageCompress


class PerMessageDeflateMixin:
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

      :returns: object -- A new instance of :class:`autobahn.compress.PerMessageDeflateOffer`.
      """

      ## extension parameter defaults
      ##
      acceptMaxWindowBits = False
      acceptNoContextTakeover = True
      #acceptNoContextTakeover = False # FIXME: this may change in draft
      requestMaxWindowBits = 0
      requestNoContextTakeover = False

      ##
      ## verify/parse client ("client-to-server direction") parameters of permessage-deflate offer
      ##
      for p in params:

         if len(params[p]) > 1:
            raise Exception("multiple occurence of extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

         val = params[p][0]

         if p == 'client_max_window_bits':
            if val != True:
               raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
            else:
               acceptMaxWindowBits = True

         elif p == 'client_no_context_takeover':
            if val != True:
               raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
            else:
               acceptNoContextTakeover = True

         elif p == 'server_max_window_bits':
            try:
               val = int(val)
            except:
               raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
            else:
               if val not in PerMessageDeflateMixin.WINDOW_SIZE_PERMISSIBLE_VALUES:
                  raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
               else:
                  requestMaxWindowBits = val

         elif p == 'server_no_context_takeover':
            if val != True:
               raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
            else:
               requestNoContextTakeover = True

         else:
            raise Exception("illegal extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

      offer = cls(acceptNoContextTakeover,
                    acceptMaxWindowBits,
                    requestNoContextTakeover,
                    requestMaxWindowBits)
      return offer


   def __init__(self,
                acceptNoContextTakeover = True,
                acceptMaxWindowBits = True,
                requestNoContextTakeover = False,
                requestMaxWindowBits = 0):
      """
      Constructor.

      :param acceptNoContextTakeover: Iff true, client accepts "no context takeover" feature.
      :type acceptNoContextTakeover: bool
      :param acceptMaxWindowBits: Iff true, client accepts setting "max window size".
      :type acceptMaxWindowBits: bool
      :param requestNoContextTakeover: Iff true, client request "no context takeover" feature.
      :type requestNoContextTakeover: bool
      :param requestMaxWindowBits: Iff non-zero, client requests given "max window size" - must be 8-15.
      :type requestMaxWindowBits: int
      """
      if type(acceptNoContextTakeover) != bool:
         raise Exception("invalid type %s for acceptNoContextTakeover" % type(acceptNoContextTakeover))

      self.acceptNoContextTakeover = acceptNoContextTakeover

      if type(acceptMaxWindowBits) != bool:
         raise Exception("invalid type %s for acceptMaxWindowBits" % type(acceptMaxWindowBits))

      self.acceptMaxWindowBits = acceptMaxWindowBits

      if type(requestNoContextTakeover) != bool:
         raise Exception("invalid type %s for requestNoContextTakeover" % type(requestNoContextTakeover))

      self.requestNoContextTakeover = requestNoContextTakeover

      if requestMaxWindowBits != 0 and requestMaxWindowBits not in self.WINDOW_SIZE_PERMISSIBLE_VALUES:
         raise Exception("invalid value %s for requestMaxWindowBits - permissible values %s" % (requestMaxWindowBits, self.WINDOW_SIZE_PERMISSIBLE_VALUES))

      self.requestMaxWindowBits = requestMaxWindowBits


   def getExtensionString(self):
      """
      Returns the WebSocket extension configuration string as sent to the server.

      :returns: str -- PMCE configuration string.
      """
      pmceString = self.EXTENSION_NAME
      if self.acceptNoContextTakeover:
         pmceString += "; client_no_context_takeover"
      if self.acceptMaxWindowBits:
         pmceString += "; client_max_window_bits"
      if self.requestNoContextTakeover:
         pmceString += "; server_no_context_takeover"
      if self.requestMaxWindowBits != 0:
         pmceString += "; server_max_window_bits=%d" % self.requestMaxWindowBits
      return pmceString


   def __json__(self):
      """
      Returns a JSON serializable object representation.

      :returns: object -- JSON serializable represention.
      """
      return {'extension': self.EXTENSION_NAME,
              'acceptNoContextTakeover': self.acceptNoContextTakeover,
              'acceptMaxWindowBits': self.acceptMaxWindowBits,
              'requestNoContextTakeover': self.requestNoContextTakeover,
              'requestMaxWindowBits': self.requestMaxWindowBits}


   def __repr__(self):
      """
      Returns Python object representation that can be eval'ed to reconstruct the object.

      :returns: str -- Python string representation.
      """
      return "PerMessageDeflateOffer(acceptNoContextTakeover = %s, acceptMaxWindowBits = %s, requestNoContextTakeover = %s, requestMaxWindowBits = %s)" % (self.acceptNoContextTakeover, self.acceptMaxWindowBits, self.requestNoContextTakeover, self.requestMaxWindowBits)



class PerMessageDeflateOfferAccept(PerMessageCompressOfferAccept, PerMessageDeflateMixin):
   """
   Set of parameters with which to accept an `permessage-deflate` offer
   from a client by a server.
   """

   def __init__(self,
                offer,
                requestNoContextTakeover = False,
                requestMaxWindowBits = 0,
                noContextTakeover = None,
                windowBits = None,
                memLevel = None):
      """
      Constructor.

      :param offer: The offer being accepted.
      :type offer: Instance of :class:`autobahn.compress.PerMessageDeflateOffer`.
      :param requestNoContextTakeover: Iff true, server request "no context takeover" feature.
      :type requestNoContextTakeover: bool
      :param requestMaxCompressLevel: Iff non-zero, server requests given "maximum compression level" - must be 1-9.
      :type requestMaxCompressLevel: int
      :param noContextTakeover: Override server ("server-to-client direction") context takeover (this must be compatible with offer).
      :type noContextTakeover: bool
      :param windowBits: Override server ("server-to-client direction") window size (this must be compatible with offer).
      :type windowBits: int
      :param memLevel: Set server ("server-to-client direction") memory level.
      :type memLevel: int
      """
      if not isinstance(offer, PerMessageDeflateOffer):
         raise Exception("invalid type %s for offer" % type(offer))

      self.offer = offer

      if type(requestNoContextTakeover) != bool:
         raise Exception("invalid type %s for requestNoContextTakeover" % type(requestNoContextTakeover))

      if requestNoContextTakeover and not offer.acceptNoContextTakeover:
         raise Exception("invalid value %s for requestNoContextTakeover - feature unsupported by client" % requestNoContextTakeover)

      self.requestNoContextTakeover = requestNoContextTakeover

      if requestMaxWindowBits != 0 and requestMaxWindowBits not in self.WINDOW_SIZE_PERMISSIBLE_VALUES:
         raise Exception("invalid value %s for requestMaxWindowBits - permissible values %s" % (requestMaxWindowBits, self.WINDOW_SIZE_PERMISSIBLE_VALUES))

      if requestMaxWindowBits != 0 and not offer.acceptMaxWindowBits:
         raise Exception("invalid value %s for requestMaxWindowBits - feature unsupported by client" % requestMaxWindowBits)

      self.requestMaxWindowBits = requestMaxWindowBits

      if noContextTakeover is not None:
         if type(noContextTakeover) != bool:
            raise Exception("invalid type %s for noContextTakeover" % type(noContextTakeover))

         if offer.requestNoContextTakeover and not noContextTakeover:
            raise Exception("invalid value %s for noContextTakeover - client requested feature" % noContextTakeover)

      self.noContextTakeover = noContextTakeover

      if windowBits is not None:
         if windowBits not in self.WINDOW_SIZE_PERMISSIBLE_VALUES:
            raise Exception("invalid value %s for windowBits - permissible values %s" % (windowBits, self.WINDOW_SIZE_PERMISSIBLE_VALUES))

         if offer.requestMaxWindowBits != 0 and windowBits > offer.requestMaxWindowBits:
            raise Exception("invalid value %s for windowBits - client requested lower maximum value" % windowBits)

      self.windowBits = windowBits

      if memLevel is not None:
         if memLevel not in self.MEM_LEVEL_PERMISSIBLE_VALUES:
            raise Exception("invalid value %s for memLevel - permissible values %s" % (memLevel, self.MEM_LEVEL_PERMISSIBLE_VALUES))

      self.memLevel = memLevel


   def getExtensionString(self):
      """
      Returns the WebSocket extension configuration string as sent to the server.

      :returns: str -- PMCE configuration string.
      """
      pmceString = self.EXTENSION_NAME
      if self.offer.requestNoContextTakeover:
         pmceString += "; server_no_context_takeover"
      if self.offer.requestMaxWindowBits != 0:
         pmceString += "; server_max_window_bits=%d" % self.offer.requestMaxWindowBits
      if self.requestNoContextTakeover:
         pmceString += "; client_no_context_takeover"
      if self.requestMaxWindowBits != 0:
         pmceString += "; client_max_window_bits=%d" % self.requestMaxWindowBits
      return pmceString


   def __json__(self):
      """
      Returns a JSON serializable object representation.

      :returns: object -- JSON serializable represention.
      """
      return {'extension': self.EXTENSION_NAME,
              'offer': self.offer.__json__(),
              'requestNoContextTakeover': self.requestNoContextTakeover,
              'requestMaxWindowBits': self.requestMaxWindowBits,
              'noContextTakeover': self.noContextTakeover,
              'windowBits': self.windowBits,
              'memLevel': self.memLevel}


   def __repr__(self):
      """
      Returns Python object representation that can be eval'ed to reconstruct the object.

      :returns: str -- Python string representation.
      """
      return "PerMessageDeflateOfferAccept(offer = %s, requestNoContextTakeover = %s, requestMaxWindowBits = %s, noContextTakeover = %s, windowBits = %s, memLevel = %s)" % (self.offer.__repr__(), self.requestNoContextTakeover, self.requestMaxWindowBits, self.noContextTakeover, self.windowBits, self.memLevel)



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

      :returns: object -- A new instance of :class:`autobahn.compress.PerMessageDeflateResponse`.
      """
      client_max_window_bits = 0
      client_no_context_takeover = False
      server_max_window_bits = 0
      server_no_context_takeover = False

      for p in params:

         if len(params[p]) > 1:
            raise Exception("multiple occurence of extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

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
            if val != True:
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
            if val != True:
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
      self.client_max_window_bits = client_max_window_bits
      self.client_no_context_takeover = client_no_context_takeover
      self.server_max_window_bits = server_max_window_bits
      self.server_no_context_takeover = server_no_context_takeover


   def __json__(self):
      """
      Returns a JSON serializable object representation.

      :returns: object -- JSON serializable represention.
      """
      return {'extension': self.EXTENSION_NAME,
              'client_max_window_bits': self.client_max_window_bits,
              'client_no_context_takeover': self.client_no_context_takeover,
              'server_max_window_bits': self.server_max_window_bits,
              'server_no_context_takeover': self.server_no_context_takeover}


   def __repr__(self):
      """
      Returns Python object representation that can be eval'ed to reconstruct the object.

      :returns: str -- Python string representation.
      """
      return "PerMessageDeflateResponse(client_max_window_bits = %s, client_no_context_takeover = %s, server_max_window_bits = %s, server_no_context_takeover = %s)" % (self.client_max_window_bits, self.client_no_context_takeover, self.server_max_window_bits, self.server_no_context_takeover)


class PerMessageDeflateResponseAccept(PerMessageCompressResponseAccept, PerMessageDeflateMixin):
   """
   Set of parameters with which to accept an `permessage-deflate` response
   from a server by a client.
   """

   def __init__(self,
                response,
                noContextTakeover = None,
                windowBits = None,
                memLevel = None):
      """
      Constructor.

      :param response: The response being accepted.
      :type response: Instance of :class:`autobahn.compress.PerMessageDeflateResponse`.
      :param noContextTakeover: Override client ("client-to-server direction") context takeover (this must be compatible with response).
      :type noContextTakeover: bool
      :param windowBits: Override client ("client-to-server direction") window size (this must be compatible with response).
      :type windowBits: int
      :param memLevel: Set client ("client-to-server direction") memory level.
      :type memLevel: int
      """
      if not isinstance(response, PerMessageDeflateResponse):
         raise Exception("invalid type %s for response" % type(response))

      self.response = response

      if noContextTakeover is not None:
         if type(noContextTakeover) != bool:
            raise Exception("invalid type %s for noContextTakeover" % type(noContextTakeover))

         if response.client_no_context_takeover and not noContextTakeover:
            raise Exception("invalid value %s for noContextTakeover - server requested feature" % noContextTakeover)

      self.noContextTakeover = noContextTakeover

      if windowBits is not None:
         if windowBits not in self.WINDOW_SIZE_PERMISSIBLE_VALUES:
            raise Exception("invalid value %s for windowBits - permissible values %s" % (windowBits, self.WINDOW_SIZE_PERMISSIBLE_VALUES))

         if response.client_max_window_bits != 0 and windowBits > response.client_max_window_bits:
            raise Exception("invalid value %s for windowBits - server requested lower maximum value" % windowBits)

      self.windowBits = windowBits

      if memLevel is not None:
         if memLevel not in self.MEM_LEVEL_PERMISSIBLE_VALUES:
            raise Exception("invalid value %s for memLevel - permissible values %s" % (memLevel, self.MEM_LEVEL_PERMISSIBLE_VALUES))

      self.memLevel = memLevel


   def __json__(self):
      """
      Returns a JSON serializable object representation.

      :returns: object -- JSON serializable represention.
      """
      return {'extension': self.EXTENSION_NAME,
              'response': self.response.__json__(),
              'noContextTakeover': self.noContextTakeover,
              'windowBits': self.windowBits,
              'memLevel': self.memLevel}


   def __repr__(self):
      """
      Returns Python object representation that can be eval'ed to reconstruct the object.

      :returns: str -- Python string representation.
      """
      return "PerMessageDeflateResponseAccept(response = %s, noContextTakeover = %s, windowBits = %s, memLevel = %s)" % (self.response.__repr__(), self.noContextTakeover, self.windowBits, self.memLevel)


# noinspection PyArgumentList
class PerMessageDeflate(PerMessageCompress, PerMessageDeflateMixin):
   """
   `permessage-deflate` WebSocket extension processor.
   """
   DEFAULT_WINDOW_BITS = zlib.MAX_WBITS
   DEFAULT_MEM_LEVEL = 8


   @classmethod
   def createFromResponseAccept(cls, isServer, accept):
      ## accept: instance of PerMessageDeflateResponseAccept
      pmce = cls(isServer,
                   accept.response.server_no_context_takeover,
                   accept.noContextTakeover if accept.noContextTakeover is not None else accept.response.client_no_context_takeover,
                   accept.response.server_max_window_bits,
                   accept.windowBits if accept.windowBits is not None else accept.response.client_max_window_bits,
                   accept.memLevel)
      return pmce


   @classmethod
   def createFromOfferAccept(cls, isServer, accept):
      ## accept: instance of PerMessageDeflateOfferAccept
      pmce = cls(isServer,
                   accept.noContextTakeover if accept.noContextTakeover is not None else accept.offer.requestNoContextTakeover,
                   accept.requestNoContextTakeover,
                   accept.windowBits if accept.windowBits is not None else accept.offer.requestMaxWindowBits,
                   accept.requestMaxWindowBits,
                   accept.memLevel)
      return pmce


   def __init__(self,
                isServer,
                server_no_context_takeover,
                client_no_context_takeover,
                server_max_window_bits,
                client_max_window_bits,
                mem_level):
      self._isServer = isServer

      self.server_no_context_takeover = server_no_context_takeover
      self.client_no_context_takeover = client_no_context_takeover

      self.server_max_window_bits = server_max_window_bits if server_max_window_bits != 0 else self.DEFAULT_WINDOW_BITS
      self.client_max_window_bits = client_max_window_bits if client_max_window_bits != 0 else self.DEFAULT_WINDOW_BITS

      self.mem_level = mem_level if mem_level else self.DEFAULT_MEM_LEVEL

      self._compressor = None
      self._decompressor = None


   def __json__(self):
      return {'extension': self.EXTENSION_NAME,
              'isServer': self._isServer,
              'server_no_context_takeover': self.server_no_context_takeover,
              'client_no_context_takeover': self.client_no_context_takeover,
              'server_max_window_bits': self.server_max_window_bits,
              'client_max_window_bits': self.client_max_window_bits,
              'mem_level': self.mem_level}


   def __repr__(self):
      return "PerMessageDeflate(isServer = %s, server_no_context_takeover = %s, client_no_context_takeover = %s, server_max_window_bits = %s, client_max_window_bits = %s, mem_level = %s)" % (self._isServer, self.server_no_context_takeover, self.client_no_context_takeover, self.server_max_window_bits, self.client_max_window_bits, self.mem_level)


   def startCompressMessage(self):
      # compressobj([level[, method[, wbits[, memlevel[, strategy]]]]])
      # http://bugs.python.org/issue19278
      # http://hg.python.org/cpython/rev/c54c8e71b79a
      #
      if self._isServer:
         if self._compressor is None or self.server_no_context_takeover:
            self._compressor = zlib.compressobj(zlib.Z_DEFAULT_COMPRESSION, zlib.DEFLATED, -self.server_max_window_bits, self.mem_level)
      else:
         if self._compressor is None or self.client_no_context_takeover:
            self._compressor = zlib.compressobj(zlib.Z_DEFAULT_COMPRESSION, zlib.DEFLATED, -self.client_max_window_bits, self.mem_level)


   def compressMessageData(self, data):
      return self._compressor.compress(data)


   def endCompressMessage(self):
      data = self._compressor.flush(zlib.Z_SYNC_FLUSH)
      return data[:-4]


   def startDecompressMessage(self):
      if self._isServer:
         if self._decompressor is None or self.client_no_context_takeover:
            self._decompressor = zlib.decompressobj(-self.client_max_window_bits)
      else:
         if self._decompressor is None or self.server_no_context_takeover:
            self._decompressor = zlib.decompressobj(-self.server_max_window_bits)


   def decompressMessageData(self, data):
      return self._decompressor.decompress(data)


   def endDecompressMessage(self):
      ## Eat stripped LEN and NLEN field of a non-compressed block added
      ## for Z_SYNC_FLUSH.
      ##
      self._decompressor.decompress(b'\x00\x00\xff\xff')
