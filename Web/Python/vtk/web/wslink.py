r"""wslink is a module that extends any
wslink related classes for the purposes of vtkWeb.

"""

from __future__ import absolute_import, division, print_function

# import inspect, types, string, random, logging, six, json, re, base64
import json, base64, time

from twisted.python         import log
from twisted.internet       import reactor

from autobahn.twisted.websocket import WebSocketServerProtocol

from wslink import websocket
from wslink import register as exportRpc

from vtk.web import protocols
from vtk.vtkWebCore import vtkWebApplication

# =============================================================================
application = None

# =============================================================================
#
# Base class for vtkWeb ServerProtocol
#
# =============================================================================

class ServerProtocol(websocket.ServerProtocol):
    """
    Defines the core server protocol for vtkWeb. Adds support to
    marshall/unmarshall RPC callbacks that involve ServerManager proxies as
    arguments or return values.

    Applications typically don't use this class directly, but instead
    sub-class it and call self.registerVtkWebProtocol() with useful vtkWebProtocols.
    """

    def __init__(self):
        log.msg('Creating SP')
        self.setSharedObject("app", self.initApplication())
        websocket.ServerProtocol.__init__(self)

    def initApplication(self):
        """
        Let subclass optionally initialize a custom application in lieu
        of the default vtkWebApplication.
        """
        global application
        if not application:
            application = vtkWebApplication()
        return application

    def setApplication(self, application):
        self.setSharedObject("app", application)

    def getApplication(self):
        return self.getSharedObject("app")

    def registerVtkWebProtocol(self, protocol):
        self.registerLinkProtocol(protocol)

    def getVtkWebProtocols(self):
        return self.getLinkProtocols()
