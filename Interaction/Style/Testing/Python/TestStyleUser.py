#!/usr/bin/env python
import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script exercises vtkInteractorStyleUser. It creates some
# simple bindings for the mouse bottons and scroll wheel.

# Helper math stuff
import math
def magnitude(v):
    return math.sqrt(sum(v[i]*v[i] for i in range(0,3)))
def dot(u, v):
    return sum(u[i]*v[i] for i in range(0,3))
def normalize(v):
    vmag = magnitude(v)
    return [ v[i]/vmag  for i in range(0,3) ]

# Variables local to this module
left = 0
middle = 0
right = 0

oldPickD = []
newPickD = []
oldPickW = [0,0,0,0]
newPickW = [0,0,0,0]
fpD = [0,0,0]
motionD = [0,0]
motionW = [0,0,0]
cam = ""

# The binding actions are defined below
# This general method computes some common information from mouse
# motion and button presses.
def getMotion(iRen,ren):
    global oldPickD, newPickD, oldPickW, newPickW, fpD, fpW, posW
    global motionD, motionW, cam
    style = iRen.GetInteractorStyle()
    oldPickD = iRen.GetLastEventPosition()
    newPickD = iRen.GetEventPosition()
    motionD[0] = newPickD[0] - oldPickD[0];
    motionD[1] = newPickD[1] - oldPickD[1];
    cam = ren.GetActiveCamera()
    fpW = cam.GetFocalPoint()
    posW = cam.GetPosition()
    style.ComputeWorldToDisplay(ren, fpW[0], fpW[1], fpW[2], fpD)
    focalDepth = fpD[2]
    style.ComputeDisplayToWorld(ren,oldPickD[0],oldPickD[1],focalDepth,oldPickW)
    style.ComputeDisplayToWorld(ren,newPickD[0],newPickD[1],focalDepth,newPickW)
    motionW[0] = oldPickW[0] - newPickW[0]
    motionW[1] = oldPickW[1] - newPickW[1]
    motionW[2] = oldPickW[2] - newPickW[2]

# Left mouse button is pan
def mouseLeftMove(iRen,ren):
    getMotion(iRen,ren)
    cam.SetFocalPoint(fpW[0]+motionW[0],fpW[1]+motionW[1],fpW[2]+motionW[2])
    cam.SetPosition(posW[0]+motionW[0],posW[1]+motionW[1],posW[2]+motionW[2])
    iRen.Render()

# Mouse middle button up/down is elevation, and left and right is azimuth
def mouseMiddleMove(iRen,ren):
    getMotion(iRen,ren)
    if abs(motionD[0]) > abs(motionD[1]):
        cam.Azimuth(-2.0*motionD[0])
    else:
        vup = cam.GetViewUp()
        normalize(vup)
        dop = cam.GetDirectionOfProjection();
        normalize(dop)
        theta = math.degrees( math.acos(-dot(vup,dop)) )
        cam.Elevation(-motionD[1])
    iRen.Render()

# Right mouse button is rotate (left/right movement) and zoom in/out (up/down
# movement)
def mouseRightMove(iRen,ren):
    getMotion(iRen,ren)
    if abs(motionD[0]) > abs(motionD[1]):
        cam.Azimuth(-2.0*motionD[0])
    else:
        cam.Zoom(1 + motionD[1]/100.0)
    iRen.Render()

# Mouse scroll wheel is zoom in/out
def mouseWheelForward(iRen,ren):
    cam = ren.GetActiveCamera()
    cam.Zoom(1.1)
    iRen.Render()
def mouseWheelBackward(iRen,ren):
    cam = ren.GetActiveCamera()
    cam.Zoom(0.9)
    iRen.Render()

# The events to be played back
Recording = \
    "# StreamVersion 1\n\
    EnterEvent 299 92 0 0 0 0 0\n\
    MouseMoveEvent 299 92 0 0 0 0 0\n\
    MouseMoveEvent 296 98 0 0 0 0 0\n\
    MouseMoveEvent 293 104 0 0 0 0 0\n\
    MouseMoveEvent 290 108 0 0 0 0 0\n\
    MouseMoveEvent 287 112 0 0 0 0 0\n\
    MouseMoveEvent 285 116 0 0 0 0 0\n\
    MouseMoveEvent 282 120 0 0 0 0 0\n\
    MouseMoveEvent 281 122 0 0 0 0 0\n\
    MouseMoveEvent 279 124 0 0 0 0 0\n\
    MouseMoveEvent 277 126 0 0 0 0 0\n\
    MouseMoveEvent 276 127 0 0 0 0 0\n\
    MouseMoveEvent 275 128 0 0 0 0 0\n\
    MouseMoveEvent 273 129 0 0 0 0 0\n\
    MouseMoveEvent 272 130 0 0 0 0 0\n\
    MouseMoveEvent 271 131 0 0 0 0 0\n\
    MouseMoveEvent 269 132 0 0 0 0 0\n\
    LeftButtonPressEvent 269 132 0 0 0 0 0\n\
    MouseMoveEvent 268 132 0 0 0 0 0\n\
    MouseMoveEvent 267 132 0 0 0 0 0\n\
    MouseMoveEvent 267 132 0 0 0 0 0\n\
    MouseMoveEvent 265 132 0 0 0 0 0\n\
    MouseMoveEvent 263 132 0 0 0 0 0\n\
    MouseMoveEvent 260 132 0 0 0 0 0\n\
    MouseMoveEvent 258 132 0 0 0 0 0\n\
    MouseMoveEvent 255 133 0 0 0 0 0\n\
    MouseMoveEvent 250 134 0 0 0 0 0\n\
    MouseMoveEvent 246 135 0 0 0 0 0\n\
    MouseMoveEvent 245 136 0 0 0 0 0\n\
    MouseMoveEvent 243 136 0 0 0 0 0\n\
    MouseMoveEvent 242 136 0 0 0 0 0\n\
    MouseMoveEvent 241 136 0 0 0 0 0\n\
    MouseMoveEvent 240 136 0 0 0 0 0\n\
    MouseMoveEvent 239 136 0 0 0 0 0\n\
    MouseMoveEvent 238 136 0 0 0 0 0\n\
    MouseMoveEvent 237 136 0 0 0 0 0\n\
    MouseMoveEvent 235 136 0 0 0 0 0\n\
    MouseMoveEvent 231 136 0 0 0 0 0\n\
    MouseMoveEvent 230 136 0 0 0 0 0\n\
    MouseMoveEvent 227 136 0 0 0 0 0\n\
    MouseMoveEvent 224 136 0 0 0 0 0\n\
    MouseMoveEvent 219 136 0 0 0 0 0\n\
    MouseMoveEvent 215 136 0 0 0 0 0\n\
    MouseMoveEvent 211 136 0 0 0 0 0\n\
    MouseMoveEvent 208 136 0 0 0 0 0\n\
    MouseMoveEvent 205 136 0 0 0 0 0\n\
    MouseMoveEvent 203 136 0 0 0 0 0\n\
    MouseMoveEvent 201 136 0 0 0 0 0\n\
    MouseMoveEvent 199 136 0 0 0 0 0\n\
    MouseMoveEvent 197 136 0 0 0 0 0\n\
    MouseMoveEvent 194 136 0 0 0 0 0\n\
    MouseMoveEvent 191 136 0 0 0 0 0\n\
    MouseMoveEvent 189 136 0 0 0 0 0\n\
    MouseMoveEvent 187 136 0 0 0 0 0\n\
    MouseMoveEvent 185 136 0 0 0 0 0\n\
    MouseMoveEvent 181 136 0 0 0 0 0\n\
    MouseMoveEvent 178 136 0 0 0 0 0\n\
    MouseMoveEvent 175 136 0 0 0 0 0\n\
    MouseMoveEvent 172 136 0 0 0 0 0\n\
    MouseMoveEvent 168 136 0 0 0 0 0\n\
    MouseMoveEvent 163 136 0 0 0 0 0\n\
    MouseMoveEvent 159 136 0 0 0 0 0\n\
    MouseMoveEvent 157 136 0 0 0 0 0\n\
    MouseMoveEvent 155 136 0 0 0 0 0\n\
    MouseMoveEvent 154 136 0 0 0 0 0\n\
    MouseMoveEvent 153 136 0 0 0 0 0\n\
    MouseMoveEvent 152 136 0 0 0 0 0\n\
    MouseMoveEvent 151 136 0 0 0 0 0\n\
    MouseMoveEvent 151 136 0 0 0 0 0\n\
    MouseMoveEvent 153 135 0 0 0 0 0\n\
    MouseMoveEvent 157 135 0 0 0 0 0\n\
    MouseMoveEvent 163 133 0 0 0 0 0\n\
    MouseMoveEvent 173 133 0 0 0 0 0\n\
    MouseMoveEvent 180 133 0 0 0 0 0\n\
    MouseMoveEvent 187 132 0 0 0 0 0\n\
    MouseMoveEvent 197 132 0 0 0 0 0\n\
    MouseMoveEvent 206 132 0 0 0 0 0\n\
    MouseMoveEvent 212 131 0 0 0 0 0\n\
    MouseMoveEvent 216 131 0 0 0 0 0\n\
    MouseMoveEvent 218 131 0 0 0 0 0\n\
    MouseMoveEvent 221 131 0 0 0 0 0\n\
    MouseMoveEvent 221 131 0 0 0 0 0\n\
    MouseMoveEvent 222 131 0 0 0 0 0\n\
    MouseMoveEvent 223 131 0 0 0 0 0\n\
    MouseMoveEvent 225 130 0 0 0 0 0\n\
    MouseMoveEvent 225 130 0 0 0 0 0\n\
    MouseMoveEvent 227 130 0 0 0 0 0\n\
    MouseMoveEvent 227 130 0 0 0 0 0\n\
    MouseMoveEvent 229 130 0 0 0 0 0\n\
    MouseMoveEvent 231 130 0 0 0 0 0\n\
    MouseMoveEvent 233 130 0 0 0 0 0\n\
    MouseMoveEvent 234 130 0 0 0 0 0\n\
    MouseMoveEvent 235 130 0 0 0 0 0\n\
    MouseMoveEvent 237 130 0 0 0 0 0\n\
    MouseMoveEvent 238 130 0 0 0 0 0\n\
    MouseMoveEvent 239 130 0 0 0 0 0\n\
    MouseMoveEvent 239 130 0 0 0 0 0\n\
    MouseMoveEvent 239 130 0 0 0 0 0\n\
    MouseMoveEvent 238 130 0 0 0 0 0\n\
    MouseMoveEvent 237 130 0 0 0 0 0\n\
    MouseMoveEvent 235 130 0 0 0 0 0\n\
    MouseMoveEvent 234 130 0 0 0 0 0\n\
    MouseMoveEvent 233 130 0 0 0 0 0\n\
    LeftButtonReleaseEvent 233 130 0 0 0 0 0\n\
    MouseMoveEvent 233 130 0 0 0 0 0\n\
    MouseMoveEvent 231 129 0 0 0 0 0\n\
    MouseMoveEvent 228 128 0 0 0 0 0\n\
    MouseMoveEvent 227 126 0 0 0 0 0\n\
    MouseMoveEvent 226 126 0 0 0 0 0\n\
    MouseMoveEvent 224 125 0 0 0 0 0\n\
    MouseMoveEvent 219 124 0 0 0 0 0\n\
    MouseMoveEvent 217 124 0 0 0 0 0\n\
    MouseMoveEvent 216 123 0 0 0 0 0\n\
    MouseMoveEvent 215 122 0 0 0 0 0\n\
    MouseMoveEvent 213 122 0 0 0 0 0\n\
    MouseMoveEvent 212 122 0 0 0 0 0\n\
    MouseMoveEvent 209 121 0 0 0 0 0\n\
    MouseMoveEvent 208 120 0 0 0 0 0\n\
    MouseMoveEvent 207 120 0 0 0 0 0\n\
    MouseMoveEvent 205 118 0 0 0 0 0\n\
    MouseMoveEvent 204 117 0 0 0 0 0\n\
    MouseMoveEvent 204 118 0 0 0 0 0\n\
    MouseMoveEvent 204 122 0 0 0 0 0\n\
    MouseMoveEvent 204 125 0 0 0 0 0\n\
    MouseMoveEvent 204 129 0 0 0 0 0\n\
    MouseMoveEvent 204 133 0 0 0 0 0\n\
    MouseMoveEvent 204 136 0 0 0 0 0\n\
    MouseMoveEvent 204 140 0 0 0 0 0\n\
    MouseMoveEvent 204 142 0 0 0 0 0\n\
    MouseMoveEvent 204 145 0 0 0 0 0\n\
    MouseMoveEvent 204 147 0 0 0 0 0\n\
    MouseMoveEvent 204 149 0 0 0 0 0\n\
    MouseMoveEvent 204 152 0 0 0 0 0\n\
    MouseMoveEvent 205 154 0 0 0 0 0\n\
    MouseMoveEvent 205 156 0 0 0 0 0\n\
    MouseMoveEvent 205 158 0 0 0 0 0\n\
    MouseMoveEvent 206 160 0 0 0 0 0\n\
    MouseMoveEvent 206 162 0 0 0 0 0\n\
    MouseMoveEvent 207 163 0 0 0 0 0\n\
    MouseMoveEvent 207 166 0 0 0 0 0\n\
    MouseMoveEvent 207 168 0 0 0 0 0\n\
    MouseMoveEvent 207 169 0 0 0 0 0\n\
    MouseMoveEvent 208 169 0 0 0 0 0\n\
    MouseMoveEvent 208 170 0 0 0 0 0\n\
    MouseMoveEvent 208 172 0 0 0 0 0\n\
    MiddleButtonPressEvent 208 172 0 0 0 0 0\n\
    MouseMoveEvent 209 172 0 0 0 0 0\n\
    MouseMoveEvent 209 173 0 0 0 0 0\n\
    MouseMoveEvent 209 174 0 0 0 0 0\n\
    MouseMoveEvent 209 174 0 0 0 0 0\n\
    MouseMoveEvent 209 176 0 0 0 0 0\n\
    MouseMoveEvent 209 177 0 0 0 0 0\n\
    MouseMoveEvent 209 178 0 0 0 0 0\n\
    MouseMoveEvent 209 179 0 0 0 0 0\n\
    MouseMoveEvent 209 180 0 0 0 0 0\n\
    MouseMoveEvent 210 182 0 0 0 0 0\n\
    MouseMoveEvent 211 184 0 0 0 0 0\n\
    MouseMoveEvent 211 186 0 0 0 0 0\n\
    MouseMoveEvent 212 188 0 0 0 0 0\n\
    MouseMoveEvent 212 190 0 0 0 0 0\n\
    MouseMoveEvent 213 192 0 0 0 0 0\n\
    MouseMoveEvent 214 196 0 0 0 0 0\n\
    MouseMoveEvent 214 198 0 0 0 0 0\n\
    MouseMoveEvent 215 201 0 0 0 0 0\n\
    MouseMoveEvent 215 204 0 0 0 0 0\n\
    MouseMoveEvent 216 206 0 0 0 0 0\n\
    MouseMoveEvent 217 209 0 0 0 0 0\n\
    MouseMoveEvent 217 212 0 0 0 0 0\n\
    MouseMoveEvent 218 213 0 0 0 0 0\n\
    MouseMoveEvent 218 214 0 0 0 0 0\n\
    MouseMoveEvent 218 216 0 0 0 0 0\n\
    MouseMoveEvent 218 217 0 0 0 0 0\n\
    MouseMoveEvent 218 218 0 0 0 0 0\n\
    MouseMoveEvent 218 218 0 0 0 0 0\n\
    MouseMoveEvent 218 219 0 0 0 0 0\n\
    MouseMoveEvent 218 218 0 0 0 0 0\n\
    MouseMoveEvent 218 215 0 0 0 0 0\n\
    MouseMoveEvent 218 212 0 0 0 0 0\n\
    MouseMoveEvent 218 210 0 0 0 0 0\n\
    MouseMoveEvent 218 206 0 0 0 0 0\n\
    MouseMoveEvent 218 203 0 0 0 0 0\n\
    MouseMoveEvent 218 200 0 0 0 0 0\n\
    MouseMoveEvent 218 197 0 0 0 0 0\n\
    MouseMoveEvent 218 194 0 0 0 0 0\n\
    MouseMoveEvent 218 192 0 0 0 0 0\n\
    MouseMoveEvent 218 189 0 0 0 0 0\n\
    MouseMoveEvent 218 186 0 0 0 0 0\n\
    MouseMoveEvent 218 183 0 0 0 0 0\n\
    MouseMoveEvent 218 182 0 0 0 0 0\n\
    MouseMoveEvent 218 179 0 0 0 0 0\n\
    MouseMoveEvent 218 178 0 0 0 0 0\n\
    MouseMoveEvent 218 176 0 0 0 0 0\n\
    MouseMoveEvent 218 175 0 0 0 0 0\n\
    MouseMoveEvent 218 174 0 0 0 0 0\n\
    MouseMoveEvent 218 172 0 0 0 0 0\n\
    MouseMoveEvent 218 171 0 0 0 0 0\n\
    MouseMoveEvent 218 169 0 0 0 0 0\n\
    MouseMoveEvent 218 168 0 0 0 0 0\n\
    MouseMoveEvent 218 167 0 0 0 0 0\n\
    MouseMoveEvent 218 166 0 0 0 0 0\n\
    MouseMoveEvent 218 166 0 0 0 0 0\n\
    MouseMoveEvent 218 164 0 0 0 0 0\n\
    MouseMoveEvent 218 163 0 0 0 0 0\n\
    MouseMoveEvent 218 162 0 0 0 0 0\n\
    MouseMoveEvent 218 161 0 0 0 0 0\n\
    MouseMoveEvent 218 160 0 0 0 0 0\n\
    MouseMoveEvent 218 159 0 0 0 0 0\n\
    MouseMoveEvent 218 158 0 0 0 0 0\n\
    MouseMoveEvent 218 157 0 0 0 0 0\n\
    MouseMoveEvent 218 156 0 0 0 0 0\n\
    MouseMoveEvent 218 156 0 0 0 0 0\n\
    MiddleButtonReleaseEvent 218 156 0 0 0 0 0\n\
    MouseMoveEvent 218 156 0 0 0 0 0\n\
    MouseMoveEvent 217 156 0 0 0 0 0\n\
    MouseMoveEvent 214 156 0 0 0 0 0\n\
    MouseMoveEvent 210 156 0 0 0 0 0\n\
    MouseMoveEvent 204 156 0 0 0 0 0\n\
    MouseMoveEvent 197 158 0 0 0 0 0\n\
    MouseMoveEvent 193 158 0 0 0 0 0\n\
    MouseMoveEvent 183 161 0 0 0 0 0\n\
    MouseMoveEvent 173 163 0 0 0 0 0\n\
    MiddleButtonPressEvent 173 163 0 0 0 0 0\n\
    MouseMoveEvent 165 164 0 0 0 0 0\n\
    MouseMoveEvent 155 166 0 0 0 0 0\n\
    MouseMoveEvent 147 167 0 0 0 0 0\n\
    MouseMoveEvent 137 169 0 0 0 0 0\n\
    MouseMoveEvent 131 171 0 0 0 0 0\n\
    MouseMoveEvent 124 172 0 0 0 0 0\n\
    MouseMoveEvent 121 173 0 0 0 0 0\n\
    MouseMoveEvent 119 173 0 0 0 0 0\n\
    MouseMoveEvent 118 173 0 0 0 0 0\n\
    MouseMoveEvent 117 174 0 0 0 0 0\n\
    MouseMoveEvent 116 174 0 0 0 0 0\n\
    MouseMoveEvent 115 174 0 0 0 0 0\n\
    MouseMoveEvent 113 174 0 0 0 0 0\n\
    MouseMoveEvent 110 174 0 0 0 0 0\n\
    MouseMoveEvent 108 174 0 0 0 0 0\n\
    MouseMoveEvent 106 174 0 0 0 0 0\n\
    MouseMoveEvent 105 174 0 0 0 0 0\n\
    MouseMoveEvent 103 174 0 0 0 0 0\n\
    MouseMoveEvent 103 174 0 0 0 0 0\n\
    MouseMoveEvent 101 175 0 0 0 0 0\n\
    MouseMoveEvent 101 175 0 0 0 0 0\n\
    MouseMoveEvent 100 175 0 0 0 0 0\n\
    MouseMoveEvent 101 175 0 0 0 0 0\n\
    MouseMoveEvent 104 175 0 0 0 0 0\n\
    MouseMoveEvent 107 175 0 0 0 0 0\n\
    MouseMoveEvent 112 175 0 0 0 0 0\n\
    MouseMoveEvent 118 175 0 0 0 0 0\n\
    MouseMoveEvent 122 175 0 0 0 0 0\n\
    MouseMoveEvent 128 175 0 0 0 0 0\n\
    MouseMoveEvent 132 175 0 0 0 0 0\n\
    MouseMoveEvent 136 175 0 0 0 0 0\n\
    MouseMoveEvent 142 175 0 0 0 0 0\n\
    MouseMoveEvent 145 175 0 0 0 0 0\n\
    MouseMoveEvent 148 175 0 0 0 0 0\n\
    MouseMoveEvent 151 175 0 0 0 0 0\n\
    MouseMoveEvent 153 175 0 0 0 0 0\n\
    MouseMoveEvent 154 175 0 0 0 0 0\n\
    MouseMoveEvent 155 175 0 0 0 0 0\n\
    MouseMoveEvent 155 174 0 0 0 0 0\n\
    MouseMoveEvent 156 174 0 0 0 0 0\n\
    MouseMoveEvent 157 174 0 0 0 0 0\n\
    MouseMoveEvent 157 174 0 0 0 0 0\n\
    MouseMoveEvent 158 174 0 0 0 0 0\n\
    MouseMoveEvent 159 174 0 0 0 0 0\n\
    MouseMoveEvent 159 174 0 0 0 0 0\n\
    MouseMoveEvent 161 174 0 0 0 0 0\n\
    MouseMoveEvent 164 174 0 0 0 0 0\n\
    MouseMoveEvent 166 174 0 0 0 0 0\n\
    MouseMoveEvent 169 174 0 0 0 0 0\n\
    MouseMoveEvent 172 174 0 0 0 0 0\n\
    MouseMoveEvent 174 174 0 0 0 0 0\n\
    MouseMoveEvent 177 174 0 0 0 0 0\n\
    MouseMoveEvent 178 174 0 0 0 0 0\n\
    MouseMoveEvent 179 174 0 0 0 0 0\n\
    MouseMoveEvent 181 174 0 0 0 0 0\n\
    MouseMoveEvent 181 174 0 0 0 0 0\n\
    MouseMoveEvent 182 174 0 0 0 0 0\n\
    MouseMoveEvent 183 175 0 0 0 0 0\n\
    MouseMoveEvent 183 175 0 0 0 0 0\n\
    MouseMoveEvent 184 175 0 0 0 0 0\n\
    MouseMoveEvent 185 175 0 0 0 0 0\n\
    MiddleButtonReleaseEvent 185 175 0 0 0 0 0\n\
    MouseMoveEvent 185 175 0 0 0 0 0\n\
    RightButtonPressEvent 185 175 0 0 0 0 0\n\
    MouseMoveEvent 185 176 0 0 0 0 0\n\
    MouseMoveEvent 185 178 0 0 0 0 0\n\
    MouseMoveEvent 183 181 0 0 0 0 0\n\
    MouseMoveEvent 183 183 0 0 0 0 0\n\
    MouseMoveEvent 183 185 0 0 0 0 0\n\
    MouseMoveEvent 182 188 0 0 0 0 0\n\
    MouseMoveEvent 182 189 0 0 0 0 0\n\
    MouseMoveEvent 182 191 0 0 0 0 0\n\
    MouseMoveEvent 182 193 0 0 0 0 0\n\
    MouseMoveEvent 182 194 0 0 0 0 0\n\
    MouseMoveEvent 182 195 0 0 0 0 0\n\
    MouseMoveEvent 182 196 0 0 0 0 0\n\
    MouseMoveEvent 182 198 0 0 0 0 0\n\
    MouseMoveEvent 182 199 0 0 0 0 0\n\
    MouseMoveEvent 182 200 0 0 0 0 0\n\
    MouseMoveEvent 182 201 0 0 0 0 0\n\
    MouseMoveEvent 182 202 0 0 0 0 0\n\
    MouseMoveEvent 182 203 0 0 0 0 0\n\
    MouseMoveEvent 182 204 0 0 0 0 0\n\
    MouseMoveEvent 182 204 0 0 0 0 0\n\
    MouseMoveEvent 182 206 0 0 0 0 0\n\
    MouseMoveEvent 182 206 0 0 0 0 0\n\
    MouseMoveEvent 182 207 0 0 0 0 0\n\
    MouseMoveEvent 182 208 0 0 0 0 0\n\
    MouseMoveEvent 182 210 0 0 0 0 0\n\
    MouseMoveEvent 182 212 0 0 0 0 0\n\
    MouseMoveEvent 182 213 0 0 0 0 0\n\
    MouseMoveEvent 182 215 0 0 0 0 0\n\
    MouseMoveEvent 182 216 0 0 0 0 0\n\
    MouseMoveEvent 182 218 0 0 0 0 0\n\
    MouseMoveEvent 182 221 0 0 0 0 0\n\
    MouseMoveEvent 183 224 0 0 0 0 0\n\
    MouseMoveEvent 184 226 0 0 0 0 0\n\
    MouseMoveEvent 184 228 0 0 0 0 0\n\
    MouseMoveEvent 185 230 0 0 0 0 0\n\
    MouseMoveEvent 185 232 0 0 0 0 0\n\
    MouseMoveEvent 185 233 0 0 0 0 0\n\
    MouseMoveEvent 185 234 0 0 0 0 0\n\
    MouseMoveEvent 185 237 0 0 0 0 0\n\
    MouseMoveEvent 186 238 0 0 0 0 0\n\
    MouseMoveEvent 187 240 0 0 0 0 0\n\
    MouseMoveEvent 187 241 0 0 0 0 0\n\
    MouseMoveEvent 187 242 0 0 0 0 0\n\
    MouseMoveEvent 187 244 0 0 0 0 0\n\
    MouseMoveEvent 188 245 0 0 0 0 0\n\
    MouseMoveEvent 188 246 0 0 0 0 0\n\
    MouseMoveEvent 188 247 0 0 0 0 0\n\
    MouseMoveEvent 189 248 0 0 0 0 0\n\
    MouseMoveEvent 189 249 0 0 0 0 0\n\
    MouseMoveEvent 189 250 0 0 0 0 0\n\
    MouseMoveEvent 189 250 0 0 0 0 0\n\
    MouseMoveEvent 189 251 0 0 0 0 0\n\
    MouseMoveEvent 190 252 0 0 0 0 0\n\
    MouseMoveEvent 190 252 0 0 0 0 0\n\
    MouseMoveEvent 190 254 0 0 0 0 0\n\
    MouseMoveEvent 191 255 0 0 0 0 0\n\
    MouseMoveEvent 191 256 0 0 0 0 0\n\
    MouseMoveEvent 192 258 0 0 0 0 0\n\
    MouseMoveEvent 192 259 0 0 0 0 0\n\
    MouseMoveEvent 193 261 0 0 0 0 0\n\
    MouseMoveEvent 193 264 0 0 0 0 0\n\
    MouseMoveEvent 193 265 0 0 0 0 0\n\
    MouseMoveEvent 194 267 0 0 0 0 0\n\
    MouseMoveEvent 194 269 0 0 0 0 0\n\
    MouseMoveEvent 194 270 0 0 0 0 0\n\
    MouseMoveEvent 195 273 0 0 0 0 0\n\
    MouseMoveEvent 196 275 0 0 0 0 0\n\
    MouseMoveEvent 196 276 0 0 0 0 0\n\
    MouseMoveEvent 197 278 0 0 0 0 0\n\
    MouseMoveEvent 197 279 0 0 0 0 0\n\
    MouseMoveEvent 197 280 0 0 0 0 0\n\
    MouseMoveEvent 197 280 0 0 0 0 0\n\
    MouseMoveEvent 197 282 0 0 0 0 0\n\
    MouseMoveEvent 197 282 0 0 0 0 0\n\
    MouseMoveEvent 197 284 0 0 0 0 0\n\
    MouseMoveEvent 197 284 0 0 0 0 0\n\
    MouseMoveEvent 197 286 0 0 0 0 0\n\
    MouseMoveEvent 197 287 0 0 0 0 0\n\
    MouseMoveEvent 198 288 0 0 0 0 0\n\
    MouseMoveEvent 198 288 0 0 0 0 0\n\
    MouseMoveEvent 198 289 0 0 0 0 0\n\
    MouseMoveEvent 198 288 0 0 0 0 0\n\
    MouseMoveEvent 198 287 0 0 0 0 0\n\
    MouseMoveEvent 198 286 0 0 0 0 0\n\
    MouseMoveEvent 197 283 0 0 0 0 0\n\
    MouseMoveEvent 197 280 0 0 0 0 0\n\
    MouseMoveEvent 196 278 0 0 0 0 0\n\
    MouseMoveEvent 195 276 0 0 0 0 0\n\
    MouseMoveEvent 193 272 0 0 0 0 0\n\
    MouseMoveEvent 193 269 0 0 0 0 0\n\
    MouseMoveEvent 193 266 0 0 0 0 0\n\
    MouseMoveEvent 192 262 0 0 0 0 0\n\
    MouseMoveEvent 191 258 0 0 0 0 0\n\
    MouseMoveEvent 191 256 0 0 0 0 0\n\
    MouseMoveEvent 190 253 0 0 0 0 0\n\
    MouseMoveEvent 189 251 0 0 0 0 0\n\
    MouseMoveEvent 189 249 0 0 0 0 0\n\
    MouseMoveEvent 189 248 0 0 0 0 0\n\
    MouseMoveEvent 189 246 0 0 0 0 0\n\
    MouseMoveEvent 189 246 0 0 0 0 0\n\
    MouseMoveEvent 189 244 0 0 0 0 0\n\
    MouseMoveEvent 189 244 0 0 0 0 0\n\
    MouseMoveEvent 189 243 0 0 0 0 0\n\
    MouseMoveEvent 189 242 0 0 0 0 0\n\
    MouseMoveEvent 189 242 0 0 0 0 0\n\
    MouseMoveEvent 188 241 0 0 0 0 0\n\
    MouseMoveEvent 188 240 0 0 0 0 0\n\
    MouseMoveEvent 188 238 0 0 0 0 0\n\
    MouseMoveEvent 188 237 0 0 0 0 0\n\
    MouseMoveEvent 188 236 0 0 0 0 0\n\
    MouseMoveEvent 188 235 0 0 0 0 0\n\
    MouseMoveEvent 187 234 0 0 0 0 0\n\
    MouseMoveEvent 187 233 0 0 0 0 0\n\
    MouseMoveEvent 187 231 0 0 0 0 0\n\
    MouseMoveEvent 187 230 0 0 0 0 0\n\
    MouseMoveEvent 187 228 0 0 0 0 0\n\
    MouseMoveEvent 187 227 0 0 0 0 0\n\
    MouseMoveEvent 187 226 0 0 0 0 0\n\
    MouseMoveEvent 186 225 0 0 0 0 0\n\
    MouseMoveEvent 185 224 0 0 0 0 0\n\
    MouseMoveEvent 185 222 0 0 0 0 0\n\
    MouseMoveEvent 185 221 0 0 0 0 0\n\
    MouseMoveEvent 185 220 0 0 0 0 0\n\
    MouseMoveEvent 185 218 0 0 0 0 0\n\
    MouseMoveEvent 185 217 0 0 0 0 0\n\
    MouseMoveEvent 185 216 0 0 0 0 0\n\
    MouseMoveEvent 184 215 0 0 0 0 0\n\
    MouseMoveEvent 184 214 0 0 0 0 0\n\
    MouseMoveEvent 184 212 0 0 0 0 0\n\
    MouseMoveEvent 184 212 0 0 0 0 0\n\
    MouseMoveEvent 184 210 0 0 0 0 0\n\
    MouseMoveEvent 184 209 0 0 0 0 0\n\
    MouseMoveEvent 183 208 0 0 0 0 0\n\
    MouseMoveEvent 183 208 0 0 0 0 0\n\
    MouseMoveEvent 183 207 0 0 0 0 0\n\
    MouseMoveEvent 183 206 0 0 0 0 0\n\
    MouseMoveEvent 183 204 0 0 0 0 0\n\
    MouseMoveEvent 183 204 0 0 0 0 0\n\
    MouseMoveEvent 183 202 0 0 0 0 0\n\
    MouseMoveEvent 183 202 0 0 0 0 0\n\
    MouseMoveEvent 183 200 0 0 0 0 0\n\
    MouseMoveEvent 183 198 0 0 0 0 0\n\
    MouseMoveEvent 183 198 0 0 0 0 0\n\
    MouseMoveEvent 183 196 0 0 0 0 0\n\
    MouseMoveEvent 183 195 0 0 0 0 0\n\
    MouseMoveEvent 183 193 0 0 0 0 0\n\
    MouseMoveEvent 183 192 0 0 0 0 0\n\
    MouseMoveEvent 183 192 0 0 0 0 0\n\
    MouseMoveEvent 183 190 0 0 0 0 0\n\
    MouseMoveEvent 183 189 0 0 0 0 0\n\
    MouseMoveEvent 183 188 0 0 0 0 0\n\
    MouseMoveEvent 183 188 0 0 0 0 0\n\
    MouseMoveEvent 182 186 0 0 0 0 0\n\
    MouseMoveEvent 182 185 0 0 0 0 0\n\
    MouseMoveEvent 182 184 0 0 0 0 0\n\
    MouseMoveEvent 182 183 0 0 0 0 0\n\
    MouseMoveEvent 182 182 0 0 0 0 0\n\
    MouseMoveEvent 181 182 0 0 0 0 0\n\
    MouseMoveEvent 181 181 0 0 0 0 0\n\
    MouseMoveEvent 181 180 0 0 0 0 0\n\
    MouseMoveEvent 181 180 0 0 0 0 0\n\
    MouseMoveEvent 181 179 0 0 0 0 0\n\
    MouseMoveEvent 181 178 0 0 0 0 0\n\
    MouseMoveEvent 181 178 0 0 0 0 0\n\
    MouseMoveEvent 181 176 0 0 0 0 0\n\
    MouseMoveEvent 181 175 0 0 0 0 0\n\
    MouseMoveEvent 181 174 0 0 0 0 0\n\
    MouseMoveEvent 181 172 0 0 0 0 0\n\
    MouseMoveEvent 181 172 0 0 0 0 0\n\
    MouseMoveEvent 181 170 0 0 0 0 0\n\
    MouseMoveEvent 181 170 0 0 0 0 0\n\
    MouseMoveEvent 181 168 0 0 0 0 0\n\
    MouseMoveEvent 181 168 0 0 0 0 0\n\
    MouseMoveEvent 181 167 0 0 0 0 0\n\
    MouseMoveEvent 181 166 0 0 0 0 0\n\
    MouseMoveEvent 181 165 0 0 0 0 0\n\
    MouseMoveEvent 181 164 0 0 0 0 0\n\
    MouseMoveEvent 181 163 0 0 0 0 0\n\
    MouseMoveEvent 181 162 0 0 0 0 0\n\
    MouseMoveEvent 181 160 0 0 0 0 0\n\
    MouseMoveEvent 181 159 0 0 0 0 0\n\
    MouseMoveEvent 181 158 0 0 0 0 0\n\
    MouseMoveEvent 181 158 0 0 0 0 0\n\
    MouseMoveEvent 180 157 0 0 0 0 0\n\
    MouseMoveEvent 180 156 0 0 0 0 0\n\
    MouseMoveEvent 180 156 0 0 0 0 0\n\
    MouseMoveEvent 180 155 0 0 0 0 0\n\
    MouseMoveEvent 180 154 0 0 0 0 0\n\
    MouseMoveEvent 179 154 0 0 0 0 0\n\
    MouseMoveEvent 177 154 0 0 0 0 0\n\
    MouseMoveEvent 173 154 0 0 0 0 0\n\
    MouseMoveEvent 169 154 0 0 0 0 0\n\
    MouseMoveEvent 166 154 0 0 0 0 0\n\
    MouseMoveEvent 163 154 0 0 0 0 0\n\
    MouseMoveEvent 159 154 0 0 0 0 0\n\
    MouseMoveEvent 155 154 0 0 0 0 0\n\
    MouseMoveEvent 152 154 0 0 0 0 0\n\
    MouseMoveEvent 148 154 0 0 0 0 0\n\
    MouseMoveEvent 145 155 0 0 0 0 0\n\
    MouseMoveEvent 141 155 0 0 0 0 0\n\
    MouseMoveEvent 137 156 0 0 0 0 0\n\
    MouseMoveEvent 135 156 0 0 0 0 0\n\
    MouseMoveEvent 132 156 0 0 0 0 0\n\
    MouseMoveEvent 130 156 0 0 0 0 0\n\
    MouseMoveEvent 129 156 0 0 0 0 0\n\
    MouseMoveEvent 128 156 0 0 0 0 0\n\
    MouseMoveEvent 127 156 0 0 0 0 0\n\
    MouseMoveEvent 126 156 0 0 0 0 0\n\
    MouseMoveEvent 125 156 0 0 0 0 0\n\
    MouseMoveEvent 123 157 0 0 0 0 0\n\
    MouseMoveEvent 123 157 0 0 0 0 0\n\
    MouseMoveEvent 121 157 0 0 0 0 0\n\
    MouseMoveEvent 121 157 0 0 0 0 0\n\
    MouseMoveEvent 119 158 0 0 0 0 0\n\
    MouseMoveEvent 119 158 0 0 0 0 0\n\
    MouseMoveEvent 118 158 0 0 0 0 0\n\
    MouseMoveEvent 117 158 0 0 0 0 0\n\
    MouseMoveEvent 117 158 0 0 0 0 0\n\
    MouseMoveEvent 116 158 0 0 0 0 0\n\
    MouseMoveEvent 115 158 0 0 0 0 0\n\
    MouseMoveEvent 114 158 0 0 0 0 0\n\
    MouseMoveEvent 113 158 0 0 0 0 0\n\
    MouseMoveEvent 111 158 0 0 0 0 0\n\
    MouseMoveEvent 110 158 0 0 0 0 0\n\
    MouseMoveEvent 109 158 0 0 0 0 0\n\
    MouseMoveEvent 107 158 0 0 0 0 0\n\
    MouseMoveEvent 106 158 0 0 0 0 0\n\
    MouseMoveEvent 105 158 0 0 0 0 0\n\
    MouseMoveEvent 103 158 0 0 0 0 0\n\
    MouseMoveEvent 103 158 0 0 0 0 0\n\
    MouseMoveEvent 102 158 0 0 0 0 0\n\
    MouseMoveEvent 101 158 0 0 0 0 0\n\
    MouseMoveEvent 101 158 0 0 0 0 0\n\
    MouseMoveEvent 100 158 0 0 0 0 0\n\
    MouseMoveEvent 99 158 0 0 0 0 0\n\
    MouseMoveEvent 99 158 0 0 0 0 0\n\
    MouseMoveEvent 98 158 0 0 0 0 0\n\
    MouseMoveEvent 97 158 0 0 0 0 0\n\
    MouseMoveEvent 97 158 0 0 0 0 0\n\
    MouseMoveEvent 97 159 0 0 0 0 0\n\
    MouseMoveEvent 97 160 0 0 0 0 0\n\
    MouseMoveEvent 96 161 0 0 0 0 0\n\
    MouseMoveEvent 96 162 0 0 0 0 0\n\
    MouseMoveEvent 96 163 0 0 0 0 0\n\
    MouseMoveEvent 96 164 0 0 0 0 0\n\
    MouseMoveEvent 96 165 0 0 0 0 0\n\
    MouseMoveEvent 96 166 0 0 0 0 0\n\
    MouseMoveEvent 96 167 0 0 0 0 0\n\
    MouseMoveEvent 96 168 0 0 0 0 0\n\
    MouseMoveEvent 96 168 0 0 0 0 0\n\
    MouseMoveEvent 96 170 0 0 0 0 0\n\
    MouseMoveEvent 96 170 0 0 0 0 0\n\
    MouseMoveEvent 96 171 0 0 0 0 0\n\
    MouseMoveEvent 97 171 0 0 0 0 0\n\
    MouseMoveEvent 97 172 0 0 0 0 0\n\
    MouseMoveEvent 97 172 0 0 0 0 0\n\
    MouseMoveEvent 98 173 0 0 0 0 0\n\
    MouseMoveEvent 99 174 0 0 0 0 0\n\
    MouseMoveEvent 99 174 0 0 0 0 0\n\
    MouseMoveEvent 99 175 0 0 0 0 0\n\
    MouseMoveEvent 100 176 0 0 0 0 0\n\
    MouseMoveEvent 100 176 0 0 0 0 0\n\
    MouseMoveEvent 101 177 0 0 0 0 0\n\
    MouseMoveEvent 101 178 0 0 0 0 0\n\
    MouseMoveEvent 103 179 0 0 0 0 0\n\
    MouseMoveEvent 103 180 0 0 0 0 0\n\
    MouseMoveEvent 103 180 0 0 0 0 0\n\
    MouseMoveEvent 104 181 0 0 0 0 0\n\
    MouseMoveEvent 105 182 0 0 0 0 0\n\
    MouseMoveEvent 105 183 0 0 0 0 0\n\
    MouseMoveEvent 105 184 0 0 0 0 0\n\
    MouseMoveEvent 106 185 0 0 0 0 0\n\
    MouseMoveEvent 107 186 0 0 0 0 0\n\
    MouseMoveEvent 107 187 0 0 0 0 0\n\
    MouseMoveEvent 108 188 0 0 0 0 0\n\
    MouseMoveEvent 108 188 0 0 0 0 0\n\
    MouseMoveEvent 108 189 0 0 0 0 0\n\
    MouseMoveEvent 108 190 0 0 0 0 0\n\
    MouseMoveEvent 109 190 0 0 0 0 0\n\
    MouseMoveEvent 109 190 0 0 0 0 0\n\
    MouseMoveEvent 110 192 0 0 0 0 0\n\
    MouseMoveEvent 110 192 0 0 0 0 0\n\
    MouseMoveEvent 111 194 0 0 0 0 0\n\
    MouseMoveEvent 111 195 0 0 0 0 0\n\
    MouseMoveEvent 111 196 0 0 0 0 0\n\
    MouseMoveEvent 112 198 0 0 0 0 0\n\
    MouseMoveEvent 112 198 0 0 0 0 0\n\
    MouseMoveEvent 113 199 0 0 0 0 0\n\
    MouseMoveEvent 113 200 0 0 0 0 0\n\
    MouseMoveEvent 113 201 0 0 0 0 0\n\
    MouseMoveEvent 113 202 0 0 0 0 0\n\
    MouseMoveEvent 113 202 0 0 0 0 0\n\
    MouseMoveEvent 114 203 0 0 0 0 0\n\
    MouseMoveEvent 114 204 0 0 0 0 0\n\
    MouseMoveEvent 114 204 0 0 0 0 0\n\
    MouseMoveEvent 114 205 0 0 0 0 0\n\
    MouseMoveEvent 114 206 0 0 0 0 0\n\
    MouseMoveEvent 114 206 0 0 0 0 0\n\
    MouseMoveEvent 114 207 0 0 0 0 0\n\
    MouseMoveEvent 114 208 0 0 0 0 0\n\
    MouseMoveEvent 114 210 0 0 0 0 0\n\
    RightButtonReleaseEvent 114 210 0 0 0 0 0\n\
"

# Now define a script to build a pipeline
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

recorder = vtk.vtkInteractorEventRecorder()
recorder.SetInteractor(iRen)
#recorder.SetFileName("c:/d/VTK/record.log")
#recorder.Record()
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

# This is where the bindings are set up

# The callback stubs are defined here.  The callback takes two arguments.
# The first being the object that generates the event and the second argument
# the event name (which is a string).

def leftDown(widget, event_string):
    global left
    left = 1
def leftUp(widget, event_string):
    global left
    left = 0

def middleDown(widget, event_string):
    global middle
    middle = 1
def middleUp(widget, event_string):
    global middle
    middle = 0

def rightDown(widget, event_string):
    global right
    right = 1
def rightUp(widget, event_string):
    global right
    right = 0

def wheelForward(widget, event_string):
    mouseWheelForward(iRen,ren)
def wheelBackward(widget, event_string):
    mouseWheelBackward(iRen,ren)

def mouseMove(widget, event_string):
    global left, middle, right
    if left == 1:
        mouseLeftMove(iRen,ren)
    if middle == 1:
        mouseMiddleMove(iRen,ren)
    if right == 1:
        mouseRightMove(iRen,ren)
    if ( (left == 0) and (middle == 0) and (right == 0) ) :
        pass

# This is where the interaction style is defined
style = vtk.vtkInteractorStyleUser()
iRen.SetInteractorStyle(style)

style.AddObserver("LeftButtonPressEvent", leftDown)
style.AddObserver("LeftButtonReleaseEvent", leftUp)
style.AddObserver("MiddleButtonPressEvent", middleDown)
style.AddObserver("MiddleButtonReleaseEvent", middleUp)
style.AddObserver("RightButtonPressEvent", rightDown)
style.AddObserver("RightButtonReleaseEvent", rightUp)
style.AddObserver("MouseWheelForwardEvent", wheelForward)
style.AddObserver("MouseWheelBackwardEvent", wheelBackward)
style.AddObserver("MouseMoveEvent", mouseMove)

iRen.Initialize()
renWin.Render()

# render the image
renWin.Render()
recorder.Play()

#iRen.Start()
