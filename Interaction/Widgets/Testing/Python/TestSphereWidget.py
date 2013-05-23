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

class TestSphereWidget(vtk.test.Testing.vtkTest):

    def testSphereWidget(self):

        # This example demonstrates how to use the vtkSphereWidget to control the
        # position of a light.

        # These are the pre-recorded events
        Recording = \
           "# StreamVersion 1\n\
            CharEvent 23 266 0 0 105 1 i\n\
            KeyReleaseEvent 23 266 0 0 105 1 i\n\
            EnterEvent 69 294 0 0 0 0 i\n\
            MouseMoveEvent 69 294 0 0 0 0 i\n\
            MouseMoveEvent 68 293 0 0 0 0 i\n\
            MouseMoveEvent 67 292 0 0 0 0 i\n\
            MouseMoveEvent 66 289 0 0 0 0 i\n\
            MouseMoveEvent 66 282 0 0 0 0 i\n\
            MouseMoveEvent 66 271 0 0 0 0 i\n\
            MouseMoveEvent 69 253 0 0 0 0 i\n\
            MouseMoveEvent 71 236 0 0 0 0 i\n\
            MouseMoveEvent 74 219 0 0 0 0 i\n\
            MouseMoveEvent 76 208 0 0 0 0 i\n\
            MouseMoveEvent 78 190 0 0 0 0 i\n\
            MouseMoveEvent 78 173 0 0 0 0 i\n\
            MouseMoveEvent 77 162 0 0 0 0 i\n\
            MouseMoveEvent 77 151 0 0 0 0 i\n\
            MouseMoveEvent 77 139 0 0 0 0 i\n\
            MouseMoveEvent 76 125 0 0 0 0 i\n\
            MouseMoveEvent 73 114 0 0 0 0 i\n\
            MouseMoveEvent 73 106 0 0 0 0 i\n\
            MouseMoveEvent 73 101 0 0 0 0 i\n\
            MouseMoveEvent 72 95 0 0 0 0 i\n\
            MouseMoveEvent 72 92 0 0 0 0 i\n\
            MouseMoveEvent 70 89 0 0 0 0 i\n\
            MouseMoveEvent 69 86 0 0 0 0 i\n\
            MouseMoveEvent 67 84 0 0 0 0 i\n\
            MouseMoveEvent 65 81 0 0 0 0 i\n\
            MouseMoveEvent 60 79 0 0 0 0 i\n\
            MouseMoveEvent 59 79 0 0 0 0 i\n\
            MouseMoveEvent 58 79 0 0 0 0 i\n\
            MouseMoveEvent 57 78 0 0 0 0 i\n\
            MouseMoveEvent 55 78 0 0 0 0 i\n\
            MouseMoveEvent 54 77 0 0 0 0 i\n\
            LeftButtonPressEvent 54 77 0 0 0 0 i\n\
            MouseMoveEvent 61 79 0 0 0 0 i\n\
            MouseMoveEvent 67 83 0 0 0 0 i\n\
            MouseMoveEvent 72 88 0 0 0 0 i\n\
            MouseMoveEvent 77 90 0 0 0 0 i\n\
            MouseMoveEvent 78 91 0 0 0 0 i\n\
            MouseMoveEvent 80 92 0 0 0 0 i\n\
            MouseMoveEvent 84 93 0 0 0 0 i\n\
            MouseMoveEvent 85 94 0 0 0 0 i\n\
            MouseMoveEvent 88 97 0 0 0 0 i\n\
            MouseMoveEvent 90 100 0 0 0 0 i\n\
            MouseMoveEvent 92 102 0 0 0 0 i\n\
            MouseMoveEvent 94 103 0 0 0 0 i\n\
            MouseMoveEvent 97 105 0 0 0 0 i\n\
            MouseMoveEvent 101 107 0 0 0 0 i\n\
            MouseMoveEvent 102 109 0 0 0 0 i\n\
            MouseMoveEvent 104 111 0 0 0 0 i\n\
            MouseMoveEvent 108 113 0 0 0 0 i\n\
            MouseMoveEvent 112 115 0 0 0 0 i\n\
            MouseMoveEvent 118 119 0 0 0 0 i\n\
            MouseMoveEvent 118 120 0 0 0 0 i\n\
            MouseMoveEvent 118 123 0 0 0 0 i\n\
            MouseMoveEvent 120 125 0 0 0 0 i\n\
            MouseMoveEvent 122 128 0 0 0 0 i\n\
            MouseMoveEvent 123 129 0 0 0 0 i\n\
            MouseMoveEvent 125 132 0 0 0 0 i\n\
            MouseMoveEvent 125 134 0 0 0 0 i\n\
            MouseMoveEvent 127 138 0 0 0 0 i\n\
            MouseMoveEvent 127 142 0 0 0 0 i\n\
            MouseMoveEvent 127 147 0 0 0 0 i\n\
            MouseMoveEvent 126 152 0 0 0 0 i\n\
            MouseMoveEvent 126 155 0 0 0 0 i\n\
            MouseMoveEvent 125 160 0 0 0 0 i\n\
            MouseMoveEvent 125 167 0 0 0 0 i\n\
            MouseMoveEvent 125 169 0 0 0 0 i\n\
            MouseMoveEvent 125 174 0 0 0 0 i\n\
            MouseMoveEvent 122 179 0 0 0 0 i\n\
            MouseMoveEvent 120 183 0 0 0 0 i\n\
            MouseMoveEvent 116 187 0 0 0 0 i\n\
            MouseMoveEvent 113 192 0 0 0 0 i\n\
            MouseMoveEvent 113 193 0 0 0 0 i\n\
            MouseMoveEvent 111 195 0 0 0 0 i\n\
            MouseMoveEvent 108 198 0 0 0 0 i\n\
            MouseMoveEvent 106 200 0 0 0 0 i\n\
            MouseMoveEvent 104 202 0 0 0 0 i\n\
            MouseMoveEvent 103 203 0 0 0 0 i\n\
            MouseMoveEvent 99 205 0 0 0 0 i\n\
            MouseMoveEvent 97 207 0 0 0 0 i\n\
            MouseMoveEvent 94 208 0 0 0 0 i\n\
            MouseMoveEvent 91 210 0 0 0 0 i\n\
            MouseMoveEvent 89 211 0 0 0 0 i\n\
            MouseMoveEvent 86 211 0 0 0 0 i\n\
            MouseMoveEvent 84 211 0 0 0 0 i\n\
            MouseMoveEvent 80 211 0 0 0 0 i\n\
            MouseMoveEvent 77 211 0 0 0 0 i\n\
            MouseMoveEvent 75 211 0 0 0 0 i\n\
            MouseMoveEvent 71 211 0 0 0 0 i\n\
            MouseMoveEvent 68 211 0 0 0 0 i\n\
            MouseMoveEvent 66 210 0 0 0 0 i\n\
            MouseMoveEvent 62 210 0 0 0 0 i\n\
            MouseMoveEvent 58 209 0 0 0 0 i\n\
            MouseMoveEvent 54 207 0 0 0 0 i\n\
            MouseMoveEvent 52 204 0 0 0 0 i\n\
            MouseMoveEvent 51 203 0 0 0 0 i\n\
            MouseMoveEvent 51 200 0 0 0 0 i\n\
            MouseMoveEvent 48 196 0 0 0 0 i\n\
            MouseMoveEvent 45 187 0 0 0 0 i\n\
            MouseMoveEvent 45 181 0 0 0 0 i\n\
            MouseMoveEvent 44 168 0 0 0 0 i\n\
            MouseMoveEvent 40 161 0 0 0 0 i\n\
            MouseMoveEvent 39 154 0 0 0 0 i\n\
            MouseMoveEvent 38 146 0 0 0 0 i\n\
            MouseMoveEvent 35 131 0 0 0 0 i\n\
            MouseMoveEvent 34 121 0 0 0 0 i\n\
            MouseMoveEvent 34 110 0 0 0 0 i\n\
            MouseMoveEvent 34 103 0 0 0 0 i\n\
            MouseMoveEvent 34 91 0 0 0 0 i\n\
            MouseMoveEvent 34 86 0 0 0 0 i\n\
            MouseMoveEvent 34 73 0 0 0 0 i\n\
            MouseMoveEvent 35 66 0 0 0 0 i\n\
            MouseMoveEvent 37 60 0 0 0 0 i\n\
            MouseMoveEvent 37 53 0 0 0 0 i\n\
            MouseMoveEvent 38 50 0 0 0 0 i\n\
            MouseMoveEvent 38 48 0 0 0 0 i\n\
            MouseMoveEvent 41 45 0 0 0 0 i\n\
            MouseMoveEvent 43 45 0 0 0 0 i\n\
            MouseMoveEvent 44 45 0 0 0 0 i\n\
            MouseMoveEvent 47 43 0 0 0 0 i\n\
            MouseMoveEvent 51 44 0 0 0 0 i\n\
            MouseMoveEvent 54 44 0 0 0 0 i\n\
            MouseMoveEvent 55 44 0 0 0 0 i\n\
            MouseMoveEvent 59 44 0 0 0 0 i\n\
            MouseMoveEvent 64 44 0 0 0 0 i\n\
            MouseMoveEvent 67 44 0 0 0 0 i\n\
            MouseMoveEvent 68 44 0 0 0 0 i\n\
            MouseMoveEvent 71 44 0 0 0 0 i\n\
            MouseMoveEvent 74 44 0 0 0 0 i\n\
            MouseMoveEvent 77 44 0 0 0 0 i\n\
            MouseMoveEvent 80 45 0 0 0 0 i\n\
            MouseMoveEvent 81 45 0 0 0 0 i\n\
            MouseMoveEvent 85 49 0 0 0 0 i\n\
            MouseMoveEvent 89 50 0 0 0 0 i\n\
            MouseMoveEvent 94 52 0 0 0 0 i\n\
            MouseMoveEvent 99 56 0 0 0 0 i\n\
            MouseMoveEvent 104 58 0 0 0 0 i\n\
            MouseMoveEvent 107 61 0 0 0 0 i\n\
            MouseMoveEvent 109 63 0 0 0 0 i\n\
            MouseMoveEvent 109 67 0 0 0 0 i\n\
            MouseMoveEvent 111 83 0 0 0 0 i\n\
            MouseMoveEvent 113 86 0 0 0 0 i\n\
            MouseMoveEvent 113 87 0 0 0 0 i\n\
            MouseMoveEvent 113 89 0 0 0 0 i\n\
            MouseMoveEvent 112 93 0 0 0 0 i\n\
            MouseMoveEvent 112 97 0 0 0 0 i\n\
            MouseMoveEvent 111 104 0 0 0 0 i\n\
            MouseMoveEvent 112 108 0 0 0 0 i\n\
            MouseMoveEvent 116 115 0 0 0 0 i\n\
            MouseMoveEvent 116 123 0 0 0 0 i\n\
            MouseMoveEvent 116 129 0 0 0 0 i\n\
            MouseMoveEvent 119 138 0 0 0 0 i\n\
            MouseMoveEvent 122 141 0 0 0 0 i\n\
            MouseMoveEvent 127 148 0 0 0 0 i\n\
            MouseMoveEvent 128 161 0 0 0 0 i\n\
            MouseMoveEvent 131 166 0 0 0 0 i\n\
            MouseMoveEvent 134 168 0 0 0 0 i\n\
            MouseMoveEvent 135 171 0 0 0 0 i\n\
            MouseMoveEvent 134 174 0 0 0 0 i\n\
            MouseMoveEvent 132 176 0 0 0 0 i\n\
            MouseMoveEvent 132 178 0 0 0 0 i\n\
            MouseMoveEvent 129 180 0 0 0 0 i\n\
            MouseMoveEvent 127 182 0 0 0 0 i\n\
            MouseMoveEvent 124 185 0 0 0 0 i\n\
            MouseMoveEvent 122 186 0 0 0 0 i\n\
            MouseMoveEvent 118 189 0 0 0 0 i\n\
            MouseMoveEvent 114 191 0 0 0 0 i\n\
            MouseMoveEvent 114 193 0 0 0 0 i\n\
            MouseMoveEvent 112 193 0 0 0 0 i\n\
            MouseMoveEvent 111 194 0 0 0 0 i\n\
            MouseMoveEvent 110 197 0 0 0 0 i\n\
            MouseMoveEvent 110 198 0 0 0 0 i\n\
            MouseMoveEvent 109 199 0 0 0 0 i\n\
            MouseMoveEvent 108 200 0 0 0 0 i\n\
            MouseMoveEvent 108 201 0 0 0 0 i\n\
            MouseMoveEvent 108 202 0 0 0 0 i\n\
            MouseMoveEvent 108 203 0 0 0 0 i\n\
            MouseMoveEvent 104 206 0 0 0 0 i\n\
            LeftButtonReleaseEvent 104 206 0 0 0 0 i\n\
            MouseMoveEvent 104 205 0 0 0 0 i\n\
            MouseMoveEvent 104 204 0 0 0 0 i\n\
            MouseMoveEvent 105 205 0 0 0 0 i\n\
            MouseMoveEvent 105 206 0 0 0 0 i\n\
        "

        # Start by loading some data.
        #
        dem = vtk.vtkDEMReader()
        dem.SetFileName(VTK_DATA_ROOT + "/Data/SainteHelens.dem")
        dem.Update()

        Scale = 2
        lut = vtk.vtkLookupTable()
        lut.SetHueRange(0.6, 0)
        lut.SetSaturationRange(1.0, 0)
        lut.SetValueRange(0.5, 1.0)
        lo = Scale * dem.GetElevationBounds()[0]

        hi = Scale * dem.GetElevationBounds()[1]


        shrink = vtk.vtkImageShrink3D()
        shrink.SetShrinkFactors(4, 4, 1)
        shrink.SetInputConnection(dem.GetOutputPort())
        shrink.AveragingOn()

        geom = vtk.vtkImageDataGeometryFilter()
        geom.SetInputConnection(shrink.GetOutputPort())
        geom.ReleaseDataFlagOn()

        warp = vtk.vtkWarpScalar()
        warp.SetInputConnection(geom.GetOutputPort())
        warp.SetNormal(0, 0, 1)
        warp.UseNormalOn()
        warp.SetScaleFactor(Scale)
        warp.ReleaseDataFlagOn()

        elevation = vtk.vtkElevationFilter()
        elevation.SetInputConnection(warp.GetOutputPort())
        elevation.SetLowPoint(0, 0, lo)
        elevation.SetHighPoint(0, 0, hi)
        elevation.SetScalarRange(lo, hi)
        elevation.ReleaseDataFlagOn()

        normals = vtk.vtkPolyDataNormals()
        normals.SetInputConnection(elevation.GetOutputPort())
        normals.SetFeatureAngle(60)
        normals.ConsistencyOff()
        normals.SplittingOff()
        normals.ReleaseDataFlagOn()
        normals.Update()

        demMapper = vtk.vtkPolyDataMapper()
        demMapper.SetInputConnection(normals.GetOutputPort())
        demMapper.SetScalarRange(lo, hi)
        demMapper.SetLookupTable(lut)
        demMapper.ImmediateModeRenderingOn()

        demActor = vtk.vtkActor()
        demActor.SetMapper(demMapper)

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.SetMultiSamples(0)
        renWin.AddRenderer(ren)
        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin)
        iRen.LightFollowCameraOff()
        #    iRen.SetInteractorStyle("")

        # The callback takes two arguments.
        # The first being the object that generates the event and
        # the second argument the event name (which is a string).
        def MoveLight(widget, event_string):
            light.SetPosition(rep.GetHandlePosition())

        # Associate the line widget with the interactor
        rep = vtk.vtkSphereRepresentation()
        rep.SetPlaceFactor(4)
        rep.PlaceWidget(normals.GetOutput().GetBounds())
        rep.HandleVisibilityOn()
        rep.SetRepresentationToWireframe()
        #  rep HandleVisibilityOff
        #  rep HandleTextOff
        sphereWidget = vtk.vtkSphereWidget2()
        sphereWidget.SetInteractor(iRen)
        sphereWidget.SetRepresentation(rep)
        #  sphereWidget.TranslationEnabledOff()
        #  sphereWidget.ScalingEnabledOff()
        sphereWidget.AddObserver("InteractionEvent", MoveLight)

        recorder = vtk.vtkInteractorEventRecorder()
        recorder.SetInteractor(iRen)
        #  recorder.SetFileName("c:/record.log")
        #  recorder.Record()
        recorder.ReadFromInputStringOn()
        recorder.SetInputString(Recording)

        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(demActor)
        ren.SetBackground(1, 1, 1)
        renWin.SetSize(300, 300)
        ren.SetBackground(0.1, 0.2, 0.4)

        cam1 = ren.GetActiveCamera()
        cam1.SetViewUp(0, 0, 1)
        cam1.SetFocalPoint(dem.GetOutput().GetCenter())
        cam1.SetPosition(1, 0, 0)
        ren.ResetCamera()
        cam1.Elevation(25)
        cam1.Azimuth(125)
        cam1.Zoom(1.25)

        light = vtk.vtkLight()
        light.SetFocalPoint(rep.GetCenter())
        light.SetPosition(rep.GetHandlePosition())
        ren.AddLight(light)

        iRen.Initialize()
        renWin.Render()

        # render the image
        renWin.Render()

        # Actually probe the data
        recorder.Play()

        img_file = "TestSphereWidget.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(TestSphereWidget, 'test')])