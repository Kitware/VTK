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


__all__= ['WampRawSocketServerProtocol',
          'WampRawSocketClientProtocol',
          'WampRawSocketServerFactory',
          'WampRawSocketClientFactory']

from twisted.python import log
from twisted.internet.protocol import Factory
from twisted.protocols.basic import Int32StringReceiver
from twisted.internet.error import ConnectionDone

from autobahn.wamp.exception import ProtocolError, SerializationError, TransportLost

import binascii



class WampRawSocketProtocol(Int32StringReceiver):
   """
   Base class for WAMP-over-Raw transport mixins.
   """

   def connectionMade(self):
      if self.factory.debug:
         log.msg("WAMP-over-RawSocket connection made")
      try:
         self._session = self.factory._factory()
         self._session.onOpen(self)
      except Exception as e:
         ## Exceptions raised in onOpen are fatal ..
         if self.factory.debug:
            log.msg("ApplicationSession constructor / onOpen raised ({})".format(e))
         self.abort()


   def connectionLost(self, reason):
      if self.factory.debug:
         log.msg("WAMP-over-RawSocket connection lost: reason = '{}'".format(reason))
      try:
         wasClean = isinstance(reason.value, ConnectionDone)
         self._session.onClose(wasClean)
      except Exception as e:
         ## silently ignore exceptions raised here ..
         if self.factory.debug:
            log.msg("ApplicationSession.onClose raised ({})".format(e))
      self._session = None


   def stringReceived(self, payload):
      if self.factory.debug:
         log.msg("RX octets: {}".format(binascii.hexlify(payload)))
      try:
         for msg in self.factory._serializer.unserialize(payload):
            if self.factory.debug:
               log.msg("RX WAMP message: {}".format(msg))
            self._session.onMessage(msg)

      except ProtocolError as e:
         if self.factory.debug:
            log.msg("WAMP Protocol Error ({}) - aborting connection".format(e))
         self.abort()

      except Exception as e:
         if self.factory.debug:
            log.msg("WAMP Internal Error ({}) - aborting connection".format(e))
         self.abort()


   def send(self, msg):
      """
      Implements :func:`autobahn.wamp.interfaces.ITransport.send`
      """
      if self.isOpen():
         if self.factory.debug:
            log.msg("TX WAMP message: {}".format(msg))
         try:
            bytes, _ = self.factory._serializer.serialize(msg)
         except Exception as e:
            ## all exceptions raised from above should be serialization errors ..
            raise SerializationError("Unable to serialize WAMP application payload ({})".format(e))
         else:
            self.sendString(bytes)
            if self.factory.debug:
               log.msg("TX octets: {}".format(binascii.hexlify(bytes)))
      else:
         raise TransportLost()


   def isOpen(self):
      """
      Implements :func:`autobahn.wamp.interfaces.ITransport.isOpen`
      """
      return self._session is not None


   def close(self):
      """
      Implements :func:`autobahn.wamp.interfaces.ITransport.close`
      """
      if self.isOpen():
         self.transport.loseConnection()
      else:
         raise TransportLost()


   def abort(self):
      """
      Implements :func:`autobahn.wamp.interfaces.ITransport.abort`
      """
      if self.isOpen():
         if hasattr(self.transport, 'abortConnection'):
            ## ProcessProtocol lacks abortConnection()
            self.transport.abortConnection()
         else:
            self.transport.loseConnection()
      else:
         raise TransportLost()



class WampRawSocketServerProtocol(WampRawSocketProtocol):
   """
   Mixin for WAMP-over-RawSocket server transports.
   """



class WampRawSocketClientProtocol(WampRawSocketProtocol):
   """
   Mixin for WAMP-over-RawSocket client transports.
   """



class WampRawSocketFactory(Factory):
   """
   Base class for WAMP-over-RawSocket transport factory mixins.
   """

   def __init__(self, factory, serializer, debug = False):
      """
      Ctor.

      :param factory: A callable that produces instances that implement
          :class:`autobahn.wamp.interfaces.ITransportHandler`
      :type factory: callable
      :param serializer: A WAMP serializer to use. A serializer must implement
          :class:`autobahn.wamp.interfaces.ISerializer`.
      :type serializer: list
      """
      assert(callable(factory))
      self._factory = factory
      self._serializer = serializer
      self.debug = debug



class WampRawSocketServerFactory(WampRawSocketFactory):
   """
   Mixin for WAMP-over-RawSocket server transport factories.
   """
   protocol = WampRawSocketServerProtocol



class WampRawSocketClientFactory(WampRawSocketFactory):
   """
   Mixin for WAMP-over-RawSocket client transport factories.
   """
   protocol = WampRawSocketClientProtocol
