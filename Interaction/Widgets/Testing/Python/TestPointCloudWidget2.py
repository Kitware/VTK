#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkFiltersSources import vtkPointSource
from vtkmodules.vtkInteractionWidgets import (
    vtkPointCloudRepresentation,
    vtkPointCloudWidget,
)
from vtkmodules.vtkRenderingCore import (
    vtkInteractorEventRecorder,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Demonstrate how to use the vtkPointCloudWidget.
# This script uses a 3D point cloud widget to select and manipulate
# a point. Make sure that you hit the "W" key to activate the widget.

# control the size of the test
npts = 10000

# These are the pre-recorded events
Recording = \
    "# StreamVersion 1.1\n\
    ExposeEvent 0 299 0 0 0 0\n\
    RenderEvent 0 299 0 0 0 0\n\
    EnterEvent 104 7 0 0 0 0\n\
    MouseMoveEvent 104 7 0 0 0 0\n\
    MouseMoveEvent 107 11 0 0 0 0\n\
    MouseMoveEvent 113 18 0 0 0 0\n\
    MouseMoveEvent 118 27 0 0 0 0\n\
    MouseMoveEvent 121 33 0 0 0 0\n\
    MouseMoveEvent 125 37 0 0 0 0\n\
    MouseMoveEvent 129 44 0 0 0 0\n\
    MouseMoveEvent 132 47 0 0 0 0\n\
    MouseMoveEvent 134 49 0 0 0 0\n\
    MouseMoveEvent 136 51 0 0 0 0\n\
    RenderEvent 136 51 0 0 0 0\n\
    MouseMoveEvent 138 53 0 0 0 0\n\
    MouseMoveEvent 140 55 0 0 0 0\n\
    MouseMoveEvent 142 55 0 0 0 0\n\
    MouseMoveEvent 142 56 0 0 0 0\n\
    MouseMoveEvent 144 57 0 0 0 0\n\
    MouseMoveEvent 144 58 0 0 0 0\n\
    MouseMoveEvent 146 60 0 0 0 0\n\
    MouseMoveEvent 147 62 0 0 0 0\n\
    MouseMoveEvent 148 64 0 0 0 0\n\
    MouseMoveEvent 149 66 0 0 0 0\n\
    MouseMoveEvent 150 68 0 0 0 0\n\
    RenderEvent 150 68 0 0 0 0\n\
    MouseMoveEvent 151 70 0 0 0 0\n\
    RenderEvent 151 70 0 0 0 0\n\
    MouseMoveEvent 154 72 0 0 0 0\n\
    RenderEvent 154 72 0 0 0 0\n\
    MouseMoveEvent 155 76 0 0 0 0\n\
    RenderEvent 155 76 0 0 0 0\n\
    MouseMoveEvent 158 83 0 0 0 0\n\
    RenderEvent 158 83 0 0 0 0\n\
    MouseMoveEvent 159 86 0 0 0 0\n\
    RenderEvent 159 86 0 0 0 0\n\
    MouseMoveEvent 160 90 0 0 0 0\n\
    RenderEvent 160 90 0 0 0 0\n\
    MouseMoveEvent 160 94 0 0 0 0\n\
    RenderEvent 160 94 0 0 0 0\n\
    MouseMoveEvent 161 95 0 0 0 0\n\
    RenderEvent 161 95 0 0 0 0\n\
    MouseMoveEvent 162 99 0 0 0 0\n\
    RenderEvent 162 99 0 0 0 0\n\
    MouseMoveEvent 162 102 0 0 0 0\n\
    RenderEvent 162 102 0 0 0 0\n\
    MouseMoveEvent 162 103 0 0 0 0\n\
    RenderEvent 162 103 0 0 0 0\n\
    MouseMoveEvent 162 106 0 0 0 0\n\
    RenderEvent 162 106 0 0 0 0\n\
    MouseMoveEvent 162 108 0 0 0 0\n\
    RenderEvent 162 108 0 0 0 0\n\
    MouseMoveEvent 162 112 0 0 0 0\n\
    RenderEvent 162 112 0 0 0 0\n\
    MouseMoveEvent 162 114 0 0 0 0\n\
    RenderEvent 162 114 0 0 0 0\n\
    MouseMoveEvent 162 117 0 0 0 0\n\
    RenderEvent 162 117 0 0 0 0\n\
    MouseMoveEvent 162 120 0 0 0 0\n\
    RenderEvent 162 120 0 0 0 0\n\
    MouseMoveEvent 162 121 0 0 0 0\n\
    MouseMoveEvent 162 122 0 0 0 0\n\
    RenderEvent 162 122 0 0 0 0\n\
    MouseMoveEvent 161 124 0 0 0 0\n\
    RenderEvent 161 124 0 0 0 0\n\
    MouseMoveEvent 161 125 0 0 0 0\n\
    MouseMoveEvent 160 127 0 0 0 0\n\
    RenderEvent 160 127 0 0 0 0\n\
    MouseMoveEvent 159 128 0 0 0 0\n\
    RenderEvent 159 128 0 0 0 0\n\
    MouseMoveEvent 159 131 0 0 0 0\n\
    RenderEvent 159 131 0 0 0 0\n\
    MouseMoveEvent 158 133 0 0 0 0\n\
    RenderEvent 158 133 0 0 0 0\n\
    MouseMoveEvent 157 135 0 0 0 0\n\
    RenderEvent 157 135 0 0 0 0\n\
    MouseMoveEvent 157 137 0 0 0 0\n\
    RenderEvent 157 137 0 0 0 0\n\
    MouseMoveEvent 155 139 0 0 0 0\n\
    RenderEvent 155 139 0 0 0 0\n\
    MouseMoveEvent 154 142 0 0 0 0\n\
    RenderEvent 154 142 0 0 0 0\n\
    MouseMoveEvent 153 144 0 0 0 0\n\
    RenderEvent 153 144 0 0 0 0\n\
    MouseMoveEvent 152 146 0 0 0 0\n\
    RenderEvent 152 146 0 0 0 0\n\
    MouseMoveEvent 150 148 0 0 0 0\n\
    RenderEvent 150 148 0 0 0 0\n\
    MouseMoveEvent 150 150 0 0 0 0\n\
    RenderEvent 150 150 0 0 0 0\n\
    MouseMoveEvent 149 151 0 0 0 0\n\
    RenderEvent 149 151 0 0 0 0\n\
    MouseMoveEvent 148 152 0 0 0 0\n\
    MouseMoveEvent 148 153 0 0 0 0\n\
    RenderEvent 148 153 0 0 0 0\n\
    MouseMoveEvent 147 154 0 0 0 0\n\
    RenderEvent 147 154 0 0 0 0\n\
    MouseMoveEvent 147 154 0 0 0 0\n\
    MouseMoveEvent 146 155 0 0 0 0\n\
    RenderEvent 146 155 0 0 0 0\n\
    MouseMoveEvent 145 156 0 0 0 0\n\
    RenderEvent 145 156 0 0 0 0\n\
    MouseMoveEvent 144 157 0 0 0 0\n\
    RenderEvent 144 157 0 0 0 0\n\
    MouseMoveEvent 143 158 0 0 0 0\n\
    RenderEvent 143 158 0 0 0 0\n\
    MouseMoveEvent 143 158 0 0 0 0\n\
    MouseMoveEvent 142 160 0 0 0 0\n\
    RenderEvent 142 160 0 0 0 0\n\
    MouseMoveEvent 142 161 0 0 0 0\n\
    RenderEvent 142 161 0 0 0 0\n\
    MouseMoveEvent 141 162 0 0 0 0\n\
    MouseMoveEvent 141 163 0 0 0 0\n\
    MouseMoveEvent 141 163 0 0 0 0\n\
    MouseMoveEvent 141 164 0 0 0 0\n\
    RenderEvent 141 164 0 0 0 0\n\
    MouseMoveEvent 141 165 0 0 0 0\n\
    MouseMoveEvent 141 166 0 0 0 0\n\
    RenderEvent 141 166 0 0 0 0\n\
    MouseMoveEvent 141 167 0 0 0 0\n\
    RenderEvent 141 167 0 0 0 0\n\
    MouseMoveEvent 141 167 0 0 0 0\n\
    RenderEvent 141 167 0 0 0 0\n\
    LeftButtonPressEvent 141 167 0 0 0 0\n\
    RenderEvent 141 167 0 0 0 0\n\
    LeftButtonReleaseEvent 141 167 0 0 0 0\n\
    RenderEvent 141 167 0 0 0 0\n\
    MouseMoveEvent 141 167 0 0 0 0\n\
    RenderEvent 141 167 0 0 0 0\n\
    MouseMoveEvent 141 166 0 0 0 0\n\
    RenderEvent 141 166 0 0 0 0\n\
    MouseMoveEvent 141 165 0 0 0 0\n\
    RenderEvent 141 165 0 0 0 0\n\
    MouseMoveEvent 141 165 0 0 0 0\n\
    MouseMoveEvent 141 164 0 0 0 0\n\
    MouseMoveEvent 141 163 0 0 0 0\n\
    RenderEvent 141 163 0 0 0 0\n\
    MouseMoveEvent 140 162 0 0 0 0\n\
    MouseMoveEvent 140 161 0 0 0 0\n\
    MouseMoveEvent 140 159 0 0 0 0\n\
    RenderEvent 140 159 0 0 0 0\n\
    MouseMoveEvent 140 158 0 0 0 0\n\
    RenderEvent 140 158 0 0 0 0\n\
    MouseMoveEvent 140 157 0 0 0 0\n\
    RenderEvent 140 157 0 0 0 0\n\
    MouseMoveEvent 139 155 0 0 0 0\n\
    RenderEvent 139 155 0 0 0 0\n\
    MouseMoveEvent 139 154 0 0 0 0\n\
    RenderEvent 139 154 0 0 0 0\n\
    MouseMoveEvent 138 153 0 0 0 0\n\
    MouseMoveEvent 138 152 0 0 0 0\n\
    MouseMoveEvent 138 150 0 0 0 0\n\
    RenderEvent 138 150 0 0 0 0\n\
    MouseMoveEvent 138 149 0 0 0 0\n\
    MouseMoveEvent 138 148 0 0 0 0\n\
    MouseMoveEvent 138 147 0 0 0 0\n\
    RenderEvent 138 147 0 0 0 0\n\
    MouseMoveEvent 138 146 0 0 0 0\n\
    MouseMoveEvent 138 145 0 0 0 0\n\
    RenderEvent 138 145 0 0 0 0\n\
    MouseMoveEvent 138 144 0 0 0 0\n\
    MouseMoveEvent 138 143 0 0 0 0\n\
    RenderEvent 138 143 0 0 0 0\n\
    MouseMoveEvent 137 141 0 0 0 0\n\
    RenderEvent 137 141 0 0 0 0\n\
    MouseMoveEvent 137 139 0 0 0 0\n\
    RenderEvent 137 139 0 0 0 0\n\
    MouseMoveEvent 137 138 0 0 0 0\n\
    LeftButtonPressEvent 137 138 0 0 0 0\n\
    RenderEvent 137 138 0 0 0 0\n\
    LeftButtonReleaseEvent 137 138 0 0 0 0\n\
    RenderEvent 137 138 0 0 0 0\n\
    "

# create a point source
#
pc = vtkPointSource()
pc.SetNumberOfPoints(npts)
pc.SetCenter(5,10,20)
pc.SetRadius(7.5)
pc.Update()

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)

iRen = vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin);

# Add the actors to the renderer, set the background and size
#
ren.SetBackground(0.1, 0.2, 0.4)
renWin.SetSize(300, 300)

def SelectPoint(widget, event_string):
    pcWidget,rep
    print("Point Id {0}: ".format(rep.GetPointId()))

def ReportCoords(widget, event_string):
    pcWidget,rep
    pId = rep.GetPointId()
    print("Selected Point Id {0}: ".format(rep.GetPointId()))
    print("Point Coordinates {0}: ".format(pc.GetOutput().GetPoints().GetPoint(pId)))

# Conveniently the representation creates an actor/mapper
# to render the point cloud.
rep = vtkPointCloudRepresentation()
rep.SetPlaceFactor(1.0);
rep.PlacePointCloud(pc.GetOutput());
rep.SetPickingModeToHardware()

pcWidget = vtkPointCloudWidget()
pcWidget.SetInteractor(iRen)
pcWidget.SetRepresentation(rep);
pcWidget.AddObserver("PickEvent",SelectPoint);
pcWidget.AddObserver("WidgetActivateEvent",ReportCoords);
pcWidget.On()

# Handle playback of events
recorder = vtkInteractorEventRecorder()
recorder.SetInteractor(iRen)
#recorder.SetFileName("record.log")
#recorder.On()
#recorder.Record()
recorder.ReadFromInputStringOn()
recorder.SetInputString(Recording)

# render and interact with data
ren.ResetCamera()
renWin.Render()

recorder.Play()

iRen.Start()
