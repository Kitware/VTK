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

__all__ = ['RoleFeatures',
           'RoleBrokerFeatures',
           'RoleSubscriberFeatures',
           'RolePublisherFeatures',
           'RoleDealerFeatures',
           'RoleCallerFeatures',
           'RoleCalleeFeatures',
           'ROLE_NAME_TO_CLASS']


import json, types
from autobahn import util
from autobahn.wamp.exception import ProtocolError


class RoleFeatures(util.EqualityMixin):

   ROLE = None

   def __str__(self):
      return json.dumps(self.__dict__)

   def _check_all_bool(self):
      ## check feature attributes
      for k in self.__dict__:
         if not k.startswith('_') and k != 'ROLE':
            if getattr(self, k) is not None and type(getattr(self, k)) != bool:
               raise ProtocolError("invalid type {} for feature '{}' for role '{}'".format(getattr(self, k), k, self.ROLE))



class RoleCommonPubSubFeatures(RoleFeatures):

   def __init__(self,
                publisher_identification = None,
                partitioned_pubsub = None):

      self.publisher_identification = publisher_identification
      self.partitioned_pubsub = partitioned_pubsub



class RoleBrokerFeatures(RoleCommonPubSubFeatures):

   ROLE = u'broker'

   def __init__(self,
                subscriber_blackwhite_listing = None,
                publisher_exclusion = None,
                publication_trustlevels = None,
                pattern_based_subscription = None,
                subscriber_metaevents = None,
                subscriber_list = None,
                event_history = None,
                **kwargs):
      self.subscriber_blackwhite_listing = subscriber_blackwhite_listing
      self.publisher_exclusion = publisher_exclusion
      self.publication_trustlevels = publication_trustlevels
      self.pattern_based_subscription = pattern_based_subscription
      self.subscriber_metaevents = subscriber_metaevents
      self.subscriber_list = subscriber_list
      self.event_history = event_history
      RoleCommonPubSubFeatures.__init__(self, **kwargs)
      self._check_all_bool()



class RoleSubscriberFeatures(RoleCommonPubSubFeatures):

   ROLE = u'subscriber'

   def __init__(self,
                publication_trustlevels = None,
                pattern_based_subscription = None,
                subscriber_metaevents = None,
                subscriber_list = None,
                event_history = None,
                **kwargs):
      self.publication_trustlevels = publication_trustlevels
      self.pattern_based_subscription = pattern_based_subscription
      self.subscriber_metaevents = subscriber_metaevents
      self.subscriber_list = subscriber_list
      self.event_history = event_history
      RoleCommonPubSubFeatures.__init__(self, **kwargs)
      self._check_all_bool()



class RolePublisherFeatures(RoleCommonPubSubFeatures):

   ROLE = u'publisher'

   def __init__(self,
                subscriber_blackwhite_listing = None,
                publisher_exclusion = None,
                **kwargs):
      self.subscriber_blackwhite_listing = subscriber_blackwhite_listing
      self.publisher_exclusion = publisher_exclusion
      RoleCommonPubSubFeatures.__init__(self, **kwargs)
      self._check_all_bool()



class RoleCommonRpcFeatures(RoleFeatures):

   def __init__(self,
                caller_identification = None,
                partitioned_rpc = None,
                call_timeout = None,
                call_canceling = None,
                progressive_call_results = None):
      self.caller_identification = caller_identification
      self.partitioned_rpc = partitioned_rpc
      self.call_timeout = call_timeout
      self.call_canceling = call_canceling
      self.progressive_call_results = progressive_call_results



class RoleDealerFeatures(RoleCommonRpcFeatures):

   ROLE = u'dealer'

   def __init__(self,
                callee_blackwhite_listing = None,
                caller_exclusion = None,
                call_trustlevels = None,
                pattern_based_registration = None,
                **kwargs):
      self.callee_blackwhite_listing = callee_blackwhite_listing
      self.caller_exclusion = caller_exclusion
      self.call_trustlevels = call_trustlevels
      self.pattern_based_registration = pattern_based_registration
      RoleCommonRpcFeatures.__init__(self, **kwargs)
      self._check_all_bool()



class RoleCallerFeatures(RoleCommonRpcFeatures):

   ROLE = u'caller'

   def __init__(self,
                callee_blackwhite_listing = None,
                caller_exclusion = None,
                **kwargs):
      self.callee_blackwhite_listing = callee_blackwhite_listing
      self.caller_exclusion = caller_exclusion
      RoleCommonRpcFeatures.__init__(self, **kwargs)
      self._check_all_bool()



class RoleCalleeFeatures(RoleCommonRpcFeatures):

   ROLE = u'callee'

   def __init__(self,
                call_trustlevels = None,
                pattern_based_registration = None,
                **kwargs):
      self.call_trustlevels = call_trustlevels
      self.pattern_based_registration = pattern_based_registration
      RoleCommonRpcFeatures.__init__(self, **kwargs)
      self._check_all_bool()



ROLE_NAME_TO_CLASS = {
   u'broker': RoleBrokerFeatures,
   u'subscriber': RoleSubscriberFeatures,
   u'publisher': RolePublisherFeatures,
   u'dealer': RoleDealerFeatures,
   u'caller': RoleCallerFeatures,
   u'callee': RoleCalleeFeatures,
}
