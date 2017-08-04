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
from autobahn.wamp.uri import error

__all__ = (
    'Error',
    'SessionNotReady',
    'SerializationError',
    'ProtocolError',
    'TransportLost',
    'ApplicationError',
    'NotAuthorized',
    'InvalidUri',
)


@public
class Error(RuntimeError):
    """
    Base class for all exceptions related to WAMP.
    """


@public
class SessionNotReady(Error):
    """
    The application tried to perform a WAMP interaction, but the
    session is not yet fully established.
    """


@public
class SerializationError(Error):
    """
    Exception raised when the WAMP serializer could not serialize the
    application payload (``args`` or ``kwargs`` for ``CALL``, ``PUBLISH``, etc).
    """


@public
class ProtocolError(Error):
    """
    Exception raised when WAMP protocol was violated. Protocol errors
    are fatal and are handled by the WAMP implementation. They are
    not supposed to be handled at the application level.
    """


@public
class TransportLost(Error):
    """
    Exception raised when the transport underlying the WAMP session
    was lost or is not connected.
    """


@public
class ApplicationError(Error):
    """
    Base class for all exceptions that can/may be handled
    at the application level.
    """

    INVALID_URI = u"wamp.error.invalid_uri"
    """
    Peer provided an incorrect URI for a URI-based attribute of a WAMP message
    such as a realm, topic or procedure.
    """

    INVALID_PAYLOAD = u"wamp.error.invalid_payload"
    """
    The application payload could not be serialized.
    """

    NO_SUCH_PROCEDURE = u"wamp.error.no_such_procedure"
    """
    A Dealer could not perform a call, since not procedure is currently registered
    under the given URI.
    """

    PROCEDURE_ALREADY_EXISTS = u"wamp.error.procedure_already_exists"
    """
    A procedure could not be registered, since a procedure with the given URI is
    already registered.
    """

    PROCEDURE_EXISTS_INVOCATION_POLICY_CONFLICT = u"wamp.error.procedure_exists_with_different_invocation_policy"
    """
    A procedure could not be registered, since a procedure with the given URI is
    already registered, and the registration has a conflicting invocation policy.
    """

    NO_SUCH_REGISTRATION = u"wamp.error.no_such_registration"
    """
    A Dealer could not perform a unregister, since the given registration is not active.
    """

    NO_SUCH_SUBSCRIPTION = u"wamp.error.no_such_subscription"
    """
    A Broker could not perform a unsubscribe, since the given subscription is not active.
    """

    NO_SUCH_SESSION = u"wamp.error.no_such_session"
    """
    A router could not perform an operation, since a session ID specified was non-existant.
    """

    INVALID_ARGUMENT = u"wamp.error.invalid_argument"
    """
    A call failed, since the given argument types or values are not acceptable to the
    called procedure - in which case the *Callee* may throw this error. Or a Router
    performing *payload validation* checked the payload (``args`` / ``kwargs``) of a call,
    call result, call error or publish, and the payload did not conform.
    """

    # FIXME: this currently isn't used neither in Autobahn nor Crossbar. Check!
    SYSTEM_SHUTDOWN = u"wamp.error.system_shutdown"
    """
    The *Peer* is shutting down completely - used as a ``GOODBYE`` (or ``ABORT``) reason.
    """

    # FIXME: this currently isn't used neither in Autobahn nor Crossbar. Check!
    CLOSE_REALM = u"wamp.error.close_realm"
    """
    The *Peer* want to leave the realm - used as a ``GOODBYE`` reason.
    """

    # FIXME: this currently isn't used neither in Autobahn nor Crossbar. Check!
    GOODBYE_AND_OUT = u"wamp.error.goodbye_and_out"
    """
    A *Peer* acknowledges ending of a session - used as a ``GOOBYE`` reply reason.
    """

    NOT_AUTHORIZED = u"wamp.error.not_authorized"
    """
    A call, register, publish or subscribe failed, since the session is not authorized
    to perform the operation.
    """

    AUTHORIZATION_FAILED = u"wamp.error.authorization_failed"
    """
    A Dealer or Broker could not determine if the *Peer* is authorized to perform
    a join, call, register, publish or subscribe, since the authorization operation
    *itself* failed. E.g. a custom authorizer did run into an error.
    """

    AUTHENTICATION_FAILED = u"wamp.error.authentication_failed"
    """
    Something failed with the authentication itself, that is, authentication could
    not run to end.
    """

    NO_AUTH_METHOD = u"wamp.error.no_auth_method"
    """
    No authentication method the peer offered is available or active.
    """

    NO_SUCH_REALM = u"wamp.error.no_such_realm"
    """
    Peer wanted to join a non-existing realm (and the *Router* did not allow to auto-create
    the realm).
    """

    NO_SUCH_ROLE = u"wamp.error.no_such_role"
    """
    A *Peer* was to be authenticated under a Role that does not (or no longer) exists on the Router.
    For example, the *Peer* was successfully authenticated, but the Role configured does not
    exists - hence there is some misconfiguration in the Router.
    """

    NO_SUCH_PRINCIPAL = u"wamp.error.no_such_principal"
    """
    A *Peer* was authenticated for an authid that does not or longer exists.
    """

    # FIXME: this currently isn't used neither in Autobahn nor Crossbar. Check!
    CANCELED = u"wamp.error.canceled"
    """
    A Dealer or Callee canceled a call previously issued (WAMP AP).
    """

    # FIXME: this currently isn't used neither in Autobahn nor Crossbar. Check!
    NO_ELIGIBLE_CALLEE = u"wamp.error.no_eligible_callee"
    """
    A *Dealer* could not perform a call, since a procedure with the given URI is registered,
    but *Callee Black- and Whitelisting* and/or *Caller Exclusion* lead to the
    exclusion of (any) *Callee* providing the procedure (WAMP AP).
    """

    ENC_NO_PAYLOAD_CODEC = u"wamp.error.no_payload_codec"
    """
    WAMP message in payload transparency mode received, but no codec set
    or codec did not decode the payload.
    """

    ENC_TRUSTED_URI_MISMATCH = u"wamp.error.encryption.trusted_uri_mismatch"
    """
    WAMP-cryptobox application payload end-to-end encryption error.
    """

    ENC_DECRYPT_ERROR = u"wamp.error.encryption.decrypt_error"
    """
    WAMP-cryptobox application payload end-to-end encryption error.
    """

    def __init__(self, error, *args, **kwargs):
        """

        :param error: The URI of the error that occurred, e.g. ``wamp.error.not_authorized``.
        :type error: str
        """
        Exception.__init__(self, *args)
        self.kwargs = kwargs
        self.error = error
        self.enc_algo = kwargs.pop('enc_algo', None)

    @public
    def error_message(self):
        """
        Get the error message of this exception.

        :returns: The error message.
        :rtype: str
        """
        return u'{0}: {1}'.format(
            self.error,
            u' '.join([six.text_type(a) for a in self.args]),
        )

    def __unicode__(self):
        if self.kwargs and 'traceback' in self.kwargs:
            tb = u':\n' + u'\n'.join(self.kwargs.pop('traceback')) + u'\n'
            self.kwargs['traceback'] = u'...'
        else:
            tb = u''
        return u"ApplicationError(error=<{0}>, args={1}, kwargs={2}, enc_algo={3}){4}".format(
            self.error, list(self.args), self.kwargs, self.enc_algo, tb)

    def __str__(self):
        if six.PY3:
            return self.__unicode__()
        else:
            return self.__unicode__().encode('utf8')


@error(ApplicationError.NOT_AUTHORIZED)
class NotAuthorized(Exception):
    """
    Not authorized to perform the respective action.
    """


@error(ApplicationError.INVALID_URI)
class InvalidUri(Exception):
    """
    The URI for a topic, procedure or error is not a valid WAMP URI.
    """


@error(ApplicationError.INVALID_PAYLOAD)
class InvalidPayload(Exception):
    """
    The URI for a topic, procedure or error is not a valid WAMP URI.
    """
