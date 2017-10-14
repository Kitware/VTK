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

import json

from autobahn import util
from autobahn.wamp.exception import ProtocolError

__all__ = ('RoleFeatures',
           'RoleBrokerFeatures',
           'RoleSubscriberFeatures',
           'RolePublisherFeatures',
           'RoleDealerFeatures',
           'RoleCallerFeatures',
           'RoleCalleeFeatures',
           'ROLE_NAME_TO_CLASS',
           'DEFAULT_CLIENT_ROLES')


class RoleFeatures(util.EqualityMixin):

    """
    Base class for WAMP role features.
    """

    ROLE = None

    def __str__(self):
        return json.dumps(self.__dict__)

    def __repr__(self):
        configured_options = {}
        for k, v in self.__dict__.items():
            if v is not None:
                configured_options[k] = v
        return "{0}({1})".format(self.ROLE, ", ".join([k + '=' + str(v)
                                                       for k, v in configured_options.items()]))

    def _check_all_bool(self):
        # check feature attributes
        for k in self.__dict__:
            if not k.startswith('_') and k != 'ROLE':
                if getattr(self, k) is not None and type(getattr(self, k)) != bool:
                    raise ProtocolError("invalid type {0} for feature '{1}' for role '{2}'".format(getattr(self, k), k, self.ROLE))


class RoleBrokerFeatures(RoleFeatures):

    """
    WAMP broker role features.
    """

    ROLE = u'broker'

    def __init__(self,
                 publisher_identification=None,
                 publication_trustlevels=None,
                 pattern_based_subscription=None,
                 session_meta_api=None,
                 subscription_meta_api=None,
                 subscriber_blackwhite_listing=None,
                 publisher_exclusion=None,
                 subscription_revocation=None,
                 event_history=None,
                 payload_transparency=None,
                 x_acknowledged_event_delivery=None,
                 payload_encryption_cryptobox=None,
                 event_retention=None,
                 **kwargs):
        self.publisher_identification = publisher_identification
        self.publication_trustlevels = publication_trustlevels
        self.pattern_based_subscription = pattern_based_subscription
        self.session_meta_api = session_meta_api
        self.subscription_meta_api = subscription_meta_api
        self.subscriber_blackwhite_listing = subscriber_blackwhite_listing
        self.publisher_exclusion = publisher_exclusion
        self.subscription_revocation = subscription_revocation
        self.event_history = event_history
        self.payload_transparency = payload_transparency
        self.x_acknowledged_event_delivery = x_acknowledged_event_delivery
        self.payload_encryption_cryptobox = payload_encryption_cryptobox
        self.event_retention = event_retention
        self._check_all_bool()


class RoleSubscriberFeatures(RoleFeatures):

    """
    WAMP subscriber role features.
    """

    ROLE = u'subscriber'

    def __init__(self,
                 publisher_identification=None,
                 publication_trustlevels=None,
                 pattern_based_subscription=None,
                 subscription_revocation=None,
                 event_history=None,
                 payload_transparency=None,
                 payload_encryption_cryptobox=None,
                 **kwargs):
        self.publisher_identification = publisher_identification
        self.publication_trustlevels = publication_trustlevels
        self.pattern_based_subscription = pattern_based_subscription
        self.subscription_revocation = subscription_revocation
        self.event_history = event_history
        self.payload_transparency = payload_transparency
        self.payload_encryption_cryptobox = payload_encryption_cryptobox
        self._check_all_bool()


class RolePublisherFeatures(RoleFeatures):

    """
    WAMP publisher role features.
    """

    ROLE = u'publisher'

    def __init__(self,
                 publisher_identification=None,
                 subscriber_blackwhite_listing=None,
                 publisher_exclusion=None,
                 payload_transparency=None,
                 x_acknowledged_event_delivery=None,
                 payload_encryption_cryptobox=None,
                 **kwargs):
        self.publisher_identification = publisher_identification
        self.subscriber_blackwhite_listing = subscriber_blackwhite_listing
        self.publisher_exclusion = publisher_exclusion
        self.payload_transparency = payload_transparency
        self.x_acknowledged_event_delivery = x_acknowledged_event_delivery
        self.payload_encryption_cryptobox = payload_encryption_cryptobox
        self._check_all_bool()


class RoleDealerFeatures(RoleFeatures):

    """
    WAMP dealer role features.
    """

    ROLE = u'dealer'

    def __init__(self,
                 caller_identification=None,
                 call_trustlevels=None,
                 pattern_based_registration=None,
                 session_meta_api=None,
                 registration_meta_api=None,
                 shared_registration=None,
                 call_timeout=None,
                 call_canceling=None,
                 progressive_call_results=None,
                 registration_revocation=None,
                 payload_transparency=None,
                 testament_meta_api=None,
                 payload_encryption_cryptobox=None,
                 **kwargs):
        self.caller_identification = caller_identification
        self.call_trustlevels = call_trustlevels
        self.pattern_based_registration = pattern_based_registration
        self.session_meta_api = session_meta_api
        self.registration_meta_api = registration_meta_api
        self.shared_registration = shared_registration
        self.call_timeout = call_timeout
        self.call_canceling = call_canceling
        self.progressive_call_results = progressive_call_results
        self.registration_revocation = registration_revocation
        self.payload_transparency = payload_transparency
        self.testament_meta_api = testament_meta_api
        self.payload_encryption_cryptobox = payload_encryption_cryptobox
        self._check_all_bool()


class RoleCallerFeatures(RoleFeatures):

    """
    WAMP caller role features.
    """

    ROLE = u'caller'

    def __init__(self,
                 caller_identification=None,
                 call_timeout=None,
                 call_canceling=None,
                 progressive_call_results=None,
                 payload_transparency=None,
                 payload_encryption_cryptobox=None,
                 **kwargs):
        self.caller_identification = caller_identification
        self.call_timeout = call_timeout
        self.call_canceling = call_canceling
        self.progressive_call_results = progressive_call_results
        self.payload_transparency = payload_transparency
        self.payload_encryption_cryptobox = payload_encryption_cryptobox
        self._check_all_bool()


class RoleCalleeFeatures(RoleFeatures):

    """
    WAMP callee role features.
    """

    ROLE = u'callee'

    def __init__(self,
                 caller_identification=None,
                 call_trustlevels=None,
                 pattern_based_registration=None,
                 shared_registration=None,
                 call_timeout=None,
                 call_canceling=None,
                 progressive_call_results=None,
                 registration_revocation=None,
                 payload_transparency=None,
                 payload_encryption_cryptobox=None,
                 **kwargs):
        self.caller_identification = caller_identification
        self.call_trustlevels = call_trustlevels
        self.pattern_based_registration = pattern_based_registration
        self.shared_registration = shared_registration
        self.call_timeout = call_timeout
        self.call_canceling = call_canceling
        self.progressive_call_results = progressive_call_results
        self.registration_revocation = registration_revocation
        self.payload_transparency = payload_transparency
        self.payload_encryption_cryptobox = payload_encryption_cryptobox
        self._check_all_bool()


# map of role names to role class
ROLE_NAME_TO_CLASS = {
    u'broker': RoleBrokerFeatures,
    u'subscriber': RoleSubscriberFeatures,
    u'publisher': RolePublisherFeatures,
    u'dealer': RoleDealerFeatures,
    u'caller': RoleCallerFeatures,
    u'callee': RoleCalleeFeatures,
}


# default role features for client roles supported
DEFAULT_CLIENT_ROLES = {
    u'subscriber': RoleSubscriberFeatures(publisher_identification=True, pattern_based_subscription=True, subscription_revocation=True, payload_transparency=True, payload_encryption_cryptobox=True),
    u'publisher': RolePublisherFeatures(publisher_identification=True, subscriber_blackwhite_listing=True, publisher_exclusion=True, payload_transparency=True, x_acknowledged_event_delivery=True, payload_encryption_cryptobox=True),
    u'caller': RoleCallerFeatures(caller_identification=True, progressive_call_results=True, payload_transparency=True, payload_encryption_cryptobox=True),
    u'callee': RoleCalleeFeatures(caller_identification=True, pattern_based_registration=True, shared_registration=True, progressive_call_results=True, registration_revocation=True, payload_transparency=True, payload_encryption_cryptobox=True),
}
