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

import abc
import six

from autobahn.util import public

__all__ = (
    'IObjectSerializer',
    'ISerializer',
    'IMessage',
    'ITransport',
    'ITransportHandler',
    'ISession',
    'IPayloadCodec'
)


@public
@six.add_metaclass(abc.ABCMeta)
class IObjectSerializer(object):
    """
    Raw Python object serialization and deserialization. Object serializers are
    used by classes implementing WAMP serializers, that is instances of
    :class:`autobahn.wamp.interfaces.ISerializer`.
    """

    @public
    @abc.abstractproperty
    def BINARY(self):
        """
        Flag (read-only) to indicate if serializer requires a binary clean
        transport or if UTF8 transparency is sufficient.
        """

    @public
    @abc.abstractmethod
    def serialize(self, obj):
        """
        Serialize an object to a byte string.

        :param obj: Object to serialize.
        :type obj: any (serializable type)

        :returns: Serialized bytes.
        :rtype: bytes
        """

    @public
    @abc.abstractmethod
    def unserialize(self, payload):
        """
        Unserialize objects from a byte string.

        :param payload: Objects to unserialize.
        :type payload: bytes

        :returns: List of (raw) objects unserialized.
        :rtype: list
        """


@public
@six.add_metaclass(abc.ABCMeta)
class ISerializer(object):
    """
    WAMP message serialization and deserialization.
    """

    @public
    @abc.abstractproperty
    def MESSAGE_TYPE_MAP(self):
        """
        Mapping of WAMP message type codes to WAMP message classes.
        """

    @public
    @abc.abstractproperty
    def SERIALIZER_ID(self):
        """
        The WAMP serialization format ID.
        """

    @public
    @abc.abstractmethod
    def serialize(self, message):
        """
        Serializes a WAMP message to bytes for sending over a transport.

        :param message: The WAMP message to be serialized.
        :type message: object implementing :class:`autobahn.wamp.interfaces.IMessage`

        :returns: A pair ``(payload, isBinary)``.
        :rtype: tuple
        """

    @public
    @abc.abstractmethod
    def unserialize(self, payload, isBinary):
        """
        Deserialize bytes from a transport and parse into WAMP messages.

        :param payload: Byte string from wire.
        :type payload: bytes

        :param is_binary: Type of payload. True if payload is a binary string, else
            the payload is UTF-8 encoded Unicode text.
        :type is_binary: bool

        :returns: List of ``a.w.m.Message`` objects.
        :rtype: list
        """


@public
@six.add_metaclass(abc.ABCMeta)
class IMessage(object):
    """
    """

    @public
    @abc.abstractproperty
    def MESSAGE_TYPE(self):
        """
        WAMP message type code.
        """

    # the following requires Python 3.3+ and exactly this order of decorators
    # http://stackoverflow.com/questions/4474395/staticmethod-and-abc-abstractmethod-will-it-blend
    @public
    @staticmethod
    @abc.abstractmethod
    def parse(wmsg):
        """
        Factory method that parses a unserialized raw message (as returned byte
        :func:`autobahn.interfaces.ISerializer.unserialize`) into an instance
        of this class.

        :returns: The parsed WAMP message.
        :rtype: object implementing :class:`autobahn.wamp.interfaces.IMessage`
        """

    @public
    @abc.abstractmethod
    def serialize(self, serializer):
        """
        Serialize this object into a wire level bytes representation and cache
        the resulting bytes. If the cache already contains an entry for the given
        serializer, return the cached representation directly.

        :param serializer: The wire level serializer to use.
        :type serializer: object implementing :class:`autobahn.wamp.interfaces.ISerializer`

        :returns: The serialized bytes.
        :rtype: bytes
        """

    @public
    @abc.abstractmethod
    def uncache(self):
        """
        Resets the serialization cache for this message.
        """


@public
@six.add_metaclass(abc.ABCMeta)
class ITransport(object):
    """
    A WAMP transport is a bidirectional, full-duplex, reliable, ordered,
    message-based channel.
    """

    @public
    @abc.abstractmethod
    def send(self, message):
        """
        Send a WAMP message over the transport to the peer. If the transport is
        not open, this raises :class:`autobahn.wamp.exception.TransportLost`.
        Returns a deferred/future when the message has been processed and more
        messages may be sent. When send() is called while a previous deferred/future
        has not yet fired, the send will fail immediately.

        :param message: The WAMP message to send over the transport.
        :type message: object implementing :class:`autobahn.wamp.interfaces.IMessage`

        :returns: obj -- A Deferred/Future
        """

    @public
    @abc.abstractmethod
    def isOpen(self):
        """
        Check if the transport is open for messaging.

        :returns: ``True``, if the transport is open.
        :rtype: bool
        """

    @public
    @abc.abstractmethod
    def close(self):
        """
        Close the transport regularly. The transport will perform any
        closing handshake if applicable. This should be used for any
        application initiated closing.
        """

    @public
    @abc.abstractmethod
    def abort(self):
        """
        Abort the transport abruptly. The transport will be destroyed as
        fast as possible, and without playing nice to the peer. This should
        only be used in case of fatal errors, protocol violations or possible
        detected attacks.
        """

    @public
    @abc.abstractmethod
    def get_channel_id(self):
        """
        Return the unique channel ID of the underlying transport. This is used to
        mitigate credential forwarding man-in-the-middle attacks when running
        application level authentication (eg WAMP-cryptosign) which are decoupled
        from the underlying transport.

        The channel ID is only available when running over TLS (either WAMP-WebSocket
        or WAMP-RawSocket). It is not available for non-TLS transports (plain TCP or
        Unix domain sockets). It is also not available for WAMP-over-HTTP/Longpoll.
        Further, it is currently unimplemented for asyncio (only works on Twisted).

        The channel ID is computed as follows:

           - for a client, the SHA256 over the "TLS Finished" message sent by the client
             to the server is returned.

           - for a server, the SHA256 over the "TLS Finished" message the server expected
             the client to send

        Note: this is similar to `tls-unique` as described in RFC5929, but instead
        of returning the raw "TLS Finished" message, it returns a SHA256 over such a
        message. The reason is that we use the channel ID mainly with WAMP-cryptosign,
        which is based on Ed25519, where keys are always 32 bytes. And having a channel ID
        which is always 32 bytes (independent of the TLS ciphers/hashfuns in use) allows
        use to easily XOR channel IDs with Ed25519 keys and WAMP-cryptosign challenges.

        WARNING: For safe use of this (that is, for safely binding app level authentication
        to the underlying transport), you MUST use TLS, and you SHOULD deactivate both
        TLS session renegotiation and TLS session resumption.

        References:

           - https://tools.ietf.org/html/rfc5056
           - https://tools.ietf.org/html/rfc5929
           - http://www.pyopenssl.org/en/stable/api/ssl.html#OpenSSL.SSL.Connection.get_finished
           - http://www.pyopenssl.org/en/stable/api/ssl.html#OpenSSL.SSL.Connection.get_peer_finished

        :returns: The channel ID (if available) of the underlying WAMP transport. The
            channel ID is a 32 bytes value.
        :rtype: binary or None
        """


@public
@six.add_metaclass(abc.ABCMeta)
class ITransportHandler(object):

    @public
    @abc.abstractproperty
    def transport(self):
        """
        When the transport this handler is attached to is currently open, this property
        can be read from. The property should be considered read-only. When the transport
        is gone, this property is set to None.
        """

    @public
    @abc.abstractmethod
    def onOpen(self, transport):
        """
        Callback fired when transport is open. May run asynchronously. The transport
        is considered running and is_open() would return true, as soon as this callback
        has completed successfully.

        :param transport: The WAMP transport.
        :type transport: object implementing :class:`autobahn.wamp.interfaces.ITransport`
        """

    @public
    @abc.abstractmethod
    def onMessage(self, message):
        """
        Callback fired when a WAMP message was received. May run asynchronously. The callback
        should return or fire the returned deferred/future when it's done processing the message.
        In particular, an implementation of this callback must not access the message afterwards.

        :param message: The WAMP message received.
        :type message: object implementing :class:`autobahn.wamp.interfaces.IMessage`
        """

    @public
    @abc.abstractmethod
    def onClose(self, wasClean):
        """
        Callback fired when the transport has been closed.

        :param wasClean: Indicates if the transport has been closed regularly.
        :type wasClean: bool
        """


@public
@six.add_metaclass(abc.ABCMeta)
class ISession(object):
    """
    Interface for WAMP sessions.
    """

    @abc.abstractmethod
    def __init__(self, config=None):
        """

        :param config: Configuration for session.
        :type config: instance of :class:`autobahn.wamp.types.ComponentConfig`.
        """

    @public
    @abc.abstractmethod
    def onUserError(self, fail, msg):
        """
        This is called when we try to fire a callback, but get an
        exception from user code -- for example, a registered publish
        callback or a registered method. By default, this prints the
        current stack-trace and then error-message to stdout.

        ApplicationSession-derived objects may override this to
        provide logging if they prefer. The Twisted implemention does
        this. (See :class:`autobahn.twisted.wamp.ApplicationSession`)

        :param fail: The failure that occurred.
        :type fail: instance implementing txaio.IFailedFuture

        :param msg: an informative message from the library. It is
            suggested you log this immediately after the exception.
        :type msg: str
        """

    @public
    @abc.abstractmethod
    def onConnect(self):
        """
        Callback fired when the transport this session will run over has been established.
        """

    @public
    @abc.abstractmethod
    def join(self,
             realm,
             authmethods=None,
             authid=None,
             authrole=None,
             authextra=None,
             resumable=None,
             resume_session=None,
             resume_token=None):
        """
        Attach the session to the given realm. A session is open as soon as it is attached to a realm.
        """

    @public
    @abc.abstractmethod
    def onChallenge(self, challenge):
        """
        Callback fired when the peer demands authentication.

        May return a Deferred/Future.

        :param challenge: The authentication challenge.
        :type challenge: Instance of :class:`autobahn.wamp.types.Challenge`.
        """

    @public
    @abc.abstractmethod
    def onJoin(self, details):
        """
        Callback fired when WAMP session has been established.

        May return a Deferred/Future.

        :param details: Session information.
        :type details: Instance of :class:`autobahn.wamp.types.SessionDetails`.
        """

    @public
    @abc.abstractmethod
    def leave(self, reason=None, message=None):
        """
        Actively close this WAMP session.

        :param reason: An optional URI for the closing reason. If you
            want to permanently log out, this should be `wamp.close.logout`
        :type reason: str

        :param message: An optional (human readable) closing message, intended for
                        logging purposes.
        :type message: str

        :return: may return a Future/Deferred that fires when we've disconnected
        """

    @public
    @abc.abstractmethod
    def onLeave(self, details):
        """
        Callback fired when WAMP session has is closed

        :param details: Close information.
        :type details: Instance of :class:`autobahn.wamp.types.CloseDetails`.
        """

    @public
    @abc.abstractmethod
    def disconnect(self):
        """
        Close the underlying transport.
        """

    @public
    @abc.abstractmethod
    def onDisconnect(self):
        """
        Callback fired when underlying transport has been closed.
        """

    @public
    @abc.abstractmethod
    def is_connected(self):
        """
        Check if the underlying transport is connected.
        """

    @public
    @abc.abstractmethod
    def is_attached(self):
        """
        Check if the session has currently joined a realm.
        """

    @public
    @abc.abstractmethod
    def set_payload_codec(self, payload_codec):
        """
        Set a payload codec on the session. To remove a previously set payload codec,
        set the codec to ``None``.

        Payload codecs are used with WAMP payload transparency mode.

        :param payload_codec: The payload codec that should process application
            payload of the given encoding.
        :type payload_codec: object
            implementing :class:`autobahn.wamp.interfaces.IPayloadCodec` or ``None``
        """

    @public
    @abc.abstractmethod
    def get_payload_codec(self):
        """
        Get the current payload codec (if any) for the session.

        Payload codecs are used with WAMP payload transparency mode.

        :returns: The current payload codec or ``None`` if no codec is active.
        :rtype: object implementing
            :class:`autobahn.wamp.interfaces.IPayloadCodec` or ``None``
        """

    @public
    @abc.abstractmethod
    def define(self, exception, error=None):
        """
        Defines an exception for a WAMP error in the context of this WAMP session.

        :param exception: The exception class to define an error mapping for.
        :type exception: A class that derives of ``Exception``.

        :param error: The URI (or URI pattern) the exception class should be mapped for.
            Iff the ``exception`` class is decorated, this must be ``None``.
        :type error: str
        """

    @public
    @abc.abstractmethod
    def call(self, procedure, *args, **kwargs):
        """
        Call a remote procedure.

        This will return a Deferred/Future, that when resolved, provides the actual result
        returned by the called remote procedure.

        - If the result is a single positional return value, it'll be returned "as-is".

        - If the result contains multiple positional return values or keyword return values,
          the result is wrapped in an instance of :class:`autobahn.wamp.types.CallResult`.

        - If the call fails, the returned Deferred/Future will be rejected with an instance
          of :class:`autobahn.wamp.exception.ApplicationError`.

        If ``kwargs`` contains an ``options`` keyword argument that is an instance of
        :class:`autobahn.wamp.types.CallOptions`, this will provide specific options for
        the call to perform.

        When the *Caller* and *Dealer* implementations support canceling of calls, the call may
        be canceled by canceling the returned Deferred/Future.

        :param procedure: The URI of the remote procedure to be called, e.g. ``u"com.myapp.hello"``.
        :type procedure: unicode

        :param args: Any positional arguments for the call.
        :type args: list

        :param kwargs: Any keyword arguments for the call.
        :type kwargs: dict

        :returns: A Deferred/Future for the call result -
        :rtype: instance of :tx:`twisted.internet.defer.Deferred` / :py:class:`asyncio.Future`
        """

    @public
    @abc.abstractmethod
    def register(self, endpoint, procedure=None, options=None):
        """
        Register a procedure for remote calling.

        When ``endpoint`` is a callable (function, method or object that implements ``__call__``),
        then ``procedure`` must be provided and an instance of
        :tx:`twisted.internet.defer.Deferred` (when running on **Twisted**) or an instance
        of :py:class:`asyncio.Future` (when running on **asyncio**) is returned.

        - If the registration *succeeds* the returned Deferred/Future will *resolve* to
          an object that implements :class:`autobahn.wamp.interfaces.IRegistration`.

        - If the registration *fails* the returned Deferred/Future will *reject* with an
          instance of :class:`autobahn.wamp.exception.ApplicationError`.

        When ``endpoint`` is an object, then each of the object's methods that is decorated
        with :func:`autobahn.wamp.register` is automatically registered and a (single)
        DeferredList or Future is returned that gathers all individual underlying Deferreds/Futures.

        :param endpoint: The endpoint called under the procedure.
        :type endpoint: callable or object

        :param procedure: When ``endpoint`` is a callable, the URI (or URI pattern)
           of the procedure to register for. When ``endpoint`` is an object,
           the argument is ignored (and should be ``None``).
        :type procedure: unicode

        :param options: Options for registering.
        :type options: instance of :class:`autobahn.wamp.types.RegisterOptions`.

        :returns: A registration or a list of registrations (or errors)
        :rtype: instance(s) of :tx:`twisted.internet.defer.Deferred` / :py:class:`asyncio.Future`
        """

    @public
    @abc.abstractmethod
    def publish(self, topic, *args, **kwargs):
        """
        Publish an event to a topic.

        If ``kwargs`` contains an ``options`` keyword argument that is an instance of
        :class:`autobahn.wamp.types.PublishOptions`, this will provide
        specific options for the publish to perform.

        .. note::
           By default, publications are non-acknowledged and the publication can
           fail silently, e.g. because the session is not authorized to publish
           to the topic.

        When publication acknowledgement is requested via ``options.acknowledge == True``,
        this function returns a Deferred/Future:

        - If the publication succeeds the Deferred/Future will resolve to an object
          that implements :class:`autobahn.wamp.interfaces.IPublication`.

        - If the publication fails the Deferred/Future will reject with an instance
          of :class:`autobahn.wamp.exception.ApplicationError`.

        :param topic: The URI of the topic to publish to, e.g. ``u"com.myapp.mytopic1"``.
        :type topic: unicode

        :param args: Arbitrary application payload for the event (positional arguments).
        :type args: list

        :param kwargs: Arbitrary application payload for the event (keyword arguments).
        :type kwargs: dict

        :returns: Acknowledgement for acknowledge publications - otherwise nothing.
        :rtype: ``None`` or instance of :tx:`twisted.internet.defer.Deferred` / :py:class:`asyncio.Future`
        """

    @public
    @abc.abstractmethod
    def subscribe(self, handler, topic=None, options=None):
        """
        Subscribe to a topic for receiving events.

        When ``handler`` is a callable (function, method or object that implements ``__call__``),
        then `topic` must be provided and an instance of
        :tx:`twisted.internet.defer.Deferred` (when running on **Twisted**) or an instance
        of :class:`asyncio.Future` (when running on **asyncio**) is returned.

        - If the subscription succeeds the Deferred/Future will resolve to an object
          that implements :class:`autobahn.wamp.interfaces.ISubscription`.

        - If the subscription fails the Deferred/Future will reject with an instance
          of :class:`autobahn.wamp.exception.ApplicationError`.

        When ``handler`` is an object, then each of the object's methods that is decorated
        with :func:`autobahn.wamp.subscribe` is automatically subscribed as event handlers,
        and a list of Deferreds/Futures is returned that each resolves or rejects as above.

        :param handler: The event handler to receive events.
        :type handler: callable or object

        :param topic: When ``handler`` is a callable, the URI (or URI pattern)
           of the topic to subscribe to. When ``handler`` is an object, this
           value is ignored (and should be ``None``).
        :type topic: unicode

        :param options: Options for subscribing.
        :type options: An instance of :class:`autobahn.wamp.types.SubscribeOptions`.

        :returns: A single Deferred/Future or a list of such objects
        :rtype: instance(s) of :tx:`twisted.internet.defer.Deferred` / :py:class:`asyncio.Future`
        """


# experimental authentication API
@six.add_metaclass(abc.ABCMeta)
class IAuthenticator(object):

    @abc.abstractmethod
    def on_challenge(self, session, challenge):
        """
        """


@public
@six.add_metaclass(abc.ABCMeta)
class IPayloadCodec(object):
    """
    WAMP payload codecs are used with WAMP payload transparency mode.

    In payload transparency mode, application payloads are transmitted "raw",
    as binary strings, without any processing at the WAMP router.

    Payload transparency can be used eg for these use cases:

    * end-to-end encryption of application payloads (WAMP-cryptobox)
    * using serializers with custom user types, where the serializer and
      the serializer implementation has native support for serializing
      custom types (such as CBOR)
    * transmitting MQTT payloads within WAMP, when the WAMP router is
      providing a MQTT-WAMP bridge
    """

    @public
    @abc.abstractmethod
    def encode(self, is_originating, uri, args=None, kwargs=None):
        """
        Encodes application payload.

        :param is_originating: Flag indicating whether the encoding
            is to be done from an originator (a caller or publisher).
        :type is_originating: bool

        :param uri: The WAMP URI associated with the WAMP message for which
            the payload is to be encoded (eg topic or procedure).
        :type uri: str

        :param args: Positional application payload.
        :type args: list or None

        :param kwargs: Keyword-based application payload.
        :type kwargs: dict or None

        :returns: The encoded application payload or None to
            signal no encoding should be used.
        :rtype: instance of :class:`autobahn.wamp.types.EncodedPayload`
        """

    @public
    @abc.abstractmethod
    def decode(self, is_originating, uri, encoded_payload):
        """
        Decode application payload.

        :param is_originating: Flag indicating whether the encoding
            is to be done from an originator (a caller or publisher).
        :type is_originating: bool

        :param uri: The WAMP URI associated with the WAMP message for which
            the payload is to be encoded (eg topic or procedure).
        :type uri: str

        :param payload: The encoded application payload to be decoded.
        :type payload: instance of :class:`autobahn.wamp.types.EncodedPayload`

        :returns: A tuple with the decoded positional and keyword-based
            application payload: ``(uri, args, kwargs)``
        :rtype: tuple
        """
