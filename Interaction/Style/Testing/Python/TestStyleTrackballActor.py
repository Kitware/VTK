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
    KeyPressEvent 138 165 0 116 1 t\n\
    CharEvent 138 165 0 116 1 t\n\
    KeyReleaseEvent 138 165 0 116 1 t\n\
    KeyPressEvent 138 165 0 97 1 a\n\
    CharEvent 138 165 0 97 1 a\n\
    KeyReleaseEvent 138 165 0 97 1 a\n\
    MouseMoveEvent 138 165 0 0 0 a\n\
    MouseMoveEvent 140 165 0 0 0 a\n\
    MouseMoveEvent 141 164 0 0 0 a\n\
    MouseMoveEvent 142 163 0 0 0 a\n\
    MouseMoveEvent 143 162 0 0 0 a\n\
    MouseMoveEvent 144 161 0 0 0 a\n\
    MouseMoveEvent 145 160 0 0 0 a\n\
    MouseMoveEvent 146 160 0 0 0 a\n\
    MouseMoveEvent 147 158 0 0 0 a\n\
    MouseMoveEvent 148 158 0 0 0 a\n\
    MouseMoveEvent 149 157 0 0 0 a\n\
    MouseMoveEvent 150 156 0 0 0 a\n\
    MouseMoveEvent 151 155 0 0 0 a\n\
    MouseMoveEvent 152 154 0 0 0 a\n\
    MouseMoveEvent 153 153 0 0 0 a\n\
    MouseMoveEvent 154 152 0 0 0 a\n\
    MouseMoveEvent 155 151 0 0 0 a\n\
    MouseMoveEvent 156 150 0 0 0 a\n\
    MouseMoveEvent 157 149 0 0 0 a\n\
    MouseMoveEvent 158 148 0 0 0 a\n\
    MouseMoveEvent 159 147 0 0 0 a\n\
    MouseMoveEvent 160 146 0 0 0 a\n\
    MouseMoveEvent 160 146 0 0 0 a\n\
    MouseMoveEvent 160 147 0 0 0 a\n\
    MouseMoveEvent 161 148 0 0 0 a\n\
    MouseMoveEvent 161 150 0 0 0 a\n\
    MouseMoveEvent 162 150 0 0 0 a\n\
    MouseMoveEvent 162 151 0 0 0 a\n\
    MouseMoveEvent 163 152 0 0 0 a\n\
    MouseMoveEvent 163 153 0 0 0 a\n\
    MouseMoveEvent 164 153 0 0 0 a\n\
    MouseMoveEvent 164 154 0 0 0 a\n\
    MouseMoveEvent 165 155 0 0 0 a\n\
    MouseMoveEvent 166 155 0 0 0 a\n\
    MouseMoveEvent 166 156 0 0 0 a\n\
    MouseMoveEvent 167 157 0 0 0 a\n\
    MouseMoveEvent 168 158 0 0 0 a\n\
    MouseMoveEvent 169 159 0 0 0 a\n\
    MouseMoveEvent 170 160 0 0 0 a\n\
    MouseMoveEvent 170 162 0 0 0 a\n\
    MouseMoveEvent 171 162 0 0 0 a\n\
    MouseMoveEvent 173 164 0 0 0 a\n\
    MouseMoveEvent 174 164 0 0 0 a\n\
    MouseMoveEvent 175 164 0 0 0 a\n\
    MouseMoveEvent 177 164 0 0 0 a\n\
    MouseMoveEvent 178 164 0 0 0 a\n\
    MouseMoveEvent 179 164 0 0 0 a\n\
    MouseMoveEvent 180 164 0 0 0 a\n\
    MouseMoveEvent 180 165 0 0 0 a\n\
    MouseMoveEvent 181 165 0 0 0 a\n\
    MouseMoveEvent 181 166 0 0 0 a\n\
    MouseMoveEvent 181 168 0 0 0 a\n\
    MouseMoveEvent 182 168 0 0 0 a\n\
    MouseMoveEvent 182 168 0 0 0 a\n\
    MouseMoveEvent 182 170 0 0 0 a\n\
    MouseMoveEvent 183 170 0 0 0 a\n\
    MouseMoveEvent 183 171 0 0 0 a\n\
    MouseMoveEvent 183 171 0 0 0 a\n\
    LeftButtonPressEvent 183 171 0 0 0 a\n\
    StartInteractionEvent 183 171 0 0 0 a\n\
    MouseMoveEvent 184 171 0 0 0 a\n\
    InteractionEvent 184 171 0 0 0 a\n\
    MouseMoveEvent 184 171 0 0 0 a\n\
    InteractionEvent 184 171 0 0 0 a\n\
    MouseMoveEvent 185 171 0 0 0 a\n\
    InteractionEvent 185 171 0 0 0 a\n\
    MouseMoveEvent 186 171 0 0 0 a\n\
    InteractionEvent 186 171 0 0 0 a\n\
    MouseMoveEvent 187 171 0 0 0 a\n\
    InteractionEvent 187 171 0 0 0 a\n\
    MouseMoveEvent 188 171 0 0 0 a\n\
    InteractionEvent 188 171 0 0 0 a\n\
    MouseMoveEvent 189 171 0 0 0 a\n\
    InteractionEvent 189 171 0 0 0 a\n\
    MouseMoveEvent 190 171 0 0 0 a\n\
    InteractionEvent 190 171 0 0 0 a\n\
    MouseMoveEvent 191 171 0 0 0 a\n\
    InteractionEvent 191 171 0 0 0 a\n\
    MouseMoveEvent 192 171 0 0 0 a\n\
    InteractionEvent 192 171 0 0 0 a\n\
    MouseMoveEvent 193 171 0 0 0 a\n\
    InteractionEvent 193 171 0 0 0 a\n\
    MouseMoveEvent 194 169 0 0 0 a\n\
    InteractionEvent 194 169 0 0 0 a\n\
    MouseMoveEvent 195 169 0 0 0 a\n\
    InteractionEvent 195 169 0 0 0 a\n\
    MouseMoveEvent 195 169 0 0 0 a\n\
    InteractionEvent 195 169 0 0 0 a\n\
    MouseMoveEvent 196 169 0 0 0 a\n\
    InteractionEvent 196 169 0 0 0 a\n\
    MouseMoveEvent 197 168 0 0 0 a\n\
    InteractionEvent 197 168 0 0 0 a\n\
    MouseMoveEvent 198 168 0 0 0 a\n\
    InteractionEvent 198 168 0 0 0 a\n\
    MouseMoveEvent 198 168 0 0 0 a\n\
    InteractionEvent 198 168 0 0 0 a\n\
    MouseMoveEvent 199 168 0 0 0 a\n\
    InteractionEvent 199 168 0 0 0 a\n\
    MouseMoveEvent 200 168 0 0 0 a\n\
    InteractionEvent 200 168 0 0 0 a\n\
    MouseMoveEvent 201 168 0 0 0 a\n\
    InteractionEvent 201 168 0 0 0 a\n\
    MouseMoveEvent 202 168 0 0 0 a\n\
    InteractionEvent 202 168 0 0 0 a\n\
    MouseMoveEvent 203 168 0 0 0 a\n\
    InteractionEvent 203 168 0 0 0 a\n\
    MouseMoveEvent 205 167 0 0 0 a\n\
    InteractionEvent 205 167 0 0 0 a\n\
    MouseMoveEvent 207 167 0 0 0 a\n\
    InteractionEvent 207 167 0 0 0 a\n\
    MouseMoveEvent 208 167 0 0 0 a\n\
    InteractionEvent 208 167 0 0 0 a\n\
    MouseMoveEvent 208 167 0 0 0 a\n\
    InteractionEvent 208 167 0 0 0 a\n\
    MouseMoveEvent 209 167 0 0 0 a\n\
    InteractionEvent 209 167 0 0 0 a\n\
    MouseMoveEvent 210 167 0 0 0 a\n\
    InteractionEvent 210 167 0 0 0 a\n\
    MouseMoveEvent 210 167 0 0 0 a\n\
    InteractionEvent 210 167 0 0 0 a\n\
    MouseMoveEvent 211 167 0 0 0 a\n\
    InteractionEvent 211 167 0 0 0 a\n\
    MouseMoveEvent 212 167 0 0 0 a\n\
    InteractionEvent 212 167 0 0 0 a\n\
    MouseMoveEvent 213 167 0 0 0 a\n\
    InteractionEvent 213 167 0 0 0 a\n\
    MouseMoveEvent 214 166 0 0 0 a\n\
    InteractionEvent 214 166 0 0 0 a\n\
    MouseMoveEvent 214 166 0 0 0 a\n\
    InteractionEvent 214 166 0 0 0 a\n\
    MouseMoveEvent 215 166 0 0 0 a\n\
    InteractionEvent 215 166 0 0 0 a\n\
    MouseMoveEvent 216 166 0 0 0 a\n\
    InteractionEvent 216 166 0 0 0 a\n\
    MouseMoveEvent 218 165 0 0 0 a\n\
    InteractionEvent 218 165 0 0 0 a\n\
    MouseMoveEvent 219 165 0 0 0 a\n\
    InteractionEvent 219 165 0 0 0 a\n\
    MouseMoveEvent 220 165 0 0 0 a\n\
    InteractionEvent 220 165 0 0 0 a\n\
    MouseMoveEvent 221 165 0 0 0 a\n\
    InteractionEvent 221 165 0 0 0 a\n\
    MouseMoveEvent 222 165 0 0 0 a\n\
    InteractionEvent 222 165 0 0 0 a\n\
    MouseMoveEvent 224 164 0 0 0 a\n\
    InteractionEvent 224 164 0 0 0 a\n\
    MouseMoveEvent 226 164 0 0 0 a\n\
    InteractionEvent 226 164 0 0 0 a\n\
    MouseMoveEvent 228 164 0 0 0 a\n\
    InteractionEvent 228 164 0 0 0 a\n\
    MouseMoveEvent 231 163 0 0 0 a\n\
    InteractionEvent 231 163 0 0 0 a\n\
    MouseMoveEvent 235 163 0 0 0 a\n\
    InteractionEvent 235 163 0 0 0 a\n\
    MouseMoveEvent 237 161 0 0 0 a\n\
    InteractionEvent 237 161 0 0 0 a\n\
    MouseMoveEvent 239 162 0 0 0 a\n\
    InteractionEvent 239 162 0 0 0 a\n\
    MouseMoveEvent 241 162 0 0 0 a\n\
    InteractionEvent 241 162 0 0 0 a\n\
    MouseMoveEvent 242 162 0 0 0 a\n\
    InteractionEvent 242 162 0 0 0 a\n\
    MouseMoveEvent 244 161 0 0 0 a\n\
    InteractionEvent 244 161 0 0 0 a\n\
    LeftButtonReleaseEvent 244 161 0 0 0 a\n\
    EndInteractionEvent 244 161 0 0 0 a\n\
    MouseMoveEvent 244 161 0 0 0 a\n\
    MouseMoveEvent 243 161 0 0 0 a\n\
    MouseMoveEvent 242 160 0 0 0 a\n\
    MouseMoveEvent 242 159 0 0 0 a\n\
    MouseMoveEvent 240 159 0 0 0 a\n\
    MouseMoveEvent 239 158 0 0 0 a\n\
    MouseMoveEvent 238 157 0 0 0 a\n\
    MouseMoveEvent 237 157 0 0 0 a\n\
    MouseMoveEvent 236 156 0 0 0 a\n\
    MouseMoveEvent 235 155 0 0 0 a\n\
    MouseMoveEvent 232 154 0 0 0 a\n\
    MouseMoveEvent 231 152 0 0 0 a\n\
    MouseMoveEvent 230 152 0 0 0 a\n\
    MouseMoveEvent 228 151 0 0 0 a\n\
    MouseMoveEvent 226 150 0 0 0 a\n\
    MouseMoveEvent 225 149 0 0 0 a\n\
    MouseMoveEvent 223 149 0 0 0 a\n\
    MouseMoveEvent 222 147 0 0 0 a\n\
    MouseMoveEvent 220 147 0 0 0 a\n\
    MouseMoveEvent 218 146 0 0 0 a\n\
    MouseMoveEvent 216 144 0 0 0 a\n\
    MouseMoveEvent 212 143 0 0 0 a\n\
    MouseMoveEvent 209 140 0 0 0 a\n\
    MouseMoveEvent 204 136 0 0 0 a\n\
    MouseMoveEvent 201 135 0 0 0 a\n\
    MouseMoveEvent 199 133 0 0 0 a\n\
    MouseMoveEvent 196 131 0 0 0 a\n\
    MouseMoveEvent 192 128 0 0 0 a\n\
    MouseMoveEvent 190 127 0 0 0 a\n\
    MouseMoveEvent 188 126 0 0 0 a\n\
    MouseMoveEvent 184 125 0 0 0 a\n\
    MouseMoveEvent 183 125 0 0 0 a\n\
    MouseMoveEvent 181 123 0 0 0 a\n\
    MouseMoveEvent 180 123 0 0 0 a\n\
    MouseMoveEvent 179 123 0 0 0 a\n\
    MouseMoveEvent 178 123 0 0 0 a\n\
    MouseMoveEvent 176 122 0 0 0 a\n\
    MouseMoveEvent 175 122 0 0 0 a\n\
    MouseMoveEvent 174 122 0 0 0 a\n\
    MouseMoveEvent 173 122 0 0 0 a\n\
    MouseMoveEvent 172 121 0 0 0 a\n\
    MouseMoveEvent 171 121 0 0 0 a\n\
    MouseMoveEvent 170 121 0 0 0 a\n\
    MouseMoveEvent 169 121 0 0 0 a\n\
    MouseMoveEvent 168 121 0 0 0 a\n\
    MouseMoveEvent 168 122 0 0 0 a\n\
    MouseMoveEvent 167 122 0 0 0 a\n\
    MouseMoveEvent 166 123 0 0 0 a\n\
    MouseMoveEvent 165 123 0 0 0 a\n\
    MouseMoveEvent 164 123 0 0 0 a\n\
    MouseMoveEvent 163 124 0 0 0 a\n\
    MiddleButtonPressEvent 163 124 0 0 0 a\n\
    StartInteractionEvent 163 124 0 0 0 a\n\
    MouseMoveEvent 163 124 0 0 0 a\n\
    InteractionEvent 163 124 0 0 0 a\n\
    MouseMoveEvent 163 124 0 0 0 a\n\
    InteractionEvent 163 124 0 0 0 a\n\
    MouseMoveEvent 163 125 0 0 0 a\n\
    InteractionEvent 163 125 0 0 0 a\n\
    MouseMoveEvent 162 127 0 0 0 a\n\
    InteractionEvent 162 127 0 0 0 a\n\
    MouseMoveEvent 162 130 0 0 0 a\n\
    InteractionEvent 162 130 0 0 0 a\n\
    MouseMoveEvent 161 132 0 0 0 a\n\
    InteractionEvent 161 132 0 0 0 a\n\
    MouseMoveEvent 161 134 0 0 0 a\n\
    InteractionEvent 161 134 0 0 0 a\n\
    MouseMoveEvent 161 136 0 0 0 a\n\
    InteractionEvent 161 136 0 0 0 a\n\
    MouseMoveEvent 160 137 0 0 0 a\n\
    InteractionEvent 160 137 0 0 0 a\n\
    MouseMoveEvent 160 139 0 0 0 a\n\
    InteractionEvent 160 139 0 0 0 a\n\
    MouseMoveEvent 159 141 0 0 0 a\n\
    InteractionEvent 159 141 0 0 0 a\n\
    MouseMoveEvent 159 142 0 0 0 a\n\
    InteractionEvent 159 142 0 0 0 a\n\
    MouseMoveEvent 159 143 0 0 0 a\n\
    InteractionEvent 159 143 0 0 0 a\n\
    MouseMoveEvent 159 144 0 0 0 a\n\
    InteractionEvent 159 144 0 0 0 a\n\
    MouseMoveEvent 159 145 0 0 0 a\n\
    InteractionEvent 159 145 0 0 0 a\n\
    MouseMoveEvent 159 146 0 0 0 a\n\
    InteractionEvent 159 146 0 0 0 a\n\
    MouseMoveEvent 159 147 0 0 0 a\n\
    InteractionEvent 159 147 0 0 0 a\n\
    MouseMoveEvent 159 148 0 0 0 a\n\
    InteractionEvent 159 148 0 0 0 a\n\
    MouseMoveEvent 158 149 0 0 0 a\n\
    InteractionEvent 158 149 0 0 0 a\n\
    MouseMoveEvent 158 150 0 0 0 a\n\
    InteractionEvent 158 150 0 0 0 a\n\
    MouseMoveEvent 156 153 0 0 0 a\n\
    InteractionEvent 156 153 0 0 0 a\n\
    MouseMoveEvent 156 154 0 0 0 a\n\
    InteractionEvent 156 154 0 0 0 a\n\
    MouseMoveEvent 156 155 0 0 0 a\n\
    InteractionEvent 156 155 0 0 0 a\n\
    MouseMoveEvent 156 156 0 0 0 a\n\
    InteractionEvent 156 156 0 0 0 a\n\
    MouseMoveEvent 155 156 0 0 0 a\n\
    InteractionEvent 155 156 0 0 0 a\n\
    MouseMoveEvent 154 157 0 0 0 a\n\
    InteractionEvent 154 157 0 0 0 a\n\
    MouseMoveEvent 154 158 0 0 0 a\n\
    InteractionEvent 154 158 0 0 0 a\n\
    MouseMoveEvent 154 159 0 0 0 a\n\
    InteractionEvent 154 159 0 0 0 a\n\
    MiddleButtonReleaseEvent 154 159 0 0 0 a\n\
    EndInteractionEvent 154 159 0 0 0 a\n\
    MouseMoveEvent 154 159 0 0 0 a\n\
    MouseMoveEvent 155 158 0 0 0 a\n\
    MouseMoveEvent 156 156 0 0 0 a\n\
    MouseMoveEvent 158 153 0 0 0 a\n\
    MouseMoveEvent 159 152 0 0 0 a\n\
    MouseMoveEvent 159 151 0 0 0 a\n\
    MouseMoveEvent 160 147 0 0 0 a\n\
    MouseMoveEvent 161 145 0 0 0 a\n\
    MouseMoveEvent 162 143 0 0 0 a\n\
    MouseMoveEvent 163 141 0 0 0 a\n\
    MouseMoveEvent 164 139 0 0 0 a\n\
    MouseMoveEvent 165 136 0 0 0 a\n\
    MouseMoveEvent 166 135 0 0 0 a\n\
    MouseMoveEvent 166 134 0 0 0 a\n\
    MouseMoveEvent 167 132 0 0 0 a\n\
    MouseMoveEvent 169 129 0 0 0 a\n\
    MouseMoveEvent 169 128 0 0 0 a\n\
    MouseMoveEvent 170 127 0 0 0 a\n\
    MouseMoveEvent 170 126 0 0 0 a\n\
    MouseMoveEvent 171 125 0 0 0 a\n\
    MouseMoveEvent 171 124 0 0 0 a\n\
    MouseMoveEvent 171 124 0 0 0 a\n\
    MouseMoveEvent 172 123 0 0 0 a\n\
    MouseMoveEvent 172 122 0 0 0 a\n\
    MouseMoveEvent 172 121 0 0 0 a\n\
    MouseMoveEvent 172 120 0 0 0 a\n\
    MouseMoveEvent 173 119 0 0 0 a\n\
    MouseMoveEvent 173 118 0 0 0 a\n\
    MouseMoveEvent 173 117 0 0 0 a\n\
    MouseMoveEvent 173 116 0 0 0 a\n\
    MouseMoveEvent 173 115 0 0 0 a\n\
    MouseMoveEvent 173 114 0 0 0 a\n\
    MouseMoveEvent 174 113 0 0 0 a\n\
    MouseMoveEvent 173 113 0 0 0 a\n\
    MouseMoveEvent 171 113 0 0 0 a\n\
    MouseMoveEvent 170 113 0 0 0 a\n\
    MouseMoveEvent 169 113 0 0 0 a\n\
    MouseMoveEvent 167 113 0 0 0 a\n\
    MouseMoveEvent 167 113 0 0 0 a\n\
    MouseMoveEvent 166 113 0 0 0 a\n\
    MouseMoveEvent 165 114 0 0 0 a\n\
    MouseMoveEvent 164 114 0 0 0 a\n\
    RightButtonPressEvent 164 114 0 0 0 a\n\
    StartInteractionEvent 164 114 0 0 0 a\n\
    MouseMoveEvent 164 114 0 0 0 a\n\
    InteractionEvent 164 114 0 0 0 a\n\
    MouseMoveEvent 164 113 0 0 0 a\n\
    InteractionEvent 164 113 0 0 0 a\n\
    MouseMoveEvent 164 112 0 0 0 a\n\
    InteractionEvent 164 112 0 0 0 a\n\
    MouseMoveEvent 164 111 0 0 0 a\n\
    InteractionEvent 164 111 0 0 0 a\n\
    MouseMoveEvent 164 109 0 0 0 a\n\
    InteractionEvent 164 109 0 0 0 a\n\
    MouseMoveEvent 165 107 0 0 0 a\n\
    InteractionEvent 165 107 0 0 0 a\n\
    MouseMoveEvent 165 106 0 0 0 a\n\
    InteractionEvent 165 106 0 0 0 a\n\
    MouseMoveEvent 165 106 0 0 0 a\n\
    InteractionEvent 165 106 0 0 0 a\n\
    MouseMoveEvent 166 105 0 0 0 a\n\
    InteractionEvent 166 105 0 0 0 a\n\
    MouseMoveEvent 166 103 0 0 0 a\n\
    InteractionEvent 166 103 0 0 0 a\n\
    MouseMoveEvent 166 101 0 0 0 a\n\
    InteractionEvent 166 101 0 0 0 a\n\
    MouseMoveEvent 166 102 0 0 0 a\n\
    InteractionEvent 166 102 0 0 0 a\n\
    MouseMoveEvent 166 103 0 0 0 a\n\
    InteractionEvent 166 103 0 0 0 a\n\
    MouseMoveEvent 166 104 0 0 0 a\n\
    InteractionEvent 166 104 0 0 0 a\n\
    MouseMoveEvent 166 105 0 0 0 a\n\
    InteractionEvent 166 105 0 0 0 a\n\
    MouseMoveEvent 165 107 0 0 0 a\n\
    InteractionEvent 165 107 0 0 0 a\n\
    MouseMoveEvent 165 108 0 0 0 a\n\
    InteractionEvent 165 108 0 0 0 a\n\
    MouseMoveEvent 164 110 0 0 0 a\n\
    InteractionEvent 164 110 0 0 0 a\n\
    MouseMoveEvent 163 111 0 0 0 a\n\
    InteractionEvent 163 111 0 0 0 a\n\
    MouseMoveEvent 163 112 0 0 0 a\n\
    InteractionEvent 163 112 0 0 0 a\n\
    MouseMoveEvent 163 113 0 0 0 a\n\
    InteractionEvent 163 113 0 0 0 a\n\
    MouseMoveEvent 162 114 0 0 0 a\n\
    InteractionEvent 162 114 0 0 0 a\n\
    MouseMoveEvent 161 114 0 0 0 a\n\
    InteractionEvent 161 114 0 0 0 a\n\
    MouseMoveEvent 161 115 0 0 0 a\n\
    InteractionEvent 161 115 0 0 0 a\n\
    MouseMoveEvent 161 116 0 0 0 a\n\
    InteractionEvent 161 116 0 0 0 a\n\
    MouseMoveEvent 161 116 0 0 0 a\n\
    InteractionEvent 161 116 0 0 0 a\n\
    MouseMoveEvent 161 117 0 0 0 a\n\
    InteractionEvent 161 117 0 0 0 a\n\
    MouseMoveEvent 161 118 0 0 0 a\n\
    InteractionEvent 161 118 0 0 0 a\n\
    MouseMoveEvent 160 119 0 0 0 a\n\
    InteractionEvent 160 119 0 0 0 a\n\
    MouseMoveEvent 160 121 0 0 0 a\n\
    InteractionEvent 160 121 0 0 0 a\n\
    MouseMoveEvent 159 122 0 0 0 a\n\
    InteractionEvent 159 122 0 0 0 a\n\
    MouseMoveEvent 159 123 0 0 0 a\n\
    InteractionEvent 159 123 0 0 0 a\n\
    MouseMoveEvent 158 124 0 0 0 a\n\
    InteractionEvent 158 124 0 0 0 a\n\
    MouseMoveEvent 158 125 0 0 0 a\n\
    InteractionEvent 158 125 0 0 0 a\n\
    MouseMoveEvent 157 127 0 0 0 a\n\
    InteractionEvent 157 127 0 0 0 a\n\
    MouseMoveEvent 157 128 0 0 0 a\n\
    InteractionEvent 157 128 0 0 0 a\n\
    MouseMoveEvent 156 128 0 0 0 a\n\
    InteractionEvent 156 128 0 0 0 a\n\
    MouseMoveEvent 156 129 0 0 0 a\n\
    InteractionEvent 156 129 0 0 0 a\n\
    MouseMoveEvent 155 130 0 0 0 a\n\
    InteractionEvent 155 130 0 0 0 a\n\
    MouseMoveEvent 155 130 0 0 0 a\n\
    InteractionEvent 155 130 0 0 0 a\n\
    MouseMoveEvent 155 131 0 0 0 a\n\
    InteractionEvent 155 131 0 0 0 a\n\
    MouseMoveEvent 155 132 0 0 0 a\n\
    InteractionEvent 155 132 0 0 0 a\n\
    MouseMoveEvent 154 133 0 0 0 a\n\
    InteractionEvent 154 133 0 0 0 a\n\
    MouseMoveEvent 154 134 0 0 0 a\n\
    InteractionEvent 154 134 0 0 0 a\n\
    MouseMoveEvent 153 135 0 0 0 a\n\
    InteractionEvent 153 135 0 0 0 a\n\
    MouseMoveEvent 153 136 0 0 0 a\n\
    InteractionEvent 153 136 0 0 0 a\n\
    MouseMoveEvent 153 138 0 0 0 a\n\
    InteractionEvent 153 138 0 0 0 a\n\
    MouseMoveEvent 152 139 0 0 0 a\n\
    InteractionEvent 152 139 0 0 0 a\n\
    MouseMoveEvent 151 140 0 0 0 a\n\
    InteractionEvent 151 140 0 0 0 a\n\
    MouseMoveEvent 151 141 0 0 0 a\n\
    InteractionEvent 151 141 0 0 0 a\n\
    MouseMoveEvent 150 143 0 0 0 a\n\
    InteractionEvent 150 143 0 0 0 a\n\
    MouseMoveEvent 150 145 0 0 0 a\n\
    InteractionEvent 150 145 0 0 0 a\n\
    MouseMoveEvent 149 146 0 0 0 a\n\
    InteractionEvent 149 146 0 0 0 a\n\
    MouseMoveEvent 148 147 0 0 0 a\n\
    InteractionEvent 148 147 0 0 0 a\n\
    MouseMoveEvent 148 148 0 0 0 a\n\
    InteractionEvent 148 148 0 0 0 a\n\
    MouseMoveEvent 147 149 0 0 0 a\n\
    InteractionEvent 147 149 0 0 0 a\n\
    MouseMoveEvent 147 149 0 0 0 a\n\
    InteractionEvent 147 149 0 0 0 a\n\
    MouseMoveEvent 147 150 0 0 0 a\n\
    InteractionEvent 147 150 0 0 0 a\n\
    MouseMoveEvent 146 152 0 0 0 a\n\
    InteractionEvent 146 152 0 0 0 a\n\
    MouseMoveEvent 146 153 0 0 0 a\n\
    InteractionEvent 146 153 0 0 0 a\n\
    MouseMoveEvent 145 154 0 0 0 a\n\
    InteractionEvent 145 154 0 0 0 a\n\
    MouseMoveEvent 144 155 0 0 0 a\n\
    InteractionEvent 144 155 0 0 0 a\n\
    MouseMoveEvent 144 156 0 0 0 a\n\
    InteractionEvent 144 156 0 0 0 a\n\
    RightButtonReleaseEvent 144 156 0 0 0 a\n\
    EndInteractionEvent 144 156 0 0 0 a\n\
    MouseMoveEvent 144 155 0 0 0 a\n\
    MouseMoveEvent 144 155 0 0 0 a\n\
"

recorder = vtkInteractorEventRecorder()
recorder.SetInteractor(iRen)
#recorder.SetFileName("record.log")
#recorder.On()
#recorder.Record()
recorder.ReadFromInputStringOn()
recorder.SetInputString(Recording)

# Render and play the events
recorder.Play()
iRen.Start()
