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


from __future__ import absolute_import, print_function

from autobahn.wamp.cryptosign import HAS_CRYPTOSIGN, SigningKey

from twisted.internet.defer import inlineCallbacks, returnValue

__all__ = [
    'HAS_CRYPTOSIGN_SSHAGENT'
]

if HAS_CRYPTOSIGN:
    try:
        # WAMP-cryptosign support for SSH agent is currently
        # only available on Twisted (on Python 2)
        from twisted.internet.protocol import Factory
        from twisted.internet.endpoints import UNIXClientEndpoint
        from twisted.conch.ssh.agent import SSHAgentClient
    except ImportError:
        # twisted.conch is not yet fully ported to Python 3
        HAS_CRYPTOSIGN_SSHAGENT = False
    else:
        HAS_CRYPTOSIGN_SSHAGENT = True
        __all__.append('SSHAgentSigningKey')


if HAS_CRYPTOSIGN_SSHAGENT:

    import os
    from nacl import signing
    from autobahn.wamp.cryptosign import _read_ssh_ed25519_pubkey, _unpack, _pack

    class SSHAgentSigningKey(SigningKey):
        """
        A WAMP-cryptosign signing key that is a proxy to a private Ed25510 key
        actually held in SSH agent.

        An instance of this class must be create via the class method new().
        The instance only holds the public key part, whereas the private key
        counterpart is held in SSH agent.
        """

        def __init__(self, key, comment=None, reactor=None):
            SigningKey.__init__(self, key, comment)
            if not reactor:
                from twisted.internet import reactor
            self._reactor = reactor

        @classmethod
        def new(cls, pubkey=None, reactor=None):
            """
            Create a proxy for a key held in SSH agent.

            :param pubkey: A string with a public Ed25519 key in SSH format.
            :type pubkey: unicode
            """
            if not HAS_CRYPTOSIGN_SSHAGENT:
                raise Exception("SSH agent integration is not supported on this platform")

            pubkey, _ = _read_ssh_ed25519_pubkey(pubkey)

            if not reactor:
                from twisted.internet import reactor

            if "SSH_AUTH_SOCK" not in os.environ:
                raise Exception("no ssh-agent is running!")

            factory = Factory()
            factory.noisy = False
            factory.protocol = SSHAgentClient
            endpoint = UNIXClientEndpoint(reactor, os.environ["SSH_AUTH_SOCK"])
            d = endpoint.connect(factory)

            @inlineCallbacks
            def on_connect(agent):
                keys = yield agent.requestIdentities()

                # if the key is found in ssh-agent, the raw public key (32 bytes), and the
                # key comment as returned from ssh-agent
                key_data = None
                key_comment = None

                for blob, comment in keys:
                    raw = _unpack(blob)
                    algo = raw[0]
                    if algo == u'ssh-ed25519':
                        algo, _pubkey = raw
                        if _pubkey == pubkey:
                            key_data = _pubkey
                            key_comment = comment.decode('utf8')
                            break

                agent.transport.loseConnection()

                if key_data:
                    key = signing.VerifyKey(key_data)
                    returnValue(cls(key, key_comment, reactor))
                else:
                    raise Exception("Ed25519 key not held in ssh-agent")

            return d.addCallback(on_connect)

        def sign(self, challenge):
            if "SSH_AUTH_SOCK" not in os.environ:
                raise Exception("no ssh-agent is running!")

            factory = Factory()
            factory.noisy = False
            factory.protocol = SSHAgentClient
            endpoint = UNIXClientEndpoint(self._reactor, os.environ["SSH_AUTH_SOCK"])
            d = endpoint.connect(factory)

            @inlineCallbacks
            def on_connect(agent):
                # we are now connected to the locally running ssh-agent
                # that agent might be the openssh-agent, or eg on Ubuntu 14.04 by
                # default the gnome-keyring / ssh-askpass-gnome application
                blob = _pack(['ssh-ed25519', self.public_key(binary=True)])

                # now ask the agent
                signature_blob = yield agent.signData(blob, challenge)
                algo, signature = _unpack(signature_blob)

                agent.transport.loseConnection()

                returnValue(signature)

            return d.addCallback(on_connect)
