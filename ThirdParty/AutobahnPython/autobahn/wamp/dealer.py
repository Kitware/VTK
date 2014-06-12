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

from autobahn import util
from autobahn.wamp import types
from autobahn.wamp import role
from autobahn.wamp import message
from autobahn.wamp.exception import ProtocolError, ApplicationError
from autobahn.wamp.interfaces import IDealer

from autobahn.wamp.message import _URI_PAT_STRICT_NON_EMPTY, _URI_PAT_LOOSE_NON_EMPTY



class Dealer:
   """
   Basic WAMP dealer, implements :class:`autobahn.wamp.interfaces.IDealer`.
   """

   def __init__(self, realm, options):
      """
      Constructor.

      :param realm: The realm this dealer is working for.
      :type realm: str
      :param options: Router options.
      :type options: Instance of :class:`autobahn.wamp.types.RouterOptions`.
      """
      self.realm = realm
      self._options = options or types.RouterOptions()

      ## map: session -> set(registration)
      ## needed for removeSession
      self._session_to_registrations = {}

      ## map: session_id -> session
      ## needed for exclude/eligible
      self._session_id_to_session = {}

      ## map: procedure -> (registration, session)
      self._procs_to_regs = {}

      ## map: registration -> procedure
      self._regs_to_procs = {}

      ## pending callee invocation requests
      self._invocations = {}

      ## check all procedure URIs with strict rules
      self._option_uri_strict = self._options.uri_check == types.RouterOptions.URI_CHECK_STRICT

      ## supported features from "WAMP Advanced Profile"
      self._role_features = role.RoleDealerFeatures(caller_identification = True, progressive_call_results = True)


   def attach(self, session):
      """
      Implements :func:`autobahn.wamp.interfaces.IDealer.attach`
      """
      assert(session not in self._session_to_registrations)

      self._session_to_registrations[session] = set()
      self._session_id_to_session[session._session_id] = session


   def detach(self, session):
      """
      Implements :func:`autobahn.wamp.interfaces.IDealer.detach`
      """
      assert(session in self._session_to_registrations)

      for registration in self._session_to_registrations[session]:
         del self._procs_to_regs[self._regs_to_procs[registration]]
         del self._regs_to_procs[registration]

      del self._session_to_registrations[session]
      del self._session_id_to_session[session._session_id]


   def processRegister(self, session, register):
      """
      Implements :func:`autobahn.wamp.interfaces.IDealer.processRegister`
      """
      assert(session in self._session_to_registrations)

      ## check procedure URI
      ##
      if (not self._option_uri_strict and not  _URI_PAT_LOOSE_NON_EMPTY.match(register.procedure)) or \
         (    self._option_uri_strict and not _URI_PAT_STRICT_NON_EMPTY.match(register.procedure)):

         reply = message.Error(message.Register.MESSAGE_TYPE, register.request, ApplicationError.INVALID_URI, ["register for invalid procedure URI '{}'".format(register.procedure)])

      else:

         if not register.procedure in self._procs_to_regs:
            registration_id = util.id()
            self._procs_to_regs[register.procedure] = (registration_id, session, register.discloseCaller)
            self._regs_to_procs[registration_id] = register.procedure

            self._session_to_registrations[session].add(registration_id)

            reply = message.Registered(register.request, registration_id)
         else:
            reply = message.Error(message.Register.MESSAGE_TYPE, register.request, ApplicationError.PROCEDURE_ALREADY_EXISTS, ["register for already registered procedure URI '{}'".format(register.procedure)])

      session._transport.send(reply)


   def processUnregister(self, session, unregister):
      """
      Implements :func:`autobahn.wamp.interfaces.IDealer.processUnregister`
      """
      assert(session in self._session_to_registrations)

      if unregister.registration in self._regs_to_procs:
         del self._procs_to_regs[self._regs_to_procs[unregister.registration]]
         del self._regs_to_procs[unregister.registration]

         self._session_to_registrations[session].discard(unregister.registration)

         reply = message.Unregistered(unregister.request)
      else:
         reply = message.Error(message.Unregister.MESSAGE_TYPE, unregister.request, ApplicationError.NO_SUCH_REGISTRATION)

      session._transport.send(reply)


   def processCall(self, session, call):
      """
      Implements :func:`autobahn.wamp.interfaces.IDealer.processCall`
      """
      assert(session in self._session_to_registrations)

      ## check procedure URI
      ##
      if (not self._option_uri_strict and not  _URI_PAT_LOOSE_NON_EMPTY.match(call.procedure)) or \
         (    self._option_uri_strict and not _URI_PAT_STRICT_NON_EMPTY.match(call.procedure)):

         reply = message.Error(message.Register.MESSAGE_TYPE, call.request, ApplicationError.INVALID_URI, ["call with invalid procedure URI '{}'".format(call.procedure)])
         session._transport.send(reply)

      else:

         if call.procedure in self._procs_to_regs:
            registration_id, endpoint_session, discloseCaller = self._procs_to_regs[call.procedure]

            request_id = util.id()

            if discloseCaller or call.discloseMe:
               caller = session._session_id
               authid = session._authid
               authrole = session._authrole
               authmethod = session._authmethod
            else:
               caller = None
               authid = None
               authrole = None
               authmethod = None

            invocation = message.Invocation(request_id,
                                            registration_id,
                                            args = call.args,
                                            kwargs = call.kwargs,
                                            timeout = call.timeout,
                                            receive_progress = call.receive_progress,
                                            caller = caller,
                                            authid = authid,
                                            authrole = authrole,
                                            authmethod = authmethod)

            self._invocations[request_id] = (call, session)
            endpoint_session._transport.send(invocation)
         else:
            reply = message.Error(message.Call.MESSAGE_TYPE, call.request, ApplicationError.NO_SUCH_PROCEDURE, ["no procedure '{}' registered".format(call.procedure)])
            session._transport.send(reply)


   def processCancel(self, session, cancel):
      """
      Implements :func:`autobahn.wamp.interfaces.IDealer.processCancel`
      """
      assert(session in self._session_to_registrations)

      raise Exception("not implemented")


   def processYield(self, session, yield_):
      """
      Implements :func:`autobahn.wamp.interfaces.IDealer.processYield`
      """
      assert(session in self._session_to_registrations)

      if yield_.request in self._invocations:
         call_msg, call_session = self._invocations[yield_.request]
         msg = message.Result(call_msg.request, args = yield_.args, kwargs = yield_.kwargs, progress = yield_.progress)

         ## the calling session might have been lost in the meantime ..
         if call_session._transport:
            call_session._transport.send(msg)

         if not yield_.progress:
            del self._invocations[yield_.request]
      else:
         raise ProtocolError("Dealer.onYield(): YIELD received for non-pending request ID {}".format(yield_.request))


   def processInvocationError(self, session, error):
      """
      Implements :func:`autobahn.wamp.interfaces.IDealer.processInvocationError`
      """
      assert(session in self._session_to_registrations)

      if error.request in self._invocations:
         call_msg, call_session = self._invocations[error.request]
         msg = message.Error(message.Call.MESSAGE_TYPE, call_msg.request, error.error, args = error.args, kwargs = error.kwargs)

         ## the calling session might have been lost in the meantime ..
         if call_session._transport:
            call_session._transport.send(msg)

         del self._invocations[error.request]
      else:
         raise ProtocolError("Dealer.onInvocationError(): ERROR received for non-pending request_type {} and request ID {}".format(error.request_type, error.request))



IDealer.register(Dealer)
