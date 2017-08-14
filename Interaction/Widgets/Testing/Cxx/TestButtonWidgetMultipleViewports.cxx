
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestButtonWidgetMultipleViewports.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkButtonWidget.h"
#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkCoordinate.h"
#include "vtkEllipticalButtonSource.h"
#include "vtkGlyph3D.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPNGReader.h"
#include "vtkPlatonicSolidSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3DButtonRepresentation.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkStructuredPointsReader.h"
#include "vtkTIFFReader.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkTexturedButtonRepresentation.h"
#include "vtkTexturedButtonRepresentation2D.h"

static char ButtonWidgetMultipleViewportsEventLog[] =
  "# StreamVersion 1\n"
  "EnterEvent 198 290 0 0 0 0 0\n"
  "MouseMoveEvent 194 296 0 0 0 0 0\n"
  "MouseMoveEvent 190 299 0 0 0 0 0\n"
  "LeaveEvent 185 303 0 0 0 0 0\n"
  "EnterEvent 187 295 0 0 0 0 0\n"
  "MouseMoveEvent 187 295 0 0 0 0 0\n"
  "MouseMoveEvent 190 287 0 0 0 0 0\n"
  "MouseMoveEvent 194 277 0 0 0 0 0\n"
  "MouseMoveEvent 199 265 0 0 0 0 0\n"
  "MouseMoveEvent 204 253 0 0 0 0 0\n"
  "MouseMoveEvent 208 240 0 0 0 0 0\n"
  "MouseMoveEvent 213 225 0 0 0 0 0\n"
  "MouseMoveEvent 217 212 0 0 0 0 0\n"
  "MouseMoveEvent 220 199 0 0 0 0 0\n"
  "MouseMoveEvent 222 194 0 0 0 0 0\n"
  "MouseMoveEvent 224 183 0 0 0 0 0\n"
  "MouseMoveEvent 225 174 0 0 0 0 0\n"
  "RenderEvent 225 174 0 0 0 0 0\n"
  "MouseMoveEvent 226 168 0 0 0 0 0\n"
  "MouseMoveEvent 226 165 0 0 0 0 0\n"
  "MouseMoveEvent 226 162 0 0 0 0 0\n"
  "LeftButtonPressEvent 226 162 0 0 0 0 0\n"
  "RenderEvent 226 162 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 226 162 0 0 0 0 0\n"
  "RenderEvent 226 162 0 0 0 0 0\n"
  "LeftButtonPressEvent 226 162 0 0 0 0 0\n"
  "RenderEvent 226 162 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 226 162 0 0 0 0 0\n"
  "RenderEvent 226 162 0 0 0 0 0\n"
  "LeftButtonPressEvent 226 162 0 0 0 0 0\n"
  "RenderEvent 226 162 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 226 162 0 0 0 0 0\n"
  "RenderEvent 226 162 0 0 0 0 0\n"
  "MouseMoveEvent 226 161 0 0 0 0 0\n"
  "MouseMoveEvent 226 161 0 0 0 0 0\n"
  "MouseMoveEvent 226 158 0 0 0 0 0\n"
  "MouseMoveEvent 226 158 0 0 0 0 0\n"
  "MouseMoveEvent 225 154 0 0 0 0 0\n"
  "MouseMoveEvent 225 154 0 0 0 0 0\n"
  "MouseMoveEvent 224 150 0 0 0 0 0\n"
  "MouseMoveEvent 224 150 0 0 0 0 0\n"
  "MouseMoveEvent 223 145 0 0 0 0 0\n"
  "MouseMoveEvent 223 145 0 0 0 0 0\n"
  "MouseMoveEvent 221 140 0 0 0 0 0\n"
  "MouseMoveEvent 221 140 0 0 0 0 0\n"
  "MouseMoveEvent 220 135 0 0 0 0 0\n"
  "MouseMoveEvent 220 135 0 0 0 0 0\n"
  "MouseMoveEvent 218 131 0 0 0 0 0\n"
  "MouseMoveEvent 218 131 0 0 0 0 0\n"
  "MouseMoveEvent 215 123 0 0 0 0 0\n"
  "MouseMoveEvent 215 123 0 0 0 0 0\n"
  "MouseMoveEvent 213 117 0 0 0 0 0\n"
  "RenderEvent 213 117 0 0 0 0 0\n"
  "MouseMoveEvent 213 117 0 0 0 0 0\n"
  "MouseMoveEvent 211 111 0 0 0 0 0\n"
  "MouseMoveEvent 211 111 0 0 0 0 0\n"
  "MouseMoveEvent 210 110 0 0 0 0 0\n"
  "MouseMoveEvent 210 110 0 0 0 0 0\n"
  "MouseMoveEvent 209 106 0 0 0 0 0\n"
  "MouseMoveEvent 209 106 0 0 0 0 0\n"
  "MouseMoveEvent 207 103 0 0 0 0 0\n"
  "MouseMoveEvent 207 103 0 0 0 0 0\n"
  "MouseMoveEvent 206 101 0 0 0 0 0\n"
  "MouseMoveEvent 206 101 0 0 0 0 0\n"
  "MouseMoveEvent 205 98 0 0 0 0 0\n"
  "MouseMoveEvent 205 98 0 0 0 0 0\n"
  "MouseMoveEvent 204 96 0 0 0 0 0\n"
  "MouseMoveEvent 204 96 0 0 0 0 0\n"
  "MouseMoveEvent 204 95 0 0 0 0 0\n"
  "MouseMoveEvent 204 95 0 0 0 0 0\n"
  "MouseMoveEvent 203 94 0 0 0 0 0\n"
  "MouseMoveEvent 203 94 0 0 0 0 0\n"
  "MouseMoveEvent 203 92 0 0 0 0 0\n"
  "MouseMoveEvent 203 92 0 0 0 0 0\n"
  "MouseMoveEvent 202 92 0 0 0 0 0\n"
  "MouseMoveEvent 202 92 0 0 0 0 0\n"
  "MouseMoveEvent 202 91 0 0 0 0 0\n"
  "MouseMoveEvent 202 91 0 0 0 0 0\n"
  "MouseMoveEvent 202 91 0 0 0 0 0\n"
  "MouseMoveEvent 202 91 0 0 0 0 0\n"
  "MouseMoveEvent 202 90 0 0 0 0 0\n"
  "RenderEvent 202 90 0 0 0 0 0\n"
  "MouseMoveEvent 202 90 0 0 0 0 0\n"
  "MouseMoveEvent 202 90 0 0 0 0 0\n"
  "MouseMoveEvent 202 90 0 0 0 0 0\n"
  "MouseMoveEvent 202 90 0 0 0 0 0\n"
  "MouseMoveEvent 202 90 0 0 0 0 0\n"
  "MouseMoveEvent 202 90 0 0 0 0 0\n"
  "MouseMoveEvent 202 90 0 0 0 0 0\n"
  "MouseMoveEvent 201 89 0 0 0 0 0\n"
  "MouseMoveEvent 201 89 0 0 0 0 0\n"
  "MouseMoveEvent 201 89 0 0 0 0 0\n"
  "MouseMoveEvent 201 89 0 0 0 0 0\n"
  "MouseMoveEvent 201 89 0 0 0 0 0\n"
  "MouseMoveEvent 201 89 0 0 0 0 0\n"
  "MouseMoveEvent 201 88 0 0 0 0 0\n"
  "MouseMoveEvent 201 88 0 0 0 0 0\n"
  "MouseMoveEvent 200 87 0 0 0 0 0\n"
  "MouseMoveEvent 200 87 0 0 0 0 0\n"
  "MouseMoveEvent 200 86 0 0 0 0 0\n"
  "MouseMoveEvent 200 86 0 0 0 0 0\n"
  "MouseMoveEvent 200 85 0 0 0 0 0\n"
  "MouseMoveEvent 200 85 0 0 0 0 0\n"
  "MouseMoveEvent 200 84 0 0 0 0 0\n"
  "MouseMoveEvent 200 84 0 0 0 0 0\n"
  "MouseMoveEvent 199 84 0 0 0 0 0\n"
  "MouseMoveEvent 199 84 0 0 0 0 0\n"
  "MouseMoveEvent 199 83 0 0 0 0 0\n"
  "MouseMoveEvent 199 83 0 0 0 0 0\n"
  "MouseMoveEvent 199 83 0 0 0 0 0\n"
  "MouseMoveEvent 199 83 0 0 0 0 0\n"
  "MouseMoveEvent 199 82 0 0 0 0 0\n"
  "MouseMoveEvent 199 82 0 0 0 0 0\n"
  "MouseMoveEvent 199 82 0 0 0 0 0\n"
  "MouseMoveEvent 199 82 0 0 0 0 0\n"
  "MouseMoveEvent 198 82 0 0 0 0 0\n"
  "MouseMoveEvent 198 82 0 0 0 0 0\n"
  "LeftButtonPressEvent 198 82 0 0 0 0 0\n"
  "RenderEvent 198 82 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 198 82 0 0 0 0 0\n"
  "RenderEvent 198 82 0 0 0 0 0\n"
  "MouseMoveEvent 198 83 0 0 0 0 0\n"
  "MouseMoveEvent 198 83 0 0 0 0 0\n"
  "MouseMoveEvent 198 84 0 0 0 0 0\n"
  "MouseMoveEvent 198 84 0 0 0 0 0\n"
  "MouseMoveEvent 198 87 0 0 0 0 0\n"
  "MouseMoveEvent 198 87 0 0 0 0 0\n"
  "MouseMoveEvent 198 90 0 0 0 0 0\n"
  "MouseMoveEvent 198 90 0 0 0 0 0\n"
  "MouseMoveEvent 199 95 0 0 0 0 0\n"
  "RenderEvent 199 95 0 0 0 0 0\n"
  "MouseMoveEvent 199 95 0 0 0 0 0\n"
  "MouseMoveEvent 201 102 0 0 0 0 0\n"
  "MouseMoveEvent 201 102 0 0 0 0 0\n"
  "MouseMoveEvent 202 110 0 0 0 0 0\n"
  "MouseMoveEvent 202 110 0 0 0 0 0\n"
  "MouseMoveEvent 204 120 0 0 0 0 0\n"
  "MouseMoveEvent 204 120 0 0 0 0 0\n"
  "MouseMoveEvent 207 130 0 0 0 0 0\n"
  "RenderEvent 207 130 0 0 0 0 0\n"
  "MouseMoveEvent 207 130 0 0 0 0 0\n"
  "MouseMoveEvent 209 141 0 0 0 0 0\n"
  "MouseMoveEvent 209 141 0 0 0 0 0\n"
  "MouseMoveEvent 212 153 0 0 0 0 0\n"
  "MouseMoveEvent 212 153 0 0 0 0 0\n"
  "MouseMoveEvent 215 164 0 0 0 0 0\n"
  "MouseMoveEvent 215 164 0 0 0 0 0\n"
  "MouseMoveEvent 220 176 0 0 0 0 0\n"
  "MouseMoveEvent 220 176 0 0 0 0 0\n"
  "MouseMoveEvent 224 187 0 0 0 0 0\n"
  "RenderEvent 224 187 0 0 0 0 0\n"
  "MouseMoveEvent 224 187 0 0 0 0 0\n"
  "MouseMoveEvent 228 197 0 0 0 0 0\n"
  "MouseMoveEvent 228 197 0 0 0 0 0\n"
  "MouseMoveEvent 232 205 0 0 0 0 0\n"
  "MouseMoveEvent 232 205 0 0 0 0 0\n"
  "MouseMoveEvent 237 214 0 0 0 0 0\n"
  "MouseMoveEvent 237 214 0 0 0 0 0\n"
  "MouseMoveEvent 239 218 0 0 0 0 0\n"
  "MouseMoveEvent 239 218 0 0 0 0 0\n"
  "MouseMoveEvent 243 224 0 0 0 0 0\n"
  "MouseMoveEvent 243 224 0 0 0 0 0\n"
  "MouseMoveEvent 246 229 0 0 0 0 0\n"
  "MouseMoveEvent 246 229 0 0 0 0 0\n"
  "MouseMoveEvent 249 234 0 0 0 0 0\n"
  "MouseMoveEvent 249 234 0 0 0 0 0\n"
  "MouseMoveEvent 251 238 0 0 0 0 0\n"
  "MouseMoveEvent 251 238 0 0 0 0 0\n"
  "MouseMoveEvent 254 242 0 0 0 0 0\n"
  "MouseMoveEvent 254 242 0 0 0 0 0\n"
  "MouseMoveEvent 254 243 0 0 0 0 0\n"
  "MouseMoveEvent 254 243 0 0 0 0 0\n"
  "MouseMoveEvent 257 247 0 0 0 0 0\n"
  "MouseMoveEvent 257 247 0 0 0 0 0\n"
  "MouseMoveEvent 258 248 0 0 0 0 0\n"
  "MouseMoveEvent 258 248 0 0 0 0 0\n"
  "MouseMoveEvent 259 250 0 0 0 0 0\n"
  "MouseMoveEvent 259 250 0 0 0 0 0\n"
  "MouseMoveEvent 260 251 0 0 0 0 0\n"
  "MouseMoveEvent 260 251 0 0 0 0 0\n"
  "MouseMoveEvent 261 253 0 0 0 0 0\n"
  "MouseMoveEvent 261 253 0 0 0 0 0\n"
  "MouseMoveEvent 262 254 0 0 0 0 0\n"
  "MouseMoveEvent 262 254 0 0 0 0 0\n"
  "MouseMoveEvent 262 254 0 0 0 0 0\n"
  "MouseMoveEvent 262 254 0 0 0 0 0\n"
  "MouseMoveEvent 263 255 0 0 0 0 0\n"
  "MouseMoveEvent 263 255 0 0 0 0 0\n"
  "MouseMoveEvent 263 256 0 0 0 0 0\n"
  "MouseMoveEvent 263 256 0 0 0 0 0\n"
  "MouseMoveEvent 264 256 0 0 0 0 0\n"
  "MouseMoveEvent 264 256 0 0 0 0 0\n"
  "MouseMoveEvent 264 256 0 0 0 0 0\n"
  "MouseMoveEvent 264 256 0 0 0 0 0\n"
  "MouseMoveEvent 264 257 0 0 0 0 0\n"
  "MouseMoveEvent 264 257 0 0 0 0 0\n"
  "MouseMoveEvent 265 257 0 0 0 0 0\n"
  "MouseMoveEvent 265 257 0 0 0 0 0\n"
  "MouseMoveEvent 265 257 0 0 0 0 0\n"
  "MouseMoveEvent 265 257 0 0 0 0 0\n"
  "MouseMoveEvent 265 257 0 0 0 0 0\n"
  "MouseMoveEvent 265 257 0 0 0 0 0\n"
  "MouseMoveEvent 265 257 0 0 0 0 0\n"
  "MouseMoveEvent 265 257 0 0 0 0 0\n"
  "MouseMoveEvent 266 258 0 0 0 0 0\n"
  "MouseMoveEvent 266 258 0 0 0 0 0\n"
  "MouseMoveEvent 266 258 0 0 0 0 0\n"
  "MouseMoveEvent 266 258 0 0 0 0 0\n"
  "MouseMoveEvent 266 258 0 0 0 0 0\n"
  "MouseMoveEvent 266 258 0 0 0 0 0\n"
  "MouseMoveEvent 267 258 0 0 0 0 0\n"
  "MouseMoveEvent 267 258 0 0 0 0 0\n"
  "MouseMoveEvent 267 258 0 0 0 0 0\n"
  "MouseMoveEvent 267 258 0 0 0 0 0\n"
  "MouseMoveEvent 267 258 0 0 0 0 0\n"
  "MouseMoveEvent 267 258 0 0 0 0 0\n"
  "MouseMoveEvent 267 258 0 0 0 0 0\n"
  "MouseMoveEvent 267 258 0 0 0 0 0\n"
  "MouseMoveEvent 267 258 0 0 0 0 0\n"
  "MouseMoveEvent 267 258 0 0 0 0 0\n"
  "MouseMoveEvent 267 259 0 0 0 0 0\n"
  "MouseMoveEvent 267 259 0 0 0 0 0\n"
  "MouseMoveEvent 268 260 0 0 0 0 0\n"
  "MouseMoveEvent 268 260 0 0 0 0 0\n"
  "MouseMoveEvent 268 261 0 0 0 0 0\n"
  "MouseMoveEvent 268 261 0 0 0 0 0\n"
  "MouseMoveEvent 268 262 0 0 0 0 0\n"
  "MouseMoveEvent 268 262 0 0 0 0 0\n"
  "MouseMoveEvent 269 263 0 0 0 0 0\n"
  "MouseMoveEvent 269 263 0 0 0 0 0\n"
  "MouseMoveEvent 271 266 0 0 0 0 0\n"
  "MouseMoveEvent 271 266 0 0 0 0 0\n"
  "MouseMoveEvent 272 268 0 0 0 0 0\n"
  "MouseMoveEvent 272 268 0 0 0 0 0\n"
  "MouseMoveEvent 272 269 0 0 0 0 0\n"
  "MouseMoveEvent 272 269 0 0 0 0 0\n"
  "MouseMoveEvent 274 271 0 0 0 0 0\n"
  "MouseMoveEvent 274 271 0 0 0 0 0\n"
  "MouseMoveEvent 275 273 0 0 0 0 0\n"
  "MouseMoveEvent 275 273 0 0 0 0 0\n"
  "MouseMoveEvent 276 275 0 0 0 0 0\n"
  "RenderEvent 276 275 0 0 0 0 0\n"
  "MouseMoveEvent 276 275 0 0 0 0 0\n"
  "MouseMoveEvent 276 277 0 0 0 0 0\n"
  "MouseMoveEvent 276 277 0 0 0 0 0\n"
  "MouseMoveEvent 277 278 0 0 0 0 0\n"
  "MouseMoveEvent 277 278 0 0 0 0 0\n"
  "MouseMoveEvent 279 279 0 0 0 0 0\n"
  "MouseMoveEvent 279 279 0 0 0 0 0\n"
  "MouseMoveEvent 279 280 0 0 0 0 0\n"
  "MouseMoveEvent 279 280 0 0 0 0 0\n"
  "MouseMoveEvent 280 281 0 0 0 0 0\n"
  "MouseMoveEvent 280 281 0 0 0 0 0\n"
  "MouseMoveEvent 280 282 0 0 0 0 0\n"
  "MouseMoveEvent 280 282 0 0 0 0 0\n"
  "MouseMoveEvent 281 282 0 0 0 0 0\n"
  "MouseMoveEvent 281 282 0 0 0 0 0\n"
  "MouseMoveEvent 282 282 0 0 0 0 0\n"
  "MouseMoveEvent 282 282 0 0 0 0 0\n"
  "MouseMoveEvent 282 283 0 0 0 0 0\n"
  "MouseMoveEvent 282 283 0 0 0 0 0\n"
  "MouseMoveEvent 282 283 0 0 0 0 0\n"
  "MouseMoveEvent 282 283 0 0 0 0 0\n"
  "MouseMoveEvent 283 284 0 0 0 0 0\n"
  "MouseMoveEvent 283 284 0 0 0 0 0\n"
  "MouseMoveEvent 283 284 0 0 0 0 0\n"
  "MouseMoveEvent 283 284 0 0 0 0 0\n"
  "MouseMoveEvent 283 284 0 0 0 0 0\n"
  "MouseMoveEvent 283 284 0 0 0 0 0\n"
  "MouseMoveEvent 283 284 0 0 0 0 0\n"
  "MouseMoveEvent 283 284 0 0 0 0 0\n"
  "LeftButtonPressEvent 283 284 0 0 0 0 0\n"
  "RenderEvent 283 284 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 283 284 0 0 0 0 0\n"
  "RenderEvent 283 284 0 0 0 0 0\n"
  "MouseMoveEvent 283 283 0 0 0 0 0\n"
  "MouseMoveEvent 283 283 0 0 0 0 0\n"
  "MouseMoveEvent 282 281 0 0 0 0 0\n"
  "MouseMoveEvent 282 281 0 0 0 0 0\n"
  "MouseMoveEvent 280 279 0 0 0 0 0\n"
  "MouseMoveEvent 280 279 0 0 0 0 0\n"
  "MouseMoveEvent 278 276 0 0 0 0 0\n"
  "MouseMoveEvent 278 276 0 0 0 0 0\n"
  "MouseMoveEvent 271 270 0 0 0 0 0\n"
  "RenderEvent 271 270 0 0 0 0 0\n"
  "MouseMoveEvent 271 270 0 0 0 0 0\n"
  "MouseMoveEvent 262 261 0 0 0 0 0\n"
  "MouseMoveEvent 262 261 0 0 0 0 0\n"
  "MouseMoveEvent 252 251 0 0 0 0 0\n"
  "MouseMoveEvent 252 251 0 0 0 0 0\n"
  "MouseMoveEvent 227 230 0 0 0 0 0\n"
  "MouseMoveEvent 227 230 0 0 0 0 0\n"
  "MouseMoveEvent 205 213 0 0 0 0 0\n"
  "MouseMoveEvent 205 213 0 0 0 0 0\n"
  "MouseMoveEvent 194 204 0 0 0 0 0\n"
  "MouseMoveEvent 194 204 0 0 0 0 0\n"
  "MouseMoveEvent 172 187 0 0 0 0 0\n"
  "MouseMoveEvent 172 187 0 0 0 0 0\n"
  "MouseMoveEvent 150 171 0 0 0 0 0\n"
  "MouseMoveEvent 150 171 0 0 0 0 0\n"
  "MouseMoveEvent 129 157 0 0 0 0 0\n"
  "MouseMoveEvent 129 157 0 0 0 0 0\n"
  "MouseMoveEvent 121 152 0 0 0 0 0\n"
  "MouseMoveEvent 121 152 0 0 0 0 0\n"
  "MouseMoveEvent 105 143 0 0 0 0 0\n"
  "MouseMoveEvent 105 143 0 0 0 0 0\n"
  "MouseMoveEvent 100 140 0 0 0 0 0\n"
  "MouseMoveEvent 100 140 0 0 0 0 0\n"
  "MouseMoveEvent 90 136 0 0 0 0 0\n"
  "MouseMoveEvent 90 136 0 0 0 0 0\n"
  "MouseMoveEvent 86 134 0 0 0 0 0\n"
  "MouseMoveEvent 86 134 0 0 0 0 0\n"
  "MouseMoveEvent 80 132 0 0 0 0 0\n"
  "MouseMoveEvent 80 132 0 0 0 0 0\n"
  "MouseMoveEvent 77 131 0 0 0 0 0\n"
  "MouseMoveEvent 77 131 0 0 0 0 0\n"
  "MouseMoveEvent 74 130 0 0 0 0 0\n"
  "MouseMoveEvent 74 130 0 0 0 0 0\n"
  "MouseMoveEvent 72 130 0 0 0 0 0\n"
  "MouseMoveEvent 72 130 0 0 0 0 0\n"
  "MouseMoveEvent 71 129 0 0 0 0 0\n"
  "MouseMoveEvent 71 129 0 0 0 0 0\n"
  "MouseMoveEvent 70 129 0 0 0 0 0\n"
  "MouseMoveEvent 70 129 0 0 0 0 0\n"
  "MouseMoveEvent 70 129 0 0 0 0 0\n"
  "MouseMoveEvent 70 129 0 0 0 0 0\n"
  "MouseMoveEvent 70 129 0 0 0 0 0\n"
  "MouseMoveEvent 70 129 0 0 0 0 0\n"
  "MouseMoveEvent 70 129 0 0 0 0 0\n"
  "MouseMoveEvent 70 129 0 0 0 0 0\n"
  "MouseMoveEvent 69 129 0 0 0 0 0\n"
  "MouseMoveEvent 69 129 0 0 0 0 0\n"
  "MouseMoveEvent 69 129 0 0 0 0 0\n"
  "MouseMoveEvent 69 129 0 0 0 0 0\n"
  "MouseMoveEvent 69 129 0 0 0 0 0\n"
  "MouseMoveEvent 69 129 0 0 0 0 0\n"
  "MouseMoveEvent 69 130 0 0 0 0 0\n"
  "MouseMoveEvent 69 130 0 0 0 0 0\n"
  "MouseMoveEvent 68 131 0 0 0 0 0\n"
  "MouseMoveEvent 68 131 0 0 0 0 0\n"
  "MouseMoveEvent 68 132 0 0 0 0 0\n"
  "MouseMoveEvent 68 132 0 0 0 0 0\n"
  "MouseMoveEvent 68 134 0 0 0 0 0\n"
  "MouseMoveEvent 68 134 0 0 0 0 0\n"
  "MouseMoveEvent 67 136 0 0 0 0 0\n"
  "MouseMoveEvent 67 136 0 0 0 0 0\n"
  "MouseMoveEvent 67 138 0 0 0 0 0\n"
  "MouseMoveEvent 67 138 0 0 0 0 0\n"
  "MouseMoveEvent 66 142 0 0 0 0 0\n"
  "MouseMoveEvent 66 142 0 0 0 0 0\n"
  "MouseMoveEvent 66 146 0 0 0 0 0\n"
  "MouseMoveEvent 66 146 0 0 0 0 0\n"
  "MouseMoveEvent 66 147 0 0 0 0 0\n"
  "MouseMoveEvent 66 147 0 0 0 0 0\n"
  "MouseMoveEvent 65 153 0 0 0 0 0\n"
  "MouseMoveEvent 65 153 0 0 0 0 0\n"
  "MouseMoveEvent 65 155 0 0 0 0 0\n"
  "MouseMoveEvent 65 155 0 0 0 0 0\n"
  "MouseMoveEvent 65 157 0 0 0 0 0\n"
  "MouseMoveEvent 65 157 0 0 0 0 0\n"
  "MouseMoveEvent 65 160 0 0 0 0 0\n"
  "MouseMoveEvent 65 160 0 0 0 0 0\n"
  "MouseMoveEvent 65 162 0 0 0 0 0\n"
  "MouseMoveEvent 65 162 0 0 0 0 0\n"
  "MouseMoveEvent 65 163 0 0 0 0 0\n"
  "MouseMoveEvent 65 163 0 0 0 0 0\n"
  "MouseMoveEvent 65 164 0 0 0 0 0\n"
  "MouseMoveEvent 65 164 0 0 0 0 0\n"
  "MouseMoveEvent 65 164 0 0 0 0 0\n"
  "MouseMoveEvent 65 164 0 0 0 0 0\n"
  "LeftButtonPressEvent 65 164 0 0 0 0 0\n"
  "StartInteractionEvent 65 164 0 0 0 0 0\n"
  "TimerEvent 65 164 0 0 0 0 0\n"
  "RenderEvent 65 164 0 0 0 0 0\n"
  "TimerEvent 65 164 0 0 0 0 0\n"
  "RenderEvent 65 164 0 0 0 0 0\n"
  "MouseMoveEvent 65 157 0 0 0 0 0\n"
  "InteractionEvent 65 157 0 0 0 0 0\n"
  "TimerEvent 65 157 0 0 0 0 0\n"
  "RenderEvent 65 157 0 0 0 0 0\n"
  "MouseMoveEvent 65 134 0 0 0 0 0\n"
  "InteractionEvent 65 134 0 0 0 0 0\n"
  "TimerEvent 65 134 0 0 0 0 0\n"
  "RenderEvent 65 134 0 0 0 0 0\n"
  "MouseMoveEvent 67 118 0 0 0 0 0\n"
  "InteractionEvent 67 118 0 0 0 0 0\n"
  "MouseMoveEvent 68 114 0 0 0 0 0\n"
  "InteractionEvent 68 114 0 0 0 0 0\n"
  "TimerEvent 68 114 0 0 0 0 0\n"
  "RenderEvent 68 114 0 0 0 0 0\n"
  "MouseMoveEvent 72 99 0 0 0 0 0\n"
  "InteractionEvent 72 99 0 0 0 0 0\n"
  "MouseMoveEvent 72 98 0 0 0 0 0\n"
  "InteractionEvent 72 98 0 0 0 0 0\n"
  "TimerEvent 72 98 0 0 0 0 0\n"
  "RenderEvent 72 98 0 0 0 0 0\n"
  "MouseMoveEvent 75 89 0 0 0 0 0\n"
  "InteractionEvent 75 89 0 0 0 0 0\n"
  "MouseMoveEvent 75 87 0 0 0 0 0\n"
  "InteractionEvent 75 87 0 0 0 0 0\n"
  "TimerEvent 75 87 0 0 0 0 0\n"
  "RenderEvent 75 87 0 0 0 0 0\n"
  "MouseMoveEvent 77 83 0 0 0 0 0\n"
  "InteractionEvent 77 83 0 0 0 0 0\n"
  "MouseMoveEvent 77 83 0 0 0 0 0\n"
  "InteractionEvent 77 83 0 0 0 0 0\n"
  "TimerEvent 77 83 0 0 0 0 0\n"
  "RenderEvent 77 83 0 0 0 0 0\n"
  "MouseMoveEvent 79 79 0 0 0 0 0\n"
  "InteractionEvent 79 79 0 0 0 0 0\n"
  "TimerEvent 79 79 0 0 0 0 0\n"
  "RenderEvent 79 79 0 0 0 0 0\n"
  "MouseMoveEvent 80 75 0 0 0 0 0\n"
  "InteractionEvent 80 75 0 0 0 0 0\n"
  "MouseMoveEvent 80 74 0 0 0 0 0\n"
  "InteractionEvent 80 74 0 0 0 0 0\n"
  "TimerEvent 80 74 0 0 0 0 0\n"
  "RenderEvent 80 74 0 0 0 0 0\n"
  "MouseMoveEvent 81 73 0 0 0 0 0\n"
  "InteractionEvent 81 73 0 0 0 0 0\n"
  "MouseMoveEvent 81 73 0 0 0 0 0\n"
  "InteractionEvent 81 73 0 0 0 0 0\n"
  "TimerEvent 81 73 0 0 0 0 0\n"
  "RenderEvent 81 73 0 0 0 0 0\n"
  "MouseMoveEvent 81 72 0 0 0 0 0\n"
  "InteractionEvent 81 72 0 0 0 0 0\n"
  "TimerEvent 81 72 0 0 0 0 0\n"
  "RenderEvent 81 72 0 0 0 0 0\n"
  "MouseMoveEvent 81 70 0 0 0 0 0\n"
  "InteractionEvent 81 70 0 0 0 0 0\n"
  "MouseMoveEvent 81 70 0 0 0 0 0\n"
  "InteractionEvent 81 70 0 0 0 0 0\n"
  "TimerEvent 81 70 0 0 0 0 0\n"
  "RenderEvent 81 70 0 0 0 0 0\n"
  "MouseMoveEvent 82 69 0 0 0 0 0\n"
  "InteractionEvent 82 69 0 0 0 0 0\n"
  "TimerEvent 82 69 0 0 0 0 0\n"
  "RenderEvent 82 69 0 0 0 0 0\n"
  "MouseMoveEvent 82 69 0 0 0 0 0\n"
  "InteractionEvent 82 69 0 0 0 0 0\n"
  "TimerEvent 82 69 0 0 0 0 0\n"
  "RenderEvent 82 69 0 0 0 0 0\n"
  "TimerEvent 82 69 0 0 0 0 0\n"
  "RenderEvent 82 69 0 0 0 0 0\n"
  "MouseMoveEvent 82 70 0 0 0 0 0\n"
  "InteractionEvent 82 70 0 0 0 0 0\n"
  "TimerEvent 82 70 0 0 0 0 0\n"
  "RenderEvent 82 70 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 82 70 0 0 0 0 0\n"
  "EndInteractionEvent 82 70 0 0 0 0 0\n"
  "RenderEvent 82 70 0 0 0 0 0\n"
  "KeyPressEvent 82 70 0 0 116 1 t\n"
  "CharEvent 82 70 0 0 116 1 t\n"
  "KeyReleaseEvent 82 70 0 0 116 1 t\n"
  "MouseMoveEvent 82 71 0 0 0 0 t\n"
  "MouseMoveEvent 82 71 0 0 0 0 t\n"
  "MouseMoveEvent 82 74 0 0 0 0 t\n"
  "MouseMoveEvent 82 74 0 0 0 0 t\n"
  "MouseMoveEvent 82 79 0 0 0 0 t\n"
  "MouseMoveEvent 82 79 0 0 0 0 t\n"
  "MouseMoveEvent 84 85 0 0 0 0 t\n"
  "MouseMoveEvent 84 85 0 0 0 0 t\n"
  "MouseMoveEvent 85 94 0 0 0 0 t\n"
  "MouseMoveEvent 85 94 0 0 0 0 t\n"
  "MouseMoveEvent 88 105 0 0 0 0 t\n"
  "MouseMoveEvent 88 105 0 0 0 0 t\n"
  "MouseMoveEvent 91 115 0 0 0 0 t\n"
  "MouseMoveEvent 91 115 0 0 0 0 t\n"
  "MouseMoveEvent 94 123 0 0 0 0 t\n"
  "MouseMoveEvent 94 123 0 0 0 0 t\n"
  "MouseMoveEvent 95 128 0 0 0 0 t\n"
  "MouseMoveEvent 95 128 0 0 0 0 t\n"
  "MouseMoveEvent 98 135 0 0 0 0 t\n"
  "MouseMoveEvent 98 135 0 0 0 0 t\n"
  "MouseMoveEvent 100 141 0 0 0 0 t\n"
  "MouseMoveEvent 100 141 0 0 0 0 t\n"
  "MouseMoveEvent 102 144 0 0 0 0 t\n"
  "MouseMoveEvent 102 144 0 0 0 0 t\n"
  "MouseMoveEvent 103 147 0 0 0 0 t\n"
  "MouseMoveEvent 103 147 0 0 0 0 t\n"
  "MouseMoveEvent 105 150 0 0 0 0 t\n"
  "MouseMoveEvent 105 150 0 0 0 0 t\n"
  "MouseMoveEvent 105 153 0 0 0 0 t\n"
  "MouseMoveEvent 105 153 0 0 0 0 t\n"
  "MouseMoveEvent 106 154 0 0 0 0 t\n"
  "MouseMoveEvent 106 154 0 0 0 0 t\n"
  "MouseMoveEvent 107 155 0 0 0 0 t\n"
  "MouseMoveEvent 107 155 0 0 0 0 t\n"
  "MouseMoveEvent 107 155 0 0 0 0 t\n"
  "MouseMoveEvent 107 155 0 0 0 0 t\n"
  "MouseMoveEvent 107 156 0 0 0 0 t\n"
  "MouseMoveEvent 107 156 0 0 0 0 t\n"
  "MouseMoveEvent 108 156 0 0 0 0 t\n"
  "MouseMoveEvent 108 156 0 0 0 0 t\n"
  "MouseMoveEvent 108 156 0 0 0 0 t\n"
  "MouseMoveEvent 108 156 0 0 0 0 t\n"
  "MouseMoveEvent 108 157 0 0 0 0 t\n"
  "MouseMoveEvent 108 157 0 0 0 0 t\n"
  "MouseMoveEvent 108 157 0 0 0 0 t\n"
  "MouseMoveEvent 108 157 0 0 0 0 t\n"
  "MouseMoveEvent 108 157 0 0 0 0 t\n"
  "MouseMoveEvent 108 157 0 0 0 0 t\n"
  "MouseMoveEvent 108 157 0 0 0 0 t\n"
  "MouseMoveEvent 108 157 0 0 0 0 t\n"
  "MouseMoveEvent 108 158 0 0 0 0 t\n"
  "MouseMoveEvent 108 158 0 0 0 0 t\n"
  "MouseMoveEvent 107 158 0 0 0 0 t\n"
  "MouseMoveEvent 107 158 0 0 0 0 t\n"
  "MouseMoveEvent 107 159 0 0 0 0 t\n"
  "MouseMoveEvent 107 159 0 0 0 0 t\n"
  "MouseMoveEvent 107 161 0 0 0 0 t\n"
  "MouseMoveEvent 107 161 0 0 0 0 t\n"
  "MouseMoveEvent 107 162 0 0 0 0 t\n"
  "MouseMoveEvent 107 162 0 0 0 0 t\n"
  "MouseMoveEvent 106 163 0 0 0 0 t\n"
  "MouseMoveEvent 106 163 0 0 0 0 t\n"
  "MouseMoveEvent 106 164 0 0 0 0 t\n"
  "MouseMoveEvent 106 164 0 0 0 0 t\n"
  "MouseMoveEvent 106 165 0 0 0 0 t\n"
  "MouseMoveEvent 106 165 0 0 0 0 t\n"
  "MouseMoveEvent 106 166 0 0 0 0 t\n"
  "MouseMoveEvent 106 166 0 0 0 0 t\n"
  "MouseMoveEvent 106 167 0 0 0 0 t\n"
  "MouseMoveEvent 106 167 0 0 0 0 t\n"
  "MouseMoveEvent 106 167 0 0 0 0 t\n"
  "MouseMoveEvent 106 167 0 0 0 0 t\n"
  "MouseMoveEvent 106 168 0 0 0 0 t\n"
  "MouseMoveEvent 106 168 0 0 0 0 t\n"
  "MouseMoveEvent 106 168 0 0 0 0 t\n"
  "MouseMoveEvent 106 168 0 0 0 0 t\n"
  "MouseMoveEvent 106 168 0 0 0 0 t\n"
  "MouseMoveEvent 106 168 0 0 0 0 t\n"
  "MouseMoveEvent 105 168 0 0 0 0 t\n"
  "MouseMoveEvent 105 168 0 0 0 0 t\n"
  "MouseMoveEvent 105 168 0 0 0 0 t\n"
  "MouseMoveEvent 105 168 0 0 0 0 t\n"
  "MouseWheelBackwardEvent 105 168 0 0 0 0 t\n"
  "StartInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "EndInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "MouseWheelBackwardEvent 105 168 0 0 0 0 t\n"
  "StartInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "EndInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "MouseWheelBackwardEvent 105 168 0 0 0 0 t\n"
  "StartInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "EndInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "MouseWheelForwardEvent 105 168 0 0 0 0 t\n"
  "StartInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "EndInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "MouseWheelForwardEvent 105 168 0 0 0 0 t\n"
  "StartInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "EndInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "MouseWheelForwardEvent 105 168 0 0 0 0 t\n"
  "StartInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "EndInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "MouseWheelForwardEvent 105 168 0 0 0 0 t\n"
  "StartInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "EndInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "MouseWheelForwardEvent 105 168 0 0 0 0 t\n"
  "StartInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "EndInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "MouseWheelForwardEvent 105 168 0 0 0 0 t\n"
  "StartInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "EndInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "MouseWheelForwardEvent 105 168 0 0 0 0 t\n"
  "StartInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "EndInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "MouseWheelForwardEvent 105 168 0 0 0 0 t\n"
  "StartInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "EndInteractionEvent 105 168 0 0 0 0 t\n"
  "RenderEvent 105 168 0 0 0 0 t\n"
  "MouseMoveEvent 105 168 0 0 0 0 t\n"
  "MouseMoveEvent 105 168 0 0 0 0 t\n"
  "MouseMoveEvent 104 168 0 0 0 0 t\n"
  "MouseMoveEvent 104 168 0 0 0 0 t\n"
  "MouseMoveEvent 103 168 0 0 0 0 t\n"
  "MouseMoveEvent 103 168 0 0 0 0 t\n"
  "MouseMoveEvent 101 168 0 0 0 0 t\n"
  "MouseMoveEvent 101 168 0 0 0 0 t\n"
  "MouseMoveEvent 100 168 0 0 0 0 t\n"
  "MouseMoveEvent 100 168 0 0 0 0 t\n"
  "MouseMoveEvent 98 170 0 0 0 0 t\n"
  "MouseMoveEvent 98 170 0 0 0 0 t\n"
  "MouseMoveEvent 96 171 0 0 0 0 t\n"
  "MouseMoveEvent 96 171 0 0 0 0 t\n"
  "MouseMoveEvent 95 172 0 0 0 0 t\n"
  "MouseMoveEvent 95 172 0 0 0 0 t\n"
  "MouseMoveEvent 93 173 0 0 0 0 t\n"
  "RenderEvent 93 173 0 0 0 0 t\n"
  "MouseMoveEvent 93 173 0 0 0 0 t\n"
  "MouseMoveEvent 90 175 0 0 0 0 t\n"
  "MouseMoveEvent 90 175 0 0 0 0 t\n"
  "MouseMoveEvent 89 177 0 0 0 0 t\n"
  "MouseMoveEvent 89 177 0 0 0 0 t\n"
  "MouseMoveEvent 88 178 0 0 0 0 t\n"
  "MouseMoveEvent 88 178 0 0 0 0 t\n"
  "MouseMoveEvent 87 179 0 0 0 0 t\n"
  "MouseMoveEvent 87 179 0 0 0 0 t\n"
  "MouseMoveEvent 86 179 0 0 0 0 t\n"
  "MouseMoveEvent 86 179 0 0 0 0 t\n"
  "MouseMoveEvent 85 180 0 0 0 0 t\n"
  "MouseMoveEvent 85 180 0 0 0 0 t\n"
  "MouseMoveEvent 84 180 0 0 0 0 t\n"
  "MouseMoveEvent 84 180 0 0 0 0 t\n"
  "MouseMoveEvent 84 181 0 0 0 0 t\n"
  "MouseMoveEvent 84 181 0 0 0 0 t\n"
  "MouseMoveEvent 83 181 0 0 0 0 t\n"
  "MouseMoveEvent 83 181 0 0 0 0 t\n"
  "MouseMoveEvent 83 182 0 0 0 0 t\n"
  "MouseMoveEvent 83 182 0 0 0 0 t\n"
  "MouseMoveEvent 83 182 0 0 0 0 t\n"
  "MouseMoveEvent 83 182 0 0 0 0 t\n"
  "MouseMoveEvent 82 182 0 0 0 0 t\n"
  "MouseMoveEvent 82 182 0 0 0 0 t\n"
  "MouseMoveEvent 82 183 0 0 0 0 t\n"
  "MouseMoveEvent 82 183 0 0 0 0 t\n"
  "MouseMoveEvent 82 183 0 0 0 0 t\n"
  "MouseMoveEvent 82 183 0 0 0 0 t\n"
  "MouseMoveEvent 82 183 0 0 0 0 t\n"
  "MouseMoveEvent 82 183 0 0 0 0 t\n"
  "MouseMoveEvent 82 183 0 0 0 0 t\n"
  "MouseMoveEvent 82 183 0 0 0 0 t\n"
  "MouseMoveEvent 82 183 0 0 0 0 t\n"
  "MouseMoveEvent 82 183 0 0 0 0 t\n"
  "MouseMoveEvent 82 184 0 0 0 0 t\n"
  "MouseMoveEvent 82 184 0 0 0 0 t\n"
  "LeftButtonPressEvent 82 184 0 0 0 0 t\n"
  "RenderEvent 82 184 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 82 184 0 0 0 0 t\n"
  "RenderEvent 82 184 0 0 0 0 t\n"
  "MouseMoveEvent 83 184 0 0 0 0 t\n"
  "MouseMoveEvent 83 184 0 0 0 0 t\n"
  "MouseMoveEvent 84 184 0 0 0 0 t\n"
  "MouseMoveEvent 84 184 0 0 0 0 t\n"
  "MouseMoveEvent 86 184 0 0 0 0 t\n"
  "MouseMoveEvent 86 184 0 0 0 0 t\n"
  "MouseMoveEvent 89 184 0 0 0 0 t\n"
  "RenderEvent 89 184 0 0 0 0 t\n"
  "MouseMoveEvent 89 184 0 0 0 0 t\n"
  "MouseMoveEvent 97 183 0 0 0 0 t\n"
  "MouseMoveEvent 97 183 0 0 0 0 t\n"
  "MouseMoveEvent 102 182 0 0 0 0 t\n"
  "MouseMoveEvent 102 182 0 0 0 0 t\n"
  "MouseMoveEvent 105 180 0 0 0 0 t\n"
  "MouseMoveEvent 105 180 0 0 0 0 t\n"
  "MouseMoveEvent 109 179 0 0 0 0 t\n"
  "MouseMoveEvent 109 179 0 0 0 0 t\n"
  "MouseMoveEvent 112 178 0 0 0 0 t\n"
  "MouseMoveEvent 112 178 0 0 0 0 t\n"
  "MouseMoveEvent 113 178 0 0 0 0 t\n"
  "MouseMoveEvent 113 178 0 0 0 0 t\n"
  "MouseMoveEvent 116 176 0 0 0 0 t\n"
  "MouseMoveEvent 116 176 0 0 0 0 t\n"
  "MouseMoveEvent 117 175 0 0 0 0 t\n"
  "MouseMoveEvent 117 175 0 0 0 0 t\n"
  "MouseMoveEvent 119 174 0 0 0 0 t\n"
  "MouseMoveEvent 119 174 0 0 0 0 t\n"
  "MouseMoveEvent 120 172 0 0 0 0 t\n"
  "MouseMoveEvent 120 172 0 0 0 0 t\n"
  "MouseMoveEvent 121 170 0 0 0 0 t\n"
  "MouseMoveEvent 121 170 0 0 0 0 t\n"
  "MouseMoveEvent 121 169 0 0 0 0 t\n"
  "MouseMoveEvent 121 169 0 0 0 0 t\n"
  "MouseMoveEvent 121 166 0 0 0 0 t\n"
  "MouseMoveEvent 121 166 0 0 0 0 t\n"
  "MouseMoveEvent 121 165 0 0 0 0 t\n"
  "MouseMoveEvent 121 165 0 0 0 0 t\n"
  "MouseMoveEvent 121 164 0 0 0 0 t\n"
  "MouseMoveEvent 121 164 0 0 0 0 t\n"
  "MouseMoveEvent 121 162 0 0 0 0 t\n"
  "MouseMoveEvent 121 162 0 0 0 0 t\n"
  "MouseMoveEvent 121 162 0 0 0 0 t\n"
  "MouseMoveEvent 121 162 0 0 0 0 t\n"
  "MouseMoveEvent 121 161 0 0 0 0 t\n"
  "MouseMoveEvent 121 161 0 0 0 0 t\n"
  "MouseMoveEvent 121 160 0 0 0 0 t\n"
  "MouseMoveEvent 121 160 0 0 0 0 t\n"
  "MouseMoveEvent 120 160 0 0 0 0 t\n"
  "MouseMoveEvent 120 160 0 0 0 0 t\n"
  "MouseMoveEvent 120 159 0 0 0 0 t\n"
  "MouseMoveEvent 120 159 0 0 0 0 t\n"
  "MouseMoveEvent 119 159 0 0 0 0 t\n"
  "MouseMoveEvent 119 159 0 0 0 0 t\n"
  "MouseMoveEvent 119 159 0 0 0 0 t\n"
  "MouseMoveEvent 119 159 0 0 0 0 t\n"
  "MouseMoveEvent 118 159 0 0 0 0 t\n"
  "MouseMoveEvent 118 159 0 0 0 0 t\n"
  "MouseMoveEvent 118 159 0 0 0 0 t\n"
  "MouseMoveEvent 118 159 0 0 0 0 t\n"
  "MouseMoveEvent 117 160 0 0 0 0 t\n"
  "MouseMoveEvent 117 160 0 0 0 0 t\n"
  "MouseMoveEvent 117 161 0 0 0 0 t\n"
  "MouseMoveEvent 117 161 0 0 0 0 t\n"
  "MouseMoveEvent 116 161 0 0 0 0 t\n"
  "MouseMoveEvent 116 161 0 0 0 0 t\n"
  "MouseMoveEvent 116 162 0 0 0 0 t\n"
  "MouseMoveEvent 116 162 0 0 0 0 t\n"
  "MouseMoveEvent 115 162 0 0 0 0 t\n"
  "MouseMoveEvent 115 162 0 0 0 0 t\n"
  "MouseMoveEvent 115 163 0 0 0 0 t\n"
  "MouseMoveEvent 115 163 0 0 0 0 t\n"
  "LeftButtonPressEvent 115 163 0 0 0 0 t\n"
  "StartInteractionEvent 115 163 0 0 0 0 t\n"
  "MouseMoveEvent 115 163 0 0 0 0 t\n"
  "RenderEvent 115 163 0 0 0 0 t\n"
  "InteractionEvent 115 163 0 0 0 0 t\n"
  "MouseMoveEvent 109 168 0 0 0 0 t\n"
  "RenderEvent 109 168 0 0 0 0 t\n"
  "InteractionEvent 109 168 0 0 0 0 t\n"
  "MouseMoveEvent 101 179 0 0 0 0 t\n"
  "RenderEvent 101 179 0 0 0 0 t\n"
  "InteractionEvent 101 179 0 0 0 0 t\n"
  "MouseMoveEvent 95 188 0 0 0 0 t\n"
  "RenderEvent 95 188 0 0 0 0 t\n"
  "InteractionEvent 95 188 0 0 0 0 t\n"
  "MouseMoveEvent 93 191 0 0 0 0 t\n"
  "RenderEvent 93 191 0 0 0 0 t\n"
  "InteractionEvent 93 191 0 0 0 0 t\n"
  "MouseMoveEvent 91 193 0 0 0 0 t\n"
  "RenderEvent 91 193 0 0 0 0 t\n"
  "InteractionEvent 91 193 0 0 0 0 t\n"
  "MouseMoveEvent 88 196 0 0 0 0 t\n"
  "RenderEvent 88 196 0 0 0 0 t\n"
  "InteractionEvent 88 196 0 0 0 0 t\n"
  "MouseMoveEvent 83 198 0 0 0 0 t\n"
  "RenderEvent 83 198 0 0 0 0 t\n"
  "InteractionEvent 83 198 0 0 0 0 t\n"
  "MouseMoveEvent 78 203 0 0 0 0 t\n"
  "RenderEvent 78 203 0 0 0 0 t\n"
  "InteractionEvent 78 203 0 0 0 0 t\n"
  "MouseMoveEvent 73 212 0 0 0 0 t\n"
  "RenderEvent 73 212 0 0 0 0 t\n"
  "InteractionEvent 73 212 0 0 0 0 t\n"
  "MouseMoveEvent 69 218 0 0 0 0 t\n"
  "RenderEvent 69 218 0 0 0 0 t\n"
  "InteractionEvent 69 218 0 0 0 0 t\n"
  "MouseMoveEvent 64 224 0 0 0 0 t\n"
  "RenderEvent 64 224 0 0 0 0 t\n"
  "InteractionEvent 64 224 0 0 0 0 t\n"
  "MouseMoveEvent 62 227 0 0 0 0 t\n"
  "RenderEvent 62 227 0 0 0 0 t\n"
  "InteractionEvent 62 227 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 62 227 0 0 0 0 t\n"
  "EndInteractionEvent 62 227 0 0 0 0 t\n"
  "RenderEvent 62 227 0 0 0 0 t\n"
  "MouseMoveEvent 63 227 0 0 0 0 t\n"
  "MouseMoveEvent 63 227 0 0 0 0 t\n"
  "MouseMoveEvent 64 226 0 0 0 0 t\n"
  "MouseMoveEvent 64 226 0 0 0 0 t\n"
  "MouseMoveEvent 65 225 0 0 0 0 t\n"
  "MouseMoveEvent 65 225 0 0 0 0 t\n"
  "MouseMoveEvent 66 223 0 0 0 0 t\n"
  "MouseMoveEvent 66 223 0 0 0 0 t\n"
  "MouseMoveEvent 67 222 0 0 0 0 t\n"
  "MouseMoveEvent 67 222 0 0 0 0 t\n"
  "MouseMoveEvent 69 220 0 0 0 0 t\n"
  "MouseMoveEvent 69 220 0 0 0 0 t\n"
  "MouseMoveEvent 70 218 0 0 0 0 t\n"
  "MouseMoveEvent 70 218 0 0 0 0 t\n"
  "MouseMoveEvent 72 215 0 0 0 0 t\n"
  "MouseMoveEvent 72 215 0 0 0 0 t\n"
  "MouseMoveEvent 74 213 0 0 0 0 t\n"
  "MouseMoveEvent 74 213 0 0 0 0 t\n"
  "MouseMoveEvent 75 210 0 0 0 0 t\n"
  "MouseMoveEvent 75 210 0 0 0 0 t\n"
  "MouseMoveEvent 77 208 0 0 0 0 t\n"
  "MouseMoveEvent 77 208 0 0 0 0 t\n"
  "MouseMoveEvent 78 207 0 0 0 0 t\n"
  "MouseMoveEvent 78 207 0 0 0 0 t\n"
  "MouseMoveEvent 79 205 0 0 0 0 t\n"
  "MouseMoveEvent 79 205 0 0 0 0 t\n"
  "MouseMoveEvent 81 203 0 0 0 0 t\n"
  "MouseMoveEvent 81 203 0 0 0 0 t\n"
  "MouseMoveEvent 82 202 0 0 0 0 t\n"
  "MouseMoveEvent 82 202 0 0 0 0 t\n"
  "MouseMoveEvent 83 201 0 0 0 0 t\n"
  "MouseMoveEvent 83 201 0 0 0 0 t\n"
  "MouseMoveEvent 83 201 0 0 0 0 t\n"
  "MouseMoveEvent 83 201 0 0 0 0 t\n"
  "MouseMoveEvent 84 200 0 0 0 0 t\n"
  "MouseMoveEvent 84 200 0 0 0 0 t\n"
  "MouseMoveEvent 85 199 0 0 0 0 t\n"
  "MouseMoveEvent 85 199 0 0 0 0 t\n"
  "MouseMoveEvent 85 199 0 0 0 0 t\n"
  "MouseMoveEvent 85 199 0 0 0 0 t\n"
  "MouseMoveEvent 86 199 0 0 0 0 t\n"
  "MouseMoveEvent 86 199 0 0 0 0 t\n"
  "MouseMoveEvent 86 199 0 0 0 0 t\n"
  "MouseMoveEvent 86 199 0 0 0 0 t\n"
  "MouseMoveEvent 86 198 0 0 0 0 t\n"
  "RenderEvent 86 198 0 0 0 0 t\n"
  "MouseMoveEvent 86 198 0 0 0 0 t\n"
  "MouseMoveEvent 87 198 0 0 0 0 t\n"
  "MouseMoveEvent 87 198 0 0 0 0 t\n"
  "MouseMoveEvent 87 198 0 0 0 0 t\n"
  "MouseMoveEvent 87 198 0 0 0 0 t\n"
  "MouseMoveEvent 87 198 0 0 0 0 t\n"
  "MouseMoveEvent 87 198 0 0 0 0 t\n"
  "MouseMoveEvent 87 198 0 0 0 0 t\n"
  "MouseMoveEvent 87 198 0 0 0 0 t\n"
  "MouseMoveEvent 87 197 0 0 0 0 t\n"
  "MouseMoveEvent 87 197 0 0 0 0 t\n"
  "MouseMoveEvent 87 197 0 0 0 0 t\n"
  "MouseMoveEvent 87 197 0 0 0 0 t\n"
  "MouseMoveEvent 87 197 0 0 0 0 t\n"
  "MouseMoveEvent 87 197 0 0 0 0 t\n"
  "MouseMoveEvent 87 197 0 0 0 0 t\n"
  "MouseMoveEvent 87 197 0 0 0 0 t\n"
  "MouseMoveEvent 87 196 0 0 0 0 t\n"
  "MouseMoveEvent 87 196 0 0 0 0 t\n"
  "MouseMoveEvent 87 196 0 0 0 0 t\n"
  "MouseMoveEvent 87 196 0 0 0 0 t\n"
  "MouseMoveEvent 87 195 0 0 0 0 t\n"
  "MouseMoveEvent 87 195 0 0 0 0 t\n"
  "MouseMoveEvent 87 195 0 0 0 0 t\n"
  "MouseMoveEvent 87 195 0 0 0 0 t\n"
  "MouseMoveEvent 87 195 0 0 0 0 t\n"
  "MouseMoveEvent 87 195 0 0 0 0 t\n"
  "MouseMoveEvent 87 194 0 0 0 0 t\n"
  "MouseMoveEvent 87 194 0 0 0 0 t\n"
  "MouseMoveEvent 88 194 0 0 0 0 t\n"
  "MouseMoveEvent 88 194 0 0 0 0 t\n"
  "MouseMoveEvent 88 193 0 0 0 0 t\n"
  "MouseMoveEvent 88 193 0 0 0 0 t\n"
  "MouseMoveEvent 88 193 0 0 0 0 t\n"
  "MouseMoveEvent 88 193 0 0 0 0 t\n"
  "MouseMoveEvent 88 192 0 0 0 0 t\n"
  "MouseMoveEvent 88 192 0 0 0 0 t\n"
  "MouseMoveEvent 88 192 0 0 0 0 t\n"
  "MouseMoveEvent 88 192 0 0 0 0 t\n"
  "MouseMoveEvent 88 191 0 0 0 0 t\n"
  "MouseMoveEvent 88 191 0 0 0 0 t\n"
  "LeftButtonPressEvent 88 191 0 0 0 0 t\n"
  "RenderEvent 88 191 0 0 0 0 t\n"
  "LeftButtonReleaseEvent 88 191 0 0 0 0 t\n"
  "RenderEvent 88 191 0 0 0 0 t\n"
  "MouseMoveEvent 88 191 0 0 0 0 t\n"
  "MouseMoveEvent 88 191 0 0 0 0 t\n"
  "MouseMoveEvent 88 191 0 0 0 0 t\n"
  "MouseMoveEvent 88 191 0 0 0 0 t\n"
  "MouseMoveEvent 88 191 0 0 0 0 t\n"
  "MouseMoveEvent 88 191 0 0 0 0 t\n"
  "MouseMoveEvent 88 192 0 0 0 0 t\n"
  "MouseMoveEvent 88 192 0 0 0 0 t\n"
  "MouseMoveEvent 88 192 0 0 0 0 t\n"
  "MouseMoveEvent 88 192 0 0 0 0 t\n"
  "MouseMoveEvent 88 192 0 0 0 0 t\n"
  "MouseMoveEvent 88 192 0 0 0 0 t\n"
  "MouseMoveEvent 88 193 0 0 0 0 t\n"
  "MouseMoveEvent 88 193 0 0 0 0 t\n"
  "MouseMoveEvent 88 193 0 0 0 0 t\n"
  "MouseMoveEvent 88 193 0 0 0 0 t\n"
  "MouseMoveEvent 88 194 0 0 0 0 t\n"
  "MouseMoveEvent 88 194 0 0 0 0 t\n"
  "MouseMoveEvent 88 194 0 0 0 0 t\n"
  "MouseMoveEvent 88 194 0 0 0 0 t\n"
  "MouseMoveEvent 88 195 0 0 0 0 t\n"
  "MouseMoveEvent 88 195 0 0 0 0 t\n"
  "MouseMoveEvent 88 196 0 0 0 0 t\n"
  "MouseMoveEvent 88 196 0 0 0 0 t\n"
  "MouseMoveEvent 88 196 0 0 0 0 t\n"
  "MouseMoveEvent 88 196 0 0 0 0 t\n"
  "MouseMoveEvent 88 196 0 0 0 0 t\n"
  "MouseMoveEvent 88 196 0 0 0 0 t\n"
  "MouseMoveEvent 88 197 0 0 0 0 t\n"
  "MouseMoveEvent 88 197 0 0 0 0 t\n"
  "MouseMoveEvent 88 197 0 0 0 0 t\n"
  "MouseMoveEvent 88 197 0 0 0 0 t\n"
  "MouseMoveEvent 88 198 0 0 0 0 t\n"
  "MouseMoveEvent 88 198 0 0 0 0 t\n"
  "MouseMoveEvent 88 198 0 0 0 0 t\n"
  "MouseMoveEvent 88 198 0 0 0 0 t\n"
  "MouseMoveEvent 88 198 0 0 0 0 t\n"
  "MouseMoveEvent 88 198 0 0 0 0 t\n"
  "MouseMoveEvent 89 199 0 0 0 0 t\n"
  "RenderEvent 89 199 0 0 0 0 t\n"
  "MouseMoveEvent 89 199 0 0 0 0 t\n"
  "MouseMoveEvent 89 199 0 0 0 0 t\n"
  "MouseMoveEvent 89 199 0 0 0 0 t\n"
  "MouseMoveEvent 89 199 0 0 0 0 t\n"
  "MouseMoveEvent 89 199 0 0 0 0 t\n"
  "MouseMoveEvent 89 200 0 0 0 0 t\n"
  "MouseMoveEvent 89 200 0 0 0 0 t\n"
  "MouseMoveEvent 90 200 0 0 0 0 t\n"
  "MouseMoveEvent 90 200 0 0 0 0 t\n"
  "MouseMoveEvent 90 201 0 0 0 0 t\n"
  "MouseMoveEvent 90 201 0 0 0 0 t\n"
  "MouseMoveEvent 90 201 0 0 0 0 t\n"
  "MouseMoveEvent 90 201 0 0 0 0 t\n"
  "MouseMoveEvent 91 202 0 0 0 0 t\n"
  "MouseMoveEvent 91 202 0 0 0 0 t\n"
  "MouseMoveEvent 91 202 0 0 0 0 t\n"
  "MouseMoveEvent 91 202 0 0 0 0 t\n"
  "MouseMoveEvent 91 203 0 0 0 0 t\n"
  "MouseMoveEvent 91 203 0 0 0 0 t\n"
  "MouseMoveEvent 91 203 0 0 0 0 t\n"
  "MouseMoveEvent 91 203 0 0 0 0 t\n"
  "MouseMoveEvent 92 203 0 0 0 0 t\n"
  "MouseMoveEvent 92 203 0 0 0 0 t\n"
  "MouseMoveEvent 92 203 0 0 0 0 t\n"
  "MouseMoveEvent 92 203 0 0 0 0 t\n"
  "MouseMoveEvent 92 204 0 0 0 0 t\n"
  "MouseMoveEvent 92 204 0 0 0 0 t\n";

// This does the actual work: updates the scale of the glyphs
// Callback for the interaction
class vtkButtonWidgetMultipleViewportsCallback : public vtkCommand
{
public:
  static vtkButtonWidgetMultipleViewportsCallback* New()
  {
    return new vtkButtonWidgetMultipleViewportsCallback;
  }
  virtual void Execute(vtkObject* caller, unsigned long, void*)
  {
    vtkButtonWidget* buttonWidget = reinterpret_cast<vtkButtonWidget*>(caller);
    vtkTexturedButtonRepresentation* rep =
      reinterpret_cast<vtkTexturedButtonRepresentation*>(
        buttonWidget->GetRepresentation());
    int state = rep->GetState();
    cout << "State: " << state << "\n";
    this->Glyph->SetScaleFactor(0.05 * (1 + state));
  }
  vtkButtonWidgetMultipleViewportsCallback()
    : Glyph(0)
  {
  }
  vtkGlyph3D* Glyph;
};

int TestButtonWidgetMultipleViewports(int argc, char* argv[])
{
  // Create an image for the button
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.tif");
  vtkNew<vtkTIFFReader> image1;
  image1->SetFileName(fname);
  image1->SetOrientationType(4);
  image1->Update();
  delete[] fname;

  // Create an image for the button
  char* fname2 =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/fran_cut.png");
  vtkNew<vtkPNGReader> image2;
  image2->SetFileName(fname2);
  image2->Update();
  delete[] fname2;

  // Create a mace out of filters.
  //
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkConeSource> cone;
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);
  glyph->Update();

  // Appending just makes things simpler to manage.
  vtkNew<vtkAppendPolyData> apd;
  apd->AddInputConnection(glyph->GetOutputPort());
  apd->AddInputConnection(sphere->GetOutputPort());

  vtkNew<vtkPolyDataMapper> maceMapper;
  maceMapper->SetInputConnection(apd->GetOutputPort());

  vtkNew<vtkActor> maceActor;
  maceActor->SetMapper(maceMapper.GetPointer());
  maceActor->VisibilityOn();

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkNew<vtkRenderer> ren1;
  ren1->SetViewport(0, 0, 0.5, 1.0);
  vtkNew<vtkRenderer> ren2;
  ren2->SetViewport(0.5, 0, 1.0, 1.0);
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1.GetPointer());
  renWin->AddRenderer(ren2.GetPointer());
  // set the background and size
  ren1->SetBackground(0., 0.2, 0.4);
  ren2->SetBackground(0.9, 0.8, 0.6);
  renWin->SetSize(300, 300);
  renWin->Render();

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  vtkNew<vtkButtonWidgetMultipleViewportsCallback> myCallback;
  myCallback->Glyph = glyph.GetPointer();

  vtkNew<vtkEllipticalButtonSource> button;
  button->TwoSidedOn();
  button->SetCircumferentialResolution(24);
  button->SetShoulderResolution(24);
  button->SetTextureResolution(24);

  vtkNew<vtkTexturedButtonRepresentation> rep;
  rep->SetNumberOfStates(2);
  rep->SetButtonTexture(0, image1->GetOutput());
  rep->SetButtonTexture(1, image2->GetOutput());
  rep->SetButtonGeometryConnection(button->GetOutputPort());
  rep->SetPlaceFactor(1);
  double bds[6];
  bds[0] = 0.6;
  bds[1] = 0.75;
  bds[2] = 0.6;
  bds[3] = 0.75;
  bds[4] = 0.6;
  bds[5] = 0.75;
  rep->PlaceWidget(bds);
  rep->FollowCameraOn();

  vtkNew<vtkButtonWidget> buttonWidget;
  buttonWidget->SetInteractor(iren.GetPointer());
  buttonWidget->SetRepresentation(rep.GetPointer());
  buttonWidget->AddObserver(
    vtkCommand::StateChangedEvent, myCallback.GetPointer());

  // Another 3D button widget, this time use alternative PlaceWidget() method
  vtkNew<vtkEllipticalButtonSource> button2;
  button2->TwoSidedOn();
  button2->SetCircumferentialResolution(24);
  button2->SetShoulderResolution(24);
  button2->SetTextureResolution(24);
  button2->SetWidth(0.65);
  button2->SetHeight(0.45);
  button2->SetTextureStyleToFitImage();

  vtkNew<vtkTexturedButtonRepresentation> rep2;
  rep2->SetNumberOfStates(2);
  rep2->SetButtonTexture(0, image1->GetOutput());
  rep2->SetButtonTexture(1, image2->GetOutput());
  rep2->SetButtonGeometryConnection(button2->GetOutputPort());
  rep2->SetPlaceFactor(1);
  bds[0] = 0.0;
  bds[1] = 0.0;
  bds[2] = 0.65;
  bds[3] = 0.0;
  bds[4] = 0.0;
  bds[5] = 1;
  rep2->PlaceWidget(0.5, bds, bds + 3);
  rep2->FollowCameraOff();

  vtkNew<vtkButtonWidget> buttonWidget2;
  buttonWidget2->SetInteractor(iren.GetPointer());
  buttonWidget2->SetRepresentation(rep2.GetPointer());
  buttonWidget2->AddObserver(
    vtkCommand::StateChangedEvent, myCallback.GetPointer());

  // Okay now for the 2D version of the widget (in display space)
  vtkNew<vtkTexturedButtonRepresentation2D> rep3;
  rep3->SetNumberOfStates(2);
  rep3->SetButtonTexture(0, image1->GetOutput());
  rep3->SetButtonTexture(1, image2->GetOutput());
  rep3->SetPlaceFactor(1);
  bds[0] = 25;
  bds[1] = 65;
  bds[2] = 50;
  bds[3] = 200;
  rep3->PlaceWidget(bds);

  vtkNew<vtkButtonWidget> buttonWidget3;
  buttonWidget3->SetInteractor(iren.GetPointer());
  buttonWidget3->SetRepresentation(rep3.GetPointer());
  buttonWidget3->SetCurrentRenderer(ren2.GetPointer());
  buttonWidget3->AddObserver(
    vtkCommand::StateChangedEvent, myCallback.GetPointer());

  // Okay now for the 2D version of the widget (in world space)
  vtkNew<vtkTexturedButtonRepresentation2D> rep4;
  rep4->SetNumberOfStates(2);
  rep4->SetButtonTexture(0, image1->GetOutput());
  rep4->SetButtonTexture(1, image2->GetOutput());
  rep4->SetPlaceFactor(1);
  bds[0] = 0.75;
  bds[1] = 0;
  bds[2] = 0;
  int size[2];
  size[0] = 25;
  size[1] = 45;
  rep4->PlaceWidget(bds, size);

  vtkNew<vtkButtonWidget> buttonWidget4;
  buttonWidget4->SetInteractor(iren.GetPointer());
  buttonWidget4->SetRepresentation(rep4.GetPointer());
  buttonWidget4->SetDefaultRenderer(ren2.GetPointer());
  buttonWidget4->AddObserver(
    vtkCommand::StateChangedEvent, myCallback.GetPointer());

  // Finally now a set of vtkProp3D's to define a vtkProp3DButtonRepresentation
  vtkNew<vtkLookupTable> lut;
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

  vtkNew<vtkPlatonicSolidSource> tet;
  tet->SetSolidTypeToTetrahedron();
  vtkNew<vtkPolyDataMapper> tetMapper;
  tetMapper->SetInputConnection(tet->GetOutputPort());
  tetMapper->SetLookupTable(lut.GetPointer());
  tetMapper->SetScalarRange(0, 19);
  vtkNew<vtkActor> tetActor;
  tetActor->SetMapper(tetMapper.GetPointer());

  vtkNew<vtkPlatonicSolidSource> cube;
  cube->SetSolidTypeToCube();
  vtkNew<vtkPolyDataMapper> cubeMapper;
  cubeMapper->SetInputConnection(cube->GetOutputPort());
  cubeMapper->SetLookupTable(lut.GetPointer());
  cubeMapper->SetScalarRange(0, 19);
  vtkNew<vtkActor> cubeActor;
  cubeActor->SetMapper(cubeMapper.GetPointer());

  vtkNew<vtkPlatonicSolidSource> oct;
  oct->SetSolidTypeToOctahedron();
  vtkNew<vtkPolyDataMapper> octMapper;
  octMapper->SetInputConnection(oct->GetOutputPort());
  octMapper->SetLookupTable(lut.GetPointer());
  octMapper->SetScalarRange(0, 19);
  vtkNew<vtkActor> octActor;
  octActor->SetMapper(octMapper.GetPointer());

  vtkNew<vtkPlatonicSolidSource> ico;
  ico->SetSolidTypeToIcosahedron();
  vtkNew<vtkPolyDataMapper> icoMapper;
  icoMapper->SetInputConnection(ico->GetOutputPort());
  icoMapper->SetLookupTable(lut.GetPointer());
  icoMapper->SetScalarRange(0, 19);
  vtkNew<vtkActor> icoActor;
  icoActor->SetMapper(icoMapper.GetPointer());

  vtkNew<vtkPlatonicSolidSource> dode;
  dode->SetSolidTypeToDodecahedron();
  vtkNew<vtkPolyDataMapper> dodeMapper;
  dodeMapper->SetInputConnection(dode->GetOutputPort());
  dodeMapper->SetLookupTable(lut.GetPointer());
  dodeMapper->SetScalarRange(0, 19);
  vtkNew<vtkActor> dodeActor;
  dodeActor->SetMapper(dodeMapper.GetPointer());

  vtkNew<vtkProp3DButtonRepresentation> rep5;
  rep5->SetNumberOfStates(5);
  rep5->SetButtonProp(0, tetActor.GetPointer());
  rep5->SetButtonProp(1, cubeActor.GetPointer());
  rep5->SetButtonProp(2, octActor.GetPointer());
  rep5->SetButtonProp(3, icoActor.GetPointer());
  rep5->SetButtonProp(4, dodeActor.GetPointer());
  rep5->SetPlaceFactor(1);
  bds[0] = 0.65;
  bds[1] = 0.75;
  bds[2] = -0.75;
  bds[3] = -0.65;
  bds[4] = 0.65;
  bds[5] = 0.75;
  rep5->PlaceWidget(bds);
  rep5->FollowCameraOn();

  vtkNew<vtkButtonWidget> buttonWidget5;
  buttonWidget5->SetInteractor(iren.GetPointer());
  buttonWidget5->SetRepresentation(rep5.GetPointer());
  buttonWidget5->SetDefaultRenderer(ren2.GetPointer());
  buttonWidget5->AddObserver(
    vtkCommand::StateChangedEvent, myCallback.GetPointer());

  ren1->AddActor(maceActor.GetPointer());

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  ren2->SetBackground(0.9, 0.8, 0.6);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  buttonWidget->EnabledOn();
  buttonWidget2->EnabledOn();
  buttonWidget3->EnabledOn();
  buttonWidget4->EnabledOn();
  buttonWidget5->EnabledOn();

  ren1->ResetCamera();
  ren2->ResetCamera();

  return vtkTesting::InteractorEventLoop(
    argc, argv, iren.GetPointer(), ButtonWidgetMultipleViewportsEventLog);
}
