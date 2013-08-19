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

__all__ = ("utcnow",
           "parseutc",
           "utcstr",
           "newid",
           "rtime",
           "Stopwatch",)

import datetime
import time
import random
import sys

UTC_TIMESTAMP_FORMAT = "%Y-%m-%dT%H:%M:%SZ"


def utcnow():
   """
   Get current time in UTC as ISO 8601 string.
   """
   now = datetime.datetime.utcnow()
   return now.strftime(UTC_TIMESTAMP_FORMAT)


def parseutc(s):
   """
   Parse an ISO 8601 combined date and time string, like i.e. 2011-11-23T12:23Z
   into a UTC datetime instance.
   """
   try:
      return datetime.datetime.strptime(s, UTC_TIMESTAMP_FORMAT)
   except:
      return None


def utcstr(dt):
   """
   Convert an UTC datetime instance into an ISO 8601 combined date and time,
   like i.e. 2011-11-23T12:23Z
   """
   try:
      return dt.strftime(UTC_TIMESTAMP_FORMAT)
   except:
      return None


def newid():
   """
   Generate a new random object ID.
   """
   return ''.join([random.choice("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_") for i in xrange(16)])



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
