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

__all__ = ["PerMessageCompressOffer",
           "PerMessageCompressOfferAccept",
           "PerMessageCompressResponse",
           "PerMessageCompressResponseAccept",
           "PerMessageCompress"]


class PerMessageCompressOffer:
   """
   Base class for WebSocket compression parameter client offers.
   """
   pass



class PerMessageCompressOfferAccept:
   """
   Base class for WebSocket compression parameter client offer accepts by the server.
   """
   pass



class PerMessageCompressResponse:
   """
   Base class for WebSocket compression parameter server responses.
   """
   pass



class PerMessageCompressResponseAccept:
   """
   Base class for WebSocket compression parameter server response accepts by client.
   """
   pass



class PerMessageCompress:
   """
   Base class for WebSocket compression negotiated parameters.
   """
   pass
