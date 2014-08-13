###############################################################################
##
##  Copyright (C) 2012-2013 Tavendo GmbH
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

import six


## use Cython implementation of XorMasker validator if available
##
try:
   from wsaccel.xormask import XorMaskerNull, createXorMasker

except:
   ## fallback to pure Python implementation

   ## http://stackoverflow.com/questions/15014310/python3-xrange-lack-hurts
   try:
      xrange
   except NameError:
      ## Python 3
      xrange = range

   from array import array

   class XorMaskerNull:

      # noinspection PyUnusedLocal
      def __init__(self, mask = None):
         self.ptr = 0

      def pointer(self):
         return self.ptr

      def reset(self):
         self.ptr = 0

      def process(self, data):
         self.ptr += len(data)
         return data


   class XorMaskerSimple:

      def __init__(self, mask):
         assert len(mask) == 4
         self.ptr = 0
         self.msk = array('B', mask)

      def pointer(self):
         return self.ptr

      def reset(self):
         self.ptr = 0

      def process(self, data):
         dlen = len(data)
         payload = array('B', data)
         for k in xrange(dlen):
            payload[k] ^= self.msk[self.ptr & 3]
            self.ptr += 1
         return payload.tostring()


   class XorMaskerShifted1:

      def __init__(self, mask):
         assert len(mask) == 4
         self.ptr = 0
         self.mskarray = [array('B'), array('B'), array('B'), array('B')]
         if six.PY3:
            for j in xrange(4):
               self.mskarray[0].append(mask[ j & 3])
               self.mskarray[1].append(mask[(j + 1) & 3])
               self.mskarray[2].append(mask[(j + 2) & 3])
               self.mskarray[3].append(mask[(j + 3) & 3])
         else:
            for j in xrange(4):
               self.mskarray[0].append(ord(mask[ j & 3]))
               self.mskarray[1].append(ord(mask[(j + 1) & 3]))
               self.mskarray[2].append(ord(mask[(j + 2) & 3]))
               self.mskarray[3].append(ord(mask[(j + 3) & 3]))

      def pointer(self):
         return self.ptr

      def reset(self):
         self.ptr = 0

      def process(self, data):
         dlen = len(data)
         payload = array('B', data)
         msk = self.mskarray[self.ptr & 3]
         for k in xrange(dlen):
            payload[k] ^= msk[k & 3]
         self.ptr += dlen
         return payload.tostring()


   def createXorMasker(mask, len = None):
      if len is None or len < 128:
         return XorMaskerSimple(mask)
      else:
         return XorMaskerShifted1(mask)
