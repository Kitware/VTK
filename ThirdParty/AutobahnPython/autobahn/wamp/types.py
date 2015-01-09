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

import six



class ComponentConfig:
   """
   WAMP application component configuration. An instance of this class is
   provided to the constructor of :class:`autobahn.wamp.protocol.ApplicationSession`.
   """

   def __init__(self, realm = None, extra = None):
      """
      Ctor.

      :param realm: The realm the session should join.
      :type realm: str
      :param extra: Optional dictionary with extra configuration.
      :type extra: dict
      """
      if six.PY2 and type(realm) == str:
         realm = six.u(realm)
      self.realm = realm
      self.extra = extra

   def __str__(self):
      return "ComponentConfig(realm = {}, extra = {})".format(self.realm, self.extra)



class RouterOptions:
   """
   Router options for creating routers.
   """
   URI_CHECK_LOOSE = "loose"
   URI_CHECK_STRICT = "strict"

   def __init__(self, uri_check = None):
      """
      Ctor.

      :param uri_check: Method which should be applied to check WAMP URIs.
      :type uri_check: str
      """
      self.uri_check = uri_check or RouterOptions.URI_CHECK_STRICT

   def __str__(self):
      return "RouterOptions(uri_check = {})".format(self.uri_check)



class HelloReturn:
   """
   Base class for `HELLO` return information.
   """



class Accept(HelloReturn):
   """
   Information to accept a `HELLO`.
   """

   def __init__(self, authid = None, authrole = None, authmethod = None, authprovider = None):
      """
      Ctor.

      :param authid: The authentication ID the client is assigned, e.g. `"joe"` or `"joe@example.com"`.
      :type authid: str
      :param authrole: The authentication role the client is assigned, e.g. `"anonymous"`, `"user"` or `"com.myapp.user"`.
      :type authrole: str
      :param authmethod: The authentication method that was used to authenticate the client, e.g. `"cookie"` or `"wampcra"`.
      :type authmethod: str
      :param authprovider: The authentication provider that was used to authenticate the client, e.g. `"mozilla-persona"`.
      :type authprovider: str
      """
      if six.PY2:
         if type(authid) == str:
            authid = six.u(authid)
         if type(authrole) == str:
            authrole = six.u(authrole)
         if type(authmethod) == str:
            authmethod = six.u(authmethod)
         if type(authprovider) == str:
            authprovider = six.u(authprovider)

      assert(authid is None or type(authid) == six.text_type)
      assert(authrole is None or type(authrole) == six.text_type)
      assert(authmethod is None or type(authmethod) == six.text_type)
      assert(authprovider is None or type(authprovider) == six.text_type)

      self.authid = authid
      self.authrole = authrole
      self.authmethod = authmethod
      self.authprovider = authprovider


   def __str__(self):
      return "Accept(authid = {}, authrole = {}, authmethod = {}, authprovider = {})".format(self.authid, self.authrole, self.authmethod, self.authprovider)



class Deny(HelloReturn):
   """
   Information to deny a `HELLO`.
   """

   def __init__(self, reason = u"wamp.error.not_authorized", message = None):
      """
      Ctor.

      :param reason: The reason of denying the authentication (an URI, e.g. `wamp.error.not_authorized`)
      :type reason: str
      :param message: A human readable message (for logging purposes).
      :type message: str
      """
      if six.PY2:
         if type(reason) == str:
            reason = six.u(reason)
         if type(message) == str:
            message = six.u(message)

      assert(type(reason) == six.text_type)
      assert(message is None or type(message) == six.text_type)

      self.reason = reason
      self.message = message


   def __str__(self):
      return "Deny(reason = {}, message = '{}'')".format(self.reason, self.message)



class Challenge(HelloReturn):
   """
   Information to challenge the client upon `HELLO`.
   """

   def __init__(self, method, extra = None):
      """
      Ctor.

      :param method: The authentication method for the challenge (e.g. `"wampcra"`).
      :type method: str
      :param extra: Any extra information for the authentication challenge. This is
         specific to the authentication method.
      :type extra: dict
      """
      if six.PY2:
         if type(method) == str:
            method = six.u(method)

      self.method = method
      self.extra = extra or {}


   def __str__(self):
      return "Challenge(method = {}, extra = {})".format(self.method, self.extra)



class HelloDetails:
   """
   Provides details of a WAMP session while still attaching.
   """

   def __init__(self, roles = None, authmethods = None, authid = None, pending_session = None):
      """
      Ctor.

      :param roles: The WAMP roles and features supported by the attaching client.
      :type roles: dict
      :param authmethods: The authentication methods the client is willing to perform.
      :type authmethods: list
      :param authid: The authentication ID the client wants to authenticate as. Required for WAMP-CRA.
      :type authid: str
      :param pending_session: The session ID the session will get once successfully attached.
      :type pending_session: int
      """
      self.roles = roles
      self.authmethods = authmethods
      self.authid = authid
      self.pending_session = pending_session


   def __str__(self):
      return "HelloDetails(roles = {}, authmethods = {}, authid = {}, pending_session = {})".format(self.roles, self.authmethods, self.authid, self.pending_session)



class SessionDetails:
   """
   Provides details for a WAMP session upon open.

   @see: :func:`autobahn.wamp.interfaces.ISession.onJoin`
   """

   def __init__(self, realm, session, authid = None, authrole = None, authmethod = None, authprovider = None):
      """
      Ctor.

      :param realm: The realm this WAMP session is attached to.
      :type realm: str
      :param session: WAMP session ID of this session.
      :type session: int
      """
      self.realm = realm
      self.session = session
      self.authid = authid
      self.authrole = authrole
      self.authmethod = authmethod
      self.authprovider = authprovider


   def __str__(self):
      return "SessionDetails(realm = {}, session = {}, authid = {}, authrole = {}, authmethod = {})".format(self.realm, self.session, self.authid, self.authrole, self.authmethod)



class CloseDetails:
   """
   Provides details for a WAMP session upon open.

   @see: :func:`autobahn.wamp.interfaces.ISession.onLeave`
   """

   def __init__(self, reason = None, message = None):
      """
      Ctor.

      :param reason: The close reason (an URI, e.g. `wamp.close.normal`)
      :type reason: str
      :param message: Closing log message.
      :type message: str
      """
      self.reason = reason
      self.message = message


   def __str__(self):
      return "CloseDetails(reason = {}, message = '{}'')".format(self.reason, self.message)



class SubscribeOptions:
   """
   Used to provide options for subscribing in
   :func:`autobahn.wamp.interfaces.ISubscriber.subscribe`.
   """

   def __init__(self, match = None, details_arg = None):
      """
      :param match: The topic matching method to be used for the subscription.
      :type match: str
      :param details_arg: When invoking the handler, provide event details
                          in this keyword argument to the callable.
      :type details_arg: str
      """
      assert(match is None or (type(match) == str and match in ['exact', 'prefix', 'wildcard']))
      assert(details_arg is None or type(details_arg) == str)

      if match and six.PY2 and type(match) == str:
         match = six.u(match)
      self.match = match
      self.details_arg = details_arg

      ## options dict as sent within WAMP message
      self.options = {'match': match}


   def __str__(self):
      return "SubscribeOptions(match = {}, details_arg = {})".format(self.match, self.details_arg)



class EventDetails:
   """
   Provides details on an event when calling an event handler
   previously registered.
   """
   def __init__(self, publication, publisher = None):
      """
      Ctor.

      :param publication: The publication ID of the event (always present).
      :type publication: int
      :param publisher: The WAMP session ID of the original publisher of this event.
      :type publisher: int
      """
      self.publication = publication
      self.publisher = publisher


   def __str__(self):
      return "EventDetails(publication = {}, publisher = {})".format(self.publication, self.publisher)



class PublishOptions:
   """
   Used to provide options for subscribing in
   :func:`autobahn.wamp.interfaces.IPublisher.publish`.
   """

   def __init__(self,
                acknowledge = None,
                excludeMe = None,
                exclude = None,
                eligible = None,
                discloseMe = None):
      """
      Constructor.

      :param acknowledge: If True, acknowledge the publication with a success or
                          error response.
      :type acknowledge: bool
      :param excludeMe: If True, exclude the publisher from receiving the event, even
                        if he is subscribed (and eligible).
      :type excludeMe: bool
      :param exclude: List of WAMP session IDs to exclude from receiving this event.
      :type exclude: list
      :param eligible: List of WAMP session IDs eligible to receive this event.
      :type eligible: list
      :param discloseMe: If True, request to disclose the publisher of this event
                         to subscribers.
      :type discloseMe: bool
      """
      assert(acknowledge is None or type(acknowledge) == bool)
      assert(excludeMe is None or type(excludeMe) == bool)
      assert(exclude is None or (type(exclude) == list and all(type(x) in six.integer_types for x in exclude)))
      assert(eligible is None or (type(eligible) == list and all(type(x) in six.integer_types for x in eligible)))
      assert(discloseMe is None or type(discloseMe) == bool)

      self.acknowledge = acknowledge
      self.excludeMe = excludeMe
      self.exclude = exclude
      self.eligible = eligible
      self.discloseMe = discloseMe

      ## options dict as sent within WAMP message
      self.options = {
         'acknowledge': acknowledge,
         'excludeMe': excludeMe,
         'exclude': exclude,
         'eligible': eligible,
         'discloseMe': discloseMe
      }


   def __str__(self):
      return "PublishOptions(acknowledge = {}, excludeMe = {}, exclude = {}, eligible = {}, discloseMe = {})".format(self.acknowledge, self.excludeMe, self.exclude, self.eligible, self.discloseMe)



class RegisterOptions:
   """
   Used to provide options for registering in
   :func:`autobahn.wamp.interfaces.ICallee.register`.
   """

   def __init__(self, details_arg = None, pkeys = None, discloseCaller = None):
      """
      Ctor.

      :param details_arg: When invoking the endpoint, provide call details
                          in this keyword argument to the callable.
      :type details_arg: str
      """
      self.details_arg = details_arg
      self.pkeys = pkeys
      self.discloseCaller = discloseCaller

      ## options dict as sent within WAMP message
      self.options = {
         'pkeys': pkeys,
         'discloseCaller': discloseCaller
      }


   def __str__(self):
      return "RegisterOptions(details_arg = {}, pkeys = {}, discloseCaller = {})".format(self.details_arg, self.pkeys, self.discloseCaller)



class CallDetails:
   """
   Provides details on a call when an endpoint previously
   registered is being called and opted to receive call details.
   """

   def __init__(self, progress = None, caller = None, authid = None, authrole = None, authmethod = None):
      """
      Ctor.

      :param progress: A callable that will receive progressive call results.
      :type progress: callable
      :param caller: The WAMP session ID of the caller, if the latter is disclosed.
      :type caller: int
      :param authid: The authentication ID of the caller.
      :type authid: str
      :param authrole: The authentication role of the caller.
      :type authrole: str
      """
      self.progress = progress
      self.caller = caller
      self.authid = authid
      self.authrole = authrole
      self.authmethod = authmethod

   def __str__(self):
      return "CallDetails(progress = {}, caller = {}, authid = {}, authrole = {}, authmethod = {})".format(self.progress, self.caller, self.authid, self.authrole, self.authmethod)



class CallOptions:
   """
   Used to provide options for calling with :func:`autobahn.wamp.interfaces.ICaller.call`.
   """

   def __init__(self,
                onProgress = None,
                timeout = None,
                discloseMe = None,
                runOn = None):
      """
      Constructor.

      :param onProgress: A callback that will be called when the remote endpoint
                         called yields interim call progress results.
      :type onProgress: a callable
      :param timeout: Time in seconds after which the call should be automatically canceled.
      :type timeout: float
      :param discloseMe: Request to disclose the identity of the caller (it's WAMP session ID)
                         to Callees. Note that a Dealer, depending on Dealer configuration, might
                         reject the request, or might disclose the Callee's identity without
                         a request to do so.
      :type discloseMe: bool
      :param runOn: If present (non-None), indicates a distributed call. Distributed calls allows
                    to run a call issued by a Caller on one or more endpoints implementing the
                    called procedure. Permissible value are: "all", "any" and "partition".
                    If `runOne == "partition"`, then `runPartitions` MUST be present.
      :type runOn: str
      """
      assert(onProgress is None or callable(onProgress))
      assert(timeout is None or (type(timeout) in list(six.integer_types) + [float] and timeout > 0))
      assert(discloseMe is None or type(discloseMe) == bool)
      assert(runOn is None or (type(runOn) == six.text_type and runOn in [u"all", u"any", u"partition"]))

      self.onProgress = onProgress
      self.timeout = timeout
      self.discloseMe = discloseMe
      self.runOn = runOn

      ## options dict as sent within WAMP message
      self.options = {
         'timeout': timeout,
         'discloseMe': discloseMe
      }
      if onProgress:
         self.options['receive_progress'] = True


   def __str__(self):
      return "CallOptions(onProgress = {}, timeout = {}, discloseMe = {}, runOn = {})".format(self.onProgress, self.timeout, self.discloseMe, self.runOn)



class CallResult:
   """
   Wrapper for remote procedure call results that contain multiple positional
   return values or keyword return values.
   """

   def __init__(self, *results, **kwresults):
      """
      Constructor.

      :param results: The positional result values.
      :type results: list
      :param kwresults: The keyword result values.
      :type kwresults: dict
      """
      self.results = results
      self.kwresults = kwresults


   def __str__(self):
      return "CallResult(results = {}, kwresults = {})".format(self.results, self.kwresults)
