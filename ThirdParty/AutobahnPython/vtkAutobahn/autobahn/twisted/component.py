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

import itertools

from twisted.internet.defer import inlineCallbacks  # XXX FIXME?
from twisted.internet.interfaces import IStreamClientEndpoint
from twisted.internet.endpoints import UNIXClientEndpoint
from twisted.internet.endpoints import TCP4ClientEndpoint

try:
    _TLS = True
    from twisted.internet.endpoints import SSL4ClientEndpoint
    from twisted.internet.ssl import optionsForClientTLS, CertificateOptions, Certificate
    from twisted.internet.interfaces import IOpenSSLClientConnectionCreator
    from OpenSSL import SSL
except ImportError as e:
    _TLS = False
    if 'OpenSSL' not in str(e):
        raise

import six
import txaio

from autobahn.twisted.websocket import WampWebSocketClientFactory
from autobahn.twisted.rawsocket import WampRawSocketClientFactory

from autobahn.wamp import component

from autobahn.twisted.util import sleep
from autobahn.twisted.wamp import Session
from autobahn.wamp.exception import ApplicationError


__all__ = ('Component',)


def _is_ssl_error(e):
    """
    Internal helper.

    This is so we can just return False if we didn't import any
    TLS/SSL libraries. Otherwise, returns True if this is an
    OpenSSL.SSL.Error
    """
    if _TLS:
        return isinstance(e, SSL.Error)
    return False


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


def _create_transport_factory(reactor, transport, session_factory):
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


def _create_transport_endpoint(reactor, endpoint_config):
    """
    Create a Twisted client endpoint for a WAMP-over-XXX transport.
    """
    if IStreamClientEndpoint.providedBy(endpoint_config):
        endpoint = IStreamClientEndpoint(endpoint_config)
    else:
        # create a connecting TCP socket
        if endpoint_config[u'type'] == u'tcp':

            version = endpoint_config.get(u'version', 4)
            if version not in [4, 6]:
                raise ValueError('invalid IP version {} in client endpoint configuration'.format(version))

            host = endpoint_config[u'host']
            if type(host) != six.text_type:
                raise ValueError('invalid type {} for host in client endpoint configuration'.format(type(host)))

            port = endpoint_config[u'port']
            if type(port) not in six.integer_types:
                raise ValueError('invalid type {} for port in client endpoint configuration'.format(type(port)))

            timeout = endpoint_config.get(u'timeout', 10)  # in seconds
            if type(timeout) not in six.integer_types:
                raise ValueError('invalid type {} for timeout in client endpoint configuration'.format(type(timeout)))

            tls = endpoint_config.get(u'tls', None)

            # create a TLS enabled connecting TCP socket
            if tls:
                if not _TLS:
                    raise RuntimeError('TLS configured in transport, but TLS support is not installed (eg OpenSSL?)')

                # FIXME: create TLS context from configuration
                if IOpenSSLClientConnectionCreator.providedBy(tls):
                    # eg created from twisted.internet.ssl.optionsForClientTLS()
                    context = IOpenSSLClientConnectionCreator(tls)

                elif isinstance(tls, dict):
                    for k in tls.keys():
                        if k not in [u"hostname", u"trust_root"]:
                            raise ValueError("Invalid key '{}' in 'tls' config".format(k))
                    hostname = tls.get(u'hostname', host)
                    if type(hostname) != six.text_type:
                        raise ValueError('invalid type {} for hostname in TLS client endpoint configuration'.format(hostname))
                    trust_root = None
                    cert_fname = tls.get(u"trust_root", None)
                    if cert_fname is not None:
                        trust_root = Certificate.loadPEM(six.u(open(cert_fname, 'r').read()))
                    context = optionsForClientTLS(hostname, trustRoot=trust_root)

                elif isinstance(tls, CertificateOptions):
                    context = tls

                elif tls is True:
                    context = optionsForClientTLS(host)

                else:
                    raise RuntimeError('unknown type {} for "tls" configuration in transport'.format(type(tls)))

                if version == 4:
                    endpoint = SSL4ClientEndpoint(reactor, host, port, context, timeout=timeout)
                elif version == 6:
                    # there is no SSL6ClientEndpoint!
                    raise RuntimeError('TLS on IPv6 not implemented')
                else:
                    assert(False), 'should not arrive here'

            # create a non-TLS connecting TCP socket
            else:
                if version == 4:
                    endpoint = TCP4ClientEndpoint(reactor, host, port, timeout=timeout)
                elif version == 6:
                    try:
                        from twisted.internet.endpoints import TCP6ClientEndpoint
                    except ImportError:
                        raise RuntimeError('IPv6 is not supported (please upgrade Twisted)')
                    endpoint = TCP6ClientEndpoint(reactor, host, port, timeout=timeout)
                else:
                    assert(False), 'should not arrive here'

        # create a connecting Unix domain socket
        elif endpoint_config[u'type'] == u'unix':
            path = endpoint_config[u'path']
            timeout = int(endpoint_config.get(u'timeout', 10))  # in seconds
            endpoint = UNIXClientEndpoint(reactor, path, timeout=timeout)

        else:
            assert(False), 'should not arrive here'

    return endpoint


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
        if IStreamClientEndpoint.providedBy(endpoint):
            pass
        elif isinstance(endpoint, dict):
            if u'tls' in endpoint:
                tls = endpoint[u'tls']
                if isinstance(tls, (dict, bool)):
                    pass
                elif IOpenSSLClientConnectionCreator.providedBy(tls):
                    pass
                elif isinstance(tls, CertificateOptions):
                    pass
                else:
                    raise ValueError(
                        "'tls' configuration must be a dict, CertificateOptions or"
                        " IOpenSSLClientConnectionCreator provider"
                    )
        else:
            raise ValueError(
                "'endpoint' configuration must be a dict or IStreamClientEndpoint"
                " provider"
            )

    def _connect_transport(self, reactor, transport, session_factory):
        """
        Create and connect a WAMP-over-XXX transport.
        """
        transport_factory = _create_transport_factory(reactor, transport, session_factory)
        transport_endpoint = _create_transport_endpoint(reactor, transport.endpoint)
        return transport_endpoint.connect(transport_factory)

    # XXX think: is it okay to use inlineCallbacks (in this
    # twisted-only file) even though we're using txaio?
    @inlineCallbacks
    def start(self, reactor=None):
        """
        This starts the Component, which means it will start connecting
        (and re-connecting) to its configured transports. A Component
        runs until it is "done", which means one of:

        - There was a "main" function defined, and it completed successfully;
        - Something called ``.leave()`` on our session, and we left successfully;
        - ``.stop()`` was called, and completed successfully;
        - none of our transports were able to connect successfully (failure);

        :returns: a Deferred that fires (with ``None``) when we are
            "done" or with a Failure if something went wrong.
        """
        if reactor is None:
            self.log.warn("Using default reactor")
            from twisted.internet import reactor

        yield self.fire('start', reactor, self)

        # transports to try again and again ..
        transport_gen = itertools.cycle(self._transports)

        reconnect = True

        self.log.debug('Entering re-connect loop')

        while reconnect:
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
                yield sleep(delay)
                try:
                    yield self._connect_once(reactor, transport)
                except Exception as e:
                    f = txaio.create_failure()
                    self.log.error(u'component failed: {error}', error=txaio.failure_message(f))
                    self.log.debug(u'{tb}', tb=txaio.failure_format_traceback(f))
                    # If this is a "fatal error" that will never work,
                    # we bail out now
                    if isinstance(e, ApplicationError):
                        if e.error in [u'wamp.error.no_such_realm']:
                            reconnect = False
                            self.log.error(u"Fatal error, not reconnecting")
                            # The thinking here is that we really do
                            # want to 'raise' (and thereby fail the
                            # entire "start" / reconnect loop) because
                            # if the realm isn't valid, we're "never"
                            # going to succeed...
                            raise
                        self.log.error(u"{msg}", msg=e.error_message())
                    elif _is_ssl_error(e):
                        # Quoting pyOpenSSL docs: "Whenever
                        # [SSL.Error] is raised directly, it has a
                        # list of error messages from the OpenSSL
                        # error queue, where each item is a tuple
                        # (lib, function, reason). Here lib, function
                        # and reason are all strings, describing where
                        # and what the problem is. See err(3) for more
                        # information."
                        for (lib, fn, reason) in e.args[0]:
                            self.log.error(u"TLS failure: {reason}", reason=reason)
                        self.log.error(u"Marking this transport as failed")
                        transport.failed()
                    else:
                        f = txaio.create_failure()
                        self.log.error(
                            u'Connection failed: {error}',
                            error=txaio.failure_message(f),
                        )
                        # some types of errors should probably have
                        # stacktraces logged immediately at error
                        # level, e.g. SyntaxError?
                        self.log.debug(u'{tb}', tb=txaio.failure_format_traceback(f))
                else:
                    self.log.debug(u"Not reconnecting")
                    reconnect = False
            else:
                # check if there is any transport left we can use
                # to connect
                if not self._can_reconnect():
                    self.log.info("No remaining transports to try")
                    reconnect = False

    def stop(self):
        return self._session.leave()


def run(components, log_level='info'):
    """
    High-level API to run a series of components.

    This will only return once all the components have stopped
    (including, possibly, after all re-connections have failed if you
    have re-connections enabled). Under the hood, this calls
    :meth:`twisted.internet.reactor.run` -- if you wish to manage the
    reactor loop yourself, use the
    :meth:`autobahn.twisted.component.Component.start` method to start
    each component yourself.

    :param components: the Component(s) you wish to run
    :type components: Component or list of Components

    :param log_level: a valid log-level (or None to avoid calling start_logging)
    :type log_level: string
    """
    # only for Twisted > 12
    # ...so this isn't in all Twisted versions we test against -- need
    # to do "something else" if we can't import .. :/ (or drop some
    # support)
    from twisted.internet.task import react

    # actually, should we even let people "not start" the logging? I'm
    # not sure that's wise... (double-check: if they already called
    # txaio.start_logging() what happens if we call it again?)
    if log_level is not None:
        txaio.start_logging(level=log_level)
    react(component._run, (components, ))
