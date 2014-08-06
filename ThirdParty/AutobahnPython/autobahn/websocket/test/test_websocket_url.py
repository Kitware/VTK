###############################################################################
##
##  Copyright (C) 2014 Tavendo GmbH
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

#from twisted.trial import unittest
import unittest

from autobahn.websocket.protocol import createWsUrl, parseWsUrl



class TestCreateWsUrl(unittest.TestCase):

   def test_create_url01(self):
      self.assertEqual(createWsUrl("localhost"), "ws://localhost:80/")

   def test_create_url02(self):
      self.assertEqual(createWsUrl("localhost", port = 8090), "ws://localhost:8090/")

   def test_create_url03(self):
      self.assertEqual(createWsUrl("localhost", path = "ws"), "ws://localhost:80/ws")

   def test_create_url04(self):
      self.assertEqual(createWsUrl("localhost", path = "/ws"), "ws://localhost:80/ws")

   def test_create_url05(self):
      self.assertEqual(createWsUrl("localhost", path = "/ws/foobar"), "ws://localhost:80/ws/foobar")

   def test_create_url06(self):
      self.assertEqual(createWsUrl("localhost", isSecure = True), "wss://localhost:443/")

   def test_create_url07(self):
      self.assertEqual(createWsUrl("localhost", isSecure = True, port = 443), "wss://localhost:443/")

   def test_create_url08(self):
      self.assertEqual(createWsUrl("localhost", isSecure = True, port = 80), "wss://localhost:80/")

   def test_create_url09(self):
      self.assertEqual(createWsUrl("localhost", isSecure = True, port = 9090, path = "ws", params = {'foo': 'bar'}), "wss://localhost:9090/ws?foo=bar")

   def test_create_url10(self):
      wsurl = createWsUrl("localhost", isSecure = True, port = 9090, path = "ws", params = {'foo': 'bar', 'moo': 23})
      self.assertTrue(wsurl == "wss://localhost:9090/ws?foo=bar&moo=23" or wsurl == "wss://localhost:9090/ws?moo=23&foo=bar")

   def test_create_url11(self):
      self.assertEqual(createWsUrl("127.0.0.1", path = "ws"), "ws://127.0.0.1:80/ws")

   def test_create_url12(self):
      self.assertEqual(createWsUrl("62.146.25.34", path = "ws"), "ws://62.146.25.34:80/ws")

   def test_create_url13(self):
      self.assertEqual(createWsUrl("subsub1.sub1.something.com", path = "ws"), "ws://subsub1.sub1.something.com:80/ws")

   def test_create_url14(self):
      self.assertEqual(createWsUrl("::1", path = "ws"), "ws://::1:80/ws")

   def test_create_url15(self):
      self.assertEqual(createWsUrl("0:0:0:0:0:0:0:1", path = "ws"), "ws://0:0:0:0:0:0:0:1:80/ws")




class TestParseWsUrl(unittest.TestCase):

   # parseWsUrl -> (isSecure, host, port, resource, path, params)

   def test_parse_url01(self):
      self.assertEqual(parseWsUrl("ws://localhost"), (False, 'localhost', 80, '/', '/', {}))

   def test_parse_url02(self):
      self.assertEqual(parseWsUrl("ws://localhost:80"), (False, 'localhost', 80, '/', '/', {}))

   def test_parse_url03(self):
      self.assertEqual(parseWsUrl("wss://localhost"), (True, 'localhost', 443, '/', '/', {}))

   def test_parse_url04(self):
      self.assertEqual(parseWsUrl("wss://localhost:443"), (True, 'localhost', 443, '/', '/', {}))

   def test_parse_url05(self):
      self.assertEqual(parseWsUrl("wss://localhost/ws"), (True, 'localhost', 443, '/ws', '/ws', {}))

   def test_parse_url06(self):
      self.assertEqual(parseWsUrl("wss://localhost/ws?foo=bar"), (True, 'localhost', 443, '/ws?foo=bar', '/ws', {'foo': ['bar']}))

   def test_parse_url07(self):
      self.assertEqual(parseWsUrl("wss://localhost/ws?foo=bar&moo=23"), (True, 'localhost', 443, '/ws?foo=bar&moo=23', '/ws', {'moo': ['23'], 'foo': ['bar']}))

   def test_parse_url08(self):
      self.assertEqual(parseWsUrl("wss://localhost/ws?foo=bar&moo=23&moo=44"), (True, 'localhost', 443, '/ws?foo=bar&moo=23&moo=44', '/ws', {'moo': ['23', '44'], 'foo': ['bar']}))


   def test_parse_url09(self):
      self.assertRaises(Exception, parseWsUrl, "http://localhost")

   def test_parse_url10(self):
      self.assertRaises(Exception, parseWsUrl, "https://localhost")

   def test_parse_url11(self):
      self.assertRaises(Exception, parseWsUrl, "http://localhost:80")

   def test_parse_url12(self):
      self.assertRaises(Exception, parseWsUrl, "http://localhost#frag1")

   def test_parse_url13(self):
      self.assertRaises(Exception, parseWsUrl, "wss://")

   def test_parse_url14(self):
      self.assertRaises(Exception, parseWsUrl, "ws://")



if __name__ == '__main__':
   unittest.main()
