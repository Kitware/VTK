#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNamedColorsIntegration.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# These are the pre-recorded events
Recording = \
            "# StreamVersion 1\n\
            RenderEvent 0 0 0 0 0 0 0\n\
            EnterEvent 298 238 0 0 0 0 0\n\
            MouseMoveEvent 298 238 0 0 0 0 0\n\
            MouseMoveEvent 293 240 0 0 0 0 0\n\
            MouseMoveEvent 290 240 0 0 0 0 0\n\
            MouseMoveEvent 286 240 0 0 0 0 0\n\
            MouseMoveEvent 283 242 0 0 0 0 0\n\
            MouseMoveEvent 281 242 0 0 0 0 0\n\
            MouseMoveEvent 278 243 0 0 0 0 0\n\
            MouseMoveEvent 276 243 0 0 0 0 0\n\
            MouseMoveEvent 273 244 0 0 0 0 0\n\
            MouseMoveEvent 271 244 0 0 0 0 0\n\
            MouseMoveEvent 270 244 0 0 0 0 0\n\
            MouseMoveEvent 269 245 0 0 0 0 0\n\
            MouseMoveEvent 268 245 0 0 0 0 0\n\
            MouseMoveEvent 267 245 0 0 0 0 0\n\
            MouseMoveEvent 266 245 0 0 0 0 0\n\
            MouseMoveEvent 264 246 0 0 0 0 0\n\
            MouseMoveEvent 264 246 0 0 0 0 0\n\
            MouseMoveEvent 262 246 0 0 0 0 0\n\
            MouseMoveEvent 262 248 0 0 0 0 0\n\
            MouseMoveEvent 259 248 0 0 0 0 0\n\
            MouseMoveEvent 259 248 0 0 0 0 0\n\
            MouseMoveEvent 257 249 0 0 0 0 0\n\
            MouseMoveEvent 257 250 0 0 0 0 0\n\
            MouseMoveEvent 255 250 0 0 0 0 0\n\
            RenderEvent 255 250 0 0 0 0 0\n\
            MouseMoveEvent 252 250 0 0 0 0 0\n\
            RenderEvent 252 250 0 0 0 0 0\n\
            MouseMoveEvent 251 251 0 0 0 0 0\n\
            RenderEvent 251 251 0 0 0 0 0\n\
            MouseMoveEvent 249 252 0 0 0 0 0\n\
            RenderEvent 249 252 0 0 0 0 0\n\
            MouseMoveEvent 248 252 0 0 0 0 0\n\
            RenderEvent 248 252 0 0 0 0 0\n\
            MouseMoveEvent 247 252 0 0 0 0 0\n\
            RenderEvent 247 252 0 0 0 0 0\n\
            MouseMoveEvent 247 253 0 0 0 0 0\n\
            RenderEvent 247 253 0 0 0 0 0\n\
            LeftButtonPressEvent 247 253 0 0 0 0 0\n\
            RenderEvent 247 253 0 0 0 0 0\n\
            MouseMoveEvent 246 251 0 0 0 0 0\n\
            RenderEvent 246 251 0 0 0 0 0\n\
            MouseMoveEvent 193 206 0 0 0 0 0\n\
            RenderEvent 193 206 0 0 0 0 0\n\
            MouseMoveEvent 186 202 0 0 0 0 0\n\
            RenderEvent 186 202 0 0 0 0 0\n\
            MouseMoveEvent 179 201 0 0 0 0 0\n\
            RenderEvent 179 201 0 0 0 0 0\n\
            MouseMoveEvent 160 201 0 0 0 0 0\n\
            RenderEvent 160 201 0 0 0 0 0\n\
            MouseMoveEvent 157 202 0 0 0 0 0\n\
            RenderEvent 157 202 0 0 0 0 0\n\
            MouseMoveEvent 155 208 0 0 0 0 0\n\
            RenderEvent 155 208 0 0 0 0 0\n\
            MouseMoveEvent 153 222 0 0 0 0 0\n\
            RenderEvent 153 222 0 0 0 0 0\n\
            MouseMoveEvent 151 230 0 0 0 0 0\n\
            RenderEvent 151 230 0 0 0 0 0\n\
            MouseMoveEvent 152 236 0 0 0 0 0\n\
            RenderEvent 152 236 0 0 0 0 0\n\
            MouseMoveEvent 163 242 0 0 0 0 0\n\
            RenderEvent 163 242 0 0 0 0 0\n\
            MouseMoveEvent 165 242 0 0 0 0 0\n\
            RenderEvent 165 242 0 0 0 0 0\n\
            MouseMoveEvent 185 244 0 0 0 0 0\n\
            RenderEvent 185 244 0 0 0 0 0\n\
            MouseMoveEvent 199 244 0 0 0 0 0\n\
            RenderEvent 199 244 0 0 0 0 0\n\
            MouseMoveEvent 205 243 0 0 0 0 0\n\
            RenderEvent 205 243 0 0 0 0 0\n\
            MouseMoveEvent 205 243 0 0 0 0 0\n\
            RenderEvent 205 243 0 0 0 0 0\n\
            MouseMoveEvent 206 243 0 0 0 0 0\n\
            RenderEvent 206 243 0 0 0 0 0\n\
            MouseMoveEvent 214 245 0 0 0 0 0\n\
            RenderEvent 214 245 0 0 0 0 0\n\
            MouseMoveEvent 215 245 0 0 0 0 0\n\
            RenderEvent 215 245 0 0 0 0 0\n\
            MouseMoveEvent 221 248 0 0 0 0 0\n\
            RenderEvent 221 248 0 0 0 0 0\n\
            MouseMoveEvent 222 255 0 0 0 0 0\n\
            RenderEvent 222 255 0 0 0 0 0\n\
            MouseMoveEvent 221 266 0 0 0 0 0\n\
            RenderEvent 221 266 0 0 0 0 0\n\
            MouseMoveEvent 216 274 0 0 0 0 0\n\
            RenderEvent 216 274 0 0 0 0 0\n\
            MouseMoveEvent 212 279 0 0 0 0 0\n\
            RenderEvent 212 279 0 0 0 0 0\n\
            MouseMoveEvent 213 280 0 0 0 0 0\n\
            RenderEvent 213 280 0 0 0 0 0\n\
            MouseMoveEvent 225 284 0 0 0 0 0\n\
            RenderEvent 225 284 0 0 0 0 0\n\
            MouseMoveEvent 229 284 0 0 0 0 0\n\
            RenderEvent 229 284 0 0 0 0 0\n\
            MouseMoveEvent 234 284 0 0 0 0 0\n\
            RenderEvent 234 284 0 0 0 0 0\n\
            MouseMoveEvent 244 282 0 0 0 0 0\n\
            RenderEvent 244 282 0 0 0 0 0\n\
            MouseMoveEvent 249 280 0 0 0 0 0\n\
            RenderEvent 249 280 0 0 0 0 0\n\
            MouseMoveEvent 253 276 0 0 0 0 0\n\
            RenderEvent 253 276 0 0 0 0 0\n\
            MouseMoveEvent 257 271 0 0 0 0 0\n\
            RenderEvent 257 271 0 0 0 0 0\n\
            MouseMoveEvent 260 266 0 0 0 0 0\n\
            RenderEvent 260 266 0 0 0 0 0\n\
            MouseMoveEvent 261 264 0 0 0 0 0\n\
            RenderEvent 261 264 0 0 0 0 0\n\
            MouseMoveEvent 261 262 0 0 0 0 0\n\
            RenderEvent 261 262 0 0 0 0 0\n\
            LeftButtonReleaseEvent 261 262 0 0 0 0 0\n\
            RenderEvent 261 262 0 0 0 0 0\n\
            MouseMoveEvent 261 262 0 0 0 0 0\n\
            RenderEvent 261 262 0 0 0 0 0\n\
            "

# Create some synthetic data
#
# Create a synthetic source: sample a sphere across a volume
sphere = vtk.vtkSphere()
sphere.SetCenter( 0.0,0.0,0.0)
sphere.SetRadius(0.25)

res = 100
sample = vtk.vtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sample.SetSampleDimensions(res,res,res)
sample.SetOutputScalarTypeToFloat()
sample.Update()

# The cut plane
plane = vtk.vtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(1,1,1)

cut = vtk.vtkFlyingEdgesPlaneCutter()
cut.SetInputConnection(sample.GetOutputPort())
cut.SetPlane(plane)
cut.ComputeNormalsOff()

cutMapper = vtk.vtkPolyDataMapper()
cutMapper.SetInputConnection(cut.GetOutputPort())

cutActor = vtk.vtkActor()
cutActor.SetMapper(cutMapper)
cutActor.GetProperty().SetColor(1,1,1)
cutActor.GetProperty().SetOpacity(1)

# Create the RenderWindow, Renderer and both Actors
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren)
iRen = vtk.vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin)

# Create the widget, its representation, and callback
def MovePlane(widget, event_string):
    rep.GetPlane(plane)

rep = vtk.vtkImplicitPlaneRepresentation()
rep.SetPlaceFactor(1.0);
rep.PlaceWidget(sample.GetOutput().GetBounds())
rep.DrawPlaneOff()
rep.SetPlane(plane)

planeWidget = vtk.vtkImplicitPlaneWidget2()
planeWidget.SetInteractor(iRen)
planeWidget.SetRepresentation(rep);
planeWidget.AddObserver("InteractionEvent",MovePlane);

recorder = vtk.vtkInteractorEventRecorder()
recorder.SetInteractor(iRen)
recorder.ReadFromInputStringOn()
recorder.SetInputString(Recording)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(cutActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(300, 300)
ren.SetBackground(0.1, 0.2, 0.4)

iRen.Initialize()
renWin.Render()
planeWidget.On()

# Actually cut the data
recorder.Play()
#iRen.Start()
