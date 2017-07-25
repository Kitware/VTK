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

import re

import six

from autobahn.util import public
from autobahn.wamp.types import RegisterOptions, SubscribeOptions

__all__ = (
    'Pattern',
    'register',
    'subscribe',
    'error',
    'convert_starred_uri'
)


def convert_starred_uri(uri):
    """
    Convert a starred URI to a standard WAMP URI and a detected matching
    policy. A starred URI is one that may contain the character '*' used
    to mark URI wildcard components or URI prefixes. Starred URIs are
    more comfortable / intuitive to use at the user/API level, but need
    to be converted for use on the wire (WAMP protocol level).

    This function takes a possibly starred URI, detects the matching policy
    implied by stars, and returns a pair (uri, match) with any stars
    removed from the URI and the detected matching policy.

    An URI like 'com.example.topic1' (without any stars in it) is
    detected as an exact-matching URI.

    An URI like 'com.example.*' (with exactly one star at the very end)
    is detected as a prefix-matching URI on 'com.example.'.

    An URI like 'com.*.foobar.*' (with more than one star anywhere) is
    detected as a wildcard-matching URI on 'com..foobar.' (in this example,
    there are two wildcard URI components).

    Note that an URI like 'com.example.*' is always detected as
    a prefix-matching URI 'com.example.'. You cannot express a wildcard-matching
    URI 'com.example.' using the starred URI notation! A wildcard matching on
    'com.example.' is different from prefix-matching on 'com.example.' (which
    matches a strict superset of the former!). This is one reason we don't use
    starred URIs for WAMP at the protocol level.
    """
    assert(type(uri) == six.text_type)

    cnt_stars = uri.count(u'*')

    if cnt_stars == 0:
        match = u'exact'

    elif cnt_stars == 1 and uri[-1] == u'*':
        match = u'prefix'
        uri = uri[:-1]

    else:
        match = u'wildcard'
        uri = uri.replace(u'*', u'')

    return uri, match


@public
class Pattern(object):
    """
    A WAMP URI Pattern.

    .. todo::

       * suffix matches
       * args + kwargs
       * uuid converter
       * multiple URI patterns per decorated object
       * classes: Pattern, EndpointPattern, ..
    """

    URI_TARGET_ENDPOINT = 1
    URI_TARGET_HANDLER = 2
    URI_TARGET_EXCEPTION = 3

    URI_TYPE_EXACT = 1
    URI_TYPE_PREFIX = 2
    URI_TYPE_WILDCARD = 3

    _URI_COMPONENT = re.compile(r"^[a-z0-9][a-z0-9_\-]*$")
    """
    Compiled regular expression for a WAMP URI component.
    """

    _URI_NAMED_COMPONENT = re.compile(r"^<([a-z][a-z0-9_]*)>$")
    """
    Compiled regular expression for a named WAMP URI component.

    .. note::
        This pattern is stricter than a general WAMP URI component since a valid Python identifier is required.
    """

    _URI_NAMED_CONVERTED_COMPONENT = re.compile(r"^<([a-z][a-z0-9_]*):([a-z]*)>$")
    """
    Compiled regular expression for a named and type-converted WAMP URI component.

    .. note::
        This pattern is stricter than a general WAMP URI component since a valid Python identifier is required.
    """

    def __init__(self, uri, target, options=None):
        """

        :param uri: The URI or URI pattern, e.g. ``"com.myapp.product.<product:int>.update"``.
        :type uri: str

        :param target: The target for this pattern: a procedure endpoint (a callable),
           an event handler (a callable) or an exception (a class).
        :type target: callable or obj

        :param options: An optional options object
        :type options: None or RegisterOptions or SubscribeOptions
        """
        assert(type(uri) == six.text_type)
        assert(len(uri) > 0)
        assert(target in [Pattern.URI_TARGET_ENDPOINT,
                          Pattern.URI_TARGET_HANDLER,
                          Pattern.URI_TARGET_EXCEPTION])
        if target == Pattern.URI_TARGET_ENDPOINT:
            assert(options is None or type(options) == RegisterOptions)
        elif target == Pattern.URI_TARGET_HANDLER:
            assert(options is None or type(options) == SubscribeOptions)
        else:
            options = None

        components = uri.split('.')
        pl = []
        nc = {}
        group_count = 0
        for i in range(len(components)):
            component = components[i]

            match = Pattern._URI_NAMED_CONVERTED_COMPONENT.match(component)
            if match:
                ctype = match.groups()[1]
                if ctype not in ['string', 'int', 'suffix']:
                    raise Exception("invalid URI")

                if ctype == 'suffix' and i != len(components) - 1:
                    raise Exception("invalid URI")

                name = match.groups()[0]
                if name in nc:
                    raise Exception("invalid URI")

                if ctype in ['string', 'suffix']:
                    nc[name] = str
                elif ctype == 'int':
                    nc[name] = int
                else:
                    # should not arrive here
                    raise Exception("logic error")

                pl.append("(?P<{0}>[a-z0-9_]+)".format(name))
                group_count += 1
                continue

            match = Pattern._URI_NAMED_COMPONENT.match(component)
            if match:
                name = match.groups()[0]
                if name in nc:
                    raise Exception("invalid URI")

                nc[name] = str
                pl.append("(?P<{0}>[a-z0-9_]+)".format(name))
                group_count += 1
                continue

            match = Pattern._URI_COMPONENT.match(component)
            if match:
                pl.append(component)
                continue

            if component == '':
                group_count += 1
                pl.append("([a-z0-9][a-z0-9_\-]*)")
                nc[group_count] = str
                continue

            raise Exception("invalid URI")

        if nc:
            # URI pattern
            self._type = Pattern.URI_TYPE_WILDCARD
            p = "^" + "\.".join(pl) + "$"
            self._pattern = re.compile(p)
            self._names = nc
        else:
            # exact URI
            self._type = Pattern.URI_TYPE_EXACT
            self._pattern = None
            self._names = None
        self._uri = uri
        self._target = target
        self._options = options

    @public
    @property
    def options(self):
        """
        Returns the Options instance (if present) for this pattern.

        :return: None or the Options instance
        :rtype: None or RegisterOptions or SubscribeOptions
        """
        return self._options

    @public
    @property
    def uri_type(self):
        """
        Returns the URI type of this pattern

        :return:
        :rtype: Pattern.URI_TYPE_EXACT, Pattern.URI_TYPE_PREFIX or Pattern.URI_TYPE_WILDCARD
        """
        return self._type

    @public
    def uri(self):
        """
        Returns the original URI (pattern) for this pattern.

        :returns: The URI (pattern), e.g. ``"com.myapp.product.<product:int>.update"``.
        :rtype: str
        """
        return self._uri

    def match(self, uri):
        """
        Match the given (fully qualified) URI according to this pattern
        and return extracted args and kwargs.

        :param uri: The URI to match, e.g. ``"com.myapp.product.123456.update"``.
        :type uri: str

        :returns: A tuple ``(args, kwargs)``
        :rtype: tuple
        """
        args = []
        kwargs = {}
        if self._type == Pattern.URI_TYPE_EXACT:
            return args, kwargs
        elif self._type == Pattern.URI_TYPE_WILDCARD:
            match = self._pattern.match(uri)
            if match:
                for key in self._names:
                    val = match.group(key)
                    val = self._names[key](val)
                    kwargs[key] = val
                return args, kwargs
            else:
                raise Exception("no match")

    @public
    def is_endpoint(self):
        """
        Check if this pattern is for a procedure endpoint.

        :returns: ``True``, iff this pattern is for a procedure endpoint.
        :rtype: bool
        """
        return self._target == Pattern.URI_TARGET_ENDPOINT

    @public
    def is_handler(self):
        """
        Check if this pattern is for an event handler.

        :returns: ``True``, iff this pattern is for an event handler.
        :rtype: bool
        """
        return self._target == Pattern.URI_TARGET_HANDLER

    @public
    def is_exception(self):
        """
        Check if this pattern is for an exception.

        :returns: ``True``, iff this pattern is for an exception.
        :rtype: bool
        """
        return self._target == Pattern.URI_TARGET_EXCEPTION


@public
def register(uri, options=None):
    """
    Decorator for WAMP procedure endpoints.

    :param uri:
    :type uri: str

    :param options:
    :type options: None or RegisterOptions
    """
    def decorate(f):
        assert(callable(f))
        if not hasattr(f, '_wampuris'):
            f._wampuris = []
        f._wampuris.append(Pattern(uri, Pattern.URI_TARGET_ENDPOINT, options))
        return f
    return decorate


@public
def subscribe(uri, options=None):
    """
    Decorator for WAMP event handlers.

    :param uri:
    :type uri: str

    :param options:
    :type options: None or SubscribeOptions
    """
    def decorate(f):
        assert(callable(f))
        if not hasattr(f, '_wampuris'):
            f._wampuris = []
        f._wampuris.append(Pattern(uri, Pattern.URI_TARGET_HANDLER, options))
        return f
    return decorate


@public
def error(uri):
    """
    Decorator for WAMP error classes.
    """
    def decorate(cls):
        assert(issubclass(cls, Exception))
        if not hasattr(cls, '_wampuris'):
            cls._wampuris = []
        cls._wampuris.append(Pattern(uri, Pattern.URI_TARGET_EXCEPTION))
        return cls
    return decorate
