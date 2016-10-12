r"""protocols is a module that contains a set of VTK Web related
protocols that can be combined together to provide a flexible way to define
very specific web application.
"""

from time import time
import os, sys, logging, types, inspect, traceback, logging, re

try:
    from vtk.vtkWebCore import vtkWebApplication, vtkWebInteractionEvent
except ImportError:
    from vtkWebCore import vtkWebApplication, vtkWebInteractionEvent
from autobahn.wamp import register as exportRpc

# =============================================================================
#
# Base class for any VTK Web based protocol
#
# =============================================================================

class vtkWebProtocol(object):

    def setApplication(self, app):
        self.Application = app

    def getApplication(self):
        return self.Application

    def mapIdToObject(self, id):
        """
        Maps global-id for a vtkObject to the vtkObject instance. May return None if the
        id is not valid.
        """
        id = int(id)
        if id <= 0:
            return None
        return self.Application.GetObjectIdMap().GetVTKObject(id)

    def getGlobalId(self, obj):
        """
        Return the id for a given vtkObject
        """
        return self.Application.GetObjectIdMap().GetGlobalId(obj)

    def getView(self, vid):
        """
        Returns the view for a given view ID, if vid is None then return the
        current active view.
        :param vid: The view ID
        :type vid: str
        """
        view = self.mapIdToObject(vid)

        if not view:
            # Use active view is none provided.
            view = self.Application.GetObjectIdMap().GetActiveObject("VIEW")
        if not view:
            raise Exception("no view provided: " + vid)

        return view

    def setActiveView(self, view):
        """
        Set a vtkRenderWindow to be the active one
        """
        self.Application.GetObjectIdMap().SetActiveObject("VIEW", view)

# =============================================================================
#
# Handle Mouse interaction on any type of view
#
# =============================================================================

class vtkWebMouseHandler(vtkWebProtocol):

    @exportRpc("viewport.mouse.interaction")
    def mouseInteraction(self, event):
        """
        RPC Callback for mouse interactions.
        """
        view = self.getView(event['view'])

        buttons = 0
        if event["buttonLeft"]:
            buttons |= vtkWebInteractionEvent.LEFT_BUTTON;
        if event["buttonMiddle"]:
            buttons |= vtkWebInteractionEvent.MIDDLE_BUTTON;
        if event["buttonRight"]:
            buttons |= vtkWebInteractionEvent.RIGHT_BUTTON;

        modifiers = 0
        if event["shiftKey"]:
            modifiers |= vtkWebInteractionEvent.SHIFT_KEY
        if event["ctrlKey"]:
            modifiers |= vtkWebInteractionEvent.CTRL_KEY
        if event["altKey"]:
            modifiers |= vtkWebInteractionEvent.ALT_KEY
        if event["metaKey"]:
            modifiers |= vtkWebInteractionEvent.META_KEY

        pvevent = vtkWebInteractionEvent()
        pvevent.SetButtons(buttons)
        pvevent.SetModifiers(modifiers)
        if event.has_key("x"):
            pvevent.SetX(event["x"])
        if event.has_key("y"):
            pvevent.SetY(event["y"])
        if event.has_key("scroll"):
            pvevent.SetScroll(event["scroll"])
        if event["action"] == 'dblclick':
            pvevent.SetRepeatCount(2)
        #pvevent.SetKeyCode(event["charCode"])
        retVal = self.getApplication().HandleInteractionEvent(view, pvevent)
        del pvevent

        if retVal:
            self.getApplication().InvokeEvent('PushRender')

        return retVal

# =============================================================================
#
# Basic 3D Viewport API (Camera + Orientation + CenterOfRotation
#
# =============================================================================

class vtkWebViewPort(vtkWebProtocol):

    @exportRpc("viewport.camera.reset")
    def resetCamera(self, viewId):
        """
        RPC callback to reset camera.
        """
        view = self.getView(viewId)
        camera = view.GetRenderer().GetActiveCamera()
        camera.ResetCamera()
        try:
            # FIXME seb: view.CenterOfRotation = camera.GetFocalPoint()
            print "FIXME"
        except:
            pass

        self.getApplication().InvalidateCache(view)
        self.getApplication().InvokeEvent('PushRender')

        return str(self.getGlobalId(view))

    @exportRpc("viewport.axes.orientation.visibility.update")
    def updateOrientationAxesVisibility(self, viewId, showAxis):
        """
        RPC callback to show/hide OrientationAxis.
        """
        view = self.getView(viewId)
        # FIXME seb: view.OrientationAxesVisibility = (showAxis if 1 else 0);

        self.getApplication().InvalidateCache(view)
        self.getApplication().InvokeEvent('PushRender')

        return str(self.getGlobalId(view))

    @exportRpc("viewport.axes.center.visibility.update")
    def updateCenterAxesVisibility(self, viewId, showAxis):
        """
        RPC callback to show/hide CenterAxesVisibility.
        """
        view = self.getView(viewId)
        # FIXME seb: view.CenterAxesVisibility = (showAxis if 1 else 0);

        self.getApplication().InvalidateCache(view)
        self.getApplication().InvokeEvent('PushRender')

        return str(self.getGlobalId(view))

    @exportRpc("viewport.camera.update")
    def updateCamera(self, view_id, focal_point, view_up, position):
        view = self.getView(view_id)

        camera = view.GetRenderer().GetActiveCamera()
        camera.SetFocalPoint(focal_point)
        camera.SetCameraViewUp(view_up)
        camera.SetCameraPosition(position)
        self.getApplication().InvalidateCache(view)

        self.getApplication().InvokeEvent('PushRender')

# =============================================================================
#
# Provide Image delivery mechanism
#
# =============================================================================

class vtkWebViewPortImageDelivery(vtkWebProtocol):

    @exportRpc("viewport.image.render")
    def stillRender(self, options):
        """
        RPC Callback to render a view and obtain the rendered image.
        """
        beginTime = int(round(time() * 1000))
        view = self.getView(options["view"])
        size = [view.GetSize()[0], view.GetSize()[1]]
        resize = size != options.get("size", size)
        if resize:
            size = options["size"]
            if size[0] > 0 and size[1] > 0:
              view.SetSize(size)
        t = 0
        if options and options.has_key("mtime"):
            t = options["mtime"]
        quality = 100
        if options and options.has_key("quality"):
            quality = options["quality"]
        localTime = 0
        if options and options.has_key("localTime"):
            localTime = options["localTime"]
        reply = {}
        app = self.getApplication()
        if t == 0:
            app.InvalidateCache(view)
        reply["image"] = app.StillRenderToString(view, t, quality)
        # Check that we are getting image size we have set if not wait until we
        # do. The render call will set the actual window size.
        tries = 10;
        while resize and list(view.GetSize()) != size \
              and size != [0, 0] and tries > 0:
            app.InvalidateCache(view)
            reply["image"] = app.StillRenderToString(view, t, quality)
            tries -= 1

        reply["stale"] = app.GetHasImagesBeingProcessed(view)
        reply["mtime"] = app.GetLastStillRenderToStringMTime()
        reply["size"] = [view.GetSize()[0], view.GetSize()[1]]
        reply["format"] = "jpeg;base64"
        reply["global_id"] = str(self.getGlobalId(view))
        reply["localTime"] = localTime

        endTime = int(round(time() * 1000))
        reply["workTime"] = (endTime - beginTime)

        return reply


# =============================================================================
#
# Provide Geometry delivery mechanism (WebGL)
#
# =============================================================================

class vtkWebViewPortGeometryDelivery(vtkWebProtocol):

    @exportRpc("viewport.webgl.metadata")
    def getSceneMetaData(self, view_id):
        view  = self.getView(view_id);
        data = self.getApplication().GetWebGLSceneMetaData(view)
        return data

    @exportRpc("viewport.webgl.data")
    def getWebGLData(self, view_id, object_id, part):
        view  = self.getView(view_id)
        data = self.getApplication().GetWebGLBinaryData(view, str(object_id), part-1)
        return data

# =============================================================================
#
# Provide File/Directory listing
#
# =============================================================================

class vtkWebFileBrowser(vtkWebProtocol):

    def __init__(self, basePath, name, excludeRegex=r"^\.|~$|^\$", groupRegex=r"[0-9]+\."):
        """
        Configure the way the WebFile browser will expose the server content.
         - basePath: specify the base directory that we should start with
         - name: Name of that base directory that will show up on the web
         - excludeRegex: Regular expression of what should be excluded from the list of files/directories
        """
        self.baseDirectory = basePath
        self.rootName = name
        self.pattern = re.compile(excludeRegex)
        self.gPattern = re.compile(groupRegex)

    @exportRpc("file.server.directory.list")
    def listServerDirectory(self, relativeDir='.'):
        """
        RPC Callback to list a server directory relative to the basePath
        provided at start-up.
        """
        path = [ self.rootName ]
        if len(relativeDir) > len(self.rootName):
            relativeDir = relativeDir[len(self.rootName)+1:]
            path += relativeDir.replace('\\','/').split('/')

        currentPath = os.path.join(self.baseDirectory, relativeDir)
        result =  { 'label': relativeDir, 'files': [], 'dirs': [], 'groups': [], 'path': path }
        if relativeDir == '.':
            result['label'] = self.rootName
        for file in os.listdir(currentPath):
            if os.path.isfile(os.path.join(currentPath, file)) and not re.search(self.pattern, file):
                result['files'].append({'label': file, 'size': -1})
            elif os.path.isdir(os.path.join(currentPath, file)) and not re.search(self.pattern, file):
                result['dirs'].append(file)

        # Filter files to create groups
        files = result['files']
        files.sort()
        groups = result['groups']
        groupIdx = {}
        filesToRemove = []
        for file in files:
            fileSplit = re.split(self.gPattern, file['label'])
            if len(fileSplit) == 2:
                filesToRemove.append(file)
                gName = '*.'.join(fileSplit)
                if groupIdx.has_key(gName):
                    groupIdx[gName]['files'].append(file['label'])
                else:
                    groupIdx[gName] = { 'files' : [file['label']], 'label': gName }
                    groups.append(groupIdx[gName])
        for file in filesToRemove:
            gName = '*.'.join(re.split(self.gPattern, file['label']))
            if len(groupIdx[gName]['files']) > 1:
                files.remove(file)
            else:
                groups.remove(groupIdx[gName])

        return result
