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

import re
import six

# TODO:
# suffix matches
# args + kwargs
# uuid converter
# multiple URI patterns per decorated object
# classes: Pattern, EndpointPattern, ..

class Pattern:
   """
   A WAMP URI Pattern.
   """

   URI_TARGET_ENDPOINT = 1
   URI_TARGET_HANDLER = 2
   URI_TARGET_EXCEPTION = 3

   URI_TYPE_EXACT = 1
   URI_TYPE_PREFIX = 2
   URI_TYPE_WILDCARD = 3

   _URI_COMPONENT = re.compile(r"^[a-z][a-z0-9_]*$")
   _URI_NAMED_COMPONENT = re.compile(r"^<([a-z][a-z0-9_]*)>$")
   _URI_NAMED_CONVERTED_COMPONENT = re.compile(r"^<([a-z][a-z0-9_]*):([a-z]*)>$")


   def __init__(self, uri, target):
      """
      Constructor for WAMP URI pattern.

      :param uri: The URI or URI pattern, e.g. `"com.myapp.product.<product:int>.update"`.
      :type uri: str
      :param target: The target for this pattern: a procedure endpoint (a callable),
                     an event handler (a callable) or an exception (a class).
      """
      assert(type(uri) == six.text_type)
      assert(target in [Pattern.URI_TARGET_ENDPOINT,
                        Pattern.URI_TARGET_HANDLER,
                        Pattern.URI_TARGET_EXCEPTION])

      components = uri.split('.')
      pl = []
      nc = {}
      i = 0
      for component in components:

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
            continue

         match = Pattern._URI_NAMED_COMPONENT.match(component)
         if match:
            name = match.groups()[0]
            if name in nc:
               raise Exception("invalid URI")

            nc[name] = str
            pl.append("(?P<{0}>[a-z][a-z0-9_]*)".format(name))
            continue

         match = Pattern._URI_COMPONENT.match(component)
         if match:
            pl.append(component)
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


   def uri(self):
      """
      Returns the original URI (pattern) for this pattern.

      :returns: str -- The URI (pattern), e.g. `"com.myapp.product.<product:int>.update"`.
      """
      return self._uri


   def match(self, uri):
      """
      Match the given (fully qualified) URI according to this pattern
      and return extracted args and kwargs.

      :param uri: The URI to match, e.g. `"com.myapp.product.123456.update"`.
      :type uri: str

      :returns: tuple -- A tuple `(args, kwargs)`
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


   def is_endpoint(self):
      """
      Check if this pattern is for a procedure endpoint.

      :returns: bool -- `True`, iff this pattern is for a procedure endpoint.
      """
      return self._target == Pattern.URI_TARGET_ENDPOINT


   def is_handler(self):
      """
      Check if this pattern is for an event handler.

      :returns: bool -- `True`, iff this pattern is for an event handler.
      """
      return self._target == Pattern.URI_TARGET_HANDLER


   def is_exception(self):
      """
      Check if this pattern is for an exception.

      :returns: bool -- `True`, iff this pattern is for an exception.
      """
      return self._target == Pattern.URI_TARGET_EXCEPTION
