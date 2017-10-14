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

import sys
import platform

import twisted

import autobahn

# Twisted specific utilities (these should really be in Twisted, but
# they aren't, and we use these in example code, so it must be part of
# the public API)
from autobahn.twisted.util import sleep
from autobahn.twisted.choosereactor import install_reactor

# WebSocket protocol support
from autobahn.twisted.websocket import \
    WebSocketServerProtocol, \
    WebSocketClientProtocol, \
    WebSocketServerFactory, \
    WebSocketClientFactory

# support for running Twisted stream protocols over WebSocket
from autobahn.twisted.websocket import WrappingWebSocketServerFactory, \
    WrappingWebSocketClientFactory

# Twisted Web support - FIXME: these imports trigger import of Twisted reactor!
# from autobahn.twisted.resource import WebSocketResource, WSGIRootResource

# WAMP support
from autobahn.twisted.wamp import ApplicationSession


__all__ = (
    # this should really be in Twisted
    'sleep',
    'install_reactor',

    # WebSocket
    'WebSocketServerProtocol',
    'WebSocketClientProtocol',
    'WebSocketServerFactory',
    'WebSocketClientFactory',

    # wrapping stream protocols in WebSocket
    'WrappingWebSocketServerFactory',
    'WrappingWebSocketClientFactory',

    # Twisted Web - FIXME: see comment for import above
    # 'WebSocketResource',

    # this should really be in Twisted - FIXME: see comment for import above
    # 'WSGIRootResource',

    # WAMP support
    'ApplicationSession',
)

__ident__ = u'Autobahn/{}-Twisted/{}-{}/{}'.format(autobahn.__version__, twisted.__version__, platform.python_implementation(), '.'.join([str(x) for x in list(sys.version_info[:3])]))
"""
AutobahnPython library implementation (eg. "Autobahn/0.13.0-Twisted/15.5.0-CPython/3.5.1")
"""
