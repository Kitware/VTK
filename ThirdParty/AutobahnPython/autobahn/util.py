###############################################################################
##
##  Copyright (C) 2011-2014 Tavendo GmbH
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

__all__ = ("utcnow",
           "parseutc",
           "utcstr",
           "id",
           "newid",
           "rtime",
           "Stopwatch",
           "Tracker",)


import time
import random
import sys
from datetime import datetime
from pprint import pformat



def utcnow():
   """
   Get current time in UTC as ISO 8601 string.

   :returns str -- Current time as string in ISO 8601 format.
   """
   now = datetime.utcnow()
   return now.strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3] + "Z"



def utcstr(ts):
   """
   Format UTC timestamp in ISO 8601 format.

   :param ts: Timestamp.
   :type ts: instance of datetime.
   :returns str -- Timestamp formatted in ISO 8601 format.
   """
   if ts:
      return ts.strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3] + "Z"
   else:
      return ts



def parseutc(s):
   """
   Parse an ISO 8601 combined date and time string, like i.e. 2011-11-23T12:23Z
   into a UTC datetime instance.

   @deprecated: Use the iso8601 module (eg, iso8601.parse_date("2014-05-23T13:03:44.123Z"))
   """
   try:
      return datetime.strptime(s, "%Y-%m-%dT%H:%M:%SZ")
   except:
      return None



def id():
   """
   Generate a new random object ID from range [0, 2**53]. The upper bound 2**53
   is chosen since it is the maximum integer that can be represented as
   a IEEE double such that all smaller integers are representable as well.
   Hence, IDs can be safely used with languages that use IEEE double as their
   main (or only) number type (JavaScript, Lua, ..).
   """
   return random.randint(0, 9007199254740992)



def newid(len = 16):
   """
   Generate a new random object ID.
   """
   return ''.join([random.choice("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_") for i in xrange(len)])



## Select the most precise walltime measurement function available
## on the platform
##
if sys.platform.startswith('win'):
   ## On Windows, this function returns wall-clock seconds elapsed since the
   ## first call to this function, as a floating point number, based on the
   ## Win32 function QueryPerformanceCounter(). The resolution is typically
   ## better than one microsecond
   rtime = time.clock
   _ = rtime()
else:
   ## On Unix-like platforms, this used the first available from this list:
   ## (1) gettimeofday() -- resolution in microseconds
   ## (2) ftime() -- resolution in milliseconds
   ## (3) time() -- resolution in seconds
   rtime = time.time



class Stopwatch:
   """
   Stopwatch based on walltime. Can be used to do code timing and uses the
   most precise walltime measurement available on the platform. This is
   a very light-weight object, so create/dispose is very cheap.
   """

   def __init__(self, start = True):
      """
      Creates a new stopwatch and by default immediately starts (= resumes) it.
      """
      self._elapsed = 0
      if start:
         self._started = rtime()
         self._running = True
      else:
         self._started = None
         self._running = False

   def elapsed(self):
      """
      Return total time elapsed in seconds during which the stopwatch was running.
      """
      if self._running:
         now = rtime()
         return self._elapsed + (now - self._started)
      else:
         return self._elapsed

   def pause(self):
      """
      Pauses the stopwatch and returns total time elapsed in seconds during which
      the stopwatch was running.
      """
      if self._running:
         now = rtime()
         self._elapsed += now - self._started
         self._running = False
         return self._elapsed
      else:
         return self._elapsed

   def resume(self):
      """
      Resumes a paused stopwatch and returns total elapsed time in seconds
      during which the stopwatch was running.
      """
      if not self._running:
         self._started = rtime()
         self._running = True
         return self._elapsed
      else:
         now = rtime()
         return self._elapsed + (now - self._started)

   def stop(self):
      """
      Stops the stopwatch and returns total time elapsed in seconds during which
      the stopwatch was (previously) running.
      """
      elapsed = self.pause()
      self._elapsed = 0
      self._started = None
      self._running = False
      return elapsed



class Tracker:

   def __init__(self, tracker, tracked):
      """
      """
      self.tracker = tracker
      self.tracked = tracked
      self._timings = {}
      self._stopwatch = Stopwatch()


   def track(self, key):
      """
      Track elapsed for key.

      :param key: Key under which to track the timing.
      :type key: str
      """
      self._timings[key] = self._stopwatch.elapsed()


   def diff(self, startKey, endKey, format = True):
      """
      Get elapsed difference between two previously tracked keys.

      :param startKey: First key for interval (older timestamp).
      :type startKey: str
      :param endKey: Second key for interval (younger timestamp).
      :type endKey: str
      :param format: If `True`, format computed time period and return string.
      :type format: bool

      :returns: float or str -- Computed time period in seconds (or formatted string).
      """
      if endKey in self._timings and startKey in self._timings:
         d = self._timings[endKey] - self._timings[startKey]
         if format:
            if d < 0.00001: # 10us
               s = "%d ns" % round(d * 1000000000.)
            elif d < 0.01: # 10ms
               s = "%d us" % round(d * 1000000.)
            elif d < 10: # 10s
               s = "%d ms" % round(d * 1000.)
            else:
               s = "%d s" % round(d)
            return s.rjust(8)
         else:
            return d
      else:
         if format:
            return "n.a.".rjust(8)
         else:
            return None


   def __getitem__(self, key):
      return self._timings.get(key, None)


   def __iter__(self):
      return self._timings.__iter__(self)


   def __str__(self):
      return pformat(self._timings)



class EqualityMixin:

   def __eq__(self, other):
      if not isinstance(other, self.__class__):
         return False
      # we only want the actual message data attributes (not eg _serialize)
      for k in self.__dict__:
         if not k.startswith('_'):
            if not self.__dict__[k] == other.__dict__[k]:
               return False
      return True
      #return (isinstance(other, self.__class__) and self.__dict__ == other.__dict__)


   def __ne__(self, other):
      return not self.__eq__(other)
