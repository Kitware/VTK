###############################################################################
#
# The MIT License (MIT)
#
# Copyright (c) Crossbar.io Technologies GmbH
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
###############################################################################


from __future__ import absolute_import

from zope.interface import implementer

from twisted.protocols.policies import ProtocolWrapper
try:
    # noinspection PyUnresolvedReferences
    from twisted.web.error import NoResource
except ImportError:
    # starting from Twisted 12.2, NoResource has moved
    from twisted.web.resource import NoResource
from twisted.web.resource import IResource, Resource
from six import PY3

# The following triggers an import of reactor at module level!
#
from twisted.web.server import NOT_DONE_YET

__all__ = (
    'WebSocketResource',
    'WSGIRootResource',
)


class WSGIRootResource(Resource):
    """
    Root resource when you want a WSGI resource be the default serving
    resource for a Twisted Web site, but have subpaths served by
    different resources.

    This is a hack needed since
    `twisted.web.wsgi.WSGIResource <http://twistedmatrix.com/documents/current/api/twisted.web.wsgi.WSGIResource.html>`_.
    does not provide a ``putChild()`` method.

    .. seealso::

       * `Autobahn Twisted Web WSGI example <https://github.com/crossbario/autobahn-python/tree/master/examples/twisted/websocket/echo_wsgi>`_
       * `Original hack <http://blog.vrplumber.com/index.php?/archives/2426-Making-your-Twisted-resources-a-url-sub-tree-of-your-WSGI-resource....html>`_
    """

    def __init__(self, wsgiResource, children):
        """

        :param wsgiResource: The WSGI to serve as root resource.
        :type wsgiResource: Instance of `twisted.web.wsgi.WSGIResource <http://twistedmatrix.com/documents/current/api/twisted.web.wsgi.WSGIResource.html>`_.
        :param children: A dictionary with string keys constituting URL subpaths, and Twisted Web resources as values.
        :type children: dict
        """
        Resource.__init__(self)
        self._wsgiResource = wsgiResource
        self.children = children

    def getChild(self, path, request):
        request.prepath.pop()
        request.postpath.insert(0, path)
        return self._wsgiResource


@implementer(IResource)
class WebSocketResource(object):
    """
    A Twisted Web resource for WebSocket.
    """
    isLeaf = True

    def __init__(self, factory):
        """

        :param factory: An instance of :class:`autobahn.twisted.websocket.WebSocketServerFactory`.
        :type factory: obj
        """
        self._factory = factory

    # noinspection PyUnusedLocal
    def getChildWithDefault(self, name, request):
        """
        This resource cannot have children, hence this will always fail.
        """
        return NoResource("No such child resource.")

    def putChild(self, path, child):
        """
        This resource cannot have children, hence this is always ignored.
        """

    def render(self, request):
        """
        Render the resource. This will takeover the transport underlying
        the request, create a :class:`autobahn.twisted.websocket.WebSocketServerProtocol`
        and let that do any subsequent communication.
        """
        # Create Autobahn WebSocket protocol.
        #
        protocol = self._factory.buildProtocol(request.transport.getPeer())
        if not protocol:
            # If protocol creation fails, we signal "internal server error"
            request.setResponseCode(500)
            return b""

        # Take over the transport from Twisted Web
        #
        transport, request.channel.transport = request.channel.transport, None

        # Connect the transport to our protocol. Once #3204 is fixed, there
        # may be a cleaner way of doing this.
        # http://twistedmatrix.com/trac/ticket/3204
        #
        if isinstance(transport, ProtocolWrapper):
            # i.e. TLS is a wrapping protocol
            transport.wrappedProtocol = protocol
        else:
            transport.protocol = protocol
        protocol.makeConnection(transport)

        # On Twisted 16+, the transport is paused whilst the existing
        # request is served; there won't be any requests after us so
        # we can just resume this ourselves.
        # 17.1 version
        if hasattr(transport, "_networkProducer"):
            transport._networkProducer.resumeProducing()
        # 16.x version
        elif hasattr(transport, "resumeProducing"):
            transport.resumeProducing()

        # We recreate the request and forward the raw data. This is somewhat
        # silly (since Twisted Web already did the HTTP request parsing
        # which we will do a 2nd time), but it's totally non-invasive to our
        # code. Maybe improve this.
        #
        if PY3:

            data = request.method + b' ' + request.uri + b' HTTP/1.1\x0d\x0a'
            for h in request.requestHeaders.getAllRawHeaders():
                data += h[0] + b': ' + b",".join(h[1]) + b'\x0d\x0a'
            data += b"\x0d\x0a"
            data += request.content.read()

        else:
            data = "%s %s HTTP/1.1\x0d\x0a" % (request.method, request.uri)
            for h in request.requestHeaders.getAllRawHeaders():
                data += "%s: %s\x0d\x0a" % (h[0], ",".join(h[1]))
            data += "\x0d\x0a"
        protocol.dataReceived(data)

        return NOT_DONE_YET
