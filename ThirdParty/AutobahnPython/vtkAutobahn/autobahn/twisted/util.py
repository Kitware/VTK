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

import hashlib

from twisted.internet.defer import Deferred
from twisted.internet.address import IPv4Address, UNIXAddress
try:
    from twisted.internet.stdio import PipeAddress
except ImportError:
    # stdio.PipeAddress is only avail on Twisted 13.0+
    PipeAddress = type(None)

try:
    from twisted.internet.address import IPv6Address
    _HAS_IPV6 = True
except ImportError:
    _HAS_IPV6 = False

__all = (
    'sleep',
    'peer2str',
    'transport_channel_id'
)


def sleep(delay, reactor=None):
    """
    Inline sleep for use in co-routines (Twisted ``inlineCallback`` decorated functions).

    .. seealso::
       * `twisted.internet.defer.inlineCallbacks <http://twistedmatrix.com/documents/current/api/twisted.internet.defer.html#inlineCallbacks>`__
       * `twisted.internet.interfaces.IReactorTime <http://twistedmatrix.com/documents/current/api/twisted.internet.interfaces.IReactorTime.html>`__

    :param delay: Time to sleep in seconds.
    :type delay: float
    :param reactor: The Twisted reactor to use.
    :type reactor: None or provider of ``IReactorTime``.
    """
    if not reactor:
        from twisted.internet import reactor
    d = Deferred()
    reactor.callLater(delay, d.callback, None)
    return d


def peer2str(addr):
    """
    Convert a Twisted address as returned from ``self.transport.getPeer()`` to a string.

    :returns: Returns a string representation of the peer on a Twisted transport.
    :rtype: unicode
    """
    if isinstance(addr, IPv4Address):
        res = u"tcp4:{0}:{1}".format(addr.host, addr.port)
    elif _HAS_IPV6 and isinstance(addr, IPv6Address):
        res = u"tcp6:{0}:{1}".format(addr.host, addr.port)
    elif isinstance(addr, UNIXAddress):
        res = u"unix:{0}".format(addr.name)
    elif isinstance(addr, PipeAddress):
        res = u"<pipe>"
    else:
        # gracefully fallback if we can't map the peer's address
        res = u"?:{0}".format(addr)

    return res


def transport_channel_id(transport, is_server, channel_id_type):
    """
    Application-layer user authentication protocols are vulnerable to generic
    credential forwarding attacks, where an authentication credential sent by
    a client C to a server M may then be used by M to impersonate C at another
    server S. To prevent such credential forwarding attacks, modern authentication
    protocols rely on channel bindings. For example, WAMP-cryptosign can use
    the tls-unique channel identifier provided by the TLS layer to strongly bind
    authentication credentials to the underlying channel, so that a credential
    received on one TLS channel cannot be forwarded on another.

    """
    if channel_id_type is None:
        return None

    if channel_id_type not in [u'tls-unique']:
        raise Exception("invalid channel ID type {}".format(channel_id_type))

    if hasattr(transport, '_tlsConnection'):
        # Obtain latest TLS Finished message that we expected from peer, or None if handshake is not completed.
        # http://www.pyopenssl.org/en/stable/api/ssl.html#OpenSSL.SSL.Connection.get_peer_finished

        if is_server:
            # for routers (=servers), the channel ID is based on the TLS Finished message we
            # expected to receive from the client
            tls_finished_msg = transport._tlsConnection.get_peer_finished()
        else:
            # for clients, the channel ID is based on the TLS Finished message we sent
            # to the router (=server)
            tls_finished_msg = transport._tlsConnection.get_finished()

        m = hashlib.sha256()
        m.update(tls_finished_msg)
        return m.digest()

    else:
        return None
