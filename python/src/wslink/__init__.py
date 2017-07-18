r"""
Wslink allows easy, bi-directional communication between a python server and a
javascript client over a websocket.

wslink.server creates the python server
wslink.websocket handles the communication
"""
__version__ = '0.1.2'
__license__ = 'BSD-3-Clause'

from .uri import checkURI
# name is chosen to match Autobahn RPC decorator.
def register(uri):
   """
   Decorator for RPC procedure endpoints.
   """
   def decorate(f):
      # called once when method is decorated, because we return 'f'.
      assert(callable(f))
      if not hasattr(f, '_wslinkuris'):
         f._wslinkuris = []
      f._wslinkuris.append({ "uri": checkURI(uri) })
      return f
   return decorate
