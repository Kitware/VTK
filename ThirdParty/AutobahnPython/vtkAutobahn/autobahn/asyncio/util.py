###############################################################################
#
# The MIT License (MIT)
#
# Copyright (c) Crossbar.io Technologies GmbH
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
###############################################################################

from __future__ import absolute_import

__all = (
    'sleep',
    'peer2str',
)


def peer2str(peer):
    if isinstance(peer, tuple):
        ip_ver = 4 if len(peer) == 2 else 6
        return u"tcp{2}:{0}:{1}".format(peer[0], peer[1], ip_ver)
    elif isinstance(peer, str):
        return u"unix:{0}".format(peer)
    else:
        return u"?:{0}".format(peer)


def get_serializers():
    from autobahn.wamp import serializer

    serializers = ['CBORSerializer', 'MsgPackSerializer', 'UBJSONSerializer', 'JsonSerializer']
    serializers = list(filter(lambda x: x, map(lambda s: getattr(serializer, s) if hasattr(serializer, s)
                                               else None, serializers)))
    return serializers
