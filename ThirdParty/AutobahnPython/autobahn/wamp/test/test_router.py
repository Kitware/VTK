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
   USE_TWISTED = True
   USE_ASYNCIO = False
elif os.environ.get('USE_ASYNCIO', False):
   USE_TWISTED = False
   USE_ASYNCIO = True
else:
   raise Exception("Neither USE_TWISTED nor USE_ASYNCIO set")


from autobahn.wamp import types

if USE_TWISTED:
   from twisted.trial import unittest
   #import unittest

   from twisted.internet.defer import inlineCallbacks, Deferred
   from autobahn.twisted.wamp import FutureMixin, \
                                     RouterFactory, \
                                     RouterSessionFactory, \
                                     ApplicationSession
elif USE_ASYNCIO:
   import unittest

   import logging
   logger = logging.getLogger('trollius')
   logger.setLevel(logging.DEBUG)
   handler = logging.StreamHandler()
   handler.setLevel(logging.DEBUG)
   logger.addHandler(handler)

   try:
      import asyncio
   except ImportError:
      import trollius as asyncio

   import gc

   from autobahn.asyncio.wamp import FutureMixin, \
                                     RouterFactory, \
                                     RouterSessionFactory, \
                                     ApplicationSession




class TestEmbeddedSessions(unittest.TestCase):
   """
   Test cases for application session running embedded in router.
   """

   def setUp(self):
      """
      Setup router and router session factories.
      """
      if USE_ASYNCIO:
         self.loop = asyncio.new_event_loop()
         asyncio.set_event_loop(self.loop)

      self.router_factory = RouterFactory()
      self.session_factory = RouterSessionFactory(self.router_factory)


   def tearDown(self):
      if USE_ASYNCIO:
         self.loop.close()
         asyncio.set_event_loop(None)
         gc.collect()


   def test_add(self):
      """
      Create an application session and add it to a router to
      run embedded.
      """
      d = FutureMixin._create_future()

      class TestSession(ApplicationSession):

         def onJoin(self, details):
            FutureMixin._resolve_future(d, None)

      session = TestSession(types.ComponentConfig('realm1'))

      self.session_factory.add(session)

      if USE_ASYNCIO:
         self.loop.run_until_complete(d)
      elif USE_TWISTED:
         return d
      else:
         raise Exception("logic error")


   def test_add_and_subscribe(self):
      """
      Create an application session that subscribes to some
      topic and add it to a router to run embedded.
      """
      d = FutureMixin._create_future()

      class TestSession(ApplicationSession):

         def onJoin(self, details):
            def on_event(*arg, **kwargs):
               pass

            d2 = self.subscribe(on_event, u'com.example.topic1')

            def ok(_):
               FutureMixin._resolve_future(d, None)
            def error(err):
               FutureMixin._reject_future(d, err)

            FutureMixin._add_future_callbacks(d2, ok, error)

      session = TestSession(types.ComponentConfig('realm1'))

      self.session_factory.add(session)

      if USE_ASYNCIO:
         self.loop.run_until_complete(d)
      elif USE_TWISTED:
         return d
      else:
         raise Exception("logic error")

      #d.addCallback(self.assertEqual, expected)
