r"""protocols is a module that contains a set of VTK Web related
protocols that can be combined together to provide a flexible way to define
very specific web application.
"""

from __future__ import absolute_import, division, print_function

import os, sys, logging, types, inspect, traceback, re, base64, time

from vtkmodules.vtkWebCore import vtkWebInteractionEvent

from vtkmodules.web.errors import WebDependencyMissingError
from vtkmodules.web.render_window_serializer import (
    serializeInstance,
    SynchronizationContext,
    getReferenceId,
    initializeSerializers,
)

try:
    from wslink import schedule_callback
    from wslink import register as exportRpc
    from wslink.websocket import LinkProtocol
except ImportError:
    raise WebDependencyMissingError()

# =============================================================================
#
# Base class for any VTK Web based protocol
#
# =============================================================================


class vtkWebProtocol(LinkProtocol):
    def getApplication(self):
        return self.getSharedObject("app")

    # no need for a setApplication anymore, but keep for compatibility
    def setApplication(self, app):
        pass

    def mapIdToObject(self, id):
        """
        Maps global-id for a vtkObject to the vtkObject instance. May return None if the
        id is not valid.
        """
        id = int(id)
        if id <= 0:
            return None
        return self.getApplication().GetObjectIdMap().GetVTKObject(id)

    def getGlobalId(self, obj):
        """
        Return the id for a given vtkObject
        """
        return self.getApplication().GetObjectIdMap().GetGlobalId(obj)

    def freeObject(self, obj):
        """
        Delete the given vtkObject from the objectIdMap. Returns true if delete succeeded.
        """
        return self.getApplication().GetObjectIdMap().FreeObject(obj)

    def freeObjectById(self, id):
        """
        Delete the vtkObject corresponding to the given objectId from the objectIdMap.
        Returns true if delete succeeded.
        """
        return self.getApplication().GetObjectIdMap().FreeObjectById(id)

    def getView(self, vid):
        """
        Returns the view for a given view ID, if vid is None then return the
        current active view.
        :param vid: The view ID
        :type vid: str
        """
        v = self.mapIdToObject(vid)

        if not v:
            # Use active view is none provided.
            v = self.getApplication().GetObjectIdMap().GetActiveObject("VIEW")
        if not v:
            raise Exception("no view provided: %s" % vid)

        return v

    def setActiveView(self, view):
        """
        Set a vtkRenderWindow to be the active one
        """
        self.getApplication().GetObjectIdMap().SetActiveObject("VIEW", view)


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
        view = self.getView(event["view"])

        buttons = 0
        if event["buttonLeft"]:
            buttons |= vtkWebInteractionEvent.LEFT_BUTTON
        if event["buttonMiddle"]:
            buttons |= vtkWebInteractionEvent.MIDDLE_BUTTON
        if event["buttonRight"]:
            buttons |= vtkWebInteractionEvent.RIGHT_BUTTON

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
        if "x" in event:
            pvevent.SetX(event["x"])
        if "y" in event:
            pvevent.SetY(event["y"])
        if "scroll" in event:
            pvevent.SetScroll(event["scroll"])
        if event["action"] == "dblclick":
            pvevent.SetRepeatCount(2)
        # pvevent.SetKeyCode(event["charCode"])
        retVal = self.getApplication().HandleInteractionEvent(view, pvevent)
        del pvevent

        if event["action"] == "down":
            self.getApplication().InvokeEvent("StartInteractionEvent")

        if event["action"] == "up":
            self.getApplication().InvokeEvent("EndInteractionEvent")

        if retVal:
            self.getApplication().InvokeEvent("UpdateEvent")

        return retVal

    @exportRpc("viewport.mouse.zoom.wheel")
    def updateZoomFromWheel(self, event):
        if "Start" in event["type"]:
            self.getApplication().InvokeEvent("StartInteractionEvent")

        renderWindow = self.getView(event["view"])
        if renderWindow and "spinY" in event:
            zoomFactor = 1.0 - event["spinY"] / 10.0

            camera = renderWindow.GetRenderers().GetFirstRenderer().GetActiveCamera()
            fp = camera.GetFocalPoint()
            pos = camera.GetPosition()
            delta = [fp[i] - pos[i] for i in range(3)]
            camera.Zoom(zoomFactor)

            pos2 = camera.GetPosition()
            camera.SetFocalPoint([pos2[i] + delta[i] for i in range(3)])
            renderWindow.Modified()

        if "End" in event["type"]:
            self.getApplication().InvokeEvent("EndInteractionEvent")


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
        renderer = view.GetRenderers().GetFirstRenderer()
        renderer.ResetCamera()

        self.getApplication().InvalidateCache(view)
        self.getApplication().InvokeEvent("UpdateEvent")

        return str(self.getGlobalId(view))

    @exportRpc("viewport.axes.orientation.visibility.update")
    def updateOrientationAxesVisibility(self, viewId, showAxis):
        """
        RPC callback to show/hide OrientationAxis.
        """
        view = self.getView(viewId)
        # FIXME seb: view.OrientationAxesVisibility = (showAxis if 1 else 0);

        self.getApplication().InvalidateCache(view)
        self.getApplication().InvokeEvent("UpdateEvent")

        return str(self.getGlobalId(view))

    @exportRpc("viewport.axes.center.visibility.update")
    def updateCenterAxesVisibility(self, viewId, showAxis):
        """
        RPC callback to show/hide CenterAxesVisibility.
        """
        view = self.getView(viewId)
        # FIXME seb: view.CenterAxesVisibility = (showAxis if 1 else 0);

        self.getApplication().InvalidateCache(view)
        self.getApplication().InvokeEvent("UpdateEvent")

        return str(self.getGlobalId(view))

    @exportRpc("viewport.camera.update")
    def updateCamera(self, view_id, focal_point, view_up, position, forceUpdate=True):
        view = self.getView(view_id)

        camera = view.GetRenderers().GetFirstRenderer().GetActiveCamera()
        camera.SetFocalPoint(focal_point)
        camera.SetViewUp(view_up)
        camera.SetPosition(position)

        if forceUpdate:
            self.getApplication().InvalidateCache(view)
            self.getApplication().InvokeEvent("UpdateEvent")


# =============================================================================
#
# Provide Image delivery mechanism (deprecated - will be removed in VTK 10+)
#
# =============================================================================


class vtkWebViewPortImageDelivery(vtkWebProtocol):
    @exportRpc("viewport.image.render")
    def stillRender(self, options):
        """
        RPC Callback to render a view and obtain the rendered image.
        """
        beginTime = int(round(time.time() * 1000))
        view = self.getView(options["view"])
        size = [view.GetSize()[0], view.GetSize()[1]]
        # use existing size, overridden only if options["size"] is set.
        resize = size != options.get("size", size)
        if resize:
            size = options["size"]
            if size[0] > 0 and size[1] > 0:
                view.SetSize(size)
        t = 0
        if options and "mtime" in options:
            t = options["mtime"]
        quality = 100
        if options and "quality" in options:
            quality = options["quality"]
        localTime = 0
        if options and "localTime" in options:
            localTime = options["localTime"]
        reply = {}
        app = self.getApplication()
        if t == 0:
            app.InvalidateCache(view)
        reply["image"] = app.StillRenderToString(view, t, quality)
        # Check that we are getting image size we have set. If not, wait until we
        # do. The render call will set the actual window size.
        tries = 10
        while resize and list(view.GetSize()) != size and size != [0, 0] and tries > 0:
            app.InvalidateCache(view)
            reply["image"] = app.StillRenderToString(view, t, quality)
            tries -= 1

        reply["stale"] = app.GetHasImagesBeingProcessed(view)
        reply["mtime"] = app.GetLastStillRenderToMTime()
        reply["size"] = [view.GetSize()[0], view.GetSize()[1]]
        reply["format"] = "jpeg;base64"
        reply["global_id"] = str(self.getGlobalId(view))
        reply["localTime"] = localTime

        endTime = int(round(time.time() * 1000))
        reply["workTime"] = endTime - beginTime

        return reply


# =============================================================================
#
# Provide publish-based Image delivery mechanism
#
# =============================================================================


class vtkWebPublishImageDelivery(vtkWebProtocol):
    def __init__(self, decode=True):
        super(vtkWebPublishImageDelivery, self).__init__()
        self.trackingViews = {}
        self.lastStaleTime = 0
        self.staleHandlerCount = 0
        self.deltaStaleTimeBeforeRender = 0.5  # 0.5s
        self.decode = decode
        self.viewsInAnimations = []
        self.targetFrameRate = 30.0
        self.minFrameRate = 12.0
        self.maxFrameRate = 30.0

    def pushRender(self, vId, ignoreAnimation=False):
        if vId not in self.trackingViews:
            return

        if not self.trackingViews[vId]["enabled"]:
            return

        if not ignoreAnimation and len(self.viewsInAnimations) > 0:
            return

        if "originalSize" not in self.trackingViews[vId]:
            view = self.getView(vId)
            self.trackingViews[vId]["originalSize"] = list(view.GetSize())

        if "ratio" not in self.trackingViews[vId]:
            self.trackingViews[vId]["ratio"] = 1

        ratio = self.trackingViews[vId]["ratio"]
        mtime = self.trackingViews[vId]["mtime"]
        quality = self.trackingViews[vId]["quality"]
        size = [int(s * ratio) for s in self.trackingViews[vId]["originalSize"]]

        reply = self.stillRender(
            {"view": vId, "mtime": mtime, "quality": quality, "size": size}
        )
        stale = reply["stale"]
        if reply["image"]:
            # depending on whether the app has encoding enabled:
            if self.decode:
                reply["image"] = base64.standard_b64decode(reply["image"])

            reply["image"] = self.addAttachment(reply["image"])
            reply["format"] = "jpeg"
            # save mtime for next call.
            self.trackingViews[vId]["mtime"] = reply["mtime"]
            # echo back real ID, instead of -1 for 'active'
            reply["id"] = vId
            self.publish("viewport.image.push.subscription", reply)
        if stale:
            self.lastStaleTime = time.time()
            if self.staleHandlerCount == 0:
                self.staleHandlerCount += 1
                schedule_callback(
                    self.deltaStaleTimeBeforeRender, lambda: self.renderStaleImage(vId)
                )
        else:
            self.lastStaleTime = 0

    def renderStaleImage(self, vId):
        self.staleHandlerCount -= 1

        if self.lastStaleTime != 0:
            delta = time.time() - self.lastStaleTime
            if delta >= self.deltaStaleTimeBeforeRender:
                self.pushRender(vId)
            else:
                self.staleHandlerCount += 1
                schedule_callback(
                    self.deltaStaleTimeBeforeRender - delta + 0.001,
                    lambda: self.renderStaleImage(vId),
                )

    def animate(self):
        if len(self.viewsInAnimations) == 0:
            return

        nextAnimateTime = time.time() + 1.0 / self.targetFrameRate
        for vId in self.viewsInAnimations:
            self.pushRender(vId, True)

        nextAnimateTime -= time.time()

        if self.targetFrameRate > self.maxFrameRate:
            self.targetFrameRate = self.maxFrameRate

        if nextAnimateTime < 0:
            if nextAnimateTime < -1.0:
                self.targetFrameRate = 1
            if self.targetFrameRate > self.minFrameRate:
                self.targetFrameRate -= 1.0
            schedule_callback(0.001, lambda: self.animate())
        else:
            if self.targetFrameRate < self.maxFrameRate and nextAnimateTime > 0.005:
                self.targetFrameRate += 1.0
            schedule_callback(nextAnimateTime, lambda: self.animate())

    @exportRpc("viewport.image.animation.fps.max")
    def setMaxFrameRate(self, fps=30):
        self.maxFrameRate = fps

    @exportRpc("viewport.image.animation.fps.get")
    def getCurrentFrameRate(self):
        return self.targetFrameRate

    @exportRpc("viewport.image.animation.start")
    def startViewAnimation(self, viewId="-1"):
        sView = self.getView(viewId)
        realViewId = str(self.getGlobalId(sView))

        self.viewsInAnimations.append(realViewId)
        if len(self.viewsInAnimations) == 1:
            self.animate()

    @exportRpc("viewport.image.animation.stop")
    def stopViewAnimation(self, viewId="-1"):
        sView = self.getView(viewId)
        realViewId = str(self.getGlobalId(sView))

        if realViewId in self.viewsInAnimations:
            self.viewsInAnimations.remove(realViewId)

    @exportRpc("viewport.image.push")
    def imagePush(self, options):
        sView = self.getView(options["view"])
        realViewId = str(self.getGlobalId(sView))
        # Make sure an image is pushed
        self.getApplication().InvalidateCache(sView)
        self.pushRender(realViewId)

    # Internal function since the reply[image] is not
    # JSON(serializable) it can not be an RPC one
    def stillRender(self, options):
        """
        RPC Callback to render a view and obtain the rendered image.
        """
        beginTime = int(round(time.time() * 1000))
        view = self.getView(options["view"])
        size = view.GetSize()[0:2]
        resize = size != options.get("size", size)
        if resize:
            size = options["size"]
            if size[0] > 10 and size[1] > 10:
                view.SetSize(size)
        t = 0
        if options and "mtime" in options:
            t = options["mtime"]
        quality = 100
        if options and "quality" in options:
            quality = options["quality"]
        localTime = 0
        if options and "localTime" in options:
            localTime = options["localTime"]
        reply = {}
        app = self.getApplication()
        if t == 0:
            app.InvalidateCache(view)
        if self.decode:
            stillRender = app.StillRenderToString
        else:
            stillRender = app.StillRenderToBuffer
        reply_image = stillRender(view, t, quality)

        # Check that we are getting image size we have set if not wait until we
        # do. The render call will set the actual window size.
        tries = 10
        while resize and list(view.GetSize()) != size and size != [0, 0] and tries > 0:
            app.InvalidateCache(view)
            reply_image = stillRender(view, t, quality)
            tries -= 1

        if (
            not resize
            and options
            and ("clearCache" in options)
            and options["clearCache"]
        ):
            app.InvalidateCache(view)
            reply_image = stillRender(view, t, quality)

        reply["stale"] = app.GetHasImagesBeingProcessed(view)
        reply["mtime"] = app.GetLastStillRenderToMTime()
        reply["size"] = view.GetSize()[0:2]
        reply["memsize"] = reply_image.GetDataSize() if reply_image else 0
        reply["format"] = "jpeg;base64" if self.decode else "jpeg"
        reply["global_id"] = str(self.getGlobalId(view))
        reply["localTime"] = localTime
        if self.decode:
            reply["image"] = reply_image
        else:
            # Convert the vtkUnsignedCharArray into a bytes object, required by Autobahn websockets
            reply["image"] = memoryview(reply_image).tobytes() if reply_image else None

        endTime = int(round(time.time() * 1000))
        reply["workTime"] = endTime - beginTime

        return reply

    @exportRpc("viewport.image.push.observer.add")
    def addRenderObserver(self, viewId):
        sView = self.getView(viewId)
        if not sView:
            return {"error": "Unable to get view with id %s" % viewId}

        realViewId = str(self.getGlobalId(sView))

        if not realViewId in self.trackingViews:
            observerCallback = lambda *args, **kwargs: self.pushRender(realViewId)
            startCallback = lambda *args, **kwargs: self.startViewAnimation(realViewId)
            stopCallback = lambda *args, **kwargs: self.stopViewAnimation(realViewId)
            tag = self.getApplication().AddObserver("UpdateEvent", observerCallback)
            tagStart = self.getApplication().AddObserver(
                "StartInteractionEvent", startCallback
            )
            tagStop = self.getApplication().AddObserver(
                "EndInteractionEvent", stopCallback
            )
            # TODO do we need self.getApplication().AddObserver('ResetActiveView', resetActiveView())
            self.trackingViews[realViewId] = {
                "tags": [tag, tagStart, tagStop],
                "observerCount": 1,
                "mtime": 0,
                "enabled": True,
                "quality": 100,
            }
        else:
            # There is an observer on this view already
            self.trackingViews[realViewId]["observerCount"] += 1

        self.pushRender(realViewId)
        return {"success": True, "viewId": realViewId}

    @exportRpc("viewport.image.push.observer.remove")
    def removeRenderObserver(self, viewId):
        sView = self.getView(viewId)
        if not sView:
            return {"error": "Unable to get view with id %s" % viewId}

        realViewId = str(self.getGlobalId(sView))

        observerInfo = None
        if realViewId in self.trackingViews:
            observerInfo = self.trackingViews[realViewId]

        if not observerInfo:
            return {"error": "Unable to find subscription for view %s" % realViewId}

        observerInfo["observerCount"] -= 1

        if observerInfo["observerCount"] <= 0:
            for tag in observerInfo["tags"]:
                self.getApplication().RemoveObserver(tag)
            del self.trackingViews[realViewId]

        return {"result": "success"}

    @exportRpc("viewport.image.push.quality")
    def setViewQuality(self, viewId, quality, ratio=1):
        sView = self.getView(viewId)
        if not sView:
            return {"error": "Unable to get view with id %s" % viewId}

        realViewId = str(self.getGlobalId(sView))
        observerInfo = None
        if realViewId in self.trackingViews:
            observerInfo = self.trackingViews[realViewId]

        if not observerInfo:
            return {"error": "Unable to find subscription for view %s" % realViewId}

        observerInfo["quality"] = quality
        observerInfo["ratio"] = ratio

        # Update image size right now!
        if "originalSize" in self.trackingViews[realViewId]:
            size = [
                int(s * ratio) for s in self.trackingViews[realViewId]["originalSize"]
            ]
            if hasattr(sView, "SetSize"):
                sView.SetSize(size)
            else:
                sView.ViewSize = size

        return {"result": "success"}

    @exportRpc("viewport.image.push.original.size")
    def setViewSize(self, viewId, width, height):
        sView = self.getView(viewId)
        if not sView:
            return {"error": "Unable to get view with id %s" % viewId}

        realViewId = str(self.getGlobalId(sView))
        observerInfo = None
        if realViewId in self.trackingViews:
            observerInfo = self.trackingViews[realViewId]

        if not observerInfo:
            return {"error": "Unable to find subscription for view %s" % realViewId}

        observerInfo["originalSize"] = [width, height]

        return {"result": "success"}

    @exportRpc("viewport.image.push.enabled")
    def enableView(self, viewId, enabled):
        sView = self.getView(viewId)
        if not sView:
            return {"error": "Unable to get view with id %s" % viewId}

        realViewId = str(self.getGlobalId(sView))
        observerInfo = None
        if realViewId in self.trackingViews:
            observerInfo = self.trackingViews[realViewId]

        if not observerInfo:
            return {"error": "Unable to find subscription for view %s" % realViewId}

        observerInfo["enabled"] = enabled

        return {"result": "success"}

    @exportRpc("viewport.image.push.invalidate.cache")
    def invalidateCache(self, viewId):
        sView = self.getView(viewId)
        if not sView:
            return {"error": "Unable to get view with id %s" % viewId}

        self.getApplication().InvalidateCache(sView)
        self.getApplication().InvokeEvent("UpdateEvent")
        return {"result": "success"}


# =============================================================================
#
# Provide Geometry delivery mechanism (WebGL) (deprecated - will be removed in VTK 10+)
#
# =============================================================================


class vtkWebViewPortGeometryDelivery(vtkWebProtocol):
    @exportRpc("viewport.webgl.metadata")
    def getSceneMetaData(self, view_id):
        view = self.getView(view_id)
        data = self.getApplication().GetWebGLSceneMetaData(view)
        return data

    @exportRpc("viewport.webgl.data")
    def getWebGLData(self, view_id, object_id, part):
        view = self.getView(view_id)
        data = self.getApplication().GetWebGLBinaryData(view, str(object_id), part - 1)
        return data


# =============================================================================
#
# Provide File/Directory listing
#
# =============================================================================


class vtkWebFileBrowser(vtkWebProtocol):
    def __init__(
        self, basePath, name, excludeRegex=r"^\.|~$|^\$", groupRegex=r"[0-9]+\."
    ):
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
    def listServerDirectory(self, relativeDir="."):
        """
        RPC Callback to list a server directory relative to the basePath
        provided at start-up.
        """
        path = [self.rootName]
        if len(relativeDir) > len(self.rootName):
            relativeDir = relativeDir[len(self.rootName) + 1 :]
            path += relativeDir.replace("\\", "/").split("/")

        currentPath = os.path.join(self.baseDirectory, relativeDir)
        result = {
            "label": relativeDir,
            "files": [],
            "dirs": [],
            "groups": [],
            "path": path,
        }
        if relativeDir == ".":
            result["label"] = self.rootName
        for file in os.listdir(currentPath):
            if os.path.isfile(os.path.join(currentPath, file)) and not re.search(
                self.pattern, file
            ):
                result["files"].append({"label": file, "size": -1})
            elif os.path.isdir(os.path.join(currentPath, file)) and not re.search(
                self.pattern, file
            ):
                result["dirs"].append(file)

        # Filter files to create groups
        files = result["files"]
        files.sort()
        groups = result["groups"]
        groupIdx = {}
        filesToRemove = []
        for file in files:
            fileSplit = re.split(self.gPattern, file["label"])
            if len(fileSplit) == 2:
                filesToRemove.append(file)
                gName = "*.".join(fileSplit)
                if gName in groupIdx:
                    groupIdx[gName]["files"].append(file["label"])
                else:
                    groupIdx[gName] = {"files": [file["label"]], "label": gName}
                    groups.append(groupIdx[gName])
        for file in filesToRemove:
            gName = "*.".join(re.split(self.gPattern, file["label"]))
            if len(groupIdx[gName]["files"]) > 1:
                files.remove(file)
            else:
                groups.remove(groupIdx[gName])

        return result


# =============================================================================
#
# Provide an updated geometry delivery mechanism which better matches the
# client-side rendering capability we have in vtk.js
#
# =============================================================================


class vtkWebLocalRendering(vtkWebProtocol):
    def __init__(self, **kwargs):
        super(vtkWebLocalRendering, self).__init__()
        initializeSerializers()
        self.context = SynchronizationContext()
        self.trackingViews = {}
        self.mtime = 0

    # RpcName: getArray => viewport.geometry.array.get
    @exportRpc("viewport.geometry.array.get")
    def getArray(self, dataHash, binary=False):
        if binary:
            return self.addAttachment(self.context.getCachedDataArray(dataHash, binary))
        return self.context.getCachedDataArray(dataHash, binary)

    # RpcName: addViewObserver => viewport.geometry.view.observer.add
    @exportRpc("viewport.geometry.view.observer.add")
    def addViewObserver(self, viewId):
        sView = self.getView(viewId)
        if not sView:
            return {"error": "Unable to get view with id %s" % viewId}

        realViewId = self.getApplication().GetObjectIdMap().GetGlobalId(sView)

        def pushGeometry(newSubscription=False):
            stateToReturn = self.getViewState(realViewId, newSubscription)
            stateToReturn["mtime"] = 0 if newSubscription else self.mtime
            self.mtime += 1
            return stateToReturn

        if not realViewId in self.trackingViews:
            observerCallback = lambda *args, **kwargs: self.publish(
                "viewport.geometry.view.subscription", pushGeometry()
            )
            tag = self.getApplication().AddObserver("UpdateEvent", observerCallback)
            self.trackingViews[realViewId] = {"tags": [tag], "observerCount": 1}
        else:
            # There is an observer on this view already
            self.trackingViews[realViewId]["observerCount"] += 1

        self.publish("viewport.geometry.view.subscription", pushGeometry(True))
        return {"success": True, "viewId": realViewId}

    # RpcName: removeViewObserver => viewport.geometry.view.observer.remove
    @exportRpc("viewport.geometry.view.observer.remove")
    def removeViewObserver(self, viewId):
        sView = self.getView(viewId)
        if not sView:
            return {"error": "Unable to get view with id %s" % viewId}

        realViewId = self.getApplication().GetObjectIdMap().GetGlobalId(sView)

        observerInfo = None
        if realViewId in self.trackingViews:
            observerInfo = self.trackingViews[realViewId]

        if not observerInfo:
            return {"error": "Unable to find subscription for view %s" % realViewId}

        observerInfo["observerCount"] -= 1

        if observerInfo["observerCount"] <= 0:
            for tag in observerInfo["tags"]:
                self.getApplication().RemoveObserver(tag)
            del self.trackingViews[realViewId]

        return {"result": "success"}

    # RpcName: getViewState => viewport.geometry.view.get.state
    @exportRpc("viewport.geometry.view.get.state")
    def getViewState(self, viewId, newSubscription=False):
        sView = self.getView(viewId)
        if not sView:
            return {"error": "Unable to get view with id %s" % viewId}

        self.context.setIgnoreLastDependencies(newSubscription)

        # Get the active view and render window, use it to iterate over renderers
        renderWindow = sView
        renderer = renderWindow.GetRenderers().GetFirstRenderer()
        camera = renderer.GetActiveCamera()
        renderWindowId = self.getApplication().GetObjectIdMap().GetGlobalId(sView)
        viewInstance = serializeInstance(
            None, renderWindow, renderWindowId, self.context, 1
        )
        viewInstance["extra"] = {
            "vtkRefId": getReferenceId(renderWindow),
            "centerOfRotation": camera.GetFocalPoint(),
            "camera": getReferenceId(camera),
        }

        self.context.setIgnoreLastDependencies(False)
        self.context.checkForArraysToRelease()

        if viewInstance:
            return viewInstance

        return None
