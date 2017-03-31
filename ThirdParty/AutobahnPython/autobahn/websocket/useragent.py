###############################################################################
##
##  Copyright (C) 2011-2013 Tavendo GmbH
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

__all__ = ("lookupWsSupport",)


import re

UA_FIREFOX = re.compile(".*Firefox/(\d*).*")
UA_CHROME = re.compile(".*Chrome/(\d*).*")
UA_CHROMEFRAME = re.compile(".*chromeframe/(\d*).*")
UA_WEBKIT = re.compile(".*AppleWebKit/([0-9+\.]*)\w*.*")
UA_WEBOS = re.compile(".*webos/([0-9+\.]*)\w*.*")
UA_HPWEBOS = re.compile(".*hpwOS/([0-9+\.]*)\w*.*")



# Chrome =============================================================

# Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.56 Safari/535.11


# Chrome Frame =======================================================

# IE6 on Windows with Chrome Frame
# Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; chromeframe/11.0.660.0)


# Firefox ============================================================

# Windows 7 64 Bit
# Mozilla/5.0 (Windows NT 6.1; WOW64; rv:12.0a2) Gecko/20120227 Firefox/12.0a2


# Android ============================================================

# Firefox Mobile
# Mozilla/5.0 (Android; Linux armv7l; rv:10.0.2) Gecko/20120215 Firefox/10.0.2 Fennec/10.0.2

# Chrome for Android (on ICS)
# Mozilla/5.0 (Linux; U; Android-4.0.3; en-us; Galaxy Nexus Build/IML74K) AppleWebKit/535.7 (KHTML, like Gecko) CrMo/16.0.912.75 Mobile Safari/535.7

# Android builtin browser

# Samsung Galaxy Tab 1
# Mozilla/5.0 (Linux; U; Android 2.2; de-de; GT-P1000 Build/FROYO) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1

# Samsung Galaxy S
# Mozilla/5.0 (Linux; U; Android 2.3.3; de-de; GT-I9000 Build/GINGERBREAD) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1

# Samsung Galaxy Note
# Mozilla/5.0 (Linux; U; Android 2.3.6; de-de; GT-N7000 Build/GINGERBREAD) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1

# Samsung Galaxy ACE (no Flash since ARM)
# Mozilla/5.0 (Linux; U; Android 2.2.1; de-de; GT-S5830 Build/FROYO) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1


# WebOS ==============================================================

# HP Touchpad
# Mozilla/5.0 (hp-tablet; Linux; hpwOS/3.0.5; U; en-US) AppleWebKit/534.6 (KHTML, like Gecko) wOSBrowser/234.83 Safari/534.6 TouchPad/1.0
# => Qt-WebKit, Hixie-76, Flash


# Safari =============================================================

# iPod Touch, iOS 4.2.1
# Mozilla/5.0 (iPod; U; CPU iPhone OS 4_2_1 like Mac OS X; de-de) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8C148 Safari/6533.18.5
# => Hixie-76

# MacBook Pro, OSX 10.5.8, Safari 5.0.6
# Mozilla/5.0 (Macintosh; Intel Mac OS X 10_5_8) AppleWebKit/534.50.2 (KHTML, like Gecko) Version/5.0.6 Safari/533.22.3
# => Hixie-76

# RFC6455
# Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/534+ (KHTML, like Gecko) Version/5.1.2 Safari/534.52.7
# Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7_3) AppleWebKit/535.24+ (KHTML, like Gecko) Version/5.1.3 Safari/534.53.10

# Hixie-76
# Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7_3) AppleWebKit/534.53.11 (KHTML, like Gecko) Version/5.1.3 Safari/534.53.10

# Hixie-76
# Mozilla/5.0 (Macintosh; Intel Mac OS X 10_5_8) AppleWebKit/534.50.2 (KHTML, like Gecko) Version/5.0.6 Safari/533.22.3


# Opera ==============================================================

# Windows 7 32-Bit
# Opera/9.80 (Windows NT 6.1; U; de) Presto/2.10.229 Version/11.61

# Windows 7 64-Bit
# Opera/9.80 (Windows NT 6.1; WOW64; U; de) Presto/2.10.229 Version/11.62

# Samsung Galaxy S
# Opera/9.80 (Android 2.3.3; Linux; Opera Mobi/ADR-1202231246; U; de) Presto/2.10.254 Version/12.00

# Samsung Galaxy Tab 1
# Opera/9.80 (Android 2.2; Linux; Opera Tablet/ADR-1203051631; U; de) Presto/2.10.254 Version/12.00

# Samsung Galaxy ACE:
# Opera/9.80 (Android 2.2.1; Linux; Opera Mobi/ADR-1203051631; U; de) Presto/2.10.254 Version/12.00

# Nokia N8, Symbian S60 5th Ed., S60 Bell
# Opera/9.80 (S60; SymbOS; Opera Mobi/SYB-1111151949; U; de) Presto/2.9.201 Version/11.50


def _lookupWsSupport(ua):
   ## Internet Explorer
   ##
   ## FIXME: handle Windows Phone
   ##
   if ua.find("MSIE") >= 0:
      # IE10 has native support
      if ua.find("MSIE 10") >= 0:
         # native Hybi-10+
         return (True, False, True)

      # first, check for Google Chrome Frame
      # http://www.chromium.org/developers/how-tos/chrome-frame-getting-started/understanding-chrome-frame-user-agent
      if ua.find("chromeframe") >= 0:

         r = UA_CHROMEFRAME.match(ua)
         try:
            v = int(r.groups()[0])
            if v >= 14:
               # native Hybi-10+
               return (True, False, True)
         except:
            # detection problem
            return (False, False, False)

      # Flash fallback
      if ua.find("MSIE 8") >= 0 or ua.find("MSIE 9") >= 0:
         return (True, True, True)

      # unsupported
      return (False, False, True)


   ## iOS
   ##
   if ua.find("iPhone") >= 0 or ua.find("iPad") >= 0 or ua.find("iPod") >= 0:
      ## native Hixie76 (as of March 2012), no Flash, no alternative browsers
      return (True, False, True)


   ## Android
   ##
   if ua.find("Android") >= 0:

      ## Firefox Mobile
      ##
      if ua.find("Firefox") >= 0:
         # Hybi-10+ for FF Mobile 8+
         return (True, False, True)

      ## Opera Mobile
      ##
      if ua.find("Opera") >= 0:
         # Hixie76 for Opera 11+
         return (True, False, True)

      ## Chrome for Android
      ##
      if ua.find("CrMo") >= 0:
         # http://code.google.com/chrome/mobile/docs/faq.html
         return (True, False, True)

      ## Android builtin Browser (ooold WebKit)
      ##
      if ua.find("AppleWebKit") >= 0:

         # Though we return WS = True, and Flash = True here, when the device has no actual Flash support, that
         # will get later detected in JS. This applies to i.e. ARMv6 devices like Samsung Galaxy ACE

         # builtin browser, only works via Flash
         return (True, True, True)

      # detection problem
      return (False, False, False)


   ## webOS
   ##
   if ua.find("hpwOS") >= 0 or ua.find("webos") >= 0:
      try:
         if ua.find("hpwOS") >= 0:
            vv = [int(x) for x in UA_HPWEBOS.match(ua).groups()[0].split('.')]
            if vv[0] >= 3:
               return (True, False, True)
         elif ua.find("webos") >= 0:
            vv = [int(x) for x in UA_WEBOS.match(ua).groups()[0].split('.')]
            if vv[0] >= 2:
               return (True, False, True)
      except:
         # detection problem
         return (False, False, False)
      else:
         # unsupported
         return (False, False, True)


   ## Opera
   ##
   if ua.find("Opera") >= 0:
      # Opera 11+ has Hixie76 (needs to be manually activated though)
      return (True, False, True)


   ## Firefox
   ##
   if ua.find("Firefox") >= 0:
      r = UA_FIREFOX.match(ua)
      try:
         v = int(r.groups()[0])
         if v >= 7:
            # native Hybi-10+
            return (True, False, True)
         elif v >= 3:
            # works with Flash bridge
            return (True, True, True)
         else:
            # unsupported
            return (False, False, True)
      except:
         # detection problem
         return (False, False, False)


   ## Safari
   ##
   if ua.find("Safari") >= 0 and not ua.find("Chrome") >= 0:

      # rely on at least Hixie76
      return (True, False, True)


   ## Chrome
   ##
   if ua.find("Chrome") >= 0:
      r = UA_CHROME.match(ua)
      try:
         v = int(r.groups()[0])
         if v >= 14:
            # native Hybi-10+
            return (True, False, True)
         elif v >= 4:
            # works with Flash bridge
            return (True, True, True)
         else:
            # unsupported
            return (False, False, True)
      except:
         # detection problem
         return (False, False, False)


   # detection problem
   return (False, False, False)


UA_DETECT_WS_SUPPORT_DB = {}

def lookupWsSupport(ua, debug = True):
   """
   Lookup if browser supports WebSocket (Hixie76, Hybi10+, RFC6455) natively,
   and if not, whether the `web-socket-js <https://github.com/gimite/web-socket-js>`__
   Flash bridge works to polyfill that.

   Returns a tuple of booleans ``(ws_supported, needs_flash, detected)`` where

      * ``ws_supported``: WebSocket is supported
      * ``needs_flash``: Flash Bridge is needed for support
      * ``detected`` the code has explicitly mapped support

   :param ua: The browser user agent string as sent in the HTTP header, e.g. provided as `flask.request.user_agent.string` in Flask.
   :type ua: str

   :returns: tuple -- A tuple ``(ws_supported, needs_flash, detected)``.
   """
   ws = _lookupWsSupport(ua)
   if debug:
      if not ua in UA_DETECT_WS_SUPPORT_DB:
         UA_DETECT_WS_SUPPORT_DB[ua] = ws

      if not ws[2]:
         msg = "UNDETECTED"
      elif ws[0]:
         msg = "SUPPORTED"
      elif not ws[0]:
         msg = "UNSUPPORTED"
      else:
         msg = "ERROR"

      print("DETECT_WS_SUPPORT: %s %s %s %s %s" % (ua, ws[0], ws[1], ws[2], msg))

   return ws
