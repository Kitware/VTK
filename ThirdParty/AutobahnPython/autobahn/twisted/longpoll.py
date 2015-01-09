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

__all__ = ['WampLongPollResource']


import json
import traceback
import binascii

from collections import deque

from twisted.python import log
from twisted.web.resource import Resource, NoResource

## Each of the following 2 trigger a reactor import at module level
from twisted.web import http
from twisted.web.server import NOT_DONE_YET

from autobahn.util import newid

from autobahn.wamp.websocket import parseSubprotocolIdentifier

from autobahn.wamp.exception import ProtocolError, \
                                    SerializationError, \
                                    TransportLost



class WampLongPollResourceSessionSend(Resource):
   """
   A Web resource for sending via XHR that is part of :class:`autobahn.twisted.longpoll.WampLongPollResourceSession`.
   """

   def __init__(self, parent):
      """
      Ctor.

      :param parent: The Web parent resource for the WAMP session.
      :type parent: Instance of :class:`autobahn.twisted.longpoll.WampLongPollResourceSession`.
      """
      Resource.__init__(self)
      self._parent = parent
      self._debug = self._parent._parent._debug


   def render_POST(self, request):
      """
      A client sends a message via WAMP-over-Longpoll by HTTP/POSTing
      to this Web resource. The body of the POST should contain a batch
      ## of WAMP messages which are serialized according to the selected
      serializer, and delimited by a single \0 byte in between two WAMP
      messages in the batch.
      """
      payload = request.content.read()
      if self._debug:
         log.msg("WampLongPoll: receiving data for transport '{0}'\n{1}".format(self._parent._transportid, binascii.hexlify(payload)))

      try:
         ## process (batch of) WAMP message(s)
         self._parent.onMessage(payload, None)

      except Exception as e:
         return self._parent._parent._failRequest(request, "could not unserialize WAMP message: {0}".format(e))

      else:
         request.setResponseCode(http.NO_CONTENT)
         self._parent._parent._setStandardHeaders(request)
         self._parent._isalive = True
         return ""



class WampLongPollResourceSessionReceive(Resource):
   """
   A Web resource for receiving via XHR that is part of :class:`autobahn.twisted.longpoll.WampLongPollResourceSession`.
   """

   def __init__(self, parent):
      """
      Ctor.

      :param parent: The Web parent resource for the WAMP session.
      :type parent: Instance of :class:`autobahn.twisted.longpoll.WampLongPollResourceSession`.
      """
      Resource.__init__(self)
      self._parent = parent
      self._debug = self._parent._parent._debug
      self.reactor = self._parent._parent.reactor

      self._queue = deque()
      self._request = None
      self._killed = False

      if self._debug:
         def logqueue():
            if not self._killed:
               log.msg("WampLongPoll: transport '{0}' - currently polled {1}, pending messages {2}".format(self._parent._transportid, self._request is not None, len(self._queue)))
               self.reactor.callLater(1, logqueue)
         logqueue()


   def queue(self, data):
      """
      Enqueue data to be received by client.

      :param data: The data to be received by the client.
      :type data: bytes
      """
      self._queue.append(data)
      self._trigger()


   def _kill(self):
      """
      Kill any outstanding request.
      """
      if self._request:
         self._request.finish()
         self._request = None
      self._killed = True


   def _trigger(self):
      """
      Trigger batched sending of queued messages.
      """
      if self._request and len(self._queue):

         if self._parent._serializer._serializer._batched:
            ## in batched mode, write all pending messages
            while len(self._queue) > 0:
               msg = self._queue.popleft()
               self._request.write(msg)
         else:
            ## in unbatched mode, only write 1 pending message
            msg = self._queue.popleft()
            self._request.write(msg)

         self._request.finish()
         self._request = None


   def render_POST(self, request):
      """
      A client receives WAMP messages by issuing a HTTP/POST to this
      Web resource. The request will immediately return when there are
      messages pending to be received. When there are no such messages
      pending, the request will "just hang", until either a message
      arrives to be received or a timeout occurs.
      """
      ## remember request, which marks the session as being polled
      self._request = request

      self._parent._parent._setStandardHeaders(request)
      request.setHeader('content-type', self._parent._serializer.MIME_TYPE)

      def cancel(err):
         if self._debug:
            log.msg("WampLongPoll: poll request for transport '{0}' has gone away".format(self._parent._transportid))
         self._request = None

      request.notifyFinish().addErrback(cancel)

      self._parent._isalive = True
      self._trigger()

      return NOT_DONE_YET



class WampLongPollResourceSessionClose(Resource):
   """
   A Web resource for closing the Long-poll session WampLongPollResourceSession.
   """

   def __init__(self, parent):
      """
      Ctor.

      :param parent: The Web parent resource for the WAMP session.
      :type parent: Instance of :class:`autobahn.twisted.longpoll.WampLongPollResourceSession`.
      """
      Resource.__init__(self)
      self._parent = parent
      self._debug = self._parent._parent._debug


   def render_POST(self, request):
      """
      A client may actively close a session (and the underlying long-poll transport)
      by issuing a HTTP/POST with empty body to this resource.
      """
      if self._debug:
         log.msg("WampLongPoll: closing transport '{0}'".format(self._parent._transportid))

      ## now actually close the session
      self._parent.close()

      if self._debug:
         log.msg("WampLongPoll: session ended and transport {0} closed".format(self._parent._transportid))

      request.setResponseCode(http.NO_CONTENT)
      self._parent._parent._setStandardHeaders(request)
      return ""



class WampLongPollResourceSession(Resource):
   """
   A Web resource representing an open WAMP session.
   """

   def __init__(self, parent, transportid, serializer):
      """
      Create a new Web resource representing a WAMP session.

      :param parent: The WAMP Web base resource.
      :type parent: Instance of WampLongPollResource.
      :param serializer: The WAMP serializer in use for this session.
      :type serializer: An instance of WampSerializer.
      """
      Resource.__init__(self)

      self._parent = parent
      self._debug = self._parent._debug
      self._debug_wamp = True
      self.reactor = self._parent.reactor

      self._transportid = transportid
      self._serializer = serializer
      self._session = None

      ## session authentication information
      ##
      self._authid = None
      self._authrole = None
      self._authmethod = None

      self._send = WampLongPollResourceSessionSend(self)
      self._receive = WampLongPollResourceSessionReceive(self)
      self._close = WampLongPollResourceSessionClose(self)

      self.putChild("send", self._send)
      self.putChild("receive", self._receive)
      self.putChild("close", self._close)

      killAfter = self._parent._killAfter
      self._isalive = False

      def killIfDead():
         if not self._isalive:
            if self._debug:
               log.msg("WampLongPoll: killing inactive WAMP session with transport '{0}'".format(self._transportid))

            self.onClose(False, 5000, "session inactive")
            self._receive._kill()
            del self._parent._transports[self._transportid]
         else:
            if self._debug:
               log.msg("WampLongPoll: transport '{0}'' is still alive".format(self._transportid))

            self._isalive = False
            self.reactor.callLater(killAfter, killIfDead)

      self.reactor.callLater(killAfter, killIfDead)

      if self._debug:
         log.msg("WampLongPoll: session resource for transport '{0}' initialized)".format(self._transportid))

      self.onOpen()


   def close(self):
      """
      Implements :func:`autobahn.wamp.interfaces.ITransport.close`
      """
      if self.isOpen():
         self.onClose(True, 1000, "session closed")
         self._receive._kill()
         del self._parent._transports[self._transportid]
      else:
         raise TransportLost()


   def abort(self):
      """
      Implements :func:`autobahn.wamp.interfaces.ITransport.abort`
      """
      if self.isOpen():
         self.onClose(True, 1000, "session aborted")
         self._receive._kill()
         del self._parent._transports[self._transportid]
      else:
         raise TransportLost()


   def onClose(self, wasClean, code, reason):
      """
      Callback from :func:`autobahn.websocket.interfaces.IWebSocketChannel.onClose`
      """
      if self._session:
         try:
            self._session.onClose(wasClean)
         except Exception:
            ## silently ignore exceptions raised here ..
            if self._debug:
               traceback.print_exc()
         self._session = None


   def onOpen(self):
      """
      Callback from :func:`autobahn.websocket.interfaces.IWebSocketChannel.onOpen`
      """
      try:
         self._session = self._parent._factory()
         self._session.onOpen(self)
      except Exception:
         if self._debug:
            traceback.print_exc()


   def onMessage(self, payload, isBinary):
      """
      Callback from :func:`autobahn.websocket.interfaces.IWebSocketChannel.onMessage`
      """
      for msg in self._serializer.unserialize(payload, isBinary):
         if self._debug:
            print("WampLongPoll: RX {0}".format(msg))
         self._session.onMessage(msg)


   def send(self, msg):
      """
      Implements :func:`autobahn.wamp.interfaces.ITransport.send`
      """
      if self.isOpen():
         try:
            if self._debug:
               print("WampLongPoll: TX {0}".format(msg))
            bytes, isBinary = self._serializer.serialize(msg)
         except Exception as e:
            ## all exceptions raised from above should be serialization errors ..
            raise SerializationError("unable to serialize WAMP application payload ({0})".format(e))
         else:
            self._receive.queue(bytes)
      else:
         raise TransportLost()


   def isOpen(self):
      """
      Implements :func:`autobahn.wamp.interfaces.ITransport.isOpen`
      """
      return self._session is not None



class WampLongPollResourceOpen(Resource):
   """
   A Web resource for creating new WAMP sessions.
   """

   def __init__(self, parent):
      """
      Ctor.

      :param parent: The parent Web resource.
      :type parent: Instance of `WampLongPollResource`.
      """
      Resource.__init__(self)
      self._parent = parent
      self._debug = self._parent._debug


   def render_POST(self, request):
      """
      Request to create a new WAMP session.
      """
      if self._debug:
         log.msg("WampLongPoll: creating new session ..")

      payload = request.content.read()
      try:
         options = json.loads(payload)
      except Exception as e:
         return self._parent._failRequest(request, "could not parse WAMP session open request body: {0}".format(e))

      if type(options) != dict:
         return self._parent._failRequest(request, "invalid type for WAMP session open request [was {0}, expected dictionary]".format(type(options)))

      if not 'protocols' in options:
         return self._parent._failRequest(request, "missing attribute 'protocols' in WAMP session open request")

      ## determine the protocol to speak
      ##
      protocol = None
      serializer = None
      for p in options['protocols']:
         version, serializerId = parseSubprotocolIdentifier(p)
         if version == 2 and serializerId in self._parent._serializers.keys():
            serializer = self._parent._serializers[serializerId]
            protocol = p
            break

      if protocol is None:
         return self.__failRequest(request, "no common protocol to speak (I speak: {0})".format(["wamp.2.{}".format(s) for s in self._parent._serializers.keys()]))

      ## make up new transport ID
      ##
      if self._parent._debug_transport_id:
         ## use fixed transport ID for debugging purposes
         transport = self._parent._debug_transport_id
      else:
         transport = newid()

      ## create instance of WampLongPollResourceSession or subclass thereof ..
      ##
      self._parent._transports[transport] = self._parent.protocol(self._parent, transport, serializer)

      ## create response
      ##
      self._parent._setStandardHeaders(request)
      request.setHeader('content-type', 'application/json; charset=utf-8')

      result = {
         'transport': transport,
         'protocol': protocol
      }

      bytes = json.dumps(result)

      if self._debug:
         log.msg("WampLongPoll: new session created on transport '{0}'".format(transport))

      return bytes



class WampLongPollResource(Resource):
   """
   A WAMP-over-Longpoll resource for use with Twisted Web Resource trees.

   This class provides an implementation of the
   `WAMP-over-Longpoll Transport <https://github.com/tavendo/WAMP/blob/master/spec/advanced.md#long-poll-transport>`_
   for WAMP.

   The Resource exposes the following paths (child resources).

   Opening a new WAMP session:

      * ``<base-url>/open``

   Once a transport is created and the session is opened:

      * ``<base-url>/<transport-id>/send``
      * ``<base-url>/<transport-id>/receive``
      * ``<base-url>/<transport-id>/close``
   """

   protocol = WampLongPollResourceSession


   def __init__(self,
                factory,
                serializers = None,
                timeout = 10,
                killAfter = 30,
                queueLimitBytes = 128 * 1024,
                queueLimitMessages = 100,
                debug = False,
                debug_transport_id = None,
                reactor = None):
      """
      Create new HTTP WAMP Web resource.

      :param factory: A (router) session factory.
      :type factory: Instance of `RouterSessionFactory`.
      :param serializers: List of WAMP serializers.
      :type serializers: List of WampSerializer objects.
      :param timeout: XHR polling timeout in seconds.
      :type timeout: int
      :param killAfter: Kill WAMP session after inactivity in seconds.
      :type killAfter: int
      :param queueLimitBytes: Kill WAMP session after accumulation of this many bytes in send queue (XHR poll).
      :type queueLimitBytes: int
      :param queueLimitMessages: Kill WAMP session after accumulation of this many message in send queue (XHR poll).
      :type queueLimitMessages: int
      :param debug: Enable debug logging.
      :type debug: bool
      :param debug_transport_id: If given, use this fixed transport ID.
      :type debug_transport_id: str
      :param reactor: The Twisted reactor to run under.
      :type reactor: obj
      """
      Resource.__init__(self)

      ## RouterSessionFactory
      self._factory = factory

      ## lazy import to avoid reactor install upon module import
      if reactor is None:
         from twisted.internet import reactor
      self.reactor = reactor

      self._debug = debug
      self._debug_transport_id = debug_transport_id
      self._timeout = timeout
      self._killAfter = killAfter
      self._queueLimitBytes = queueLimitBytes
      self._queueLimitMessages = queueLimitMessages

      if serializers is None:
         serializers = []

         ## try MsgPack WAMP serializer
         try:
            from autobahn.wamp.serializer import MsgPackSerializer
            serializers.append(MsgPackSerializer(batched = True))
            serializers.append(MsgPackSerializer())
         except ImportError:
            pass

         ## try JSON WAMP serializer
         try:
            from autobahn.wamp.serializer import JsonSerializer
            serializers.append(JsonSerializer(batched = True))
            serializers.append(JsonSerializer())
         except ImportError:
            pass

         if not serializers:
            raise Exception("could not import any WAMP serializers")

      self._serializers = {}
      for ser in serializers:
         self._serializers[ser.SERIALIZER_ID] = ser

      self._transports = {}

      ## <Base URL>/open
      ##
      self.putChild("open", WampLongPollResourceOpen(self))

      if self._debug:
         log.msg("WampLongPollResource initialized")


   def render_GET(self, request):
      request.setHeader('content-type', 'text/html; charset=UTF-8')
      peer = "{}:{}".format(request.client.host, request.client.port)
      return self.getNotice(peer = peer)


   def getChild(self, name, request):
      """
      Returns send/receive/close resource for transport.

      .. seealso::

         * :class:`twisted.web.resource.Resource`
         * :class:`zipfile.ZipFile`
      """
      if name not in self._transports:
         return NoResource("no WAMP transport '{}'".format(name))

      if len(request.postpath) != 1 or request.postpath[0] not in ['send', 'receive', 'close']:
         return NoResource("invalid WAMP transport operation '{}'".format(request.postpath))

      return self._transports[name]


   def _setStandardHeaders(self, request):
      """
      Set standard HTTP response headers.
      """
      origin = request.getHeader("origin")
      if origin is None or origin == "null":
         origin = "*"
      request.setHeader('access-control-allow-origin', origin)
      request.setHeader('access-control-allow-credentials', 'true')
      request.setHeader('cache-control', 'no-store, no-cache, must-revalidate, max-age=0')

      headers = request.getHeader('access-control-request-headers')
      if headers is not None:
         request.setHeader('access-control-allow-headers', headers)


   def _failRequest(self, request, msg):
      """
      Fails a request to the long-poll service.
      """
      self._setStandardHeaders(request)
      request.setHeader('content-type', 'text/plain; charset=UTF-8')
      request.setResponseCode(http.BAD_REQUEST)
      return msg


   def getNotice(self, peer, redirectUrl = None, redirectAfter = 0):
      """
      Render a user notice (HTML page) when the Long-Poll root resource
      is accessed via HTTP/GET (by a user).

      :param redirectUrl: Optional URL to redirect the user to.
      :type redirectUrl: str
      :param redirectAfter: When `redirectUrl` is provided, redirect after this time (seconds).
      :type redirectAfter: int
      """
      from autobahn import __version__

      if redirectUrl:
         redirect = """<meta http-equiv="refresh" content="%d;URL='%s'">""" % (redirectAfter, redirectUrl)
      else:
         redirect = ""
      html = """
<!DOCTYPE html>
<html>
   <head>
      %s
      <style>
         body {
            color: #fff;
            background-color: #027eae;
            font-family: "Segoe UI", "Lucida Grande", "Helvetica Neue", Helvetica, Arial, sans-serif;
            font-size: 16px;
         }

         a, a:visited, a:hover {
            color: #fff;
         }
      </style>
   </head>
   <body>
      <h1>AutobahnPython %s</h1>
      <p>
         I am not Web server, but a <b>WAMP-over-LongPoll</b> transport endpoint.
      </p>
      <p>
         You can talk to me using the <a href="https://github.com/tavendo/WAMP/blob/master/spec/advanced.md#long-poll-transport">WAMP-over-LongPoll</a> protocol.
      </p>
      <p>
         For more information, please see:
         <ul>
            <li><a href="http://wamp.ws/">WAMP</a></li>
            <li><a href="http://autobahn.ws/python">AutobahnPython</a></li>
         </ul>
      </p>
   </body>
</html>
""" % (redirect, __version__)
      return html
