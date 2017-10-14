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


from __future__ import absolute_import, print_function

import six
import ssl  # XXX what Python version is this always available at?
import signal
import itertools
from functools import partial

try:
    import asyncio
except ImportError:
    # Trollius >= 0.3 was renamed to asyncio
    # noinspection PyUnresolvedReferences
    import trollius as asyncio

import txaio
txaio.use_asyncio()  # noqa

from autobahn.asyncio.websocket import WampWebSocketClientFactory
from autobahn.asyncio.rawsocket import WampRawSocketClientFactory

from autobahn.wamp import component

from autobahn.asyncio.wamp import Session
from autobahn.wamp.exception import ApplicationError


__all__ = ('Component',)


def _is_ssl_error(e):
    """
    Internal helper.
    """
    return isinstance(e, ssl.SSLError)


def _unique_list(seq):
    """
    Return a list with unique elements from sequence, preserving order.
    """
    seen = set()
    return [x for x in seq if x not in seen and not seen.add(x)]


def _create_transport_serializer(serializer_id):
    if serializer_id in [u'msgpack', u'mgspack.batched']:
        # try MsgPack WAMP serializer
        try:
            from autobahn.wamp.serializer import MsgPackSerializer
        except ImportError:
            pass
        else:
            if serializer_id == u'mgspack.batched':
                return MsgPackSerializer(batched=True)
            else:
                return MsgPackSerializer()

    if serializer_id in [u'json', u'json.batched']:
        # try JSON WAMP serializer
        try:
            from autobahn.wamp.serializer import JsonSerializer
        except ImportError:
            pass
        else:
            if serializer_id == u'json.batched':
                return JsonSerializer(batched=True)
            else:
                return JsonSerializer()

    raise RuntimeError('could not create serializer for "{}"'.format(serializer_id))


def _create_transport_serializers(transport):
    """
    Create a list of serializers to use with a WAMP protocol factory.
    """
    serializers = []
    for serializer_id in transport.serializers:
        if serializer_id == u'msgpack':
            # try MsgPack WAMP serializer
            try:
                from autobahn.wamp.serializer import MsgPackSerializer
            except ImportError:
                pass
            else:
                serializers.append(MsgPackSerializer(batched=True))
                serializers.append(MsgPackSerializer())

        elif serializer_id == u'json':
            # try JSON WAMP serializer
            try:
                from autobahn.wamp.serializer import JsonSerializer
            except ImportError:
                pass
            else:
                serializers.append(JsonSerializer(batched=True))
                serializers.append(JsonSerializer())

        else:
            raise RuntimeError(
                "Unknown serializer '{}'".format(serializer_id)
            )

    return serializers


def _camel_case_from_snake_case(snake):
    parts = snake.split('_')
    return parts[0] + ''.join([s.capitalize() for s in parts[1:]])


def _create_transport_factory(loop, transport, session_factory):
    """
    Create a WAMP-over-XXX transport factory.
    """
    if transport.type == u'websocket':
        serializers = _create_transport_serializers(transport)
        factory = WampWebSocketClientFactory(session_factory, url=transport.url, serializers=serializers)

    elif transport.type == u'rawsocket':
        serializer = _create_transport_serializer(transport.serializers[0])
        factory = WampRawSocketClientFactory(session_factory, serializer=serializer)

    else:
        assert(False), 'should not arrive here'

    # set the options one at a time so we can give user better feedback
    for k, v in transport.options.items():
        try:
            factory.setProtocolOptions(**{k: v})
        except (TypeError, KeyError):
            # this allows us to document options as snake_case
            # until everything internally is upgraded from
            # camelCase
            try:
                factory.setProtocolOptions(
                    **{_camel_case_from_snake_case(k): v}
                )
            except (TypeError, KeyError):
                raise ValueError(
                    "Unknown {} transport option: {}={}".format(transport.type, k, v)
                )
    return factory


class Component(component.Component):
    """
    A component establishes a transport and attached a session
    to a realm using the transport for communication.

    The transports a component tries to use can be configured,
    as well as the auto-reconnect strategy.
    """

    log = txaio.make_logger()

    session_factory = Session
    """
    The factory of the session we will instantiate.
    """

    def _check_native_endpoint(self, endpoint):
        if isinstance(endpoint, dict):
            if u'tls' in endpoint:
                tls = endpoint[u'tls']
                if isinstance(tls, (dict, bool)):
                    pass
                elif isinstance(tls, ssl.SSLContext):
                    pass
                else:
                    raise ValueError(
                        "'tls' configuration must be a dict, bool or "
                        "SSLContext instance"
                    )
        else:
            raise ValueError(
                "'endpoint' configuration must be a dict or IStreamClientEndpoint"
                " provider"
            )

    # async function
    def _connect_transport(self, loop, transport, session_factory):
        """
        Create and connect a WAMP-over-XXX transport.
        """
        factory = _create_transport_factory(loop, transport, session_factory)

        if transport.endpoint[u'type'] == u'tcp':

            version = transport.endpoint.get(u'version', 4)
            if version not in [4, 6]:
                raise ValueError('invalid IP version {} in client endpoint configuration'.format(version))

            host = transport.endpoint[u'host']
            if type(host) != six.text_type:
                raise ValueError('invalid type {} for host in client endpoint configuration'.format(type(host)))

            port = transport.endpoint[u'port']
            if type(port) not in six.integer_types:
                raise ValueError('invalid type {} for port in client endpoint configuration'.format(type(port)))

            timeout = transport.endpoint.get(u'timeout', 10)  # in seconds
            if type(timeout) not in six.integer_types:
                raise ValueError('invalid type {} for timeout in client endpoint configuration'.format(type(timeout)))

            tls = transport.endpoint.get(u'tls', None)
            tls_hostname = None

            # create a TLS enabled connecting TCP socket
            if tls:
                if isinstance(tls, dict):
                    for k in tls.keys():
                        if k not in [u"hostname", u"trust_root"]:
                            raise ValueError("Invalid key '{}' in 'tls' config".format(k))
                    hostname = tls.get(u'hostname', host)
                    if type(hostname) != six.text_type:
                        raise ValueError('invalid type {} for hostname in TLS client endpoint configuration'.format(hostname))
                    cert_fname = tls.get(u'trust_root', None)

                    tls_hostname = hostname
                    tls = True
                    if cert_fname is not None:
                        tls = ssl.create_default_context(
                            purpose=ssl.Purpose.SERVER_AUTH,
                            cafile=cert_fname,
                        )

                elif isinstance(tls, ssl.SSLContext):
                    # tls=<an SSLContext> is valid
                    tls_hostname = host

                elif tls in [False, True]:
                    if tls:
                        tls_hostname = host

                else:
                    raise RuntimeError('unknown type {} for "tls" configuration in transport'.format(type(tls)))

            f = loop.create_connection(
                protocol_factory=factory,
                host=host,
                port=port,
                ssl=tls,
                server_hostname=tls_hostname,
            )
            time_f = asyncio.ensure_future(asyncio.wait_for(f, timeout=timeout))
            return time_f

        elif transport.endpoint[u'type'] == u'unix':
            path = transport.endpoint[u'path']
            timeout = int(transport.endpoint.get(u'timeout', 10))  # in seconds

            f = loop.create_unix_connection(
                protocol_factory=factory,
                path=path,
            )
            time_f = asyncio.ensure_future(asyncio.wait_for(f, timeout=timeout))
            return time_f

        else:
            assert(False), 'should not arrive here'

    # async function
    def start(self, loop=None):
        """
        This starts the Component, which means it will start connecting
        (and re-connecting) to its configured transports. A Component
        runs until it is "done", which means one of:

        - There was a "main" function defined, and it completed successfully;
        - Something called ``.leave()`` on our session, and we left successfully;
        - ``.stop()`` was called, and completed successfully;
        - none of our transports were able to connect successfully (failure);

        :returns: a Future which will resolve (to ``None``) when we are
            "done" or with an error if something went wrong.
        """

        if loop is None:
            self.log.warn("Using default loop")
            loop = asyncio.get_default_loop()

        # this future will be returned, and thus has the semantics
        # specified in the docstring.
        done_f = txaio.create_future()

        # transports to try again and again ..
        transport_gen = itertools.cycle(self._transports)

        # issue our first event, then start the reconnect loop
        f0 = self.fire('start', loop, self)

        # this is a 1-element list so we can set it from closures in
        # this function
        reconnect = [True]

        def one_reconnect_loop(_):
            self.log.debug('Entering re-connect loop')
            if not reconnect[0]:
                return

            # cycle through all transports forever ..
            transport = next(transport_gen)

            # only actually try to connect using the transport,
            # if the transport hasn't reached max. connect count
            if transport.can_reconnect():
                delay = transport.next_delay()
                self.log.debug(
                    'trying transport {transport_idx} using connect delay {transport_delay}',
                    transport_idx=transport.idx,
                    transport_delay=delay,
                )

                delay_f = asyncio.ensure_future(txaio.sleep(delay))

                def actual_connect(_):
                    f = self._connect_once(loop, transport)

                    def session_done(x):
                        txaio.resolve(done_f, None)

                    def connect_error(fail):
                        if isinstance(fail.value, asyncio.CancelledError):
                            reconnect[0] = False
                            txaio.reject(done_f, fail)
                            return

                        self.log.debug(u'component failed: {error}', error=txaio.failure_message(fail))
                        self.log.debug(u'{tb}', tb=txaio.failure_format_traceback(fail))
                        # If this is a "fatal error" that will never work,
                        # we bail out now
                        if isinstance(fail.value, ApplicationError):
                            if fail.value.error in [u'wamp.error.no_such_realm']:
                                reconnect[0] = False
                                self.log.error(u"Fatal error, not reconnecting")
                                txaio.reject(done_f, fail)
                                return

                            self.log.error(u"{msg}", msg=fail.value.error_message())
                            return one_reconnect_loop(None)

                        elif isinstance(fail.value, OSError):
                            # failed to connect entirely, like nobody
                            # listening etc.
                            self.log.info(u"Connection failed: {msg}", msg=txaio.failure_message(fail))
                            return one_reconnect_loop(None)

                        elif _is_ssl_error(fail.value):
                            # Quoting pyOpenSSL docs: "Whenever
                            # [SSL.Error] is raised directly, it has a
                            # list of error messages from the OpenSSL
                            # error queue, where each item is a tuple
                            # (lib, function, reason). Here lib, function
                            # and reason are all strings, describing where
                            # and what the problem is. See err(3) for more
                            # information."
                            self.log.error(u"TLS failure: {reason}", reason=fail.value.args[1])
                            self.log.error(u"Marking this transport as failed")
                            transport.failed()
                        else:
                            self.log.error(
                                u'Connection failed: {error}',
                                error=txaio.failure_message(fail),
                            )
                            # some types of errors should probably have
                            # stacktraces logged immediately at error
                            # level, e.g. SyntaxError?
                            self.log.debug(u'{tb}', tb=txaio.failure_format_traceback(fail))
                            return one_reconnect_loop(None)

                    txaio.add_callbacks(f, session_done, connect_error)

            txaio.add_callbacks(delay_f, actual_connect, error)

            if False:
                # check if there is any transport left we can use
                # to connect
                if not self._can_reconnect():
                    self.log.info("No remaining transports to try")
                    reconnect[0] = False

        def error(fail):
            self.log.info("Internal error {msg}", msg=txaio.failure_message(fail))
            self.log.debug("{tb}", tb=txaio.failure_format_traceback(fail))
            txaio.reject(done_f, fail)

        txaio.add_callbacks(f0, one_reconnect_loop, error)
        return done_f

    def stop(self):
        return self._session.leave()


def run(components, log_level='info'):
    """
    High-level API to run a series of components.

    This will only return once all the components have stopped
    (including, possibly, after all re-connections have failed if you
    have re-connections enabled). Under the hood, this calls

    XXX fixme for asyncio

    -- if you wish to manage the loop loop yourself, use the
    :meth:`autobahn.asyncio.component.Component.start` method to start
    each component yourself.

    :param components: the Component(s) you wish to run
    :type components: Component or list of Components

    :param log_level: a valid log-level (or None to avoid calling start_logging)
    :type log_level: string
    """

    # actually, should we even let people "not start" the logging? I'm
    # not sure that's wise... (double-check: if they already called
    # txaio.start_logging() what happens if we call it again?)
    if log_level is not None:
        txaio.start_logging(level=log_level)
    loop = asyncio.get_event_loop()
    log = txaio.make_logger()

    # see https://github.com/python/asyncio/issues/341 asyncio has
    # "odd" handling of KeyboardInterrupt when using Tasks (as
    # run_until_complete does). Another option is to just resture
    # default SIGINT handling, which is to exit:
    #   import signal
    #   signal.signal(signal.SIGINT, signal.SIG_DFL)

    @asyncio.coroutine
    def exit():
        return loop.stop()

    def nicely_exit(signal):
        log.info("Shutting down due to {signal}", signal=signal)
        for task in asyncio.Task.all_tasks():
            task.cancel()
        asyncio.ensure_future(exit())

    loop.add_signal_handler(signal.SIGINT, partial(nicely_exit, 'SIGINT'))
    loop.add_signal_handler(signal.SIGTERM, partial(nicely_exit, 'SIGTERM'))

    # returns a future; could run_until_complete() but see below
    component._run(loop, components)

    try:
        loop.run_forever()
        # this is probably more-correct, but then you always get
        # "Event loop stopped before Future completed":
        # loop.run_until_complete(f)
    except asyncio.CancelledError:
        pass
    # finally:
    #     signal.signal(signal.SIGINT, signal.SIG_DFL)
    #     signal.signal(signal.SIGTERM, signal.SIG_DFL)
