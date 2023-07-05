#!/usr/bin/env python
# -*- coding: utf-8 -*-


from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkFiltersCore import vtkGlyph3D
from vtkmodules.vtkFiltersSources import (
    vtkConeSource,
    vtkSphereSource,
)
from vtkmodules.vtkRenderingCore import (
    vtkInteractorEventRecorder,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingLOD import vtkLODActor
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing

# Make grabbing color a little easier
def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# Create a sphere source and actor
sphere = vtkSphereSource()

sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())

sphereActor = vtkLODActor()
sphereActor.SetMapper(sphereMapper)

sphereActor.GetProperty().SetDiffuseColor(GetRGBColor('banana'))
sphereActor.GetProperty().SetSpecular(.4)
sphereActor.GetProperty().SetSpecularPower(20)

# Create the spikes using a cone source and the sphere source

cone = vtkConeSource()
cone.SetResolution(20)

glyph = vtkGlyph3D()
glyph.SetInputConnection(sphere.GetOutputPort())
glyph.SetSourceConnection(cone.GetOutputPort())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor(0.25)

spikeMapper = vtkPolyDataMapper()
spikeMapper.SetInputConnection(glyph.GetOutputPort())

spikeActor = vtkLODActor()
spikeActor.SetMapper(spikeMapper)

spikeActor.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))
spikeActor.GetProperty().SetSpecular(.4)
spikeActor.GetProperty().SetSpecularPower(20)

# Render the image

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren)

iRen = vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin)

ren.AddActor(sphereActor)
ren.AddActor(spikeActor)
ren.SetBackground(0.1, 0.2, 0.4)

cam1 = ren.GetActiveCamera()
cam1.Zoom(1.4)
cam1.Azimuth(30)
cam1.Elevation(30)
ren.ResetCamera()

# Setup for recording
renWin.Render()

Recording = \
    "# StreamVersion 1.1\n\
    KeyPressEvent 165 173 0 106 1 j\n\
    CharEvent 165 173 0 106 1 j\n\
    KeyReleaseEvent 165 173 0 106 1 j\n\
    KeyPressEvent 165 173 0 97 1 a\n\
    CharEvent 165 173 0 97 1 a\n\
    KeyReleaseEvent 165 173 0 97 1 a\n\
    MouseMoveEvent 164 173 0 0 0 a\n\
    MouseMoveEvent 162 173 0 0 0 a\n\
    MouseMoveEvent 161 173 0 0 0 a\n\
    MouseMoveEvent 159 173 0 0 0 a\n\
    MouseMoveEvent 157 173 0 0 0 a\n\
    MouseMoveEvent 156 173 0 0 0 a\n\
    MouseMoveEvent 156 172 0 0 0 a\n\
    MouseMoveEvent 155 172 0 0 0 a\n\
    MouseMoveEvent 153 172 0 0 0 a\n\
    MouseMoveEvent 151 172 0 0 0 a\n\
    MouseMoveEvent 151 172 0 0 0 a\n\
    MouseMoveEvent 151 171 0 0 0 a\n\
    MouseMoveEvent 150 170 0 0 0 a\n\
    MouseMoveEvent 149 169 0 0 0 a\n\
    MouseMoveEvent 149 168 0 0 0 a\n\
    MouseMoveEvent 148 168 0 0 0 a\n\
    MouseMoveEvent 148 167 0 0 0 a\n\
    MouseMoveEvent 148 167 0 0 0 a\n\
    MouseMoveEvent 147 166 0 0 0 a\n\
    MouseMoveEvent 147 165 0 0 0 a\n\
    MouseMoveEvent 147 164 0 0 0 a\n\
    MouseMoveEvent 147 163 0 0 0 a\n\
    MouseMoveEvent 147 162 0 0 0 a\n\
    MouseMoveEvent 147 161 0 0 0 a\n\
    MouseMoveEvent 146 160 0 0 0 a\n\
    MouseMoveEvent 146 158 0 0 0 a\n\
    MouseMoveEvent 146 156 0 0 0 a\n\
    MouseMoveEvent 146 153 0 0 0 a\n\
    MouseMoveEvent 146 150 0 0 0 a\n\
    MouseMoveEvent 145 148 0 0 0 a\n\
    MouseMoveEvent 144 145 0 0 0 a\n\
    MouseMoveEvent 144 142 0 0 0 a\n\
    MouseMoveEvent 143 139 0 0 0 a\n\
    MouseMoveEvent 142 137 0 0 0 a\n\
    MouseMoveEvent 142 135 0 0 0 a\n\
    MouseMoveEvent 141 133 0 0 0 a\n\
    MouseMoveEvent 141 131 0 0 0 a\n\
    MouseMoveEvent 141 130 0 0 0 a\n\
    MouseMoveEvent 141 128 0 0 0 a\n\
    MouseMoveEvent 141 126 0 0 0 a\n\
    MouseMoveEvent 140 125 0 0 0 a\n\
    MouseMoveEvent 140 124 0 0 0 a\n\
    MouseMoveEvent 140 122 0 0 0 a\n\
    MouseMoveEvent 139 120 0 0 0 a\n\
    MouseMoveEvent 138 118 0 0 0 a\n\
    MouseMoveEvent 138 116 0 0 0 a\n\
    MouseMoveEvent 137 115 0 0 0 a\n\
    MouseMoveEvent 136 114 0 0 0 a\n\
    MouseMoveEvent 136 113 0 0 0 a\n\
    MouseMoveEvent 136 110 0 0 0 a\n\
    MouseMoveEvent 135 109 0 0 0 a\n\
    MouseMoveEvent 135 107 0 0 0 a\n\
    MouseMoveEvent 134 105 0 0 0 a\n\
    MouseMoveEvent 134 103 0 0 0 a\n\
    MouseMoveEvent 133 101 0 0 0 a\n\
    MouseMoveEvent 133 100 0 0 0 a\n\
    MouseMoveEvent 132 98 0 0 0 a\n\
    MouseMoveEvent 131 97 0 0 0 a\n\
    MouseMoveEvent 131 96 0 0 0 a\n\
    MouseMoveEvent 131 95 0 0 0 a\n\
    MouseMoveEvent 130 93 0 0 0 a\n\
    MouseMoveEvent 129 92 0 0 0 a\n\
    MouseMoveEvent 129 91 0 0 0 a\n\
    MouseMoveEvent 129 90 0 0 0 a\n\
    MouseMoveEvent 129 89 0 0 0 a\n\
    MouseMoveEvent 128 88 0 0 0 a\n\
    MouseMoveEvent 128 88 0 0 0 a\n\
    MouseMoveEvent 129 88 0 0 0 a\n\
    MiddleButtonPressEvent 129 88 0 0 0 a\n\
    StartInteractionEvent 129 88 0 0 0 a\n\
    TimerEvent 129 88 0 0 0 a\n\
    TimerEvent 129 88 0 0 0 a\n\
    TimerEvent 129 88 0 0 0 a\n\
    TimerEvent 129 88 0 0 0 a\n\
    TimerEvent 129 88 0 0 0 a\n\
    TimerEvent 129 88 0 0 0 a\n\
    MouseMoveEvent 129 87 0 0 0 a\n\
    InteractionEvent 129 87 0 0 0 a\n\
    TimerEvent 129 87 0 0 0 a\n\
    TimerEvent 129 87 0 0 0 a\n\
    TimerEvent 129 87 0 0 0 a\n\
    TimerEvent 129 87 0 0 0 a\n\
    TimerEvent 129 87 0 0 0 a\n\
    TimerEvent 129 87 0 0 0 a\n\
    TimerEvent 129 87 0 0 0 a\n\
    TimerEvent 129 87 0 0 0 a\n\
    MouseMoveEvent 129 87 0 0 0 a\n\
    InteractionEvent 129 87 0 0 0 a\n\
    TimerEvent 129 87 0 0 0 a\n\
    TimerEvent 129 87 0 0 0 a\n\
    TimerEvent 129 87 0 0 0 a\n\
    MouseMoveEvent 129 87 0 0 0 a\n\
    InteractionEvent 129 87 0 0 0 a\n\
    TimerEvent 129 87 0 0 0 a\n\
    MouseMoveEvent 129 86 0 0 0 a\n\
    InteractionEvent 129 86 0 0 0 a\n\
    TimerEvent 129 86 0 0 0 a\n\
    MouseMoveEvent 129 84 0 0 0 a\n\
    InteractionEvent 129 84 0 0 0 a\n\
    TimerEvent 129 84 0 0 0 a\n\
    MouseMoveEvent 128 81 0 0 0 a\n\
    InteractionEvent 128 81 0 0 0 a\n\
    TimerEvent 128 81 0 0 0 a\n\
    MouseMoveEvent 127 79 0 0 0 a\n\
    InteractionEvent 127 79 0 0 0 a\n\
    TimerEvent 127 79 0 0 0 a\n\
    MouseMoveEvent 127 75 0 0 0 a\n\
    InteractionEvent 127 75 0 0 0 a\n\
    TimerEvent 127 75 0 0 0 a\n\
    MouseMoveEvent 126 71 0 0 0 a\n\
    InteractionEvent 126 71 0 0 0 a\n\
    TimerEvent 126 71 0 0 0 a\n\
    MouseMoveEvent 125 71 0 0 0 a\n\
    InteractionEvent 125 71 0 0 0 a\n\
    TimerEvent 125 71 0 0 0 a\n\
    MouseMoveEvent 125 69 0 0 0 a\n\
    InteractionEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    TimerEvent 125 69 0 0 0 a\n\
    MiddleButtonReleaseEvent 125 69 0 0 0 a\n\
    EndInteractionEvent 125 69 0 0 0 a\n\
    MouseMoveEvent 125 70 0 0 0 a\n\
    MouseMoveEvent 124 70 0 0 0 a\n\
    MouseMoveEvent 124 71 0 0 0 a\n\
    MouseMoveEvent 124 73 0 0 0 a\n\
    MouseMoveEvent 124 76 0 0 0 a\n\
    MouseMoveEvent 124 78 0 0 0 a\n\
    MouseMoveEvent 124 82 0 0 0 a\n\
    MouseMoveEvent 124 85 0 0 0 a\n\
    MouseMoveEvent 124 90 0 0 0 a\n\
    MouseMoveEvent 125 92 0 0 0 a\n\
    MouseMoveEvent 127 97 0 0 0 a\n\
    MouseMoveEvent 127 100 0 0 0 a\n\
    MouseMoveEvent 127 105 0 0 0 a\n\
    MouseMoveEvent 128 107 0 0 0 a\n\
    MouseMoveEvent 129 112 0 0 0 a\n\
    MouseMoveEvent 129 117 0 0 0 a\n\
    MouseMoveEvent 130 119 0 0 0 a\n\
    MouseMoveEvent 131 124 0 0 0 a\n\
    MouseMoveEvent 131 126 0 0 0 a\n\
    MouseMoveEvent 131 129 0 0 0 a\n\
    MouseMoveEvent 131 131 0 0 0 a\n\
    MouseMoveEvent 131 133 0 0 0 a\n\
    MouseMoveEvent 131 135 0 0 0 a\n\
    MouseMoveEvent 132 136 0 0 0 a\n\
    MouseMoveEvent 132 137 0 0 0 a\n\
    MouseMoveEvent 133 139 0 0 0 a\n\
    MouseMoveEvent 133 140 0 0 0 a\n\
    MouseMoveEvent 134 141 0 0 0 a\n\
    MouseMoveEvent 134 142 0 0 0 a\n\
    MouseMoveEvent 135 143 0 0 0 a\n\
    MouseMoveEvent 135 144 0 0 0 a\n\
    MouseMoveEvent 136 145 0 0 0 a\n\
    MouseMoveEvent 137 145 0 0 0 a\n\
    MouseMoveEvent 137 146 0 0 0 a\n\
    MouseMoveEvent 138 146 0 0 0 a\n\
    MouseMoveEvent 138 147 0 0 0 a\n\
    MouseMoveEvent 139 148 0 0 0 a\n\
    MiddleButtonPressEvent 139 148 0 0 0 a\n\
    StartInteractionEvent 139 148 0 0 0 a\n\
    TimerEvent 139 148 0 0 0 a\n\
    TimerEvent 139 148 0 0 0 a\n\
    TimerEvent 139 148 0 0 0 a\n\
    MouseMoveEvent 139 148 0 0 0 a\n\
    InteractionEvent 139 148 0 0 0 a\n\
    TimerEvent 139 148 0 0 0 a\n\
    TimerEvent 139 148 0 0 0 a\n\
    MouseMoveEvent 139 149 0 0 0 a\n\
    InteractionEvent 139 149 0 0 0 a\n\
    TimerEvent 139 149 0 0 0 a\n\
    MouseMoveEvent 141 151 0 0 0 a\n\
    InteractionEvent 141 151 0 0 0 a\n\
    TimerEvent 141 151 0 0 0 a\n\
    MouseMoveEvent 142 154 0 0 0 a\n\
    InteractionEvent 142 154 0 0 0 a\n\
    TimerEvent 142 154 0 0 0 a\n\
    MouseMoveEvent 144 155 0 0 0 a\n\
    InteractionEvent 144 155 0 0 0 a\n\
    TimerEvent 144 155 0 0 0 a\n\
    MouseMoveEvent 146 157 0 0 0 a\n\
    InteractionEvent 146 157 0 0 0 a\n\
    TimerEvent 146 157 0 0 0 a\n\
    MouseMoveEvent 146 160 0 0 0 a\n\
    InteractionEvent 146 160 0 0 0 a\n\
    TimerEvent 146 160 0 0 0 a\n\
    MouseMoveEvent 148 162 0 0 0 a\n\
    InteractionEvent 148 162 0 0 0 a\n\
    TimerEvent 148 162 0 0 0 a\n\
    MouseMoveEvent 149 164 0 0 0 a\n\
    InteractionEvent 149 164 0 0 0 a\n\
    TimerEvent 149 164 0 0 0 a\n\
    MouseMoveEvent 150 165 0 0 0 a\n\
    InteractionEvent 150 165 0 0 0 a\n\
    TimerEvent 150 165 0 0 0 a\n\
    MouseMoveEvent 151 167 0 0 0 a\n\
    InteractionEvent 151 167 0 0 0 a\n\
    TimerEvent 151 167 0 0 0 a\n\
    MouseMoveEvent 152 168 0 0 0 a\n\
    InteractionEvent 152 168 0 0 0 a\n\
    TimerEvent 152 168 0 0 0 a\n\
    MouseMoveEvent 153 170 0 0 0 a\n\
    InteractionEvent 153 170 0 0 0 a\n\
    TimerEvent 153 170 0 0 0 a\n\
    TimerEvent 153 170 0 0 0 a\n\
    MouseMoveEvent 155 172 0 0 0 a\n\
    InteractionEvent 155 172 0 0 0 a\n\
    TimerEvent 155 172 0 0 0 a\n\
    MouseMoveEvent 156 173 0 0 0 a\n\
    InteractionEvent 156 173 0 0 0 a\n\
    TimerEvent 156 173 0 0 0 a\n\
    MouseMoveEvent 158 176 0 0 0 a\n\
    InteractionEvent 158 176 0 0 0 a\n\
    TimerEvent 158 176 0 0 0 a\n\
    MouseMoveEvent 160 179 0 0 0 a\n\
    InteractionEvent 160 179 0 0 0 a\n\
    TimerEvent 160 179 0 0 0 a\n\
    MouseMoveEvent 162 181 0 0 0 a\n\
    InteractionEvent 162 181 0 0 0 a\n\
    TimerEvent 162 181 0 0 0 a\n\
    MouseMoveEvent 165 183 0 0 0 a\n\
    InteractionEvent 165 183 0 0 0 a\n\
    TimerEvent 165 183 0 0 0 a\n\
    MouseMoveEvent 166 184 0 0 0 a\n\
    InteractionEvent 166 184 0 0 0 a\n\
    TimerEvent 166 184 0 0 0 a\n\
    TimerEvent 166 184 0 0 0 a\n\
    MouseMoveEvent 166 185 0 0 0 a\n\
    InteractionEvent 166 185 0 0 0 a\n\
    TimerEvent 166 185 0 0 0 a\n\
    MouseMoveEvent 167 185 0 0 0 a\n\
    InteractionEvent 167 185 0 0 0 a\n\
    TimerEvent 167 185 0 0 0 a\n\
    TimerEvent 167 185 0 0 0 a\n\
    MouseMoveEvent 168 185 0 0 0 a\n\
    InteractionEvent 168 185 0 0 0 a\n\
    TimerEvent 168 185 0 0 0 a\n\
    MouseMoveEvent 168 186 0 0 0 a\n\
    InteractionEvent 168 186 0 0 0 a\n\
    TimerEvent 168 186 0 0 0 a\n\
    MouseMoveEvent 170 186 0 0 0 a\n\
    InteractionEvent 170 186 0 0 0 a\n\
    TimerEvent 170 186 0 0 0 a\n\
    MouseMoveEvent 171 187 0 0 0 a\n\
    InteractionEvent 171 187 0 0 0 a\n\
    TimerEvent 171 187 0 0 0 a\n\
    MouseMoveEvent 171 188 0 0 0 a\n\
    InteractionEvent 171 188 0 0 0 a\n\
    TimerEvent 171 188 0 0 0 a\n\
    TimerEvent 171 188 0 0 0 a\n\
    MouseMoveEvent 172 190 0 0 0 a\n\
    InteractionEvent 172 190 0 0 0 a\n\
    TimerEvent 172 190 0 0 0 a\n\
    MouseMoveEvent 173 190 0 0 0 a\n\
    InteractionEvent 173 190 0 0 0 a\n\
    TimerEvent 173 190 0 0 0 a\n\
    MouseMoveEvent 175 192 0 0 0 a\n\
    InteractionEvent 175 192 0 0 0 a\n\
    TimerEvent 175 192 0 0 0 a\n\
    MouseMoveEvent 175 193 0 0 0 a\n\
    InteractionEvent 175 193 0 0 0 a\n\
    TimerEvent 175 193 0 0 0 a\n\
    MouseMoveEvent 176 194 0 0 0 a\n\
    InteractionEvent 176 194 0 0 0 a\n\
    TimerEvent 176 194 0 0 0 a\n\
    MouseMoveEvent 177 194 0 0 0 a\n\
    InteractionEvent 177 194 0 0 0 a\n\
    TimerEvent 177 194 0 0 0 a\n\
    MouseMoveEvent 177 196 0 0 0 a\n\
    InteractionEvent 177 196 0 0 0 a\n\
    TimerEvent 177 196 0 0 0 a\n\
    MouseMoveEvent 178 196 0 0 0 a\n\
    InteractionEvent 178 196 0 0 0 a\n\
    TimerEvent 178 196 0 0 0 a\n\
    MouseMoveEvent 179 196 0 0 0 a\n\
    InteractionEvent 179 196 0 0 0 a\n\
    TimerEvent 179 196 0 0 0 a\n\
    TimerEvent 179 196 0 0 0 a\n\
    MouseMoveEvent 179 197 0 0 0 a\n\
    InteractionEvent 179 197 0 0 0 a\n\
    TimerEvent 179 197 0 0 0 a\n\
    TimerEvent 179 197 0 0 0 a\n\
    TimerEvent 179 197 0 0 0 a\n\
    MouseMoveEvent 179 198 0 0 0 a\n\
    InteractionEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    MouseMoveEvent 179 198 0 0 0 a\n\
    InteractionEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    MouseMoveEvent 180 198 0 0 0 a\n\
    InteractionEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    MiddleButtonReleaseEvent 180 198 0 0 0 a\n\
    EndInteractionEvent 180 198 0 0 0 a\n\
    MouseMoveEvent 180 198 0 0 0 a\n\
    LeftButtonPressEvent 180 198 0 0 0 a\n\
    StartInteractionEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    TimerEvent 180 198 0 0 0 a\n\
    MouseMoveEvent 179 198 0 0 0 a\n\
    InteractionEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    MouseMoveEvent 179 198 0 0 0 a\n\
    InteractionEvent 179 198 0 0 0 a\n\
    TimerEvent 179 198 0 0 0 a\n\
    MouseMoveEvent 178 198 0 0 0 a\n\
    InteractionEvent 178 198 0 0 0 a\n\
    TimerEvent 178 198 0 0 0 a\n\
    MouseMoveEvent 176 198 0 0 0 a\n\
    InteractionEvent 176 198 0 0 0 a\n\
    TimerEvent 176 198 0 0 0 a\n\
    MouseMoveEvent 174 197 0 0 0 a\n\
    InteractionEvent 174 197 0 0 0 a\n\
    TimerEvent 174 197 0 0 0 a\n\
    MouseMoveEvent 170 197 0 0 0 a\n\
    InteractionEvent 170 197 0 0 0 a\n\
    TimerEvent 170 197 0 0 0 a\n\
    MouseMoveEvent 167 197 0 0 0 a\n\
    InteractionEvent 167 197 0 0 0 a\n\
    TimerEvent 167 197 0 0 0 a\n\
    MouseMoveEvent 164 196 0 0 0 a\n\
    InteractionEvent 164 196 0 0 0 a\n\
    TimerEvent 164 196 0 0 0 a\n\
    MouseMoveEvent 160 196 0 0 0 a\n\
    InteractionEvent 160 196 0 0 0 a\n\
    TimerEvent 160 196 0 0 0 a\n\
    MouseMoveEvent 154 196 0 0 0 a\n\
    InteractionEvent 154 196 0 0 0 a\n\
    TimerEvent 154 196 0 0 0 a\n\
    MouseMoveEvent 146 195 0 0 0 a\n\
    InteractionEvent 146 195 0 0 0 a\n\
    TimerEvent 146 195 0 0 0 a\n\
    MouseMoveEvent 134 195 0 0 0 a\n\
    InteractionEvent 134 195 0 0 0 a\n\
    TimerEvent 134 195 0 0 0 a\n\
    MouseMoveEvent 128 195 0 0 0 a\n\
    InteractionEvent 128 195 0 0 0 a\n\
    TimerEvent 128 195 0 0 0 a\n\
    MouseMoveEvent 124 194 0 0 0 a\n\
    InteractionEvent 124 194 0 0 0 a\n\
    TimerEvent 124 194 0 0 0 a\n\
    MouseMoveEvent 122 194 0 0 0 a\n\
    InteractionEvent 122 194 0 0 0 a\n\
    TimerEvent 122 194 0 0 0 a\n\
    MouseMoveEvent 120 194 0 0 0 a\n\
    InteractionEvent 120 194 0 0 0 a\n\
    TimerEvent 120 194 0 0 0 a\n\
    TimerEvent 120 194 0 0 0 a\n\
    MouseMoveEvent 119 193 0 0 0 a\n\
    InteractionEvent 119 193 0 0 0 a\n\
    TimerEvent 119 193 0 0 0 a\n\
    MouseMoveEvent 119 193 0 0 0 a\n\
    InteractionEvent 119 193 0 0 0 a\n\
    TimerEvent 119 193 0 0 0 a\n\
    TimerEvent 119 193 0 0 0 a\n\
    TimerEvent 119 193 0 0 0 a\n\
    TimerEvent 119 193 0 0 0 a\n\
    TimerEvent 119 193 0 0 0 a\n\
    MouseMoveEvent 117 193 0 0 0 a\n\
    InteractionEvent 117 193 0 0 0 a\n\
    TimerEvent 117 193 0 0 0 a\n\
    TimerEvent 117 193 0 0 0 a\n\
    MouseMoveEvent 113 192 0 0 0 a\n\
    InteractionEvent 113 192 0 0 0 a\n\
    TimerEvent 113 192 0 0 0 a\n\
    MouseMoveEvent 110 192 0 0 0 a\n\
    InteractionEvent 110 192 0 0 0 a\n\
    TimerEvent 110 192 0 0 0 a\n\
    MouseMoveEvent 108 192 0 0 0 a\n\
    InteractionEvent 108 192 0 0 0 a\n\
    TimerEvent 108 192 0 0 0 a\n\
    MouseMoveEvent 105 192 0 0 0 a\n\
    InteractionEvent 105 192 0 0 0 a\n\
    TimerEvent 105 192 0 0 0 a\n\
    MouseMoveEvent 103 191 0 0 0 a\n\
    InteractionEvent 103 191 0 0 0 a\n\
    TimerEvent 103 191 0 0 0 a\n\
    MouseMoveEvent 100 191 0 0 0 a\n\
    InteractionEvent 100 191 0 0 0 a\n\
    TimerEvent 100 191 0 0 0 a\n\
    MouseMoveEvent 98 191 0 0 0 a\n\
    InteractionEvent 98 191 0 0 0 a\n\
    TimerEvent 98 191 0 0 0 a\n\
    MouseMoveEvent 94 190 0 0 0 a\n\
    InteractionEvent 94 190 0 0 0 a\n\
    TimerEvent 94 190 0 0 0 a\n\
    MouseMoveEvent 89 190 0 0 0 a\n\
    InteractionEvent 89 190 0 0 0 a\n\
    TimerEvent 89 190 0 0 0 a\n\
    MouseMoveEvent 85 190 0 0 0 a\n\
    InteractionEvent 85 190 0 0 0 a\n\
    TimerEvent 85 190 0 0 0 a\n\
    MouseMoveEvent 81 189 0 0 0 a\n\
    InteractionEvent 81 189 0 0 0 a\n\
    TimerEvent 81 189 0 0 0 a\n\
    MouseMoveEvent 80 189 0 0 0 a\n\
    InteractionEvent 80 189 0 0 0 a\n\
    TimerEvent 80 189 0 0 0 a\n\
    MouseMoveEvent 79 189 0 0 0 a\n\
    InteractionEvent 79 189 0 0 0 a\n\
    TimerEvent 79 189 0 0 0 a\n\
    MouseMoveEvent 78 189 0 0 0 a\n\
    InteractionEvent 78 189 0 0 0 a\n\
    TimerEvent 78 189 0 0 0 a\n\
    TimerEvent 78 189 0 0 0 a\n\
    TimerEvent 78 189 0 0 0 a\n\
    MouseMoveEvent 76 189 0 0 0 a\n\
    InteractionEvent 76 189 0 0 0 a\n\
    TimerEvent 76 189 0 0 0 a\n\
    TimerEvent 76 189 0 0 0 a\n\
    MouseMoveEvent 73 189 0 0 0 a\n\
    InteractionEvent 73 189 0 0 0 a\n\
    TimerEvent 73 189 0 0 0 a\n\
    MouseMoveEvent 72 189 0 0 0 a\n\
    InteractionEvent 72 189 0 0 0 a\n\
    TimerEvent 72 189 0 0 0 a\n\
    MouseMoveEvent 71 189 0 0 0 a\n\
    InteractionEvent 71 189 0 0 0 a\n\
    TimerEvent 71 189 0 0 0 a\n\
    MouseMoveEvent 70 189 0 0 0 a\n\
    InteractionEvent 70 189 0 0 0 a\n\
    TimerEvent 70 189 0 0 0 a\n\
    MouseMoveEvent 69 189 0 0 0 a\n\
    InteractionEvent 69 189 0 0 0 a\n\
    TimerEvent 69 189 0 0 0 a\n\
    TimerEvent 69 189 0 0 0 a\n\
    MouseMoveEvent 69 189 0 0 0 a\n\
    InteractionEvent 69 189 0 0 0 a\n\
    TimerEvent 69 189 0 0 0 a\n\
    MouseMoveEvent 67 189 0 0 0 a\n\
    InteractionEvent 67 189 0 0 0 a\n\
    TimerEvent 67 189 0 0 0 a\n\
    MouseMoveEvent 66 189 0 0 0 a\n\
    InteractionEvent 66 189 0 0 0 a\n\
    TimerEvent 66 189 0 0 0 a\n\
    MouseMoveEvent 65 189 0 0 0 a\n\
    InteractionEvent 65 189 0 0 0 a\n\
    TimerEvent 65 189 0 0 0 a\n\
    MouseMoveEvent 63 189 0 0 0 a\n\
    InteractionEvent 63 189 0 0 0 a\n\
    TimerEvent 63 189 0 0 0 a\n\
    MouseMoveEvent 61 189 0 0 0 a\n\
    InteractionEvent 61 189 0 0 0 a\n\
    TimerEvent 61 189 0 0 0 a\n\
    MouseMoveEvent 60 189 0 0 0 a\n\
    InteractionEvent 60 189 0 0 0 a\n\
    TimerEvent 60 189 0 0 0 a\n\
    MouseMoveEvent 58 189 0 0 0 a\n\
    InteractionEvent 58 189 0 0 0 a\n\
    TimerEvent 58 189 0 0 0 a\n\
    MouseMoveEvent 56 189 0 0 0 a\n\
    InteractionEvent 56 189 0 0 0 a\n\
    TimerEvent 56 189 0 0 0 a\n\
    MouseMoveEvent 55 189 0 0 0 a\n\
    InteractionEvent 55 189 0 0 0 a\n\
    TimerEvent 55 189 0 0 0 a\n\
    MouseMoveEvent 54 189 0 0 0 a\n\
    InteractionEvent 54 189 0 0 0 a\n\
    TimerEvent 54 189 0 0 0 a\n\
    MouseMoveEvent 53 189 0 0 0 a\n\
    InteractionEvent 53 189 0 0 0 a\n\
    TimerEvent 53 189 0 0 0 a\n\
    MouseMoveEvent 52 189 0 0 0 a\n\
    InteractionEvent 52 189 0 0 0 a\n\
    TimerEvent 52 189 0 0 0 a\n\
    MouseMoveEvent 51 188 0 0 0 a\n\
    InteractionEvent 51 188 0 0 0 a\n\
    TimerEvent 51 188 0 0 0 a\n\
    TimerEvent 51 188 0 0 0 a\n\
    MouseMoveEvent 50 188 0 0 0 a\n\
    InteractionEvent 50 188 0 0 0 a\n\
    TimerEvent 50 188 0 0 0 a\n\
    TimerEvent 50 188 0 0 0 a\n\
    TimerEvent 50 188 0 0 0 a\n\
    TimerEvent 50 188 0 0 0 a\n\
    TimerEvent 50 188 0 0 0 a\n\
    TimerEvent 50 188 0 0 0 a\n\
    TimerEvent 50 188 0 0 0 a\n\
    TimerEvent 50 188 0 0 0 a\n\
    TimerEvent 50 188 0 0 0 a\n\
    TimerEvent 50 188 0 0 0 a\n\
    TimerEvent 50 188 0 0 0 a\n\
    TimerEvent 50 188 0 0 0 a\n\
    LeftButtonReleaseEvent 50 188 0 0 0 a\n\
    EndInteractionEvent 50 188 0 0 0 a\n\
    MouseMoveEvent 51 188 0 0 0 a\n\
    MouseMoveEvent 52 188 0 0 0 a\n\
    MouseMoveEvent 54 189 0 0 0 a\n\
    MouseMoveEvent 56 189 0 0 0 a\n\
    MouseMoveEvent 58 190 0 0 0 a\n\
    MouseMoveEvent 61 190 0 0 0 a\n\
    MouseMoveEvent 64 191 0 0 0 a\n\
    MouseMoveEvent 66 191 0 0 0 a\n\
    MouseMoveEvent 68 191 0 0 0 a\n\
    MouseMoveEvent 73 192 0 0 0 a\n\
    MouseMoveEvent 76 192 0 0 0 a\n\
    MouseMoveEvent 81 192 0 0 0 a\n\
    MouseMoveEvent 83 191 0 0 0 a\n\
    MouseMoveEvent 87 191 0 0 0 a\n\
    MouseMoveEvent 90 190 0 0 0 a\n\
    MouseMoveEvent 91 189 0 0 0 a\n\
    MouseMoveEvent 92 189 0 0 0 a\n\
    MouseMoveEvent 94 188 0 0 0 a\n\
    MouseMoveEvent 95 187 0 0 0 a\n\
    MouseMoveEvent 96 187 0 0 0 a\n\
    MouseMoveEvent 97 187 0 0 0 a\n\
    MouseMoveEvent 99 187 0 0 0 a\n\
    MouseMoveEvent 101 185 0 0 0 a\n\
    MouseMoveEvent 102 185 0 0 0 a\n\
    MouseMoveEvent 104 184 0 0 0 a\n\
    MouseMoveEvent 105 183 0 0 0 a\n\
    MouseMoveEvent 106 183 0 0 0 a\n\
    MouseMoveEvent 107 183 0 0 0 a\n\
    MouseMoveEvent 108 183 0 0 0 a\n\
    MouseMoveEvent 109 183 0 0 0 a\n\
    MouseMoveEvent 111 183 0 0 0 a\n\
    MouseMoveEvent 113 183 0 0 0 a\n\
    MouseMoveEvent 115 184 0 0 0 a\n\
    MouseMoveEvent 116 185 0 0 0 a\n\
    MouseMoveEvent 117 185 0 0 0 a\n\
    MouseMoveEvent 120 185 0 0 0 a\n\
    MouseMoveEvent 122 187 0 0 0 a\n\
    MouseMoveEvent 123 187 0 0 0 a\n\
    MouseMoveEvent 124 187 0 0 0 a\n\
    MouseMoveEvent 128 188 0 0 0 a\n\
    MouseMoveEvent 130 188 0 0 0 a\n\
    MouseMoveEvent 131 188 0 0 0 a\n\
    MouseMoveEvent 132 188 0 0 0 a\n\
    MouseMoveEvent 133 188 0 0 0 a\n\
    MouseMoveEvent 134 188 0 0 0 a\n\
    MouseMoveEvent 134 188 0 0 0 a\n\
    MouseMoveEvent 135 188 0 0 0 a\n\
    MouseMoveEvent 137 188 0 0 0 a\n\
    MouseMoveEvent 138 188 0 0 0 a\n\
    RightButtonPressEvent 138 188 0 0 0 a\n\
    StartInteractionEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    MouseMoveEvent 138 188 0 0 0 a\n\
    InteractionEvent 138 188 0 0 0 a\n\
    TimerEvent 138 188 0 0 0 a\n\
    MouseMoveEvent 137 188 0 0 0 a\n\
    InteractionEvent 137 188 0 0 0 a\n\
    TimerEvent 137 188 0 0 0 a\n\
    MouseMoveEvent 136 187 0 0 0 a\n\
    InteractionEvent 136 187 0 0 0 a\n\
    TimerEvent 136 187 0 0 0 a\n\
    MouseMoveEvent 133 185 0 0 0 a\n\
    InteractionEvent 133 185 0 0 0 a\n\
    TimerEvent 133 185 0 0 0 a\n\
    MouseMoveEvent 131 183 0 0 0 a\n\
    InteractionEvent 131 183 0 0 0 a\n\
    TimerEvent 131 183 0 0 0 a\n\
    MouseMoveEvent 128 180 0 0 0 a\n\
    InteractionEvent 128 180 0 0 0 a\n\
    TimerEvent 128 180 0 0 0 a\n\
    MouseMoveEvent 125 177 0 0 0 a\n\
    InteractionEvent 125 177 0 0 0 a\n\
    TimerEvent 125 177 0 0 0 a\n\
    MouseMoveEvent 123 175 0 0 0 a\n\
    InteractionEvent 123 175 0 0 0 a\n\
    TimerEvent 123 175 0 0 0 a\n\
    MouseMoveEvent 123 174 0 0 0 a\n\
    InteractionEvent 123 174 0 0 0 a\n\
    TimerEvent 123 174 0 0 0 a\n\
    MouseMoveEvent 121 172 0 0 0 a\n\
    InteractionEvent 121 172 0 0 0 a\n\
    TimerEvent 121 172 0 0 0 a\n\
    TimerEvent 121 172 0 0 0 a\n\
    MouseMoveEvent 121 171 0 0 0 a\n\
    InteractionEvent 121 171 0 0 0 a\n\
    TimerEvent 121 171 0 0 0 a\n\
    MouseMoveEvent 119 170 0 0 0 a\n\
    InteractionEvent 119 170 0 0 0 a\n\
    TimerEvent 119 170 0 0 0 a\n\
    TimerEvent 119 170 0 0 0 a\n\
    MouseMoveEvent 119 168 0 0 0 a\n\
    InteractionEvent 119 168 0 0 0 a\n\
    TimerEvent 119 168 0 0 0 a\n\
    MouseMoveEvent 118 167 0 0 0 a\n\
    InteractionEvent 118 167 0 0 0 a\n\
    TimerEvent 118 167 0 0 0 a\n\
    MouseMoveEvent 117 167 0 0 0 a\n\
    InteractionEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    TimerEvent 117 167 0 0 0 a\n\
    MouseMoveEvent 116 166 0 0 0 a\n\
    InteractionEvent 116 166 0 0 0 a\n\
    TimerEvent 116 166 0 0 0 a\n\
    TimerEvent 116 166 0 0 0 a\n\
    TimerEvent 116 166 0 0 0 a\n\
    TimerEvent 116 166 0 0 0 a\n\
    TimerEvent 116 166 0 0 0 a\n\
    TimerEvent 116 166 0 0 0 a\n\
    MouseMoveEvent 116 167 0 0 0 a\n\
    InteractionEvent 116 167 0 0 0 a\n\
    TimerEvent 116 167 0 0 0 a\n\
    MouseMoveEvent 116 173 0 0 0 a\n\
    InteractionEvent 116 173 0 0 0 a\n\
    TimerEvent 116 173 0 0 0 a\n\
    MouseMoveEvent 116 178 0 0 0 a\n\
    InteractionEvent 116 178 0 0 0 a\n\
    TimerEvent 116 178 0 0 0 a\n\
    RightButtonReleaseEvent 116 176 0 0 0 a\n\
"

recorder = vtkInteractorEventRecorder()
recorder.SetInteractor(iRen)
recorder.SetFileName("record.log")
#recorder.On()
#recorder.Record()
recorder.ReadFromInputStringOn()
recorder.SetInputString(Recording)

# Render and play the events
recorder.Play()
iRen.Start()
