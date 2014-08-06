###############################################################################
##
##  Copyright (C) 2013-2014 Tavendo GmbH
##
##  Licensed under the Apache License, Version 2.0 (the "License");
##  you may not use this file except in compliance with the License.
##  You may obtain a copy of the License at
##
##      http://www.apache.org/licenses/LICENSE-2.0
##
##  Unless required by applicable law or agreed to in writing, software
##  distributed under the License is distributed on an "AS IS" BASIS,
##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
##  See the License for the specific language governing permissions and
##  limitations under the License.
##
###############################################################################

from __future__ import absolute_import

import six

from autobahn.wamp.uri import Pattern


def register(uri):
   """
   Decorator for WAMP procedure endpoints.
   """
   def decorate(f):
      assert(callable(f))
      if not hasattr(f, '_wampuris'):
         f._wampuris = []
      f._wampuris.append(Pattern(six.u(uri), Pattern.URI_TARGET_ENDPOINT))
      return f
   return decorate


def subscribe(uri):
   """
   Decorator for WAMP event handlers.
   """
   def decorate(f):
      assert(callable(f))
      if not hasattr(f, '_wampuris'):
         f._wampuris = []
      f._wampuris.append(Pattern(six.u(uri), Pattern.URI_TARGET_HANDLER))
      return f
   return decorate


def error(uri):
   """
   Decorator for WAMP error classes.
   """
   def decorate(cls):
      assert(issubclass(cls, Exception))
      if not hasattr(cls, '_wampuris'):
         cls._wampuris = []
      cls._wampuris.append(Pattern(six.u(uri), Pattern.URI_TARGET_EXCEPTION))
      return cls
   return decorate
