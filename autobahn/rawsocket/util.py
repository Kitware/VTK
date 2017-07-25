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

import six

from autobahn.util import public

# The Python urlparse module currently does not contain the rs/rss
# schemes, so we add those dynamically (which is a hack of course).
# Since the urllib from six.moves does not seem to expose the stuff
# we monkey patch here, we do it manually.
#
# Important: if you change this stuff (you shouldn't), make sure
# _all_ our unit tests for WS URLs succeed
#
if not six.PY3:
    # Python 2
    import urlparse
else:
    # Python 3
    from urllib import parse as urlparse

wsschemes = ["rs", "rss"]
urlparse.uses_relative.extend(wsschemes)
urlparse.uses_netloc.extend(wsschemes)
urlparse.uses_params.extend(wsschemes)
urlparse.uses_query.extend(wsschemes)
urlparse.uses_fragment.extend(wsschemes)

__all__ = (
    "create_url",
    "parse_url",
)


@public
def create_url(hostname, port=None, isSecure=False):
    """
    Create a RawSocket URL from components.

    :param hostname: RawSocket server hostname.
    :type hostname: str

    :param port: RawSocket service port or None (to select default
        ports ``80`` or ``443`` depending on ``isSecure``.
    :type port: int

    :param isSecure: Set ``True`` for secure RawSocket (``rss`` scheme).
    :type isSecure: bool

    :returns: Constructed RawSocket URL.
    :rtype: str
    """
    if port is not None:
        netloc = "%s:%d" % (hostname, port)
    else:
        if isSecure:
            netloc = u"{}:443".format(hostname)
        else:
            netloc = u"{}:80".format(hostname)
    if isSecure:
        scheme = u"rss"
    else:
        scheme = u"rs"
    return u"{}://{}".format(scheme, netloc)


@public
def parse_url(url):
    """
    Parses as RawSocket URL into it's components and returns a tuple (isSecure, host, port).

     - ``isSecure`` is a flag which is ``True`` for ``rss`` URLs.
     - ``host`` is the hostname or IP from the URL.
     - ``port`` is the port from the URL or standard port derived from
       scheme (``rs`` => ``80``, ``rss`` => ``443``).

    :param url: A valid RawSocket URL, i.e. ``rs://localhost:9000``
    :type url: str

    :returns: A tuple ``(isSecure, host, port)``.
    :rtype: tuple
    """
    parsed = urlparse.urlparse(url)

    if parsed.scheme not in ["rs", "rss"]:
        raise Exception("invalid RawSocket URL: protocol scheme '{}' is not for RawSocket".format(parsed.scheme))

    if not parsed.hostname or parsed.hostname == "":
        raise Exception("invalid RawSocket URL: missing hostname")

    if parsed.path is not None and parsed.path != "":
        raise Exception("invalid RawSocket URL: non-empty path '{}'".format(parsed.path))

    if parsed.query is not None and parsed.query != "":
        raise Exception("invalid RawSocket URL: non-empty query '{}'".format(parsed.query))

    if parsed.fragment is not None and parsed.fragment != "":
        raise Exception("invalid RawSocket URL: non-empty fragment '{}'".format(parsed.fragment))

    if parsed.port is None or parsed.port == "":
        if parsed.scheme == "rs":
            port = 80
        else:
            port = 443
    else:
        port = int(parsed.port)

    if port < 1 or port > 65535:
        raise Exception("invalid port {}".format(port))

    return parsed.scheme == "rss", parsed.hostname, port
