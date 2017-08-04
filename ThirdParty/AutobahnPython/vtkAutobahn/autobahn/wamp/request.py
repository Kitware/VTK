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

__all__ = (
    'Publication',
    'Subscription',
    'Handler',
    'Registration',
    'Endpoint',
    'PublishRequest',
    'SubscribeRequest',
    'UnsubscribeRequest',
    'CallRequest',
    'InvocationRequest',
    'RegisterRequest',
    'UnregisterRequest',
)


class Publication(object):
    """
    Object representing a publication (feedback from publishing an event when doing
    an acknowledged publish).
    """

    __slots__ = ('id', 'was_encrypted')

    def __init__(self, publication_id, was_encrypted):
        """

        :param publication_id: The publication ID of the published event.
        :type publication_id: int

        :param was_encrypted: Flag indicating whether the app payload was encrypted.
        :type was_encrypted: bool
        """
        self.id = publication_id
        self.was_encrypted = was_encrypted

    def __str__(self):
        return "Publication(id={0}, was_encrypted={1})".format(self.id, self.was_encrypted)


class Subscription(object):
    """
    Object representing a handler subscription.
    """

    __slots__ = ('id', 'topic', 'active', 'session', 'handler')

    def __init__(self, subscription_id, topic, session, handler):
        """

        :param subscription_id: The subscription ID.
        :type subscription_id: int

        :param topic: The subscription URI or URI pattern.
        :type topic: str

        :param session: The ApplicationSession this subscription is living on.
        :type session: instance of ApplicationSession

        :param handler: The user event callback.
        :type handler: callable
        """
        self.id = subscription_id
        self.topic = topic
        self.active = True
        self.session = session
        self.handler = handler

    def unsubscribe(self):
        """
        Unsubscribe this subscription.
        """
        if self.active:
            return self.session._unsubscribe(self)
        else:
            raise Exception("subscription no longer active")

    def __str__(self):
        return "Subscription(id={0}, is_active={1})".format(self.id, self.active)


class Handler(object):
    """
    Object representing an event handler attached to a subscription.
    """

    __slots__ = ('fn', 'obj', 'details_arg')

    def __init__(self, fn, obj=None, details_arg=None):
        """

        :param fn: The event handler function to be called.
        :type fn: callable

        :param obj: The (optional) object upon which to call the function.
        :type obj: obj or None

        :param details_arg: The keyword argument under which event details should be provided.
        :type details_arg: str or None
        """
        self.fn = fn
        self.obj = obj
        self.details_arg = details_arg


class Registration(object):
    """
    Object representing a registration.
    """

    __slots__ = ('id', 'active', 'session', 'procedure', 'endpoint')

    def __init__(self, session, registration_id, procedure, endpoint):
        """

        :param id: The registration ID.
        :type id: int

        :param active: Flag indicating whether this registration is active.
        :type active: bool

        :param procedure: The procedure URI or URI pattern.
        :type procedure: callable

        :param endpoint: The user callback.
        :type endpoint: callable
        """
        self.id = registration_id
        self.active = True
        self.session = session
        self.procedure = procedure
        self.endpoint = endpoint

    def unregister(self):
        """
        """
        if self.active:
            return self.session._unregister(self)
        else:
            raise Exception("registration no longer active")


class Endpoint(object):
    """
    Object representing an procedure endpoint attached to a registration.
    """

    __slots__ = ('fn', 'obj', 'details_arg')

    def __init__(self, fn, obj=None, details_arg=None):
        """

        :param fn: The endpoint procedure to be called.
        :type fn: callable

        :param obj: The (optional) object upon which to call the function.
        :type obj: obj or None

        :param details_arg: The keyword argument under which call details should be provided.
        :type details_arg: str or None
        """
        self.fn = fn
        self.obj = obj
        self.details_arg = details_arg


class Request(object):
    """
    Object representing an outstanding request, such as for subscribe/unsubscribe,
    register/unregister or call/publish.
    """

    __slots__ = ('request_id', 'on_reply')

    def __init__(self, request_id, on_reply):
        """

        :param request_id: The WAMP request ID.
        :type request_id: int

        :param on_reply: The Deferred/Future to be fired when the request returns.
        :type on_reply: Deferred/Future
        """
        self.request_id = request_id
        self.on_reply = on_reply


class PublishRequest(Request):
    """
    Object representing an outstanding request to publish (acknowledged) an event.
    """

    __slots__ = ('was_encrypted')

    def __init__(self, request_id, on_reply, was_encrypted):
        """

        :param request_id: The WAMP request ID.
        :type request_id: int

        :param on_reply: The Deferred/Future to be fired when the request returns.
        :type on_reply: Deferred/Future

        :param was_encrypted: Flag indicating whether the app payload was encrypted.
        :type was_encrypted: bool
        """
        Request.__init__(self, request_id, on_reply)
        self.was_encrypted = was_encrypted


class SubscribeRequest(Request):
    """
    Object representing an outstanding request to subscribe to a topic.
    """

    __slots__ = ('handler', 'topic')

    def __init__(self, request_id, topic, on_reply, handler):
        """

        :param request_id: The WAMP request ID.
        :type request_id: int

        :param topic: The topic URI being subscribed to.
        :type topic: unicode

        :param on_reply: The Deferred/Future to be fired when the request returns.
        :type on_reply: Deferred/Future

        :param handler: WAMP call options that are in use for this call.
        :type handler: callable
        """
        Request.__init__(self, request_id, on_reply)
        self.topic = topic
        self.handler = handler


class UnsubscribeRequest(Request):
    """
    Object representing an outstanding request to unsubscribe a subscription.
    """

    __slots__ = ('subscription_id',)

    def __init__(self, request_id, on_reply, subscription_id):
        """
        """
        Request.__init__(self, request_id, on_reply)
        self.subscription_id = subscription_id


class CallRequest(Request):
    """
    Object representing an outstanding request to call a procedure.
    """

    __slots__ = ('procedure', 'options',)

    def __init__(self, request_id, procedure, on_reply, options):
        """

        :param request_id: The WAMP request ID.
        :type request_id: int

        :param on_reply: The Deferred/Future to be fired when the request returns.
        :type on_reply: Deferred/Future

        :param options: WAMP call options that are in use for this call.
        :type options: dict
        """
        Request.__init__(self, request_id, on_reply)
        self.procedure = procedure
        self.options = options


class InvocationRequest(Request):
    """
    Object representing an outstanding request to invoke an endpoint.
    """


class RegisterRequest(Request):
    """
    Object representing an outstanding request to register a procedure.
    """

    __slots__ = ('procedure', 'endpoint',)

    def __init__(self, request_id, on_reply, procedure, endpoint):
        """
        """
        Request.__init__(self, request_id, on_reply)
        self.procedure = procedure
        self.endpoint = endpoint


class UnregisterRequest(Request):
    """
    Object representing an outstanding request to unregister a registration.
    """

    __slots__ = ('registration_id',)

    def __init__(self, request_id, on_reply, registration_id):
        """
        """
        Request.__init__(self, request_id, on_reply)
        self.registration_id = registration_id
