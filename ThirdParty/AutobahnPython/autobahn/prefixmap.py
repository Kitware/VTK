###############################################################################
##
##  Copyright 2011-2013 Tavendo GmbH
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

__all__ = ("PrefixMap",)

class PrefixMap:
   """
   Provides a two-way mapping between CURIEs (Compact URI Expressions) and
   full URIs. See http://www.w3.org/TR/curie/.
   """

   def __init__(self):
      self.index = {}
      self.rindex = {}

      ## add a couple of well-know prefixes
      ##
      #self.set("owl", "http://www.w3.org/2002/07/owl#")
      #self.set("rdf", "http://www.w3.org/1999/02/22-rdf-syntax-ns#")
      #self.set("rdfs", "http://www.w3.org/2000/01/rdf-schema#")
      #self.set("rdfa", "http://www.w3.org/ns/rdfa#")
      #self.set("xhv", "http://www.w3.org/1999/xhtml/vocab#")
      #self.set("xml", "http://www.w3.org/XML/1998/namespace")
      #self.set("xsd", "http://www.w3.org/2001/XMLSchema#")


   def get(self, prefix):
      """
      Returns the URI for the prefix or None if prefix has no mapped URI.

      :param prefix: Prefix to map.
      :type prefix: str
      :returns: str -- Mapped URI for prefix or None.
      """
      return self.index.get(prefix, None)


   def set(self, prefix, uri):
      """
      Set mapping of prefix to URI.

      :param prefix: Prefix to be mapped.
      :type prefix: str
      :param uri: URI the prefix is to be mapped to.
      :type uri: str
      """
      self.index[prefix] = uri
      self.rindex[uri] = prefix


   def setDefault(self, uri):
      """
      Set default URI mapping of empty prefix (prefix of length 0).

      :param uri: URI the empty prefix to be mapped to (i.e. :label should map to uri:label).
      :type str
      """
      self.set("", uri)


   def remove(self, prefix):
      """
      Remove mapping of prefix to URI.

      :param prefix: Prefix for which mapping should be removed.
      :type str
      """
      uri = index.get(index, None)
      if uri:
         del self.index[prefix]
         del self.rindex[uri]


   def resolve(self, curie):
      """
      Resolve given CURIE to full URI.

      :param curie: CURIE (i.e. "rdf:label").
      :type curie: str
      :returns: str -- Full URI for CURIE or None.
      """
      i = curie.find(":")
      if i > 0:
         prefix = curie[:i]
         if self.index.has_key(prefix):
            return self.index[prefix] + curie[i+1:]
      return None


   def resolveOrPass(self, curieOrUri):
      """
      Resolve given CURIE/URI and return string verbatim if cannot be resolved.

      :param curieOrUri: CURIE or URI.
      :type curieOrUri: str
      :returns: str -- Full URI for CURIE or original string.
      """
      u = self.resolve(curieOrUri)
      if u:
         return u
      else:
         return curieOrUri


   def shrink(self, uri):
      """
      Shrink given URI to CURIE. If no appropriate prefix mapping is available,
      return original URI.

      :param uri: URI to shrink.
      :type uri: str
      :returns str -- CURIE or original URI.
      """
      for i in xrange(len(uri), 1, -1):
         u = uri[:i]
         p = self.rindex.get(u, None)
         if p:
            return p + ":" + uri[i:]
      return uri


if __name__ == '__main__':
   m = PrefixMap()
   print m.resolve("http://www.w3.org/1999/02/22-rdf-syntax-ns#label")
   print m.resolve("rdf:label")
   print m.resolve("foobar:label")
   print m.shrink("http://www.w3.org/1999/02/22-rdf-syntax-ns#")
   print m.shrink("http://www.w3.org/1999/02/22-rdf-syntax-ns#label")
   print m.shrink("http://foobar.org#label")
