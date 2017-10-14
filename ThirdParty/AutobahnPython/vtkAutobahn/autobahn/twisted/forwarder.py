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

import txaio
txaio.use_twisted()

from twisted.python import usage
from twisted.internet.defer import inlineCallbacks
from twisted.internet.protocol import Factory, Protocol
from twisted.internet.endpoints import clientFromString, serverFromString
from twisted.application import service


class DestEndpointForwardingProtocol(Protocol):

    log = txaio.make_logger()

    def connectionMade(self):
        self.log.debug("DestEndpointForwardingProtocol.connectionMade")
        pass

    def dataReceived(self, data):
        self.log.debug(
            "DestEndpointForwardingProtocol.dataReceived: {data}",
            data=data,
        )
        if self.factory._sourceProtocol:
            self.factory._sourceProtocol.transport.write(data)

    def connectionLost(self, reason):
        self.log.debug("DestEndpointForwardingProtocol.connectionLost")
        if self.factory._sourceProtocol:
            self.factory._sourceProtocol.transport.loseConnection()


class DestEndpointForwardingFactory(Factory):

    def __init__(self, sourceProtocol):
        self._sourceProtocol = sourceProtocol
        self._proto = None

    def buildProtocol(self, addr):
        self._proto = DestEndpointForwardingProtocol()
        self._proto.factory = self
        return self._proto


class EndpointForwardingProtocol(Protocol):

    log = txaio.make_logger()

    @inlineCallbacks
    def connectionMade(self):
        self.log.debug("EndpointForwardingProtocol.connectionMade")
        self._destFactory = DestEndpointForwardingFactory(self)
        self._destEndpoint = clientFromString(self.factory.service._reactor,
                                              self.factory.service._destEndpointDescriptor)
        self._destEndpointPort = yield self._destEndpoint.connect(self._destFactory)

    def dataReceived(self, data):
        self.log.debug(
            "EndpointForwardingProtocol.dataReceived: {data}",
            data=data,
        )
        if self._destFactory._proto:
            self._destFactory._proto.transport.write(data)

    def connectionLost(self, reason):
        self.log.debug("EndpointForwardingProtocol.connectionLost")
        if self._destFactory._proto:
            self._destFactory._proto.transport.loseConnection()


class EndpointForwardingService(service.Service):

    def __init__(self, endpointDescriptor, destEndpointDescriptor, reactor=None):
        if reactor is None:
            from twisted.internet import reactor
        self._reactor = reactor
        self._endpointDescriptor = endpointDescriptor
        self._destEndpointDescriptor = destEndpointDescriptor

    @inlineCallbacks
    def startService(self):
        factory = Factory.forProtocol(EndpointForwardingProtocol)
        factory.service = self
        self._endpoint = serverFromString(self._reactor, self._endpointDescriptor)
        self._endpointPort = yield self._endpoint.listen(factory)

    def stopService(self):
        return self._endpointPort.stopListening()


class Options(usage.Options):
    synopsis = "[options]"
    longdesc = 'Endpoint Forwarder.'
    optParameters = [
        ["endpoint", "e", None, "Source endpoint."],
        ["dest_endpoint", "d", None, "Destination endpoint."]
    ]


def makeService(config):
    service = EndpointForwardingService(config['endpoint'], config['dest_endpoint'])
    return service
