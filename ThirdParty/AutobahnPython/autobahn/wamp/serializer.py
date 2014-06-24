###############################################################################
##
##  Copyright (C) 2013-2014 Tavendo GmbH
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

__all__ = ['Serializer',
           'JsonObjectSerializer',
           'JsonSerializer']

import six

from autobahn.wamp.interfaces import IObjectSerializer, ISerializer
from autobahn.wamp.exception import ProtocolError
from autobahn.wamp import message



class Serializer:
   """
   Base class for WAMP serializers. A WAMP serializer is the core glue between
   parsed WAMP message objects and the bytes on wire (the transport).
   """

   MESSAGE_TYPE_MAP = {
      message.Hello.MESSAGE_TYPE:           message.Hello,
      message.Welcome.MESSAGE_TYPE:         message.Welcome,
      message.Abort.MESSAGE_TYPE:           message.Abort,
      message.Challenge.MESSAGE_TYPE:       message.Challenge,
      message.Authenticate.MESSAGE_TYPE:    message.Authenticate,
      message.Goodbye.MESSAGE_TYPE:         message.Goodbye,
      message.Heartbeat.MESSAGE_TYPE:       message.Heartbeat,
      message.Error.MESSAGE_TYPE:           message.Error,

      message.Publish.MESSAGE_TYPE:         message.Publish,
      message.Published.MESSAGE_TYPE:       message.Published,

      message.Subscribe.MESSAGE_TYPE:       message.Subscribe,
      message.Subscribed.MESSAGE_TYPE:      message.Subscribed,
      message.Unsubscribe.MESSAGE_TYPE:     message.Unsubscribe,
      message.Unsubscribed.MESSAGE_TYPE:    message.Unsubscribed,
      message.Event.MESSAGE_TYPE:           message.Event,

      message.Call.MESSAGE_TYPE:            message.Call,
      message.Cancel.MESSAGE_TYPE:          message.Cancel,
      message.Result.MESSAGE_TYPE:          message.Result,

      message.Register.MESSAGE_TYPE:        message.Register,
      message.Registered.MESSAGE_TYPE:      message.Registered,
      message.Unregister.MESSAGE_TYPE:      message.Unregister,
      message.Unregistered.MESSAGE_TYPE:    message.Unregistered,
      message.Invocation.MESSAGE_TYPE:      message.Invocation,
      message.Interrupt.MESSAGE_TYPE:       message.Interrupt,
      message.Yield.MESSAGE_TYPE:           message.Yield
   }
   """
   Mapping of WAMP message type codes to WAMP message classes.
   """


   def __init__(self, serializer):
      """
      Constructor.

      :param serializer: The object serializer to use for WAMP wire-level serialization.
      :type serializer: An object that implements :class:`autobahn.interfaces.IObjectSerializer`.
      """
      self._serializer = serializer


   def serialize(self, msg):
      """
      Implements :func:`autobahn.wamp.interfaces.ISerializer.serialize`
      """
      return msg.serialize(self._serializer), self._serializer.BINARY


   def unserialize(self, payload, isBinary = None):
      """
      Implements :func:`autobahn.wamp.interfaces.ISerializer.unserialize`
      """
      if isBinary is not None:
         if isBinary != self._serializer.BINARY:
            raise ProtocolError("invalid serialization of WAMP message (binary {}, but expected {})".format(isBinary, self._serializer.BINARY))

      try:
         raw_msg = self._serializer.unserialize(payload)
      except Exception as e:
         raise ProtocolError("invalid serialization of WAMP message ({})".format(e))

      if type(raw_msg) != list:
         raise ProtocolError("invalid type {} for WAMP message".format(type(raw_msg)))

      if len(raw_msg) == 0:
         raise ProtocolError("missing message type in WAMP message")

      message_type = raw_msg[0]

      if type(message_type) != int:
         raise ProtocolError("invalid type {} for WAMP message type".format(type(message_type)))

      Klass = self.MESSAGE_TYPE_MAP.get(message_type)

      if Klass is None:
         raise ProtocolError("invalid WAMP message type {}".format(message_type))

      ## this might again raise `ProtocolError` ..
      msg = Klass.parse(raw_msg)

      return msg


##
## JSON serialization is always supported
##
import json

class JsonObjectSerializer:

   BINARY = False

   def serialize(self, obj):
      """
      Implements :func:`autobahn.wamp.interfaces.IObjectSerializer.serialize`
      """
      s = json.dumps(obj, separators = (',',':'))
      if six.PY3:
         return s.encode('utf8')
      else:
         return s


   def unserialize(self, payload):
      """
      Implements :func:`autobahn.wamp.interfaces.IObjectSerializer.unserialize`
      """
      if six.PY3:
         return json.loads(payload.decode('utf8'))
      else:
         return json.loads(payload)



IObjectSerializer.register(JsonObjectSerializer)



class JsonSerializer(Serializer):

   SERIALIZER_ID = "json"

   def __init__(self):
      Serializer.__init__(self, JsonObjectSerializer())



ISerializer.register(JsonSerializer)




##
## MsgPack serialization depends on the `msgpack` package being available
##
try:
   import msgpack
except ImportError:
   pass
else:

   class MsgPackObjectSerializer:

      BINARY = True
      """
      Flag that indicates whether this serializer needs a binary clean transport.
      """

      ENABLE_V5 = True
      """
      Enable version 5 of the MsgPack specification (which differentiates
      between strings and binary).
      """

      def serialize(self, obj):
         """
         Implements :func:`autobahn.wamp.interfaces.IObjectSerializer.serialize`
         """
         return msgpack.packb(obj, use_bin_type = self.ENABLE_V5)


      def unserialize(self, payload):
         """
         Implements :func:`autobahn.wamp.interfaces.IObjectSerializer.unserialize`
         """
         return msgpack.unpackb(payload, encoding = 'utf-8')


   IObjectSerializer.register(MsgPackObjectSerializer)


   __all__.append('MsgPackObjectSerializer')



   class MsgPackSerializer(Serializer):

      SERIALIZER_ID = "msgpack"

      def __init__(self):
         Serializer.__init__(self, MsgPackObjectSerializer())


   ISerializer.register(MsgPackSerializer)


   __all__.append('MsgPackSerializer')
