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


__all__ = ["PerMessageSnappyMixin",
           "PerMessageSnappyOffer",
           "PerMessageSnappyOfferAccept",
           "PerMessageSnappyResponse",
           "PerMessageSnappyResponseAccept",
           "PerMessageSnappy"]


import snappy

from autobahn.websocket.compress_base import PerMessageCompressOffer, \
                                             PerMessageCompressOfferAccept, \
                                             PerMessageCompressResponse, \
                                             PerMessageCompressResponseAccept, \
                                             PerMessageCompress


class PerMessageSnappyMixin:
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

      :returns: object -- A new instance of :class:`autobahn.compress.PerMessageSnappyOffer`.
      """
      ## extension parameter defaults
      ##
      acceptNoContextTakeover = False
      requestNoContextTakeover = False

      ##
      ## verify/parse client ("client-to-server direction") parameters of permessage-snappy offer
      ##
      for p in params:

         if len(params[p]) > 1:
            raise Exception("multiple occurence of extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

         val = params[p][0]

         if p == 'client_no_context_takeover':
            if val != True:
               raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
            else:
               acceptNoContextTakeover = True

         elif p == 'server_no_context_takeover':
            if val != True:
               raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
            else:
               requestNoContextTakeover = True

         else:
            raise Exception("illegal extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

      offer = cls(acceptNoContextTakeover,
                    requestNoContextTakeover)
      return offer


   def __init__(self,
                acceptNoContextTakeover = True,
                requestNoContextTakeover = False):
      """
      Constructor.

      :param acceptNoContextTakeover: Iff true, client accepts "no context takeover" feature.
      :type acceptNoContextTakeover: bool
      :param requestNoContextTakeover: Iff true, client request "no context takeover" feature.
      :type requestNoContextTakeover: bool
      """
      if type(acceptNoContextTakeover) != bool:
         raise Exception("invalid type %s for acceptNoContextTakeover" % type(acceptNoContextTakeover))

      self.acceptNoContextTakeover = acceptNoContextTakeover

      if type(requestNoContextTakeover) != bool:
         raise Exception("invalid type %s for requestNoContextTakeover" % type(requestNoContextTakeover))

      self.requestNoContextTakeover = requestNoContextTakeover


   def getExtensionString(self):
      """
      Returns the WebSocket extension configuration string as sent to the server.

      :returns: str -- PMCE configuration string.
      """
      pmceString = self.EXTENSION_NAME
      if self.acceptNoContextTakeover:
         pmceString += "; client_no_context_takeover"
      if self.requestNoContextTakeover:
         pmceString += "; server_no_context_takeover"
      return pmceString


   def __json__(self):
      """
      Returns a JSON serializable object representation.

      :returns: object -- JSON serializable represention.
      """
      return {'extension': self.EXTENSION_NAME,
              'acceptNoContextTakeover': self.acceptNoContextTakeover,
              'requestNoContextTakeover': self.requestNoContextTakeover}


   def __repr__(self):
      """
      Returns Python object representation that can be eval'ed to reconstruct the object.

      :returns: str -- Python string representation.
      """
      return "PerMessageSnappyOffer(acceptNoContextTakeover = %s, requestNoContextTakeover = %s)" % (self.acceptNoContextTakeover, self.requestNoContextTakeover)



class PerMessageSnappyOfferAccept(PerMessageCompressOfferAccept, PerMessageSnappyMixin):
   """
   Set of parameters with which to accept an `permessage-snappy` offer
   from a client by a server.
   """

   def __init__(self,
                offer,
                requestNoContextTakeover = False,
                noContextTakeover = None):
      """
      Constructor.

      :param offer: The offer being accepted.
      :type offer: Instance of :class:`autobahn.compress.PerMessageSnappyOffer`.
      :param requestNoContextTakeover: Iff true, server request "no context takeover" feature.
      :type requestNoContextTakeover: bool
      :param noContextTakeover: Override server ("server-to-client direction") context takeover (this must be compatible with offer).
      :type noContextTakeover: bool
      """
      if not isinstance(offer, PerMessageSnappyOffer):
         raise Exception("invalid type %s for offer" % type(offer))

      self.offer = offer

      if type(requestNoContextTakeover) != bool:
         raise Exception("invalid type %s for requestNoContextTakeover" % type(requestNoContextTakeover))

      if requestNoContextTakeover and not offer.acceptNoContextTakeover:
         raise Exception("invalid value %s for requestNoContextTakeover - feature unsupported by client" % requestNoContextTakeover)

      self.requestNoContextTakeover = requestNoContextTakeover

      if noContextTakeover is not None:
         if type(noContextTakeover) != bool:
            raise Exception("invalid type %s for noContextTakeover" % type(noContextTakeover))

         if offer.requestNoContextTakeover and not noContextTakeover:
            raise Exception("invalid value %s for noContextTakeover - client requested feature" % noContextTakeover)

      self.noContextTakeover = noContextTakeover


   def getExtensionString(self):
      """
      Returns the WebSocket extension configuration string as sent to the server.

      :returns: str -- PMCE configuration string.
      """
      pmceString = self.EXTENSION_NAME
      if self.offer.requestNoContextTakeover:
         pmceString += "; server_no_context_takeover"
      if self.requestNoContextTakeover:
         pmceString += "; client_no_context_takeover"
      return pmceString


   def __json__(self):
      """
      Returns a JSON serializable object representation.

      :returns: object -- JSON serializable represention.
      """
      return {'extension': self.EXTENSION_NAME,
              'offer': self.offer.__json__(),
              'requestNoContextTakeover': self.requestNoContextTakeover,
              'noContextTakeover': self.noContextTakeover}


   def __repr__(self):
      """
      Returns Python object representation that can be eval'ed to reconstruct the object.

      :returns: str -- Python string representation.
      """
      return "PerMessageSnappyAccept(offer = %s, requestNoContextTakeover = %s, noContextTakeover = %s)" % (self.offer.__repr__(), self.requestNoContextTakeover, self.noContextTakeover)



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

      :returns: object -- A new instance of :class:`autobahn.compress.PerMessageSnappyResponse`.
      """
      client_no_context_takeover = False
      server_no_context_takeover = False

      for p in params:

         if len(params[p]) > 1:
            raise Exception("multiple occurence of extension parameter '%s' for extension '%s'" % (p, cls.EXTENSION_NAME))

         val = params[p][0]

         if p == 'client_no_context_takeover':
            if val != True:
               raise Exception("illegal extension parameter value '%s' for parameter '%s' of extension '%s'" % (val, p, cls.EXTENSION_NAME))
            else:
               client_no_context_takeover = True

         elif p == 'server_no_context_takeover':
            if val != True:
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

      :returns: object -- JSON serializable represention.
      """
      return {'extension': self.EXTENSION_NAME,
              'client_no_context_takeover': self.client_no_context_takeover,
              'server_no_context_takeover': self.server_no_context_takeover}


   def __repr__(self):
      """
      Returns Python object representation that can be eval'ed to reconstruct the object.

      :returns: str -- Python string representation.
      """
      return "PerMessageSnappyResponse(client_no_context_takeover = %s, server_no_context_takeover = %s)" % (self.client_no_context_takeover, self.server_no_context_takeover)



class PerMessageSnappyResponseAccept(PerMessageCompressResponseAccept, PerMessageSnappyMixin):
   """
   Set of parameters with which to accept an `permessage-snappy` response
   from a server by a client.
   """

   def __init__(self,
                response,
                noContextTakeover = None):
      """
      Constructor.

      :param response: The response being accepted.
      :type response: Instance of :class:`autobahn.compress.PerMessageSnappyResponse`.
      :param noContextTakeover: Override client ("client-to-server direction") context takeover (this must be compatible with response).
      :type noContextTakeover: bool
      """
      if not isinstance(response, PerMessageSnappyResponse):
         raise Exception("invalid type %s for response" % type(response))

      self.response = response

      if noContextTakeover is not None:
         if type(noContextTakeover) != bool:
            raise Exception("invalid type %s for noContextTakeover" % type(noContextTakeover))

         if response.client_no_context_takeover and not noContextTakeover:
            raise Exception("invalid value %s for noContextTakeover - server requested feature" % noContextTakeover)

      self.noContextTakeover = noContextTakeover


   def __json__(self):
      """
      Returns a JSON serializable object representation.

      :returns: object -- JSON serializable represention.
      """
      return {'extension': self.EXTENSION_NAME,
              'response': self.response.__json__(),
              'noContextTakeover': self.noContextTakeover}


   def __repr__(self):
      """
      Returns Python object representation that can be eval'ed to reconstruct the object.

      :returns: str -- Python string representation.
      """
      return "PerMessageSnappyResponseAccept(response = %s, noContextTakeover = %s)" % (self.response.__repr__(), self.noContextTakeover)



class PerMessageSnappy(PerMessageCompress, PerMessageSnappyMixin):
   """
   `permessage-snappy` WebSocket extension processor.
   """

   @classmethod
   def createFromResponseAccept(cls, isServer, accept):
      pmce = cls(isServer,
                   accept.response.server_no_context_takeover,
                   accept.noContextTakeover if accept.noContextTakeover is not None else accept.response.client_no_context_takeover)
      return pmce


   @classmethod
   def createFromOfferAccept(cls, isServer, accept):
      pmce = cls(isServer,
                   accept.noContextTakeover if accept.noContextTakeover is not None else accept.offer.requestNoContextTakeover,
                   accept.requestNoContextTakeover)
      return pmce


   def __init__(self,
                isServer,
                server_no_context_takeover,
                client_no_context_takeover):
      self._isServer = isServer
      self.server_no_context_takeover = server_no_context_takeover
      self.client_no_context_takeover = client_no_context_takeover

      self._compressor = None
      self._decompressor = None


   def __json__(self):
      return {'extension': self.EXTENSION_NAME,
              'server_no_context_takeover': self.server_no_context_takeover,
              'client_no_context_takeover': self.client_no_context_takeover}


   def __repr__(self):
      return "PerMessageSnappy(isServer = %s, server_no_context_takeover = %s, client_no_context_takeover = %s)" % (self._isServer, self.server_no_context_takeover, self.client_no_context_takeover)


   def startCompressMessage(self):
      if self._isServer:
         if self._compressor is None or self.server_no_context_takeover:
            self._compressor = snappy.StreamCompressor()
      else:
         if self._compressor is None or self.client_no_context_takeover:
            self._compressor = snappy.StreamCompressor()


   def compressMessageData(self, data):
      return self._compressor.add_chunk(data)


   def endCompressMessage(self):
      return ""


   def startDecompressMessage(self):
      if self._isServer:
         if self._decompressor is None or self.client_no_context_takeover:
            self._decompressor = snappy.StreamDecompressor()
      else:
         if self._decompressor is None or self.server_no_context_takeover:
            self._decompressor = snappy.StreamDecompressor()


   def decompressMessageData(self, data):
      return self._decompressor.decompress(data)


   def endDecompressMessage(self):
      pass
