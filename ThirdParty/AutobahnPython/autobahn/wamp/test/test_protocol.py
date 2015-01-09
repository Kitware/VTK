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

import os

if os.environ.get('USE_TWISTED', False):

   from twisted.trial import unittest
   #import unittest

   from twisted.internet.defer import Deferred, inlineCallbacks

   from autobahn import wamp
   from autobahn.wamp import message
   from autobahn.wamp import serializer
   from autobahn.wamp import protocol
   from autobahn.wamp import role
   from autobahn import util
   from autobahn.wamp.exception import ApplicationError, NotAuthorized, InvalidTopic
   from autobahn.wamp import types

   from autobahn.twisted.wamp import ApplicationSession


   class MockTransport:

      def __init__(self, handler):
         self._log = False
         self._handler = handler
         self._serializer = serializer.JsonSerializer()
         self._registrations = {}
         self._invocations = {}

         self._handler.onOpen(self)

         self._my_session_id = util.id()

         roles = [
            role.RoleBrokerFeatures(),
            role.RoleDealerFeatures()
         ]

         msg = message.Welcome(self._my_session_id, roles)
         self._handler.onMessage(msg)

      def send(self, msg):
         if self._log:
            bytes, isbinary = self._serializer.serialize(msg)
            print("Send: {}".format(bytes))

         reply = None

         if isinstance(msg, message.Publish):
            if msg.topic.startswith(u'com.myapp'):
               if msg.acknowledge:
                  reply = message.Published(msg.request, util.id())
            elif len(msg.topic) == 0:
               reply = message.Error(message.Publish.MESSAGE_TYPE, msg.request, u'wamp.error.invalid_topic')
            else:
               reply = message.Error(message.Publish.MESSAGE_TYPE, msg.request, u'wamp.error.not_authorized')

         elif isinstance(msg, message.Call):
            if msg.procedure == u'com.myapp.procedure1':
               reply = message.Result(msg.request, args = [100])
            elif msg.procedure == u'com.myapp.procedure2':
               reply = message.Result(msg.request, args = [1, 2, 3])
            elif msg.procedure == u'com.myapp.procedure3':
               reply = message.Result(msg.request, args = [1, 2, 3], kwargs = {u'foo': u'bar', u'baz': 23})

            elif msg.procedure.startswith(u'com.myapp.myproc'):
               registration = self._registrations[msg.procedure]
               request = util.id()
               self._invocations[request] = msg.request
               reply = message.Invocation(request, registration, args = msg.args, kwargs = msg.kwargs)
            else:
               reply = message.Error(message.Call.MESSAGE_TYPE, msg.request, u'wamp.error.no_such_procedure')

         elif isinstance(msg, message.Yield):
            if self._invocations.has_key(msg.request):
               request = self._invocations[msg.request]
               reply = message.Result(request, args = msg.args, kwargs = msg.kwargs)

         elif isinstance(msg, message.Subscribe):
            reply = message.Subscribed(msg.request, util.id())

         elif isinstance(msg, message.Unsubscribe):
            reply = message.Unsubscribed(msg.request)

         elif isinstance(msg, message.Register):
            registration = util.id()
            self._registrations[msg.procedure] = registration
            reply = message.Registered(msg.request, registration)

         elif isinstance(msg, message.Unregister):
            reply = message.Unregistered(msg.request)

         if reply:
            if self._log:
               bytes, isbinary = self._serializer.serialize(reply)
               print("Receive: {}".format(bytes))
            self._handler.onMessage(reply)

      def isOpen(self):
         return True

      def close(self):
         pass

      def abort(self):
         pass



   class TestPublisher(unittest.TestCase):

      @inlineCallbacks
      def test_publish(self):
         handler = ApplicationSession()
         MockTransport(handler)

         publication = yield handler.publish('com.myapp.topic1')
         self.assertEqual(publication, None)

         publication = yield handler.publish('com.myapp.topic1', 1, 2, 3)
         self.assertEqual(publication, None)

         publication = yield handler.publish('com.myapp.topic1', 1, 2, 3, foo = 23, bar = 'hello')
         self.assertEqual(publication, None)

         publication = yield handler.publish('com.myapp.topic1', options = types.PublishOptions(excludeMe = False))
         self.assertEqual(publication, None)

         publication = yield handler.publish('com.myapp.topic1', 1, 2, 3, foo = 23, bar = 'hello', options = types.PublishOptions(excludeMe = False, exclude = [100, 200, 300]))
         self.assertEqual(publication, None)


      @inlineCallbacks
      def test_publish_acknowledged(self):
         handler = ApplicationSession()
         MockTransport(handler)

         publication = yield handler.publish('com.myapp.topic1', options = types.PublishOptions(acknowledge = True))
         self.assertTrue(type(publication.id) in (int, long))

         publication = yield handler.publish('com.myapp.topic1', 1, 2, 3, options = types.PublishOptions(acknowledge = True))
         self.assertTrue(type(publication.id) in (int, long))

         publication = yield handler.publish('com.myapp.topic1', 1, 2, 3, foo = 23, bar = 'hello', options = types.PublishOptions(acknowledge = True))
         self.assertTrue(type(publication.id) in (int, long))

         publication = yield handler.publish('com.myapp.topic1', options = types.PublishOptions(excludeMe = False, acknowledge = True))
         self.assertTrue(type(publication.id) in (int, long))

         publication = yield handler.publish('com.myapp.topic1', 1, 2, 3, foo = 23, bar = 'hello', options = types.PublishOptions(excludeMe = False, exclude = [100, 200, 300], acknowledge = True))
         self.assertTrue(type(publication.id) in (int, long))


      @inlineCallbacks
      def test_publish_undefined_exception(self):
         handler = ApplicationSession()
         MockTransport(handler)

         options = types.PublishOptions(acknowledge = True)

         yield self.assertFailure(handler.publish(u'de.myapp.topic1', options = options), ApplicationError)
         yield self.assertFailure(handler.publish(u'', options = options), ApplicationError)


      @inlineCallbacks
      def test_publish_defined_exception(self):
         handler = ApplicationSession()
         MockTransport(handler)

         options = types.PublishOptions(acknowledge = True)

         handler.define(NotAuthorized)
         yield self.assertFailure(handler.publish(u'de.myapp.topic1', options = options), NotAuthorized)

         handler.define(InvalidTopic)
         yield self.assertFailure(handler.publish(u'', options = options), InvalidTopic)


      @inlineCallbacks
      def test_call(self):
         handler = ApplicationSession()
         MockTransport(handler)

         res = yield handler.call('com.myapp.procedure1')
         self.assertEqual(res, 100)

         res = yield handler.call('com.myapp.procedure1', 1, 2, 3)
         self.assertEqual(res, 100)

         res = yield handler.call('com.myapp.procedure1', 1, 2, 3, foo = 23, bar = 'hello')
         self.assertEqual(res, 100)

         res = yield handler.call('com.myapp.procedure1', options = types.CallOptions(timeout = 10000))
         self.assertEqual(res, 100)

         res = yield handler.call('com.myapp.procedure1', 1, 2, 3, foo = 23, bar = 'hello', options = types.CallOptions(timeout = 10000))
         self.assertEqual(res, 100)


      @inlineCallbacks
      def test_call_with_complex_result(self):
         handler = ApplicationSession()
         MockTransport(handler)

         res = yield handler.call('com.myapp.procedure2')
         self.assertIsInstance(res, types.CallResult)
         self.assertEqual(res.results, (1, 2, 3))
         self.assertEqual(res.kwresults, {})

         res = yield handler.call('com.myapp.procedure3')
         self.assertIsInstance(res, types.CallResult)
         self.assertEqual(res.results, (1, 2, 3))
         self.assertEqual(res.kwresults, {'foo':'bar', 'baz': 23})


      @inlineCallbacks
      def test_subscribe(self):
         handler = ApplicationSession()
         MockTransport(handler)

         def on_event(*args, **kwargs):
            print("got event", args, kwargs)

         subscription = yield handler.subscribe(on_event, 'com.myapp.topic1')
         self.assertTrue(type(subscription.id) in (int, long))

         subscription = yield handler.subscribe(on_event, 'com.myapp.topic1', options = types.SubscribeOptions(match = 'wildcard'))
         self.assertTrue(type(subscription.id) in (int, long))


      @inlineCallbacks
      def test_unsubscribe(self):
         handler = ApplicationSession()
         MockTransport(handler)

         def on_event(*args, **kwargs):
            print("got event", args, kwargs)

         subscription = yield handler.subscribe(on_event, 'com.myapp.topic1')
         yield subscription.unsubscribe()


      @inlineCallbacks
      def test_register(self):
         handler = ApplicationSession()
         MockTransport(handler)

         def on_call(*args, **kwargs):
            print("got call", args, kwargs)

         registration = yield handler.register(on_call, 'com.myapp.procedure1')
         self.assertTrue(type(registration.id) in (int, long))

         registration = yield handler.register(on_call, 'com.myapp.procedure1', options = types.RegisterOptions(pkeys = [0, 1, 2]))
         self.assertTrue(type(registration.id) in (int, long))


      @inlineCallbacks
      def test_unregister(self):
         handler = ApplicationSession()
         MockTransport(handler)

         def on_call(*args, **kwargs):
            print("got call", args, kwargs)

         registration = yield handler.register(on_call, 'com.myapp.procedure1')
         yield registration.unregister()


      @inlineCallbacks
      def test_invoke(self):
         handler = ApplicationSession()
         MockTransport(handler)

         def myproc1():
            return 23

         yield handler.register(myproc1, 'com.myapp.myproc1')

         res = yield handler.call('com.myapp.myproc1')
         self.assertEqual(res, 23)


      # ## variant 1: works
      # def test_publish1(self):
      #    d = self.handler.publish('de.myapp.topic1')
      #    self.assertFailure(d, ApplicationError)

      # ## variant 2: works
      # @inlineCallbacks
      # def test_publish2(self):
      #    yield self.assertFailure(self.handler.publish('de.myapp.topic1'), ApplicationError)

      # ## variant 3: does NOT work
      # @inlineCallbacks
      # def test_publish3(self):
      #    with self.assertRaises(ApplicationError):
      #       yield self.handler.publish('de.myapp.topic1')


if __name__ == '__main__':
   unittest.main()
