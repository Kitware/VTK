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

from autobahn.util import public

import six
from six.moves import urllib
# The Python urlparse module currently does not contain the ws/wss
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

wsschemes = ["ws", "wss"]
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
def create_url(hostname, port=None, isSecure=False, path=None, params=None):
    """
    Create a WebSocket URL from components.

    :param hostname: WebSocket server hostname.
    :type hostname: str

    :param port: WebSocket service port or None (to select default
        ports 80/443 depending on isSecure).
    :type port: int

    :param isSecure: Set True for secure WebSocket ("wss" scheme).
    :type isSecure: bool

    :param path: Path component of addressed resource (will be
        properly URL escaped).
    :type path: str

    :param params: A dictionary of key-values to construct the query
        component of the addressed resource (will be properly URL
        escaped).
    :type params: dict

    :returns: str -- Constructed WebSocket URL.
    """
    if port is not None:
        netloc = "%s:%d" % (hostname, port)
    else:
        if isSecure:
            netloc = "%s:443" % hostname
        else:
            netloc = "%s:80" % hostname
    if isSecure:
        scheme = "wss"
    else:
        scheme = "ws"
    if path is not None:
        ppath = urllib.parse.quote(path)
    else:
        ppath = "/"
    if params is not None:
        query = urllib.parse.urlencode(params)
    else:
        query = None
    return urllib.parse.urlunparse((scheme, netloc, ppath, None, query, None))


@public
def parse_url(url):
    """
    Parses as WebSocket URL into it's components and returns a tuple (isSecure, host, port, resource, path, params).

     - ``isSecure`` is a flag which is True for wss URLs.
     - ``host`` is the hostname or IP from the URL.
     - ``port`` is the port from the URL or standard port derived from
       scheme (ws = 80, wss = 443).
     - ``resource`` is the /resource name/ from the URL, the /path/
       together with the (optional) /query/ component.
     - ``path`` is the /path/ component properly unescaped.
     - ``params`` is the /query/ component properly unescaped and
       returned as dictionary.

    :param url: A valid WebSocket URL, i.e. ``ws://localhost:9000/myresource?param1=23&param2=456``
    :type url: str

    :returns: tuple -- A tuple (isSecure, host, port, resource, path, params)
    """
    parsed = urlparse.urlparse(url)
    if parsed.scheme not in ["ws", "wss"]:
        raise Exception("invalid WebSocket URL: protocol scheme '{}' is not for WebSocket".format(parsed.scheme))
    if not parsed.hostname or parsed.hostname == "":
        raise Exception("invalid WebSocket URL: missing hostname")
    if parsed.port is None or parsed.port == "":
        if parsed.scheme == "ws":
            port = 80
        else:
            port = 443
    else:
        port = int(parsed.port)
    if parsed.fragment is not None and parsed.fragment != "":
        raise Exception("invalid WebSocket URL: non-empty fragment '%s" % parsed.fragment)
    if parsed.path is not None and parsed.path != "":
        ppath = parsed.path
        path = urllib.parse.unquote(ppath)
    else:
        ppath = "/"
        path = ppath
    if parsed.query is not None and parsed.query != "":
        resource = ppath + "?" + parsed.query
        params = urlparse.parse_qs(parsed.query)
    else:
        resource = ppath
        params = {}
    return parsed.scheme == "wss", parsed.hostname, port, resource, path, params
