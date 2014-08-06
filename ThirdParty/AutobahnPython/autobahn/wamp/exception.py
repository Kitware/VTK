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

from autobahn.wamp import error



class Error(RuntimeError):
   """
   Base class for all exceptions related to WAMP.
   """
   def __init__(self, reason):
      """
      Constructor.

      :param reason: Description of WAMP error that occurred (for logging purposes).
      :type reason: str
      """
      RuntimeError.__init__(self, reason)




class SessionNotReady(Error):
   """
   """



class SerializationError(Error):
   """
   Exception raised when the WAMP serializer could not serialize the
   application payload (args or kwargs for `CALL`, `PUBLISH`, etc).
   """


class ProtocolError(Error):
   """
   Exception raised when WAMP protocol was violated. Protocol errors
   are fatal and are handled by the WAMP implementation. They are
   not supposed to be handled at the application level.
   """



class TransportLost(Error):
   """
   Exception raised when transport was lost or is not connected.
   """
   def __init__(self):
      Error.__init__(self, "WAMP transport lost")



class ApplicationError(Error):
   """
   Base class for all exceptions that can/may be handled
   at the application level.
   """
   NOT_AUTHORIZED             = u"wamp.error.not_authorized"
   AUTHORIZATION_FAILED       = u"wamp.error.authorization_failed"
   INVALID_ARGUMENT           = u"wamp.error.invalid_argument"
   INVALID_URI                = u"wamp.error.invalid_uri"
   DISCLOSE_ME_NOT_ALLOWED    = u"wamp.error.disclose_me.not_allowed"
   PROCEDURE_ALREADY_EXISTS   = u"wamp.error.procedure_already_exists"
   NO_SUCH_REALM              = u"wamp.error.no_such_realm"
   NO_SUCH_ROLE               = u"wamp.error.no_such_role"
   SYSTEM_SHUTDOWN            = u"wamp.error.system_shutdown"
   CLOSE_REALM                = u"wamp.error.close_realm"
   GOODBYE_AND_OUT            = u"wamp.error.goodbye_and_out"
   NO_SUCH_REGISTRATION       = u"wamp.error.no_such_registration"
   NO_SUCH_SUBSCRIPTION       = u"wamp.error.no_such_subscription"
   NO_SUCH_PROCEDURE          = u"wamp.error.no_such_procedure"
   CANCELED                   = u"wamp.error.canceled"

   def __init__(self, error, *args, **kwargs):
      """
      Constructor.

      :param error: The URI of the error that occurred, e.g. `wamp.error.not_authorized`.
      :type error: str
      """
      Exception.__init__(self, *args)
      self.kwargs = kwargs
      self.error = error


   def __str__(self):
      if self.kwargs and 'traceback' in self.kwargs:
         tb = ':\n' + '\n'.join(self.kwargs.pop('traceback')) + '\n'
         self.kwargs['traceback'] = '...'
      else:
         tb = ''
      return "ApplicationError('{}', args = {}, kwargs = {}){}".format(self.error, self.args, self.kwargs, tb)



#class GenericException(Exception)

@error("wamp.error.not_authorized")
class NotAuthorized(Exception):
   """
   Not authorized to perform the respective action.
   """


@error("wamp.error.invalid_topic")
class InvalidTopic(Exception):
   """
   The topic to publish or subscribe to is invalid.
   """


class CallError(ApplicationError):
   """
   Remote procedure call errors.
   """

   def __init__(self, error, problem):
      """
      Constructor.

      :param error: The URI of the error that occurred, e.g. "com.myapp.error.no_such_user".
      :type error: str
      :param problem: Any application-level details for the error that occurred.
      :type problem: obj
      """
      ApplicationError.__init__(self, error)
      self.problem = problem



class CanceledError(ApplicationError):
   """
   Error for canceled calls.
   """

   def __init__(self):
      """
      Constructor.
      """
      ApplicationError.__init__(self, ApplicationError.CANCELED)
