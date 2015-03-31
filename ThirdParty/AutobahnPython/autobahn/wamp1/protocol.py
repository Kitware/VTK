###############################################################################
##
##  Copyright (C) 2011-2013 Tavendo GmbH
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
PY3 = sys.version_info >= (3,)


__all__ = ("WampProtocol",
           "WampFactory",
           "WampServerProtocol",
           "WampServerFactory",
           "WampClientProtocol",
           "WampClientFactory",
           "WampCraProtocol",
           "WampCraClientProtocol",
           "WampCraServerProtocol",
           "json_lib",
           "json_loads",
           "json_dumps",)


import inspect, types
import traceback

if PY3:
   from io import StringIO
else:
   import StringIO

import hashlib, hmac, binascii, random

from twisted.python import log
from twisted.internet.defer import Deferred, \
                                   maybeDeferred

from autobahn import __version__

from autobahn.websocket.protocol import WebSocketProtocol

from autobahn.websocket import http
from autobahn.twisted.websocket import WebSocketClientProtocol, \
                                       WebSocketClientFactory, \
                                       WebSocketServerFactory, \
                                       WebSocketServerProtocol
from autobahn.wamp1.pbkdf2 import pbkdf2_bin
from autobahn.wamp1.prefixmap import PrefixMap
from autobahn.util import utcnow, newid, Tracker


def exportRpc(arg = None):
   """
   Decorator for RPC'ed callables.
   """
   ## decorator without argument
   if type(arg) is types.FunctionType:
      arg._autobahn_rpc_id = arg.__name__
      return arg
   ## decorator with argument
   else:
      def inner(f):
         f._autobahn_rpc_id = arg
         return f
      return inner

def exportSub(arg, prefixMatch = False):
   """
   Decorator for subscription handlers.
   """
   def inner(f):
      f._autobahn_sub_id = arg
      f._autobahn_sub_prefix_match = prefixMatch
      return f
   return inner

def exportPub(arg, prefixMatch = False):
   """
   Decorator for publication handlers.
   """
   def inner(f):
      f._autobahn_pub_id = arg
      f._autobahn_pub_prefix_match = prefixMatch
      return f
   return inner


class WampProtocol:
   """
   WAMP protocol base class. Mixin for WampServerProtocol and WampClientProtocol.
   """

   URI_WAMP_BASE = "http://api.wamp.ws/"
   """
   WAMP base URI for WAMP predefined things.
   """

   URI_WAMP_ERROR = URI_WAMP_BASE + "error#"
   """
   Prefix for WAMP errors.
   """

   URI_WAMP_PROCEDURE = URI_WAMP_BASE + "procedure#"
   """
   Prefix for WAMP predefined RPC endpoints.
   """

   URI_WAMP_TOPIC = URI_WAMP_BASE + "topic#"
   """
   Prefix for WAMP predefined PubSub topics.
   """

   URI_WAMP_ERROR_GENERIC = URI_WAMP_ERROR + "generic"
   """
   WAMP error URI for generic errors.
   """

   DESC_WAMP_ERROR_GENERIC = "generic error"
   """
   Description for WAMP generic errors.
   """

   URI_WAMP_ERROR_INTERNAL = URI_WAMP_ERROR + "internal"
   """
   WAMP error URI for internal errors.
   """

   DESC_WAMP_ERROR_INTERNAL = "internal error"
   """
   Description for WAMP internal errors.
   """

   URI_WAMP_ERROR_NO_SUCH_RPC_ENDPOINT = URI_WAMP_ERROR + "NoSuchRPCEndpoint"
   """
   WAMP error URI for RPC endpoint not found.
   """

   WAMP_PROTOCOL_VERSION         = 1
   """
   WAMP version this server speaks. Versions are numbered consecutively
   (integers, no gaps).
   """

   MESSAGE_TYPEID_WELCOME        = 0
   """
   Server-to-client welcome message containing session ID.
   """

   MESSAGE_TYPEID_PREFIX         = 1
   """
   Client-to-server message establishing a URI prefix to be used in CURIEs.
   """

   MESSAGE_TYPEID_CALL           = 2
   """
   Client-to-server message initiating an RPC.
   """

   MESSAGE_TYPEID_CALL_RESULT    = 3
   """
   Server-to-client message returning the result of a successful RPC.
   """

   MESSAGE_TYPEID_CALL_ERROR     = 4
   """
   Server-to-client message returning the error of a failed RPC.
   """

   MESSAGE_TYPEID_SUBSCRIBE      = 5
   """
   Client-to-server message subscribing to a topic.
   """

   MESSAGE_TYPEID_UNSUBSCRIBE    = 6
   """
   Client-to-server message unsubscribing from a topic.
   """

   MESSAGE_TYPEID_PUBLISH        = 7
   """
   Client-to-server message publishing an event to a topic.
   """

   MESSAGE_TYPEID_EVENT          = 8
   """
   Server-to-client message providing the event of a (subscribed) topic.
   """

   def connectionMade(self):
      self.debugWamp = self.factory.debugWamp
      self.debugApp = self.factory.debugApp
      self.prefixes = PrefixMap()
      self.calls = {}
      self.procs = {}


   def connectionLost(self, reason):
      pass


   def _protocolError(self, reason):
      if self.debugWamp:
         log.msg("Closing Wamp session on protocol violation : %s" % reason)

      ## FIXME: subprotocols are probably not supposed to close with CLOSE_STATUS_CODE_PROTOCOL_ERROR
      ##
      self.protocolViolation("Wamp RPC/PubSub protocol violation ('%s')" % reason)


   def shrink(self, uri, passthrough = False):
      """
      Shrink given URI to CURIE according to current prefix mapping.
      If no appropriate prefix mapping is available, return original URI.

      :param uri: URI to shrink.
      :type uri: str

      :returns str -- CURIE or original URI.
      """
      return self.prefixes.shrink(uri)


   def resolve(self, curieOrUri, passthrough = False):
      """
      Resolve given CURIE/URI according to current prefix mapping or return
      None if cannot be resolved.

      :param curieOrUri: CURIE or URI.
      :type curieOrUri: str

      :returns: str -- Full URI for CURIE or None.
      """
      return self.prefixes.resolve(curieOrUri)


   def resolveOrPass(self, curieOrUri):
      """
      Resolve given CURIE/URI according to current prefix mapping or return
      string verbatim if cannot be resolved.

      :param curieOrUri: CURIE or URI.
      :type curieOrUri: str

      :returns: str -- Full URI for CURIE or original string.
      """
      return self.prefixes.resolveOrPass(curieOrUri)


   def serializeMessage(self, msg):
      """
      Delegate message serialization to the factory.
      :param msg: The message to be serialized.
      :type msg: str
      :return: The serialized message.
      """
      return self.factory._serialize(msg)


   def registerForRpc(self, obj, baseUri = "", methods = None):
      """
      Register an service object for RPC. A service object has methods
      which are decorated using @exportRpc.

      :param obj: The object to be registered (in this WebSockets session) for RPC.
      :type obj: Object with methods decorated using @exportRpc.
      :param baseUri: Optional base URI which is prepended to method names for export.
      :type baseUri: String.
      :param methods: If not None, a list of unbound class methods corresponding to obj
                     which should be registered. This can be used to register only a subset
                     of the methods decorated with @exportRpc.
      :type methods: List of unbound class methods.
      """
      for k in inspect.getmembers(obj.__class__, inspect.ismethod):
         if k[1].__dict__.has_key("_autobahn_rpc_id"):
            if methods is None or k[1] in methods:
               uri = baseUri + k[1].__dict__["_autobahn_rpc_id"]
               proc = k[1]
               self.registerMethodForRpc(uri, obj, proc)


   def registerMethodForRpc(self, uri, obj, proc):
      """
      Register a method of an object for RPC.

      :param uri: URI to register RPC method under.
      :type uri: str
      :param obj: The object on which to register a method for RPC.
      :type obj: object
      :param proc: Unbound object method to register RPC for.
      :type proc: unbound method
      """
      self.procs[uri] = (obj, proc, False)
      if self.debugWamp:
         log.msg("registered remote method on %s" % uri)


   def registerProcedureForRpc(self, uri, proc):
      """
      Register a (free standing) function/procedure for RPC.

      :param uri: URI to register RPC function/procedure under.
      :type uri: str
      :param proc: Free-standing function/procedure.
      :type proc: callable
      """
      self.procs[uri] = (None, proc, False)
      if self.debugWamp:
         log.msg("registered remote procedure on %s" % uri)


   def registerHandlerMethodForRpc(self, uri, obj, handler, extra = None):
      """
      Register a handler on an object for RPC.

      :param uri: URI to register RPC method under.
      :type uri: str
      :param obj: The object on which to register the RPC handler
      :type obj: object
      :param proc: Unbound object method to register RPC for.
      :type proc: unbound method
      :param extra: Optional extra data that will be given to the handler at call time.
      :type extra: object
      """
      self.procs[uri] = (obj, handler, True, extra)
      if self.debugWamp:
         log.msg("registered remote handler method on %s" % uri)


   def registerHandlerProcedureForRpc(self, uri, handler, extra = None):
      """
      Register a (free standing) handler for RPC.

      :param uri: URI to register RPC handler under.
      :type uri: str
      :param proc: Free-standing handler
      :type proc: callable
      :param extra: Optional extra data that will be given to the handler at call time.
      :type extra: object
      """
      self.procs[uri] = (None, handler, True, extra)
      if self.debugWamp:
         log.msg("registered remote handler procedure on %s" % uri)


   def procForUri(self, uri):
      """
      Returns the procedure specification for `uri` or None, if it does not exist.

      :param uri: URI to be checked.
      :type uri: str
      :returns: The procedure specification for `uri`, if it exists,
                `None` otherwise.
      """
      return self.procs[uri] if uri in self.procs else None


   def onBeforeCall(self, callid, uri, args, isRegistered):
      """
      Callback fired before executing incoming RPC. This can be used for
      logging, statistics tracking or redirecting RPCs or argument mangling i.e.

      The default implementation just returns the incoming URI/args.

      :param uri: RPC endpoint URI (fully-qualified).
      :type uri: str
      :param args: RPC arguments array.
      :type args: list
      :param isRegistered: True, iff RPC endpoint URI is registered in this session.
      :type isRegistered: bool
      :returns pair -- Must return URI/Args pair.
      """
      return uri, args


   def onAfterCallSuccess(self, result, call):
      """
      Callback fired after executing incoming RPC with success, but before
      sending the RPC success message.

      The default implementation will just return `result` to the client.

      :param result: Result returned for executing the incoming RPC.
      :type result: Anything returned by the user code for the endpoint.
      :param call: WAMP call object for incoming RPC.
      :type call: instance of Call
      :returns obj -- Result send back to client.
      """
      return result


   def onAfterCallError(self, error, call):
      """
      Callback fired after executing incoming RPC with failure, but before
      sending the RPC error message.

      The default implementation will just return `error` to the client.

      :param error: Error that occurred during incomnig RPC call execution.
      :type error: Instance of twisted.python.failure.Failure
      :param call: WAMP call object for incoming RPC.
      :type call: instance of Call
      :returns twisted.python.failure.Failure -- Error sent back to client.
      """
      return error


   def onAfterSendCallSuccess(self, msg, call):
      """
      Callback fired after sending RPC success message.

      :param msg: Serialized WAMP message.
      :type msg: str
      :param call: WAMP call object for incoming RPC.
      :type call: instance of Call
      """
      pass


   def onAfterSendCallError(self, msg, call):
      """
      Callback fired after sending RPC error message.

      :param msg: Serialized WAMP message.
      :type msg: str
      :param call: WAMP call object for incoming RPC.
      :type call: instance of Call
      """
      pass


   def call(self, *args):
      """
      Perform a remote-procedure call (RPC). The first argument is the procedure
      URI (mandatory). Subsequent positional arguments can be provided (must be
      JSON serializable). The return value is a Twisted Deferred.
      """

      if len(args) < 1:
         raise Exception("missing procedure URI")

      if type(args[0]) not in [unicode, str]:
         raise Exception("invalid type for procedure URI")

      procuri = args[0]
      callid = None
      while True:
         callid = newid()
         if not self.calls.has_key(callid):
            break
      d = Deferred()
      self.calls[callid] = d
      msg = [WampProtocol.MESSAGE_TYPEID_CALL, callid, procuri]
      msg.extend(args[1:])

      try:
         o = self.factory._serialize(msg)
      except:
         raise Exception("call argument(s) not JSON serializable")

      self.sendMessage(o)
      return d



## use Ultrajson (https://github.com/esnme/ultrajson) if available
##
try:
   import ujson
   json_lib = ujson
   json_loads = ujson.loads
   json_dumps = lambda x: ujson.dumps(x, ensure_ascii = False)
except:
   import json
   json_lib = json
   json_loads = json.loads
   json_dumps = json.dumps



class WampFactory:
   """
   WAMP factory base class. Mixin for WampServerFactory and WampClientFactory.
   """

   def __init__(self):
      if self.debugWamp:
         log.msg("Using JSON processor '%s'" % json_lib.__name__)


   def _serialize(self, obj):
      """
      Default object serializer.
      """
      return json_dumps(obj)


   def _unserialize(self, bytes):
      """
      Default object deserializer.
      """
      return json_loads(bytes)



class WampServerProtocol(WebSocketServerProtocol, WampProtocol):
   """
   Server factory for Wamp RPC/PubSub.
   """

   SUBSCRIBE = 1
   PUBLISH = 2

   def onSessionOpen(self):
      """
      Callback fired when WAMP session was fully established.
      """
      pass


   def onOpen(self):
      """
      Default implementation for WAMP connection opened sends
      Welcome message containing session ID.
      """
      self.session_id = newid()

      ## include traceback as error detail for RPC errors with
      ## no error URI - that is errors returned with URI_WAMP_ERROR_GENERIC
      self.includeTraceback = False

      msg = [WampProtocol.MESSAGE_TYPEID_WELCOME,
             self.session_id,
             WampProtocol.WAMP_PROTOCOL_VERSION,
             "Autobahn/%s" % __version__]
      o = self.factory._serialize(msg)
      self.sendMessage(o)

      self.factory._addSession(self, self.session_id)
      self.onSessionOpen()


   def onConnect(self, connectionRequest):
      """
      Default implementation for WAMP connection acceptance:
      check if client announced WAMP subprotocol, and only accept connection
      if client did so.
      """
      for p in connectionRequest.protocols:
         if p in self.factory.protocols:
            return (p, {}) # return (protocol, headers)
      raise http.HttpException(http.BAD_REQUEST[0], "this server only speaks WAMP")


   def connectionMade(self):
      WebSocketServerProtocol.connectionMade(self)
      WampProtocol.connectionMade(self)

      ## RPCs registered in this session (a URI map of (object, procedure)
      ## pairs for object methods or (None, procedure) for free standing procedures)
      self.procs = {}

      ## Publication handlers registered in this session (a URI map of (object, pubHandler) pairs
      ## pairs for object methods (handlers) or (None, None) for topic without handler)
      self.pubHandlers = {}

      ## Subscription handlers registered in this session (a URI map of (object, subHandler) pairs
      ## pairs for object methods (handlers) or (None, None) for topic without handler)
      self.subHandlers = {}

      self.handlerMapping = {
         self.MESSAGE_TYPEID_CALL: CallHandler(self, self.prefixes),
         self.MESSAGE_TYPEID_CALL_RESULT: CallResultHandler(self, self.prefixes),
         self.MESSAGE_TYPEID_CALL_ERROR: CallErrorHandler(self, self.prefixes)}


   def connectionLost(self, reason):
      self.factory._unsubscribeClient(self)
      self.factory._removeSession(self)

      WampProtocol.connectionLost(self, reason)
      WebSocketServerProtocol.connectionLost(self, reason)


   def sendMessage(self,
                   payload,
                   binary = False,
                   payload_frag_size = None,
                   sync = False,
                   doNotCompress = False):
      if self.debugWamp:
         log.msg("TX WAMP: %s" % str(payload))
      WebSocketServerProtocol.sendMessage(self,
                                          payload,
                                          binary,
                                          payload_frag_size,
                                          sync,
                                          doNotCompress)


   def _getPubHandler(self, topicUri):
      ## Longest matching prefix based resolution of (full) topic URI to
      ## publication handler.
      ## Returns a 5-tuple (consumedUriPart, unconsumedUriPart, handlerObj, handlerProc, prefixMatch)
      ##
      for i in xrange(len(topicUri), -1, -1):
         tt = topicUri[:i]
         if self.pubHandlers.has_key(tt):
            h = self.pubHandlers[tt]
            return (tt, topicUri[i:], h[0], h[1], h[2])
      return None


   def _getSubHandler(self, topicUri):
      ## Longest matching prefix based resolution of (full) topic URI to
      ## subscription handler.
      ## Returns a 5-tuple (consumedUriPart, unconsumedUriPart, handlerObj, handlerProc, prefixMatch)
      ##
      for i in xrange(len(topicUri), -1, -1):
         tt = topicUri[:i]
         if self.subHandlers.has_key(tt):
            h = self.subHandlers[tt]
            return (tt, topicUri[i:], h[0], h[1], h[2])
      return None


   def registerForPubSub(self, topicUri, prefixMatch = False, pubsub = PUBLISH | SUBSCRIBE):
      """
      Register a topic URI as publish/subscribe channel in this session.

      :param topicUri: Topic URI to be established as publish/subscribe channel.
      :type topicUri: str
      :param prefixMatch: Allow to match this topic URI by prefix.
      :type prefixMatch: bool
      :param pubsub: Allow publication and/or subscription.
      :type pubsub: WampServerProtocol.PUB, WampServerProtocol.SUB, WampServerProtocol.PUB | WampServerProtocol.SUB
      """
      if pubsub & WampServerProtocol.PUBLISH:
         self.pubHandlers[topicUri] = (None, None, prefixMatch)
         if self.debugWamp:
            log.msg("registered topic %s for publication (match by prefix = %s)" % (topicUri, prefixMatch))
      if pubsub & WampServerProtocol.SUBSCRIBE:
         self.subHandlers[topicUri] = (None, None, prefixMatch)
         if self.debugWamp:
            log.msg("registered topic %s for subscription (match by prefix = %s)" % (topicUri, prefixMatch))


   def registerHandlerForPubSub(self, obj, baseUri = ""):
      """
      Register a handler object for PubSub. A handler object has methods
      which are decorated using @exportPub and @exportSub.

      :param obj: The object to be registered (in this WebSockets session) for PubSub.
      :type obj: Object with methods decorated using @exportPub and @exportSub.
      :param baseUri: Optional base URI which is prepended to topic names for export.
      :type baseUri: String.
      """
      for k in inspect.getmembers(obj.__class__, inspect.ismethod):
         if k[1].__dict__.has_key("_autobahn_pub_id"):
            uri = baseUri + k[1].__dict__["_autobahn_pub_id"]
            prefixMatch = k[1].__dict__["_autobahn_pub_prefix_match"]
            proc = k[1]
            self.registerHandlerForPub(uri, obj, proc, prefixMatch)
         elif k[1].__dict__.has_key("_autobahn_sub_id"):
            uri = baseUri + k[1].__dict__["_autobahn_sub_id"]
            prefixMatch = k[1].__dict__["_autobahn_sub_prefix_match"]
            proc = k[1]
            self.registerHandlerForSub(uri, obj, proc, prefixMatch)


   def registerHandlerForSub(self, uri, obj, proc, prefixMatch = False):
      """
      Register a method of an object as subscription handler.

      :param uri: Topic URI to register subscription handler for.
      :type uri: str
      :param obj: The object on which to register a method as subscription handler.
      :type obj: object
      :param proc: Unbound object method to register as subscription handler.
      :type proc: unbound method
      :param prefixMatch: Allow to match this topic URI by prefix.
      :type prefixMatch: bool
      """
      self.subHandlers[uri] = (obj, proc, prefixMatch)
      if not self.pubHandlers.has_key(uri):
         self.pubHandlers[uri] = (None, None, False)
      if self.debugWamp:
         log.msg("registered subscription handler for topic %s" % uri)


   def registerHandlerForPub(self, uri, obj, proc, prefixMatch = False):
      """
      Register a method of an object as publication handler.

      :param uri: Topic URI to register publication handler for.
      :type uri: str
      :param obj: The object on which to register a method as publication handler.
      :type obj: object
      :param proc: Unbound object method to register as publication handler.
      :type proc: unbound method
      :param prefixMatch: Allow to match this topic URI by prefix.
      :type prefixMatch: bool
      """
      self.pubHandlers[uri] = (obj, proc, prefixMatch)
      if not self.subHandlers.has_key(uri):
         self.subHandlers[uri] = (None, None, False)
      if self.debugWamp:
         log.msg("registered publication handler for topic %s" % uri)


   # noinspection PyDefaultArgument
   def dispatch(self, topicUri, event, exclude = [], eligible = None):
      """
      Dispatch an event for a topic to all clients subscribed to
      and authorized for that topic.

      Optionally, exclude list of clients and/or only consider clients
      from explicit eligibles. In other words, the event is delivered
      to the set

         (subscribers - excluded) & eligible

      :param topicUri: URI of topic to publish event to.
      :type topicUri: str
      :param event: Event to dispatch.
      :type event: obj
      :param exclude: Optional list of clients (WampServerProtocol instances) to exclude.
      :type exclude: list of obj
      :param eligible: Optional list of clients (WampServerProtocol instances) eligible at all (or None for all).
      :type eligible: list of obj

      :returns twisted.internet.defer.Deferred -- Will be fired when event was
      dispatched to all subscribers. The return value provided to the deferred
      is a pair (delivered, requested), where delivered = number of actual
      receivers, and requested = number of (subscribers - excluded) & eligible.
      """
      return self.factory.dispatch(topicUri, event, exclude, eligible)


   def onMessage(self, msg, binary):
      """
      Handle WAMP messages received from WAMP client.
      """

      if self.debugWamp:
         log.msg("RX WAMP: %s" % str(msg))

      if not binary:
         try:
            obj = self.factory._unserialize(msg)
            if type(obj) == list:

               msgtype = obj[0]

               ### XXX Replace check by try...except when all handlers
               ### XXX are in place. Exception handling should create
               ### XXX a protocolError message about unsupported
               ### XXX message type
               if msgtype in [WampProtocol.MESSAGE_TYPEID_CALL,
                              WampProtocol.MESSAGE_TYPEID_CALL_RESULT,
                              WampProtocol.MESSAGE_TYPEID_CALL_ERROR]:
                  self.handlerMapping[msgtype].handleMessage(obj)

               ### XXX Move remaining code to appropriate handlers

               ## Subscribe Message
               ##
               elif msgtype == WampProtocol.MESSAGE_TYPEID_SUBSCRIBE:
                  topicUri = self.prefixes.resolveOrPass(obj[1]) ### PFX - remove
                  h = self._getSubHandler(topicUri)
                  if h:
                     ## either exact match or prefix match allowed
                     if h[1] == "" or h[4]:

                        ## direct topic
                        if h[2] is None and h[3] is None:
                           self.factory._subscribeClient(self, topicUri)

                        ## topic handled by subscription handler
                        else:
                           ## handler is object method
                           if h[2]:
                              a = maybeDeferred(h[3], h[2], str(h[0]), str(h[1]))

                           ## handler is free standing procedure
                           else:
                              a = maybeDeferred(h[3], str(h[0]), str(h[1]))

                           def fail(failure):
                              if self.debugWamp:
                                 log.msg("exception during custom subscription handler: %s" % failure)

                           def done(result):
                              ## only subscribe client if handler did return True
                              if result:
                                 self.factory._subscribeClient(self, topicUri)

                           a.addCallback(done).addErrback(fail)
                     else:
                        if self.debugWamp:
                           log.msg("topic %s matches only by prefix and prefix match disallowed" % topicUri)
                  else:
                     if self.debugWamp:
                        log.msg("no topic / subscription handler registered for %s" % topicUri)

               ## Unsubscribe Message
               ##
               elif msgtype == WampProtocol.MESSAGE_TYPEID_UNSUBSCRIBE:
                  topicUri = self.prefixes.resolveOrPass(obj[1]) ### PFX - remove
                  self.factory._unsubscribeClient(self, topicUri)

               ## Publish Message
               ##
               elif msgtype == WampProtocol.MESSAGE_TYPEID_PUBLISH:
                  topicUri = self.prefixes.resolveOrPass(obj[1]) ### PFX - remove
                  h = self._getPubHandler(topicUri)
                  if h:
                     ## either exact match or prefix match allowed
                     if h[1] == "" or h[4]:

                        ## Event
                        ##
                        event = obj[2]

                        ## Exclude Sessions List
                        ##
                        exclude = [self] # exclude publisher by default
                        if len(obj) >= 4:
                           if type(obj[3]) == bool:
                              if not obj[3]:
                                 exclude = []
                           elif type(obj[3]) == list:
                              ## map session IDs to protos
                              exclude = self.factory.sessionIdsToProtos(obj[3])
                           else:
                              ## FIXME: invalid type
                              pass

                        ## Eligible Sessions List
                        ##
                        eligible = None # all sessions are eligible by default
                        if len(obj) >= 5:
                           if type(obj[4]) == list:
                              ## map session IDs to protos
                              eligible = self.factory.sessionIdsToProtos(obj[4])
                           else:
                              ## FIXME: invalid type
                              pass

                        ## direct topic
                        if h[2] is None and h[3] is None:
                           self.factory.dispatch(topicUri, event, exclude, eligible)

                        ## topic handled by publication handler
                        else:
                           ## handler is object method
                           if h[2]:
                              e = maybeDeferred(h[3], h[2], str(h[0]), str(h[1]), event)

                           ## handler is free standing procedure
                           else:
                              e = maybeDeferred(h[3], str(h[0]), str(h[1]), event)

                           def fail(failure):
                              if self.debugWamp:
                                 log.msg("exception during custom publication handler: %s" % failure)

                           def done(result):
                              ## only dispatch event if handler did return event
                              if result:
                                 self.factory.dispatch(topicUri, result, exclude, eligible)

                           e.addCallback(done).addErrback(fail)
                     else:
                        if self.debugWamp:
                           log.msg("topic %s matches only by prefix and prefix match disallowed" % topicUri)
                  else:
                     if self.debugWamp:
                        log.msg("no topic / publication handler registered for %s" % topicUri)

               ## Define prefix to be used in CURIEs
               ##
               elif msgtype == WampProtocol.MESSAGE_TYPEID_PREFIX:
                  prefix = obj[1]
                  uri = obj[2]
                  self.prefixes.set(prefix, uri) ### PFX - remove whole block (this msg type won't survive)

               else:
                  log.msg("unknown message type")
            else:
               log.msg("msg not a list")
         except Exception:
            traceback.print_exc()
      else:
         log.msg("binary message")



class WampServerFactory(WebSocketServerFactory, WampFactory):
   """
   Server factory for Wamp RPC/PubSub.
   """

   protocol = WampServerProtocol
   """
   Twisted protocol used by default for WAMP servers.
   """

   def __init__(self,
                url,
                debug = False,
                debugCodePaths = False,
                debugWamp = False,
                debugApp = False,
                externalPort = None,
                reactor = None):
      self.debugWamp = debugWamp
      self.debugApp = debugApp
      WebSocketServerFactory.__init__(self,
                                      url,
                                      protocols = ["wamp"],
                                      debug = debug,
                                      debugCodePaths = debugCodePaths,
                                      externalPort = externalPort,
                                      reactor = reactor)
      WampFactory.__init__(self)


   def onClientSubscribed(self, proto, topicUri):
      """
      Callback fired when peer was (successfully) subscribed on some topic.

      :param proto: Peer protocol instance subscribed.
      :type proto: Instance of WampServerProtocol.
      :param topicUri: Fully qualified, resolved URI of topic subscribed.
      :type topicUri: str
      """
      pass


   def _subscribeClient(self, proto, topicUri):
      """
      Called from proto to subscribe client for topic.
      """
      if not self.subscriptions.has_key(topicUri):
         self.subscriptions[topicUri] = set()
         if self.debugWamp:
            log.msg("subscriptions map created for topic %s" % topicUri)
      if not proto in self.subscriptions[topicUri]:
         self.subscriptions[topicUri].add(proto)
         if self.debugWamp:
            log.msg("subscribed peer %s on topic %s" % (proto.peer, topicUri))
         self.onClientSubscribed(proto, topicUri)
      else:
         if self.debugWamp:
            log.msg("peer %s already subscribed on topic %s" % (proto.peer, topicUri))


   def onClientUnsubscribed(self, proto, topicUri):
      """
      Callback fired when peer was (successfully) unsubscribed from some topic.

      :param proto: Peer protocol instance unsubscribed.
      :type proto: Instance of WampServerProtocol.
      :param topicUri: Fully qualified, resolved URI of topic unsubscribed.
      :type topicUri: str
      """
      pass


   def _unsubscribeClient(self, proto, topicUri = None):
      """
      Called from proto to unsubscribe client from topic.
      """
      if topicUri:
         if self.subscriptions.has_key(topicUri) and proto in self.subscriptions[topicUri]:
            self.subscriptions[topicUri].discard(proto)
            if self.debugWamp:
               log.msg("unsubscribed peer %s from topic %s" % (proto.peer, topicUri))
            if len(self.subscriptions[topicUri]) == 0:
               del self.subscriptions[topicUri]
               if self.debugWamp:
                  log.msg("topic %s removed from subscriptions map - no one subscribed anymore" % topicUri)
            self.onClientUnsubscribed(proto, topicUri)
         else:
            if self.debugWamp:
               log.msg("peer %s not subscribed on topic %s" % (proto.peer, topicUri))
      else:
         for topicUri, subscribers in self.subscriptions.items():
            if proto in subscribers:
               subscribers.discard(proto)
               if self.debugWamp:
                  log.msg("unsubscribed peer %s from topic %s" % (proto.peer, topicUri))
               if len(subscribers) == 0:
                  del self.subscriptions[topicUri]
                  if self.debugWamp:
                     log.msg("topic %s removed from subscriptions map - no one subscribed anymore" % topicUri)
               self.onClientUnsubscribed(proto, topicUri)
         if self.debugWamp:
            log.msg("unsubscribed peer %s from all topics" % (proto.peer))


   # noinspection PyDefaultArgument
   def dispatch(self, topicUri, event, exclude = [], eligible = None):
      """
      Dispatch an event to all peers subscribed to the event topic.

      :param topicUri: Topic to publish event to.
      :type topicUri: str
      :param event: Event to publish (must be JSON serializable).
      :type event: obj
      :param exclude: List of WampServerProtocol instances to exclude from receivers.
      :type exclude: List of obj
      :param eligible: List of WampServerProtocol instances eligible as receivers (or None for all).
      :type eligible: List of obj

      :returns twisted.internet.defer.Deferred -- Will be fired when event was
      dispatched to all subscribers. The return value provided to the deferred
      is a pair (delivered, requested), where delivered = number of actual
      receivers, and requested = number of (subscribers - excluded) & eligible.
      """
      if self.debugWamp:
         log.msg("publish event %s for topicUri %s" % (str(event), topicUri))

      d = Deferred()

      if self.subscriptions.has_key(topicUri) and len(self.subscriptions[topicUri]) > 0:

         ## FIXME: this might break ordering of event delivery from a
         ## receiver perspective. We might need to have send queues
         ## per receiver OR do recvs = deque(sorted(..))

         ## However, see http://twistedmatrix.com/trac/ticket/1396

         if eligible is not None:
            subscrbs = set(eligible) & self.subscriptions[topicUri]
         else:
            subscrbs = self.subscriptions[topicUri]

         if len(exclude) > 0:
            recvs = subscrbs - set(exclude)
         else:
            recvs = subscrbs

         l = len(recvs)
         if l > 0:

            ## ok, at least 1 subscriber not excluded and eligible
            ## => prepare message for mass sending
            ##
            o = [WampProtocol.MESSAGE_TYPEID_EVENT, topicUri, event]
            try:
               msg = self._serialize(o)
               if self.debugWamp:
                  log.msg("serialized event msg: " + str(msg))
            except Exception as e:
               raise Exception("invalid type for event - serialization failed [%s]" % e)

            preparedMsg = self.prepareMessage(msg)

            ## chunked sending of prepared message
            ##
            self._sendEvents(preparedMsg, recvs.copy(), 0, l, d)

         else:
            ## receivers list empty after considering exlude and eligible sessions
            ##
            d.callback((0, 0))
      else:
         ## no one subscribed on topic
         ##
         d.callback((0, 0))

      return d


   def _sendEvents(self, preparedMsg, recvs, delivered, requested, d):
      """
      Delivers events to receivers in chunks and reenters the reactor
      in-between, so that other stuff can run.
      """
      ## deliver a batch of events
      done = False
      for i in xrange(0, 256):
         try:
            proto = recvs.pop()
            if proto.state == WebSocketProtocol.STATE_OPEN:
               try:
                  proto.sendPreparedMessage(preparedMsg)
               except:
                  pass
               else:
                  if self.debugWamp:
                     log.msg("delivered event to peer %s" % proto.peer)
                  delivered += 1
         except KeyError:
            # all receivers done
            done = True
            break

      if not done:
         ## if there are receivers left, redo
         self.reactor.callLater(0, self._sendEvents, preparedMsg, recvs, delivered, requested, d)
      else:
         ## else fire final result
         d.callback((delivered, requested))


   def _addSession(self, proto, session_id):
      """
      Add proto for session ID.
      """
      if not self.protoToSessions.has_key(proto):
         self.protoToSessions[proto] = session_id
      else:
         raise Exception("logic error - dublicate _addSession for protoToSessions")
      if not self.sessionsToProto.has_key(session_id):
         self.sessionsToProto[session_id] = proto
      else:
         raise Exception("logic error - dublicate _addSession for sessionsToProto")


   def _removeSession(self, proto):
      """
      Remove session by proto.
      """
      if self.protoToSessions.has_key(proto):
         session_id = self.protoToSessions[proto]
         del self.protoToSessions[proto]
         if self.sessionsToProto.has_key(session_id):
            del self.sessionsToProto[session_id]


   def sessionIdToProto(self, sessionId):
      """
      Map WAMP session ID to connected protocol instance (object of type WampServerProtocol).

      :param sessionId: WAMP session ID to be mapped.
      :type sessionId: str

      :returns obj -- WampServerProtocol instance or None.
      """
      return self.sessionsToProto.get(sessionId, None)


   def sessionIdsToProtos(self, sessionIds):
      """
      Map WAMP session IDs to connected protocol instances (objects of type WampServerProtocol).

      :param sessionIds: List of session IDs to be mapped.
      :type sessionIds: list of str

      :returns list -- List of WampServerProtocol instances corresponding to the WAMP session IDs.
      """
      protos = []
      for s in sessionIds:
         if self.sessionsToProto.has_key(s):
            protos.append(self.sessionsToProto[s])
      return protos


   def protoToSessionId(self, proto):
      """
      Map connected protocol instance (object of type WampServerProtocol) to WAMP session ID.

      :param proto: Instance of WampServerProtocol to be mapped.
      :type proto: obj of WampServerProtocol

      :returns str -- WAMP session ID or None.
      """
      return self.protoToSessions.get(proto, None)


   def protosToSessionIds(self, protos):
      """
      Map connected protocol instances (objects of type WampServerProtocol) to WAMP session IDs.

      :param protos: List of instances of WampServerProtocol to be mapped.
      :type protos: list of WampServerProtocol

      :returns list -- List of WAMP session IDs corresponding to the protos.
      """
      sessionIds = []
      for p in protos:
         if self.protoToSessions.has_key(p):
            sessionIds.append(self.protoToSessions[p])
      return sessionIds


   def startFactory(self):
      """
      Called by Twisted when the factory starts up. When overriding, make
      sure to call the base method.
      """
      if self.debugWamp:
         log.msg("WampServerFactory starting")
      self.subscriptions = {}
      self.protoToSessions = {}
      self.sessionsToProto = {}


   def stopFactory(self):
      """
      Called by Twisted when the factory shuts down. When overriding, make
      sure to call the base method.
      """
      if self.debugWamp:
         log.msg("WampServerFactory stopped")



class WampClientProtocol(WebSocketClientProtocol, WampProtocol):
   """
   Twisted client protocol for WAMP.
   """

   def onSessionOpen(self):
      """
      Callback fired when WAMP session was fully established. Override
      in derived class.
      """
      pass


   def onOpen(self):
      ## do nothing here .. onSessionOpen is only fired when welcome
      ## message was received (and thus session ID set)
      pass


   def onConnect(self, connectionResponse):
      if connectionResponse.protocol not in self.factory.protocols:
         raise Exception("server does not speak WAMP")


   def connectionMade(self):
      WebSocketClientProtocol.connectionMade(self)
      WampProtocol.connectionMade(self)

      self.subscriptions = {}

      self.handlerMapping = {
         self.MESSAGE_TYPEID_CALL: CallHandler(self, self.prefixes),
         self.MESSAGE_TYPEID_CALL_RESULT: CallResultHandler(self, self.prefixes),
         self.MESSAGE_TYPEID_CALL_ERROR: CallErrorHandler(self, self.prefixes)}


   def connectionLost(self, reason):
      WampProtocol.connectionLost(self, reason)
      WebSocketClientProtocol.connectionLost(self, reason)


   def sendMessage(self, payload):
      if self.debugWamp:
         log.msg("TX WAMP: %s" % str(payload))
      WebSocketClientProtocol.sendMessage(self, payload)


   def onMessage(self, msg, binary):
      """Internal method to handle WAMP messages received from WAMP server."""

      ## WAMP is text message only
      ##
      if binary:
         self._protocolError("binary WebSocket message received")
         return

      if self.debugWamp:
         log.msg("RX WAMP: %s" % str(msg))

      ## WAMP is proper JSON payload
      ##
      try:
         obj = self.factory._unserialize(msg)
      except Exception as e:
         self._protocolError("WAMP message payload could not be unserialized [%s]" % e)
         return

      ## Every WAMP message is a list
      ##
      if type(obj) != list:
         self._protocolError("WAMP message payload not a list")
         return

      ## Every WAMP message starts with an integer for message type
      ##
      if len(obj) < 1:
         self._protocolError("WAMP message without message type")
         return
      if type(obj[0]) != int:
         self._protocolError("WAMP message type not an integer")
         return

      ## WAMP message type
      ##
      msgtype = obj[0]

      ## Valid WAMP message types received by WAMP clients
      ##
      if msgtype not in [WampProtocol.MESSAGE_TYPEID_WELCOME,
                         WampProtocol.MESSAGE_TYPEID_CALL,
                         WampProtocol.MESSAGE_TYPEID_CALL_RESULT,
                         WampProtocol.MESSAGE_TYPEID_CALL_ERROR,
                         WampProtocol.MESSAGE_TYPEID_EVENT]:
         self._protocolError("invalid WAMP message type %d" % msgtype)
         return

      if msgtype in [WampProtocol.MESSAGE_TYPEID_CALL,
                     WampProtocol.MESSAGE_TYPEID_CALL_RESULT,
                     WampProtocol.MESSAGE_TYPEID_CALL_ERROR]:
         self.handlerMapping[msgtype].handleMessage(obj)

      ## WAMP EVENT
      ##
      elif msgtype == WampProtocol.MESSAGE_TYPEID_EVENT:
         ## Topic
         ##
         if len(obj) != 3:
            self._protocolError("WAMP EVENT message invalid length %d" % len(obj))
            return
         if type(obj[1]) not in [unicode, str]:
            self._protocolError("invalid type for <topic> in WAMP EVENT message")
            return
         unresolvedTopicUri = str(obj[1])
         topicUri = self.prefixes.resolveOrPass(unresolvedTopicUri) ### PFX - remove

         ## Fire PubSub Handler
         ##
         if self.subscriptions.has_key(topicUri):
            event = obj[2]
            # noinspection PyCallingNonCallable
            self.subscriptions[topicUri](topicUri, event)
         else:
            ## event received for non-subscribed topic (could be because we
            ## just unsubscribed, and server already sent out event for
            ## previous subscription)
            pass

      ## WAMP WELCOME
      ##
      elif msgtype == WampProtocol.MESSAGE_TYPEID_WELCOME:
         ## Session ID
         ##
         if len(obj) < 2:
            self._protocolError("WAMP WELCOME message invalid length %d" % len(obj))
            return
         if type(obj[1]) not in [unicode, str]:
            self._protocolError("invalid type for <sessionid> in WAMP WELCOME message")
            return
         self.session_id = str(obj[1])

         ## WAMP Protocol Version
         ##
         if len(obj) > 2:
            if type(obj[2]) not in [int]:
               self._protocolError("invalid type for <version> in WAMP WELCOME message")
               return
            else:
               self.session_protocol_version = obj[2]
         else:
            self.session_protocol_version = None

         ## Server Ident
         ##
         if len(obj) > 3:
            if type(obj[3]) not in [unicode, str]:
               self._protocolError("invalid type for <server> in WAMP WELCOME message")
               return
            else:
               self.session_server = obj[3]
         else:
            self.session_server = None

         self.onSessionOpen()

      else:
         raise Exception("logic error")


   def prefix(self, prefix, uri):
      """
      Establishes a prefix to be used in `CURIEs <http://en.wikipedia.org/wiki/CURIE>`_
      instead of URIs having that prefix for both client-to-server and
      server-to-client messages.

      :param prefix: Prefix to be used in CURIEs.
      :type prefix: str
      :param uri: URI that this prefix will resolve to.
      :type uri: str
      """

      if type(prefix) != str:
         raise Exception("invalid type for prefix")

      if type(uri) not in [unicode, str]:
         raise Exception("invalid type for URI")

      if self.prefixes.get(prefix):  ### PFX - keep
         raise Exception("prefix already defined")

      self.prefixes.set(prefix, uri) ### PFX - keep

      msg = [WampProtocol.MESSAGE_TYPEID_PREFIX, prefix, uri]

      self.sendMessage(self.factory._serialize(msg))


   def publish(self, topicUri, event, excludeMe = None, exclude = None, eligible = None):
      """
      Publish an event under a topic URI. The latter may be abbreviated using a
      CURIE which has been previously defined using prefix(). The event must
      be JSON serializable.

      :param topicUri: The topic URI or CURIE.
      :type topicUri: str
      :param event: Event to be published (must be JSON serializable) or None.
      :type event: value
      :param excludeMe: When True, don't deliver the published event to myself (when I'm subscribed).
      :type excludeMe: bool
      :param exclude: Optional list of session IDs to exclude from receivers.
      :type exclude: list of str
      :param eligible: Optional list of session IDs to that are eligible as receivers.
      :type eligible: list of str
      """

      if type(topicUri) not in [unicode, str]:
         raise Exception("invalid type for parameter 'topicUri' - must be string (was %s)" % type(topicUri))

      if excludeMe is not None:
         if type(excludeMe) != bool:
            raise Exception("invalid type for parameter 'excludeMe' - must be bool (was %s)" % type(excludeMe))

      if exclude is not None:
         if type(exclude) != list:
            raise Exception("invalid type for parameter 'exclude' - must be list (was %s)" % type(exclude))

      if eligible is not None:
         if type(eligible) != list:
            raise Exception("invalid type for parameter 'eligible' - must be list (was %s)" % type(eligible))

      if exclude is not None or eligible is not None:
         if exclude is None:
            if excludeMe is not None:
               if excludeMe:
                  exclude = [self.session_id]
               else:
                  exclude = []
            else:
               exclude = [self.session_id]
         if eligible is not None:
            msg = [WampProtocol.MESSAGE_TYPEID_PUBLISH, topicUri, event, exclude, eligible]
         else:
            msg = [WampProtocol.MESSAGE_TYPEID_PUBLISH, topicUri, event, exclude]
      else:
         if excludeMe:
            msg = [WampProtocol.MESSAGE_TYPEID_PUBLISH, topicUri, event]
         else:
            msg = [WampProtocol.MESSAGE_TYPEID_PUBLISH, topicUri, event, excludeMe]

      try:
         o = self.factory._serialize(msg)
      except:
         raise Exception("invalid type for parameter 'event' - not JSON serializable")

      self.sendMessage(o)


   def subscribe(self, topicUri, handler):
      """
      Subscribe to topic. When already subscribed, will overwrite the handler.

      :param topicUri: URI or CURIE of topic to subscribe to.
      :type topicUri: str
      :param handler: Event handler to be invoked upon receiving events for topic.
      :type handler: Python callable, will be called as in <callable>(eventUri, event).
      """
      if type(topicUri) not in [unicode, str]:
         raise Exception("invalid type for parameter 'topicUri' - must be string (was %s)" % type(topicUri))

      if not hasattr(handler, '__call__'):
         raise Exception("invalid type for parameter 'handler' - must be a callable (was %s)" % type(handler))

      turi = self.prefixes.resolveOrPass(topicUri) ### PFX - keep
      if not self.subscriptions.has_key(turi):
         msg = [WampProtocol.MESSAGE_TYPEID_SUBSCRIBE, topicUri]
         o = self.factory._serialize(msg)
         self.sendMessage(o)
      self.subscriptions[turi] = handler


   def unsubscribe(self, topicUri):
      """
      Unsubscribe from topic. Will do nothing when currently not subscribed to the topic.

      :param topicUri: URI or CURIE of topic to unsubscribe from.
      :type topicUri: str
      """
      if type(topicUri) not in [unicode, str]:
         raise Exception("invalid type for parameter 'topicUri' - must be string (was %s)" % type(topicUri))

      turi = self.prefixes.resolveOrPass(topicUri) ### PFX - keep
      if self.subscriptions.has_key(turi):
         msg = [WampProtocol.MESSAGE_TYPEID_UNSUBSCRIBE, topicUri]
         o = self.factory._serialize(msg)
         self.sendMessage(o)
         del self.subscriptions[turi]



class WampClientFactory(WebSocketClientFactory, WampFactory):
   """
   Twisted client factory for WAMP.
   """

   protocol = WampClientProtocol

   def __init__(self,
                url,
                debug = False,
                debugCodePaths = False,
                debugWamp = False,
                debugApp = False,
                reactor = None):
      self.debugWamp = debugWamp
      self.debugApp = debugApp
      WebSocketClientFactory.__init__(self,
                                      url,
                                      protocols = ["wamp"],
                                      debug = debug,
                                      debugCodePaths = debugCodePaths,
                                      reactor = reactor)
      WampFactory.__init__(self)


   def startFactory(self):
      """
      Called by Twisted when the factory starts up. When overriding, make
      sure to call the base method.
      """
      if self.debugWamp:
         log.msg("WebSocketClientFactory starting")


   def stopFactory(self):
      """
      Called by Twisted when the factory shuts down. When overriding, make
      sure to call the base method.
      """
      if self.debugWamp:
         log.msg("WebSocketClientFactory stopped")



class WampCraProtocol(WampProtocol):
   """
   Base class for WAMP Challenge-Response Authentication protocols (client and server).

   WAMP-CRA is a cryptographically strong challenge response authentication
   protocol based on HMAC-SHA256.

   The protocol performs in-band authentication of WAMP clients to WAMP servers.

   WAMP-CRA does not introduce any new WAMP protocol level message types, but
   implements the authentication handshake via standard WAMP RPCs with well-known
   procedure URIs and signatures.
   """

   def deriveKey(secret, extra = None):
      """
      Computes a derived cryptographic key from a password according to PBKDF2
      http://en.wikipedia.org/wiki/PBKDF2.

      The function will only return a derived key if at least 'salt' is
      present in the 'extra' dictionary. The complete set of attributes
      that can be set in 'extra':

         salt: The salt value to be used.
         iterations: Number of iterations of derivation algorithm to run.
         keylen: Key length to derive.

      :returns str -- The derived key or the original secret.
      """
      if type(extra) == dict and extra.has_key('salt'):
         salt = str(extra['salt'])
         iterations = int(extra.get('iterations', 10000))
         keylen = int(extra.get('keylen', 32))
         b = pbkdf2_bin(secret, salt, iterations, keylen, hashlib.sha256)
         return binascii.b2a_base64(b).strip()
      else:
         return secret

   deriveKey = staticmethod(deriveKey)


   def authSignature(self, authChallenge, authSecret = None, authExtra = None):
      """
      Compute the authentication signature from an authentication challenge and a secret.

      :param authChallenge: The authentication challenge.
      :type authChallenge: str
      :param authSecret: The authentication secret.
      :type authSecret: str
      :authExtra: Extra authentication information for salting the secret. (salt, keylen,
              iterations)
      :type authExtra: dict

      :returns str -- The authentication signature.
      """
      if authSecret is None:
         authSecret = ""
      if isinstance(authSecret, unicode):
         authSecret = authSecret.encode('utf8')
      authSecret = WampCraProtocol.deriveKey(authSecret, authExtra)
      h = hmac.new(authSecret, authChallenge, hashlib.sha256)
      sig = binascii.b2a_base64(h.digest()).strip()
      return sig



class WampCraClientProtocol(WampClientProtocol, WampCraProtocol):
   """
   Simple, authenticated WAMP client protocol.

   The client can perform WAMP-Challenge-Response-Authentication ("WAMP-CRA") to authenticate
   itself to a WAMP server. The server needs to implement WAMP-CRA also of course.
   """

   def authenticate(self, authKey = None, authExtra = None, authSecret = None):
      """
      Authenticate the WAMP session to server.

      :param authKey: The key of the authentication credentials, something like a user or application name.
      :type authKey: str
      :param authExtra: Any extra authentication information.
      :type authExtra: dict
      :param authSecret: The secret of the authentication credentials, something like the user password or application secret key.
      :type authsecret: str

      :returns Deferred -- Deferred that fires upon authentication success (with permissions) or failure.
      """

      def _onAuthChallenge(challenge):
         if authKey is not None:
            challengeObj =  self.factory._unserialize(challenge)
            if 'authextra' in challengeObj:
                authExtra = challengeObj['authextra']
                sig = self.authSignature(challenge, authSecret, authExtra)
            else:
                sig = self.authSignature(challenge, authSecret)
         else:
            sig = None
         d = self.call(WampProtocol.URI_WAMP_PROCEDURE + "auth", sig)
         return d

      d = self.call(WampProtocol.URI_WAMP_PROCEDURE + "authreq", authKey, authExtra)
      d.addCallback(_onAuthChallenge)
      return d



class WampCraServerProtocol(WampServerProtocol, WampCraProtocol):
   """
   Simple, authenticating WAMP server protocol.

   The server lets clients perform WAMP-Challenge-Response-Authentication ("WAMP-CRA")
   to authenticate. The clients need to implement WAMP-CRA also of course.

   To implement an authenticating server, override:

      * getAuthSecret
      * getAuthPermissions
      * onAuthenticated

   in your class deriving from this class.
   """

   clientAuthTimeout = 0
   """
   Client authentication timeout in seconds or 0 for infinite. A client
   must perform authentication after the initial WebSocket handshake within
   this timeout or the connection is failed.
   """

   clientAuthAllowAnonymous = True
   """
   Allow anonymous client authentication. When this is set to True, a client
   may "authenticate" as anonymous.
   """


   def getAuthPermissions(self, authKey, authExtra):
      """
      Get the permissions the session is granted when the authentication succeeds
      for the given key / extra information.

      Override in derived class to implement your authentication.

      A permissions object is structured like this::

         {'permissions': {'rpc': [
                                    {'uri':  / RPC Endpoint URI - String /,
                                     'call': / Allow to call? - Boolean /}
                                 ],
                          'pubsub': [
                                       {'uri':    / PubSub Topic URI / URI prefix - String /,
                                        'prefix': / URI matched by prefix? - Boolean /,
                                        'pub':    / Allow to publish? - Boolean /,
                                        'sub':    / Allow to subscribe? - Boolean /}
                                    ]
                          }
         }

      You can add custom information to this object. The object will be provided again
      when the client authentication succeeded in :meth:`onAuthenticated`.

      :param authKey: The authentication key.
      :type authKey: str
      :param authExtra: Authentication extra information.
      :type authExtra: dict

      :returns obj or Deferred -- Return a permissions object or None when no permissions granted.
      """
      return None


   def getAuthSecret(self, authKey):
      """
      Get the authentication secret for an authentication key, i.e. the
      user password for the user name. Return None when the authentication
      key does not exist.

      Override in derived class to implement your authentication.

      :param authKey: The authentication key.
      :type authKey: str

      :returns str or Deferred -- The authentication secret for the key or None when the key does not exist.
      """
      return None


   def onAuthTimeout(self):
      """
      Fired when the client does not authenticate itself in time. The default implementation
      will simply fail the connection.

      May be overridden in derived class.
      """
      if not self._clientAuthenticated:
         log.msg("failing connection upon client authentication timeout [%s secs]" % self.clientAuthTimeout)
         self.failConnection()


   def onAuthenticated(self, authKey, permissions):
      """
      Fired when client authentication was successful.

      Override in derived class and register PubSub topics and/or RPC endpoints.

      :param authKey: The authentication key the session was authenticated for.
      :type authKey: str
      :param permissions: The permissions object returned from :meth:`getAuthPermissions`.
      :type permissions: obj
      """
      pass


   def registerForPubSubFromPermissions(self, permissions):
      """
      Register topics for PubSub from auth permissions.

      :param permissions: The permissions granted to the now authenticated client.
      :type permissions: list
      """
      for p in permissions['pubsub']:
         ## register topics for the clients
         ##
         pubsub = (WampServerProtocol.PUBLISH if p['pub'] else 0) | \
                  (WampServerProtocol.SUBSCRIBE if p['sub'] else 0)
         topic = p['uri']
         if self.pubHandlers.has_key(topic) or self.subHandlers.has_key(topic):
            ## FIXME: handle dups!
            log.msg("DUPLICATE TOPIC PERMISSION !!! " + topic)
         self.registerForPubSub(topic, p['prefix'], pubsub)


   def onSessionOpen(self):
      """
      Called when WAMP session has been established, but not yet authenticated. The default
      implementation will prepare the session allowing the client to authenticate itself.
      """

      ## register RPC endpoints for WAMP-CRA authentication
      ##
      self.registerForRpc(self, WampProtocol.URI_WAMP_PROCEDURE, [WampCraServerProtocol.authRequest,
                                                                  WampCraServerProtocol.auth])

      ## reset authentication state
      ##
      self._clientAuthenticated = False
      self._clientPendingAuth = None
      self._clientAuthTimeoutCall = None

      ## client authentication timeout
      ##
      if self.clientAuthTimeout > 0:
         self._clientAuthTimeoutCall = self.factory.reactor.callLater(self.clientAuthTimeout, self.onAuthTimeout)


   @exportRpc("authreq")
   def authRequest(self, authKey = None, extra = None):
      """
      RPC endpoint for clients to initiate the authentication handshake.

      :param authKey: Authentication key, such as user name or application name.
      :type authKey: str
      :param extra: Authentication extra information.
      :type extra: dict

      :returns str -- Authentication challenge. The client will need to create an authentication signature from this.
      """

      ## check authentication state
      ##
      if self._clientAuthenticated:
         raise Exception(self.shrink(WampProtocol.URI_WAMP_ERROR + "already-authenticated"), "already authenticated")
      if self._clientPendingAuth is not None:
         raise Exception(self.shrink(WampProtocol.URI_WAMP_ERROR + "authentication-already-requested"), "authentication request already issues - authentication pending")

      ## check extra
      ##
      if extra:
         if type(extra) != dict:
            raise Exception(self.shrink(WampProtocol.URI_WAMP_ERROR + "invalid-argument"), "extra not a dictionary (was %s)." % str(type(extra)))
      else:
         extra = {}
      #for k in extra:
      #   if type(extra[k]) not in [str, unicode, int, long, float, bool, types.NoneType]:
      #      raise Exception(self.shrink(WampProtocol.URI_WAMP_ERROR + "invalid-argument"), "attribute '%s' in extra not a primitive type (was %s)" % (k, str(type(extra[k]))))

      ## check authKey
      ##
      if authKey is None and not self.clientAuthAllowAnonymous:
         raise Exception(self.shrink(WampProtocol.URI_WAMP_ERROR + "anonymous-auth-forbidden"), "authentication as anonymous forbidden")

      if type(authKey) not in [str, unicode, types.NoneType]:
         raise Exception(self.shrink(WampProtocol.URI_WAMP_ERROR + "invalid-argument"), "authentication key must be a string (was %s)" % str(type(authKey)))

      d = maybeDeferred(self.getAuthSecret, authKey)

      def onGetAuthSecretOk(authSecret, authKey, extra):
         if authKey is not None and authSecret is None:
            raise Exception(self.shrink(WampProtocol.URI_WAMP_ERROR + "no-such-authkey"), "authentication key '%s' does not exist." % authKey)

         ## each authentication request gets a unique authid, which can only be used (later) once!
         ##
         authid = newid()

         ## create authentication challenge
         ##
         info = {'authid': authid, 'authkey': authKey, 'timestamp': utcnow(), 'sessionid': self.session_id,
                 'extra': extra}

         pp = maybeDeferred(self.getAuthPermissions, authKey, extra)

         def onAuthPermissionsOk(res):
            if res is None:
               res = {'permissions': {'pubsub': [], 'rpc': []}}
            info['permissions'] = res['permissions']
            if 'authextra' in res:
                info['authextra'] = res['authextra']

            if authKey:
               ## authenticated session
               ##
               infoser = self.factory._serialize(info)
               sig = self.authSignature(infoser, authSecret)

               self._clientPendingAuth = (info, sig, res)
               return infoser
            else:
               ## anonymous session
               ##
               self._clientPendingAuth = (info, None, res)
               return None

         def onAuthPermissionsError(e):
            raise Exception(self.shrink(WampProtocol.URI_WAMP_ERROR + "auth-permissions-error"), str(e))

         pp.addCallbacks(onAuthPermissionsOk, onAuthPermissionsError)

         return pp

      d.addCallback(onGetAuthSecretOk, authKey, extra)
      return d


   @exportRpc("auth")
   def auth(self, signature = None):
      """
      RPC endpoint for clients to actually authenticate after requesting authentication and computing
      a signature from the authentication challenge.

      :param signature: Authentication signature computed by the client.
      :type signature: str

      :returns list -- A list of permissions the client is granted when authentication was successful.
      """

      ## check authentication state
      ##
      if self._clientAuthenticated:
         raise Exception(self.shrink(WampProtocol.URI_WAMP_ERROR + "already-authenticated"), "already authenticated")
      if self._clientPendingAuth is None:
         raise Exception(self.shrink(WampProtocol.URI_WAMP_ERROR + "no-authentication-requested"), "no authentication previously requested")

      ## check signature
      ##
      if type(signature) not in [str, unicode, types.NoneType]:
         raise Exception(self.shrink(WampProtocol.URI_WAMP_ERROR + "invalid-argument"), "signature must be a string or None (was %s)" % str(type(signature)))
      if self._clientPendingAuth[1] != signature:
         ## delete pending authentication, so that no retries are possible. authid is only valid for 1 try!!
         ##
         self._clientPendingAuth = None

         ## notify the client of failed authentication, but only after a random,
         ## exponentially distributed delay. this (further) protects against
         ## timing attacks
         ##
         d = Deferred()
         def fail():
            ## FIXME: (optionally) drop the connection instead of returning RPC error?
            ##
            d.errback(Exception(self.shrink(WampProtocol.URI_WAMP_ERROR + "invalid-signature"), "signature for authentication request is invalid"))
         failDelaySecs = random.expovariate(1.0 / 0.8) # mean = 0.8 secs
         self.factory.reactor.callLater(failDelaySecs, fail)
         return d

      ## at this point, the client has successfully authenticated!

      ## get the permissions we determined earlier
      ##
      perms = self._clientPendingAuth[2]

      ## delete auth request and mark client as authenticated
      ##
      authKey = self._clientPendingAuth[0]['authkey']
      self._clientAuthenticated = True
      self._clientPendingAuth = None
      if self._clientAuthTimeoutCall is not None:
         self._clientAuthTimeoutCall.cancel()
         self._clientAuthTimeoutCall = None

      ## fire authentication callback
      ##
      self.onAuthenticated(authKey, perms)

      ## return permissions to client
      ##
      return perms['permissions']



class Call:
   """
   Thin-wrapper for incoming RPCs provided to call handlers registered via

     - registerHandlerMethodForRpc
     - registerHandlerProcedureForRpc
   """


   def __init__(self,
             proto,
             callid,
             uri,
             args,
             extra = None):
      self.proto = proto
      self.callid = callid
      self.uri = uri
      self.args = args
      self.extra = extra
      if self.proto.trackTimings:
          self.timings = Tracker(tracker=None, tracked=None)
      else:
          self.timings = None

   def track(self, key):
       if self.timings:
           self.timings.track(key)



class Handler(object):
   """
   A handler for a certain class of messages.
   """


   typeid = None
   tracker = None


   def __init__(self, proto, prefixes):
      """
      Remember protocol and prefix map in instance variables.
      """
      self.proto = proto
      self.prefixes = prefixes


   def handleMessage(self, msg_parts):
      """
      Template method for handling a message.

      Check if the correct handler for the message type was
      called. Afterwards, assign all relevant parts of the message to
      instance variables and call the (overridden) method
      _handleMessage to actually handle the message.
      """
      msgtype = msg_parts[0]
      if self.typeid:
         assert msgtype == self.typeid, \
             "Message type %s does not match type id %s" % (msgtype,
                                                            self.typeid)
      else:
         assert False, \
             "No typeid defined for %s" % self.__class__.__name__

      if self._messageIsValid(msg_parts):
         self._parseMessageParts(msg_parts)
         self._handleMessage()


   def _parseMessageParts(self, msg_parts):
      """
      Assign the message parts to instance variables.
      Has to be overridden in subclasses.
      """
      raise NotImplementedError

   def _messageIsValid(self, msg_parts):
      """
      Check if the message parts have expected properties (type, etc.).
      Has to be overridden in subclasses.
      """
      raise NotImplementedError


   def _handleMessage(self):
      """
      Handle a specific kind of message.
      Has to be overridden in subclasses.
      """
      raise NotImplementedError



class CallHandler(Handler):
   """
   A handler for incoming RPC calls.
   """

   typeid = WampProtocol.MESSAGE_TYPEID_CALL


   def _messageIsValid(self, msg_parts):
      callid, uri = msg_parts[1:3]
      if not isinstance(callid, (str, unicode)):
         self.proto._protocolError(
            ("WAMP CALL message with invalid type %s for "
            "<callid>") % type(callid))
         return False

      if not isinstance(uri, (str, unicode)):
         self.proto._protocolError(
            ("WAMP CALL message with invalid type %s for "
            "<uri>") % type(uri))
         return False

      return True


   def _parseMessageParts(self, msg_parts):
      """
      Parse message and create call object.
      """
      self.callid = msg_parts[1]
      self.uri = self.prefixes.resolveOrPass(msg_parts[2]) ### PFX - remove
      self.args = msg_parts[3:]


   def _handleMessage(self):
      """
      Perform the RPC call and attach callbacks to its deferred object.
      """
      call = self._onBeforeCall()

      ## execute incoming RPC
      d = maybeDeferred(self._callProcedure, call)

      ## register callback and errback with extra argument call
      d.addCallbacks(self._onAfterCallSuccess,
                     self._onAfterCallError,
                     callbackArgs = (call,),
                     errbackArgs = (call,))


   def _onBeforeCall(self):
      """
      Create call object to move around call data
      """
      uri, args = self.proto.onBeforeCall(self.callid, self.uri, self.args, bool(self.proto.procForUri(self.uri)))

      call = Call(self.proto, self.callid, uri, args)
      call.track("onBeforeCall")
      return call


   def _callProcedure(self, call):
      """
      Actually performs the call of a procedure invoked via RPC.
      """
      m = self.proto.procForUri(call.uri)
      if m is None:
         raise Exception(WampProtocol.URI_WAMP_ERROR_NO_SUCH_RPC_ENDPOINT, "No RPC endpoint registered for %s." % call.uri)

      obj, method_or_proc, is_handler = m[:3]
      if not is_handler:
         return self._performProcedureCall(call, obj, method_or_proc)
      else:
         call.extra = m[3]
         return self._delegateToRpcHandler(call, obj, method_or_proc)


   def _performProcedureCall(self, call, obj, method_or_proc):
      """
      Perform a RPC method / procedure call.
      """
      cargs = tuple(call.args) if call.args else ()
      if obj:
         ## call object method
         return method_or_proc(obj, *cargs)
      else:
         ## call free-standing function/procedure
         return method_or_proc(*cargs)


   def _delegateToRpcHandler(self, call, obj, method_or_proc):
      """
      Delegate call to RPC handler.
      """
      if obj:
         ## call RPC handler on object
         return method_or_proc(obj, call)
      else:
         ## call free-standing RPC handler
         return method_or_proc(call)


   def _onAfterCallSuccess(self, result, call):
      """
      Execute custom success handler and send call result.
      """
      ## track timing and fire user callback
      call.track("onAfterCallSuccess")
      call.result = self.proto.onAfterCallSuccess(result, call)

      ## send out WAMP message
      self._sendCallResult(call)


   def _onAfterCallError(self, error, call):
      """
      Execute custom error handler and send call error.
      """
      ## track timing and fire user callback
      call.track("onAfterCallError")
      call.error = self.proto.onAfterCallError(error, call)

      ## send out WAMP message
      self._sendCallError(call)


   def _sendCallResult(self, call):
      """
      Marshal and send a RPC success result.
      """
      msg = [WampProtocol.MESSAGE_TYPEID_CALL_RESULT, call.callid, call.result]
      try:
         rmsg = self.proto.serializeMessage(msg)
      except:
         raise Exception("call result not JSON serializable")
      else:
         ## now actually send WAMP message
         self.proto.sendMessage(rmsg)

         ## track timing and fire user callback
         call.track("onAfterSendCallSuccess")
         self.proto.onAfterSendCallSuccess(rmsg, call)


   def _sendCallError(self, call):
      """
      Marshal and send a RPC error result.
      """
      killsession = False
      rmsg = None
      try:
         error_info, killsession = self._extractErrorInfo(call)
         rmsg = self._assembleErrorMessage(call, *error_info)
      except Exception as e:
         rmsg = self._handleProcessingError(call, e)
      finally:
         if rmsg:
            ## now actually send WAMP message
            self.proto.sendMessage(rmsg)

            ## track timing and fire user callback
            call.track("onAfterSendCallError")
            self.proto.onAfterSendCallError(rmsg, call)

            if killsession:
               self.proto.sendClose(3000, u"killing WAMP session upon request by application exception")
         else:
            raise Exception("fatal: internal error in CallHandler._sendCallError")


   def _extractErrorInfo(self, call):
      """
      Extract error information from the call.
      """
      ## get error args and len
      ##
      eargs = call.error.value.args
      nargs = len(eargs)

      if nargs > 4:
         raise Exception("invalid args length %d for exception" % nargs)

      ## erroruri & errordesc
      ##
      if nargs == 0:
         erroruri = WampProtocol.URI_WAMP_ERROR_GENERIC
         errordesc = WampProtocol.DESC_WAMP_ERROR_GENERIC
      elif nargs == 1:
         erroruri = WampProtocol.URI_WAMP_ERROR_GENERIC
         errordesc = eargs[0]
      else:
         erroruri = eargs[0]
         errordesc = eargs[1]

      ## errordetails
      ##
      errordetails = None
      if nargs >= 3:
         errordetails = eargs[2]
      elif self.proto.includeTraceback:
         try:
            ## we'd like to do ..
            #tb = call.error.getTraceback()

            ## .. but the implementation in Twisted
            ## http://twistedmatrix.com/trac/browser/tags/releases/twisted-13.1.0/twisted/python/failure.py#L529
            ## uses cStringIO which cannot handle Unicode string in tracebacks. Hence we do our own:
            io = StringIO.StringIO()
            call.error.printTraceback(file = io)
            tb = io.getvalue()

         except Exception as ie:
            print("INTERNAL ERROR [_extractErrorInfo / getTraceback()]: %s" % ie)
            traceback.print_stack()
         else:
            errordetails = tb.splitlines()

      ## killsession
      ##
      killsession = False
      if nargs >= 4:
         killsession = eargs[3]

      ## recheck all error component types
      ##
      if type(erroruri) not in [str, unicode]:
         raise Exception("invalid type %s for errorUri" % type(erroruri))

      if type(errordesc) not in [str, unicode]:
         raise Exception("invalid type %s for errorDesc" % type(errordesc))

      ## errordetails must be JSON serializable. If not, we get exception later in sendMessage.
      ## We don't check here, since the only way would be to serialize to JSON and
      ## then we'd serialize twice (here and in sendMessage)

      if type(killsession) not in [bool, types.NoneType]:
         raise Exception("invalid type %s for killSession" % type(killsession))

      return (erroruri, errordesc, errordetails), killsession


   def _assembleErrorMessage(self, call, erroruri, errordesc, errordetails):
      """
      Assemble a WAMP RPC error message.
      """
      if errordetails is not None:
         msg = [WampProtocol.MESSAGE_TYPEID_CALL_ERROR,
                call.callid,
                self.prefixes.shrink(erroruri), ### PFX - remove
                errordesc,
                errordetails]
      else:
         msg = [WampProtocol.MESSAGE_TYPEID_CALL_ERROR,
                call.callid,
                self.prefixes.shrink(erroruri), ### PFX - remove
                errordesc]

      ## serialize message. this can fail if errorDetails is not
      ## serializable
      try:
         rmsg = self.proto.serializeMessage(msg)
      except Exception as e:
         raise Exception(
            "invalid object for errorDetails - not serializable (%s)" %
            str(e))

      return rmsg


   def _handleProcessingError(self, call, e):
      """
      Create a message describing what went wrong during processing an
      exception.
      """
      msg = [WampProtocol.MESSAGE_TYPEID_CALL_ERROR,
             call.callid,
              ### PFX - remove
             self.prefixes.shrink(WampProtocol.URI_WAMP_ERROR_INTERNAL),
             str(e)]

      if self.proto.includeTraceback:
         try:
            tb = call.error.getTraceback()
         except Exception as ie:
            ## FIXME: find out why this can fail with
            ## "'unicode' does not have the buffer interface"
            print("INTERNAL ERROR (getTraceback): %s" % ie)
         else:
            msg.append(tb.splitlines())

      result = self.proto.serializeMessage(msg)
      return result




class CallResultHandler(Handler):
   """
   A handler for to RPC call results.
   """

   typeid = WampProtocol.MESSAGE_TYPEID_CALL_RESULT


   def _messageIsValid(self, msg_parts):
      if len(msg_parts) < 2:
         self.proto._protocolError(
            "WAMP CALL_RESULT message without <callid>")
         return False
      if len(msg_parts) != 3:
         self.proto._protocolError(
            "WAMP CALL_RESULT message with invalid length %d" % len(msg_parts))
         return False

      if type(msg_parts[1]) not in [unicode, str]:
         self.proto._protocolError(
            ("WAMP CALL_RESULT message with invalid type %s for "
            "<callid>") % type(msg_parts[1]))
         return False

      return True


   def _parseMessageParts(self, msg_parts):
      """
      Extract call result from message parts.
      """
      self.callid = str(msg_parts[1])
      self.result = msg_parts[2]


   def _handleMessage(self):
      ## Pop and process Call Deferred
      ##
      d = self.proto.calls.pop(self.callid, None)
      if d:
         ## WAMP CALL_RESULT
         ##
         d.callback(self.result)
      else:
         if self.proto.debugWamp:
            log.msg("callid not found for received call result message")



class CallErrorHandler(Handler):

   typeid = WampProtocol.MESSAGE_TYPEID_CALL_ERROR


   def _messageIsValid(self, msg_parts):
      if len(msg_parts) not in [4, 5]:
         self.proto._protocolError(
            "call error message invalid length %d" % len(msg_parts))
         return False

      ## Error URI
      ##
      if type(msg_parts[2]) not in [unicode, str]:
         self.proto._protocolError(
            "invalid type %s for errorUri in call error message" %
            str(type(msg_parts[2])))
         return False

      ## Error Description
      ##
      if type(msg_parts[3]) not in [unicode, str]:
         self.proto._protocolError(
            "invalid type %s for errorDesc in call error message" %
            str(type(msg_parts[3])))
         return False

      return True


   def _parseMessageParts(self, msg_parts):
      """
      Extract error information from message parts.
      """
      self.callid = str(msg_parts[1])
      self.erroruri = str(msg_parts[2])
      self.errordesc = str(msg_parts[3])

      ## Error Details
      ##
      if len(msg_parts) > 4:
         self.errordetails = msg_parts[4]
      else:
         self.errordetails = None


   def _handleMessage(self):
      """
      Fire Call Error Deferred.
      """
      ##
      ## Pop and process Call Deferred
      d = self.proto.calls.pop(self.callid, None)
      if d:
         e = Exception(self.erroruri, self.errordesc, self.errordetails)
         d.errback(e)
      else:
         if self.proto.debugWamp:
            log.msg("callid not found for received call error message")
