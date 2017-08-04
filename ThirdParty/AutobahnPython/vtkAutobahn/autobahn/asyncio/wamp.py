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
import signal

import six

try:
    import asyncio
except ImportError:
    # Trollius >= 0.3 was renamed to asyncio
    # noinspection PyUnresolvedReferences
    import trollius as asyncio

import txaio
txaio.use_asyncio()  # noqa

from autobahn.util import public
from autobahn.wamp import protocol
from autobahn.wamp.types import ComponentConfig

from autobahn.websocket.util import parse_url as parse_ws_url
from autobahn.rawsocket.util import parse_url as parse_rs_url

from autobahn.asyncio.websocket import WampWebSocketClientFactory
from autobahn.asyncio.rawsocket import WampRawSocketClientFactory

from autobahn.websocket.compress import PerMessageDeflateOffer, \
    PerMessageDeflateResponse, PerMessageDeflateResponseAccept

__all__ = (
    'ApplicationSession',
    'ApplicationSessionFactory',
    'ApplicationRunner'
)


@public
class ApplicationSession(protocol.ApplicationSession):
    """
    WAMP application session for asyncio-based applications.

    Implements:

        * :class:`autobahn.wamp.interfaces.ITransportHandler`
        * :class:`autobahn.wamp.interfaces.ISession`
    """

    log = txaio.make_logger()


class ApplicationSessionFactory(protocol.ApplicationSessionFactory):
    """
    WAMP application session factory for asyncio-based applications.
    """

    session = ApplicationSession
    """
    The application session class this application session factory will use.
    Defaults to :class:`autobahn.asyncio.wamp.ApplicationSession`.
    """

    log = txaio.make_logger()


@public
class ApplicationRunner(object):
    """
    This class is a convenience tool mainly for development and quick hosting
    of WAMP application components.

    It can host a WAMP application component in a WAMP-over-WebSocket client
    connecting to a WAMP router.
    """

    log = txaio.make_logger()

    def __init__(self,
                 url,
                 realm=None,
                 extra=None,
                 serializers=None,
                 ssl=None,
                 proxy=None,
                 headers=None):
        """

        :param url: The WebSocket URL of the WAMP router to connect to (e.g. `ws://somehost.com:8090/somepath`)
        :type url: str

        :param realm: The WAMP realm to join the application session to.
        :type realm: str

        :param extra: Optional extra configuration to forward to the application component.
        :type extra: dict

        :param serializers: A list of WAMP serializers to use (or None for default serializers).
           Serializers must implement :class:`autobahn.wamp.interfaces.ISerializer`.
        :type serializers: list

        :param ssl: An (optional) SSL context instance or a bool. See
           the documentation for the `loop.create_connection` asyncio
           method, to which this value is passed as the ``ssl``
           keyword parameter.
        :type ssl: :class:`ssl.SSLContext` or bool

        :param proxy: Explicit proxy server to use; a dict with ``host`` and ``port`` keys
        :type proxy: dict or None

        :param headers: Additional headers to send (only applies to WAMP-over-WebSocket).
        :type headers: dict
        """
        assert(type(url) == six.text_type)
        assert(realm is None or type(realm) == six.text_type)
        assert(extra is None or type(extra) == dict)
        assert(headers is None or type(headers) == dict)
        assert(proxy is None or type(proxy) == dict)
        self.url = url
        self.realm = realm
        self.extra = extra or dict()
        self.serializers = serializers
        self.ssl = ssl
        self.proxy = proxy
        self.headers = headers

    @public
    def stop(self):
        """
        Stop reconnecting, if auto-reconnecting was enabled.
        """
        raise NotImplementedError()

    @public
    def run(self, make, start_loop=True, log_level='info'):
        """
        Run the application component. Under the hood, this runs the event
        loop (unless `start_loop=False` is passed) so won't return
        until the program is done.

        :param make: A factory that produces instances of :class:`autobahn.asyncio.wamp.ApplicationSession`
           when called with an instance of :class:`autobahn.wamp.types.ComponentConfig`.
        :type make: callable

        :param start_loop: When ``True`` (the default) this method
            start a new asyncio loop.
        :type start_loop: bool

        :returns: None is returned, unless you specify
            `start_loop=False` in which case the coroutine from calling
            `loop.create_connection()` is returned. This will yield the
            (transport, protocol) pair.
        """
        if callable(make):
            def create():
                cfg = ComponentConfig(self.realm, self.extra)
                try:
                    session = make(cfg)
                except Exception as e:
                    self.log.error('ApplicationSession could not be instantiated: {}'.format(e))
                    loop = asyncio.get_event_loop()
                    if loop.is_running():
                        loop.stop()
                    raise
                else:
                    return session
        else:
            create = make

        if self.url.startswith(u'rs'):
            # try to parse RawSocket URL ..
            isSecure, host, port = parse_rs_url(self.url)

            # use the first configured serializer if any (which means, auto-choose "best")
            serializer = self.serializers[0] if self.serializers else None

            # create a WAMP-over-RawSocket transport client factory
            transport_factory = WampRawSocketClientFactory(create, serializer=serializer)

        else:
            # try to parse WebSocket URL ..
            isSecure, host, port, resource, path, params = parse_ws_url(self.url)

            # create a WAMP-over-WebSocket transport client factory
            transport_factory = WampWebSocketClientFactory(create, url=self.url, serializers=self.serializers, proxy=self.proxy, headers=self.headers)

            # client WebSocket settings - similar to:
            # - http://crossbar.io/docs/WebSocket-Compression/#production-settings
            # - http://crossbar.io/docs/WebSocket-Options/#production-settings

            # The permessage-deflate extensions offered to the server ..
            offers = [PerMessageDeflateOffer()]

            # Function to accept permessage_delate responses from the server ..
            def accept(response):
                if isinstance(response, PerMessageDeflateResponse):
                    return PerMessageDeflateResponseAccept(response)

            # set WebSocket options for all client connections
            transport_factory.setProtocolOptions(maxFramePayloadSize=1048576,
                                                 maxMessagePayloadSize=1048576,
                                                 autoFragmentSize=65536,
                                                 failByDrop=False,
                                                 openHandshakeTimeout=2.5,
                                                 closeHandshakeTimeout=1.,
                                                 tcpNoDelay=True,
                                                 autoPingInterval=10.,
                                                 autoPingTimeout=5.,
                                                 autoPingSize=4,
                                                 perMessageCompressionOffers=offers,
                                                 perMessageCompressionAccept=accept)
        # SSL context for client connection
        if self.ssl is None:
            ssl = isSecure
        else:
            if self.ssl and not isSecure:
                raise RuntimeError(
                    'ssl argument value passed to %s conflicts with the "ws:" '
                    'prefix of the url argument. Did you mean to use "wss:"?' %
                    self.__class__.__name__)
            ssl = self.ssl

        # start the client connection
        loop = asyncio.get_event_loop()
        txaio.use_asyncio()
        txaio.config.loop = loop
        coro = loop.create_connection(transport_factory, host, port, ssl=ssl)

        # start a asyncio loop
        if not start_loop:
            return coro
        else:
            (transport, protocol) = loop.run_until_complete(coro)

            # start logging
            txaio.start_logging(level=log_level)

            try:
                loop.add_signal_handler(signal.SIGTERM, loop.stop)
            except NotImplementedError:
                # signals are not available on Windows
                pass

            # 4) now enter the asyncio event loop
            try:
                loop.run_forever()
            except KeyboardInterrupt:
                # wait until we send Goodbye if user hit ctrl-c
                # (done outside this except so SIGTERM gets the same handling)
                pass

            # give Goodbye message a chance to go through, if we still
            # have an active session
            if protocol._session:
                loop.run_until_complete(protocol._session.leave())

            loop.close()


class Session(protocol._SessionShim):
    # XXX these methods are redundant, but put here for possibly
    # better clarity; maybe a bad idea.

    def on_join(self, details):
        pass

    def on_leave(self, details):
        self.disconnect()

    def on_connect(self):
        self.join(self.config.realm)

    def on_disconnect(self):
        pass
