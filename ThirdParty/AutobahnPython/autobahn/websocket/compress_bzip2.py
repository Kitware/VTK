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


__all__ = ["PerMessageBzip2Mixin",
           "PerMessageBzip2Offer",
           "PerMessageBzip2OfferAccept",
           "PerMessageBzip2Response",
           "PerMessageBzip2ResponseAccept",
           "PerMessageBzip2"]


import bz2

from autobahn.websocket.compress_base import PerMessageCompressOffer, \
                                             PerMessageCompressOfferAccept, \
                                             PerMessageCompressResponse, \
                                             PerMessageCompressResponseAccept, \
                                             PerMessageCompress


class PerMessageBzip2Mixin:
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
      ## extension parameter defaults
      ##
      acceptMaxCompressLevel = False
      requestMaxCompressLevel = 0

      ##
      ## verify/parse client ("client-to-server direction") parameters of permessage-bzip2 offer
      ##
      for p in params:

         if len(params[p]) > 1:
            raise Exception("multiple occurence of extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

         val = params[p][0]

         if p == 'client_max_compress_level':
            if val != True:
               raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
            else:
               acceptMaxCompressLevel = True

         elif p == 'server_max_compress_level':
            try:
               val = int(val)
            except:
               raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
            if val not in PerMessageBzip2Mixin.COMPRESS_LEVEL_PERMISSIBLE_VALUES:
               raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
            else:
               requestMaxCompressLevel = val

         else:
            raise Exception("illegal extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

      offer = cls(acceptMaxCompressLevel,
                    requestMaxCompressLevel)
      return offer


   def __init__(self,
                acceptMaxCompressLevel = True,
                requestMaxCompressLevel = 0):
      """
      Constructor.

      :param acceptMaxCompressLevel: Iff true, client accepts "maximum compression level" parameter.
      :type acceptMaxCompressLevel: bool
      :param requestMaxCompressLevel: Iff non-zero, client requests given "maximum compression level" - must be 1-9.
      :type requestMaxCompressLevel: int
      """
      if type(acceptMaxCompressLevel) != bool:
         raise Exception("invalid type %s for acceptMaxCompressLevel" % type(acceptMaxCompressLevel))

      self.acceptMaxCompressLevel = acceptMaxCompressLevel

      if requestMaxCompressLevel != 0 and requestMaxCompressLevel not in self.COMPRESS_LEVEL_PERMISSIBLE_VALUES:
         raise Exception("invalid value %s for requestMaxCompressLevel - permissible values %s" % (requestMaxCompressLevel, self.COMPRESS_LEVEL_PERMISSIBLE_VALUES))

      self.requestMaxCompressLevel = requestMaxCompressLevel


   def getExtensionString(self):
      """
      Returns the WebSocket extension configuration string as sent to the server.

      :returns: str -- PMCE configuration string.
      """
      pmceString = self.EXTENSION_NAME
      if self.acceptMaxCompressLevel:
         pmceString += "; client_max_compress_level"
      if self.requestMaxCompressLevel != 0:
         pmceString += "; server_max_compress_level=%d" % self.requestMaxCompressLevel
      return pmceString


   def __json__(self):
      """
      Returns a JSON serializable object representation.

      :returns: object -- JSON serializable represention.
      """
      return {'extension': self.EXTENSION_NAME,
              'acceptMaxCompressLevel': self.acceptMaxCompressLevel,
              'requestMaxCompressLevel': self.requestMaxCompressLevel}


   def __repr__(self):
      """
      Returns Python object representation that can be eval'ed to reconstruct the object.

      :returns: str -- Python string representation.
      """
      return "PerMessageBzip2Offer(acceptMaxCompressLevel = %s, requestMaxCompressLevel = %s)" % (self.acceptMaxCompressLevel, self.requestMaxCompressLevel)



class PerMessageBzip2OfferAccept(PerMessageCompressOfferAccept, PerMessageBzip2Mixin):
   """
   Set of parameters with which to accept an `permessage-bzip2` offer
   from a client by a server.
   """

   def __init__(self,
                offer,
                requestMaxCompressLevel = 0,
                compressLevel = None):
      """
      Constructor.

      :param offer: The offer being accepted.
      :type offer: Instance of :class:`autobahn.compress.PerMessageBzip2Offer`.
      :param requestMaxCompressLevel: Iff non-zero, server requests given "maximum compression level" - must be 1-9.
      :type requestMaxCompressLevel: int
      :param compressLevel: Override server ("server-to-client direction") compress level (this must be compatible with offer).
      :type compressLevel: int
      """
      if not isinstance(offer, PerMessageBzip2Offer):
         raise Exception("invalid type %s for offer" % type(offer))

      self.offer = offer

      if requestMaxCompressLevel != 0 and requestMaxCompressLevel not in self.COMPRESS_LEVEL_PERMISSIBLE_VALUES:
         raise Exception("invalid value %s for requestMaxCompressLevel - permissible values %s" % (requestMaxCompressLevel, self.COMPRESS_LEVEL_PERMISSIBLE_VALUES))

      if requestMaxCompressLevel != 0 and not offer.acceptMaxCompressLevel:
         raise Exception("invalid value %s for requestMaxCompressLevel - feature unsupported by client" % requestMaxCompressLevel)

      self.requestMaxCompressLevel = requestMaxCompressLevel

      if compressLevel is not None:
         if compressLevel not in self.COMPRESS_LEVEL_PERMISSIBLE_VALUES:
            raise Exception("invalid value %s for compressLevel - permissible values %s" % (compressLevel, self.COMPRESS_LEVEL_PERMISSIBLE_VALUES))

         if offer.requestMaxCompressLevel != 0 and compressLevel > offer.requestMaxCompressLevel:
            raise Exception("invalid value %s for compressLevel - client requested lower maximum value" % compressLevel)

      self.compressLevel = compressLevel


   def getExtensionString(self):
      """
      Returns the WebSocket extension configuration string as sent to the server.

      :returns: str -- PMCE configuration string.
      """
      pmceString = self.EXTENSION_NAME
      if self.offer.requestMaxCompressLevel != 0:
         pmceString += "; server_max_compress_level=%d" % self.offer.requestMaxCompressLevel
      if self.requestMaxCompressLevel != 0:
         pmceString += "; client_max_compress_level=%d" % self.requestMaxCompressLevel
      return pmceString


   def __json__(self):
      """
      Returns a JSON serializable object representation.

      :returns: object -- JSON serializable represention.
      """
      return {'extension': self.EXTENSION_NAME,
              'offer': self.offer.__json__(),
              'requestMaxCompressLevel': self.requestMaxCompressLevel,
              'compressLevel': self.compressLevel}


   def __repr__(self):
      """
      Returns Python object representation that can be eval'ed to reconstruct the object.

      :returns: str -- Python string representation.
      """
      return "PerMessageBzip2Accept(offer = %s, requestMaxCompressLevel = %s, compressLevel = %s)" % (self.offer.__repr__(), self.requestMaxCompressLevel, self.compressLevel)



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

      :returns: object -- A new instance of :class:`autobahn.compress.PerMessageBzip2Response`.
      """
      client_max_compress_level = 0
      server_max_compress_level = 0

      for p in params:

         if len(params[p]) > 1:
            raise Exception("multiple occurence of extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

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

      :returns: object -- JSON serializable represention.
      """
      return {'extension': self.EXTENSION_NAME,
              'client_max_compress_level': self.client_max_compress_level,
              'server_max_compress_level': self.server_max_compress_level}


   def __repr__(self):
      """
      Returns Python object representation that can be eval'ed to reconstruct the object.

      :returns: str -- Python string representation.
      """
      return "PerMessageBzip2Response(client_max_compress_level = %s, server_max_compress_level = %s)" % (self.client_max_compress_level, self.server_max_compress_level)



class PerMessageBzip2ResponseAccept(PerMessageCompressResponseAccept, PerMessageBzip2Mixin):
   """
   Set of parameters with which to accept an `permessage-bzip2` response
   from a server by a client.
   """

   def __init__(self,
                response,
                compressLevel = None):
      """
      Constructor.

      :param response: The response being accepted.
      :type response: Instance of :class:`autobahn.compress.PerMessageBzip2Response`.
      :param compressLevel: Override client ("client-to-server direction") compress level (this must be compatible with response).
      :type compressLevel: int
      """
      if not isinstance(response, PerMessageBzip2Response):
         raise Exception("invalid type %s for response" % type(response))

      self.response = response

      if compressLevel is not None:
         if compressLevel not in self.WINDOW_SIZE_PERMISSIBLE_VALUES:
            raise Exception("invalid value %s for compressLevel - permissible values %s" % (compressLevel, self.COMPRESS_LEVEL_PERMISSIBLE_VALUES))

         if response.client_max_compress_level != 0 and compressLevel > response.client_max_compress_level:
            raise Exception("invalid value %s for compressLevel - server requested lower maximum value" % compressLevel)

      self.compressLevel = compressLevel


   def __json__(self):
      """
      Returns a JSON serializable object representation.

      :returns: object -- JSON serializable represention.
      """
      return {'extension': self.EXTENSION_NAME,
              'response': self.response.__json__(),
              'compressLevel': self.compressLevel}


   def __repr__(self):
      """
      Returns Python object representation that can be eval'ed to reconstruct the object.

      :returns: str -- Python string representation.
      """
      return "PerMessageBzip2ResponseAccept(response = %s, compressLevel = %s)" % (self.response.__repr__(), self.compressLevel)



class PerMessageBzip2(PerMessageCompress, PerMessageBzip2Mixin):
   """
   `permessage-bzip2` WebSocket extension processor.
   """
   DEFAULT_COMPRESS_LEVEL = 9

   @classmethod
   def createFromResponseAccept(cls, isServer, accept):
      pmce = cls(isServer,
                   accept.response.server_max_compress_level,
                   accept.compressLevel if accept.compressLevel is not None else accept.response.client_max_compress_level)
      return pmce


   @classmethod
   def createFromOfferAccept(cls, isServer, accept):
      pmce = cls(isServer,
                   accept.compressLevel if accept.compressLevel is not None else accept.offer.requestMaxCompressLevel,
                   accept.requestMaxCompressLevel)
      return pmce


   def __init__(self,
                isServer,
                server_max_compress_level,
                client_max_compress_level):
      self._isServer = isServer
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


   def startCompressMessage(self):
      if self._isServer:
         if self._compressor is None:
            self._compressor = bz2.BZ2Compressor(self.server_max_compress_level)
      else:
         if self._compressor is None:
            self._compressor = bz2.BZ2Compressor(self.client_max_compress_level)


   def compressMessageData(self, data):
      return self._compressor.compress(data)


   def endCompressMessage(self):
      data = self._compressor.flush()

      ## there seems to be no "flush without close stream", and after
      ## full flush, compressor must not be reused
      self._compressor = None

      return data


   def startDecompressMessage(self):
      if self._decompressor is None:
         self._decompressor = bz2.BZ2Decompressor()


   def decompressMessageData(self, data):
      return self._decompressor.decompress(data)


   def endDecompressMessage(self):
      self._decompressor = None
