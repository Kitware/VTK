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

from autobahn.wamp import role
from autobahn.wamp import message
from autobahn.wamp.exception import ProtocolError


class TestErrorMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Error(message.Call.MESSAGE_TYPE, 123456, u'com.myapp.error1')
      msg = e.marshal()
      self.assertEqual(len(msg), 5)
      self.assertEqual(msg[0], message.Error.MESSAGE_TYPE)
      self.assertEqual(msg[1], message.Call.MESSAGE_TYPE)
      self.assertEqual(msg[2], 123456)
      self.assertEqual(msg[3], {})
      self.assertEqual(msg[4], u'com.myapp.error1')

      e = message.Error(message.Call.MESSAGE_TYPE, 123456, u'com.myapp.error1', args = [1, 2, 3], kwargs = {u'foo': 23, u'bar': u'hello'})
      msg = e.marshal()
      self.assertEqual(len(msg), 7)
      self.assertEqual(msg[0], message.Error.MESSAGE_TYPE)
      self.assertEqual(msg[1], message.Call.MESSAGE_TYPE)
      self.assertEqual(msg[2], 123456)
      self.assertEqual(msg[3], {})
      self.assertEqual(msg[4], u'com.myapp.error1')
      self.assertEqual(msg[5], [1, 2, 3])
      self.assertEqual(msg[6], {u'foo': 23, u'bar': u'hello'})


   def test_parse_and_marshal(self):
      wmsg = [message.Error.MESSAGE_TYPE, message.Call.MESSAGE_TYPE, 123456, {}, u'com.myapp.error1']
      msg = message.Error.parse(wmsg)
      self.assertIsInstance(msg, message.Error)
      self.assertEqual(msg.request_type, message.Call.MESSAGE_TYPE)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.error, u'com.myapp.error1')
      self.assertEqual(msg.args, None)
      self.assertEqual(msg.kwargs, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Error.MESSAGE_TYPE, message.Call.MESSAGE_TYPE, 123456, {}, u'com.myapp.error1', [1, 2, 3], {u'foo': 23,  u'bar':  u'hello'}]
      msg = message.Error.parse(wmsg)
      self.assertIsInstance(msg, message.Error)
      self.assertEqual(msg.request_type, message.Call.MESSAGE_TYPE)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.error, u'com.myapp.error1')
      self.assertEqual(msg.args, [1, 2, 3])
      self.assertEqual(msg.kwargs, {u'foo': 23,  u'bar':  u'hello'})
      self.assertEqual(msg.marshal(), wmsg)



class TestSubscribeMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Subscribe(123456, u'com.myapp.topic1')
      msg = e.marshal()
      self.assertEqual(len(msg), 4)
      self.assertEqual(msg[0], message.Subscribe.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {})
      self.assertEqual(msg[3], u'com.myapp.topic1')

      e = message.Subscribe(123456, u'com.myapp.topic1', match = message.Subscribe.MATCH_PREFIX)
      msg = e.marshal()
      self.assertEqual(len(msg), 4)
      self.assertEqual(msg[0], message.Subscribe.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {u'match': u'prefix'})
      self.assertEqual(msg[3], u'com.myapp.topic1')


   def test_parse_and_marshal(self):
      wmsg = [message.Subscribe.MESSAGE_TYPE, 123456, {}, u'com.myapp.topic1']
      msg = message.Subscribe.parse(wmsg)
      self.assertIsInstance(msg, message.Subscribe)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.topic, u'com.myapp.topic1')
      self.assertEqual(msg.match, message.Subscribe.MATCH_EXACT)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Subscribe.MESSAGE_TYPE, 123456, {u'match': u'prefix'}, u'com.myapp.topic1']
      msg = message.Subscribe.parse(wmsg)
      self.assertIsInstance(msg, message.Subscribe)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.topic, u'com.myapp.topic1')
      self.assertEqual(msg.match, message.Subscribe.MATCH_PREFIX)
      self.assertEqual(msg.marshal(), wmsg)



class TestSubscribedMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Subscribed(123456, 789123)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Subscribed.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], 789123)


   def test_parse_and_marshal(self):
      wmsg = [message.Subscribed.MESSAGE_TYPE, 123456, 789123]
      msg = message.Subscribed.parse(wmsg)
      self.assertIsInstance(msg, message.Subscribed)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.subscription, 789123)
      self.assertEqual(msg.marshal(), wmsg)



class TestUnsubscribeMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Unsubscribe(123456, 789123)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Unsubscribe.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], 789123)


   def test_parse_and_marshal(self):
      wmsg = [message.Unsubscribe.MESSAGE_TYPE, 123456, 789123]
      msg = message.Unsubscribe.parse(wmsg)
      self.assertIsInstance(msg, message.Unsubscribe)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.subscription, 789123)
      self.assertEqual(msg.marshal(), wmsg)



class TestUnsubscribedMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Unsubscribed(123456)
      msg = e.marshal()
      self.assertEqual(len(msg), 2)
      self.assertEqual(msg[0], message.Unsubscribed.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)


   def test_parse_and_marshal(self):
      wmsg = [message.Unsubscribed.MESSAGE_TYPE, 123456]
      msg = message.Unsubscribed.parse(wmsg)
      self.assertIsInstance(msg, message.Unsubscribed)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.marshal(), wmsg)



class TestPublishMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Publish(123456, u'com.myapp.topic1')
      msg = e.marshal()
      self.assertEqual(len(msg), 4)
      self.assertEqual(msg[0], message.Publish.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {})
      self.assertEqual(msg[3], u'com.myapp.topic1')

      e = message.Publish(123456, u'com.myapp.topic1', args = [1, 2, 3], kwargs = {u'foo': 23,  u'bar':  u'hello'})
      msg = e.marshal()
      self.assertEqual(len(msg), 6)
      self.assertEqual(msg[0], message.Publish.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {})
      self.assertEqual(msg[3], u'com.myapp.topic1')
      self.assertEqual(msg[4], [1, 2, 3])
      self.assertEqual(msg[5], {u'foo': 23,  u'bar':  u'hello'})

      e = message.Publish(123456, u'com.myapp.topic1', excludeMe = False, exclude = [300], eligible = [100, 200, 300], discloseMe = True)
      msg = e.marshal()
      self.assertEqual(len(msg), 4)
      self.assertEqual(msg[0], message.Publish.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {u'exclude_me': False, u'disclose_me': True, u'exclude': [300], u'eligible': [100, 200, 300]})
      self.assertEqual(msg[3], u'com.myapp.topic1')


   def test_parse_and_marshal(self):
      wmsg = [message.Publish.MESSAGE_TYPE, 123456, {}, u'com.myapp.topic1']
      msg = message.Publish.parse(wmsg)
      self.assertIsInstance(msg, message.Publish)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.topic, u'com.myapp.topic1')
      self.assertEqual(msg.args, None)
      self.assertEqual(msg.kwargs, None)
      self.assertEqual(msg.excludeMe, None)
      self.assertEqual(msg.exclude, None)
      self.assertEqual(msg.eligible, None)
      self.assertEqual(msg.discloseMe, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Publish.MESSAGE_TYPE, 123456, {}, u'com.myapp.topic1', [1, 2, 3], {u'foo': 23,  u'bar':  u'hello'}]
      msg = message.Publish.parse(wmsg)
      self.assertIsInstance(msg, message.Publish)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.topic, u'com.myapp.topic1')
      self.assertEqual(msg.args, [1, 2, 3])
      self.assertEqual(msg.kwargs, {u'foo': 23,  u'bar':  u'hello'})
      self.assertEqual(msg.excludeMe, None)
      self.assertEqual(msg.exclude, None)
      self.assertEqual(msg.eligible, None)
      self.assertEqual(msg.discloseMe, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Publish.MESSAGE_TYPE, 123456, {u'exclude_me': False, u'disclose_me': True, u'exclude': [300], u'eligible': [100, 200, 300]}, u'com.myapp.topic1']
      msg = message.Publish.parse(wmsg)
      self.assertIsInstance(msg, message.Publish)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.topic, u'com.myapp.topic1')
      self.assertEqual(msg.args, None)
      self.assertEqual(msg.kwargs, None)
      self.assertEqual(msg.excludeMe, False)
      self.assertEqual(msg.exclude, [300])
      self.assertEqual(msg.eligible, [100, 200, 300])
      self.assertEqual(msg.discloseMe, True)
      self.assertEqual(msg.marshal(), wmsg)



class TestPublishedMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Published(123456, 789123)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Published.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], 789123)


   def test_parse_and_marshal(self):
      wmsg = [message.Published.MESSAGE_TYPE, 123456, 789123]
      msg = message.Published.parse(wmsg)
      self.assertIsInstance(msg, message.Published)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.publication, 789123)
      self.assertEqual(msg.marshal(), wmsg)



class TestEventMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Event(123456, 789123)
      msg = e.marshal()
      self.assertEqual(len(msg), 4)
      self.assertEqual(msg[0], message.Event.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], 789123)
      self.assertEqual(msg[3], {})

      e = message.Event(123456, 789123, args = [1, 2, 3], kwargs = {u'foo': 23,  u'bar':  u'hello'})
      msg = e.marshal()
      self.assertEqual(len(msg), 6)
      self.assertEqual(msg[0], message.Event.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], 789123)
      self.assertEqual(msg[3], {})
      self.assertEqual(msg[4], [1, 2, 3])
      self.assertEqual(msg[5], {u'foo': 23,  u'bar':  u'hello'})

      e = message.Event(123456, 789123, publisher = 300)
      msg = e.marshal()
      self.assertEqual(len(msg), 4)
      self.assertEqual(msg[0], message.Event.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], 789123)
      self.assertEqual(msg[3], {u'publisher': 300})


   def test_parse_and_marshal(self):
      wmsg = [message.Event.MESSAGE_TYPE, 123456, 789123, {}]
      msg = message.Event.parse(wmsg)
      self.assertIsInstance(msg, message.Event)
      self.assertEqual(msg.subscription, 123456)
      self.assertEqual(msg.publication, 789123)
      self.assertEqual(msg.args, None)
      self.assertEqual(msg.kwargs, None)
      self.assertEqual(msg.publisher, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Event.MESSAGE_TYPE, 123456, 789123, {}, [1, 2, 3], {u'foo': 23,  u'bar':  u'hello'}]
      msg = message.Event.parse(wmsg)
      self.assertIsInstance(msg, message.Event)
      self.assertEqual(msg.subscription, 123456)
      self.assertEqual(msg.publication, 789123)
      self.assertEqual(msg.args, [1, 2, 3])
      self.assertEqual(msg.kwargs, {u'foo': 23,  u'bar':  u'hello'})
      self.assertEqual(msg.publisher, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Event.MESSAGE_TYPE, 123456, 789123, {u'publisher': 300}]
      msg = message.Event.parse(wmsg)
      self.assertIsInstance(msg, message.Event)
      self.assertEqual(msg.subscription, 123456)
      self.assertEqual(msg.publication, 789123)
      self.assertEqual(msg.args, None)
      self.assertEqual(msg.kwargs, None)
      self.assertEqual(msg.publisher, 300)
      self.assertEqual(msg.marshal(), wmsg)



class TestRegisterMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Register(123456, u'com.myapp.procedure1')
      msg = e.marshal()
      self.assertEqual(len(msg), 4)
      self.assertEqual(msg[0], message.Register.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {})
      self.assertEqual(msg[3], u'com.myapp.procedure1')

      e = message.Register(123456, u'com.myapp.procedure1', pkeys = [10, 11, 12])
      msg = e.marshal()
      self.assertEqual(len(msg), 4)
      self.assertEqual(msg[0], message.Register.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {u'pkeys': [10, 11, 12]})
      self.assertEqual(msg[3], u'com.myapp.procedure1')


   def test_parse_and_marshal(self):
      wmsg = [message.Register.MESSAGE_TYPE, 123456, {}, u'com.myapp.procedure1']
      msg = message.Register.parse(wmsg)
      self.assertIsInstance(msg, message.Register)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.procedure, u'com.myapp.procedure1')
      self.assertEqual(msg.pkeys, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Register.MESSAGE_TYPE, 123456, {u'pkeys': [10, 11, 12]}, u'com.myapp.procedure1']
      msg = message.Register.parse(wmsg)
      self.assertIsInstance(msg, message.Register)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.procedure, u'com.myapp.procedure1')
      self.assertEqual(msg.pkeys, [10, 11, 12])
      self.assertEqual(msg.marshal(), wmsg)



class TestRegisteredMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Registered(123456, 789123)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Registered.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], 789123)


   def test_parse_and_marshal(self):
      wmsg = [message.Registered.MESSAGE_TYPE, 123456, 789123]
      msg = message.Registered.parse(wmsg)
      self.assertIsInstance(msg, message.Registered)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.registration, 789123)
      self.assertEqual(msg.marshal(), wmsg)



class TestUnregisterMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Unregister(123456, 789123)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Unregister.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], 789123)


   def test_parse_and_marshal(self):
      wmsg = [message.Unregister.MESSAGE_TYPE, 123456, 789123]
      msg = message.Unregister.parse(wmsg)
      self.assertIsInstance(msg, message.Unregister)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.registration, 789123)
      self.assertEqual(msg.marshal(), wmsg)



class TestUnregisteredMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Unregistered(123456)
      msg = e.marshal()
      self.assertEqual(len(msg), 2)
      self.assertEqual(msg[0], message.Unregistered.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)


   def test_parse_and_marshal(self):
      wmsg = [message.Unregistered.MESSAGE_TYPE, 123456]
      msg = message.Unregistered.parse(wmsg)
      self.assertIsInstance(msg, message.Unregistered)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.marshal(), wmsg)



class TestCallMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Call(123456, u'com.myapp.procedure1')
      msg = e.marshal()
      self.assertEqual(len(msg), 4)
      self.assertEqual(msg[0], message.Call.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {})
      self.assertEqual(msg[3], u'com.myapp.procedure1')

      e = message.Call(123456, u'com.myapp.procedure1', args = [1, 2, 3], kwargs = {u'foo': 23,  u'bar':  u'hello'})
      msg = e.marshal()
      self.assertEqual(len(msg), 6)
      self.assertEqual(msg[0], message.Call.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {})
      self.assertEqual(msg[3], u'com.myapp.procedure1')
      self.assertEqual(msg[4], [1, 2, 3])
      self.assertEqual(msg[5], {u'foo': 23,  u'bar':  u'hello'})

      e = message.Call(123456, u'com.myapp.procedure1', timeout = 10000)
      msg = e.marshal()
      self.assertEqual(len(msg), 4)
      self.assertEqual(msg[0], message.Call.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {u'timeout': 10000})
      self.assertEqual(msg[3], u'com.myapp.procedure1')


   def test_parse_and_marshal(self):
      wmsg = [message.Call.MESSAGE_TYPE, 123456, {}, u'com.myapp.procedure1']
      msg = message.Call.parse(wmsg)
      self.assertIsInstance(msg, message.Call)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.procedure, u'com.myapp.procedure1')
      self.assertEqual(msg.args, None)
      self.assertEqual(msg.kwargs, None)
      self.assertEqual(msg.timeout, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Call.MESSAGE_TYPE, 123456, {}, u'com.myapp.procedure1', [1, 2, 3], {u'foo': 23,  u'bar':  u'hello'}]
      msg = message.Call.parse(wmsg)
      self.assertIsInstance(msg, message.Call)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.procedure, u'com.myapp.procedure1')
      self.assertEqual(msg.args, [1, 2, 3])
      self.assertEqual(msg.kwargs, {u'foo': 23,  u'bar':  u'hello'})
      self.assertEqual(msg.timeout, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Call.MESSAGE_TYPE, 123456, {u'timeout': 10000}, u'com.myapp.procedure1']
      msg = message.Call.parse(wmsg)
      self.assertIsInstance(msg, message.Call)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.procedure, u'com.myapp.procedure1')
      self.assertEqual(msg.args, None)
      self.assertEqual(msg.kwargs, None)
      self.assertEqual(msg.timeout, 10000)
      self.assertEqual(msg.marshal(), wmsg)



class TestCancelMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Cancel(123456)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Cancel.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {})

      e = message.Cancel(123456, mode = message.Cancel.KILL)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Cancel.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {u'mode': message.Cancel.KILL})


   def test_parse_and_marshal(self):
      wmsg = [message.Cancel.MESSAGE_TYPE, 123456, {}]
      msg = message.Cancel.parse(wmsg)
      self.assertIsInstance(msg, message.Cancel)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.mode, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Cancel.MESSAGE_TYPE, 123456, {u'mode': message.Cancel.KILL}]
      msg = message.Cancel.parse(wmsg)
      self.assertIsInstance(msg, message.Cancel)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.mode, message.Cancel.KILL)
      self.assertEqual(msg.marshal(), wmsg)



class TestResultMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Result(123456)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Result.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {})

      e = message.Result(123456, args = [1, 2, 3], kwargs = {u'foo': 23,  u'bar':  u'hello'})
      msg = e.marshal()
      self.assertEqual(len(msg), 5)
      self.assertEqual(msg[0], message.Result.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {})
      self.assertEqual(msg[3], [1, 2, 3])
      self.assertEqual(msg[4], {u'foo': 23,  u'bar':  u'hello'})

      e = message.Result(123456, progress = True)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Result.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {u'progress': True})


   def test_parse_and_marshal(self):
      wmsg = [message.Result.MESSAGE_TYPE, 123456, {}]
      msg = message.Result.parse(wmsg)
      self.assertIsInstance(msg, message.Result)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.args, None)
      self.assertEqual(msg.kwargs, None)
      self.assertEqual(msg.progress, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Result.MESSAGE_TYPE, 123456, {}, [1, 2, 3], {u'foo': 23,  u'bar':  u'hello'}]
      msg = message.Result.parse(wmsg)
      self.assertIsInstance(msg, message.Result)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.args, [1, 2, 3])
      self.assertEqual(msg.kwargs, {u'foo': 23,  u'bar':  u'hello'})
      self.assertEqual(msg.progress, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Result.MESSAGE_TYPE, 123456, {u'progress': True}]
      msg = message.Result.parse(wmsg)
      self.assertIsInstance(msg, message.Result)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.args, None)
      self.assertEqual(msg.kwargs, None)
      self.assertEqual(msg.progress, True)
      self.assertEqual(msg.marshal(), wmsg)



class TestInvocationMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Invocation(123456, 789123)
      msg = e.marshal()
      self.assertEqual(len(msg), 4)
      self.assertEqual(msg[0], message.Invocation.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], 789123)
      self.assertEqual(msg[3], {})

      e = message.Invocation(123456, 789123, args = [1, 2, 3], kwargs = {u'foo': 23,  u'bar':  u'hello'})
      msg = e.marshal()
      self.assertEqual(len(msg), 6)
      self.assertEqual(msg[0], message.Invocation.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], 789123)
      self.assertEqual(msg[3], {})
      self.assertEqual(msg[4], [1, 2, 3])
      self.assertEqual(msg[5], {u'foo': 23,  u'bar':  u'hello'})

      e = message.Invocation(123456, 789123, timeout = 10000)
      msg = e.marshal()
      self.assertEqual(len(msg), 4)
      self.assertEqual(msg[0], message.Invocation.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], 789123)
      self.assertEqual(msg[3], {u'timeout': 10000})


   def test_parse_and_marshal(self):
      wmsg = [message.Invocation.MESSAGE_TYPE, 123456, 789123, {}]
      msg = message.Invocation.parse(wmsg)
      self.assertIsInstance(msg, message.Invocation)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.registration, 789123)
      self.assertEqual(msg.args, None)
      self.assertEqual(msg.kwargs, None)
      self.assertEqual(msg.timeout, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Invocation.MESSAGE_TYPE, 123456, 789123, {}, [1, 2, 3], {u'foo': 23,  u'bar':  u'hello'}]
      msg = message.Invocation.parse(wmsg)
      self.assertIsInstance(msg, message.Invocation)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.registration, 789123)
      self.assertEqual(msg.args, [1, 2, 3])
      self.assertEqual(msg.kwargs, {u'foo': 23,  u'bar':  u'hello'})
      self.assertEqual(msg.timeout, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Invocation.MESSAGE_TYPE, 123456, 789123, {u'timeout': 10000}]
      msg = message.Invocation.parse(wmsg)
      self.assertIsInstance(msg, message.Invocation)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.registration, 789123)
      self.assertEqual(msg.args, None)
      self.assertEqual(msg.kwargs, None)
      self.assertEqual(msg.timeout, 10000)
      self.assertEqual(msg.marshal(), wmsg)



class TestInterruptMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Interrupt(123456)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Interrupt.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {})

      e = message.Interrupt(123456, mode = message.Interrupt.KILL)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Interrupt.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {u'mode': message.Interrupt.KILL})


   def test_parse_and_marshal(self):
      wmsg = [message.Interrupt.MESSAGE_TYPE, 123456, {}]
      msg = message.Interrupt.parse(wmsg)
      self.assertIsInstance(msg, message.Interrupt)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.mode, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Interrupt.MESSAGE_TYPE, 123456, {u'mode': message.Interrupt.KILL}]
      msg = message.Interrupt.parse(wmsg)
      self.assertIsInstance(msg, message.Interrupt)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.mode, message.Interrupt.KILL)
      self.assertEqual(msg.marshal(), wmsg)



class TestYieldMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Yield(123456)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Yield.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {})

      e = message.Yield(123456, args = [1, 2, 3], kwargs = {u'foo': 23,  u'bar':  u'hello'})
      msg = e.marshal()
      self.assertEqual(len(msg), 5)
      self.assertEqual(msg[0], message.Yield.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {})
      self.assertEqual(msg[3], [1, 2, 3])
      self.assertEqual(msg[4], {u'foo': 23,  u'bar':  u'hello'})

      e = message.Yield(123456, progress = True)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Yield.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123456)
      self.assertEqual(msg[2], {u'progress': True})


   def test_parse_and_marshal(self):
      wmsg = [message.Yield.MESSAGE_TYPE, 123456, {}]
      msg = message.Yield.parse(wmsg)
      self.assertIsInstance(msg, message.Yield)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.args, None)
      self.assertEqual(msg.kwargs, None)
      self.assertEqual(msg.progress, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Yield.MESSAGE_TYPE, 123456, {}, [1, 2, 3], {u'foo': 23,  u'bar':  u'hello'}]
      msg = message.Yield.parse(wmsg)
      self.assertIsInstance(msg, message.Yield)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.args, [1, 2, 3])
      self.assertEqual(msg.kwargs, {u'foo': 23,  u'bar':  u'hello'})
      self.assertEqual(msg.progress, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Yield.MESSAGE_TYPE, 123456, {u'progress': True}]
      msg = message.Yield.parse(wmsg)
      self.assertIsInstance(msg, message.Yield)
      self.assertEqual(msg.request, 123456)
      self.assertEqual(msg.args, None)
      self.assertEqual(msg.kwargs, None)
      self.assertEqual(msg.progress, True)
      self.assertEqual(msg.marshal(), wmsg)



class TestHelloMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Hello(u"realm1", [role.RoleBrokerFeatures()])
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Hello.MESSAGE_TYPE)
      self.assertEqual(msg[1], u"realm1")
      self.assertEqual(msg[2], {u'roles': {u'broker': {}}})

      e = message.Hello(u"realm1", [role.RoleBrokerFeatures(subscriber_blackwhite_listing = True)])
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Hello.MESSAGE_TYPE)
      self.assertEqual(msg[1], u"realm1")
      self.assertEqual(msg[2], {u'roles': {u'broker': {u'features': {u'subscriber_blackwhite_listing': True}}}})


   def test_parse_and_marshal(self):
      wmsg = [message.Hello.MESSAGE_TYPE, u"realm1", {u'roles': {u'broker': {}}}]
      msg = message.Hello.parse(wmsg)
      self.assertIsInstance(msg, message.Hello)
      self.assertEqual(msg.realm, u"realm1")
      self.assertEqual(msg.roles, [role.RoleBrokerFeatures()])
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Hello.MESSAGE_TYPE, u"realm1", {u'roles': {u'broker': {u'features': {u'subscriber_blackwhite_listing': True}}}}]
      msg = message.Hello.parse(wmsg)
      self.assertIsInstance(msg, message.Hello)
      self.assertEqual(msg.realm, u"realm1")
      self.assertEqual(msg.roles, [role.RoleBrokerFeatures(subscriber_blackwhite_listing = True)])
      self.assertEqual(msg.marshal(), wmsg)

   def test_str(self):
      e = message.Hello(u"realm1", [role.RoleBrokerFeatures()])
      self.assertIsInstance(str(e), str)



class TestGoodbyeMessage(unittest.TestCase):

   def test_ctor(self):
      reason = u'wamp.error.system_shutdown'
      reason_msg = u'The host is shutting down now.'

      e = message.Goodbye()
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Goodbye.MESSAGE_TYPE)
      self.assertEqual(msg[1], {})
      self.assertEqual(msg[2], message.Goodbye.DEFAULT_REASON)

      e = message.Goodbye(reason = reason)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Goodbye.MESSAGE_TYPE)
      self.assertEqual(msg[1], {})
      self.assertEqual(msg[2], reason)

      e = message.Goodbye(reason = reason, message = reason_msg)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Goodbye.MESSAGE_TYPE)
      self.assertEqual(msg[1], {u'message': reason_msg})
      self.assertEqual(msg[2], reason)


   def test_parse_and_marshal(self):
      reason = u'wamp.error.system_shutdown'
      reason_msg = u'The host is shutting down now.'

      wmsg = [message.Goodbye.MESSAGE_TYPE]
      self.assertRaises(ProtocolError, message.Goodbye.parse, wmsg)

      wmsg = [message.Goodbye.MESSAGE_TYPE, reason]
      self.assertRaises(ProtocolError, message.Goodbye.parse, wmsg)

      wmsg = [message.Goodbye.MESSAGE_TYPE, {u'message': 100}, reason]
      self.assertRaises(ProtocolError, message.Goodbye.parse, wmsg)

      wmsg = [message.Goodbye.MESSAGE_TYPE, {}, reason]
      msg = message.Goodbye.parse(wmsg)
      self.assertIsInstance(msg, message.Goodbye)
      self.assertEqual(msg.reason, reason)
      self.assertEqual(msg.message, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Goodbye.MESSAGE_TYPE, {u'message': reason_msg}, reason]
      msg = message.Goodbye.parse(wmsg)
      self.assertIsInstance(msg, message.Goodbye)
      self.assertEqual(msg.reason, reason)
      self.assertEqual(msg.message, reason_msg)
      self.assertEqual(msg.marshal(), wmsg)

   def test_str(self):
      e = message.Goodbye(reason = u'wamp.error.system_shutdown', message = u'The host is shutting down now.')
      self.assertIsInstance(str(e), str)



class TestHeartbeatMessage(unittest.TestCase):

   def test_ctor(self):
      e = message.Heartbeat(123, 456)
      msg = e.marshal()
      self.assertEqual(len(msg), 3)
      self.assertEqual(msg[0], message.Heartbeat.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123)
      self.assertEqual(msg[2], 456)

      e = message.Heartbeat(123, 456, u"discard me")
      msg = e.marshal()
      self.assertEqual(len(msg), 4)
      self.assertEqual(msg[0], message.Heartbeat.MESSAGE_TYPE)
      self.assertEqual(msg[1], 123)
      self.assertEqual(msg[2], 456)
      self.assertEqual(msg[3], u"discard me")


   def test_parse_and_marshal(self):
      wmsg = [message.Heartbeat.MESSAGE_TYPE]
      self.assertRaises(ProtocolError, message.Heartbeat.parse, wmsg)

      wmsg = [message.Heartbeat.MESSAGE_TYPE, 100]
      self.assertRaises(ProtocolError, message.Heartbeat.parse, wmsg)

      wmsg = [message.Heartbeat.MESSAGE_TYPE, 100, u"foo"]
      self.assertRaises(ProtocolError, message.Heartbeat.parse, wmsg)

      wmsg = [message.Heartbeat.MESSAGE_TYPE, 100, 0]
      self.assertRaises(ProtocolError, message.Heartbeat.parse, wmsg)

      wmsg = [message.Heartbeat.MESSAGE_TYPE, 100, 100, 100]
      self.assertRaises(ProtocolError, message.Heartbeat.parse, wmsg)

      wmsg = [message.Heartbeat.MESSAGE_TYPE, 123, 456]
      msg = message.Heartbeat.parse(wmsg)
      self.assertIsInstance(msg, message.Heartbeat)
      self.assertEqual(msg.incoming, 123)
      self.assertEqual(msg.outgoing, 456)
      self.assertEqual(msg.discard, None)
      self.assertEqual(msg.marshal(), wmsg)

      wmsg = [message.Heartbeat.MESSAGE_TYPE, 123, 456, u"discard me"]
      msg = message.Heartbeat.parse(wmsg)
      self.assertIsInstance(msg, message.Heartbeat)
      self.assertEqual(msg.incoming, 123)
      self.assertEqual(msg.outgoing, 456)
      self.assertEqual(msg.discard, u"discard me")
      self.assertEqual(msg.marshal(), wmsg)

   def test_str(self):
      e = message.Heartbeat(123, 456, u"discard me")
      self.assertIsInstance(str(e), str)



if __name__ == '__main__':
   unittest.main()
