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

from twisted.trial import unittest
#import unittest

from autobahn import wamp
from autobahn.wamp import message
from autobahn.wamp import role
from autobahn.wamp import serializer


import sys
import io
import six
import datetime

from twisted.python import log
from twisted.internet.defer import inlineCallbacks, Deferred, DeferredList
from twisted.internet.endpoints import serverFromString
from twisted.internet.endpoints import clientFromString
from autobahn.twisted.util import sleep

from autobahn.wamp import router
from autobahn.twisted.util import sleep
from autobahn.twisted import wamp, websocket

from autobahn.wamp.router import RouterFactory
from autobahn.twisted.wamp import RouterSessionFactory
from autobahn.wamp import types


from autobahn.wamp.serializer import MsgPackSerializer
from autobahn.wamp.serializer import JsonSerializer

from autobahn.twisted.wamp import ApplicationSessionFactory


from autobahn.twisted.rawsocket import WampRawSocketServerFactory, \
                                       WampRawSocketClientFactory, \
                                       WampRawSocketServerProtocol, \
                                       WampRawSocketClientProtocol

from autobahn.twisted.websocket import WampWebSocketServerFactory, \
                                       WampWebSocketClientFactory, \
                                       WampWebSocketClientProtocol, \
                                       WampWebSocketServerProtocol




class CaseComponent(wamp.ApplicationSession):
   """
   Application code goes here. This is an example component that calls
   a remote procedure on a WAMP peer, subscribes to a topic to receive
   events, and then stops the world after some events.
   """

   def __init__(self, config, done):
      wamp.ApplicationSession.__init__(self, config)
      self._done = done
      self.stop = False
      self._logline = 1
      self.finished = False

   def log(self, *args):
      if len(args) > 1:
         sargs = ", ".join(str(s) for s in args)
      elif len(args) == 1:
         sargs = args[0]
      else:
         sargs = "-"

      msg = u'= : {:>3} : {:<20} : {}'.format(self._logline, self.__class__.__name__, sargs)
      self._logline += 1
      print(msg)
      self.config.dlog.append(msg)
      if self.config.log and not self.config.log.closed:
         self.config.log.write(msg + "\n")
         self.config.log.flush()
      else:
         print("log already closed")

   def finish(self):
      if not self.finished:
         self._done.callback(None)
         self.finished = True
         self.disconnect()
      else:
         print("already finished")



class Case1_Backend(CaseComponent):

   @inlineCallbacks
   def onJoin(self, details):

      self.log("joined")

      def add2(x, y):
         self.log("add2 invoked: {}, {}".format(x, y))
         return x + y

      yield self.register(add2, 'com.mathservice.add2')
      self.log("add2 registered")

      self.finish()



class Case1_Frontend(CaseComponent):

   @inlineCallbacks
   def onJoin(self, details):

      self.log("joined")

      try:
         res = yield self.call('com.mathservice.add2', 2, 3)
      except Exception as e:
         self.log("call error: {}".format(e))
      else:
         self.log("call result: {}".format(res))

      self.finish()



class Case2_Backend(CaseComponent):

   @inlineCallbacks
   def onJoin(self, details):

      self.log("joined")

      def ping():
         self.log("ping() is invoked")
         return

      def add2(a, b):
         self.log("add2() is invoked", a, b)
         return a + b

      def stars(nick = "somebody", stars = 0):
         self.log("stars() is invoked", nick, stars)
         return u"{} starred {}x".format(nick, stars)

      def orders(product, limit = 5):
         self.log("orders() is invoked", product, limit)
         return [u"Product {}".format(i) for i in range(50)][:limit]

      def arglen(*args, **kwargs):
         self.log("arglen() is invoked", args, kwargs)
         return [len(args), len(kwargs)]

      yield self.register(ping, u'com.arguments.ping')
      yield self.register(add2, u'com.arguments.add2')
      yield self.register(stars, u'com.arguments.stars')
      yield self.register(orders, u'com.arguments.orders')
      yield self.register(arglen, u'com.arguments.arglen')

      self.log("procedures registered")

      yield sleep(3)

      self.log("finishing")

      self.finish()


class Case2_Frontend(CaseComponent):

   @inlineCallbacks
   def onJoin(self, details):

      self.log("joined")

      yield sleep(1)

      yield self.call(u'com.arguments.ping')
      self.log("Pinged!")

      res = yield self.call(u'com.arguments.add2', 2, 3)
      self.log("Add2: {}".format(res))

      starred = yield self.call(u'com.arguments.stars')
      self.log("Starred 1: {}".format(starred))

      starred = yield self.call(u'com.arguments.stars', nick = u'Homer')
      self.log("Starred 2: {}".format(starred))

      starred = yield self.call(u'com.arguments.stars', stars = 5)
      self.log("Starred 3: {}".format(starred))

      starred = yield self.call(u'com.arguments.stars', nick = u'Homer', stars = 5)
      self.log("Starred 4: {}".format(starred))

      orders = yield self.call(u'com.arguments.orders', u'coffee')
      self.log("Orders 1: {}".format(orders))

      orders = yield self.call(u'com.arguments.orders', u'coffee', limit = 10)
      self.log("Orders 2: {}".format(orders))

      arglengths = yield self.call(u'com.arguments.arglen')
      self.log("Arglen 1: {}".format(arglengths))

      arglengths = yield self.call(u'com.arguments.arglen', 1, 2, 3)
      self.log("Arglen 1: {}".format(arglengths))

      arglengths = yield self.call(u'com.arguments.arglen', a = 1, b = 2, c = 3)
      self.log("Arglen 2: {}".format(arglengths))

      arglengths = yield self.call(u'com.arguments.arglen', 1, 2, 3, a = 1, b = 2, c = 3)
      self.log("Arglen 3: {}".format(arglengths))

      self.log("finishing")

      self.finish()


if False:
   class TestRpc(unittest.TestCase):

      def setUp(self):
         self.debug = False
         self.realm = "realm1"
         self.transport = "websocket"
         self.url = "ws://127.0.0.1:8080"
         self.client = "tcp:127.0.0.1:8080"
         self.server = "tcp:8080"


      def test_minimal(self):

         embedded_components, client_components = [], [Case2_Backend, Case2_Frontend]

         ## create a WAMP router factory
         ##
         router_factory = RouterFactory()


         ## create a WAMP router session factory
         ##
         session_factory = RouterSessionFactory(router_factory)



         ## .. and create and add an WAMP application session to
         ## run next to the router
         ##
         config = types.ComponentConfig(realm = self.realm,
            extra = {
               'caselog': 'case1.log'
            }
         )
         try:
            log = io.open('caselog.log', 'w')
         except Exception as e:
            print(e)
            return
   #      log = io.open(config.extra['caselog'], 'w')
         config.log = log
         config.dlog = []
         config.components = []

         config.all_done = []

         for C in embedded_components:
            one_done = Deferred()
            config.all_done.append(one_done)
            c = C(config, one_done)
            config.components.append(c)
            session_factory.add(c)

         if self.transport == "websocket":

            ## create a WAMP-over-WebSocket transport server factory
            ##
            transport_factory = WampWebSocketServerFactory(session_factory, debug_wamp = self.debug)
            transport_factory.setProtocolOptions(failByDrop = False, openHandshakeTimeout = 0, closeHandshakeTimeout = 0)

         elif self.transport in ['rawsocket-json', 'rawsocket-msgpack']:

            ## create a WAMP-over-RawSocket transport server factory
            ##
            if self.transport == 'rawsocket-msgpack':
               serializer = MsgPackSerializer()
            elif self.transport == 'rawsocket-json':
               serializer = JsonSerializer()
            else:
               raise Exception("should not arrive here")

            transport_factory = WampRawSocketServerFactory(session_factory, serializer, debug = self.debug)

         else:
            raise Exception("should not arrive here")


         ## start the server from an endpoint
         ##
         from twisted.internet import reactor
         server = serverFromString(reactor, self.server)
         d = server.listen(transport_factory)

         def onlisten(port):
            config.port = port

         d.addCallback(onlisten)

         clients = []
         clients_d = []
         for C in client_components:
            ## create a WAMP application session factory
            ##
            session_factory = ApplicationSessionFactory(config)

            one_done = Deferred()
            config.all_done.append(one_done)

            def make_make(Klass, done):
               def make(config):
                  c = Klass(config, done)
                  config.components.append(c)
                  return c
               return make

            ## .. and set the session class on the factory
            ##
            session_factory.session = make_make(C, one_done)

            if self.transport == "websocket":

               serializers = [JsonSerializer()]

               ## create a WAMP-over-WebSocket transport client factory
               ##
               transport_factory = WampWebSocketClientFactory(session_factory, serializers = serializers, url = self.url, debug_wamp = self.debug)

               if True:
                  def maker(Klass):
                     class TestClientProtocol(WampWebSocketClientProtocol):
                        def onOpen(self):
                           self.txcnt = 0
                           self.rxcnt = 0
                           WampWebSocketClientProtocol.onOpen(self)

                        def sendMessage(self, bytes, isBinary):
                           self.txcnt += 1
                           print("> : {:>3} : {:<20} : {}".format(self.txcnt, Klass.__name__, bytes))
                           WampWebSocketClientProtocol.sendMessage(self, bytes, isBinary)

                        def onMessage(self, bytes, isBinary):
                           self.rxcnt += 1
                           print("< : {:>3} : {:<20} : {}".format(self.rxcnt, Klass.__name__, bytes))
                           WampWebSocketClientProtocol.onMessage(self, bytes, isBinary)
                     return TestClientProtocol

                  transport_factory.protocol = maker(C)
               else:
                  transport_factory.protocol = WampWebSocketClientProtocol

               transport_factory.setProtocolOptions(failByDrop = False, openHandshakeTimeout = 0, closeHandshakeTimeout = 0)

            elif self.transport in ['rawsocket-json', 'rawsocket-msgpack']:

               ## create a WAMP-over-RawSocket transport client factory
               ##
               if self.transport == 'rawsocket-msgpack':
                  serializer = MsgPackSerializer()
               elif self.transport == 'rawsocket-json':
                  serializer = JsonSerializer()
               else:
                  raise Exception("should not arrive here")

               transport_factory = WampRawSocketClientFactory(session_factory, serializer, debug = self.debug)


            ## start the client from an endpoint
            ##
            cl = clientFromString(reactor, self.client)
            clients_d.append(cl.connect(transport_factory))

            clients.append(cl)

         config.connected_clients = None

         def client_connected(res):
            config.connected_clients = [proto for success, proto in res if success]

         DeferredList(clients_d).addCallback(client_connected)


         d = DeferredList(config.all_done, consumeErrors = True)
         #d = config.components[1]._done

         def done(res):
            log.flush()
            log.close()
            if config.port:
               config.port.stopListening()
            if config.connected_clients:
               for proto in config.connected_clients:
                  proto.transport.abortConnection()
            print("Log length: {}".format(len(config.dlog)))
            print(config.dlog)
            #from twisted.internet import reactor
            #reactor.callLater(1, reactor.stop)

         def error(err):
            print(err)

         d.addCallbacks(done, error)

   #      d2 = Deferred()

         return d


if __name__ == '__main__':
   unittest.main()
