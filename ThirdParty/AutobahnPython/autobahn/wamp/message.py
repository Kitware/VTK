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

__all__ = ('Message',
           'Hello',
           'Welcome',
           'Abort',
           'Challenge',
           'Authenticate',
           'Goodbye',
           'Heartbeat',
           'Error',
           'Publish',
           'Published',
           'Subscribe',
           'Subscribed',
           'Unsubscribe',
           'Unsubscribed',
           'Event',
           'Call',
           'Cancel',
           'Result',
           'Register',
           'Registered',
           'Unregister',
           'Unregistered',
           'Invocation',
           'Interrupt',
           'Yield')


import re
import six

import autobahn
from autobahn import util
from autobahn.wamp.exception import ProtocolError
from autobahn.wamp.interfaces import IMessage
from autobahn.wamp.role import ROLE_NAME_TO_CLASS


## strict URI check allowing empty URI components
_URI_PAT_STRICT = re.compile(r"^(([0-9a-z_]{2,}\.)|\.)*([0-9a-z_]{2,})?$")

## loose URI check allowing empty URI components
_URI_PAT_LOOSE = re.compile(r"^(([^\s\.#]+\.)|\.)*([^\s\.#]+)?$")


## strict URI check disallowing empty URI components
_URI_PAT_STRICT_NON_EMPTY = re.compile(r"^([0-9a-z_]{2,}\.)*([0-9a-z_]{2,})?$")

## loose URI check disallowing empty URI components
_URI_PAT_LOOSE_NON_EMPTY = re.compile(r"^([^\s\.#]+\.)*([^\s\.#]+)?$")



def check_or_raise_uri(value, message):
   if type(value) != six.text_type:
      raise ProtocolError("{0}: invalid type {1} for URI".format(message, type(value)))
   if not _URI_PAT_LOOSE.match(value):
      raise ProtocolError("{0}: invalid value '{1}' for URI".format(message, value))
   return value



def check_or_raise_id(value, message):
   if type(value) not in six.integer_types:
      raise ProtocolError("{0}: invalid type {1} for ID".format(message, type(value)))
   if value < 0 or value > 9007199254740992: # 2**53
      raise ProtocolError("{0}: invalid value {1} for ID".format(message, value))
   return value



def check_or_raise_extra(value, message):
   if type(value) != dict:
      raise ProtocolError("{0}: invalid type {1}".format(message, type(value)))
   for k in value.keys():
      if type(k) != six.text_type:
         raise ProtocolError("{0}: invalid type {1} for key '{2}'".format(message, type(k), k))
   return value



class Message(util.EqualityMixin):
   """
   WAMP message base class. Implements :class:`autobahn.wamp.interfaces.IMessage`.

   .. note:: This is not supposed to be instantiated.
   """

   def __init__(self):
      ## serialization cache: mapping from ISerializer instances to serialized bytes
      self._serialized = {}


   def uncache(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.uncache`
      """
      self._serialized = {}


   def serialize(self, serializer):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.serialize`
      """
      ## only serialize if not cached ..
      if not serializer in self._serialized:
         self._serialized[serializer] = serializer.serialize(self.marshal())
      return self._serialized[serializer]



IMessage.register(Message)



class Hello(Message):
   """
   A WAMP ``HELLO`` message.

   Format: ``[HELLO, Realm|uri, Details|dict]``
   """

   MESSAGE_TYPE = 1
   """
   The WAMP message code for this type of message.
   """


   def __init__(self, realm, roles, authmethods = None, authid = None):
      """

      :param realm: The URI of the WAMP realm to join.
      :type realm: unicode
      :param roles: The WAMP roles to announce.
      :type roles: list of :class:`autobahn.wamp.role.RoleFeatures`
      :param authmethods: The authentication methods to announce.
      :type authmethods: list of unicode or None
      :param authid: The authentication ID to announce.
      :type authid: unicode or None
      """
      assert(type(realm) == six.text_type)
      assert(type(roles) == list)
      for role in roles:
         assert(isinstance(role, autobahn.wamp.role.RoleFeatures))
      if authmethods:
         assert(type(authmethods) == list)
         for authmethod in authmethods:
            assert(type(authmethod) == six.text_type)
      assert(authid is None or type(authid) == six.text_type)

      Message.__init__(self)
      self.realm = realm
      self.roles = roles
      self.authmethods = authmethods
      self.authid = authid


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Hello.MESSAGE_TYPE)

      if len(wmsg) != 3:
         raise ProtocolError("invalid message length {0} for HELLO".format(len(wmsg)))

      realm = check_or_raise_uri(wmsg[1], "'realm' in HELLO")
      details = check_or_raise_extra(wmsg[2], "'details' in HELLO")

      roles = []

      if not u'roles' in details:
         raise ProtocolError("missing mandatory roles attribute in options in HELLO")

      details_roles = check_or_raise_extra(details[u'roles'], "'roles' in 'details' in HELLO")

      if len(details_roles) == 0:
         raise ProtocolError("empty 'roles' in 'details' in HELLO")

      for role in details_roles:
         if role not in ROLE_NAME_TO_CLASS:
            raise ProtocolError("invalid role '{0}' in 'roles' in 'details' in HELLO".format(role))

         role_cls = ROLE_NAME_TO_CLASS[role]

         details_role = check_or_raise_extra(details_roles[role], "role '{0}' in 'roles' in 'details' in HELLO".format(role))

         if u'features' in details_role:
            check_or_raise_extra(details_role[u'features'], "'features' in role '{0}' in 'roles' in 'details' in HELLO".format(role))

            ## FIXME: skip unknown attributes
            role_features = role_cls(**details_role[u'features'])

         else:
            role_features = role_cls()

         roles.append(role_features)

      authmethods = None
      if u'authmethods' in details:
         details_authmethods = details[u'authmethods']
         if type(details_authmethods) != list:
            raise ProtocolError("invalid type {0} for 'authmethods' detail in HELLO".format(type(details_authmethods)))

         for auth_method in details_authmethods:
            if type(auth_method) != six.text_type:
               raise ProtocolError("invalid type {0} for item in 'authmethods' detail in HELLO".format(type(auth_method)))

         authmethods = details_authmethods

      authid = None
      if u'authid' in details:
         details_authid = details[u'authid']
         if type(details_authid) != six.text_type:
            raise ProtocolError("invalid type {0} for 'authid' detail in HELLO".format(type(details_authid)))

         authid = details_authid

      obj = Hello(realm, roles, authmethods, authid)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      details = {u'roles': {}}
      for role in self.roles:
         details[u'roles'][role.ROLE] = {}
         for feature in role.__dict__:
            if not feature.startswith('_') and feature != 'ROLE' and getattr(role, feature) is not None:
               if not u'features' in details[u'roles'][role.ROLE]:
                  details[u'roles'][role.ROLE] = {u'features': {}}
               details[u'roles'][role.ROLE][u'features'][six.u(feature)] = getattr(role, feature)

      if self.authmethods:
         details[u'authmethods'] = self.authmethods

      if self.authid:
         details[u'authid'] = self.authid

      return [Hello.MESSAGE_TYPE, self.realm, details]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP HELLO Message (realm = {0}, roles = {1}, authmethods = {2}, authid = {3})".format(self.realm, self.roles, self.authmethods, self.authid)



class Welcome(Message):
   """
   A WAMP ``WELCOME`` message.

   Format: ``[WELCOME, Session|id, Details|dict]``
   """

   MESSAGE_TYPE = 2
   """
   The WAMP message code for this type of message.
   """


   def __init__(self, session, roles, authid = None, authrole = None, authmethod = None, authprovider = None):
      """

      :param session: The WAMP session ID the other peer is assigned.
      :type session: int
      :param roles: The WAMP roles to announce.
      :type roles: list of :class:`autobahn.wamp.role.RoleFeatures`
      :param authid: The authentication ID assigned.
      :type authid: unicode or None
      :param authrole: The authentication role assigned.
      :type authrole: unicode or None
      :param authmethod: The authentication method in use.
      :type authmethod: unicode or None
      :param authprovider: The authentication method in use.
      :type authprovider: unicode or None
      """
      assert(type(session) in six.integer_types)
      assert(type(roles) == list)
      for role in roles:
         assert(isinstance(role, autobahn.wamp.role.RoleFeatures))
      assert(authid is None or type(authid) == six.text_type)
      assert(authrole is None or type(authrole) == six.text_type)
      assert(authmethod is None or type(authmethod) == six.text_type)
      assert(authprovider is None or type(authprovider) == six.text_type)

      Message.__init__(self)
      self.session = session
      self.roles = roles
      self.authid = authid
      self.authrole = authrole
      self.authmethod = authmethod
      self.authprovider = authprovider


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Welcome.MESSAGE_TYPE)

      if len(wmsg) != 3:
         raise ProtocolError("invalid message length {0} for WELCOME".format(len(wmsg)))

      session = check_or_raise_id(wmsg[1], "'session' in WELCOME")
      details = check_or_raise_extra(wmsg[2], "'details' in WELCOME")

      authid = details.get(u'authid', None)
      authrole = details.get(u'authrole', None)
      authmethod = details.get(u'authmethod', None)
      authprovider = details.get(u'authprovider', None)

      roles = []

      if not u'roles' in details:
         raise ProtocolError("missing mandatory roles attribute in options in WELCOME")

      details_roles = check_or_raise_extra(details['roles'], "'roles' in 'details' in WELCOME")

      if len(details_roles) == 0:
         raise ProtocolError("empty 'roles' in 'details' in WELCOME")

      for role in details_roles:
         if role not in ROLE_NAME_TO_CLASS:
            raise ProtocolError("invalid role '{0}' in 'roles' in 'details' in WELCOME".format(role))

         role_cls = ROLE_NAME_TO_CLASS[role]

         if u'features' in details_roles[role]:
            check_or_raise_extra(details_roles[role][u'features'], "'features' in role '{0}' in 'roles' in 'details' in WELCOME".format(role))

            ## FIXME: skip unknown attributes
            role_features = role_cls(**details_roles[role][u'features'])

         else:
            role_features = role_cls()

         roles.append(role_features)

      obj = Welcome(session, roles, authid, authrole, authmethod, authprovider)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      details = {
         u'roles': {}
      }

      if self.authid:
         details[u'authid'] = self.authid

      if self.authrole:
         details[u'authrole'] = self.authrole

      if self.authrole:
         details[u'authmethod'] = self.authmethod

      if self.authprovider:
         details[u'authprovider'] = self.authprovider

      for role in self.roles:
         details[u'roles'][role.ROLE] = {}
         for feature in role.__dict__:
            if not feature.startswith('_') and feature != 'ROLE' and getattr(role, feature) is not None:
               if not u'features' in details[u'roles'][role.ROLE]:
                  details[u'roles'][role.ROLE] = {u'features': {}}
               details[u'roles'][role.ROLE][u'features'][six.u(feature)] = getattr(role, feature)

      return [Welcome.MESSAGE_TYPE, self.session, details]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP WELCOME Message (session = {0}, roles = {1}, authid = {2}, authrole = {3}, authmethod = {4}, authprovider = {5})".format(self.session, self.roles, self.authid, self.authrole, self.authmethod, self.authprovider)



class Abort(Message):
   """
   A WAMP ``ABORT`` message.

   Format: ``[ABORT, Details|dict, Reason|uri]``
   """

   MESSAGE_TYPE = 3
   """
   The WAMP message code for this type of message.
   """

   def __init__(self, reason, message = None):
      """

      :param reason: WAMP or application error URI for aborting reason.
      :type reason: unicode
      :param message: Optional human-readable closing message, e.g. for logging purposes.
      :type message: unicode or None
      """
      assert(type(reason) == six.text_type)
      assert(message is None or type(message) == six.text_type)

      Message.__init__(self)
      self.reason = reason
      self.message = message


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Abort.MESSAGE_TYPE)

      if len(wmsg) != 3:
         raise ProtocolError("invalid message length {0} for ABORT".format(len(wmsg)))

      details = check_or_raise_extra(wmsg[1], "'details' in ABORT")
      reason = check_or_raise_uri(wmsg[2], "'reason' in ABORT")

      message = None

      if u'message' in details:

         details_message = details[u'message']
         if type(details_message) != six.text_type:
            raise ProtocolError("invalid type {0} for 'message' detail in ABORT".format(type(details_message)))

         message = details_message

      obj = Abort(reason, message)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      details = {}
      if self.message:
         details[u'message'] = self.message

      return [Abort.MESSAGE_TYPE, details, self.reason]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP ABORT Message (message = {0}, reason = {1})".format(self.message, self.reason)



class Challenge(Message):
   """
   A WAMP ``CHALLENGE`` message.

   Format: ``[CHALLENGE, Method|string, Extra|dict]``
   """

   MESSAGE_TYPE = 4
   """
   The WAMP message code for this type of message.
   """


   def __init__(self, method, extra = None):
      """

      :param method: The authentication method.
      :type method: unicode
      :param extra: Authentication method specific information.
      :type extra: dict or None
      """
      assert(type(method) == six.text_type)
      assert(extra is None or type(extra) == dict)

      Message.__init__(self)
      self.method = method
      self.extra = extra or {}


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Challenge.MESSAGE_TYPE)

      if len(wmsg) != 3:
         raise ProtocolError("invalid message length {0} for CHALLENGE".format(len(wmsg)))

      method = wmsg[1]
      if type(method) != six.text_type:
         raise ProtocolError("invalid type {0} for 'method' in CHALLENGE".format(type(method)))

      extra = check_or_raise_extra(wmsg[2], "'extra' in CHALLENGE")

      obj = Challenge(method, extra)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      return [Challenge.MESSAGE_TYPE, self.method, self.extra]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP CHALLENGE Message (method = {0}, extra = {1})".format(self.method, self.extra)



class Authenticate(Message):
   """
   A WAMP ``AUTHENTICATE`` message.

   Format: ``[AUTHENTICATE, Signature|string, Extra|dict]``
   """

   MESSAGE_TYPE = 5
   """
   The WAMP message code for this type of message.
   """


   def __init__(self, signature, extra = None):
      """

      :param signature: The signature for the authentication challenge.
      :type signature: unicode
      :param extra: Authentication method specific information.
      :type extra: dict or None
      """
      assert(type(signature) == six.text_type)
      assert(extra is None or type(extra) == dict)

      Message.__init__(self)
      self.signature = signature
      self.extra = extra or {}


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Authenticate.MESSAGE_TYPE)

      if len(wmsg) != 3:
         raise ProtocolError("invalid message length {0} for AUTHENTICATE".format(len(wmsg)))

      signature = wmsg[1]
      if type(signature) != six.text_type:
         raise ProtocolError("invalid type {0} for 'signature' in AUTHENTICATE".format(type(signature)))

      extra = check_or_raise_extra(wmsg[2], "'extra' in AUTHENTICATE")

      obj = Authenticate(signature, extra)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      return [Authenticate.MESSAGE_TYPE, self.signature, self.extra]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP AUTHENTICATE Message (signature = {0}, extra = {1})".format(self.signature, self.extra)



class Goodbye(Message):
   """
   A WAMP ``GOODBYE`` message.

   Format: ``[GOODBYE, Details|dict, Reason|uri]``
   """

   MESSAGE_TYPE = 6
   """
   The WAMP message code for this type of message.
   """

   DEFAULT_REASON = u"wamp.goodbye.normal"
   """
   Default WAMP closing reason.
   """


   def __init__(self, reason = DEFAULT_REASON, message = None):
      """

      :param reason: Optional WAMP or application error URI for closing reason.
      :type reason: unicode
      :param message: Optional human-readable closing message, e.g. for logging purposes.
      :type message: unicode or None
      """
      assert(type(reason) == six.text_type)
      assert(message is None or type(message) == six.text_type)

      Message.__init__(self)
      self.reason = reason
      self.message = message


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Goodbye.MESSAGE_TYPE)

      if len(wmsg) != 3:
         raise ProtocolError("invalid message length {0} for GOODBYE".format(len(wmsg)))

      details = check_or_raise_extra(wmsg[1], "'details' in GOODBYE")
      reason = check_or_raise_uri(wmsg[2], "'reason' in GOODBYE")

      message = None

      if u'message' in details:

         details_message = details[u'message']
         if type(details_message) != six.text_type:
            raise ProtocolError("invalid type {0} for 'message' detail in GOODBYE".format(type(details_message)))

         message = details_message

      obj = Goodbye(reason, message)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      details = {}
      if self.message:
         details[u'message'] = self.message

      return [Goodbye.MESSAGE_TYPE, details, self.reason]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP GOODBYE Message (message = {0}, reason = {1})".format(self.message, self.reason)



class Heartbeat(Message):
   """
   A WAMP ``HEARTBEAT`` message.

   Formats:

   * ``[HEARTBEAT, Incoming|integer, Outgoing|integer]``
   * ``[HEARTBEAT, Incoming|integer, Outgoing|integer, Discard|string]``
   """

   MESSAGE_TYPE = 7
   """
   The WAMP message code for this type of message.
   """


   def __init__(self, incoming, outgoing, discard = None):
      """

      :param incoming: Last incoming heartbeat processed from peer.
      :type incoming: int
      :param outgoing: Outgoing heartbeat.
      :type outgoing: int
      :param discard: Optional data that is discarded by peer.
      :type discard: unicode or None
      """
      assert(type(incoming) in six.integer_types)
      assert(type(outgoing) in six.integer_types)
      assert(discard is None or type(discard) == six.text_type)

      Message.__init__(self)
      self.incoming = incoming
      self.outgoing = outgoing
      self.discard = discard


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Heartbeat.MESSAGE_TYPE)

      if len(wmsg) not in [3, 4]:
         raise ProtocolError("invalid message length {0} for HEARTBEAT".format(len(wmsg)))

      incoming = wmsg[1]

      if type(incoming) not in six.integer_types:
         raise ProtocolError("invalid type {0} for 'incoming' in HEARTBEAT".format(type(incoming)))

      if incoming < 0: # must be non-negative
         raise ProtocolError("invalid value {0} for 'incoming' in HEARTBEAT".format(incoming))

      outgoing = wmsg[2]

      if type(outgoing) not in six.integer_types:
         raise ProtocolError("invalid type {0} for 'outgoing' in HEARTBEAT".format(type(outgoing)))

      if outgoing <= 0: # must be positive
         raise ProtocolError("invalid value {0} for 'outgoing' in HEARTBEAT".format(outgoing))

      discard = None
      if len(wmsg) > 3:
         discard = wmsg[3]
         if type(discard) != six.text_type:
            raise ProtocolError("invalid type {0} for 'discard' in HEARTBEAT".format(type(discard)))

      obj = Heartbeat(incoming, outgoing, discard = discard)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      if self.discard:
         return [Heartbeat.MESSAGE_TYPE, self.incoming, self.outgoing, self.discard]
      else:
         return [Heartbeat.MESSAGE_TYPE, self.incoming, self.outgoing]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP HEARTBEAT Message (incoming {0}, outgoing = {1}, len(discard) = {2})".format(self.incoming, self.outgoing, len(self.discard) if self.discard else None)



class Error(Message):
   """
   A WAMP ``ERROR`` message.

   Formats:

   * ``[ERROR, REQUEST.Type|int, REQUEST.Request|id, Details|dict, Error|uri]``
   * ``[ERROR, REQUEST.Type|int, REQUEST.Request|id, Details|dict, Error|uri, Arguments|list]``
   * ``[ERROR, REQUEST.Type|int, REQUEST.Request|id, Details|dict, Error|uri, Arguments|list, ArgumentsKw|dict]``
   """

   MESSAGE_TYPE = 8
   """
   The WAMP message code for this type of message.
   """


   def __init__(self, request_type, request, error, args = None, kwargs = None):
      """

      :param request_type: The WAMP message type code for the original request.
      :type request_type: int
      :param request: The WAMP request ID of the original request (`Call`, `Subscribe`, ...) this error occurred for.
      :type request: int
      :param error: The WAMP or application error URI for the error that occurred.
      :type error: unicode
      :param args: Positional values for application-defined exception.
         Must be serializable using any serializers in use.
      :type args: list or None
      :param kwargs: Keyword values for application-defined exception.
         Must be serializable using any serializers in use.
      :type kwargs: dict or None
      """
      assert(type(request_type) in six.integer_types)
      assert(type(request) in six.integer_types)
      assert(type(error) == six.text_type)
      assert(args is None or type(args) in [list, tuple])
      assert(kwargs is None or type(kwargs) == dict)

      Message.__init__(self)
      self.request_type = request_type
      self.request = request
      self.error = error
      self.args = args
      self.kwargs = kwargs


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Error.MESSAGE_TYPE)

      if len(wmsg) not in (5, 6, 7):
         raise ProtocolError("invalid message length {0} for ERROR".format(len(wmsg)))

      request_type = wmsg[1]
      if type(request_type) not in six.integer_types:
         raise ProtocolError("invalid type {0} for 'request_type' in ERROR".format(request_type))

      if request_type not in [Subscribe.MESSAGE_TYPE,
                              Unsubscribe.MESSAGE_TYPE,
                              Publish.MESSAGE_TYPE,
                              Register.MESSAGE_TYPE,
                              Unregister.MESSAGE_TYPE,
                              Call.MESSAGE_TYPE,
                              Invocation.MESSAGE_TYPE]:
         raise ProtocolError("invalid value {0} for 'request_type' in ERROR".format(request_type))

      request = check_or_raise_id(wmsg[2], "'request' in ERROR")
      _ = check_or_raise_extra(wmsg[3], "'details' in ERROR")
      error = check_or_raise_uri(wmsg[4], "'error' in ERROR")

      args = None
      if len(wmsg) > 5:
         args = wmsg[5]
         if type(args) != list:
            raise ProtocolError("invalid type {0} for 'args' in ERROR".format(type(args)))

      kwargs = None
      if len(wmsg) > 6:
         kwargs = wmsg[6]
         if type(kwargs) != dict:
            raise ProtocolError("invalid type {0} for 'kwargs' in ERROR".format(type(kwargs)))

      obj = Error(request_type, request, error, args = args, kwargs = kwargs)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      details = {}

      if self.kwargs:
         return [self.MESSAGE_TYPE, self.request_type, self.request, details, self.error, self.args, self.kwargs]
      elif self.args:
         return [self.MESSAGE_TYPE, self.request_type, self.request, details, self.error, self.args]
      else:
         return [self.MESSAGE_TYPE, self.request_type, self.request, details, self.error]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP Error Message (request_type = {0}, request = {1}, error = {2}, args = {3}, kwargs = {4})".format(self.request_type, self.request, self.error, self.args, self.kwargs)



class Publish(Message):
   """
   A WAMP ``PUBLISH`` message.

   Formats:

   * ``[PUBLISH, Request|id, Options|dict, Topic|uri]``
   * ``[PUBLISH, Request|id, Options|dict, Topic|uri, Arguments|list]``
   * ``[PUBLISH, Request|id, Options|dict, Topic|uri, Arguments|list, ArgumentsKw|dict]``
   """

   MESSAGE_TYPE = 16
   """
   The WAMP message code for this type of message.
   """

   def __init__(self,
                request,
                topic,
                args = None,
                kwargs = None,
                acknowledge = None,
                excludeMe = None,
                exclude = None,
                eligible = None,
                discloseMe = None):
      """

      :param request: The WAMP request ID of this request.
      :type request: int
      :param topic: The WAMP or application URI of the PubSub topic the event should
         be published to.
      :type topic: unicode
      :param args: Positional values for application-defined event payload.
         Must be serializable using any serializers in use.
      :type args: list or tuple or None
      :param kwargs: Keyword values for application-defined event payload.
         Must be serializable using any serializers in use.
      :type kwargs: dict or None
      :param acknowledge: If True, acknowledge the publication with a success or
         error response.
      :type acknowledge: bool or None
      :param excludeMe: If ``True``, exclude the publisher from receiving the event, even
         if he is subscribed (and eligible).
      :type excludeMe: bool or None
      :param exclude: List of WAMP session IDs to exclude from receiving this event.
      :type exclude: list of int or None
      :param eligible: List of WAMP session IDs eligible to receive this event.
      :type eligible: list of int or None
      :param discloseMe: If True, request to disclose the publisher of this event
         to subscribers.
      :type discloseMe: bool or None
      """
      assert(type(request) in six.integer_types)
      assert(type(topic) == six.text_type)
      assert(args is None or type(args) in [list, tuple])
      assert(kwargs is None or type(kwargs) == dict)
      assert(acknowledge is None or type(acknowledge) == bool)
      assert(excludeMe is None or type(excludeMe) == bool)
      assert(exclude is None or type(exclude) == list)
      assert(eligible is None or type(eligible) == list)
      assert(discloseMe is None or type(discloseMe) == bool)

      Message.__init__(self)
      self.request = request
      self.topic = topic
      self.args = args
      self.kwargs = kwargs
      self.acknowledge = acknowledge
      self.excludeMe = excludeMe
      self.exclude = exclude
      self.eligible = eligible
      self.discloseMe = discloseMe


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Publish.MESSAGE_TYPE)

      if len(wmsg) not in (4, 5, 6):
         raise ProtocolError("invalid message length {0} for PUBLISH".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in PUBLISH")
      options = check_or_raise_extra(wmsg[2], "'options' in PUBLISH")
      topic = check_or_raise_uri(wmsg[3], "'topic' in PUBLISH")

      args = None
      if len(wmsg) > 4:
         args = wmsg[4]
         if type(args) != list:
            raise ProtocolError("invalid type {0} for 'args' in PUBLISH".format(type(args)))

      kwargs = None
      if len(wmsg) > 5:
         kwargs = wmsg[5]
         if type(kwargs) != dict:
            raise ProtocolError("invalid type {0} for 'kwargs' in PUBLISH".format(type(kwargs)))

      acknowledge = None
      excludeMe = None
      exclude = None
      eligible = None
      discloseMe = None

      if u'acknowledge' in options:

         option_acknowledge = options[u'acknowledge']
         if type(option_acknowledge) != bool:
            raise ProtocolError("invalid type {0} for 'acknowledge' option in PUBLISH".format(type(option_acknowledge)))

         acknowledge = option_acknowledge

      if u'exclude_me' in options:

         option_excludeMe = options[u'exclude_me']
         if type(option_excludeMe) != bool:
            raise ProtocolError("invalid type {0} for 'exclude_me' option in PUBLISH".format(type(option_excludeMe)))

         excludeMe = option_excludeMe

      if u'exclude' in options:

         option_exclude = options[u'exclude']
         if type(option_exclude) != list:
            raise ProtocolError("invalid type {0} for 'exclude' option in PUBLISH".format(type(option_exclude)))

         for sessionId in option_exclude:
            if type(sessionId) not in six.integer_types:
               raise ProtocolError("invalid type {0} for value in 'exclude' option in PUBLISH".format(type(sessionId)))

         exclude = option_exclude

      if u'eligible' in options:

         option_eligible = options[u'eligible']
         if type(option_eligible) != list:
            raise ProtocolError("invalid type {0} for 'eligible' option in PUBLISH".format(type(option_eligible)))

         for sessionId in option_eligible:
            if type(sessionId) not in six.integer_types:
               raise ProtocolError("invalid type {0} for value in 'eligible' option in PUBLISH".format(type(sessionId)))

         eligible = option_eligible

      if u'disclose_me' in options:

         option_discloseMe = options[u'disclose_me']
         if type(option_discloseMe) != bool:
            raise ProtocolError("invalid type {0} for 'disclose_me' option in PUBLISH".format(type(option_discloseMe)))

         discloseMe = option_discloseMe

      obj = Publish(request,
                    topic,
                    args = args,
                    kwargs = kwargs,
                    acknowledge = acknowledge,
                    excludeMe = excludeMe,
                    exclude = exclude,
                    eligible = eligible,
                    discloseMe = discloseMe)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      options = {}

      if self.acknowledge is not None:
         options[u'acknowledge'] = self.acknowledge
      if self.excludeMe is not None:
         options[u'exclude_me'] = self.excludeMe
      if self.exclude is not None:
         options[u'exclude'] = self.exclude
      if self.eligible is not None:
         options[u'eligible'] = self.eligible
      if self.discloseMe is not None:
         options[u'disclose_me'] = self.discloseMe

      if self.kwargs:
         return [Publish.MESSAGE_TYPE, self.request, options, self.topic, self.args, self.kwargs]
      elif self.args:
         return [Publish.MESSAGE_TYPE, self.request, options, self.topic, self.args]
      else:
         return [Publish.MESSAGE_TYPE, self.request, options, self.topic]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP PUBLISH Message (request = {0}, topic = {1}, args = {2}, kwargs = {3}, acknowledge = {4}, excludeMe = {5}, exclude = {6}, eligible = {7}, discloseMe = {8})".format(self.request, self.topic, self.args, self.kwargs, self.acknowledge, self.excludeMe, self.exclude, self.eligible, self.discloseMe)



class Published(Message):
   """
   A WAMP ``PUBLISHED`` message.

   Format: ``[PUBLISHED, PUBLISH.Request|id, Publication|id]``
   """

   MESSAGE_TYPE = 17
   """
   The WAMP message code for this type of message.
   """

   def __init__(self, request, publication):
      """

      :param request: The request ID of the original `PUBLISH` request.
      :type request: int
      :param publication: The publication ID for the published event.
      :type publication: int
      """
      assert(type(request) in six.integer_types)
      assert(type(publication) in six.integer_types)

      Message.__init__(self)
      self.request = request
      self.publication = publication


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Published.MESSAGE_TYPE)

      if len(wmsg) != 3:
         raise ProtocolError("invalid message length {0} for PUBLISHED".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in PUBLISHED")
      publication = check_or_raise_id(wmsg[2], "'publication' in PUBLISHED")

      obj = Published(request, publication)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      return [Published.MESSAGE_TYPE, self.request, self.publication]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP PUBLISHED Message (request = {0}, publication = {1})".format(self.request, self.publication)



class Subscribe(Message):
   """
   A WAMP ``SUBSCRIBE`` message.

   Format: ``[SUBSCRIBE, Request|id, Options|dict, Topic|uri]``
   """

   MESSAGE_TYPE = 32
   """
   The WAMP message code for this type of message.
   """

   MATCH_EXACT = u'exact'
   MATCH_PREFIX = u'prefix'
   MATCH_WILDCARD = u'wildcard'

   def __init__(self, request, topic, match = MATCH_EXACT):
      """

      :param request: The WAMP request ID of this request.
      :type request: int
      :param topic: The WAMP or application URI of the PubSub topic to subscribe to.
      :type topic: unicode
      :param match: The topic matching method to be used for the subscription.
      :type match: unicode
      """
      assert(type(request) in six.integer_types)
      assert(type(topic) == six.text_type)
      assert(match is None or type(match) == six.text_type)
      assert(match is None or match in [self.MATCH_EXACT, self.MATCH_PREFIX, self.MATCH_WILDCARD])

      Message.__init__(self)
      self.request = request
      self.topic = topic
      self.match = match


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Subscribe.MESSAGE_TYPE)

      if len(wmsg) != 4:
         raise ProtocolError("invalid message length {0} for SUBSCRIBE".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in SUBSCRIBE")
      options = check_or_raise_extra(wmsg[2], "'options' in SUBSCRIBE")
      topic = check_or_raise_uri(wmsg[3], "'topic' in SUBSCRIBE")

      match = Subscribe.MATCH_EXACT

      if u'match' in options:

         option_match = options[u'match']
         if type(option_match) != six.text_type:
            raise ProtocolError("invalid type {0} for 'match' option in SUBSCRIBE".format(type(option_match)))

         if option_match not in [Subscribe.MATCH_EXACT, Subscribe.MATCH_PREFIX, Subscribe.MATCH_WILDCARD]:
            raise ProtocolError("invalid value {0} for 'match' option in SUBSCRIBE".format(option_match))

         match = option_match

      obj = Subscribe(request, topic, match)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      options = {}

      if self.match and self.match != Subscribe.MATCH_EXACT:
         options[u'match'] = self.match

      return [Subscribe.MESSAGE_TYPE, self.request, options, self.topic]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP SUBSCRIBE Message (request = {0}, topic = {1}, match = {2})".format(self.request, self.topic, self.match)



class Subscribed(Message):
   """
   A WAMP ``SUBSCRIBED`` message.

   Format: ``[SUBSCRIBED, SUBSCRIBE.Request|id, Subscription|id]``
   """

   MESSAGE_TYPE = 33
   """
   The WAMP message code for this type of message.
   """

   def __init__(self, request, subscription):
      """

      :param request: The request ID of the original ``SUBSCRIBE`` request.
      :type request: int
      :param subscription: The subscription ID for the subscribed topic (or topic pattern).
      :type subscription: int
      """
      assert(type(request) in six.integer_types)
      assert(type(subscription) in six.integer_types)

      Message.__init__(self)
      self.request = request
      self.subscription = subscription


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Subscribed.MESSAGE_TYPE)

      if len(wmsg) != 3:
         raise ProtocolError("invalid message length {0} for SUBSCRIBED".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in SUBSCRIBED")
      subscription = check_or_raise_id(wmsg[2], "'subscription' in SUBSCRIBED")

      obj = Subscribed(request, subscription)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      return [Subscribed.MESSAGE_TYPE, self.request, self.subscription]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP SUBSCRIBED Message (request = {0}, subscription = {1})".format(self.request, self.subscription)



class Unsubscribe(Message):
   """
   A WAMP ``UNSUBSCRIBE`` message.

   Format: ``[UNSUBSCRIBE, Request|id, SUBSCRIBED.Subscription|id]``
   """

   MESSAGE_TYPE = 34
   """
   The WAMP message code for this type of message.
   """


   def __init__(self, request, subscription):
      """

      :param request: The WAMP request ID of this request.
      :type request: int
      :param subscription: The subscription ID for the subscription to unsubscribe from.
      :type subscription: int
      """
      assert(type(request) in six.integer_types)
      assert(type(subscription) in six.integer_types)

      Message.__init__(self)
      self.request = request
      self.subscription = subscription


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Unsubscribe.MESSAGE_TYPE)

      if len(wmsg) != 3:
         raise ProtocolError("invalid message length {0} for WAMP UNSUBSCRIBE".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in UNSUBSCRIBE")
      subscription = check_or_raise_id(wmsg[2], "'subscription' in UNSUBSCRIBE")

      obj = Unsubscribe(request, subscription)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      return [Unsubscribe.MESSAGE_TYPE, self.request, self.subscription]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP UNSUBSCRIBE Message (request = {0}, subscription = {1})".format(self.request, self.subscription)



class Unsubscribed(Message):
   """
   A WAMP ``UNSUBSCRIBED`` message.

   Format: ``[UNSUBSCRIBED, UNSUBSCRIBE.Request|id]``
   """

   MESSAGE_TYPE = 35
   """
   The WAMP message code for this type of message.
   """

   def __init__(self, request):
      """

      :param request: The request ID of the original ``UNSUBSCRIBE`` request.
      :type request: int
      """
      assert(type(request) in six.integer_types)

      Message.__init__(self)
      self.request = request


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Unsubscribed.MESSAGE_TYPE)

      if len(wmsg) != 2:
         raise ProtocolError("invalid message length {0} for UNSUBSCRIBED".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in UNSUBSCRIBED")

      obj = Unsubscribed(request)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      return [Unsubscribed.MESSAGE_TYPE, self.request]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP UNSUBSCRIBED Message (request = {0})".format(self.request)



class Event(Message):
   """
   A WAMP ``EVENT`` message.

   Formats:

   * ``[EVENT, SUBSCRIBED.Subscription|id, PUBLISHED.Publication|id, Details|dict]``
   * ``[EVENT, SUBSCRIBED.Subscription|id, PUBLISHED.Publication|id, Details|dict, PUBLISH.Arguments|list]``
   * ``[EVENT, SUBSCRIBED.Subscription|id, PUBLISHED.Publication|id, Details|dict, PUBLISH.Arguments|list, PUBLISH.ArgumentsKw|dict]``
   """

   MESSAGE_TYPE = 36
   """
   The WAMP message code for this type of message.
   """


   def __init__(self, subscription, publication, args = None, kwargs = None, publisher = None):
      """

      :param subscription: The subscription ID this event is dispatched under.
      :type subscription: int
      :param publication: The publication ID of the dispatched event.
      :type publication: int
      :param args: Positional values for application-defined exception.
         Must be serializable using any serializers in use.
      :type args: list or tuple or None
      :param kwargs: Keyword values for application-defined exception.
         Must be serializable using any serializers in use.
      :type kwargs: dict or None
      :param publisher: If present, the WAMP session ID of the publisher of this event.
      :type publisher: int or None
      """
      assert(type(subscription) in six.integer_types)
      assert(type(publication) in six.integer_types)
      assert(args is None or type(args) in [list, tuple])
      assert(kwargs is None or type(kwargs) == dict)
      assert(publisher is None or type(publisher) in six.integer_types)

      Message.__init__(self)
      self.subscription = subscription
      self.publication = publication
      self.args = args
      self.kwargs = kwargs
      self.publisher = publisher


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Event.MESSAGE_TYPE)

      if len(wmsg) not in (4, 5, 6):
         raise ProtocolError("invalid message length {0} for EVENT".format(len(wmsg)))

      subscription = check_or_raise_id(wmsg[1], "'subscription' in EVENT")
      publication = check_or_raise_id(wmsg[2], "'publication' in EVENT")
      details = check_or_raise_extra(wmsg[3], "'details' in EVENT")

      args = None
      if len(wmsg) > 4:
         args = wmsg[4]
         if type(args) != list:
            raise ProtocolError("invalid type {0} for 'args' in EVENT".format(type(args)))

      kwargs = None
      if len(wmsg) > 5:
         kwargs = wmsg[5]
         if type(kwargs) != dict:
            raise ProtocolError("invalid type {0} for 'kwargs' in EVENT".format(type(kwargs)))

      publisher = None
      if u'publisher' in details:

         detail_publisher = details[u'publisher']
         if type(detail_publisher) not in six.integer_types:
            raise ProtocolError("invalid type {0} for 'publisher' detail in EVENT".format(type(detail_publisher)))

         publisher = detail_publisher

      obj = Event(subscription,
                  publication,
                  args = args,
                  kwargs = kwargs,
                  publisher = publisher)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      details = {}

      if self.publisher is not None:
         details[u'publisher'] = self.publisher

      if self.kwargs:
         return [Event.MESSAGE_TYPE, self.subscription, self.publication, details, self.args, self.kwargs]
      elif self.args:
         return [Event.MESSAGE_TYPE, self.subscription, self.publication, details, self.args]
      else:
         return [Event.MESSAGE_TYPE, self.subscription, self.publication, details]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP EVENT Message (subscription = {0}, publication = {1}, args = {2}, kwargs = {3}, publisher = {4})".format(self.subscription, self.publication, self.args, self.kwargs, self.publisher)



class Call(Message):
   """
   A WAMP ``CALL`` message.

   Formats:

   * ``[CALL, Request|id, Options|dict, Procedure|uri]``
   * ``[CALL, Request|id, Options|dict, Procedure|uri, Arguments|list]``
   * ``[CALL, Request|id, Options|dict, Procedure|uri, Arguments|list, ArgumentsKw|dict]``
   """

   MESSAGE_TYPE = 48
   """
   The WAMP message code for this type of message.
   """

   def __init__(self,
                request,
                procedure,
                args = None,
                kwargs = None,
                timeout = None,
                receive_progress = None,
                discloseMe = None):
      """

      :param request: The WAMP request ID of this request.
      :type request: int
      :param procedure: The WAMP or application URI of the procedure which should be called.
      :type procedure: unicode
      :param args: Positional values for application-defined call arguments.
         Must be serializable using any serializers in use.
      :type args: list or tuple or None
      :param kwargs: Keyword values for application-defined call arguments.
         Must be serializable using any serializers in use.
      :type kwargs: dict or None
      :param timeout: If present, let the callee automatically cancel
         the call after this ms.
      :type timeout: int or None
      :param receive_progress: If ``True``, indicates that the caller wants to receive
         progressive call results.
      :type receive_progress: bool or None
      :param discloseMe: If ``True``, the caller requests to disclose itself to the callee.
      :type discloseMe: bool or None
      """
      assert(type(request) in six.integer_types)
      assert(type(procedure) == six.text_type)
      assert(args is None or type(args) in [list, tuple])
      assert(kwargs is None or type(kwargs) == dict)
      assert(timeout is None or type(timeout) in six.integer_types)
      assert(receive_progress is None or type(receive_progress) == bool)
      assert(discloseMe is None or type(discloseMe) == bool)

      Message.__init__(self)
      self.request = request
      self.procedure = procedure
      self.args = args
      self.kwargs = kwargs
      self.timeout = timeout
      self.receive_progress = receive_progress
      self.discloseMe = discloseMe


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Call.MESSAGE_TYPE)

      if len(wmsg) not in (4, 5, 6):
         raise ProtocolError("invalid message length {0} for CALL".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in CALL")
      options = check_or_raise_extra(wmsg[2], "'options' in CALL")
      procedure = check_or_raise_uri(wmsg[3], "'procedure' in CALL")

      args = None
      if len(wmsg) > 4:
         args = wmsg[4]
         if type(args) != list:
            raise ProtocolError("invalid type {0} for 'args' in CALL".format(type(args)))

      kwargs = None
      if len(wmsg) > 5:
         kwargs = wmsg[5]
         if type(kwargs) != dict:
            raise ProtocolError("invalid type {0} for 'kwargs' in CALL".format(type(kwargs)))

      timeout = None
      if u'timeout' in options:

         option_timeout = options[u'timeout']
         if type(option_timeout) not in six.integer_types:
            raise ProtocolError("invalid type {0} for 'timeout' option in CALL".format(type(option_timeout)))

         if option_timeout < 0:
            raise ProtocolError("invalid value {0} for 'timeout' option in CALL".format(option_timeout))

         timeout = option_timeout

      receive_progress = None
      if u'receive_progress' in options:

         option_receive_progress = options[u'receive_progress']
         if type(option_receive_progress) != bool:
            raise ProtocolError("invalid type {0} for 'receive_progress' option in CALL".format(type(option_receive_progress)))

         receive_progress = option_receive_progress

      discloseMe = None
      if u'disclose_me' in options:

         option_discloseMe = options[u'disclose_me']
         if type(option_discloseMe) != bool:
            raise ProtocolError("invalid type {0} for 'disclose_me' option in CALL".format(type(option_discloseMe)))

         discloseMe = option_discloseMe

      obj = Call(request,
                 procedure,
                 args = args,
                 kwargs = kwargs,
                 timeout = timeout,
                 receive_progress = receive_progress,
                 discloseMe = discloseMe)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      options = {}

      if self.timeout is not None:
         options[u'timeout'] = self.timeout

      if self.receive_progress is not None:
         options[u'receive_progress'] = self.receive_progress

      if self.discloseMe is not None:
         options[u'disclose_me'] = self.discloseMe

      if self.kwargs:
         return [Call.MESSAGE_TYPE, self.request, options, self.procedure, self.args, self.kwargs]
      elif self.args:
         return [Call.MESSAGE_TYPE, self.request, options, self.procedure, self.args]
      else:
         return [Call.MESSAGE_TYPE, self.request, options, self.procedure]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP CALL Message (request = {0}, procedure = {1}, args = {2}, kwargs = {3}, timeout = {4}, receive_progress = {5}, discloseMe = {6})".format(self.request, self.procedure, self.args, self.kwargs, self.timeout, self.receive_progress, self.discloseMe)



class Cancel(Message):
   """
   A WAMP ``CANCEL`` message.

   Format: ``[CANCEL, CALL.Request|id, Options|dict]``
   """

   MESSAGE_TYPE = 49
   """
   The WAMP message code for this type of message.
   """

   SKIP = u'skip'
   ABORT = u'abort'
   KILL = u'kill'


   def __init__(self, request, mode = None):
      """

      :param request: The WAMP request ID of the original `CALL` to cancel.
      :type request: int
      :param mode: Specifies how to cancel the call (``"skip"``, ``"abort"`` or ``"kill"``).
      :type mode: unicode or None
      """
      assert(type(request) in six.integer_types)
      assert(mode is None or type(mode) == six.text_type)
      assert(mode in [None, self.SKIP, self.ABORT, self.KILL])

      Message.__init__(self)
      self.request = request
      self.mode = mode


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Cancel.MESSAGE_TYPE)

      if len(wmsg) != 3:
         raise ProtocolError("invalid message length {0} for CANCEL".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in CANCEL")
      options = check_or_raise_extra(wmsg[2], "'options' in CANCEL")

      ## options
      ##
      mode = None

      if u'mode' in options:

         option_mode = options[u'mode']
         if type(option_mode) != six.text_type:
            raise ProtocolError("invalid type {0} for 'mode' option in CANCEL".format(type(option_mode)))

         if option_mode not in [Cancel.SKIP, Cancel.ABORT, Cancel.KILL]:
            raise ProtocolError("invalid value '{0}' for 'mode' option in CANCEL".format(option_mode))

         mode = option_mode

      obj = Cancel(request, mode = mode)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      options = {}

      if self.mode is not None:
         options[u'mode'] = self.mode

      return [Cancel.MESSAGE_TYPE, self.request, options]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP CANCEL Message (request = {0}, mode = '{1}'')".format(self.request, self.mode)



class Result(Message):
   """
   A WAMP ``RESULT`` message.

   Formats:

   * ``[RESULT, CALL.Request|id, Details|dict]``
   * ``[RESULT, CALL.Request|id, Details|dict, YIELD.Arguments|list]``
   * ``[RESULT, CALL.Request|id, Details|dict, YIELD.Arguments|list, YIELD.ArgumentsKw|dict]``
   """

   MESSAGE_TYPE = 50
   """
   The WAMP message code for this type of message.
   """

   def __init__(self, request, args = None, kwargs = None, progress = None):
      """

      :param request: The request ID of the original `CALL` request.
      :type request: int
      :param args: Positional values for application-defined event payload.
         Must be serializable using any serializers in use.
      :type args: list or tuple or None
      :param kwargs: Keyword values for application-defined event payload.
         Must be serializable using any serializers in use.
      :type kwargs: dict or None
      :param progress: If ``True``, this result is a progressive call result, and subsequent
         results (or a final error) will follow.
      :type progress: bool or None
      """
      assert(type(request) in six.integer_types)
      assert(args is None or type(args) in [list, tuple])
      assert(kwargs is None or type(kwargs) == dict)
      assert(progress is None or type(progress) == bool)

      Message.__init__(self)
      self.request = request
      self.args = args
      self.kwargs = kwargs
      self.progress = progress


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Result.MESSAGE_TYPE)

      if len(wmsg) not in (3, 4, 5):
         raise ProtocolError("invalid message length {0} for RESULT".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in RESULT")
      details = check_or_raise_extra(wmsg[2], "'details' in RESULT")

      args = None
      if len(wmsg) > 3:
         args = wmsg[3]
         if type(args) != list:
            raise ProtocolError("invalid type {0} for 'args' in RESULT".format(type(args)))

      kwargs = None
      if len(wmsg) > 4:
         kwargs = wmsg[4]
         if type(kwargs) != dict:
            raise ProtocolError("invalid type {0} for 'kwargs' in RESULT".format(type(kwargs)))

      progress = None

      if u'progress' in details:

         detail_progress = details[u'progress']
         if type(detail_progress) != bool:
            raise ProtocolError("invalid type {0} for 'progress' option in RESULT".format(type(detail_progress)))

         progress = detail_progress

      obj = Result(request, args = args, kwargs = kwargs, progress = progress)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      details = {}

      if self.progress is not None:
         details[u'progress'] = self.progress

      if self.kwargs:
         return [Result.MESSAGE_TYPE, self.request, details, self.args, self.kwargs]
      elif self.args:
         return [Result.MESSAGE_TYPE, self.request, details, self.args]
      else:
         return [Result.MESSAGE_TYPE, self.request, details]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP RESULT Message (request = {0}, args = {1}, kwargs = {2}, progress = {3})".format(self.request, self.args, self.kwargs, self.progress)



class Register(Message):
   """
   A WAMP ``REGISTER`` message.

   Format: ``[REGISTER, Request|id, Options|dict, Procedure|uri]``
   """

   MESSAGE_TYPE = 64
   """
   The WAMP message code for this type of message.
   """

   def __init__(self, request, procedure, pkeys = None, discloseCaller = None):
      """

      :param request: The WAMP request ID of this request.
      :type request: int
      :param procedure: The WAMP or application URI of the RPC endpoint provided.
      :type procedure: unicode
      :param pkeys: The endpoint can work for this list of application partition keys.
      :type pkeys: list of int or None
      :param discloseCaller: If ``True``, the (registering) callee requests to disclose
         the identity of callers whenever called.
      :type discloseCaller: bool or None
      """
      assert(type(request) in six.integer_types)
      assert(type(procedure) == six.text_type)
      assert(pkeys is None or type(pkeys) == list)
      if pkeys:
         for k in pkeys:
            assert(type(k) in six.integer_types)
      assert(discloseCaller is None or type(discloseCaller) == bool)

      Message.__init__(self)
      self.request = request
      self.procedure = procedure
      self.pkeys = pkeys
      self.discloseCaller = discloseCaller


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Register.MESSAGE_TYPE)

      if len(wmsg) != 4:
         raise ProtocolError("invalid message length {0} for REGISTER".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in REGISTER")
      options = check_or_raise_extra(wmsg[2], "'options' in REGISTER")
      procedure = check_or_raise_uri(wmsg[3], "'procedure' in REGISTER")

      pkeys = None
      discloseCaller = None

      if u'pkeys' in options:

         option_pkeys = options[u'pkeys']
         if type(option_pkeys) != list:
            raise ProtocolError("invalid type {0} for 'pkeys' option in REGISTER".format(type(option_pkeys)))

         for pk in option_pkeys:
            if type(pk) not in six.integer_types:
               raise ProtocolError("invalid type for value '{0}' in 'pkeys' option in REGISTER".format(type(pk)))

         pkeys = option_pkeys


      if u'disclose_caller' in options:

         option_discloseCaller = options[u'disclose_caller']
         if type(option_discloseCaller) != bool:
            raise ProtocolError("invalid type {0} for 'disclose_caller' option in REGISTER".format(type(option_discloseCaller)))

         discloseCaller = option_discloseCaller

      obj = Register(request, procedure, pkeys = pkeys, discloseCaller = discloseCaller)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      options = {}

      if self.pkeys is not None:
         options[u'pkeys'] = self.pkeys

      if self.discloseCaller is not None:
         options[u'disclose_caller'] = self.discloseCaller

      return [Register.MESSAGE_TYPE, self.request, options, self.procedure]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP REGISTER Message (request = {0}, procedure = {1}, pkeys = {2}, discloseCaller = {3})".format(self.request, self.procedure, self.pkeys, self.discloseCaller)



class Registered(Message):
   """
   A WAMP ``REGISTERED`` message.

   Format: ``[REGISTERED, REGISTER.Request|id, Registration|id]``
   """

   MESSAGE_TYPE = 65
   """
   The WAMP message code for this type of message.
   """

   def __init__(self, request, registration):
      """

      :param request: The request ID of the original ``REGISTER`` request.
      :type request: int
      :param registration: The registration ID for the registered procedure (or procedure pattern).
      :type registration: int
      """
      assert(type(request) in six.integer_types)
      assert(type(registration) in six.integer_types)

      Message.__init__(self)
      self.request = request
      self.registration = registration


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Registered.MESSAGE_TYPE)

      if len(wmsg) != 3:
         raise ProtocolError("invalid message length {0} for REGISTERED".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in REGISTERED")
      registration = check_or_raise_id(wmsg[2], "'registration' in REGISTERED")

      obj = Registered(request, registration)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      return [Registered.MESSAGE_TYPE, self.request, self.registration]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP REGISTERED Message (request = {0}, registration = {1})".format(self.request, self.registration)



class Unregister(Message):
   """
   A WAMP `UNREGISTER` message.

   Format: ``[UNREGISTER, Request|id, REGISTERED.Registration|id]``
   """

   MESSAGE_TYPE = 66
   """
   The WAMP message code for this type of message.
   """


   def __init__(self, request, registration):
      """

      :param request: The WAMP request ID of this request.
      :type request: int
      :param registration: The registration ID for the registration to unregister.
      :type registration: int
      """
      assert(type(request) in six.integer_types)
      assert(type(registration) in six.integer_types)

      Message.__init__(self)
      self.request = request
      self.registration = registration


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Unregister.MESSAGE_TYPE)

      if len(wmsg) != 3:
         raise ProtocolError("invalid message length {0} for WAMP UNREGISTER".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in UNREGISTER")
      registration = check_or_raise_id(wmsg[2], "'registration' in UNREGISTER")

      obj = Unregister(request, registration)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      return [Unregister.MESSAGE_TYPE, self.request, self.registration]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP UNREGISTER Message (request = {0}, registration = {1})".format(self.request, self.registration)



class Unregistered(Message):
   """
   A WAMP ``UNREGISTERED`` message.

   Format: ``[UNREGISTERED, UNREGISTER.Request|id]``
   """

   MESSAGE_TYPE = 67
   """
   The WAMP message code for this type of message.
   """

   def __init__(self, request):
      """

      :param request: The request ID of the original ``UNREGISTER`` request.
      :type request: int
      """
      assert(type(request) in six.integer_types)

      Message.__init__(self)
      self.request = request


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Unregistered.MESSAGE_TYPE)

      if len(wmsg) != 2:
         raise ProtocolError("invalid message length {0} for UNREGISTER".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in UNREGISTER")

      obj = Unregistered(request)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      return [Unregistered.MESSAGE_TYPE, self.request]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP UNREGISTER Message (request = {0})".format(self.request)



class Invocation(Message):
   """
   A WAMP ``INVOCATION`` message.

   Formats:

   * ``[INVOCATION, Request|id, REGISTERED.Registration|id, Details|dict]``
   * ``[INVOCATION, Request|id, REGISTERED.Registration|id, Details|dict, CALL.Arguments|list]``
   * ``[INVOCATION, Request|id, REGISTERED.Registration|id, Details|dict, CALL.Arguments|list, CALL.ArgumentsKw|dict]``
   """

   MESSAGE_TYPE = 68
   """
   The WAMP message code for this type of message.
   """


   def __init__(self,
                request,
                registration,
                args = None,
                kwargs = None,
                timeout = None,
                receive_progress = None,
                caller = None,
                authid = None,
                authrole = None,
                authmethod = None):
      """

      :param request: The WAMP request ID of this request.
      :type request: int
      :param registration: The registration ID of the endpoint to be invoked.
      :type registration: int
      :param args: Positional values for application-defined event payload.
         Must be serializable using any serializers in use.
      :type args: list or tuple or None
      :param kwargs: Keyword values for application-defined event payload.
         Must be serializable using any serializers in use.
      :type kwargs: dict or None
      :param timeout: If present, let the callee automatically cancels
         the invocation after this ms.
      :type timeout: int or None
      :param receive_progress: Indicates if the callee should produce progressive results.
      :type receive_progress: bool or None
      :param caller: The WAMP session ID of the caller.
      :type caller: int or None
      :param authid: The authentication ID of the caller.
      :type authid: unicode or None
      :param authrole: The authentication role of the caller.
      :type authrole: unicode or None
      :param authmethod: The authentication method under which the caller was authenticated.
      :type authmethod: unicode or None
      """
      assert(type(request) in six.integer_types)
      assert(type(registration) in six.integer_types)
      assert(args is None or type(args) in [list, tuple])
      assert(kwargs is None or type(kwargs) == dict)
      assert(timeout is None or type(timeout) in six.integer_types)
      assert(receive_progress is None or type(receive_progress) == bool)
      assert(caller is None or type(caller) in six.integer_types)
      assert(authid is None or type(authid) == six.text_type)
      assert(authrole is None or type(authrole) == six.text_type)
      assert(authmethod is None or type(authmethod) == six.text_type)

      Message.__init__(self)
      self.request = request
      self.registration = registration
      self.args = args
      self.kwargs = kwargs
      self.timeout = timeout
      self.receive_progress = receive_progress
      self.caller = caller
      self.authid = authid
      self.authrole = authrole
      self.authmethod = authmethod


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Invocation.MESSAGE_TYPE)

      if len(wmsg) not in (4, 5, 6):
         raise ProtocolError("invalid message length {0} for INVOCATION".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in INVOCATION")
      registration = check_or_raise_id(wmsg[2], "'registration' in INVOCATION")
      details = check_or_raise_extra(wmsg[3], "'details' in INVOCATION")

      args = None
      if len(wmsg) > 4:
         args = wmsg[4]
         if type(args) != list:
            raise ProtocolError("invalid type {0} for 'args' in INVOCATION".format(type(args)))

      kwargs = None
      if len(wmsg) > 5:
         kwargs = wmsg[5]
         if type(kwargs) != dict:
            raise ProtocolError("invalid type {0} for 'kwargs' in INVOCATION".format(type(kwargs)))

      timeout = None
      if u'timeout' in details:

         detail_timeout = details[u'timeout']
         if type(detail_timeout) not in six.integer_types:
            raise ProtocolError("invalid type {0} for 'timeout' detail in INVOCATION".format(type(detail_timeout)))

         if detail_timeout < 0:
            raise ProtocolError("invalid value {0} for 'timeout' detail in INVOCATION".format(detail_timeout))

         timeout = detail_timeout

      receive_progress = None
      if u'receive_progress' in details:

         detail_receive_progress = details[u'receive_progress']
         if type(detail_receive_progress) != bool:
            raise ProtocolError("invalid type {0} for 'receive_progress' detail in INVOCATION".format(type(detail_receive_progress)))

         receive_progress = detail_receive_progress

      caller = None
      if u'caller' in details:

         detail_caller = details[u'caller']
         if type(detail_caller) not in six.integer_types:
            raise ProtocolError("invalid type {0} for 'caller' detail in INVOCATION".format(type(detail_caller)))

         caller = detail_caller

      authid = None
      if u'authid' in details:

         detail_authid = details[u'authid']
         if type(detail_authid) != six.text_type:
            raise ProtocolError("invalid type {0} for 'authid' detail in INVOCATION".format(type(detail_authid)))

         authid = detail_authid

      authrole = None
      if u'authrole' in details:

         detail_authrole = details[u'authrole']
         if type(detail_authrole) != six.text_type:
            raise ProtocolError("invalid type {0} for 'authrole' detail in INVOCATION".format(type(detail_authrole)))

         authrole = detail_authrole

      authmethod = None
      if u'authmethod' in details:

         detail_authmethod = details[u'authmethod']
         if type(detail_authmethod) != six.text_type:
            raise ProtocolError("invalid type {0} for 'authmethod' detail in INVOCATION".format(type(detail_authmethod)))

         authmethod = detail_authmethod

      obj = Invocation(request,
                       registration,
                       args = args,
                       kwargs = kwargs,
                       timeout = timeout,
                       receive_progress = receive_progress,
                       caller = caller,
                       authid = authid,
                       authrole = authrole,
                       authmethod = authmethod)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      options = {}

      if self.timeout is not None:
         options[u'timeout'] = self.timeout

      if self.receive_progress is not None:
         options[u'receive_progress'] = self.receive_progress

      if self.caller is not None:
         options[u'caller'] = self.caller

      if self.authid is not None:
         options[u'authid'] = self.authid

      if self.authrole is not None:
         options[u'authrole'] = self.authrole

      if self.authmethod is not None:
         options[u'authmethod'] = self.authmethod

      if self.kwargs:
         return [Invocation.MESSAGE_TYPE, self.request, self.registration, options, self.args, self.kwargs]
      elif self.args:
         return [Invocation.MESSAGE_TYPE, self.request, self.registration, options, self.args]
      else:
         return [Invocation.MESSAGE_TYPE, self.request, self.registration, options]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP INVOCATION Message (request = {0}, registration = {1}, args = {2}, kwargs = {3}, timeout = {4}, receive_progress = {5}, caller = {6}, authid = {7}, authrole = {8}, authmethod = {9})".format(self.request, self.registration, self.args, self.kwargs, self.timeout, self.receive_progress, self.caller, self.authid, self.authrole, self.authmethod)



class Interrupt(Message):
   """
   A WAMP ``INTERRUPT`` message.

   Format: ``[INTERRUPT, INVOCATION.Request|id, Options|dict]``
   """

   MESSAGE_TYPE = 69
   """
   The WAMP message code for this type of message.
   """

   ABORT = u'abort'
   KILL = u'kill'


   def __init__(self, request, mode = None):
      """

      :param request: The WAMP request ID of the original ``INVOCATION`` to interrupt.
      :type request: int
      :param mode: Specifies how to interrupt the invocation (``"abort"`` or ``"kill"``).
      :type mode: unicode or None
      """
      assert(type(request) in six.integer_types)
      assert(mode is None or type(mode) == six.text_type)
      assert(mode is None or mode in [self.ABORT, self.KILL])

      Message.__init__(self)
      self.request = request
      self.mode = mode


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Interrupt.MESSAGE_TYPE)

      if len(wmsg) != 3:
         raise ProtocolError("invalid message length {0} for INTERRUPT".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in INTERRUPT")
      options = check_or_raise_extra(wmsg[2], "'options' in INTERRUPT")

      ## options
      ##
      mode = None

      if u'mode' in options:

         option_mode = options[u'mode']
         if type(option_mode) != six.text_type:
            raise ProtocolError("invalid type {0} for 'mode' option in INTERRUPT".format(type(option_mode)))

         if option_mode not in [Interrupt.ABORT, Interrupt.KILL]:
            raise ProtocolError("invalid value '{0}' for 'mode' option in INTERRUPT".format(option_mode))

         mode = option_mode

      obj = Interrupt(request, mode = mode)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      options = {}

      if self.mode is not None:
         options[u'mode'] = self.mode

      return [Interrupt.MESSAGE_TYPE, self.request, options]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP INTERRUPT Message (request = {0}, mode = '{1}')".format(self.request, self.mode)



class Yield(Message):
   """
   A WAMP ``YIELD`` message.

   Formats:

   * ``[YIELD, INVOCATION.Request|id, Options|dict]``
   * ``[YIELD, INVOCATION.Request|id, Options|dict, Arguments|list]``
   * ``[YIELD, INVOCATION.Request|id, Options|dict, Arguments|list, ArgumentsKw|dict]``
   """

   MESSAGE_TYPE = 70
   """
   The WAMP message code for this type of message.
   """


   def __init__(self, request, args = None, kwargs = None, progress = None):
      """

      :param request: The WAMP request ID of the original call.
      :type request: int
      :param args: Positional values for application-defined event payload.
         Must be serializable using any serializers in use.
      :type args: list or tuple or None
      :param kwargs: Keyword values for application-defined event payload.
         Must be serializable using any serializers in use.
      :type kwargs: dict or None
      :param progress: If ``True``, this result is a progressive invocation result, and subsequent
         results (or a final error) will follow.
      :type progress: bool or None
      """
      assert(type(request) in six.integer_types)
      assert(args is None or type(args) in [list, tuple])
      assert(kwargs is None or type(kwargs) == dict)
      assert(progress is None or type(progress) == bool)

      Message.__init__(self)
      self.request = request
      self.args = args
      self.kwargs = kwargs
      self.progress = progress


   @staticmethod
   def parse(wmsg):
      """
      Verifies and parses an unserialized raw message into an actual WAMP message instance.

      :param wmsg: The unserialized raw message.
      :type wmsg: list

      :returns: An instance of this class.
      """
      ## this should already be verified by WampSerializer.unserialize
      ##
      assert(len(wmsg) > 0 and wmsg[0] == Yield.MESSAGE_TYPE)

      if len(wmsg) not in (3, 4, 5):
         raise ProtocolError("invalid message length {0} for YIELD".format(len(wmsg)))

      request = check_or_raise_id(wmsg[1], "'request' in YIELD")
      options = check_or_raise_extra(wmsg[2], "'options' in YIELD")

      args = None
      if len(wmsg) > 3:
         args = wmsg[3]
         if type(args) != list:
            raise ProtocolError("invalid type {0} for 'args' in YIELD".format(type(args)))

      kwargs = None
      if len(wmsg) > 4:
         kwargs = wmsg[4]
         if type(kwargs) != dict:
            raise ProtocolError("invalid type {0} for 'kwargs' in YIELD".format(type(kwargs)))

      progress = None

      if u'progress' in options:

         option_progress = options[u'progress']
         if type(option_progress) != bool:
            raise ProtocolError("invalid type {0} for 'progress' option in YIELD".format(type(option_progress)))

         progress = option_progress

      obj = Yield(request, args = args, kwargs = kwargs, progress = progress)

      return obj


   def marshal(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.marshal`
      """
      options = {}

      if self.progress is not None:
         options[u'progress'] = self.progress

      if self.kwargs:
         return [Yield.MESSAGE_TYPE, self.request, options, self.args, self.kwargs]
      elif self.args:
         return [Yield.MESSAGE_TYPE, self.request, options, self.args]
      else:
         return [Yield.MESSAGE_TYPE, self.request, options]


   def __str__(self):
      """
      Implements :func:`autobahn.wamp.interfaces.IMessage.__str__`
      """
      return "WAMP YIELD Message (request = {0}, args = {1}, kwargs = {2}, progress = {3})".format(self.request, self.args, self.kwargs, self.progress)
