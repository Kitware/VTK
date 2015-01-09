###############################################################################
##
##  Copyright (C) 2013 Tavendo GmbH
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

__all__ = ['install_optimal_reactor','install_reactor']


def install_optimal_reactor():
   """
   Try to install the optimal Twisted reactor for platform.
   """
   import sys

   if 'bsd' in sys.platform or sys.platform.startswith('darwin'):
      try:
         v = sys.version_info
         if v[0] == 1 or (v[0] == 2 and v[1] < 6) or (v[0] == 2 and v[1] == 6 and v[2] < 5):
            raise Exception("Python version too old (%s)" % sys.version)
         from twisted.internet import kqreactor
         kqreactor.install()
      except Exception as e:
         print("""
   WARNING: Running on BSD or Darwin, but cannot use kqueue Twisted reactor.

    => %s

   To use the kqueue Twisted reactor, you will need:

     1. Python >= 2.6.5 or PyPy > 1.8
     2. Twisted > 12.0

   Note the use of >= and >.

   Will let Twisted choose a default reactor (potential performance degradation).
   """ % str(e))
         pass

   if sys.platform in ['win32']:
      try:
         from twisted.application.reactors import installReactor
         installReactor("iocp")
      except Exception as e:
         print("""
   WARNING: Running on Windows, but cannot use IOCP Twisted reactor.

    => %s

   Will let Twisted choose a default reactor (potential performance degradation).
   """ % str(e))

   if sys.platform.startswith('linux'):
      try:
         from twisted.internet import epollreactor
         epollreactor.install()
      except Exception as e:
         print("""
   WARNING: Running on Linux, but cannot use Epoll Twisted reactor.

    => %s

   Will let Twisted choose a default reactor (potential performance degradation).
   """ % str(e))



def install_reactor(explicitReactor = None, verbose = False):
   """
   Install Twisted reactor.

   :param explicitReactor: If provided, install this reactor. Else, install optimal reactor.
   :type explicitReactor: obj
   :param verbose: If `True`, print what happens.
   :type verbose: bool
   """
   import sys

   if explicitReactor:
      ## install explicitly given reactor
      ##
      from twisted.application.reactors import installReactor
      print("Trying to install explicitly specified Twisted reactor '%s'" % explicitReactor)
      try:
         installReactor(explicitReactor)
      except Exception as e:
         print("Could not install Twisted reactor %s%s" % (explicitReactor, ' ["%s"]' % e if verbose else ''))
         sys.exit(1)
   else:
      ## automatically choose optimal reactor
      ##
      if verbose:
         print("Automatically choosing optimal Twisted reactor")
      install_optimal_reactor()

   ## now the reactor is installed, import it
   from twisted.internet import reactor

   if verbose:
      from twisted.python.reflect import qual
      print("Running Twisted reactor %s" % qual(reactor.__class__))

   return reactor
