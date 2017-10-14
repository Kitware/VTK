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

from __future__ import absolute_import

import six
import txaio
import inspect
from functools import reduce

from autobahn import wamp
from autobahn.util import public, IdGenerator, ObservableMixin
from autobahn.wamp import uri
from autobahn.wamp import message
from autobahn.wamp import types
from autobahn.wamp import role
from autobahn.wamp import exception
from autobahn.wamp.exception import ApplicationError, ProtocolError, SessionNotReady, SerializationError
from autobahn.wamp.interfaces import ISession, IPayloadCodec, IAuthenticator  # noqa
from autobahn.wamp.types import SessionDetails, CloseDetails, EncodedPayload
from autobahn.wamp.request import \
    Publication, \
    Subscription, \
    Handler, \
    Registration, \
    Endpoint, \
    PublishRequest, \
    SubscribeRequest, \
    UnsubscribeRequest, \
    CallRequest, \
    InvocationRequest, \
    RegisterRequest, \
    UnregisterRequest


def is_method_or_function(f):
    return inspect.ismethod(f) or inspect.isfunction(f)


class BaseSession(ObservableMixin):
    """
    WAMP session base class.

    This class implements :class:`autobahn.wamp.interfaces.ISession`.
    """

    def __init__(self):
        """

        """
        self.set_valid_events(
            valid_events=[
                'join',         # right before onJoin runs
                'leave',        # after onLeave has run
                'ready',        # after onJoin and all 'join' listeners have completed
                'connect',      # right before onConnect
                'disconnect',   # right after onDisconnect
            ]
        )

        # this is for marshalling traceback from exceptions thrown in user
        # code within WAMP error messages (that is, when invoking remoted
        # procedures)
        self.traceback_app = False

        # mapping of exception classes to WAMP error URIs
        self._ecls_to_uri_pat = {}

        # mapping of WAMP error URIs to exception classes
        self._uri_to_ecls = {
            ApplicationError.INVALID_PAYLOAD: SerializationError
        }

        # session authentication information
        self._authid = None
        self._authrole = None
        self._authmethod = None
        self._authprovider = None

        # payload transparency codec
        self._payload_codec = None

        # generator for WAMP request IDs
        self._request_id_gen = IdGenerator()

    def define(self, exception, error=None):
        """
        Implements :func:`autobahn.wamp.interfaces.ISession.define`
        """
        if error is None:
            assert(hasattr(exception, '_wampuris'))
            self._ecls_to_uri_pat[exception] = exception._wampuris
            self._uri_to_ecls[exception._wampuris[0].uri()] = exception
        else:
            assert(not hasattr(exception, '_wampuris'))
            self._ecls_to_uri_pat[exception] = [uri.Pattern(six.u(error), uri.Pattern.URI_TARGET_HANDLER)]
            self._uri_to_ecls[six.u(error)] = exception

    def _message_from_exception(self, request_type, request, exc, tb=None, enc_algo=None):
        """
        Create a WAMP error message from an exception.

        :param request_type: The request type this WAMP error message is for.
        :type request_type: int

        :param request: The request ID this WAMP error message is for.
        :type request: int

        :param exc: The exception.
        :type exc: Instance of :class:`Exception` or subclass thereof.

        :param tb: Optional traceback. If present, it'll be included with the WAMP error message.
        :type tb: list or None
        """
        args = None
        if hasattr(exc, 'args'):
            args = list(exc.args)  # make sure tuples are made into lists

        kwargs = None
        if hasattr(exc, 'kwargs'):
            kwargs = exc.kwargs

        if tb:
            if kwargs:
                kwargs['traceback'] = tb
            else:
                kwargs = {'traceback': tb}

        if isinstance(exc, exception.ApplicationError):
            error = exc.error if type(exc.error) == six.text_type else six.u(exc.error)
        else:
            if exc.__class__ in self._ecls_to_uri_pat:
                error = self._ecls_to_uri_pat[exc.__class__][0]._uri
            else:
                error = u"wamp.error.runtime_error"

        encoded_payload = None
        if self._payload_codec:
            encoded_payload = self._payload_codec.encode(False, error, args, kwargs)

        if encoded_payload:
            msg = message.Error(request_type,
                                request,
                                error,
                                payload=encoded_payload.payload,
                                enc_algo=encoded_payload.enc_algo,
                                enc_key=encoded_payload.enc_key,
                                enc_serializer=encoded_payload.enc_serializer)
        else:
            msg = message.Error(request_type,
                                request,
                                error,
                                args,
                                kwargs)

        return msg

    def _exception_from_message(self, msg):
        """
        Create a user (or generic) exception from a WAMP error message.

        :param msg: A WAMP error message.
        :type msg: instance of :class:`autobahn.wamp.message.Error`
        """

        # FIXME:
        # 1. map to ecls based on error URI wildcard/prefix
        # 2. extract additional args/kwargs from error URI

        exc = None
        enc_err = None

        if msg.enc_algo:

            if not self._payload_codec:
                log_msg = u"received encoded payload, but no payload codec active"
                self.log.warn(log_msg)
                enc_err = ApplicationError(ApplicationError.ENC_NO_PAYLOAD_CODEC, log_msg, enc_algo=msg.enc_algo)
            else:
                try:
                    encoded_payload = EncodedPayload(msg.payload, msg.enc_algo, msg.enc_serializer, msg.enc_key)
                    decrypted_error, msg.args, msg.kwargs = self._payload_codec.decode(True, msg.error, encoded_payload)
                except Exception as e:
                    self.log.warn("failed to decrypt application payload 1: {err}", err=e)
                    enc_err = ApplicationError(
                        ApplicationError.ENC_DECRYPT_ERROR,
                        u"failed to decrypt application payload 1: {}".format(e),
                        enc_algo=msg.enc_algo,
                    )
                else:
                    if msg.error != decrypted_error:
                        self.log.warn(
                            u"URI within encrypted payload ('{decrypted_error}') does not match the envelope ('{error}')",
                            decrypted_error=decrypted_error,
                            error=msg.error,
                        )
                        enc_err = ApplicationError(
                            ApplicationError.ENC_TRUSTED_URI_MISMATCH,
                            u"URI within encrypted payload ('{}') does not match the envelope ('{}')".format(decrypted_error, msg.error),
                            enc_algo=msg.enc_algo,
                        )

        if enc_err:
            return enc_err

        if msg.error in self._uri_to_ecls:
            ecls = self._uri_to_ecls[msg.error]
            try:
                # the following might fail, eg. TypeError when
                # signature of exception constructor is incompatible
                # with args/kwargs or when the exception constructor raises
                if msg.kwargs:
                    if msg.args:
                        exc = ecls(*msg.args, **msg.kwargs)
                    else:
                        exc = ecls(**msg.kwargs)
                else:
                    if msg.args:
                        exc = ecls(*msg.args)
                    else:
                        exc = ecls()
            except Exception:
                try:
                    self.onUserError(
                        txaio.create_failure(),
                        "While re-constructing exception",
                    )
                except:
                    pass

        if not exc:
            # the following ctor never fails ..
            if msg.kwargs:
                if msg.args:
                    exc = exception.ApplicationError(msg.error, *msg.args, **msg.kwargs)
                else:
                    exc = exception.ApplicationError(msg.error, **msg.kwargs)
            else:
                if msg.args:
                    exc = exception.ApplicationError(msg.error, *msg.args)
                else:
                    exc = exception.ApplicationError(msg.error)

        if hasattr(exc, 'enc_algo'):
            exc.enc_algo = msg.enc_algo

        return exc


@public
class ApplicationSession(BaseSession):
    """
    WAMP endpoint session.
    """

    def __init__(self, config=None):
        """
        Implements :func:`autobahn.wamp.interfaces.ISession`
        """
        BaseSession.__init__(self)
        self.config = config or types.ComponentConfig(realm=u"default")

        self._transport = None
        self._session_id = None
        self._realm = None

        self._goodbye_sent = False
        self._transport_is_closing = False

        # outstanding requests
        self._publish_reqs = {}
        self._subscribe_reqs = {}
        self._unsubscribe_reqs = {}
        self._call_reqs = {}
        self._register_reqs = {}
        self._unregister_reqs = {}

        # subscriptions in place
        self._subscriptions = {}

        # registrations in place
        self._registrations = {}

        # incoming invocations
        self._invocations = {}

    @public
    def set_payload_codec(self, payload_codec):
        """
        Implements :func:`autobahn.wamp.interfaces.ISession.set_payload_codec`
        """
        assert(payload_codec is None or isinstance(payload_codec, IPayloadCodec))
        self._payload_codec = payload_codec

    @public
    def get_payload_codec(self):
        """
        Implements :func:`autobahn.wamp.interfaces.ISession.get_payload_codec`
        """
        return self._payload_codec

    @public
    def onOpen(self, transport):
        """
        Implements :func:`autobahn.wamp.interfaces.ITransportHandler.onOpen`
        """
        self._transport = transport
        d = self.fire('connect', self, transport)
        txaio.add_callbacks(
            d, None,
            lambda fail: self._swallow_error(fail, "While notifying 'connect'")
        )
        txaio.add_callbacks(
            d,
            lambda _: txaio.as_future(self.onConnect),
            None,
        )

    @public
    def onConnect(self):
        """
        Implements :func:`autobahn.wamp.interfaces.ISession.onConnect`
        """
        self.join(self.config.realm)

    @public
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
        Implements :func:`autobahn.wamp.interfaces.ISession.join`
        """
        assert(realm is None or type(realm) == six.text_type)
        assert(authmethods is None or type(authmethods) == list)
        if type(authmethods) == list:
            for authmethod in authmethods:
                assert(type(authmethod) == six.text_type)
        assert(authid is None or type(authid) == six.text_type)
        assert(authrole is None or type(authrole) == six.text_type)
        assert(authextra is None or type(authextra) == dict)

        if self._session_id:
            raise Exception("already joined")

        # store the realm requested by client, though this might be overwritten later,
        # when realm redirection kicks in
        self._realm = realm

        # closing handshake state
        self._goodbye_sent = False

        # send HELLO message to router
        msg = message.Hello(realm=realm,
                            roles=role.DEFAULT_CLIENT_ROLES,
                            authmethods=authmethods,
                            authid=authid,
                            authrole=authrole,
                            authextra=authextra,
                            resumable=resumable,
                            resume_session=resume_session,
                            resume_token=resume_token)
        self._transport.send(msg)

    @public
    def disconnect(self):
        """
        Implements :func:`autobahn.wamp.interfaces.ISession.disconnect`
        """
        if self._transport:
            self._transport.close()

    @public
    def is_connected(self):
        """
        Implements :func:`autobahn.wamp.interfaces.ISession.is_connected`
        """
        return self._transport is not None

    @public
    def is_attached(self):
        """
        Implements :func:`autobahn.wamp.interfaces.ISession.is_attached`
        """
        return self._transport is not None and self._session_id is not None

    @public
    def onUserError(self, fail, msg):
        """
        Implements :func:`autobahn.wamp.interfaces.ISession.onUserError`
        """
        if False and isinstance(fail.value, exception.ApplicationError):
            self.log.error(fail.value.error_message())
        else:
            self.log.error(
                u'{msg}: {traceback}',
                msg=msg,
                traceback=txaio.failure_format_traceback(fail),
            )

    def _swallow_error(self, fail, msg):
        '''
        This is an internal generic error-handler for errors encountered
        when calling down to on*() handlers that can reasonably be
        expected to be overridden in user code.

        Note that it *cancels* the error, so use with care!

        Specifically, this should *never* be added to the errback
        chain for a Deferred/coroutine that will make it out to user
        code.
        '''
        try:
            self.onUserError(fail, msg)
        except Exception:
            self.log.error(
                "Internal error: {tb}",
                tb=txaio.failure_format_traceback(txaio.create_failure()),
            )
        return None

    def onMessage(self, msg):
        """
        Implements :func:`autobahn.wamp.interfaces.ITransportHandler.onMessage`
        """

        if self._session_id is None:

            # the first message must be WELCOME, ABORT or CHALLENGE ..
            if isinstance(msg, message.Welcome):

                if msg.realm:
                    self._realm = msg.realm

                self._session_id = msg.session
                self._router_roles = msg.roles

                details = SessionDetails(realm=self._realm,
                                         session=self._session_id,
                                         authid=msg.authid,
                                         authrole=msg.authrole,
                                         authmethod=msg.authmethod,
                                         authprovider=msg.authprovider,
                                         authextra=msg.authextra,
                                         resumed=msg.resumed,
                                         resumable=msg.resumable,
                                         resume_token=msg.resume_token)
                # firing 'join' *before* running onJoin, so that the
                # idiom where you "do stuff" in onJoin -- possibly
                # including self.leave() -- works properly. Besides,
                # there's "ready" that fires after 'join' and onJoin
                # have all completed...
                d = self.fire('join', self, details)
                # add a logging errback first, which will ignore any
                # errors from fire()
                txaio.add_callbacks(
                    d, None,
                    lambda e: self._swallow_error(e, "While notifying 'join'")
                )
                # this should run regardless
                txaio.add_callbacks(
                    d,
                    lambda _: txaio.as_future(self.onJoin, details),
                    None
                )
                # ignore any errors from onJoin (XXX or, should that be fatal?)
                txaio.add_callbacks(
                    d, None,
                    lambda e: self._swallow_error(e, "While firing onJoin")
                )
                # this instance is now "ready"...
                txaio.add_callbacks(
                    d,
                    lambda _: self.fire('ready', self),
                    None
                )
                # ignore any errors from 'ready'
                txaio.add_callbacks(
                    d, None,
                    lambda e: self._swallow_error(e, "While notifying 'ready'")
                )

            elif isinstance(msg, message.Abort):

                # fire callback and close the transport
                details = types.CloseDetails(msg.reason, msg.message)
                d = txaio.as_future(self.onLeave, details)

                def success(arg):
                    # XXX also: handle async
                    d = self.fire('leave', self, details)

                    def return_arg(_):
                        return arg

                    def _error(e):
                        return self._swallow_error(e, "While firing 'leave' event")
                    txaio.add_callbacks(d, return_arg, _error)
                    return d

                def _error(e):
                    return self._swallow_error(e, "While firing onLeave")
                txaio.add_callbacks(d, success, _error)

            elif isinstance(msg, message.Challenge):

                challenge = types.Challenge(msg.method, msg.extra)
                d = txaio.as_future(self.onChallenge, challenge)

                def success(signature):
                    if signature is None:
                        raise Exception('onChallenge user callback did not return a signature')
                    if type(signature) == six.binary_type:
                        signature = signature.decode('utf8')
                    if type(signature) != six.text_type:
                        raise Exception('signature must be unicode (was {})'.format(type(signature)))
                    reply = message.Authenticate(signature)
                    self._transport.send(reply)

                def error(err):
                    self.onUserError(err, "Authentication failed")
                    reply = message.Abort(u"wamp.error.cannot_authenticate", u"{0}".format(err.value))
                    self._transport.send(reply)
                    # fire callback and close the transport
                    details = types.CloseDetails(reply.reason, reply.message)
                    d = txaio.as_future(self.onLeave, details)

                    def success(arg):
                        # XXX also: handle async
                        self.fire('leave', self, details)
                        return arg

                    def _error(e):
                        return self._swallow_error(e, "While firing onLeave")
                    txaio.add_callbacks(d, success, _error)
                    # switching to the callback chain, effectively
                    # cancelling error (which we've now handled)
                    return d

                txaio.add_callbacks(d, success, error)

            else:
                raise ProtocolError("Received {0} message, and session is not yet established".format(msg.__class__))

        else:
            # self._session_id != None (aka "session established")
            if isinstance(msg, message.Goodbye):
                if not self._goodbye_sent:
                    # the peer wants to close: send GOODBYE reply
                    reply = message.Goodbye()
                    self._transport.send(reply)

                self._session_id = None

                # fire callback and close the transport
                details = types.CloseDetails(msg.reason, msg.message)
                d = txaio.as_future(self.onLeave, details)

                def success(arg):
                    # XXX also: handle async
                    self.fire('leave', self, details)
                    return arg

                def _error(e):
                    errmsg = 'While firing onLeave for reason "{0}" and message "{1}"'.format(msg.reason, msg.message)
                    return self._swallow_error(e, errmsg)
                txaio.add_callbacks(d, success, _error)

            elif isinstance(msg, message.Event):

                if msg.subscription in self._subscriptions:

                    # fire all event handlers on subscription ..
                    for subscription in self._subscriptions[msg.subscription]:

                        handler = subscription.handler
                        topic = msg.topic or subscription.topic

                        if msg.enc_algo:
                            # FIXME: behavior in error cases (no keyring, decrypt issues, URI mismatch, ..)
                            if not self._payload_codec:
                                self.log.warn("received encoded payload with enc_algo={enc_algo}, but no payload codec active - ignoring encoded payload!", enc_algo=msg.enc_algo)
                                return
                            else:
                                try:
                                    encoded_payload = EncodedPayload(msg.payload, msg.enc_algo, msg.enc_serializer, msg.enc_key)
                                    decoded_topic, msg.args, msg.kwargs = self._payload_codec.decode(False, topic, encoded_payload)
                                except Exception as e:
                                    self.log.warn("failed to decode application payload encoded with enc_algo={enc_algo}: {error}", error=e, enc_algo=msg.enc_algo)
                                    return
                                else:
                                    if topic != decoded_topic:
                                        self.log.warn("envelope topic URI does not match encoded one")
                                        return

                        invoke_args = (handler.obj,) if handler.obj else tuple()
                        if msg.args:
                            invoke_args = invoke_args + tuple(msg.args)
                        invoke_kwargs = msg.kwargs if msg.kwargs else dict()

                        if handler.details_arg:
                            invoke_kwargs[handler.details_arg] = types.EventDetails(subscription, msg.publication, publisher=msg.publisher, publisher_authid=msg.publisher_authid, publisher_authrole=msg.publisher_authrole, topic=topic, retained=msg.retained, enc_algo=msg.enc_algo)

                        # FIXME: https://github.com/crossbario/autobahn-python/issues/764
                        def _success(_):
                            # Acknowledged Events -- only if we got the details header and
                            # the broker advertised it
                            if msg.x_acknowledged_delivery and self._router_roles["broker"].x_acknowledged_event_delivery:
                                if self._transport:
                                    response = message.EventReceived(msg.publication)
                                    self._transport.send(response)
                                else:
                                    self.log.warn("successfully processed event with acknowledged delivery, but could not send ACK, since the transport was lost in the meantime")

                        def _error(e):
                            errmsg = 'While firing {0} subscribed under {1}.'.format(
                                handler.fn, msg.subscription)
                            return self._swallow_error(e, errmsg)

                        future = txaio.as_future(handler.fn, *invoke_args, **invoke_kwargs)
                        txaio.add_callbacks(future, _success, _error)

                else:
                    raise ProtocolError("EVENT received for non-subscribed subscription ID {0}".format(msg.subscription))

            elif isinstance(msg, message.Published):

                if msg.request in self._publish_reqs:

                    # get and pop outstanding publish request
                    publish_request = self._publish_reqs.pop(msg.request)

                    # create a new publication object
                    publication = Publication(msg.publication, was_encrypted=publish_request.was_encrypted)

                    # resolve deferred/future for publishing successfully
                    txaio.resolve(publish_request.on_reply, publication)
                else:
                    raise ProtocolError("PUBLISHED received for non-pending request ID {0}".format(msg.request))

            elif isinstance(msg, message.Subscribed):

                if msg.request in self._subscribe_reqs:

                    # get and pop outstanding subscribe request
                    request = self._subscribe_reqs.pop(msg.request)

                    # create new handler subscription list for subscription ID if not yet tracked
                    if msg.subscription not in self._subscriptions:
                        self._subscriptions[msg.subscription] = []

                    subscription = Subscription(msg.subscription, request.topic, self, request.handler)

                    # add handler to existing subscription
                    self._subscriptions[msg.subscription].append(subscription)

                    # resolve deferred/future for subscribing successfully
                    txaio.resolve(request.on_reply, subscription)
                else:
                    raise ProtocolError("SUBSCRIBED received for non-pending request ID {0}".format(msg.request))

            elif isinstance(msg, message.Unsubscribed):

                if msg.request in self._unsubscribe_reqs:

                    # get and pop outstanding subscribe request
                    request = self._unsubscribe_reqs.pop(msg.request)

                    # if the subscription still exists, mark as inactive and remove ..
                    if request.subscription_id in self._subscriptions:
                        for subscription in self._subscriptions[request.subscription_id]:
                            subscription.active = False
                        del self._subscriptions[request.subscription_id]

                    # resolve deferred/future for unsubscribing successfully
                    txaio.resolve(request.on_reply, 0)
                else:
                    raise ProtocolError("UNSUBSCRIBED received for non-pending request ID {0}".format(msg.request))

            elif isinstance(msg, message.Result):

                if msg.request in self._call_reqs:

                    call_request = self._call_reqs[msg.request]
                    proc = call_request.procedure
                    enc_err = None

                    if msg.enc_algo:

                        if not self._payload_codec:
                            log_msg = u"received encoded payload, but no payload codec active"
                            self.log.warn(log_msg)
                            enc_err = ApplicationError(ApplicationError.ENC_NO_PAYLOAD_CODEC, log_msg)
                        else:
                            try:
                                encoded_payload = EncodedPayload(msg.payload, msg.enc_algo, msg.enc_serializer, msg.enc_key)
                                decrypted_proc, msg.args, msg.kwargs = self._payload_codec.decode(True, proc, encoded_payload)
                            except Exception as e:
                                self.log.warn(
                                    "failed to decrypt application payload 1: {err}",
                                    err=e,
                                )
                                enc_err = ApplicationError(
                                    ApplicationError.ENC_DECRYPT_ERROR,
                                    u"failed to decrypt application payload 1: {}".format(e),
                                )
                            else:
                                if proc != decrypted_proc:
                                    self.log.warn(
                                        "URI within encrypted payload ('{decrypted_proc}') does not match the envelope ('{proc}')",
                                        decrypted_proc=decrypted_proc,
                                        proc=proc,
                                    )
                                    enc_err = ApplicationError(
                                        ApplicationError.ENC_TRUSTED_URI_MISMATCH,
                                        u"URI within encrypted payload ('{}') does not match the envelope ('{}')".format(decrypted_proc, proc),
                                    )

                    if msg.progress:
                        # process progressive call result

                        if call_request.options.on_progress:
                            if enc_err:
                                self.onUserError(enc_err, "could not deliver progressive call result, because payload decryption failed")
                            else:
                                kw = msg.kwargs or dict()
                                args = msg.args or tuple()
                                try:
                                    # XXX what if on_progress returns a Deferred/Future?
                                    call_request.options.on_progress(*args, **kw)
                                except Exception:
                                    try:
                                        self.onUserError(txaio.create_failure(), "While firing on_progress")
                                    except:
                                        pass

                    else:
                        # process final call result

                        # drop original request
                        del self._call_reqs[msg.request]

                        # user callback that gets fired
                        on_reply = call_request.on_reply

                        # above might already have rejected, so we guard ..
                        if enc_err:
                            txaio.reject(on_reply, enc_err)
                        else:
                            if msg.kwargs:
                                if msg.args:
                                    res = types.CallResult(*msg.args, **msg.kwargs)
                                else:
                                    res = types.CallResult(**msg.kwargs)
                                txaio.resolve(on_reply, res)
                            else:
                                if msg.args:
                                    if len(msg.args) > 1:
                                        res = types.CallResult(*msg.args)
                                        txaio.resolve(on_reply, res)
                                    else:
                                        txaio.resolve(on_reply, msg.args[0])
                                else:
                                    txaio.resolve(on_reply, None)
                else:
                    raise ProtocolError("RESULT received for non-pending request ID {0}".format(msg.request))

            elif isinstance(msg, message.Invocation):

                if msg.request in self._invocations:

                    raise ProtocolError("INVOCATION received for request ID {0} already invoked".format(msg.request))

                else:

                    if msg.registration not in self._registrations:

                        raise ProtocolError("INVOCATION received for non-registered registration ID {0}".format(msg.registration))

                    else:
                        registration = self._registrations[msg.registration]
                        endpoint = registration.endpoint
                        proc = msg.procedure or registration.procedure
                        enc_err = None

                        if msg.enc_algo:
                            if not self._payload_codec:
                                log_msg = u"received encrypted INVOCATION payload, but no keyring active"
                                self.log.warn(log_msg)
                                enc_err = ApplicationError(ApplicationError.ENC_NO_PAYLOAD_CODEC, log_msg)
                            else:
                                try:
                                    encoded_payload = EncodedPayload(msg.payload, msg.enc_algo, msg.enc_serializer, msg.enc_key)
                                    decrypted_proc, msg.args, msg.kwargs = self._payload_codec.decode(False, proc, encoded_payload)
                                except Exception as e:
                                    self.log.warn(
                                        "failed to decrypt INVOCATION payload: {err}",
                                        err=e,
                                    )
                                    enc_err = ApplicationError(
                                        ApplicationError.ENC_DECRYPT_ERROR,
                                        "failed to decrypt INVOCATION payload: {}".format(e),
                                    )
                                else:
                                    if proc != decrypted_proc:
                                        self.log.warn(
                                            "URI within encrypted INVOCATION payload ('{decrypted_proc}') "
                                            "does not match the envelope ('{proc}')",
                                            decrypted_proc=decrypted_proc,
                                            proc=proc,
                                        )
                                        enc_err = ApplicationError(
                                            ApplicationError.ENC_TRUSTED_URI_MISMATCH,
                                            u"URI within encrypted INVOCATION payload ('{}') does not match the envelope ('{}')".format(decrypted_proc, proc),
                                        )

                        if enc_err:
                            # when there was a problem decrypting the INVOCATION payload, we obviously can't invoke
                            # the endpoint, but return and
                            reply = self._message_from_exception(message.Invocation.MESSAGE_TYPE, msg.request, enc_err)
                            self._transport.send(reply)

                        else:

                            if endpoint.obj is not None:
                                invoke_args = (endpoint.obj,)
                            else:
                                invoke_args = tuple()

                            if msg.args:
                                invoke_args = invoke_args + tuple(msg.args)

                            invoke_kwargs = msg.kwargs if msg.kwargs else dict()

                            if endpoint.details_arg:

                                if msg.receive_progress:

                                    def progress(*args, **kwargs):
                                        encoded_payload = None
                                        if msg.enc_algo:
                                            if not self._payload_codec:
                                                raise Exception(u"trying to send encrypted payload, but no keyring active")
                                            encoded_payload = self._payload_codec.encode(False, proc, args, kwargs)

                                        if encoded_payload:
                                            progress_msg = message.Yield(msg.request,
                                                                         payload=encoded_payload.payload,
                                                                         progress=True,
                                                                         enc_algo=encoded_payload.enc_algo,
                                                                         enc_key=encoded_payload.enc_key,
                                                                         enc_serializer=encoded_payload.enc_serializer)
                                        else:
                                            progress_msg = message.Yield(msg.request,
                                                                         args=args,
                                                                         kwargs=kwargs,
                                                                         progress=True)

                                        self._transport.send(progress_msg)
                                else:
                                    progress = None

                                invoke_kwargs[endpoint.details_arg] = types.CallDetails(registration, progress=progress, caller=msg.caller, caller_authid=msg.caller_authid, caller_authrole=msg.caller_authrole, procedure=proc, enc_algo=msg.enc_algo)

                            on_reply = txaio.as_future(endpoint.fn, *invoke_args, **invoke_kwargs)

                            def success(res):
                                del self._invocations[msg.request]

                                encoded_payload = None
                                if msg.enc_algo:
                                    if not self._payload_codec:
                                        log_msg = u"trying to send encrypted payload, but no keyring active"
                                        self.log.warn(log_msg)
                                    else:
                                        try:
                                            if isinstance(res, types.CallResult):
                                                encoded_payload = self._payload_codec.encode(False, proc, res.results, res.kwresults)
                                            else:
                                                encoded_payload = self._payload_codec.encode(False, proc, [res])
                                        except Exception as e:
                                            self.log.warn(
                                                "failed to encrypt application payload: {err}",
                                                err=e,
                                            )

                                if encoded_payload:
                                    reply = message.Yield(msg.request,
                                                          payload=encoded_payload.payload,
                                                          enc_algo=encoded_payload.enc_algo,
                                                          enc_key=encoded_payload.enc_key,
                                                          enc_serializer=encoded_payload.enc_serializer)
                                else:
                                    if isinstance(res, types.CallResult):
                                        reply = message.Yield(msg.request,
                                                              args=res.results,
                                                              kwargs=res.kwresults)
                                    else:
                                        reply = message.Yield(msg.request,
                                                              args=[res])

                                try:
                                    self._transport.send(reply)
                                except SerializationError as e:
                                    # the application-level payload returned from the invoked procedure can't be serialized
                                    reply = message.Error(message.Invocation.MESSAGE_TYPE, msg.request, ApplicationError.INVALID_PAYLOAD,
                                                          args=[u'success return value from invoked procedure "{0}" could not be serialized: {1}'.format(registration.procedure, e)])
                                    self._transport.send(reply)

                            def error(err):
                                del self._invocations[msg.request]

                                errmsg = txaio.failure_message(err)

                                try:
                                    self.onUserError(err, errmsg)
                                except:
                                    pass

                                formatted_tb = None
                                if self.traceback_app:
                                    formatted_tb = txaio.failure_format_traceback(err)

                                reply = self._message_from_exception(
                                    message.Invocation.MESSAGE_TYPE,
                                    msg.request,
                                    err.value,
                                    formatted_tb,
                                    msg.enc_algo
                                )

                                try:
                                    self._transport.send(reply)
                                except SerializationError as e:
                                    # the application-level payload returned from the invoked procedure can't be serialized
                                    reply = message.Error(message.Invocation.MESSAGE_TYPE, msg.request, ApplicationError.INVALID_PAYLOAD,
                                                          args=[u'error return value from invoked procedure "{0}" could not be serialized: {1}'.format(registration.procedure, e)])
                                    self._transport.send(reply)
                                # we have handled the error, so we eat it
                                return None

                            self._invocations[msg.request] = InvocationRequest(msg.request, on_reply)

                            txaio.add_callbacks(on_reply, success, error)

            elif isinstance(msg, message.Interrupt):

                if msg.request not in self._invocations:
                    raise ProtocolError("INTERRUPT received for non-pending invocation {0}".format(msg.request))
                else:
                    # noinspection PyBroadException
                    try:
                        self._invocations[msg.request].cancel()
                    except Exception:
                        # XXX can .cancel() return a Deferred/Future?
                        try:
                            self.onUserError(
                                txaio.create_failure(),
                                "While cancelling call.",
                            )
                        except:
                            pass
                    finally:
                        del self._invocations[msg.request]

            elif isinstance(msg, message.Registered):

                if msg.request in self._register_reqs:

                    # get and pop outstanding register request
                    request = self._register_reqs.pop(msg.request)

                    # create new registration if not yet tracked
                    if msg.registration not in self._registrations:
                        registration = Registration(self, msg.registration, request.procedure, request.endpoint)
                        self._registrations[msg.registration] = registration
                    else:
                        raise ProtocolError("REGISTERED received for already existing registration ID {0}".format(msg.registration))

                    txaio.resolve(request.on_reply, registration)
                else:
                    raise ProtocolError("REGISTERED received for non-pending request ID {0}".format(msg.request))

            elif isinstance(msg, message.Unregistered):

                if msg.request == 0:
                    # this is a forced un-register either from a call
                    # to the wamp.* meta-api or the force_reregister
                    # option
                    try:
                        reg = self._registrations[msg.registration]
                    except KeyError:
                        raise ProtocolError(
                            "UNREGISTERED received for non-existant registration"
                            " ID {0}".format(msg.registration)
                        )
                    self.log.info(
                        u"Router unregistered procedure '{proc}' with ID {id}",
                        proc=reg.procedure,
                        id=msg.registration,
                    )
                elif msg.request in self._unregister_reqs:

                    # get and pop outstanding subscribe request
                    request = self._unregister_reqs.pop(msg.request)

                    # if the registration still exists, mark as inactive and remove ..
                    if request.registration_id in self._registrations:
                        self._registrations[request.registration_id].active = False
                        del self._registrations[request.registration_id]

                    # resolve deferred/future for unregistering successfully
                    txaio.resolve(request.on_reply)
                else:
                    raise ProtocolError("UNREGISTERED received for non-pending request ID {0}".format(msg.request))

            elif isinstance(msg, message.Error):

                # remove outstanding request and get the reply deferred/future
                on_reply = None

                # ERROR reply to CALL
                if msg.request_type == message.Call.MESSAGE_TYPE and msg.request in self._call_reqs:
                    on_reply = self._call_reqs.pop(msg.request).on_reply

                # ERROR reply to PUBLISH
                elif msg.request_type == message.Publish.MESSAGE_TYPE and msg.request in self._publish_reqs:
                    on_reply = self._publish_reqs.pop(msg.request).on_reply

                # ERROR reply to SUBSCRIBE
                elif msg.request_type == message.Subscribe.MESSAGE_TYPE and msg.request in self._subscribe_reqs:
                    on_reply = self._subscribe_reqs.pop(msg.request).on_reply

                # ERROR reply to UNSUBSCRIBE
                elif msg.request_type == message.Unsubscribe.MESSAGE_TYPE and msg.request in self._unsubscribe_reqs:
                    on_reply = self._unsubscribe_reqs.pop(msg.request).on_reply

                # ERROR reply to REGISTER
                elif msg.request_type == message.Register.MESSAGE_TYPE and msg.request in self._register_reqs:
                    on_reply = self._register_reqs.pop(msg.request).on_reply

                # ERROR reply to UNREGISTER
                elif msg.request_type == message.Unregister.MESSAGE_TYPE and msg.request in self._unregister_reqs:
                    on_reply = self._unregister_reqs.pop(msg.request).on_reply

                if on_reply:
                    txaio.reject(on_reply, self._exception_from_message(msg))
                else:
                    raise ProtocolError("WampAppSession.onMessage(): ERROR received for non-pending request_type {0} and request ID {1}".format(msg.request_type, msg.request))

            else:

                raise ProtocolError("Unexpected message {0}".format(msg.__class__))

    @public
    def onClose(self, wasClean):
        """
        Implements :func:`autobahn.wamp.interfaces.ITransportHandler.onClose`
        """
        self._transport = None

        if self._session_id:
            # fire callback and close the transport
            details = types.CloseDetails(
                reason=types.CloseDetails.REASON_TRANSPORT_LOST,
                message=(u"WAMP transport was lost without closing the"
                         u" session before"),
            )
            d = txaio.as_future(self.onLeave, details)

            def success(arg):
                # XXX also: handle async
                self.fire('leave', self, details)
                return arg

            def _error(e):
                return self._swallow_error(e, "While firing onLeave")
            txaio.add_callbacks(d, success, _error)

            self._session_id = None

        d = txaio.as_future(self.onDisconnect)

        def success(arg):
            # XXX do we care about returning 'arg' properly?
            return self.fire('disconnect', self, was_clean=wasClean)

        def _error(e):
            return self._swallow_error(e, "While firing onDisconnect")
        txaio.add_callbacks(d, success, _error)

    @public
    def onChallenge(self, challenge):
        """
        Implements :func:`autobahn.wamp.interfaces.ISession.onChallenge`
        """
        raise Exception("received authentication challenge, but onChallenge not implemented")

    @public
    def onJoin(self, details):
        """
        Implements :func:`autobahn.wamp.interfaces.ISession.onJoin`
        """

    def _errback_outstanding_requests(self, exc):
        """
        Errback any still outstanding requests with exc.
        """
        d = txaio.create_future_success(None)
        all_requests = [
            self._publish_reqs,
            self._subscribe_reqs,
            self._unsubscribe_reqs,
            self._call_reqs,
            self._register_reqs,
            self._unregister_reqs
        ]
        outstanding = []
        for requests in all_requests:
            outstanding.extend(requests.values())
            requests.clear()

        if outstanding:
            self.log.info(
                'Cancelling {count} outstanding requests',
                count=len(outstanding),
            )
        for request in outstanding:
            self.log.debug(
                'cleaning up outstanding {request_type} request {request_id}, '
                'firing errback on user handler {request_on_reply}',
                request_on_reply=request.on_reply,
                request_id=request.request_id,
                request_type=request.__class__.__name__,
            )
            txaio.reject(request.on_reply, exc)
            # wait for any async-ness in the error handlers for on_reply
            txaio.add_callbacks(d, lambda _: request.on_reply, lambda _: request.on_reply)
        return d

    @public
    def onLeave(self, details):
        """
        Implements :func:`autobahn.wamp.interfaces.ISession.onLeave`
        """
        if details.reason != CloseDetails.REASON_DEFAULT:
            self.log.warn('session closed with reason {reason} [{message}]', reason=details.reason, message=details.message)

        # fire ApplicationError on any currently outstanding requests
        exc = ApplicationError(details.reason, details.message)
        d = self._errback_outstanding_requests(exc)

        def disconnect(_):
            if self._transport:
                self.disconnect()
        txaio.add_callbacks(d, disconnect, disconnect)
        return d

    @public
    def leave(self, reason=None, message=None):
        """
        Implements :func:`autobahn.wamp.interfaces.ISession.leave`
        """
        if not self._session_id:
            raise SessionNotReady(u"session hasn't joined a realm")

        if not self._goodbye_sent:
            if not reason:
                reason = u"wamp.close.normal"
            msg = wamp.message.Goodbye(reason=reason, message=message)
            self._transport.send(msg)
            self._goodbye_sent = True
            # deferred that fires when transport actually hits CLOSED
            is_closed = self._transport is None or self._transport.is_closed
            return is_closed
        else:
            raise SessionNotReady(u"session was alread requested to leave")

    @public
    def onDisconnect(self):
        """
        Implements :func:`autobahn.wamp.interfaces.ISession.onDisconnect`
        """
        # fire TransportLost on any _still_ outstanding requests
        # (these should have been already cleaned up in onLeave() - when
        # this actually has fired)
        exc = exception.TransportLost()
        self._errback_outstanding_requests(exc)

    @public
    def publish(self, topic, *args, **kwargs):
        """
        Implements :func:`autobahn.wamp.interfaces.IPublisher.publish`
        """
        assert(type(topic) == six.text_type)

        if not self._transport:
            raise exception.TransportLost()

        options = kwargs.pop('options', None)
        if options and not isinstance(options, types.PublishOptions):
            raise Exception("options must be of type a.w.t.PublishOptions")

        request_id = self._request_id_gen.next()

        encoded_payload = None
        if self._payload_codec:
            encoded_payload = self._payload_codec.encode(True, topic, args, kwargs)

        if encoded_payload:
            if options:
                msg = message.Publish(request_id,
                                      topic,
                                      payload=encoded_payload.payload,
                                      enc_algo=encoded_payload.enc_algo,
                                      enc_key=encoded_payload.enc_key,
                                      enc_serializer=encoded_payload.enc_serializer,
                                      **options.message_attr())
            else:
                msg = message.Publish(request_id,
                                      topic,
                                      payload=encoded_payload.payload,
                                      enc_algo=encoded_payload.enc_algo,
                                      enc_key=encoded_payload.enc_key,
                                      enc_serializer=encoded_payload.enc_serializer)
        else:
            if options:
                msg = message.Publish(request_id,
                                      topic,
                                      args=args,
                                      kwargs=kwargs,
                                      **options.message_attr())
            else:
                msg = message.Publish(request_id,
                                      topic,
                                      args=args,
                                      kwargs=kwargs)

        if options and options.acknowledge:
            # only acknowledged publications expect a reply ..
            on_reply = txaio.create_future()
            self._publish_reqs[request_id] = PublishRequest(request_id, on_reply, was_encrypted=(encoded_payload is not None))
        else:
            on_reply = None

        try:
            # Notes:
            #
            # * this might raise autobahn.wamp.exception.SerializationError
            #   when the user payload cannot be serialized
            # * we have to setup a PublishRequest() in _publish_reqs _before_
            #   calling transpor.send(), because a mock- or side-by-side transport
            #   will immediately lead on an incoming WAMP message in onMessage()
            #
            self._transport.send(msg)
        except Exception as e:
            if request_id in self._publish_reqs:
                del self._publish_reqs[request_id]
            raise e

        return on_reply

    @public
    def subscribe(self, handler, topic=None, options=None):
        """
        Implements :func:`autobahn.wamp.interfaces.ISubscriber.subscribe`
        """
        assert((callable(handler) and topic is not None) or hasattr(handler, '__class__'))
        assert(topic is None or type(topic) == six.text_type)
        assert(options is None or isinstance(options, types.SubscribeOptions))

        if not self._transport:
            raise exception.TransportLost()

        def _subscribe(obj, fn, topic, options):
            request_id = self._request_id_gen.next()
            on_reply = txaio.create_future()
            handler_obj = Handler(fn, obj, options.details_arg if options else None)
            self._subscribe_reqs[request_id] = SubscribeRequest(request_id, topic, on_reply, handler_obj)

            if options:
                msg = message.Subscribe(request_id, topic, **options.message_attr())
            else:
                msg = message.Subscribe(request_id, topic)

            self._transport.send(msg)
            return on_reply

        if callable(handler):
            # subscribe a single handler
            return _subscribe(None, handler, topic, options)

        else:

            # subscribe all methods on an object decorated with "wamp.subscribe"
            on_replies = []
            for k in inspect.getmembers(handler.__class__, is_method_or_function):
                proc = k[1]
                if "_wampuris" in proc.__dict__:
                    for pat in proc.__dict__["_wampuris"]:
                        if pat.is_handler():
                            _uri = pat.uri()
                            subopts = pat.options or options
                            if subopts is None:
                                if pat.uri_type == uri.Pattern.URI_TYPE_WILDCARD:
                                    subopts = types.SubscribeOptions(match=u"wildcard")
                                else:
                                    subopts = types.SubscribeOptions(match=u"exact")
                            on_replies.append(_subscribe(handler, proc, _uri, subopts))

            # XXX needs coverage
            return txaio.gather(on_replies, consume_exceptions=True)

    def _unsubscribe(self, subscription):
        """
        Called from :meth:`autobahn.wamp.protocol.Subscription.unsubscribe`
        """
        assert(isinstance(subscription, Subscription))
        assert subscription.active
        assert(subscription.id in self._subscriptions)
        assert(subscription in self._subscriptions[subscription.id])

        if not self._transport:
            raise exception.TransportLost()

        # remove handler subscription and mark as inactive
        self._subscriptions[subscription.id].remove(subscription)
        subscription.active = False

        # number of handler subscriptions left ..
        scount = len(self._subscriptions[subscription.id])

        if scount == 0:
            # if the last handler was removed, unsubscribe from broker ..
            request_id = self._request_id_gen.next()

            on_reply = txaio.create_future()
            self._unsubscribe_reqs[request_id] = UnsubscribeRequest(request_id, on_reply, subscription.id)

            msg = message.Unsubscribe(request_id, subscription.id)

            self._transport.send(msg)
            return on_reply
        else:
            # there are still handlers active on the subscription!
            return txaio.create_future_success(scount)

    @public
    def call(self, procedure, *args, **kwargs):
        """
        Implements :func:`autobahn.wamp.interfaces.ICaller.call`
        """
        assert(type(procedure) == six.text_type)

        if not self._transport:
            raise exception.TransportLost()

        options = kwargs.pop('options', None)
        if options and not isinstance(options, types.CallOptions):
            raise Exception("options must be of type a.w.t.CallOptions")

        request_id = self._request_id_gen.next()

        encoded_payload = None
        if self._payload_codec:
            try:
                encoded_payload = self._payload_codec.encode(True, procedure, args, kwargs)
            except:
                self.log.failure()
                raise

        if encoded_payload:
            if options:
                msg = message.Call(request_id,
                                   procedure,
                                   payload=encoded_payload.payload,
                                   enc_algo=encoded_payload.enc_algo,
                                   enc_key=encoded_payload.enc_key,
                                   enc_serializer=encoded_payload.enc_serializer,
                                   **options.message_attr())
            else:
                msg = message.Call(request_id,
                                   procedure,
                                   payload=encoded_payload.payload,
                                   enc_algo=encoded_payload.enc_algo,
                                   enc_key=encoded_payload.enc_key,
                                   enc_serializer=encoded_payload.enc_serializer)
        else:
            if options:
                msg = message.Call(request_id,
                                   procedure,
                                   args=args,
                                   kwargs=kwargs,
                                   **options.message_attr())
            else:
                msg = message.Call(request_id,
                                   procedure,
                                   args=args,
                                   kwargs=kwargs)

        # FIXME: implement call canceling
        # def canceller(_d):
        #   cancel_msg = message.Cancel(request)
        #   self._transport.send(cancel_msg)
        # d = Deferred(canceller)

        on_reply = txaio.create_future()
        self._call_reqs[request_id] = CallRequest(request_id, procedure, on_reply, options)

        try:
            # Notes:
            #
            # * this might raise autobahn.wamp.exception.SerializationError
            #   when the user payload cannot be serialized
            # * we have to setup a CallRequest() in _call_reqs _before_
            #   calling transpor.send(), because a mock- or side-by-side transport
            #   will immediately lead on an incoming WAMP message in onMessage()
            #
            self._transport.send(msg)
        except:
            if request_id in self._call_reqs:
                del self._call_reqs[request_id]
            raise

        return on_reply

    @public
    def register(self, endpoint, procedure=None, options=None):
        """
        Implements :func:`autobahn.wamp.interfaces.ICallee.register`
        """
        assert((callable(endpoint) and procedure is not None) or hasattr(endpoint, '__class__'))
        assert(procedure is None or type(procedure) == six.text_type)
        assert(options is None or isinstance(options, types.RegisterOptions))

        if not self._transport:
            raise exception.TransportLost()

        def _register(obj, fn, procedure, options):
            request_id = self._request_id_gen.next()
            on_reply = txaio.create_future()
            endpoint_obj = Endpoint(fn, obj, options.details_arg if options else None)
            self._register_reqs[request_id] = RegisterRequest(request_id, on_reply, procedure, endpoint_obj)

            if options:
                msg = message.Register(request_id, procedure, **options.message_attr())
            else:
                msg = message.Register(request_id, procedure)

            self._transport.send(msg)
            return on_reply

        if callable(endpoint):

            # register a single callable
            return _register(None, endpoint, procedure, options)

        else:

            # register all methods on an object decorated with "wamp.register"
            on_replies = []
            for k in inspect.getmembers(endpoint.__class__, is_method_or_function):
                proc = k[1]
                if "_wampuris" in proc.__dict__:
                    for pat in proc.__dict__["_wampuris"]:
                        if pat.is_endpoint():
                            _uri = pat.uri()
                            regopts = pat.options or options
                            on_replies.append(_register(endpoint, proc, _uri, regopts))

            # XXX neds coverage
            return txaio.gather(on_replies, consume_exceptions=True)

    def _unregister(self, registration):
        """
        Called from :meth:`autobahn.wamp.protocol.Registration.unregister`
        """
        assert(isinstance(registration, Registration))
        assert registration.active
        assert(registration.id in self._registrations)

        if not self._transport:
            raise exception.TransportLost()

        request_id = self._request_id_gen.next()

        on_reply = txaio.create_future()
        self._unregister_reqs[request_id] = UnregisterRequest(request_id, on_reply, registration.id)

        msg = message.Unregister(request_id, registration.id)

        self._transport.send(msg)
        return on_reply


# this is NOT public; import from either autobahn.asyncio.wamp or
# autobahn.twisted.wamp
class _SessionShim(ApplicationSession):
    """
    shim that lets us present pep8 API for user-classes to override,
    but also backwards-compatible for existing code using
    ApplicationSession "directly".
    """

    #: name -> IAuthenticator
    _authenticators = None

    def onJoin(self, details):
        return self.on_join(details)

    def onConnect(self):
        if self._authenticators:
            # authid, authrole *must* match across all authenticators
            # (checked in add_authenticator) so these lists are either
            # [None] or [None, 'some_authid']
            authid = [x._args.get('authid', None) for x in self._authenticators.values()][-1]
            authrole = [x._args.get('authrole', None) for x in self._authenticators.values()][-1]
            authextra = self._merged_authextra()
            self.join(
                self.config.realm,
                authmethods=list(self._authenticators.keys()),
                authid=authid or u'public',
                authrole=authrole or u'default',
                authextra=authextra,
            )
        else:
            self.on_connect()

    def onChallenge(self, challenge):
        try:
            authenticator = self._authenticators[challenge.method]
        except KeyError:
            raise RuntimeError(
                "Received challenge for unknown authmethod '{}'".format(
                    challenge.method
                )
            )
        return authenticator.on_challenge(self, challenge)

    def onLeave(self, details):
        return self.on_leave(details)

    def onDisconnect(self):
        return self.on_disconnect()

    # experimental authentication API

    def add_authenticator(self, authenticator):
        assert isinstance(authenticator, IAuthenticator)
        if self._authenticators is None:
            self._authenticators = {}

        # before adding this authenticator we need to validate that
        # it's consistent with any other authenticators we may have --
        # for example, they must all agree on "authid" etc because
        # .join() only takes one value for all of those.

        def at_most_one(name):
            uni = set([
                a._args[name]
                for a in list(self._authenticators.values()) + [authenticator]
                if name in a._args
            ])
            if len(uni) > 1:
                raise ValueError(
                    "Inconsistent {}s: {}".format(
                        name,
                        ' '.join(uni),
                    )
                )

        # all authids must match
        at_most_one('authid')

        # all authroles must match
        at_most_one('authrole')

        # can we do anything else other than merge all authextra keys?
        # here we check that any duplicate keys have the same values
        authextra = authenticator.authextra
        merged = self._merged_authextra()
        for k, v in merged:
            if k in authextra and authextra[k] != v:
                raise ValueError(
                    "Inconsistent authextra values for '{}': '{}' vs '{}'".format(
                        k, v, authextra[k],
                    )
                )

        # validation complete, add it
        self._authenticators[authenticator.name] = authenticator

    def _merged_authextra(self):
        authextras = [a._args.get('authextra', {}) for a in self._authenticators.values()]
        # for all existing _authenticators, we've already checked that
        # if they contain a key it has the same value as all others.
        return {
            k: v
            for k, v in zip(
                reduce(lambda x, y: x | set(y.keys()), authextras, set()),
                reduce(lambda x, y: x | set(y.values()), authextras, set())
            )
        }

    # these are the actual "new API" methods (i.e. snake_case)
    #

    def on_join(self, details):
        pass

    def on_leave(self, details):
        self.disconnect()

    def on_connect(self):
        self.join(self.config.realm)

    def on_disconnect(self):
        pass


# ISession.register collides with the abc.ABCMeta.register method
# ISession.register(ApplicationSession)


class ApplicationSessionFactory(object):
    """
    WAMP endpoint session factory.
    """

    session = ApplicationSession
    """
    WAMP application session class to be used in this factory.
    """

    def __init__(self, config=None):
        """

        :param config: The default component configuration.
        :type config: instance of :class:`autobahn.wamp.types.ComponentConfig`
        """
        self.config = config or types.ComponentConfig(realm=u"default")

    def __call__(self):
        """
        Creates a new WAMP application session.

        :returns: -- An instance of the WAMP application session class as
                     given by `self.session`.
        """
        session = self.session(self.config)
        session.factory = self
        return session
