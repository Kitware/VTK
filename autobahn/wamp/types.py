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

from autobahn.util import public

from autobahn.wamp.request import Subscription, Registration


__all__ = (
    'ComponentConfig',
    'HelloReturn',
    'Accept',
    'Deny',
    'Challenge',
    'HelloDetails',
    'SessionDetails',
    'CloseDetails',
    'SubscribeOptions',
    'EventDetails',
    'PublishOptions',
    'RegisterOptions',
    'CallDetails',
    'CallOptions',
    'CallResult',
    'EncodedPayload'
)


@public
class ComponentConfig(object):
    """
    WAMP application component configuration. An instance of this class is
    provided to the constructor of :class:`autobahn.wamp.protocol.ApplicationSession`.
    """

    __slots__ = (
        'realm',
        'extra',
        'keyring',
        'controller',
        'shared'
    )

    def __init__(self, realm=None, extra=None, keyring=None, controller=None, shared=None):
        """

        :param realm: The realm the session would like to join or ``None`` to let the router
            auto-decide the realm (if the router is configured and allowing to do so).
        :type realm: str

        :param extra: Optional user-supplied object with extra configuration.
            This can be any object you like, and is accessible in your
            `ApplicationSession` subclass via `self.config.extra`. `dict` is
            a good default choice. Important: if the component is to be hosted
            by Crossbar.io, the supplied value must be JSON serializable.
        :type extra: arbitrary

        :param keyring: A mapper from WAMP URIs to "from"/"to" Ed25519 keys. When using
            WAMP end-to-end encryption, application payload is encrypted using a
            symmetric message key, which in turn is encrypted using the "to" URI (topic being
            published to or procedure being called) public key and the "from" URI
            private key. In both cases, the key for the longest matching URI is used.
        :type keyring: obj implementing IKeyRing or None

        :param controller: A WAMP ApplicationSession instance that holds a session to
            a controlling entity. This optional feature needs to be supported by a WAMP
            component hosting run-time.
        :type controller: instance of ApplicationSession or None

        :param shared: A dict object to exchange user information or hold user objects shared
            between components run under the same controlling entity. This optional feature
            needs to be supported by a WAMP component hosting run-time. Use with caution, as using
            this feature can introduce coupling between components. A valid use case would be
            to hold a shared database connection pool.
        :type shared: dict or None
        """
        assert(realm is None or type(realm) == six.text_type)
        # assert(keyring is None or ...) # FIXME

        self.realm = realm
        self.extra = extra
        self.keyring = keyring
        self.controller = controller
        self.shared = shared

    def __str__(self):
        return u"ComponentConfig(realm=<{}>, extra={}, keyring={}, controller={}, shared={})".format(self.realm, self.extra, self.keyring, self.controller, self.shared)


@public
class HelloReturn(object):
    """
    Base class for ``HELLO`` return information.
    """


@public
class Accept(HelloReturn):
    """
    Information to accept a ``HELLO``.
    """

    __slots__ = (
        'realm',
        'authid',
        'authrole',
        'authmethod',
        'authprovider',
        'authextra',
    )

    def __init__(self, realm=None, authid=None, authrole=None, authmethod=None, authprovider=None, authextra=None):
        """

        :param realm: The realm the client is joined to.
        :type realm: str

        :param authid: The authentication ID the client is assigned, e.g. ``"joe"`` or ``"joe@example.com"``.
        :type authid: str

        :param authrole: The authentication role the client is assigned, e.g. ``"anonymous"``, ``"user"`` or ``"com.myapp.user"``.
        :type authrole: str

        :param authmethod: The authentication method that was used to authenticate the client, e.g. ``"cookie"`` or ``"wampcra"``.
        :type authmethod: str

        :param authprovider: The authentication provider that was used to authenticate the client, e.g. ``"mozilla-persona"``.
        :type authprovider: str

        :param authextra: Application-specific authextra to be forwarded to the client in `WELCOME.details.authextra`.
        :type authextra: dict
        """
        assert(realm is None or type(realm) == six.text_type)
        assert(authid is None or type(authid) == six.text_type)
        assert(authrole is None or type(authrole) == six.text_type)
        assert(authmethod is None or type(authmethod) == six.text_type)
        assert(authprovider is None or type(authprovider) == six.text_type)
        assert(authextra is None or type(authextra) == dict)

        self.realm = realm
        self.authid = authid
        self.authrole = authrole
        self.authmethod = authmethod
        self.authprovider = authprovider
        self.authextra = authextra

    def __str__(self):
        return u"Accept(realm=<{}>, authid=<{}>, authrole=<{}>, authmethod={}, authprovider={}, authextra={})".format(self.realm, self.authid, self.authrole, self.authmethod, self.authprovider, self.authextra)


@public
class Deny(HelloReturn):
    """
    Information to deny a ``HELLO``.
    """

    __slots__ = (
        'reason',
        'message',
    )

    def __init__(self, reason=u'wamp.error.not_authorized', message=None):
        """

        :param reason: The reason of denying the authentication (an URI, e.g. ``u'wamp.error.not_authorized'``)
        :type reason: str

        :param message: A human readable message (for logging purposes).
        :type message: str
        """
        assert(type(reason) == six.text_type)
        assert(message is None or type(message) == six.text_type)

        self.reason = reason
        self.message = message

    def __str__(self):
        return u"Deny(reason=<{}>, message='{}')".format(self.reason, self.message)


@public
class Challenge(HelloReturn):
    """
    Information to challenge the client upon ``HELLO``.
    """

    __slots__ = (
        'method',
        'extra',
    )

    def __init__(self, method, extra=None):
        """

        :param method: The authentication method for the challenge (e.g. ``"wampcra"``).
        :type method: str

        :param extra: Any extra information for the authentication challenge. This is
           specific to the authentication method.
        :type extra: dict
        """
        assert(type(method) == six.text_type)
        assert(extra is None or type(extra) == dict)

        self.method = method
        self.extra = extra or {}

    def __str__(self):
        return u"Challenge(method={}, extra={})".format(self.method, self.extra)


@public
class HelloDetails(object):
    """
    Provides details of a WAMP session while still attaching.
    """

    __slots__ = (
        'realm',
        'authmethods',
        'authid',
        'authrole',
        'authextra',
        'session_roles',
        'pending_session',
        'resumable',
        'resume_session',
        'resume_token',
    )

    def __init__(self, realm=None, authmethods=None, authid=None, authrole=None, authextra=None, session_roles=None, pending_session=None, resumable=None, resume_session=None, resume_token=None):
        """

        :param realm: The realm the client wants to join.
        :type realm: str or None

        :param authmethods: The authentication methods the client is willing to perform.
        :type authmethods: list of str or None

        :param authid: The authid the client wants to authenticate as.
        :type authid: str or None

        :param authrole: The authrole the client wants to authenticate as.
        :type authrole: str or None

        :param authextra: Any extra information the specific authentication method requires the client to send.
        :type authextra: arbitrary or None

        :param session_roles: The WAMP session roles and features by the connecting client.
        :type session_roles: dict or None

        :param pending_session: The session ID the session will get once successfully attached.
        :type pending_session: int or None

        :param resumable:
        :type resumable: bool or None

        :param resume_session: The session the client would like to resume.
        :type resume_session: int or None

        :param resume_token: The secure authorisation token to resume the session.
        :type resume_token: str or None
        """
        assert(realm is None or type(realm) == six.text_type)
        assert(authmethods is None or (type(authmethods) == list and all(type(x) == six.text_type for x in authmethods)))
        assert(authid is None or type(authid) == six.text_type)
        assert(authrole is None or type(authrole) == six.text_type)
        assert(authextra is None or type(authextra) == dict)
        # assert(session_roles is None or ...)  # FIXME
        assert(pending_session is None or type(pending_session) in six.integer_types)
        assert(resumable is None or type(resumable) == bool)
        assert(resume_session is None or type(resume_session) == int)
        assert(resume_token is None or type(resume_token) == six.text_type)

        self.realm = realm
        self.authmethods = authmethods
        self.authid = authid
        self.authrole = authrole
        self.authextra = authextra
        self.session_roles = session_roles
        self.pending_session = pending_session
        self.resumable = resumable
        self.resume_session = resume_session
        self.resume_token = resume_token

    def __str__(self):
        return u"HelloDetails(realm=<{}>, authmethods={}, authid=<{}>, authrole=<{}>, authextra={}, session_roles={}, pending_session={}, resumable={}, resume_session={}, resume_token={})".format(self.realm, self.authmethods, self.authid, self.authrole, self.authextra, self.session_roles, self.pending_session, self.resumable, self.resume_session, self.resume_token)


@public
class SessionDetails(object):
    """
    Provides details for a WAMP session upon open.

    .. seealso:: :func:`autobahn.wamp.interfaces.ISession.onJoin`
    """

    __slots__ = (
        'realm',
        'session',
        'authid',
        'authrole',
        'authmethod',
        'authprovider',
        'authextra',
        'resumed',
        'resumable',
        'resume_token',
    )

    def __init__(self, realm, session, authid=None, authrole=None, authmethod=None, authprovider=None, authextra=None, resumed=None, resumable=None, resume_token=None):
        """

        :param realm: The realm this WAMP session is attached to.
        :type realm: str

        :param session: WAMP session ID of this session.
        :type session: int

        :param resumed: Whether the session is a resumed one.
        :type resumed: bool or None

        :param resumable: Whether this session can be resumed later.
        :type resumable: bool or None

        :param resume_token: The secure authorisation token to resume the session.
        :type resume_token: str or None
        """
        assert(type(realm) == six.text_type)
        assert(type(session) in six.integer_types)
        assert(authid is None or type(authid) == six.text_type)
        assert(authrole is None or type(authrole) == six.text_type)
        assert(authmethod is None or type(authmethod) == six.text_type)
        assert(authprovider is None or type(authprovider) == six.text_type)
        assert(authextra is None or type(authextra) == dict)
        assert(resumed is None or type(resumed) == bool)
        assert(resumable is None or type(resumable) == bool)
        assert(resume_token is None or type(resume_token) == six.text_type)

        self.realm = realm
        self.session = session
        self.authid = authid
        self.authrole = authrole
        self.authmethod = authmethod
        self.authprovider = authprovider
        self.authextra = authextra
        self.resumed = resumed
        self.resumable = resumable
        self.resume_token = resume_token

    def __str__(self):
        return u"SessionDetails(realm=<{}>, session={}, authid=<{}>, authrole=<{}>, authmethod={}, authprovider={}, authextra={}, resumed={}, resumable={}, resume_token={})".format(self.realm, self.session, self.authid, self.authrole, self.authmethod, self.authprovider, self.authextra, self.resumed, self.resumable, self.resume_token)


@public
class CloseDetails(object):
    """
    Provides details for a WAMP session upon close.

    .. seealso:: :func:`autobahn.wamp.interfaces.ISession.onLeave`
    """
    REASON_DEFAULT = u"wamp.close.normal"
    REASON_TRANSPORT_LOST = u"wamp.close.transport_lost"

    __slots__ = (
        'reason',
        'message',
    )

    def __init__(self, reason=None, message=None):
        """

        :param reason: The close reason (an URI, e.g. ``wamp.close.normal``)
        :type reason: str

        :param message: Closing log message.
        :type message: str
        """
        assert(reason is None or type(reason) == six.text_type)
        assert(message is None or type(message) == six.text_type)

        self.reason = reason
        self.message = message

    def __str__(self):
        return u"CloseDetails(reason=<{}>, message='{}')".format(self.reason, self.message)


@public
class SubscribeOptions(object):
    """
    Used to provide options for subscribing in
    :func:`autobahn.wamp.interfaces.ISubscriber.subscribe`.
    """

    __slots__ = (
        'match',
        'details',
        'details_arg',
        'get_retained',
    )

    def __init__(self, match=None, details=None, details_arg=None, get_retained=None):
        """

        :param match: The topic matching method to be used for the subscription.
        :type match: str

        :param details: When invoking the handler, provide event details in a keyword
            parameter ``details``.
        :type details: bool

        :param details_arg: DEPCREATED (use "details" flag). When invoking the handler
            provide event details in this keyword argument to the callable.
        :type details_arg: str

        :param get_retained: Whether the client wants the retained message we may have along with the subscription.
        :type get_retained: bool or None
        """
        assert(match is None or (type(match) == six.text_type and match in [u'exact', u'prefix', u'wildcard']))
        assert(details is None or (type(details) == bool and details_arg is None))
        assert(details_arg is None or type(details_arg) == str)  # yes, "str" is correct here, since this is about Python identifiers!
        assert(get_retained is None or type(get_retained) is bool)

        self.match = match
        self.details = details

        # FIXME: this is for backwards compat, but we'll deprecate it in the future
        if details:
            self.details_arg = 'details'
        else:
            self.details_arg = details_arg

        self.get_retained = get_retained

    def message_attr(self):
        """
        Returns options dict as sent within WAMP messages.
        """
        options = {}

        if self.match is not None:
            options[u'match'] = self.match

        if self.get_retained is not None:
            options[u'get_retained'] = self.get_retained

        return options

    def __str__(self):
        return u"SubscribeOptions(match={}, details={}, details_arg={}, get_retained={})".format(self.match, self.details, self.details_arg, self.get_retained)


@public
class EventDetails(object):
    """
    Provides details on an event when calling an event handler
    previously registered.
    """

    __slots__ = (
        'subscription',
        'publication',
        'publisher',
        'publisher_authid',
        'publisher_authrole',
        'topic',
        'retained',
        'enc_algo',
    )

    def __init__(self, subscription, publication, publisher=None, publisher_authid=None, publisher_authrole=None, topic=None, retained=None, enc_algo=None):
        """

        :param subscription: The (client side) subscription object on which this event is delivered.
        :type subscription: instance of :class:`autobahn.wamp.request.Subscription`

        :param publication: The publication ID of the event (always present).
        :type publication: int

        :param publisher: The WAMP session ID of the original publisher of this event.
            Only filled when publisher is disclosed.
        :type publisher: None or int

        :param publisher_authid: The WAMP authid of the original publisher of this event.
            Only filled when publisher is disclosed.
        :type publisher_authid: str or None

        :param publisher_authrole: The WAMP authrole of the original publisher of this event.
            Only filled when publisher is disclosed.
        :type publisher_authrole: str or None

        :param topic: For pattern-based subscriptions, the actual topic URI being published to.
            Only filled for pattern-based subscriptions.
        :type topic: str or None

        :param retained: Whether the message was retained by the broker on the topic, rather than just published.
        :type retained: bool or None

        :param enc_algo: Payload encryption algorithm that
            was in use (currently, either ``None`` or ``u'cryptobox'``).
        :type enc_algo: str or None
        """
        assert(isinstance(subscription, Subscription))
        assert(type(publication) in six.integer_types)
        assert(publisher is None or type(publisher) in six.integer_types)
        assert(publisher_authid is None or type(publisher_authid) == six.text_type)
        assert(publisher_authrole is None or type(publisher_authrole) == six.text_type)
        assert(topic is None or type(topic) == six.text_type)
        assert(retained is None or type(retained) is bool)
        assert(enc_algo is None or type(enc_algo) == six.text_type)

        self.subscription = subscription
        self.publication = publication
        self.publisher = publisher
        self.publisher_authid = publisher_authid
        self.publisher_authrole = publisher_authrole
        self.topic = topic
        self.retained = retained
        self.enc_algo = enc_algo

    def __str__(self):
        return u"EventDetails(subscription={}, publication={}, publisher={}, publisher_authid={}, publisher_authrole={}, topic=<{}>, retained={}, enc_algo={})".format(self.subscription, self.publication, self.publisher, self.publisher_authid, self.publisher_authrole, self.topic, self.retained, self.enc_algo)


@public
class PublishOptions(object):
    """
    Used to provide options for subscribing in
    :func:`autobahn.wamp.interfaces.IPublisher.publish`.
    """

    __slots__ = (
        'acknowledge',
        'exclude_me',
        'exclude',
        'exclude_authid',
        'exclude_authrole',
        'eligible',
        'eligible_authid',
        'eligible_authrole',
        'retain',
    )

    def __init__(self,
                 acknowledge=None,
                 exclude_me=None,
                 exclude=None,
                 exclude_authid=None,
                 exclude_authrole=None,
                 eligible=None,
                 eligible_authid=None,
                 eligible_authrole=None,
                 retain=None):
        """

        :param acknowledge: If ``True``, acknowledge the publication with a success or
           error response.
        :type acknowledge: bool

        :param exclude_me: If ``True``, exclude the publisher from receiving the event, even
           if he is subscribed (and eligible).
        :type exclude_me: bool or None

        :param exclude: A single WAMP session ID or a list thereof to exclude from receiving this event.
        :type exclude: int or list of int or None

        :param exclude_authid: A single WAMP authid or a list thereof to exclude from receiving this event.
        :type exclude_authid: str or list of str or None

        :param exclude_authrole: A single WAMP authrole or a list thereof to exclude from receiving this event.
        :type exclude_authrole: list of str or None

        :param eligible: A single WAMP session ID or a list thereof eligible to receive this event.
        :type eligible: int or list of int or None

        :param eligible_authid: A single WAMP authid or a list thereof eligible to receive this event.
        :type eligible_authid: str or list of str or None

        :param eligible_authrole: A single WAMP authrole or a list thereof eligible to receive this event.
        :type eligible_authrole: str or list of str or None

        :param retain: If ``True``, request the broker retain this event.
        :type retain: bool or None
        """
        assert(acknowledge is None or type(acknowledge) == bool)
        assert(exclude_me is None or type(exclude_me) == bool)
        assert(exclude is None or type(exclude) in six.integer_types or (type(exclude) == list and all(type(x) in six.integer_types for x in exclude)))
        assert(exclude_authid is None or type(exclude_authid) == six.text_type or (type(exclude_authid) == list and all(type(x) == six.text_type for x in exclude_authid)))
        assert(exclude_authrole is None or type(exclude_authrole) == six.text_type or (type(exclude_authrole) == list and all(type(x) == six.text_type for x in exclude_authrole)))
        assert(eligible is None or type(eligible) in six.integer_types or (type(eligible) == list and all(type(x) in six.integer_types for x in eligible)))
        assert(eligible_authid is None or type(eligible_authid) == six.text_type or (type(eligible_authid) == list and all(type(x) == six.text_type for x in eligible_authid)))
        assert(eligible_authrole is None or type(eligible_authrole) == six.text_type or (type(eligible_authrole) == list and all(type(x) == six.text_type for x in eligible_authrole)))
        assert(retain is None or type(retain) == bool)

        self.acknowledge = acknowledge
        self.exclude_me = exclude_me
        self.exclude = exclude
        self.exclude_authid = exclude_authid
        self.exclude_authrole = exclude_authrole
        self.eligible = eligible
        self.eligible_authid = eligible_authid
        self.eligible_authrole = eligible_authrole
        self.retain = retain

    def message_attr(self):
        """
        Returns options dict as sent within WAMP messages.
        """
        options = {}

        if self.acknowledge is not None:
            options[u'acknowledge'] = self.acknowledge

        if self.exclude_me is not None:
            options[u'exclude_me'] = self.exclude_me

        if self.exclude is not None:
            options[u'exclude'] = self.exclude if type(self.exclude) == list else [self.exclude]

        if self.exclude_authid is not None:
            options[u'exclude_authid'] = self.exclude_authid if type(self.exclude_authid) == list else [self.exclude_authid]

        if self.exclude_authrole is not None:
            options[u'exclude_authrole'] = self.exclude_authrole if type(self.exclude_authrole) == list else [self.exclude_authrole]

        if self.eligible is not None:
            options[u'eligible'] = self.eligible if type(self.eligible) == list else [self.eligible]

        if self.eligible_authid is not None:
            options[u'eligible_authid'] = self.eligible_authid if type(self.eligible_authid) == list else [self.eligible_authid]

        if self.eligible_authrole is not None:
            options[u'eligible_authrole'] = self.eligible_authrole if type(self.eligible_authrole) == list else [self.eligible_authrole]

        if self.retain is not None:
            options[u'retain'] = self.retain

        return options

    def __str__(self):
        return u"PublishOptions(acknowledge={}, exclude_me={}, exclude={}, exclude_authid={}, exclude_authrole={}, eligible={}, eligible_authid={}, eligible_authrole={}, retain={})".format(self.acknowledge, self.exclude_me, self.exclude, self.exclude_authid, self.exclude_authrole, self.eligible, self.eligible_authid, self.eligible_authrole, self.retain)


@public
class RegisterOptions(object):
    """
    Used to provide options for registering in
    :func:`autobahn.wamp.interfaces.ICallee.register`.
    """

    __slots__ = (
        'match',
        'invoke',
        'concurrency',
        'force_reregister',
        'details_arg',
    )

    def __init__(self, match=None, invoke=None, concurrency=None, details_arg=None,
                 force_reregister=None):
        """

        :param details_arg: When invoking the endpoint, provide call details
           in this keyword argument to the callable.
        :type details_arg: str
        """
        assert(match is None or (type(match) == six.text_type and match in [u'exact', u'prefix', u'wildcard']))
        assert(invoke is None or (type(invoke) == six.text_type and invoke in [u'single', u'first', u'last', u'roundrobin', u'random']))
        assert(concurrency is None or (type(concurrency) in six.integer_types and concurrency > 0))
        assert(details_arg is None or type(details_arg) == str)  # yes, "str" is correct here, since this is about Python identifiers!
        assert force_reregister in [None, True, False]

        self.match = match
        self.invoke = invoke
        self.concurrency = concurrency
        self.details_arg = details_arg
        self.force_reregister = force_reregister

    def message_attr(self):
        """
        Returns options dict as sent within WAMP messages.
        """
        options = {}

        if self.match is not None:
            options[u'match'] = self.match

        if self.invoke is not None:
            options[u'invoke'] = self.invoke

        if self.concurrency is not None:
            options[u'concurrency'] = self.concurrency

        if self.force_reregister is not None:
            options[u'force_reregister'] = self.force_reregister

        return options

    def __str__(self):
        return u"RegisterOptions(match={}, invoke={}, concurrency={}, details_arg={}, force_reregister={})".format(self.match, self.invoke, self.concurrency, self.details_arg, self.force_reregister)


@public
class CallDetails(object):
    """
    Provides details on a call when an endpoint previously
    registered is being called and opted to receive call details.
    """

    __slots__ = (
        'registration',
        'progress',
        'caller',
        'caller_authid',
        'caller_authrole',
        'procedure',
        'enc_algo',
    )

    def __init__(self, registration, progress=None, caller=None, caller_authid=None, caller_authrole=None, procedure=None, enc_algo=None):
        """

        :param registration: The (client side) registration object this invocation is delivered on.
        :type registration: instance of :class:`autobahn.wamp.request.Registration`

        :param progress: A callable that will receive progressive call results.
        :type progress: callable or None

        :param caller: The WAMP session ID of the caller, if the latter is disclosed.
            Only filled when caller is disclosed.
        :type caller: int or None

        :param caller_authid: The WAMP authid of the original caller of this event.
            Only filled when caller is disclosed.
        :type caller_authid: str or None

        :param caller_authrole: The WAMP authrole of the original caller of this event.
            Only filled when caller is disclosed.
        :type caller_authrole: str or None

        :param procedure: For pattern-based registrations, the actual procedure URI being called.
        :type procedure: str or None

        :param enc_algo: Payload encryption algorithm that
            was in use (currently, either `None` or `"cryptobox"`).
        :type enc_algo: str or None
        """
        assert(isinstance(registration, Registration))
        assert(progress is None or callable(progress))
        assert(caller is None or type(caller) in six.integer_types)
        assert(caller_authid is None or type(caller_authid) == six.text_type)
        assert(caller_authrole is None or type(caller_authrole) == six.text_type)
        assert(procedure is None or type(procedure) == six.text_type)
        assert(enc_algo is None or type(enc_algo) == six.text_type)

        self.registration = registration
        self.progress = progress
        self.caller = caller
        self.caller_authid = caller_authid
        self.caller_authrole = caller_authrole
        self.procedure = procedure
        self.enc_algo = enc_algo

    def __str__(self):
        return u"CallDetails(registration={}, progress={}, caller={}, caller_authid={}, caller_authrole={}, procedure=<{}>, enc_algo={})".format(self.registration, self.progress, self.caller, self.caller_authid, self.caller_authrole, self.procedure, self.enc_algo)


@public
class CallOptions(object):
    """
    Used to provide options for calling with :func:`autobahn.wamp.interfaces.ICaller.call`.
    """

    __slots__ = (
        'on_progress',
        'timeout',
    )

    def __init__(self,
                 on_progress=None,
                 timeout=None):
        """

        :param on_progress: A callback that will be called when the remote endpoint
           called yields interim call progress results.
        :type on_progress: callable

        :param timeout: Time in seconds after which the call should be automatically canceled.
        :type timeout: float
        """
        assert(on_progress is None or callable(on_progress))
        assert(timeout is None or (type(timeout) in list(six.integer_types) + [float] and timeout > 0))

        self.on_progress = on_progress
        self.timeout = timeout

    def message_attr(self):
        """
        Returns options dict as sent within WAMP messages.
        """
        options = {}

        if self.timeout is not None:
            options[u'timeout'] = self.timeout

        if self.on_progress is not None:
            options[u'receive_progress'] = True

        return options

    def __str__(self):
        return u"CallOptions(on_progress={}, timeout={})".format(self.on_progress, self.timeout)


@public
class CallResult(object):
    """
    Wrapper for remote procedure call results that contain multiple positional
    return values or keyword-based return values.
    """

    __slots__ = (
        'results',
        'kwresults',
        'enc_algo',
    )

    def __init__(self, *results, **kwresults):
        """

        :param results: The positional result values.
        :type results: list

        :param kwresults: The keyword result values.
        :type kwresults: dict
        """
        enc_algo = kwresults.pop('enc_algo', None)
        assert(enc_algo is None or type(enc_algo) == six.text_type)

        self.enc_algo = enc_algo
        self.results = results
        self.kwresults = kwresults

    def __str__(self):
        return u"CallResult(results={}, kwresults={}, enc_algo={})".format(self.results, self.kwresults, self.enc_algo)


@public
class EncodedPayload(object):
    """
    Wrapper holding an encoded application payload when using WAMP payload transparency.
    """

    __slots__ = (
        'payload',
        'enc_algo',
        'enc_serializer',
        'enc_key'
    )

    def __init__(self, payload, enc_algo, enc_serializer=None, enc_key=None):
        """

        :param payload: The encoded application payload.
        :type payload: bytes

        :param enc_algo: The payload transparency algorithm identifier to check.
        :type enc_algo: str

        :param enc_serializer: The payload transparency serializer identifier to check.
        :type enc_serializer: str

        :param enc_key: If using payload transparency with an encryption algorithm, the payload encryption key.
        :type enc_key: str or None
        """
        assert(type(payload) == six.binary_type)
        assert(type(enc_algo) == six.text_type)
        assert(enc_serializer is None or type(enc_serializer) == six.text_type)
        assert(enc_key is None or type(enc_key) == six.text_type)

        self.payload = payload
        self.enc_algo = enc_algo
        self.enc_serializer = enc_serializer
        self.enc_key = enc_key


class IPublication(object):
    """
    Represents a publication of an event. This is used with acknowledged publications.
    """

    def id(self):
        """
        The WAMP publication ID for this publication.
        """


class ISubscription(object):
    """
    Represents a subscription to a topic.
    """

    def id(self):
        """
        The WAMP subscription ID for this subscription.
        """

    def active(self):
        """
        Flag indicating if subscription is active.
        """

    def unsubscribe(self):
        """
        Unsubscribe this subscription that was previously created from
        :func:`autobahn.wamp.interfaces.ISubscriber.subscribe`.

        After a subscription has been unsubscribed successfully, no events
        will be routed to the event handler anymore.

        Returns an instance of :tx:`twisted.internet.defer.Deferred` (when
        running on **Twisted**) or an instance of :py:class:`asyncio.Future`
        (when running on **asyncio**).

        - If the unsubscription succeeds, the returned Deferred/Future will
          *resolve* (with no return value).

        - If the unsubscription fails, the returned Deferred/Future will *reject*
          with an instance of :class:`autobahn.wamp.exception.ApplicationError`.

        :returns: A Deferred/Future for the unsubscription
        :rtype: instance(s) of :tx:`twisted.internet.defer.Deferred` / :py:class:`asyncio.Future`
        """


class IRegistration(object):
    """
    Represents a registration of an endpoint.
    """

    def id(self):
        """
        The WAMP registration ID for this registration.
        """

    def active(self):
        """
        Flag indicating if registration is active.
        """

    def unregister(self):
        """
        Unregister this registration that was previously created from
        :func:`autobahn.wamp.interfaces.ICallee.register`.

        After a registration has been unregistered successfully, no calls
        will be routed to the endpoint anymore.

        Returns an instance of :tx:`twisted.internet.defer.Deferred` (when
        running on **Twisted**) or an instance of :py:class:`asyncio.Future`
        (when running on **asyncio**).

        - If the unregistration succeeds, the returned Deferred/Future will
          *resolve* (with no return value).

        - If the unregistration fails, the returned Deferred/Future will be rejected
          with an instance of :class:`autobahn.wamp.exception.ApplicationError`.

        :returns: A Deferred/Future for the unregistration
        :rtype: instance(s) of :tx:`twisted.internet.defer.Deferred` / :py:class:`asyncio.Future`
        """
