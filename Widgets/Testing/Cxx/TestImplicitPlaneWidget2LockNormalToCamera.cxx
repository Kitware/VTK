/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImplicitPlaneWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkClipPolyData.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLODActor.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

char eventLog2LockNormalToCamera[] =
  "# StreamVersion 1\n"
  "CharEvent 108 202 0 0 105 1 i\n"
  "KeyReleaseEvent 108 202 0 0 105 1 i\n"
  "MouseMoveEvent 255 120 0 0 0 0 i\n"
  "MouseMoveEvent 255 121 0 0 0 0 i\n"
  "MouseMoveEvent 255 122 0 0 0 0 i\n"
  "MouseMoveEvent 255 123 0 0 0 0 i\n"
  "MouseMoveEvent 255 125 0 0 0 0 i\n"
  "MouseMoveEvent 255 126 0 0 0 0 i\n"
  "MouseMoveEvent 253 129 0 0 0 0 i\n"
  "MouseMoveEvent 253 133 0 0 0 0 i\n"
  "MouseMoveEvent 253 138 0 0 0 0 i\n"
  "MouseMoveEvent 253 141 0 0 0 0 i\n"
  "MouseMoveEvent 251 147 0 0 0 0 i\n"
  "MouseMoveEvent 251 146 0 0 0 0 i\n"
  "MouseMoveEvent 251 147 0 0 0 0 i\n"
  "MouseMoveEvent 251 148 0 0 0 0 i\n"
  "MouseMoveEvent 213 233 0 0 0 0 i\n"
  "MouseMoveEvent 212 231 0 0 0 0 i\n"
  "MouseMoveEvent 212 227 0 0 0 0 i\n"
  "MouseMoveEvent 211 225 0 0 0 0 i\n"
  "MouseMoveEvent 211 221 0 0 0 0 i\n"
  "MouseMoveEvent 208 216 0 0 0 0 i\n"
  "MouseMoveEvent 206 214 0 0 0 0 i\n"
  "MouseMoveEvent 206 210 0 0 0 0 i\n"
  "MouseMoveEvent 206 206 0 0 0 0 i\n"
  "MouseMoveEvent 206 201 0 0 0 0 i\n"
  "MouseMoveEvent 206 198 0 0 0 0 i\n"
  "MouseMoveEvent 206 194 0 0 0 0 i\n"
  "MouseMoveEvent 206 189 0 0 0 0 i\n"
  "MouseMoveEvent 206 185 0 0 0 0 i\n"
  "MouseMoveEvent 206 180 0 0 0 0 i\n"
  "MouseMoveEvent 205 175 0 0 0 0 i\n"
  "MouseMoveEvent 203 171 0 0 0 0 i\n"
  "MouseMoveEvent 202 168 0 0 0 0 i\n"
  "MouseMoveEvent 202 161 0 0 0 0 i\n"
  "MouseMoveEvent 201 156 0 0 0 0 i\n"
  "MouseMoveEvent 200 150 0 0 0 0 i\n"
  "MouseMoveEvent 198 147 0 0 0 0 i\n"
  "MouseMoveEvent 198 143 0 0 0 0 i\n"
  "MouseMoveEvent 196 140 0 0 0 0 i\n"
  "MouseMoveEvent 196 136 0 0 0 0 i\n"
  "MouseMoveEvent 193 131 0 0 0 0 i\n"
  "MouseMoveEvent 192 121 0 0 0 0 i\n"
  "MouseMoveEvent 192 114 0 0 0 0 i\n"
  "MouseMoveEvent 192 109 0 0 0 0 i\n"
  "MouseMoveEvent 192 104 0 0 0 0 i\n"
  "MouseMoveEvent 192 100 0 0 0 0 i\n"
  "MouseMoveEvent 192 97 0 0 0 0 i\n"
  "MouseMoveEvent 189 93 0 0 0 0 i\n"
  "MouseMoveEvent 188 92 0 0 0 0 i\n"
  "MouseMoveEvent 187 90 0 0 0 0 i\n"
  "MouseMoveEvent 186 89 0 0 0 0 i\n"
  "MouseMoveEvent 185 88 0 0 0 0 i\n"
  "MouseMoveEvent 184 86 0 0 0 0 i\n"
  "MouseMoveEvent 184 85 0 0 0 0 i\n"
  "MouseMoveEvent 183 85 0 0 0 0 i\n"
  "MouseMoveEvent 182 84 0 0 0 0 i\n"
  "MouseMoveEvent 180 83 0 0 0 0 i\n"
  "MouseMoveEvent 179 83 0 0 0 0 i\n"
  "MouseMoveEvent 178 83 0 0 0 0 i\n"
  "MouseMoveEvent 175 82 0 0 0 0 i\n"
  "MouseMoveEvent 173 81 0 0 0 0 i\n"
  "MouseMoveEvent 169 79 0 0 0 0 i\n"
  "MouseMoveEvent 168 78 0 0 0 0 i\n"
  "MouseMoveEvent 167 78 0 0 0 0 i\n"
  "MouseMoveEvent 167 79 0 0 0 0 i\n"
  "LeftButtonPressEvent 167 79 0 0 0 0 i\n"
  "MouseMoveEvent 167 80 0 0 0 0 i\n"
  "MouseMoveEvent 167 81 0 0 0 0 i\n"
  "MouseMoveEvent 169 82 0 0 0 0 i\n"
  "MouseMoveEvent 170 83 0 0 0 0 i\n"
  "MouseMoveEvent 171 84 0 0 0 0 i\n"
  "MouseMoveEvent 172 84 0 0 0 0 i\n"
  "MouseMoveEvent 174 85 0 0 0 0 i\n"
  "MouseMoveEvent 174 86 0 0 0 0 i\n"
  "MouseMoveEvent 175 86 0 0 0 0 i\n"
  "MouseMoveEvent 176 88 0 0 0 0 i\n"
  "MouseMoveEvent 177 90 0 0 0 0 i\n"
  "MouseMoveEvent 177 91 0 0 0 0 i\n"
  "MouseMoveEvent 177 92 0 0 0 0 i\n"
  "MouseMoveEvent 177 93 0 0 0 0 i\n"
  "MouseMoveEvent 177 94 0 0 0 0 i\n"
  "MouseMoveEvent 178 96 0 0 0 0 i\n"
  "MouseMoveEvent 178 97 0 0 0 0 i\n"
  "MouseMoveEvent 180 99 0 0 0 0 i\n"
  "MouseMoveEvent 181 99 0 0 0 0 i\n"
  "MouseMoveEvent 181 100 0 0 0 0 i\n"
  "MouseMoveEvent 181 101 0 0 0 0 i\n"
  "MouseMoveEvent 181 102 0 0 0 0 i\n"
  "MouseMoveEvent 181 103 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 181 103 0 0 0 0 i\n"
  "MouseMoveEvent 181 103 0 0 0 0 i\n"
  "MouseMoveEvent 181 102 0 0 0 0 i\n"
  "MouseMoveEvent 181 102 0 0 0 0 i\n"
  "MouseMoveEvent 181 102 0 0 0 0 i\n"
  "MouseMoveEvent 182 102 0 0 0 0 i\n"
  "MouseMoveEvent 183 103 0 0 0 0 i\n"
  "MouseMoveEvent 183 102 0 0 0 0 i\n"
  "MouseMoveEvent 184 102 0 0 0 0 i\n"
  "MouseMoveEvent 185 102 0 0 0 0 i\n"
  "MouseMoveEvent 185 101 0 0 0 0 i\n"
  "MouseMoveEvent 185 100 0 0 0 0 i\n"
  "MouseMoveEvent 185 99 0 0 0 0 i\n"
  "MiddleButtonPressEvent 185 99 0 0 0 0 i\n"
  "MouseMoveEvent 185 100 0 0 0 0 i\n"
  "MouseMoveEvent 182 101 0 0 0 0 i\n"
  "MouseMoveEvent 181 102 0 0 0 0 i\n"
  "MouseMoveEvent 180 103 0 0 0 0 i\n"
  "MouseMoveEvent 179 105 0 0 0 0 i\n"
  "MouseMoveEvent 176 107 0 0 0 0 i\n"
  "MouseMoveEvent 173 105 0 0 0 0 i\n"
  "MouseMoveEvent 173 106 0 0 0 0 i\n"
  "MouseMoveEvent 173 107 0 0 0 0 i\n"
  "MouseMoveEvent 173 108 0 0 0 0 i\n"
  "MouseMoveEvent 173 110 0 0 0 0 i\n"
  "MouseMoveEvent 172 112 0 0 0 0 i\n"
  "MouseMoveEvent 172 113 0 0 0 0 i\n"
  "MouseMoveEvent 172 114 0 0 0 0 i\n"
  "MouseMoveEvent 172 115 0 0 0 0 i\n"
  "MouseMoveEvent 173 116 0 0 0 0 i\n"
  "MouseMoveEvent 173 117 0 0 0 0 i\n"
  "MouseMoveEvent 174 118 0 0 0 0 i\n"
  "MouseMoveEvent 174 119 0 0 0 0 i\n"
  "MouseMoveEvent 174 120 0 0 0 0 i\n"
  "MouseMoveEvent 174 121 0 0 0 0 i\n"
  "MouseMoveEvent 174 122 0 0 0 0 i\n"
  "MouseMoveEvent 175 122 0 0 0 0 i\n"
  "MiddleButtonReleaseEvent 175 122 0 0 0 0 i\n"
  "MouseMoveEvent 175 122 0 0 0 0 i\n"
  "MouseMoveEvent 175 121 0 0 0 0 i\n"
  "MouseMoveEvent 174 120 0 0 0 0 i\n"
  "MouseMoveEvent 173 120 0 0 0 0 i\n"
  "MouseMoveEvent 173 119 0 0 0 0 i\n"
  "MouseMoveEvent 173 118 0 0 0 0 i\n"
  "MouseMoveEvent 172 117 0 0 0 0 i\n"
  "MouseMoveEvent 171 117 0 0 0 0 i\n"
  "MouseMoveEvent 171 116 0 0 0 0 i\n"
  "MouseMoveEvent 171 115 0 0 0 0 i\n"
  "MouseMoveEvent 171 114 0 0 0 0 i\n"
  "MouseMoveEvent 171 112 0 0 0 0 i\n"
  "MouseMoveEvent 171 109 0 0 0 0 i\n"
  "MouseMoveEvent 172 107 0 0 0 0 i\n"
  "MouseMoveEvent 172 105 0 0 0 0 i\n"
  "MouseMoveEvent 175 103 0 0 0 0 i\n"
  "MouseMoveEvent 176 100 0 0 0 0 i\n"
  "MouseMoveEvent 178 99 0 0 0 0 i\n"
  "MouseMoveEvent 179 97 0 0 0 0 i\n"
  "MouseMoveEvent 182 94 0 0 0 0 i\n"
  "MouseMoveEvent 183 92 0 0 0 0 i\n"
  "MouseMoveEvent 185 90 0 0 0 0 i\n"
  "MouseMoveEvent 186 87 0 0 0 0 i\n"
  "MouseMoveEvent 188 84 0 0 0 0 i\n"
  "MouseMoveEvent 190 81 0 0 0 0 i\n"
  "MouseMoveEvent 193 79 0 0 0 0 i\n"
  "MouseMoveEvent 195 78 0 0 0 0 i\n"
  "MouseMoveEvent 197 76 0 0 0 0 i\n"
  "MouseMoveEvent 201 75 0 0 0 0 i\n"
  "MouseMoveEvent 205 71 0 0 0 0 i\n"
  "MouseMoveEvent 209 67 0 0 0 0 i\n"
  "MouseMoveEvent 215 63 0 0 0 0 i\n"
  "MouseMoveEvent 221 58 0 0 0 0 i\n"
  "MouseMoveEvent 226 56 0 0 0 0 i\n"
  "MouseMoveEvent 228 54 0 0 0 0 i\n"
  "MouseMoveEvent 231 50 0 0 0 0 i\n"
  "MouseMoveEvent 235 47 0 0 0 0 i\n"
  "MouseMoveEvent 237 45 0 0 0 0 i\n"
  "MouseMoveEvent 239 42 0 0 0 0 i\n"
  "MouseMoveEvent 243 42 0 0 0 0 i\n"
  "MouseMoveEvent 245 41 0 0 0 0 i\n"
  "MouseMoveEvent 246 40 0 0 0 0 i\n"
  "MouseMoveEvent 246 39 0 0 0 0 i\n"
  "MouseMoveEvent 247 39 0 0 0 0 i\n"
  "MouseMoveEvent 248 38 0 0 0 0 i\n"
  "MouseMoveEvent 249 38 0 0 0 0 i\n"
  "MouseMoveEvent 250 38 0 0 0 0 i\n"
  "MouseMoveEvent 250 37 0 0 0 0 i\n"
  "MouseMoveEvent 252 36 0 0 0 0 i\n"
  "MouseMoveEvent 253 36 0 0 0 0 i\n"
  "MouseMoveEvent 254 36 0 0 0 0 i\n"
  "MouseMoveEvent 254 37 0 0 0 0 i\n"
  "MouseMoveEvent 255 37 0 0 0 0 i\n"
  "MouseMoveEvent 256 37 0 0 0 0 i\n"
  "MouseMoveEvent 257 37 0 0 0 0 i\n"
  "MouseMoveEvent 258 37 0 0 0 0 i\n"
  "MouseMoveEvent 259 38 0 0 0 0 i\n"
  "MouseMoveEvent 260 39 0 0 0 0 i\n"
  "MouseMoveEvent 260 40 0 0 0 0 i\n"
  "MouseMoveEvent 259 40 0 0 0 0 i\n"
  "MouseMoveEvent 258 40 0 0 0 0 i\n"
  "MouseMoveEvent 257 40 0 0 0 0 i\n"
  "MouseMoveEvent 257 41 0 0 0 0 i\n"
  "MouseMoveEvent 257 42 0 0 0 0 i\n"
  "MouseMoveEvent 257 43 0 0 0 0 i\n"
  "MouseMoveEvent 257 44 0 0 0 0 i\n"
  "MouseMoveEvent 258 44 0 0 0 0 i\n"
  "LeftButtonPressEvent 258 44 0 0 0 0 i\n"
  "MouseMoveEvent 258 45 0 0 0 0 i\n"
  "MouseMoveEvent 259 45 0 0 0 0 i\n"
  "MouseMoveEvent 259 46 0 0 0 0 i\n"
  "MouseMoveEvent 259 47 0 0 0 0 i\n"
  "MouseMoveEvent 259 49 0 0 0 0 i\n"
  "MouseMoveEvent 259 50 0 0 0 0 i\n"
  "MouseMoveEvent 259 51 0 0 0 0 i\n"
  "MouseMoveEvent 259 52 0 0 0 0 i\n"
  "MouseMoveEvent 260 53 0 0 0 0 i\n"
  "MouseMoveEvent 260 54 0 0 0 0 i\n"
  "MouseMoveEvent 260 55 0 0 0 0 i\n"
  "MouseMoveEvent 260 56 0 0 0 0 i\n"
  "MouseMoveEvent 260 57 0 0 0 0 i\n"
  "MouseMoveEvent 261 58 0 0 0 0 i\n"
  "MouseMoveEvent 261 59 0 0 0 0 i\n"
  "MouseMoveEvent 261 60 0 0 0 0 i\n"
  "MouseMoveEvent 261 61 0 0 0 0 i\n"
  "MouseMoveEvent 261 60 0 0 0 0 i\n"
  "MouseMoveEvent 260 59 0 0 0 0 i\n"
  "MouseMoveEvent 258 58 0 0 0 0 i\n"
  "MouseMoveEvent 255 57 0 0 0 0 i\n"
  "MouseMoveEvent 252 55 0 0 0 0 i\n"
  "MouseMoveEvent 251 54 0 0 0 0 i\n"
  "MouseMoveEvent 251 53 0 0 0 0 i\n"
  "MouseMoveEvent 249 51 0 0 0 0 i\n"
  "MouseMoveEvent 246 50 0 0 0 0 i\n"
  "MouseMoveEvent 245 50 0 0 0 0 i\n"
  "MouseMoveEvent 245 49 0 0 0 0 i\n"
  "MouseMoveEvent 244 48 0 0 0 0 i\n"
  "MouseMoveEvent 242 45 0 0 0 0 i\n"
  "MouseMoveEvent 241 44 0 0 0 0 i\n"
  "MouseMoveEvent 240 44 0 0 0 0 i\n"
  "MouseMoveEvent 239 44 0 0 0 0 i\n"
  "MouseMoveEvent 238 42 0 0 0 0 i\n"
  "MouseMoveEvent 237 42 0 0 0 0 i\n"
  "MouseMoveEvent 236 42 0 0 0 0 i\n"
  "MouseMoveEvent 236 41 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 236 41 0 0 0 0 i\n"
  "MouseMoveEvent 236 40 0 0 0 0 i\n"
  "MouseMoveEvent 235 40 0 0 0 0 i\n"
  "MouseMoveEvent 234 40 0 0 0 0 i\n"
  "MouseMoveEvent 233 40 0 0 0 0 i\n"
  "MouseMoveEvent 233 41 0 0 0 0 i\n"
  "MouseMoveEvent 233 42 0 0 0 0 i\n"
  "MouseMoveEvent 232 42 0 0 0 0 i\n"
  "MouseMoveEvent 231 42 0 0 0 0 i\n"
  "MouseMoveEvent 231 43 0 0 0 0 i\n"
  "RightButtonPressEvent 231 43 0 0 0 0 i\n"
  "MouseMoveEvent 231 42 0 0 0 0 i\n"
  "MouseMoveEvent 231 41 0 0 0 0 i\n"
  "MouseMoveEvent 231 40 0 0 0 0 i\n"
  "MouseMoveEvent 230 40 0 0 0 0 i\n"
  "MouseMoveEvent 229 38 0 0 0 0 i\n"
  "MouseMoveEvent 227 33 0 0 0 0 i\n"
  "MouseMoveEvent 226 27 0 0 0 0 i\n"
  "MouseMoveEvent 222 23 0 0 0 0 i\n"
  "MouseMoveEvent 214 19 0 0 0 0 i\n"
  "MouseMoveEvent 208 14 0 0 0 0 i\n"
  "MouseMoveEvent 208 15 0 0 0 0 i\n"
  "MouseMoveEvent 208 14 0 0 0 0 i\n"
  "MouseMoveEvent 208 13 0 0 0 0 i\n"
  "MouseMoveEvent 210 11 0 0 0 0 i\n"
  "MouseMoveEvent 213 9 0 0 0 0 i\n"
  "MouseMoveEvent 214 8 0 0 0 0 i\n"
  "MouseMoveEvent 214 7 0 0 0 0 i\n"
  "MouseMoveEvent 215 7 0 0 0 0 i\n"
  "MouseMoveEvent 215 6 0 0 0 0 i\n"
  "MouseMoveEvent 215 5 0 0 0 0 i\n"
  "MouseMoveEvent 216 2 0 0 0 0 i\n"
  "MouseMoveEvent 218 1 0 0 0 0 i\n"
  "LeaveEvent 220 -1 0 0 0 0 i\n"
  "MouseMoveEvent 220 -1 0 0 0 0 i\n"
  "MouseMoveEvent 221 -1 0 0 0 0 i\n"
  "MouseMoveEvent 221 -2 0 0 0 0 i\n"
  "MouseMoveEvent 220 -2 0 0 0 0 i\n"
  "MouseMoveEvent 220 -3 0 0 0 0 i\n"
  "MouseMoveEvent 220 -4 0 0 0 0 i\n"
  "MouseMoveEvent 220 -5 0 0 0 0 i\n"
  "MouseMoveEvent 220 -6 0 0 0 0 i\n"
  "MouseMoveEvent 220 -7 0 0 0 0 i\n"
  "MouseMoveEvent 220 -8 0 0 0 0 i\n"
  "MouseMoveEvent 220 -9 0 0 0 0 i\n"
  "MouseMoveEvent 220 -11 0 0 0 0 i\n"
  "MouseMoveEvent 220 -12 0 0 0 0 i\n"
  "MouseMoveEvent 220 -13 0 0 0 0 i\n"
  "MouseMoveEvent 220 -12 0 0 0 0 i\n"
  "MouseMoveEvent 220 -11 0 0 0 0 i\n"
  "RightButtonReleaseEvent 220 15 0 0 0 0 i\n"
  "EnterEvent 218 15 0 0 0 0 i\n"
  "MouseMoveEvent 218 14 0 0 0 0 i\n"
  "MouseMoveEvent 218 13 0 0 0 0 i\n"
  "MouseMoveEvent 218 12 0 0 0 0 i\n"
  "MouseMoveEvent 217 11 0 0 0 0 i\n"
  "MouseMoveEvent 217 10 0 0 0 0 i\n"
  "MouseMoveEvent 217 6 0 0 0 0 i\n"
  "MouseMoveEvent 217 5 0 0 0 0 i\n"
  "MouseMoveEvent 217 4 0 0 0 0 i\n"
  "MouseMoveEvent 217 3 0 0 0 0 i\n"
  "RightButtonPressEvent 150 160 0 0 0 0 i\n"
  "MouseMoveEvent 150 170 0 0 0 0 i\n"
  "MouseMoveEvent 150 180 0 0 0 0 i\n"
  "MouseMoveEvent 150 190 0 0 0 0 i\n"
  "RightButtonReleaseEvent 280 0 0 0 0 0 i\n"
  "KeyPressEvent 203 92 0 0 116 1 t\n"
  "CharEvent 203 92 0 0 116 1 t\n"
  "KeyReleaseEvent 203 92 0 0 116 1 t\n"
  "LeftButtonPressEvent 209 106 0 0 0 0 t\n"
  "StartInteractionEvent 209 106 0 0 0 0 t\n"
  "MouseMoveEvent 208 106 0 0 0 0 t\n"
  "RenderEvent 208 106 0 0 0 0 t\n"
  "MouseMoveEvent 207 106 0 0 0 0 t\n"
  "RenderEvent 209 106 0 0 0 0 t\n"
  "MouseMoveEvent 209 105 0 0 0 0 t\n"
  "RenderEvent 209 105 0 0 0 0 t\n"
  "MouseMoveEvent 210 105 0 0 0 0 t\n"
  "RenderEvent 210 105 0 0 0 0 t\n"
  "MouseMoveEvent 211 105 0 0 0 0 t\n"
  "RenderEvent 211 105 0 0 0 0 t\n"
  "MouseMoveEvent 212 105 0 0 0 0 t\n"
  "RenderEvent 212 105 0 0 0 0 t\n"
  "MouseMoveEvent 213 105 0 0 0 0 t\n"
  "RenderEvent 213 105 0 0 0 0 t\n"
  "MouseMoveEvent 214 107 0 0 0 0 t\n"
  "RenderEvent 214 107 0 0 0 0 t\n"
  "MouseMoveEvent 215 109 0 0 0 0 t\n"
  "RenderEvent 215 109 0 0 0 0 t\n"
  "MouseMoveEvent 216 115 0 0 0 0 t\n"
  "RenderEvent 216 115 0 0 0 0 t\n"
  "MouseMoveEvent 217 122 0 0 0 0 t\n"
  "RenderEvent 217 122 0 0 0 0 t\n"
  "MouseMoveEvent 218 134 0 0 0 0 t\n"
  "RenderEvent 218 134 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 290 0 0 0 0 t\n"
  "KeyPressEvent 147 213 0 1 62 1 greater\n"
  "CharEvent 147 213 0 1 62 1 greater\n"
  "KeyReleaseEvent 147 213 0 1 62 1 greater\n"
  "KeyPressEvent 147 213 0 1 62 1 greater\n"
  "CharEvent 147 213 0 1 62 1 greater\n"
  "KeyReleaseEvent 147 213 0 1 62 1 greater\n"
  "KeyPressEvent 147 213 0 1 62 1 greater\n"
  "CharEvent 147 213 0 1 62 1 greater\n"
  "KeyReleaseEvent 147 213 0 1 62 1 greater\n"
  "KeyPressEvent 147 213 0 1 62 1 greater\n"
  "CharEvent 147 213 0 1 62 1 greater\n"
  "KeyReleaseEvent 147 213 0 1 62 1 greater\n"
  "KeyPressEvent 147 213 0 1 62 1 greater\n"
  "CharEvent 147 213 0 1 62 1 greater\n"
  "KeyReleaseEvent 147 213 0 1 62 1 greater\n"
  "KeyPressEvent 147 213 0 1 62 1 greater\n"
  "CharEvent 147 213 0 1 62 1 greater\n"
  "KeyReleaseEvent 147 213 0 1 62 1 greater\n"
  "KeyPressEvent 147 213 0 1 62 1 greater\n"
  "CharEvent 147 213 0 1 62 1 greater\n"
  "KeyReleaseEvent 147 213 0 1 62 1 greater\n"
  "KeyPressEvent 147 213 0 1 62 1 greater\n"
  "CharEvent 147 213 0 1 62 1 greater\n"
  "KeyReleaseEvent 147 213 0 1 62 1 greater\n"
  "KeyPressEvent 147 213 0 1 62 1 greater\n"
  "CharEvent 147 213 0 1 62 1 greater\n"
  "KeyReleaseEvent 147 213 0 1 62 1 greater\n"
  "KeyPressEvent 147 213 0 1 62 1 greater\n"
  "CharEvent 147 213 0 1 62 1 greater\n"
  "KeyReleaseEvent 147 213 0 1 62 1 greater\n"
  "KeyPressEvent 147 213 0 1 62 1 greater\n"
  "CharEvent 147 213 0 1 62 1 greater\n"
  "KeyReleaseEvent 147 213 0 1 62 1 greater\n"
  "KeyPressEvent 147 213 0 1 62 1 greater\n"
  "CharEvent 147 213 0 1 62 1 greater\n"
  "KeyReleaseEvent 147 213 0 1 62 1 greater\n"
  "KeyPressEvent 147 213 0 1 62 1 greater\n"
  "CharEvent 147 213 0 1 62 1 greater\n"
  "KeyReleaseEvent 147 213 0 1 62 1 greater\n"
  ;

// This does the actual work: updates the vtkPlane implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkTIPW2Callback : public vtkCommand
{
public:
  static vtkTIPW2Callback *New()
  { return new vtkTIPW2Callback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkImplicitPlaneWidget2 *planeWidget =
      reinterpret_cast<vtkImplicitPlaneWidget2*>(caller);
    vtkImplicitPlaneRepresentation *rep =
      reinterpret_cast<vtkImplicitPlaneRepresentation*>(planeWidget->
                                                        GetRepresentation());
    rep->GetPlane(this->Plane);
    this->Actor->VisibilityOn();
  }
  vtkTIPW2Callback():Plane(0),Actor(0) {}
  vtkPlane *Plane;
  vtkActor *Actor;
};

// This permit to change the vtkImplicitPlaneWidget2 mode pressing Ctrl keypad
// It swaps the widget mode between Manual and LockNormalToCamera
// Callback for the interaction
class vtkEnableSlaveCallback : public vtkCommand
{
public:
  static vtkEnableSlaveCallback *New()
    {
    return new vtkEnableSlaveCallback;
    }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
    vtkRenderWindowInteractor *iren =
      static_cast<vtkRenderWindowInteractor*>(caller);
    if(vtkStdString(iren->GetKeySym()) == "Control_L" ||
       vtkStdString(iren->GetKeySym()) == "Control_R")
      {
      lockMode = !lockMode;
      if(lockMode)
        {
        pWidget->SetLockNormalToCamera(1);
        }
      else
        {
        pWidget->SetLockNormalToCamera(0);
        }
      }
    }

  // Ctors
  vtkEnableSlaveCallback():lockMode(true), pWidget(0) {}

  // Attributes
  bool lockMode;
  vtkImplicitPlaneWidget2* pWidget;
};

int TestImplicitPlaneWidget2LockNormalToCamera(int vtkNotUsed(argc),
                                               char* vtkNotUsed(argv)[])
{
  // Create a mace out of filters.
  //
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkConeSource> cone;
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSource(cone->GetOutput());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);
  glyph->Update();

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  vtkNew<vtkAppendPolyData> apd;
  apd->AddInput(glyph->GetOutput());
  apd->AddInput(sphere->GetOutput());

  vtkNew<vtkPolyDataMapper> maceMapper;
  maceMapper->SetInputConnection(apd->GetOutputPort());

  vtkNew<vtkLODActor> maceActor;
  maceActor->SetMapper(maceMapper.GetPointer());
  maceActor->VisibilityOn();

  // This portion of the code clips the mace with the vtkPlanes
  // implicit function. The clipped region is colored green.
  vtkNew<vtkPlane> plane;
  vtkNew<vtkClipPolyData> clipper;
  clipper->SetInputConnection(apd->GetOutputPort());
  clipper->SetClipFunction(plane.GetPointer());
  clipper->InsideOutOn();

  vtkNew<vtkPolyDataMapper> selectMapper;
  selectMapper->SetInputConnection(clipper->GetOutputPort());

  vtkNew<vtkLODActor> selectActor;
  selectActor->SetMapper(selectMapper.GetPointer());
  selectActor->GetProperty()->SetColor(0,1,0);
  selectActor->VisibilityOff();
  selectActor->SetScale(1.01, 1.01, 1.01);

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkNew<vtkRenderer> ren1;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  vtkNew<vtkTIPW2Callback> myCallback;
  myCallback->Plane = plane.GetPointer();
  myCallback->Actor = selectActor.GetPointer();

  vtkNew<vtkImplicitPlaneRepresentation> rep;
  rep->SetPlaceFactor(1.25);
  rep->PlaceWidget(glyph->GetOutput()->GetBounds());
  rep->SetEdgeColor(0.,1.,0.);

  vtkNew<vtkImplicitPlaneWidget2> planeWidget;
  planeWidget->SetInteractor(iren.GetPointer());
  planeWidget->SetRepresentation(rep.GetPointer());

  // Callback to the swaper interaction function
  vtkNew<vtkEnableSlaveCallback> lockMode;
  lockMode->pWidget = planeWidget.GetPointer();

  // Link the Swapper to the RenderWindowInteractor
  iren->AddObserver(vtkCommand::KeyPressEvent, lockMode.GetPointer());

  planeWidget->AddObserver(vtkCommand::InteractionEvent,
                           myCallback.GetPointer());
  planeWidget->AddObserver(vtkCommand::UpdateEvent, myCallback.GetPointer());

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  ren1->AddActor(maceActor.GetPointer());
  ren1->AddActor(selectActor.GetPointer());
  renWin->SetSize(300, 300);

  // record events
  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren.GetPointer());
//  recorder->SetFileName("c:/record.log");
//  recorder->Record();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog2LockNormalToCamera);

  // render the image
  //
  iren->Initialize();
  ren1->ResetCamera(glyph->GetOutput()->GetBounds());
  renWin->Render();

  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();
  planeWidget->SetEnabled(1);
  planeWidget->SetLockNormalToCamera(1);
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
