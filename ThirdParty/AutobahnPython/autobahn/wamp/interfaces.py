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

import abc
import six



@six.add_metaclass(abc.ABCMeta)
class IObjectSerializer(object):
   """
   Raw Python object serialization and deserialization. Object serializers are
   used by classes implementing WAMP serializers, that is instances of
   :class:`autobahn.wamp.interfaces.ISerializer`.
   """


   @abc.abstractproperty
   def BINARY(self):
      """
      Flag (read-only) to indicate if serializer requires a binary clean
      transport or if UTF8 transparency is sufficient.
      """


   @abc.abstractmethod
   def serialize(self, obj):
      """
      Serialize an object to a byte string.

      :param obj: Object to serialize.
      :type obj: Any serializable type.

      :returns: bytes -- Serialized byte string.
      """

   @abc.abstractmethod
   def unserialize(self, bytes):
      """
      Unserialize objects from a byte string.

      :param bytes: Objects to unserialize.
      :type bytes: bytes

      :returns: list -- List of (raw) objects unserialized.
      """



@six.add_metaclass(abc.ABCMeta)
class IMessage(object):
   """
   A WAMP message.
   """

   @abc.abstractproperty
   def MESSAGE_TYPE(self):
      """
      WAMP message type code.
      """


   @abc.abstractmethod
   def marshal(self):
      """
      Marshal this object into a raw message for subsequent serialization to bytes.

      :returns: list -- The serialized raw message.
      """


   #@abc.abstractstaticmethod ## FIXME: this is Python 3 only
   # noinspection PyMethodParameters
   def parse(wmsg):
      """
      Factory method that parses a unserialized raw message (as returned byte
      :func:`autobahn.interfaces.ISerializer.unserialize`) into an instance
      of this class.

      :returns: obj -- An instance of this class.
      """


   @abc.abstractmethod
   def serialize(self, serializer):
      """
      Serialize this object into a wire level bytes representation and cache
      the resulting bytes. If the cache already contains an entry for the given
      serializer, return the cached representation directly.

      :param serializer: The wire level serializer to use.
      :type serializer: An instance that implements :class:`autobahn.interfaces.ISerializer`

      :returns: bytes -- The serialized bytes.
      """


   @abc.abstractmethod
   def uncache(self):
      """
      Resets the serialization cache.
      """


   @abc.abstractmethod
   def __eq__(self, other):
      """
      Message equality. This does an attribute-wise comparison (but skips attributes
      that start with `_`).
      """


   @abc.abstractmethod
   def __ne__(self, other):
      """
      Message inequality (just the negate of message equality).
      """


   @abc.abstractmethod
   def __str__(self):
      """
      Returns text representation of this message.

      :returns: str -- Human readable representation (e.g. for logging or debugging purposes).
      """



@six.add_metaclass(abc.ABCMeta)
class ISerializer(object):
   """
   WAMP message serialization and deserialization.
   """

   @abc.abstractproperty
   def MESSAGE_TYPE_MAP(self):
      """
      Mapping of WAMP message type codes to WAMP message classes.
      """


   @abc.abstractproperty
   def SERIALIZER_ID(self):
      """
      The WAMP serialization format ID.
      """


   @abc.abstractmethod
   def serialize(self, message):
      """
      Serializes a WAMP message to bytes to be sent to a transport.

      :param message: An instance that implements :class:`autobahn.wamp.interfaces.IMessage`
      :type message: obj

      :returns: tuple -- A pair ``(bytes, isBinary)``.
      """


   @abc.abstractmethod
   def unserialize(self, bytes, isBinary):
      """
      Deserialize bytes from a transport and parse into WAMP messages.

      :param bytes: Byte string from wire.
      :type bytes: bytes

      :returns: list -- List of objects that implement :class:`autobahn.wamp.interfaces.IMessage`.
      """



@six.add_metaclass(abc.ABCMeta)
class ITransport(object):
   """
   A WAMP transport is a bidirectional, full-duplex, reliable, ordered,
   message-based channel.
   """

   @abc.abstractmethod
   def send(self, message):
      """
      Send a WAMP message over the transport to the peer. If the transport is
      not open, this raises :class:`autobahn.wamp.exception.TransportLost`.

      :param message: An instance that implements :class:`autobahn.wamp.interfaces.IMessage`
      :type message: obj
      """


   @abc.abstractmethod
   def isOpen(self):
      """
      Check if the transport is open for messaging.

      :returns: bool -- ``True``, if the transport is open.
      """


   @abc.abstractmethod
   def close(self):
      """
      Close the transport regularly. The transport will perform any
      closing handshake if applicable. This should be used for any
      application initiated closing.
      """


   @abc.abstractmethod
   def abort(self):
      """
      Abort the transport abruptly. The transport will be destroyed as
      fast as possible, and without playing nice to the peer. This should
      only be used in case of fatal errors, protocol violations or possible
      detected attacks.
      """



@six.add_metaclass(abc.ABCMeta)
class ITransportHandler(object):

   @abc.abstractmethod
   def onOpen(self, transport):
      """
      Callback fired when transport is open.

      :param transport: An instance that implements :class:`autobahn.wamp.interfaces.ITransport`
      :type transport: obj
      """


   @abc.abstractmethod
   def onMessage(self, message):
      """
      Callback fired when a WAMP message was received.

      :param message: An instance that implements :class:`autobahn.wamp.interfaces.IMessage`
      :type message: obj
      """


   @abc.abstractmethod
   def onClose(self, wasClean):
      """
      Callback fired when the transport has been closed.

      :param wasClean: Indicates if the transport has been closed regularly.
      :type wasClean: bool
      """



@six.add_metaclass(abc.ABCMeta)
class ISession(object):
   """
   Base interface for WAMP sessions.
   """

   @abc.abstractmethod
   def onConnect(self):
      """
      Callback fired when the transport this session will run over has been established.
      """


   @abc.abstractmethod
   def join(self, realm):
      """
      Attach the session to the given realm. A session is open as soon as it is attached to a realm.
      """


   @abc.abstractmethod
   def onJoin(self, details):
      """
      Callback fired when WAMP session has been established.

      :param details: Session information.
      :type details: Instance of :class:`autobahn.wamp.types.SessionDetails`.
      """


   @abc.abstractmethod
   def leave(self, reason = None, message = None):
      """
      Actively close this WAMP session.

      :param reason: An optional URI for the closing reason.
      :type reason: str
      :param message: An optional (human readable) closing message, intended for
                      logging purposes.
      :type message: str
      """


   @abc.abstractmethod
   def onLeave(self, details):
      """
      Callback fired when WAMP session has is closed

      :param details: Close information.
      :type details: Instance of :class:`autobahn.wamp.types.CloseDetails`.
      """


   @abc.abstractmethod
   def disconnect(self):
      """
      Close the underlying transport.
      """


   @abc.abstractmethod
   def onDisconnect(self):
      """
      Callback fired when underlying transport has been closed.
      """


   @abc.abstractmethod
   def define(self, exception, error = None):
      """
      Defines an exception for a WAMP error in the context of this WAMP session.

      :param exception: The exception class to define an error mapping for.
      :type exception: A class that derives of ``Exception``.
      :param error: The URI (or URI pattern) the exception class should be mapped for.
                    Iff the ``exception`` class is decorated, this must be ``None``.
      :type error: str
      """



class ICaller(ISession):
   """
   Interface for WAMP peers implementing role *Caller*.
   """

   @abc.abstractmethod
   def call(self, procedure, *args, **kwargs):
      """
      Call a remote procedure.

      This will return a Deferred/Future, that when resolved, provides the actual result.

      If the result is a single positional return value, it'll be returned "as-is". If the
      result contains multiple positional return values or keyword return values,
      the result is wrapped in an instance of :class:`autobahn.wamp.types.CallResult`.

      If the call fails, the returned Deferred/Future will be rejected with an instance
      of :class:`autobahn.wamp.exception.ApplicationError`.

      If the *Caller* and *Dealer* implementations support canceling of calls, the call may
      be canceled by canceling the returned Deferred/Future.

      If ``kwargs`` contains an ``options`` keyword argument that is an instance of
      :class:`autobahn.wamp.types.CallOptions`, this will provide
      specific options for the call to perform.

      :param procedure: The URI of the remote procedure to be called, e.g. ``"com.myapp.hello"``.
      :type procedure: str
      :param args: Any positional arguments for the call.
      :type args: list
      :param kwargs: Any keyword arguments for the call.
      :type kwargs: dict

      :returns: obj -- A Deferred/Future for the call result -
                       an instance of :class:`twisted.internet.defer.Deferred` (when running under Twisted) or
                       an instance of :class:`asyncio.Future` (when running under asyncio).
      """



@six.add_metaclass(abc.ABCMeta)
class IRegistration(object):
   """
   Represents a registration of an endpoint.
   """

   @abc.abstractproperty
   def id(self):
      """
      The WAMP registration ID for this registration.
      """

   @abc.abstractproperty
   def id(self):
      """
      Flag indicating if registration is active.
      """


   @abc.abstractmethod
   def unregister(self):
      """
      Unregister this registration that was previously created from
      :func:`autobahn.wamp.interfaces.ICallee.register`.

      After a registration has been unregistered, calls won't get routed
      to the endpoint any more.

      This will return a Deferred/Future, that when resolved signals
      successful unregistration.

      If the unregistration fails, the returned Deferred/Future will be rejected
      with an instance of :class:`autobahn.wamp.exception.ApplicationError`.

      :returns: obj -- A Deferred/Future for the unregistration -
                       an instance of :class:`twisted.internet.defer.Deferred` (when running under Twisted)
                       or an instance of :class:`asyncio.Future` (when running under asyncio).
      """



class ICallee(ISession):
   """
   Interface for WAMP peers implementing role *Callee*.
   """

   @abc.abstractmethod
   def register(self, endpoint, procedure = None, options = None):
      """
      Register an endpoint for a procedure to (subsequently) receive calls
      calling that procedure.

      If ``endpoint`` is a callable (function, method or object that implements ``__call__``),
      then `procedure` must be provided and an instance of
      :class:`twisted.internet.defer.Deferred` (when running on Twisted) or an instance
      of :class:`asyncio.Future` (when running on asyncio) is returned.

      If the registration succeeds the Deferred/Future will resolve to an object
      that implements :class:`autobahn.wamp.interfaces.Registration`.

      If the registration fails the Deferred/Future will reject with an instance
      of :class:`autobahn.wamp.exception.ApplicationError`.

      If ``endpoint`` is an object, then each of the object's methods that are decorated
      with :func:`autobahn.wamp.register` are registered as procedure endpoints, and a list of
      Deferreds/Futures is returned that each resolves or rejects as above.

      :param endpoint: The endpoint or endpoint object called under the procedure.
      :type endpoint: callable
      :param procedure: When ``endpoint`` is a single event handler, the URI (or URI pattern)
                    of the procedure to register for. When ``endpoint`` is an endpoint
                    object, this value is ignored (and should be ``None``).
      :type procedure: str
      :param options: Options for registering.
      :type options: An instance of :class:`autobahn.wamp.types.RegisterOptions`.

      :returns: obj -- A (list of) Deferred(s)/Future(s) for the registration(s) -
                       instance(s) of :class:`twisted.internet.defer.Deferred` (when
                       running under Twisted) or instance(s) of :class:`asyncio.Future`
                       (when running under asyncio).
      """



@six.add_metaclass(abc.ABCMeta)
class IPublication(object):
   """
   Represents a publication of an event. This is used with acknowledged publications.
   """

   @abc.abstractproperty
   def id(self):
      """
      The WAMP publication ID for this publication.
      """



class IPublisher(ISession):
   """
   Interface for WAMP peers implementing role *Publisher*.
   """

   @abc.abstractmethod
   def publish(self, topic, *args, **kwargs):
      """
      Publish an event to a topic.

      If ``kwargs`` contains an ``options`` keyword argument that is an instance of
      :class:`autobahn.wamp.types.PublishOptions`, this will provide
      specific options for the publish to perform.

      If publication acknowledgement is requested via ``options.acknowledge == True``,
      this function returns a Deferred/Future:

      - if the publication succeeds the Deferred/Future will resolve to an object
        that implements :class:`autobahn.wamp.interfaces.IPublication`.

      - if the publication fails the Deferred/Future will reject with an instance
        of :class:`autobahn.wamp.exception.ApplicationError`.

      :param topic: The URI of the topic to publish to, e.g. ``"com.myapp.mytopic1"``.
      :type topic: str
      :param args: Arbitrary application payload for the event (positional arguments).
      :type args: list
      :param kwargs: Arbitrary application payload for the event (keyword arguments).
      :type kwargs: dict

      :returns: obj -- ``None`` for non-acknowledged publications or,
                       for acknowledged publications, an instance of
                       :class:`twisted.internet.defer.Deferred` (when running under Twisted)
                       or an instance of :class:`asyncio.Future` (when running under asyncio).
      """



@six.add_metaclass(abc.ABCMeta)
class ISubscription(object):
   """
   Represents a subscription to a topic.
   """

   @abc.abstractproperty
   def id(self):
      """
      The WAMP subscription ID for this subscription.
      """


   @abc.abstractproperty
   def active(self):
      """
      Flag indicating if subscription is active.
      """


   @abc.abstractmethod
   def unsubscribe(self):
      """
      Unsubscribe this subscription that was previously created from
      :func:`autobahn.wamp.interfaces.ISubscriber.subscribe`.

      After a subscription has been unsubscribed, events won't get
      routed to the handler anymore.

      This will return a Deferred/Future, that when resolved signals
      successful unsubscription.

      If the unsubscription fails, the returned Deferred/Future will be rejected
      with an instance of :class:`autobahn.wamp.exception.ApplicationError`.

      :returns: obj -- A Deferred/Future for the unsubscription -
                       an instance of :class:`twisted.internet.defer.Deferred` (when running under Twisted)
                       or an instance of :class:`asyncio.Future` (when running under asyncio).
      """



class ISubscriber(ISession):
   """
   Interface for WAMP peers implementing role *Subscriber*.
   """

   @abc.abstractmethod
   def subscribe(self, handler, topic = None, options = None):
      """
      Subscribe to a topic and subsequently receive events published to that topic.

      If ``handler`` is a callable (function, method or object that implements ``__call__``),
      then `topic` must be provided and an instance of
      :class:`twisted.internet.defer.Deferred` (when running on Twisted) or an instance
      of :class:`asyncio.Future` (when running on asyncio) is returned.

      If the subscription succeeds the Deferred/Future will resolve to an object
      that implements :class:`autobahn.wamp.interfaces.ISubscription`.

      If the subscription fails the Deferred/Future will reject with an instance
      of :class:`autobahn.wamp.exception.ApplicationError`.

      If ``handler`` is an object, then each of the object's methods that are decorated
      with :func:`autobahn.wamp.subscribe` are subscribed as event handlers, and a list of
      Deferreds/Futures is returned that each resolves or rejects as above.

      :param handler: The event handler or handler object to receive events.
      :type handler: callable or obj
      :param topic: When ``handler`` is a single event handler, the URI (or URI pattern)
                    of the topic to subscribe to. When ``handler`` is an event handler
                    object, this value is ignored (and should be ``None``).
      :type topic: str
      :param options: Options for subscribing.
      :type options: An instance of :class:`autobahn.wamp.types.SubscribeOptions`.

      :returns: obj -- A (list of) Deferred(s)/Future(s) for the subscription(s) -
                       instance(s) of :class:`twisted.internet.defer.Deferred` (when
                       running under Twisted) or instance(s) of :class:`asyncio.Future`
                       (when running under asyncio).
      """



@six.add_metaclass(abc.ABCMeta)
class IRouterBase(object):

   @abc.abstractproperty
   def factory(self):
      """
      The router factory this router was created from.
      """


   @abc.abstractproperty
   def realm(self):
      """
      The WAMP realm this router handles.
      """


   @abc.abstractmethod
   def attach(self, session):
      """
      Attach a WAMP application session to this router.

      :param session: Application session to add.
      :type session: An instance that implements :class:`autobahn.wamp.interfaces.ISession`
      """


   @abc.abstractmethod
   def detach(self, session):
      """
      Detach a WAMP application session from this router.

      :param session: Application session to remove.
      :type session: An instance that implements :class:`autobahn.wamp.interfaces.ISession`
      """



class IRouter(IRouterBase):
   """
   WAMP router interface. Routers are responsible for event and call routing.
   """
   ACTION_CALL = 1
   ACTION_REGISTER = 2
   ACTION_PUBLISH = 3
   ACTION_SUBSCRIBE = 4

   ACTION_TO_STRING = {
      ACTION_CALL: 'call',
      ACTION_REGISTER: 'register',
      ACTION_PUBLISH: 'publish',
      ACTION_SUBSCRIBE: 'subscribe'
   }


   @abc.abstractmethod
   def process(self, session, message):
      """
      Process a WAMP message received on the given session.

      :param session: Application session on which the message was received.
      :type session: A provider of :class:`autobahn.wamp.interfaces.ISession`.
      :param message: The WAMP message to be processed.
      :type message: A provider of :class:`autobahn.wamp.interfaces.IMessage`.
      """


   @abc.abstractmethod
   def authorize(self, session, uri, action):
      """
      Authorization hook: check if the given ``session`` is authorized to perform
      the given ``action`` on the given ``uri``.

      :param session: Application session on which the action is to be authorized.
      :type session: A provider of :class:`autobahn.wamp.interfaces.ISession`.
      :param uri: The URI on which the session wants to perform the action.
      :type uri: str
      :param action: The action the session wants to perform. One of
         ``IRouter.ACTION_CALL``, ``IRouter.ACTION_REGISTER``,
         ``IRouter.ACTION_PUBLISH`` or ``IRouter.ACTION_SUBSCRIBE``.
      :type action: int
      """


   @abc.abstractmethod
   def validate(self, payload_type, uri, args, kwargs):
      """
      Validation hook: check if the given payload (``args`` and ``kwargs``) is
      valid for the given URI and payload type.

      :param uri: The URI on which the session wants to perform the action.
      :type uri: str
      :param payload_type: The payload type to be validated. One of ``["event", "call", "call_result", "call_error"]``
      :type payload_type: str
      :param args: The positional payload to be validated.
      :type args: list
      :param kwargs: The keyword payload to be validated.
      :type kwargs: dict
      """



class IBroker(IRouterBase):
   """
   WAMP broker interface. Brokers are responsible for event routing
   """

   @abc.abstractmethod
   def processPublish(self, session, publish):
      """
      Process a WAMP ``PUBLISH`` message received from a WAMP client.

      :param session: Application session on which the message was received.
      :type session: A provider of :class:`autobahn.wamp.interfaces.ISession`.
      :param publish: The WAMP ``PUBLISH`` message to be processed.
      :type publish: Instance of :class:`autobahn.wamp.message.Publish`.
      """


   @abc.abstractmethod
   def processSubscribe(self, session, subscribe):
      """
      Process a WAMP ``SUBSCRIBE`` message received from a WAMP client.

      :param session: Application session on which the message was received.
      :type session: A provider of :class:`autobahn.wamp.interfaces.ISession`.
      :param publish: The WAMP ``SUBSCRIBE`` message to be processed.
      :type publish: Instance of :class:`autobahn.wamp.message.Subscribe`.
      """


   @abc.abstractmethod
   def processUnsubscribe(self, session, unsubscribe):
      """
      Process a WAMP ``UNSUBSCRIBE`` message received from a WAMP client.

      :param session: Application session on which the message was received.
      :type session: A provider of :class:`autobahn.wamp.interfaces.ISession`.
      :param publish: The WAMP ``UNSUBSCRIBE`` message to be processed.
      :type publish: Instance of :class:`autobahn.wamp.message.Unsubscribe`.
      """



class IDealer(IRouterBase):
   """
   WAMP dealer interface. Dealers are responsible for call routing.
   """

   @abc.abstractmethod
   def processRegister(self, session, register):
      """
      Process a WAMP ``REGISTER`` message received from a WAMP client.

      :param session: Application session on which the message was received.
      :type session: A provider of :class:`autobahn.wamp.interfaces.ISession`.
      :param publish: The WAMP ``REGISTER`` message to be processed.
      :type publish: Instance of :class:`autobahn.wamp.message.Register`.
      """


   @abc.abstractmethod
   def processUnregister(self, session, unregister):
      """
      Process a WAMP ``UNREGISTER`` message received from a WAMP client.

      :param session: Application session on which the message was received.
      :type session: A provider of :class:`autobahn.wamp.interfaces.ISession`.
      :param publish: The WAMP ``UNREGISTER`` message to be processed.
      :type publish: Instance of :class:`autobahn.wamp.message.Unregister`.
      """


   @abc.abstractmethod
   def processCall(self, session, call):
      """
      Process a WAMP ``CALL`` message received from a WAMP client.

      :param session: Application session on which the message was received.
      :type session: A provider of :class:`autobahn.wamp.interfaces.ISession`.
      :param publish: The WAMP ``CALL`` message to be processed.
      :type publish: Instance of :class:`autobahn.wamp.message.Call`.
      """


   @abc.abstractmethod
   def processCancel(self, session, cancel):
      """
      Process a WAMP ``CANCEL`` message received from a WAMP client.

      :param session: Application session on which the message was received.
      :type session: A provider of :class:`autobahn.wamp.interfaces.ISession`.
      :param publish: The WAMP ``CANCEL`` message to be processed.
      :type publish: Instance of :class:`autobahn.wamp.message.Cancel`.
      """


   @abc.abstractmethod
   def processYield(self, session, yield_):
      """
      Process a WAMP ``YIELD`` message received from a WAMP client.

      :param session: Application session on which the message was received.
      :type session: A provider of :class:`autobahn.wamp.interfaces.ISession`.
      :param publish: The WAMP ``YIELD`` message to be processed.
      :type publish: Instance of :class:`autobahn.wamp.message.Yield`.
      """


   @abc.abstractmethod
   def processInvocationError(self, session, error):
      """
      Process a WAMP ``ERROR`` message received from a WAMP client.

      :param session: Application session on which the message was received.
      :type session: A provider of :class:`autobahn.wamp.interfaces.ISession`.
      :param publish: The WAMP ``ERROR`` message to be processed.
      :type publish: Instance of :class:`autobahn.wamp.message.Error`.
      """



@six.add_metaclass(abc.ABCMeta)
class IRouterFactory(object):

   @abc.abstractmethod
   def get(self, realm):
      """
      Get router for responsible for given realm.
      """
