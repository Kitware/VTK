/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestButtonWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkAppendPolyData.h"
#include "vtkSphereSource.h"
#include "vtkButtonWidget.h"
#include "vtkTexturedButtonRepresentation.h"
#include "vtkTexturedButtonRepresentation2D.h"
#include "vtkProp3DButtonRepresentation.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkLookupTable.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkEllipticalButtonSource.h"
#include "vtkTIFFReader.h"
#include "vtkPNGReader.h"
#include "vtkStructuredPointsReader.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRayCastCompositeFunction.h"
#include "vtkVolumeRayCastMapper.h"
#include "vtkVolumeTextureMapper2D.h"
#include "vtkVolume.h"
#include "vtkOutlineSource.h"
#include "vtkPlatonicSolidSource.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

static char ButtonWidgetEventLog[] =
  "# StreamVersion 1\n"
  "RenderEvent 0 0 0 0 0 0 0\n"
  "EnterEvent 125 299 0 0 0 0 0\n"
  "MouseMoveEvent 125 299 0 0 0 0 0\n"
  "MouseMoveEvent 125 298 0 0 0 0 0\n"
  "MouseMoveEvent 125 297 0 0 0 0 0\n"
  "MouseMoveEvent 124 295 0 0 0 0 0\n"
  "MouseMoveEvent 123 294 0 0 0 0 0\n"
  "MouseMoveEvent 122 293 0 0 0 0 0\n"
  "MouseMoveEvent 121 292 0 0 0 0 0\n"
  "MouseMoveEvent 120 292 0 0 0 0 0\n"
  "MouseMoveEvent 120 291 0 0 0 0 0\n"
  "MouseMoveEvent 119 291 0 0 0 0 0\n"
  "MouseMoveEvent 119 290 0 0 0 0 0\n"
  "MouseMoveEvent 119 289 0 0 0 0 0\n"
  "MouseMoveEvent 119 288 0 0 0 0 0\n"
  "MouseMoveEvent 119 287 0 0 0 0 0\n"
  "MouseMoveEvent 119 286 0 0 0 0 0\n"
  "MouseMoveEvent 119 285 0 0 0 0 0\n"
  "MouseMoveEvent 119 284 0 0 0 0 0\n"
  "MouseMoveEvent 119 283 0 0 0 0 0\n"
  "MouseMoveEvent 119 282 0 0 0 0 0\n"
  "MouseMoveEvent 119 280 0 0 0 0 0\n"
  "MouseMoveEvent 119 279 0 0 0 0 0\n"
  "MouseMoveEvent 119 278 0 0 0 0 0\n"
  "MouseMoveEvent 118 278 0 0 0 0 0\n"
  "MouseMoveEvent 118 277 0 0 0 0 0\n"
  "MouseMoveEvent 118 276 0 0 0 0 0\n"
  "MouseMoveEvent 118 275 0 0 0 0 0\n"
  "MouseMoveEvent 118 274 0 0 0 0 0\n"
  "MouseMoveEvent 117 273 0 0 0 0 0\n"
  "MouseMoveEvent 116 272 0 0 0 0 0\n"
  "MouseMoveEvent 116 271 0 0 0 0 0\n"
  "MouseMoveEvent 116 270 0 0 0 0 0\n"
  "MouseMoveEvent 116 269 0 0 0 0 0\n"
  "MouseMoveEvent 116 268 0 0 0 0 0\n"
  "MouseMoveEvent 116 267 0 0 0 0 0\n"
  "MouseMoveEvent 115 267 0 0 0 0 0\n"
  "MouseMoveEvent 115 266 0 0 0 0 0\n"
  "MouseMoveEvent 115 265 0 0 0 0 0\n"
  "MouseMoveEvent 115 264 0 0 0 0 0\n"
  "MouseMoveEvent 115 263 0 0 0 0 0\n"
  "MouseMoveEvent 115 262 0 0 0 0 0\n"
  "RenderEvent 115 262 0 0 0 0 0\n"
  "MouseMoveEvent 115 260 0 0 0 0 0\n"
  "KeyPressEvent 115 260 0 0 116 1 t\n"
  "CharEvent 115 260 0 0 116 1 t\n"
  "MouseMoveEvent 115 259 0 0 0 0 t\n"
  "KeyReleaseEvent 115 259 0 0 116 1 t\n"
  "MouseMoveEvent 115 258 0 0 0 0 t\n"
  "MouseMoveEvent 115 257 0 0 0 0 t\n"
  "MouseMoveEvent 114 256 0 0 0 0 t\n"
  "MouseMoveEvent 113 255 0 0 0 0 t\n"
  "MouseMoveEvent 111 253 0 0 0 0 t\n"
  "MouseMoveEvent 111 252 0 0 0 0 t\n"
  "MouseMoveEvent 109 252 0 0 0 0 t\n"
  "MouseMoveEvent 106 251 0 0 0 0 t\n"
  "MouseMoveEvent 105 250 0 0 0 0 t\n"
  "MouseMoveEvent 105 249 0 0 0 0 t\n"
  "MouseMoveEvent 104 249 0 0 0 0 t\n"
  "MouseMoveEvent 100 247 0 0 0 0 t\n"
  "MouseMoveEvent 99 247 0 0 0 0 t\n"
  "LeftButtonPressEvent 99 247 0 0 0 0 t\n"
  "RenderEvent 99 247 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 99 247 0 0 0 0 t\n"
  "RenderEvent 99 247 0 0 0 0 t\n"
  "MouseMoveEvent 99 247 0 0 0 0 t\n"
  "LeftButtonPressEvent 99 247 0 0 0 0 t\n"
  "RenderEvent 99 247 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 99 247 0 0 0 0 t\n"
  "RenderEvent 99 247 0 0 0 0 t\n"
  "MouseMoveEvent 99 247 0 0 0 0 t\n"
  "MouseMoveEvent 98 246 0 0 0 0 t\n"
  "MouseMoveEvent 96 245 0 0 0 0 t\n"
  "MouseMoveEvent 99 245 0 0 0 0 t\n"
  "MouseMoveEvent 165 248 0 0 0 0 t\n"
  "RenderEvent 165 248 0 0 0 0 t\n"
  "MouseMoveEvent 206 247 0 0 0 0 t\n"
  "MouseMoveEvent 213 247 0 0 0 0 t\n"
  "MouseMoveEvent 216 247 0 0 0 0 t\n"
  "MouseMoveEvent 221 247 0 0 0 0 t\n"
  "MouseMoveEvent 227 247 0 0 0 0 t\n"
  "MouseMoveEvent 234 247 0 0 0 0 t\n"
  "MouseMoveEvent 238 247 0 0 0 0 t\n"
  "MouseMoveEvent 242 248 0 0 0 0 t\n"
  "MouseMoveEvent 247 248 0 0 0 0 t\n"
  "MouseMoveEvent 248 249 0 0 0 0 t\n"
  "MouseMoveEvent 251 249 0 0 0 0 t\n"
  "MouseMoveEvent 252 249 0 0 0 0 t\n"
  "MouseMoveEvent 253 249 0 0 0 0 t\n"
  "MouseMoveEvent 254 249 0 0 0 0 t\n"
  "RenderEvent 254 249 0 0 0 0 t\n"
  "MouseMoveEvent 264 252 0 0 0 0 t\n"
  "MouseMoveEvent 264 253 0 0 0 0 t\n"
  "LeftButtonPressEvent 264 253 0 0 0 0 t\n"
  "RenderEvent 264 253 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 264 253 0 0 0 0 t\n"
  "RenderEvent 264 253 0 0 0 0 t\n"
  "MouseMoveEvent 264 253 0 0 0 0 t\n"
  "LeftButtonPressEvent 264 253 0 0 0 0 t\n"
  "RenderEvent 264 253 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 264 253 0 0 0 0 t\n"
  "RenderEvent 264 253 0 0 0 0 t\n"
  "MouseMoveEvent 264 253 0 0 0 0 t\n"
  "MouseMoveEvent 264 252 0 0 0 0 t\n"
  "MouseMoveEvent 264 246 0 0 0 0 t\n"
  "RenderEvent 264 246 0 0 0 0 t\n"
  "MouseMoveEvent 263 236 0 0 0 0 t\n"
  "MouseMoveEvent 263 233 0 0 0 0 t\n"
  "MouseMoveEvent 262 230 0 0 0 0 t\n"
  "MouseMoveEvent 262 229 0 0 0 0 t\n"
  "MouseMoveEvent 262 226 0 0 0 0 t\n"
  "MouseMoveEvent 262 223 0 0 0 0 t\n"
  "MouseMoveEvent 262 222 0 0 0 0 t\n"
  "MouseMoveEvent 262 217 0 0 0 0 t\n"
  "MouseMoveEvent 262 215 0 0 0 0 t\n"
  "MouseMoveEvent 262 212 0 0 0 0 t\n"
  "MouseMoveEvent 263 209 0 0 0 0 t\n"
  "MouseMoveEvent 263 206 0 0 0 0 t\n"
  "MouseMoveEvent 263 203 0 0 0 0 t\n"
  "MouseMoveEvent 263 201 0 0 0 0 t\n"
  "MouseMoveEvent 263 198 0 0 0 0 t\n"
  "MouseMoveEvent 263 195 0 0 0 0 t\n"
  "MouseMoveEvent 263 193 0 0 0 0 t\n"
  "MouseMoveEvent 263 190 0 0 0 0 t\n"
  "MouseMoveEvent 263 187 0 0 0 0 t\n"
  "MouseMoveEvent 263 185 0 0 0 0 t\n"
  "MouseMoveEvent 263 183 0 0 0 0 t\n"
  "MouseMoveEvent 263 180 0 0 0 0 t\n"
  "MouseMoveEvent 263 178 0 0 0 0 t\n"
  "MouseMoveEvent 263 175 0 0 0 0 t\n"
  "RenderEvent 263 175 0 0 0 0 t\n"
  "MouseMoveEvent 263 170 0 0 0 0 t\n"
  "MouseMoveEvent 263 169 0 0 0 0 t\n"
  "MouseMoveEvent 263 168 0 0 0 0 t\n"
  "MouseMoveEvent 264 167 0 0 0 0 t\n"
  "MouseMoveEvent 264 165 0 0 0 0 t\n"
  "MouseMoveEvent 264 164 0 0 0 0 t\n"
  "MouseMoveEvent 264 162 0 0 0 0 t\n"
  "MouseMoveEvent 264 161 0 0 0 0 t\n"
  "MouseMoveEvent 264 160 0 0 0 0 t\n"
  "MouseMoveEvent 264 159 0 0 0 0 t\n"
  "MouseMoveEvent 264 158 0 0 0 0 t\n"
  "LeftButtonPressEvent 264 158 0 0 0 0 t\n"
  "RenderEvent 264 158 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 264 158 0 0 0 0 t\n"
  "RenderEvent 264 158 0 0 0 0 t\n"
  "MouseMoveEvent 264 158 0 0 0 0 t\n"
  "LeftButtonPressEvent 264 158 0 0 0 0 t\n"
  "RenderEvent 264 158 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 264 158 0 0 0 0 t\n"
  "RenderEvent 264 158 0 0 0 0 t\n"
  "MouseMoveEvent 264 158 0 0 0 0 t\n"
  "MouseMoveEvent 264 157 0 0 0 0 t\n"
  "MouseMoveEvent 264 156 0 0 0 0 t\n"
  "MouseMoveEvent 264 155 0 0 0 0 t\n"
  "MouseMoveEvent 264 154 0 0 0 0 t\n"
  "MouseMoveEvent 264 152 0 0 0 0 t\n"
  "MouseMoveEvent 264 148 0 0 0 0 t\n"
  "RenderEvent 264 148 0 0 0 0 t\n"
  "MouseMoveEvent 263 134 0 0 0 0 t\n"
  "MouseMoveEvent 263 131 0 0 0 0 t\n"
  "MouseMoveEvent 264 128 0 0 0 0 t\n"
  "MouseMoveEvent 264 121 0 0 0 0 t\n"
  "MouseMoveEvent 264 114 0 0 0 0 t\n"
  "MouseMoveEvent 264 108 0 0 0 0 t\n"
  "MouseMoveEvent 264 102 0 0 0 0 t\n"
  "MouseMoveEvent 264 98 0 0 0 0 t\n"
  "MouseMoveEvent 265 95 0 0 0 0 t\n"
  "MouseMoveEvent 265 91 0 0 0 0 t\n"
  "MouseMoveEvent 265 89 0 0 0 0 t\n"
  "MouseMoveEvent 265 88 0 0 0 0 t\n"
  "MouseMoveEvent 265 86 0 0 0 0 t\n"
  "MouseMoveEvent 265 84 0 0 0 0 t\n"
  "MouseMoveEvent 265 81 0 0 0 0 t\n"
  "MouseMoveEvent 266 79 0 0 0 0 t\n"
  "MouseMoveEvent 266 77 0 0 0 0 t\n"
  "MouseMoveEvent 267 75 0 0 0 0 t\n"
  "MouseMoveEvent 267 74 0 0 0 0 t\n"
  "MouseMoveEvent 267 71 0 0 0 0 t\n"
  "MouseMoveEvent 267 69 0 0 0 0 t\n"
  "MouseMoveEvent 267 67 0 0 0 0 t\n"
  "MouseMoveEvent 269 64 0 0 0 0 t\n"
  "MouseMoveEvent 270 62 0 0 0 0 t\n"
  "MouseMoveEvent 270 60 0 0 0 0 t\n"
  "MouseMoveEvent 271 58 0 0 0 0 t\n"
  "MouseMoveEvent 271 57 0 0 0 0 t\n"
  "MouseMoveEvent 271 56 0 0 0 0 t\n"
  "MouseMoveEvent 271 55 0 0 0 0 t\n"
  "MouseMoveEvent 271 54 0 0 0 0 t\n"
  "MouseMoveEvent 271 53 0 0 0 0 t\n"
  "MouseMoveEvent 271 52 0 0 0 0 t\n"
  "MouseMoveEvent 271 51 0 0 0 0 t\n"
  "MouseMoveEvent 271 49 0 0 0 0 t\n"
  "MouseMoveEvent 271 47 0 0 0 0 t\n"
  "MouseMoveEvent 272 45 0 0 0 0 t\n"
  "MouseMoveEvent 272 43 0 0 0 0 t\n"
  "RenderEvent 272 43 0 0 0 0 t\n"
  "MouseMoveEvent 272 38 0 0 0 0 t\n"
  "MouseMoveEvent 272 37 0 0 0 0 t\n"
  "MouseMoveEvent 271 36 0 0 0 0 t\n"
  "MouseMoveEvent 271 35 0 0 0 0 t\n"
  "MouseMoveEvent 270 35 0 0 0 0 t\n"
  "MouseMoveEvent 270 34 0 0 0 0 t\n"
  "LeftButtonPressEvent 270 34 0 0 0 0 t\n"
  "RenderEvent 270 34 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 270 34 0 0 0 0 t\n"
  "RenderEvent 270 34 0 0 0 0 t\n"
  "MouseMoveEvent 270 34 0 0 0 0 t\n"
  "LeftButtonPressEvent 270 34 0 0 0 0 t\n"
  "RenderEvent 270 34 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 270 34 0 0 0 0 t\n"
  "RenderEvent 270 34 0 0 0 0 t\n"
  "MouseMoveEvent 270 34 0 0 0 0 t\n"
  "LeftButtonPressEvent 270 34 0 0 0 0 t\n"
  "RenderEvent 270 34 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 270 34 0 0 0 0 t\n"
  "RenderEvent 270 34 0 0 0 0 t\n"
  "MouseMoveEvent 270 34 0 0 0 0 t\n"
  "LeftButtonPressEvent 270 34 0 0 0 0 t\n"
  "RenderEvent 270 34 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 270 34 0 0 0 0 t\n"
  "RenderEvent 270 34 0 0 0 0 t\n"
  "MouseMoveEvent 270 34 0 0 0 0 t\n"
  "LeftButtonPressEvent 270 34 0 0 0 0 t\n"
  "RenderEvent 270 34 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 270 34 0 0 0 0 t\n"
  "RenderEvent 270 34 0 0 0 0 t\n"
  "MouseMoveEvent 270 34 0 0 0 0 t\n"
  "MouseMoveEvent 269 34 0 0 0 0 t\n"
  "MouseMoveEvent 267 34 0 0 0 0 t\n"
  "MouseMoveEvent 266 34 0 0 0 0 t\n"
  "MouseMoveEvent 264 35 0 0 0 0 t\n"
  "MouseMoveEvent 260 35 0 0 0 0 t\n"
  "MouseMoveEvent 256 36 0 0 0 0 t\n"
  "MouseMoveEvent 251 37 0 0 0 0 t\n"
  "RenderEvent 251 37 0 0 0 0 t\n"
  "MouseMoveEvent 220 46 0 0 0 0 t\n"
  "MouseMoveEvent 210 51 0 0 0 0 t\n"
  "MouseMoveEvent 198 53 0 0 0 0 t\n"
  "MouseMoveEvent 188 56 0 0 0 0 t\n"
  "MouseMoveEvent 179 57 0 0 0 0 t\n"
  "MouseMoveEvent 169 57 0 0 0 0 t\n"
  "MouseMoveEvent 163 57 0 0 0 0 t\n"
  "MouseMoveEvent 152 58 0 0 0 0 t\n"
  "MouseMoveEvent 144 58 0 0 0 0 t\n"
  "MouseMoveEvent 137 58 0 0 0 0 t\n"
  "MouseMoveEvent 130 58 0 0 0 0 t\n"
  "MouseMoveEvent 124 60 0 0 0 0 t\n"
  "MouseMoveEvent 121 61 0 0 0 0 t\n"
  "MouseMoveEvent 119 62 0 0 0 0 t\n"
  "MouseMoveEvent 115 63 0 0 0 0 t\n"
  "MouseMoveEvent 110 66 0 0 0 0 t\n"
  "MouseMoveEvent 107 67 0 0 0 0 t\n"
  "MouseMoveEvent 99 69 0 0 0 0 t\n"
  "MouseMoveEvent 93 69 0 0 0 0 t\n"
  "MouseMoveEvent 84 70 0 0 0 0 t\n"
  "MouseMoveEvent 82 70 0 0 0 0 t\n"
  "MouseMoveEvent 76 70 0 0 0 0 t\n"
  "MouseMoveEvent 71 70 0 0 0 0 t\n"
  "MouseMoveEvent 67 70 0 0 0 0 t\n"
  "MouseMoveEvent 64 70 0 0 0 0 t\n"
  "RenderEvent 64 70 0 0 0 0 t\n"
  "MouseMoveEvent 61 68 0 0 0 0 t\n"
  "MouseMoveEvent 60 68 0 0 0 0 t\n"
  "MouseMoveEvent 59 68 0 0 0 0 t\n"
  "MouseMoveEvent 58 69 0 0 0 0 t\n"
  "MouseMoveEvent 57 69 0 0 0 0 t\n"
  "MouseMoveEvent 56 69 0 0 0 0 t\n"
  "MouseMoveEvent 55 68 0 0 0 0 t\n"
  "LeftButtonPressEvent 55 68 0 0 0 0 t\n"
  "RenderEvent 55 68 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 55 68 0 0 0 0 t\n"
  "RenderEvent 55 68 0 0 0 0 t\n"
  "MouseMoveEvent 55 68 0 0 0 0 t\n"
  "LeftButtonPressEvent 55 68 0 0 0 0 t\n"
  "RenderEvent 55 68 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 55 68 0 0 0 0 t\n"
  "RenderEvent 55 68 0 0 0 0 t\n"
  "MouseMoveEvent 55 68 0 0 0 0 t\n"
  "MouseMoveEvent 57 67 0 0 0 0 t\n"
  "MouseMoveEvent 64 66 0 0 0 0 t\n"
  "MouseMoveEvent 71 66 0 0 0 0 t\n"
  "RenderEvent 71 66 0 0 0 0 t\n"
  "MouseMoveEvent 83 64 0 0 0 0 t\n"
  "MouseMoveEvent 84 64 0 0 0 0 t\n"
  "MouseMoveEvent 84 63 0 0 0 0 t\n"
  "MouseMoveEvent 85 63 0 0 0 0 t\n"
  "MouseMoveEvent 86 63 0 0 0 0 t\n"
  "MouseMoveEvent 87 63 0 0 0 0 t\n"
  "MouseMoveEvent 88 63 0 0 0 0 t\n"
  "MouseMoveEvent 89 63 0 0 0 0 t\n"
  "MouseMoveEvent 89 64 0 0 0 0 t\n"
  "MouseMoveEvent 90 65 0 0 0 0 t\n"
  "MouseMoveEvent 92 65 0 0 0 0 t\n"
  "MouseMoveEvent 92 66 0 0 0 0 t\n"
  "MouseMoveEvent 93 66 0 0 0 0 t\n"
  "LeftButtonPressEvent 93 66 0 0 0 0 t\n"
  "StartInteractionEvent 93 66 0 0 0 0 t\n"
  "MouseMoveEvent 94 66 0 0 0 0 t\n"
  "RenderEvent 94 66 0 0 0 0 t\n"
  "MouseMoveEvent 103 63 0 0 0 0 t\n"
  "RenderEvent 103 63 0 0 0 0 t\n"
  "MouseMoveEvent 110 62 0 0 0 0 t\n"
  "RenderEvent 110 62 0 0 0 0 t\n"
  "MouseMoveEvent 118 61 0 0 0 0 t\n"
  "RenderEvent 118 61 0 0 0 0 t\n"
  "MouseMoveEvent 132 60 0 0 0 0 t\n"
  "RenderEvent 132 60 0 0 0 0 t\n"
  "MouseMoveEvent 138 60 0 0 0 0 t\n"
  "RenderEvent 138 60 0 0 0 0 t\n"
  "MouseMoveEvent 142 60 0 0 0 0 t\n"
  "RenderEvent 142 60 0 0 0 0 t\n"
  "MouseMoveEvent 150 60 0 0 0 0 t\n"
  "RenderEvent 150 60 0 0 0 0 t\n"
  "MouseMoveEvent 159 63 0 0 0 0 t\n"
  "RenderEvent 159 63 0 0 0 0 t\n"
  "MouseMoveEvent 168 63 0 0 0 0 t\n"
  "RenderEvent 168 63 0 0 0 0 t\n"
  "MouseMoveEvent 176 63 0 0 0 0 t\n"
  "RenderEvent 176 63 0 0 0 0 t\n"
  "MouseMoveEvent 185 65 0 0 0 0 t\n"
  "RenderEvent 185 65 0 0 0 0 t\n"
  "MouseMoveEvent 190 65 0 0 0 0 t\n"
  "RenderEvent 190 65 0 0 0 0 t\n"
  "MouseMoveEvent 195 65 0 0 0 0 t\n"
  "RenderEvent 195 65 0 0 0 0 t\n"
  "MouseMoveEvent 200 65 0 0 0 0 t\n"
  "RenderEvent 200 65 0 0 0 0 t\n"
  "MouseMoveEvent 202 65 0 0 0 0 t\n"
  "RenderEvent 202 65 0 0 0 0 t\n"
  "MouseMoveEvent 203 67 0 0 0 0 t\n"
  "RenderEvent 203 67 0 0 0 0 t\n"
  "MouseMoveEvent 204 67 0 0 0 0 t\n"
  "RenderEvent 204 67 0 0 0 0 t\n"
  "MouseMoveEvent 205 67 0 0 0 0 t\n"
  "RenderEvent 205 67 0 0 0 0 t\n"
  "MouseMoveEvent 206 68 0 0 0 0 t\n"
  "RenderEvent 206 68 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 206 68 0 0 0 0 t\n"
  "EndInteractionEvent 206 68 0 0 0 0 t\n"
  "RenderEvent 206 68 0 0 0 0 t\n"
  "MouseMoveEvent 206 68 0 0 0 0 t\n"
  "MouseMoveEvent 204 68 0 0 0 0 t\n"
  "MouseMoveEvent 202 69 0 0 0 0 t\n"
  "MouseMoveEvent 201 69 0 0 0 0 t\n"
  "MouseMoveEvent 200 69 0 0 0 0 t\n"
  "MouseMoveEvent 198 69 0 0 0 0 t\n"
  "MouseMoveEvent 196 69 0 0 0 0 t\n"
  "MouseMoveEvent 195 69 0 0 0 0 t\n"
  "MouseMoveEvent 192 69 0 0 0 0 t\n"
  "MouseMoveEvent 190 70 0 0 0 0 t\n"
  "MouseMoveEvent 187 70 0 0 0 0 t\n"
  "MouseMoveEvent 185 70 0 0 0 0 t\n"
  "MouseMoveEvent 181 70 0 0 0 0 t\n"
  "MouseMoveEvent 175 71 0 0 0 0 t\n"
  "MouseMoveEvent 171 72 0 0 0 0 t\n"
  "MouseMoveEvent 155 76 0 0 0 0 t\n"
  "MouseMoveEvent 141 78 0 0 0 0 t\n"
  "MouseMoveEvent 128 82 0 0 0 0 t\n"
  "MouseMoveEvent 114 85 0 0 0 0 t\n"
  "MouseMoveEvent 100 90 0 0 0 0 t\n"
  "MouseMoveEvent 87 94 0 0 0 0 t\n"
  "MouseMoveEvent 78 99 0 0 0 0 t\n"
  "MouseMoveEvent 68 102 0 0 0 0 t\n"
  "MouseMoveEvent 61 105 0 0 0 0 t\n"
  "MouseMoveEvent 57 108 0 0 0 0 t\n"
  "MouseMoveEvent 53 112 0 0 0 0 t\n"
  "MouseMoveEvent 50 113 0 0 0 0 t\n"
  "MouseMoveEvent 50 114 0 0 0 0 t\n"
  "MouseMoveEvent 49 114 0 0 0 0 t\n"
  "MouseMoveEvent 49 115 0 0 0 0 t\n"
  "MouseMoveEvent 49 116 0 0 0 0 t\n"
  "MouseMoveEvent 49 118 0 0 0 0 t\n"
  "MouseMoveEvent 49 120 0 0 0 0 t\n"
  "MouseMoveEvent 50 122 0 0 0 0 t\n"
  "MouseMoveEvent 52 124 0 0 0 0 t\n"
  "MouseMoveEvent 54 127 0 0 0 0 t\n"
  "MouseMoveEvent 56 128 0 0 0 0 t\n"
  "MouseMoveEvent 57 131 0 0 0 0 t\n"
  "MouseMoveEvent 58 133 0 0 0 0 t\n"
  "MouseMoveEvent 59 135 0 0 0 0 t\n"
  "MouseMoveEvent 59 136 0 0 0 0 t\n"
  "MouseMoveEvent 59 138 0 0 0 0 t\n"
  "MouseMoveEvent 59 139 0 0 0 0 t\n"
  "MouseMoveEvent 59 140 0 0 0 0 t\n"
  "MouseMoveEvent 59 141 0 0 0 0 t\n"
  "MouseMoveEvent 59 142 0 0 0 0 t\n"
  "MouseMoveEvent 59 143 0 0 0 0 t\n"
  "MouseMoveEvent 61 143 0 0 0 0 t\n"
  "RenderEvent 61 143 0 0 0 0 t\n"
  "MouseMoveEvent 64 145 0 0 0 0 t\n"
  "MouseMoveEvent 64 146 0 0 0 0 t\n"
  "MouseMoveEvent 65 146 0 0 0 0 t\n"
  "MouseMoveEvent 66 146 0 0 0 0 t\n"
  "MouseMoveEvent 66 147 0 0 0 0 t\n"
  "LeftButtonPressEvent 66 147 0 0 0 0 t\n"
  "RenderEvent 66 147 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 66 147 0 0 0 0 t\n"
  "RenderEvent 66 147 0 0 0 0 t\n"
  "MouseMoveEvent 66 147 0 0 0 0 t\n"
  "LeftButtonPressEvent 66 147 0 0 0 0 t\n"
  "RenderEvent 66 147 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 66 147 0 0 0 0 t\n"
  "RenderEvent 66 147 0 0 0 0 t\n"
  "MouseMoveEvent 66 147 0 0 0 0 t\n"
  "MouseMoveEvent 66 148 0 0 0 0 t\n"
  "MouseMoveEvent 65 148 0 0 0 0 t\n"
  ;

// This does the actual work: updates the vtkPlane implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkButtonCallback : public vtkCommand
{
public:
  static vtkButtonCallback *New()
    { return new vtkButtonCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
      vtkButtonWidget *buttonWidget =
        reinterpret_cast<vtkButtonWidget*>(caller);
      vtkTexturedButtonRepresentation *rep =
        reinterpret_cast<vtkTexturedButtonRepresentation*>
        (buttonWidget->GetRepresentation());
      int state = rep->GetState();
      cout << "State: " << state << "\n";
      this->Glyph->SetScaleFactor(0.05*(1+state));
  }
  vtkButtonCallback():Glyph(0) {}
  vtkGlyph3D *Glyph;
};

int TestButtonWidget(int argc, char *argv[] )
{
  // Create an image for the button
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.tif");
  VTK_CREATE(vtkTIFFReader, image1);
  image1->SetFileName(fname);
  image1->SetOrientationType( 4 );
  image1->Update();
  delete [] fname;

  // Create an image for the button
  char* fname2 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/fran_cut.png");
  VTK_CREATE(vtkPNGReader, image2);
  image2->SetFileName(fname2);
  image2->Update();
  delete [] fname2;

  // Create a mace out of filters.
  //
  VTK_CREATE(vtkSphereSource, sphere);
  VTK_CREATE(vtkConeSource, cone);
  VTK_CREATE(vtkGlyph3D, glyph);
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);
  glyph->Update();

  // Appending just makes things simpler to manage.
  VTK_CREATE(vtkAppendPolyData, apd);
  apd->AddInputConnection(glyph->GetOutputPort());
  apd->AddInputConnection(sphere->GetOutputPort());

  VTK_CREATE(vtkPolyDataMapper, maceMapper);
  maceMapper->SetInputConnection(apd->GetOutputPort());

  VTK_CREATE(vtkActor, maceActor);
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();

  // Create the RenderWindow, Renderer and both Actors
  //
  VTK_CREATE(vtkRenderer, ren1);
  VTK_CREATE(vtkRenderWindow, renWin);
  renWin->AddRenderer(ren1);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renWin);

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  VTK_CREATE(vtkButtonCallback, myCallback);
  myCallback->Glyph = glyph;

  VTK_CREATE(vtkEllipticalButtonSource, button);
  button->TwoSidedOn();
  button->SetCircumferentialResolution(24);
  button->SetShoulderResolution(24);
  button->SetTextureResolution(24);

  VTK_CREATE(vtkTexturedButtonRepresentation, rep);
  rep->SetNumberOfStates(2);
  rep->SetButtonTexture(0,image1->GetOutput());
  rep->SetButtonTexture(1,image2->GetOutput());
  rep->SetButtonGeometryConnection(button->GetOutputPort());
  rep->SetPlaceFactor(1);
  double bds[6];
  bds[0] = 0.6; bds[1] = 0.75; bds[2] = 0.6; bds[3] = 0.75; bds[4] = 0.6; bds[5] = 0.75;
  rep->PlaceWidget(bds);
  rep->FollowCameraOn();

  VTK_CREATE(vtkButtonWidget, buttonWidget);
  buttonWidget->SetInteractor(iren);
  buttonWidget->SetRepresentation(rep);
  buttonWidget->AddObserver(vtkCommand::StateChangedEvent,myCallback);

  // Another 3D button widget, this time use alternative PlaceWidget() method
  VTK_CREATE(vtkEllipticalButtonSource, button2);
  button2->TwoSidedOn();
  button2->SetCircumferentialResolution(24);
  button2->SetShoulderResolution(24);
  button2->SetTextureResolution(24);
  button2->SetWidth(0.65);
  button2->SetHeight(0.45);
  button2->SetTextureStyleToFitImage();

  VTK_CREATE(vtkTexturedButtonRepresentation, rep2);
  rep2->SetNumberOfStates(2);
  rep2->SetButtonTexture(0,image1->GetOutput());
  rep2->SetButtonTexture(1,image2->GetOutput());
  rep2->SetButtonGeometryConnection(button2->GetOutputPort());
  rep2->SetPlaceFactor(1);
  bds[0] = 0.0; bds[1] = 0.0; bds[2] = 0.65; bds[3] = 0.0; bds[4] = 0.0; bds[5] = 1;
  rep2->PlaceWidget(0.5, bds, bds+3);
  rep2->FollowCameraOff();

  VTK_CREATE(vtkButtonWidget, buttonWidget2);
  buttonWidget2->SetInteractor(iren);
  buttonWidget2->SetRepresentation(rep2);
  buttonWidget2->AddObserver(vtkCommand::StateChangedEvent,myCallback);

  // Okay now for the 2D version of the widget (in display space)
  VTK_CREATE(vtkTexturedButtonRepresentation2D, rep3);
  rep3->SetNumberOfStates(2);
  rep3->SetButtonTexture(0,image1->GetOutput());
  rep3->SetButtonTexture(1,image2->GetOutput());
  rep3->SetPlaceFactor(1);
  bds[0] = 25; bds[1] = 65; bds[2] = 50; bds[3] = 200;
  rep3->PlaceWidget(bds);

  VTK_CREATE(vtkButtonWidget, buttonWidget3);
  buttonWidget3->SetInteractor(iren);
  buttonWidget3->SetRepresentation(rep3);
  buttonWidget3->AddObserver(vtkCommand::StateChangedEvent,myCallback);

  // Okay now for the 2D version of the widget (in world space)
  VTK_CREATE(vtkTexturedButtonRepresentation2D, rep4);
  rep4->SetNumberOfStates(2);
  rep4->SetButtonTexture(0,image1->GetOutput());
  rep4->SetButtonTexture(1,image2->GetOutput());
  rep4->SetPlaceFactor(1);
  bds[0] = 0.75; bds[1] = 0; bds[2] = 0;
  int size[2]; size[0] = 25; size[1] = 45;
  rep4->PlaceWidget(bds,size);

  VTK_CREATE(vtkButtonWidget, buttonWidget4);
  buttonWidget4->SetInteractor(iren);
  buttonWidget4->SetRepresentation(rep4);
  buttonWidget4->AddObserver(vtkCommand::StateChangedEvent,myCallback);

  // Finally now a set of vtkProp3D's to define a vtkProp3DButtonRepresentation
  VTK_CREATE(vtkLookupTable,lut);
  lut->SetNumberOfColors(20);
  lut->Build();
  lut->SetTableValue(0, 1, 0, 0, 1);
  lut->SetTableValue(1, 0, 1, 0, 1);
  lut->SetTableValue(2, 1, 1, 0, 1);
  lut->SetTableValue(3, 0, 0, 1, 1);
  lut->SetTableValue(4, 1, 0, 1, 1);
  lut->SetTableValue(5, 0, 1, 1, 1);
  lut->SetTableValue(6, 0.0000, 1.0000, 0.4980, 1.0);
  lut->SetTableValue(7, 0.9020, 0.9020, 0.9804, 1.0);
  lut->SetTableValue(8, 0.9608, 1.0000, 0.9804, 1.0);
  lut->SetTableValue(9, 0.5600, 0.3700, 0.6000, 1.0);
  lut->SetTableValue(10, 0.1600, 0.1400, 0.1300, 1.0);
  lut->SetTableValue(11, 1.0000, 0.4980, 0.3137, 1.0);
  lut->SetTableValue(12, 1.0000, 0.7529, 0.7961, 1.0);
  lut->SetTableValue(13, 0.9804, 0.5020, 0.4471, 1.0);
  lut->SetTableValue(14, 0.3700, 0.1500, 0.0700, 1.0);
  lut->SetTableValue(15, 0.9300, 0.5700, 0.1300, 1.0);
  lut->SetTableValue(16, 1.0000, 0.8431, 0.0000, 1.0);
  lut->SetTableValue(17, 0.1333, 0.5451, 0.1333, 1.0);
  lut->SetTableValue(18, 0.2510, 0.8784, 0.8157, 1.0);
  lut->SetTableValue(19, 0.8667, 0.6275, 0.8667, 1.0);
  lut->SetTableRange(0, 19);

  VTK_CREATE(vtkPlatonicSolidSource,tet);
  tet->SetSolidTypeToTetrahedron();
  VTK_CREATE(vtkPolyDataMapper,tetMapper);
  tetMapper->SetInputConnection(tet->GetOutputPort());
  tetMapper->SetLookupTable(lut);
  tetMapper->SetScalarRange(0,19);
  VTK_CREATE(vtkActor,tetActor);
  tetActor->SetMapper(tetMapper);

  VTK_CREATE(vtkPlatonicSolidSource,cube);
  cube->SetSolidTypeToCube();
  VTK_CREATE(vtkPolyDataMapper,cubeMapper);
  cubeMapper->SetInputConnection(cube->GetOutputPort());
  cubeMapper->SetLookupTable(lut);
  cubeMapper->SetScalarRange(0,19);
  VTK_CREATE(vtkActor,cubeActor);
  cubeActor->SetMapper(cubeMapper);

  VTK_CREATE(vtkPlatonicSolidSource,oct);
  oct->SetSolidTypeToOctahedron();
  VTK_CREATE(vtkPolyDataMapper,octMapper);
  octMapper->SetInputConnection(oct->GetOutputPort());
  octMapper->SetLookupTable(lut);
  octMapper->SetScalarRange(0,19);
  VTK_CREATE(vtkActor,octActor);
  octActor->SetMapper(octMapper);

  VTK_CREATE(vtkPlatonicSolidSource,ico);
  ico->SetSolidTypeToIcosahedron();
  VTK_CREATE(vtkPolyDataMapper,icoMapper);
  icoMapper->SetInputConnection(ico->GetOutputPort());
  icoMapper->SetLookupTable(lut);
  icoMapper->SetScalarRange(0,19);
  VTK_CREATE(vtkActor,icoActor);
  icoActor->SetMapper(icoMapper);

  VTK_CREATE(vtkPlatonicSolidSource,dode);
  dode->SetSolidTypeToDodecahedron();
  VTK_CREATE(vtkPolyDataMapper,dodeMapper);
  dodeMapper->SetInputConnection(dode->GetOutputPort());
  dodeMapper->SetLookupTable(lut);
  dodeMapper->SetScalarRange(0,19);
  VTK_CREATE(vtkActor,dodeActor);
  dodeActor->SetMapper(dodeMapper);

  VTK_CREATE(vtkProp3DButtonRepresentation, rep5);
  rep5->SetNumberOfStates(5);
  rep5->SetButtonProp(0,tetActor);
  rep5->SetButtonProp(1,cubeActor);
  rep5->SetButtonProp(2,octActor);
  rep5->SetButtonProp(3,icoActor);
  rep5->SetButtonProp(4,dodeActor);
  rep5->SetPlaceFactor(1);
  bds[0] = 0.65; bds[1] = 0.75; bds[2] = -0.75; bds[3] = -0.65; bds[4] = 0.65; bds[5] = 0.75;
  rep5->PlaceWidget(bds);
  rep5->FollowCameraOn();

  VTK_CREATE(vtkButtonWidget, buttonWidget5);
  buttonWidget5->SetInteractor(iren);
  buttonWidget5->SetRepresentation(rep5);
  buttonWidget5->AddObserver(vtkCommand::StateChangedEvent,myCallback);

  // A volume rendered button!
  // Create the reader for the data
  char* fname3 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ironProt.vtk");
  VTK_CREATE(vtkStructuredPointsReader, reader);
  reader->SetFileName(fname3);
  delete [] fname3;

  // Create transfer mapping scalar value to opacity
  VTK_CREATE(vtkPiecewiseFunction, opacityTransferFunction);
  opacityTransferFunction->AddPoint(20,0);
  opacityTransferFunction->AddPoint(255,1);

  // Create transfer mapping scalar value to color
  VTK_CREATE(vtkColorTransferFunction, colorTransferFunction);
  colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(64.0, 1.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(128.0, 0.0, 0.0, 1.0);
  colorTransferFunction->AddRGBPoint(192.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(255.0, 0.0, 0.2, 0.0);

  // The property describes how the data will look
  VTK_CREATE(vtkVolumeProperty, volumeProperty);
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationTypeToLinear();

  // The mapper / ray cast function know how to render the data
  VTK_CREATE(vtkVolumeTextureMapper2D, volumeMapper);
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  // The volume holds the mapper and the property and
  // can be used to position/orient the volume
  VTK_CREATE(vtkVolume, volume);
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  // Create transfer mapping scalar value to color
  VTK_CREATE(vtkColorTransferFunction, colorTransferFunction2);
  colorTransferFunction2->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  colorTransferFunction2->AddRGBPoint(64.0, 0.0, 0.0, 1.0);
  colorTransferFunction2->AddRGBPoint(128.0, 1.0, 0.0, 1.0);
  colorTransferFunction2->AddRGBPoint(192.0, 0.5, 0.0, 0.5);
  colorTransferFunction2->AddRGBPoint(255.0, 0.2, 0.0, 0.2);

  // The property describes how the data will look
  VTK_CREATE(vtkVolumeProperty, volumeProperty2);
  volumeProperty2->SetColor(colorTransferFunction2);
  volumeProperty2->SetScalarOpacity(opacityTransferFunction);
  volumeProperty2->ShadeOn();
  volumeProperty2->SetInterpolationTypeToLinear();

  // The mapper / ray cast function know how to render the data
  VTK_CREATE(vtkVolumeTextureMapper2D, volumeMapper2);
  volumeMapper2->SetInputConnection(reader->GetOutputPort());

  VTK_CREATE(vtkVolume, volume2);
  volume2->SetMapper(volumeMapper2);
  volume2->SetProperty(volumeProperty2);

  VTK_CREATE(vtkProp3DButtonRepresentation, rep6);
  rep6->SetNumberOfStates(2);
  rep6->SetButtonProp(0,volume);
  rep6->SetButtonProp(1,volume2);
  rep6->SetPlaceFactor(1);
  bds[0] = -0.75; bds[1] = -0.35; bds[2] = 0.6; bds[3] = 1; bds[4] = -1; bds[5] = -0.6;
  rep6->PlaceWidget(bds);
  rep6->FollowCameraOn();

  VTK_CREATE(vtkButtonWidget, buttonWidget6);
  buttonWidget6->SetInteractor(iren);
  buttonWidget6->SetRepresentation(rep6);
  buttonWidget6->AddObserver(vtkCommand::StateChangedEvent,myCallback);

  // Outline is for debugging
  VTK_CREATE(vtkOutlineSource, outline);
  outline->SetBounds(bds);

  VTK_CREATE(vtkPolyDataMapper, outlineMapper);
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  VTK_CREATE(vtkActor, outlineActor);
  outlineActor->SetMapper(outlineMapper);

  ren1->AddActor(maceActor);
//  ren1->AddActor(outlineActor); //used for debugging

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
//  recorder->SetFileName("record.log");
//  recorder->Record();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(ButtonWidgetEventLog);
  recorder->EnabledOn();

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  buttonWidget->EnabledOn();
  buttonWidget2->EnabledOn();
  buttonWidget3->EnabledOn();
  buttonWidget4->EnabledOn();
  buttonWidget5->EnabledOn();
  buttonWidget6->EnabledOn();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
