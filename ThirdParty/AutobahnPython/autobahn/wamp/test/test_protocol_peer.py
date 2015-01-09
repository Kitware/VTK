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

import sys

if sys.version_info < (2,7):
   import unittest2 as unittest
else:
   #from twisted.trial import unittest
   import unittest

from autobahn import wamp
from autobahn.wamp import message
from autobahn.wamp import exception
from autobahn.wamp import protocol



class TestPeerExceptions(unittest.TestCase):

   def test_exception_from_message(self):
      session = protocol.BaseSession()

      @wamp.error("com.myapp.error1")
      class AppError1(Exception):
         pass

      @wamp.error("com.myapp.error2")
      class AppError2(Exception):
         pass

      session.define(AppError1)
      session.define(AppError2)

      ## map defined errors to user exceptions
      ##
      emsg = message.Error(message.Call.MESSAGE_TYPE, 123456, u'com.myapp.error1')
      exc = session._exception_from_message(emsg)
      self.assertIsInstance(exc, AppError1)
      self.assertEqual(exc.args, ())

      emsg = message.Error(message.Call.MESSAGE_TYPE, 123456, u'com.myapp.error2')
      exc = session._exception_from_message(emsg)
      self.assertIsInstance(exc, AppError2)
      self.assertEqual(exc.args, ())

      ## map undefined error to (generic) exception
      ##
      emsg = message.Error(message.Call.MESSAGE_TYPE, 123456, u'com.myapp.error3')
      exc = session._exception_from_message(emsg)
      self.assertIsInstance(exc, exception.ApplicationError)
      self.assertEqual(exc.error, u'com.myapp.error3')
      self.assertEqual(exc.args, ())
      self.assertEqual(exc.kwargs, {})

      emsg = message.Error(message.Call.MESSAGE_TYPE, 123456, u'com.myapp.error3', args = [1, 2, u'hello'])
      exc = session._exception_from_message(emsg)
      self.assertIsInstance(exc, exception.ApplicationError)
      self.assertEqual(exc.error, u'com.myapp.error3')
      self.assertEqual(exc.args, (1, 2, u'hello'))
      self.assertEqual(exc.kwargs, {})

      emsg = message.Error(message.Call.MESSAGE_TYPE, 123456, u'com.myapp.error3', args = [1, 2, u'hello'], kwargs = {u'foo': 23, u'bar': u'baz'})
      exc = session._exception_from_message(emsg)
      self.assertIsInstance(exc, exception.ApplicationError)
      self.assertEqual(exc.error, u'com.myapp.error3')
      self.assertEqual(exc.args, (1, 2, u'hello'))
      self.assertEqual(exc.kwargs, {u'foo': 23, u'bar': u'baz'})


   def test_message_from_exception(self):
      session = protocol.BaseSession()

      @wamp.error("com.myapp.error1")
      class AppError1(Exception):
         pass

      @wamp.error("com.myapp.error2")
      class AppError2(Exception):
         pass

      session.define(AppError1)
      session.define(AppError2)

      exc = AppError1()
      msg = session._message_from_exception(message.Call.MESSAGE_TYPE, 123456, exc)

      self.assertEqual(msg.marshal(), [message.Error.MESSAGE_TYPE, message.Call.MESSAGE_TYPE, 123456, {}, "com.myapp.error1"])


if __name__ == '__main__':
   unittest.main()
