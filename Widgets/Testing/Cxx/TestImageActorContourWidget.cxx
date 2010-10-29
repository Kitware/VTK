/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageActorContourWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests the class vtkImageActorPointPlacer. 
//
// A feature central to most widgets is the translation of 2D coordinates to 3D
// coordinates. As you delve down into the inner workings of a widget, you will
// realize that it is critical to define how 2D coordinates are mapped to 3D
// world coordinates.
// 
// Note that interactions happen in a 2D window. ie. The XWindow server is going to 
// tell us that the user pressed his mouse button at pixel location (X,Y). 
// 
//  This location (X,Y) could map to any location in 3D space (x,y,z). How do
// we define this mapping ? VTK provides a few standard ways to define this 
// mapping and a framework to create your own mapping. This is done via
// vtkPointPlacer.
// 
//   * vtkFocalPlanePointPlacer: 
//      
//       Converts 2D display positions to world positions such that they lie on
//       the focal plane.
// 
//   * vtkPolygonalSurfacePointPlacer:
//  
//       Converts 2D display positions to world positions such that they lie on
//       the surface of one or more specified polygonal objects.
// 
//   * vtkImageActorPointPlacer
//  
//       Converts 2D display positions to world positions such that they lie on 
//       a specified ImageActor
// 
//   * vtkBoundedPlanePointPlacer
//      
//       Converts 2D display positions to world positions such that they lie 
//       within a set of specified bounding planes.
// 
//   * vtkTerrainContourPointPlacer - On a terrain..
// 
// 
// Point placers provide an extensible framework to specify constraints on the 
// placement of widgets.

#include "vtkSmartPointer.h"

#include "vtkImageData.h"
#include "vtkVolume16Reader.h"
#include "vtkImageShiftScale.h"
#include "vtkImageViewer2.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSliderRepresentation.h"
#include "vtkSliderRepresentation2D.h"
#include "vtkSliderWidget.h"
#include "vtkCommand.h"
#include "vtkContourWidget.h"
#include "vtkImageActorPointPlacer.h"
#include "vtkOrientedGlyphContourRepresentation.h"
#include "vtkCamera.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkRenderWindow.h"
#include "vtkProperty.h"

char TestImageActorContourWidgetLog[] =
"# StreamVersion 1\n"
"ConfigureEvent 0 499 0 0 0 0 0\n"
"RenderEvent 0 499 0 0 0 0 0\n"
"ExposeEvent 0 499 0 0 0 0 0\n"
"RenderEvent 0 499 0 0 0 0 0\n"
"EnterEvent 254 6 0 0 0 0 0\n"
"MouseMoveEvent 252 12 0 0 0 0 0\n"
"MouseMoveEvent 248 16 0 0 0 0 0\n"
"MouseMoveEvent 247 18 0 0 0 0 0\n"
"MouseMoveEvent 247 19 0 0 0 0 0\n"
"MouseMoveEvent 247 20 0 0 0 0 0\n"
"MouseMoveEvent 247 21 0 0 0 0 0\n"
"MouseMoveEvent 247 22 0 0 0 0 0\n"
"MouseMoveEvent 247 23 0 0 0 0 0\n"
"MouseMoveEvent 248 23 0 0 0 0 0\n"
"MouseMoveEvent 249 23 0 0 0 0 0\n"
"LeftButtonPressEvent 249 23 0 0 0 0 0\n"
"RenderEvent 249 23 0 0 0 0 0\n"
"MouseMoveEvent 249 22 0 0 0 0 0\n"
"RenderEvent 249 22 0 0 0 0 0\n"
"MouseMoveEvent 250 22 0 0 0 0 0\n"
"RenderEvent 250 22 0 0 0 0 0\n"
"MouseMoveEvent 251 22 0 0 0 0 0\n"
"RenderEvent 251 22 0 0 0 0 0\n"
"MouseMoveEvent 252 22 0 0 0 0 0\n"
"RenderEvent 252 22 0 0 0 0 0\n"
"MouseMoveEvent 255 22 0 0 0 0 0\n"
"RenderEvent 255 22 0 0 0 0 0\n"
"MouseMoveEvent 257 22 0 0 0 0 0\n"
"RenderEvent 257 22 0 0 0 0 0\n"
"MouseMoveEvent 259 22 0 0 0 0 0\n"
"RenderEvent 259 22 0 0 0 0 0\n"
"MouseMoveEvent 261 22 0 0 0 0 0\n"
"RenderEvent 261 22 0 0 0 0 0\n"
"MouseMoveEvent 264 22 0 0 0 0 0\n"
"RenderEvent 264 22 0 0 0 0 0\n"
"MouseMoveEvent 267 22 0 0 0 0 0\n"
"RenderEvent 267 22 0 0 0 0 0\n"
"MouseMoveEvent 269 21 0 0 0 0 0\n"
"RenderEvent 269 21 0 0 0 0 0\n"
"MouseMoveEvent 271 21 0 0 0 0 0\n"
"RenderEvent 271 21 0 0 0 0 0\n"
"MouseMoveEvent 273 21 0 0 0 0 0\n"
"RenderEvent 273 21 0 0 0 0 0\n"
"MouseMoveEvent 275 21 0 0 0 0 0\n"
"RenderEvent 275 21 0 0 0 0 0\n"
"MouseMoveEvent 276 21 0 0 0 0 0\n"
"RenderEvent 276 21 0 0 0 0 0\n"
"MouseMoveEvent 277 21 0 0 0 0 0\n"
"RenderEvent 277 21 0 0 0 0 0\n"
"MouseMoveEvent 278 21 0 0 0 0 0\n"
"RenderEvent 278 21 0 0 0 0 0\n"
"MouseMoveEvent 279 21 0 0 0 0 0\n"
"RenderEvent 279 21 0 0 0 0 0\n"
"MouseMoveEvent 280 21 0 0 0 0 0\n"
"RenderEvent 280 21 0 0 0 0 0\n"
"MouseMoveEvent 281 21 0 0 0 0 0\n"
"RenderEvent 281 21 0 0 0 0 0\n"
"MouseMoveEvent 282 21 0 0 0 0 0\n"
"RenderEvent 282 21 0 0 0 0 0\n"
"MouseMoveEvent 283 21 0 0 0 0 0\n"
"RenderEvent 283 21 0 0 0 0 0\n"
"MouseMoveEvent 285 21 0 0 0 0 0\n"
"RenderEvent 285 21 0 0 0 0 0\n"
"MouseMoveEvent 287 21 0 0 0 0 0\n"
"RenderEvent 287 21 0 0 0 0 0\n"
"MouseMoveEvent 288 21 0 0 0 0 0\n"
"RenderEvent 288 21 0 0 0 0 0\n"
"MouseMoveEvent 290 21 0 0 0 0 0\n"
"RenderEvent 290 21 0 0 0 0 0\n"
"MouseMoveEvent 291 21 0 0 0 0 0\n"
"RenderEvent 291 21 0 0 0 0 0\n"
"MouseMoveEvent 292 21 0 0 0 0 0\n"
"RenderEvent 292 21 0 0 0 0 0\n"
"MouseMoveEvent 293 21 0 0 0 0 0\n"
"RenderEvent 293 21 0 0 0 0 0\n"
"MouseMoveEvent 294 21 0 0 0 0 0\n"
"RenderEvent 294 21 0 0 0 0 0\n"
"MouseMoveEvent 295 21 0 0 0 0 0\n"
"RenderEvent 295 21 0 0 0 0 0\n"
"MouseMoveEvent 296 21 0 0 0 0 0\n"
"RenderEvent 296 21 0 0 0 0 0\n"
"MouseMoveEvent 297 21 0 0 0 0 0\n"
"RenderEvent 297 21 0 0 0 0 0\n"
"MouseMoveEvent 298 21 0 0 0 0 0\n"
"RenderEvent 298 21 0 0 0 0 0\n"
"MouseMoveEvent 299 21 0 0 0 0 0\n"
"RenderEvent 299 21 0 0 0 0 0\n"
"MouseMoveEvent 300 21 0 0 0 0 0\n"
"RenderEvent 300 21 0 0 0 0 0\n"
"MouseMoveEvent 301 21 0 0 0 0 0\n"
"RenderEvent 301 21 0 0 0 0 0\n"
"MouseMoveEvent 302 21 0 0 0 0 0\n"
"RenderEvent 302 21 0 0 0 0 0\n"
"MouseMoveEvent 303 21 0 0 0 0 0\n"
"RenderEvent 303 21 0 0 0 0 0\n"
"MouseMoveEvent 304 21 0 0 0 0 0\n"
"RenderEvent 304 21 0 0 0 0 0\n"
"MouseMoveEvent 305 21 0 0 0 0 0\n"
"RenderEvent 305 21 0 0 0 0 0\n"
"MouseMoveEvent 306 21 0 0 0 0 0\n"
"RenderEvent 306 21 0 0 0 0 0\n"
"LeftButtonReleaseEvent 306 21 0 0 0 0 0\n"
"RenderEvent 306 21 0 0 0 0 0\n"
"MouseMoveEvent 306 20 0 0 0 0 0\n"
"MouseMoveEvent 304 19 0 0 0 0 0\n"
"MouseMoveEvent 302 19 0 0 0 0 0\n"
"MouseMoveEvent 301 19 0 0 0 0 0\n"
"MouseMoveEvent 300 19 0 0 0 0 0\n"
"MouseMoveEvent 298 19 0 0 0 0 0\n"
"MouseMoveEvent 296 19 0 0 0 0 0\n"
"MouseMoveEvent 295 20 0 0 0 0 0\n"
"MouseMoveEvent 293 21 0 0 0 0 0\n"
"MouseMoveEvent 291 22 0 0 0 0 0\n"
"MouseMoveEvent 290 23 0 0 0 0 0\n"
"MouseMoveEvent 286 27 0 0 0 0 0\n"
"MouseMoveEvent 282 31 0 0 0 0 0\n"
"MouseMoveEvent 281 32 0 0 0 0 0\n"
"MouseMoveEvent 279 38 0 0 0 0 0\n"
"MouseMoveEvent 278 40 0 0 0 0 0\n"
"MouseMoveEvent 274 46 0 0 0 0 0\n"
"MouseMoveEvent 273 48 0 0 0 0 0\n"
"MouseMoveEvent 269 54 0 0 0 0 0\n"
"MouseMoveEvent 267 64 0 0 0 0 0\n"
"MouseMoveEvent 267 74 0 0 0 0 0\n"
"MouseMoveEvent 263 90 0 0 0 0 0\n"
"MouseMoveEvent 261 108 0 0 0 0 0\n"
"MouseMoveEvent 255 124 0 0 0 0 0\n"
"MouseMoveEvent 249 144 0 0 0 0 0\n"
"MouseMoveEvent 249 160 0 0 0 0 0\n"
"MouseMoveEvent 249 172 0 0 0 0 0\n"
"MouseMoveEvent 249 186 0 0 0 0 0\n"
"MouseMoveEvent 249 196 0 0 0 0 0\n"
"MouseMoveEvent 251 204 0 0 0 0 0\n"
"MouseMoveEvent 251 212 0 0 0 0 0\n"
"MouseMoveEvent 251 220 0 0 0 0 0\n"
"MouseMoveEvent 251 228 0 0 0 0 0\n"
"MouseMoveEvent 253 238 0 0 0 0 0\n"
"MouseMoveEvent 253 241 0 0 0 0 0\n"
"MouseMoveEvent 253 244 0 0 0 0 0\n"
"MouseMoveEvent 253 246 0 0 0 0 0\n"
"MouseMoveEvent 253 248 0 0 0 0 0\n"
"MouseMoveEvent 253 249 0 0 0 0 0\n"
"MouseMoveEvent 253 250 0 0 0 0 0\n"
"MouseMoveEvent 253 251 0 0 0 0 0\n"
"MouseMoveEvent 253 253 0 0 0 0 0\n"
"MouseMoveEvent 252 254 0 0 0 0 0\n"
"MouseMoveEvent 251 255 0 0 0 0 0\n"
"MouseMoveEvent 250 256 0 0 0 0 0\n"
"MouseMoveEvent 250 257 0 0 0 0 0\n"
"MouseMoveEvent 249 257 0 0 0 0 0\n"
"MouseMoveEvent 248 258 0 0 0 0 0\n"
"MouseMoveEvent 248 259 0 0 0 0 0\n"
"MouseMoveEvent 248 260 0 0 0 0 0\n"
"LeftButtonPressEvent 248 260 0 0 0 0 0\n"
"RenderEvent 248 260 0 0 0 0 0\n"
"LeftButtonReleaseEvent 248 260 0 0 0 0 0\n"
"MouseMoveEvent 246 259 0 0 0 0 0\n"
"MouseMoveEvent 244 258 0 0 0 0 0\n"
"MouseMoveEvent 238 254 0 0 0 0 0\n"
"MouseMoveEvent 234 248 0 0 0 0 0\n"
"MouseMoveEvent 228 242 0 0 0 0 0\n"
"MouseMoveEvent 222 238 0 0 0 0 0\n"
"MouseMoveEvent 220 232 0 0 0 0 0\n"
"MouseMoveEvent 214 224 0 0 0 0 0\n"
"MouseMoveEvent 210 220 0 0 0 0 0\n"
"MouseMoveEvent 204 212 0 0 0 0 0\n"
"MouseMoveEvent 200 208 0 0 0 0 0\n"
"MouseMoveEvent 200 206 0 0 0 0 0\n"
"MouseMoveEvent 199 204 0 0 0 0 0\n"
"MouseMoveEvent 198 203 0 0 0 0 0\n"
"MouseMoveEvent 198 202 0 0 0 0 0\n"
"MouseMoveEvent 198 201 0 0 0 0 0\n"
"MouseMoveEvent 198 200 0 0 0 0 0\n"
"MouseMoveEvent 198 199 0 0 0 0 0\n"
"MouseMoveEvent 199 199 0 0 0 0 0\n"
"LeftButtonPressEvent 199 199 0 0 0 1 0\n"
"RenderEvent 199 199 0 0 0 1 0\n"
"MouseMoveEvent 199 198 0 0 0 0 0\n"
"MouseMoveEvent 200 197 0 0 0 0 0\n"
"MouseMoveEvent 202 196 0 0 0 0 0\n"
"MouseMoveEvent 206 190 0 0 0 0 0\n"
"MouseMoveEvent 212 188 0 0 0 0 0\n"
"LeftButtonReleaseEvent 212 188 0 0 0 0 0\n"
"MouseMoveEvent 218 184 0 0 0 0 0\n"
"MouseMoveEvent 226 180 0 0 0 0 0\n"
"MouseMoveEvent 234 176 0 0 0 0 0\n"
"MouseMoveEvent 242 174 0 0 0 0 0\n"
"MouseMoveEvent 248 172 0 0 0 0 0\n"
"MouseMoveEvent 254 170 0 0 0 0 0\n"
"MouseMoveEvent 262 168 0 0 0 0 0\n"
"MouseMoveEvent 270 168 0 0 0 0 0\n"
"MouseMoveEvent 280 168 0 0 0 0 0\n"
"MouseMoveEvent 283 168 0 0 0 0 0\n"
"MouseMoveEvent 286 168 0 0 0 0 0\n"
"MouseMoveEvent 288 168 0 0 0 0 0\n"
"MouseMoveEvent 290 168 0 0 0 0 0\n"
"MouseMoveEvent 290 169 0 0 0 0 0\n"
"MouseMoveEvent 291 169 0 0 0 0 0\n"
"LeftButtonPressEvent 291 169 0 0 0 0 0\n"
"RenderEvent 291 169 0 0 0 0 0\n"
"MouseMoveEvent 292 169 0 0 0 0 0\n"
"MouseMoveEvent 292 170 0 0 0 0 0\n"
"MouseMoveEvent 292 171 0 0 0 0 0\n"
"MouseMoveEvent 292 172 0 0 0 0 0\n"
"MouseMoveEvent 293 174 0 0 0 0 0\n"
"LeftButtonReleaseEvent 293 174 0 0 0 0 0\n"
"MouseMoveEvent 294 175 0 0 0 0 0\n"
"MouseMoveEvent 295 176 0 0 0 0 0\n"
"MouseMoveEvent 296 178 0 0 0 0 0\n"
"MouseMoveEvent 296 181 0 0 0 0 0\n"
"MouseMoveEvent 297 183 0 0 0 0 0\n"
"MouseMoveEvent 297 191 0 0 0 0 0\n"
"MouseMoveEvent 297 193 0 0 0 0 0\n"
"MouseMoveEvent 299 199 0 0 0 0 0\n"
"MouseMoveEvent 301 205 0 0 0 0 0\n"
"MouseMoveEvent 301 207 0 0 0 0 0\n"
"MouseMoveEvent 303 213 0 0 0 0 0\n"
"MouseMoveEvent 303 215 0 0 0 0 0\n"
"MouseMoveEvent 303 217 0 0 0 0 0\n"
"MouseMoveEvent 303 219 0 0 0 0 0\n"
"MouseMoveEvent 304 220 0 0 0 0 0\n"
"MouseMoveEvent 304 222 0 0 0 0 0\n"
"MouseMoveEvent 304 223 0 0 0 0 0\n"
"MouseMoveEvent 304 224 0 0 0 0 0\n"
"MouseMoveEvent 304 225 0 0 0 0 0\n"
"LeftButtonPressEvent 304 225 0 0 0 1 0\n"
"RenderEvent 304 225 0 0 0 1 0\n"
"MouseMoveEvent 304 226 0 0 0 0 0\n"
"MouseMoveEvent 303 227 0 0 0 0 0\n"
"MouseMoveEvent 303 228 0 0 0 0 0\n"
"LeftButtonReleaseEvent 303 228 0 0 0 0 0\n"
"MouseMoveEvent 302 230 0 0 0 0 0\n"
"MouseMoveEvent 302 231 0 0 0 0 0\n"
"MouseMoveEvent 302 233 0 0 0 0 0\n"
"MouseMoveEvent 298 237 0 0 0 0 0\n"
"MouseMoveEvent 294 245 0 0 0 0 0\n"
"MouseMoveEvent 290 249 0 0 0 0 0\n"
"MouseMoveEvent 289 250 0 0 0 0 0\n"
"MouseMoveEvent 288 251 0 0 0 0 0\n"
"MouseMoveEvent 284 257 0 0 0 0 0\n"
"MouseMoveEvent 283 258 0 0 0 0 0\n"
"MouseMoveEvent 283 259 0 0 0 0 0\n"
"MouseMoveEvent 283 260 0 0 0 0 0\n"
"MouseMoveEvent 282 261 0 0 0 0 0\n"
"MouseMoveEvent 281 261 0 0 0 0 0\n"
"MouseMoveEvent 281 262 0 0 0 0 0\n"
"MouseMoveEvent 280 263 0 0 0 0 0\n"
"MouseMoveEvent 279 263 0 0 0 0 0\n"
"MouseMoveEvent 278 263 0 0 0 0 0\n"
"MouseMoveEvent 277 263 0 0 0 0 0\n"
"MouseMoveEvent 276 263 0 0 0 0 0\n"
"MouseMoveEvent 275 263 0 0 0 0 0\n"
"MouseMoveEvent 273 263 0 0 0 0 0\n"
"MouseMoveEvent 273 264 0 0 0 0 0\n"
"MouseMoveEvent 272 264 0 0 0 0 0\n"
"MouseMoveEvent 271 264 0 0 0 0 0\n"
"MouseMoveEvent 270 264 0 0 0 0 0\n"
"MouseMoveEvent 269 264 0 0 0 0 0\n"
"MouseMoveEvent 267 263 0 0 0 0 0\n"
"MouseMoveEvent 266 262 0 0 0 0 0\n"
"MouseMoveEvent 265 262 0 0 0 0 0\n"
"MouseMoveEvent 265 261 0 0 0 0 0\n"
"MouseMoveEvent 264 261 0 0 0 0 0\n"
"MouseMoveEvent 263 261 0 0 0 0 0\n"
"MouseMoveEvent 263 260 0 0 0 0 0\n"
"MouseMoveEvent 262 259 0 0 0 0 0\n"
"MouseMoveEvent 261 259 0 0 0 0 0\n"
"MouseMoveEvent 260 259 0 0 0 0 0\n"
"MouseMoveEvent 259 259 0 0 0 0 0\n"
"MouseMoveEvent 257 259 0 0 0 0 0\n"
"MouseMoveEvent 256 259 0 0 0 0 0\n"
"MouseMoveEvent 255 259 0 0 0 0 0\n"
"MouseMoveEvent 254 259 0 0 0 0 0\n"
"MouseMoveEvent 253 259 0 0 0 0 0\n"
"MouseMoveEvent 253 258 0 0 0 0 0\n"
"MouseMoveEvent 252 258 0 0 0 0 0\n"
"MouseMoveEvent 252 259 0 0 0 0 0\n"
"MouseMoveEvent 251 259 0 0 0 0 0\n"
"MouseMoveEvent 250 259 0 0 0 0 0\n"
"MouseMoveEvent 249 259 0 0 0 0 0\n"
"LeftButtonPressEvent 249 259 0 0 0 0 0\n"
"RenderEvent 249 259 0 0 0 0 0\n"
"LeftButtonReleaseEvent 249 259 0 0 0 0 0\n"
"MouseMoveEvent 249 258 0 0 0 0 0\n"
"RenderEvent 249 258 0 0 0 0 0\n"
"MouseMoveEvent 249 257 0 0 0 0 0\n"
"MouseMoveEvent 249 256 0 0 0 0 0\n"
"MouseMoveEvent 249 253 0 0 0 0 0\n"
"RenderEvent 249 253 0 0 0 0 0\n"
"MouseMoveEvent 249 250 0 0 0 0 0\n"
"MouseMoveEvent 249 242 0 0 0 0 0\n"
"MouseMoveEvent 249 234 0 0 0 0 0\n"
"MouseMoveEvent 253 222 0 0 0 0 0\n"
"MouseMoveEvent 253 208 0 0 0 0 0\n"
"MouseMoveEvent 255 196 0 0 0 0 0\n"
"MouseMoveEvent 263 184 0 0 0 0 0\n"
"MouseMoveEvent 267 170 0 0 0 0 0\n"
"MouseMoveEvent 271 158 0 0 0 0 0\n"
"MouseMoveEvent 277 144 0 0 0 0 0\n"
"MouseMoveEvent 281 124 0 0 0 0 0\n"
"MouseMoveEvent 281 104 0 0 0 0 0\n"
"MouseMoveEvent 283 86 0 0 0 0 0\n"
"MouseMoveEvent 283 74 0 0 0 0 0\n"
"MouseMoveEvent 287 62 0 0 0 0 0\n"
"MouseMoveEvent 293 52 0 0 0 0 0\n"
"MouseMoveEvent 299 42 0 0 0 0 0\n"
"MouseMoveEvent 307 34 0 0 0 0 0\n"
"MouseMoveEvent 311 26 0 0 0 0 0\n"
"MouseMoveEvent 313 20 0 0 0 0 0\n"
"MouseMoveEvent 314 19 0 0 0 0 0\n"
"MouseMoveEvent 314 18 0 0 0 0 0\n"
"MouseMoveEvent 313 18 0 0 0 0 0\n"
"MouseMoveEvent 313 17 0 0 0 0 0\n"
"MouseMoveEvent 312 18 0 0 0 0 0\n"
"MouseMoveEvent 311 19 0 0 0 0 0\n"
"MouseMoveEvent 311 20 0 0 0 0 0\n"
"MouseMoveEvent 311 21 0 0 0 0 0\n"
"MouseMoveEvent 311 22 0 0 0 0 0\n"
"MouseMoveEvent 311 23 0 0 0 0 0\n"
"MouseMoveEvent 311 24 0 0 0 0 0\n"
"MouseMoveEvent 310 24 0 0 0 0 0\n"
"MouseMoveEvent 309 25 0 0 0 0 0\n"
"MouseMoveEvent 308 25 0 0 0 0 0\n"
"MouseMoveEvent 308 26 0 0 0 0 0\n"
"MouseMoveEvent 307 26 0 0 0 0 0\n"
"MouseMoveEvent 306 26 0 0 0 0 0\n"
"MouseMoveEvent 306 27 0 0 0 0 0\n"
"MouseMoveEvent 305 27 0 0 0 0 0\n"
"LeftButtonPressEvent 305 27 0 0 0 0 0\n"
"RenderEvent 305 27 0 0 0 0 0\n"
"MouseMoveEvent 304 27 0 0 0 0 0\n"
"RenderEvent 304 27 0 0 0 0 0\n"
"MouseMoveEvent 303 28 0 0 0 0 0\n"
"RenderEvent 303 28 0 0 0 0 0\n"
"MouseMoveEvent 301 28 0 0 0 0 0\n"
"RenderEvent 301 28 0 0 0 0 0\n"
"MouseMoveEvent 299 28 0 0 0 0 0\n"
"RenderEvent 299 28 0 0 0 0 0\n"
"MouseMoveEvent 297 28 0 0 0 0 0\n"
"RenderEvent 297 28 0 0 0 0 0\n"
"MouseMoveEvent 295 28 0 0 0 0 0\n"
"RenderEvent 295 28 0 0 0 0 0\n"
"MouseMoveEvent 293 28 0 0 0 0 0\n"
"RenderEvent 293 28 0 0 0 0 0\n"
"MouseMoveEvent 290 28 0 0 0 0 0\n"
"RenderEvent 290 28 0 0 0 0 0\n"
"MouseMoveEvent 288 28 0 0 0 0 0\n"
"RenderEvent 288 28 0 0 0 0 0\n"
"MouseMoveEvent 285 28 0 0 0 0 0\n"
"RenderEvent 285 28 0 0 0 0 0\n"
"MouseMoveEvent 283 28 0 0 0 0 0\n"
"RenderEvent 283 28 0 0 0 0 0\n"
"MouseMoveEvent 281 28 0 0 0 0 0\n"
"RenderEvent 281 28 0 0 0 0 0\n"
"MouseMoveEvent 280 28 0 0 0 0 0\n"
"RenderEvent 280 28 0 0 0 0 0\n"
"MouseMoveEvent 278 28 0 0 0 0 0\n"
"RenderEvent 278 28 0 0 0 0 0\n"
"MouseMoveEvent 277 28 0 0 0 0 0\n"
"RenderEvent 277 28 0 0 0 0 0\n"
"MouseMoveEvent 276 28 0 0 0 0 0\n"
"RenderEvent 276 28 0 0 0 0 0\n"
"MouseMoveEvent 274 28 0 0 0 0 0\n"
"RenderEvent 274 28 0 0 0 0 0\n"
"MouseMoveEvent 272 28 0 0 0 0 0\n"
"RenderEvent 272 28 0 0 0 0 0\n"
"MouseMoveEvent 270 28 0 0 0 0 0\n"
"RenderEvent 270 28 0 0 0 0 0\n"
"MouseMoveEvent 268 28 0 0 0 0 0\n"
"RenderEvent 268 28 0 0 0 0 0\n"
"MouseMoveEvent 266 28 0 0 0 0 0\n"
"RenderEvent 266 28 0 0 0 0 0\n"
"MouseMoveEvent 264 28 0 0 0 0 0\n"
"RenderEvent 264 28 0 0 0 0 0\n"
"MouseMoveEvent 262 27 0 0 0 0 0\n"
"RenderEvent 262 27 0 0 0 0 0\n"
"MouseMoveEvent 261 27 0 0 0 0 0\n"
"RenderEvent 261 27 0 0 0 0 0\n"
"MouseMoveEvent 259 27 0 0 0 0 0\n"
"RenderEvent 259 27 0 0 0 0 0\n"
"MouseMoveEvent 258 27 0 0 0 0 0\n"
"RenderEvent 258 27 0 0 0 0 0\n"
"MouseMoveEvent 256 27 0 0 0 0 0\n"
"RenderEvent 256 27 0 0 0 0 0\n"
"MouseMoveEvent 255 27 0 0 0 0 0\n"
"RenderEvent 255 27 0 0 0 0 0\n"
"MouseMoveEvent 253 27 0 0 0 0 0\n"
"RenderEvent 253 27 0 0 0 0 0\n"
"MouseMoveEvent 252 27 0 0 0 0 0\n"
"RenderEvent 252 27 0 0 0 0 0\n"
"MouseMoveEvent 251 27 0 0 0 0 0\n"
"RenderEvent 251 27 0 0 0 0 0\n"
"MouseMoveEvent 250 27 0 0 0 0 0\n"
"RenderEvent 250 27 0 0 0 0 0\n"
"MouseMoveEvent 249 27 0 0 0 0 0\n"
"RenderEvent 249 27 0 0 0 0 0\n"
"MouseMoveEvent 248 27 0 0 0 0 0\n"
"RenderEvent 248 27 0 0 0 0 0\n"
"MouseMoveEvent 247 27 0 0 0 0 0\n"
"RenderEvent 247 27 0 0 0 0 0\n"
"MouseMoveEvent 246 27 0 0 0 0 0\n"
"RenderEvent 246 27 0 0 0 0 0\n"
"MouseMoveEvent 245 27 0 0 0 0 0\n"
"RenderEvent 245 27 0 0 0 0 0\n"
"MouseMoveEvent 244 27 0 0 0 0 0\n"
"RenderEvent 244 27 0 0 0 0 0\n"
"MouseMoveEvent 242 27 0 0 0 0 0\n"
"RenderEvent 242 27 0 0 0 0 0\n"
"MouseMoveEvent 241 27 0 0 0 0 0\n"
"RenderEvent 241 27 0 0 0 0 0\n"
"MouseMoveEvent 240 27 0 0 0 0 0\n"
"RenderEvent 240 27 0 0 0 0 0\n"
"MouseMoveEvent 239 27 0 0 0 0 0\n"
"RenderEvent 239 27 0 0 0 0 0\n"
"MouseMoveEvent 238 27 0 0 0 0 0\n"
"RenderEvent 238 27 0 0 0 0 0\n"
"MouseMoveEvent 237 27 0 0 0 0 0\n"
"RenderEvent 237 27 0 0 0 0 0\n"
"MouseMoveEvent 236 27 0 0 0 0 0\n"
"RenderEvent 236 27 0 0 0 0 0\n"
"MouseMoveEvent 235 27 0 0 0 0 0\n"
"RenderEvent 235 27 0 0 0 0 0\n"
"MouseMoveEvent 234 27 0 0 0 0 0\n"
"RenderEvent 234 27 0 0 0 0 0\n"
"MouseMoveEvent 233 27 0 0 0 0 0\n"
"RenderEvent 233 27 0 0 0 0 0\n"
"MouseMoveEvent 232 27 0 0 0 0 0\n"
"RenderEvent 232 27 0 0 0 0 0\n"
"MouseMoveEvent 231 27 0 0 0 0 0\n"
"RenderEvent 231 27 0 0 0 0 0\n"
"MouseMoveEvent 230 27 0 0 0 0 0\n"
"RenderEvent 230 27 0 0 0 0 0\n"
"MouseMoveEvent 229 27 0 0 0 0 0\n"
"RenderEvent 229 27 0 0 0 0 0\n"
"MouseMoveEvent 228 27 0 0 0 0 0\n"
"RenderEvent 228 27 0 0 0 0 0\n"
"MouseMoveEvent 227 27 0 0 0 0 0\n"
"RenderEvent 227 27 0 0 0 0 0\n"
"MouseMoveEvent 226 27 0 0 0 0 0\n"
"RenderEvent 226 27 0 0 0 0 0\n"
"LeftButtonReleaseEvent 226 27 0 0 0 0 0\n"
"RenderEvent 226 27 0 0 0 0 0\n"
"MouseMoveEvent 226 29 0 0 0 0 0\n"
"MouseMoveEvent 226 31 0 0 0 0 0\n"
"MouseMoveEvent 226 33 0 0 0 0 0\n"
"MouseMoveEvent 226 43 0 0 0 0 0\n"
"MouseMoveEvent 226 55 0 0 0 0 0\n"
"MouseMoveEvent 226 65 0 0 0 0 0\n"
"MouseMoveEvent 226 77 0 0 0 0 0\n"
"MouseMoveEvent 226 91 0 0 0 0 0\n"
"MouseMoveEvent 226 103 0 0 0 0 0\n"
"MouseMoveEvent 226 113 0 0 0 0 0\n"
"MouseMoveEvent 226 125 0 0 0 0 0\n"
"MouseMoveEvent 226 135 0 0 0 0 0\n"
"MouseMoveEvent 226 147 0 0 0 0 0\n"
"MouseMoveEvent 224 155 0 0 0 0 0\n"
"MouseMoveEvent 222 163 0 0 0 0 0\n"
"MouseMoveEvent 220 171 0 0 0 0 0\n"
"MouseMoveEvent 218 181 0 0 0 0 0\n"
"MouseMoveEvent 218 184 0 0 0 0 0\n"
"MouseMoveEvent 218 192 0 0 0 0 0\n"
"MouseMoveEvent 218 194 0 0 0 0 0\n"
"MouseMoveEvent 218 196 0 0 0 0 0\n"
"MouseMoveEvent 218 197 0 0 0 0 0\n"
"MouseMoveEvent 218 198 0 0 0 0 0\n"
"MouseMoveEvent 217 199 0 0 0 0 0\n"
"MouseMoveEvent 217 200 0 0 0 0 0\n"
"MouseMoveEvent 217 201 0 0 0 0 0\n"
"MouseMoveEvent 217 202 0 0 0 0 0\n"
"MouseMoveEvent 217 203 0 0 0 0 0\n"
"MouseMoveEvent 217 204 0 0 0 0 0\n"
"MouseMoveEvent 216 204 0 0 0 0 0\n"
"MouseMoveEvent 215 204 0 0 0 0 0\n"
"MouseMoveEvent 214 203 0 0 0 0 0\n"
"MouseMoveEvent 213 202 0 0 0 0 0\n"
"MouseMoveEvent 211 201 0 0 0 0 0\n"
"MouseMoveEvent 210 201 0 0 0 0 0\n"
"MouseMoveEvent 209 201 0 0 0 0 0\n"
"MouseMoveEvent 207 200 0 0 0 0 0\n"
"MouseMoveEvent 207 199 0 0 0 0 0\n"
"MouseMoveEvent 205 200 0 0 0 0 0\n"
"RenderEvent 205 200 0 0 0 0 0\n"
"MouseMoveEvent 204 200 0 0 0 0 0\n"
"MouseMoveEvent 203 200 0 0 0 0 0\n"
"MouseMoveEvent 202 199 0 0 0 0 0\n"
"MouseMoveEvent 201 199 0 0 0 0 0\n"
"LeftButtonPressEvent 201 199 0 0 0 0 0\n"
"MouseMoveEvent 200 199 0 0 0 0 0\n"
"RenderEvent 200 199 0 0 0 0 0\n"
"MouseMoveEvent 199 199 0 0 0 0 0\n"
"RenderEvent 199 199 0 0 0 0 0\n"
"MouseMoveEvent 198 199 0 0 0 0 0\n"
"RenderEvent 198 199 0 0 0 0 0\n"
"MouseMoveEvent 196 199 0 0 0 0 0\n"
"RenderEvent 196 199 0 0 0 0 0\n"
"MouseMoveEvent 193 199 0 0 0 0 0\n"
"RenderEvent 193 199 0 0 0 0 0\n"
"MouseMoveEvent 190 199 0 0 0 0 0\n"
"RenderEvent 190 199 0 0 0 0 0\n"
"MouseMoveEvent 187 199 0 0 0 0 0\n"
"RenderEvent 187 199 0 0 0 0 0\n"
"MouseMoveEvent 184 199 0 0 0 0 0\n"
"RenderEvent 184 199 0 0 0 0 0\n"
"MouseMoveEvent 181 199 0 0 0 0 0\n"
"RenderEvent 181 199 0 0 0 0 0\n"
"MouseMoveEvent 175 197 0 0 0 0 0\n"
"RenderEvent 175 197 0 0 0 0 0\n"
"MouseMoveEvent 167 197 0 0 0 0 0\n"
"RenderEvent 167 197 0 0 0 0 0\n"
"MouseMoveEvent 164 197 0 0 0 0 0\n"
"RenderEvent 164 197 0 0 0 0 0\n"
"MouseMoveEvent 161 197 0 0 0 0 0\n"
"RenderEvent 161 197 0 0 0 0 0\n"
"MouseMoveEvent 159 197 0 0 0 0 0\n"
"RenderEvent 159 197 0 0 0 0 0\n"
"MouseMoveEvent 157 197 0 0 0 0 0\n"
"RenderEvent 157 197 0 0 0 0 0\n"
"MouseMoveEvent 156 197 0 0 0 0 0\n"
"RenderEvent 156 197 0 0 0 0 0\n"
"MouseMoveEvent 155 197 0 0 0 0 0\n"
"RenderEvent 155 197 0 0 0 0 0\n"
"MouseMoveEvent 154 197 0 0 0 0 0\n"
"RenderEvent 154 197 0 0 0 0 0\n"
"MouseMoveEvent 153 197 0 0 0 0 0\n"
"RenderEvent 153 197 0 0 0 0 0\n"
"MouseMoveEvent 152 197 0 0 0 0 0\n"
"RenderEvent 152 197 0 0 0 0 0\n"
"MouseMoveEvent 151 197 0 0 0 0 0\n"
"RenderEvent 151 197 0 0 0 0 0\n"
"MouseMoveEvent 150 197 0 0 0 0 0\n"
"RenderEvent 150 197 0 0 0 0 0\n"
"MouseMoveEvent 149 197 0 0 0 0 0\n"
"RenderEvent 149 197 0 0 0 0 0\n"
"MouseMoveEvent 148 197 0 0 0 0 0\n"
"RenderEvent 148 197 0 0 0 0 0\n"
"MouseMoveEvent 147 197 0 0 0 0 0\n"
"RenderEvent 147 197 0 0 0 0 0\n"
"MouseMoveEvent 146 197 0 0 0 0 0\n"
"RenderEvent 146 197 0 0 0 0 0\n"
"LeftButtonReleaseEvent 146 197 0 0 0 0 0\n"
"MouseMoveEvent 146 195 0 0 0 0 0\n"
"MouseMoveEvent 146 193 0 0 0 0 0\n"
"MouseMoveEvent 146 192 0 0 0 0 0\n"
"MouseMoveEvent 148 184 0 0 0 0 0\n"
"RenderEvent 148 184 0 0 0 0 0\n"
"MouseMoveEvent 149 182 0 0 0 0 0\n"
"MouseMoveEvent 151 176 0 0 0 0 0\n"
"MouseMoveEvent 155 168 0 0 0 0 0\n"
"MouseMoveEvent 159 162 0 0 0 0 0\n"
"MouseMoveEvent 161 152 0 0 0 0 0\n"
"MouseMoveEvent 165 144 0 0 0 0 0\n"
"MouseMoveEvent 167 136 0 0 0 0 0\n"
"MouseMoveEvent 169 128 0 0 0 0 0\n"
"MouseMoveEvent 171 120 0 0 0 0 0\n"
"MouseMoveEvent 173 110 0 0 0 0 0\n"
"MouseMoveEvent 173 102 0 0 0 0 0\n"
"MouseMoveEvent 173 94 0 0 0 0 0\n"
"MouseMoveEvent 173 86 0 0 0 0 0\n"
"MouseMoveEvent 173 83 0 0 0 0 0\n"
"MouseMoveEvent 173 75 0 0 0 0 0\n"
"MouseMoveEvent 173 73 0 0 0 0 0\n"
"MouseMoveEvent 173 71 0 0 0 0 0\n"
"MouseMoveEvent 173 68 0 0 0 0 0\n"
"MouseMoveEvent 173 67 0 0 0 0 0\n"
"MouseMoveEvent 173 65 0 0 0 0 0\n"
"MouseMoveEvent 173 63 0 0 0 0 0\n"
"MouseMoveEvent 173 61 0 0 0 0 0\n"
"MouseMoveEvent 173 60 0 0 0 0 0\n"
"MouseMoveEvent 173 58 0 0 0 0 0\n"
"MouseMoveEvent 173 56 0 0 0 0 0\n"
"MouseMoveEvent 173 55 0 0 0 0 0\n"
"MouseMoveEvent 174 54 0 0 0 0 0\n"
"MouseMoveEvent 174 52 0 0 0 0 0\n"
"MouseMoveEvent 175 51 0 0 0 0 0\n"
"MouseMoveEvent 175 50 0 0 0 0 0\n"
"MouseMoveEvent 175 49 0 0 0 0 0\n"
"MouseMoveEvent 176 48 0 0 0 0 0\n"
"MouseMoveEvent 177 47 0 0 0 0 0\n"
"MouseMoveEvent 177 46 0 0 0 0 0\n"
"MouseMoveEvent 177 45 0 0 0 0 0\n"
"MouseMoveEvent 178 44 0 0 0 0 0\n"
"MouseMoveEvent 179 44 0 0 0 0 0\n"
"MouseMoveEvent 180 44 0 0 0 0 0\n"
"MouseMoveEvent 181 43 0 0 0 0 0\n"
"MouseMoveEvent 183 42 0 0 0 0 0\n"
"MouseMoveEvent 184 42 0 0 0 0 0\n"
"MouseMoveEvent 185 42 0 0 0 0 0\n"
"MouseMoveEvent 187 42 0 0 0 0 0\n"
"MouseMoveEvent 189 42 0 0 0 0 0\n"
"MouseMoveEvent 191 42 0 0 0 0 0\n"
"MouseMoveEvent 192 42 0 0 0 0 0\n"
"MouseMoveEvent 194 42 0 0 0 0 0\n"
"MouseMoveEvent 195 42 0 0 0 0 0\n"
"MouseMoveEvent 197 43 0 0 0 0 0\n"
"MouseMoveEvent 198 43 0 0 0 0 0\n"
"MouseMoveEvent 199 43 0 0 0 0 0\n"
"MouseMoveEvent 200 43 0 0 0 0 0\n"
"MouseMoveEvent 201 43 0 0 0 0 0\n"
"MouseMoveEvent 203 43 0 0 0 0 0\n"
"MouseMoveEvent 204 43 0 0 0 0 0\n"
"MouseMoveEvent 207 43 0 0 0 0 0\n"
"MouseMoveEvent 209 43 0 0 0 0 0\n"
"MouseMoveEvent 211 42 0 0 0 0 0\n"
"MouseMoveEvent 213 42 0 0 0 0 0\n"
"MouseMoveEvent 215 42 0 0 0 0 0\n"
"MouseMoveEvent 217 42 0 0 0 0 0\n"
"MouseMoveEvent 219 41 0 0 0 0 0\n"
"MouseMoveEvent 220 41 0 0 0 0 0\n"
"MouseMoveEvent 221 40 0 0 0 0 0\n"
"MouseMoveEvent 222 40 0 0 0 0 0\n"
"MouseMoveEvent 223 40 0 0 0 0 0\n"
"MouseMoveEvent 223 39 0 0 0 0 0\n"
"MouseMoveEvent 223 38 0 0 0 0 0\n"
"MouseMoveEvent 223 37 0 0 0 0 0\n"
"MouseMoveEvent 223 36 0 0 0 0 0\n"
"MouseMoveEvent 223 35 0 0 0 0 0\n"
"MouseMoveEvent 224 34 0 0 0 0 0\n"
"MouseMoveEvent 225 33 0 0 0 0 0\n"
"MouseMoveEvent 225 32 0 0 0 0 0\n"
"MouseMoveEvent 225 31 0 0 0 0 0\n"
"MouseMoveEvent 225 30 0 0 0 0 0\n"
"MouseMoveEvent 225 29 0 0 0 0 0\n"
"MouseMoveEvent 225 28 0 0 0 0 0\n"
"MouseMoveEvent 225 27 0 0 0 0 0\n"
"MouseMoveEvent 226 27 0 0 0 0 0\n"
"MouseMoveEvent 226 26 0 0 0 0 0\n"
"MouseMoveEvent 227 25 0 0 0 0 0\n"
"LeftButtonPressEvent 227 25 0 0 0 0 0\n"
"RenderEvent 227 25 0 0 0 0 0\n"
"MouseMoveEvent 228 25 0 0 0 0 0\n"
"RenderEvent 228 25 0 0 0 0 0\n"
"MouseMoveEvent 229 25 0 0 0 0 0\n"
"RenderEvent 229 25 0 0 0 0 0\n"
"MouseMoveEvent 230 25 0 0 0 0 0\n"
"RenderEvent 230 25 0 0 0 0 0\n"
"MouseMoveEvent 231 25 0 0 0 0 0\n"
"RenderEvent 231 25 0 0 0 0 0\n"
"MouseMoveEvent 232 25 0 0 0 0 0\n"
"RenderEvent 232 25 0 0 0 0 0\n"
"MouseMoveEvent 234 24 0 0 0 0 0\n"
"RenderEvent 234 24 0 0 0 0 0\n"
"MouseMoveEvent 235 24 0 0 0 0 0\n"
"RenderEvent 235 24 0 0 0 0 0\n"
"MouseMoveEvent 236 24 0 0 0 0 0\n"
"RenderEvent 236 24 0 0 0 0 0\n"
"MouseMoveEvent 237 24 0 0 0 0 0\n"
"RenderEvent 237 24 0 0 0 0 0\n"
"MouseMoveEvent 238 24 0 0 0 0 0\n"
"RenderEvent 238 24 0 0 0 0 0\n"
"MouseMoveEvent 239 24 0 0 0 0 0\n"
"RenderEvent 239 24 0 0 0 0 0\n"
"MouseMoveEvent 240 24 0 0 0 0 0\n"
"RenderEvent 240 24 0 0 0 0 0\n"
"MouseMoveEvent 241 25 0 0 0 0 0\n"
"RenderEvent 241 25 0 0 0 0 0\n"
"MouseMoveEvent 243 25 0 0 0 0 0\n"
"RenderEvent 243 25 0 0 0 0 0\n"
"MouseMoveEvent 244 25 0 0 0 0 0\n"
"RenderEvent 244 25 0 0 0 0 0\n"
"MouseMoveEvent 245 25 0 0 0 0 0\n"
"RenderEvent 245 25 0 0 0 0 0\n"
"MouseMoveEvent 247 25 0 0 0 0 0\n"
"RenderEvent 247 25 0 0 0 0 0\n"
"MouseMoveEvent 248 25 0 0 0 0 0\n"
"RenderEvent 248 25 0 0 0 0 0\n"
"MouseMoveEvent 249 25 0 0 0 0 0\n"
"RenderEvent 249 25 0 0 0 0 0\n"
"MouseMoveEvent 249 26 0 0 0 0 0\n"
"RenderEvent 249 26 0 0 0 0 0\n"
"MouseMoveEvent 251 26 0 0 0 0 0\n"
"RenderEvent 251 26 0 0 0 0 0\n"
"MouseMoveEvent 253 26 0 0 0 0 0\n"
"RenderEvent 253 26 0 0 0 0 0\n"
"MouseMoveEvent 254 26 0 0 0 0 0\n"
"RenderEvent 254 26 0 0 0 0 0\n"
"MouseMoveEvent 255 26 0 0 0 0 0\n"
"RenderEvent 255 26 0 0 0 0 0\n"
"MouseMoveEvent 257 26 0 0 0 0 0\n"
"RenderEvent 257 26 0 0 0 0 0\n"
"MouseMoveEvent 258 26 0 0 0 0 0\n"
"RenderEvent 258 26 0 0 0 0 0\n"
"MouseMoveEvent 259 26 0 0 0 0 0\n"
"RenderEvent 259 26 0 0 0 0 0\n"
"MouseMoveEvent 260 26 0 0 0 0 0\n"
"RenderEvent 260 26 0 0 0 0 0\n"
"MouseMoveEvent 261 26 0 0 0 0 0\n"
"RenderEvent 261 26 0 0 0 0 0\n"
"MouseMoveEvent 262 26 0 0 0 0 0\n"
"RenderEvent 262 26 0 0 0 0 0\n"
"MouseMoveEvent 263 26 0 0 0 0 0\n"
"RenderEvent 263 26 0 0 0 0 0\n"
"MouseMoveEvent 264 26 0 0 0 0 0\n"
"RenderEvent 264 26 0 0 0 0 0\n"
"MouseMoveEvent 265 25 0 0 0 0 0\n"
"RenderEvent 265 25 0 0 0 0 0\n"
"MouseMoveEvent 266 25 0 0 0 0 0\n"
"RenderEvent 266 25 0 0 0 0 0\n"
"MouseMoveEvent 267 25 0 0 0 0 0\n"
"RenderEvent 267 25 0 0 0 0 0\n"
"MouseMoveEvent 268 25 0 0 0 0 0\n"
"RenderEvent 268 25 0 0 0 0 0\n"
"MouseMoveEvent 269 25 0 0 0 0 0\n"
"RenderEvent 269 25 0 0 0 0 0\n"
"MouseMoveEvent 270 25 0 0 0 0 0\n"
"RenderEvent 270 25 0 0 0 0 0\n"
"MouseMoveEvent 271 24 0 0 0 0 0\n"
"RenderEvent 271 24 0 0 0 0 0\n"
"MouseMoveEvent 272 24 0 0 0 0 0\n"
"RenderEvent 272 24 0 0 0 0 0\n"
"LeftButtonReleaseEvent 272 24 0 0 0 0 0\n"
"RenderEvent 272 24 0 0 0 0 0\n"
"MouseMoveEvent 272 25 0 0 0 0 0\n"
"MouseMoveEvent 272 26 0 0 0 0 0\n"
"ExitEvent 272 26 0 0 113 1 q\n"
;

class vtkSliderCallback2 : public vtkCommand
{
public:
  static vtkSliderCallback2 *New() 
    { return new vtkSliderCallback2; }
  void SetImageViewer(vtkImageViewer2 *viewer)
    { this->Viewer =  viewer; }
  virtual void Execute(vtkObject *caller, unsigned long , void* )
    {
      vtkSliderWidget *slider = static_cast<vtkSliderWidget *>(caller);
      vtkSliderRepresentation *sliderRepres = static_cast<vtkSliderRepresentation *>(slider->GetRepresentation());
      int pos = static_cast<int>(sliderRepres->GetValue());

    this->Viewer->SetSlice(pos);
    }
protected:
  vtkImageViewer2 *Viewer;
};

int TestImageActorContourWidget(int argc, char *argv[])
{
  bool disableReplay = false, followCursor = false;
  for (int i = 0; i < argc; i++)
    {
    disableReplay |= (strcmp("--DisableReplay", argv[i]) == 0);
    followCursor  |= (strcmp("--FollowCursor", argv[i]) == 0);
    }  

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkSmartPointer<vtkVolume16Reader> v16 =
    vtkSmartPointer<vtkVolume16Reader>::New();
  v16->SetDataDimensions(64, 64);
  v16->SetDataByteOrderToLittleEndian();
  v16->SetImageRange(1, 93);
  v16->SetDataSpacing(3.2, 3.2, 1.5);
  v16->SetFilePrefix(fname);
  v16->ReleaseDataFlagOn();
  v16->SetDataMask(0x7fff);
  v16->Update();
  delete[] fname;
    
  double range[2];
  v16->GetOutput()->GetScalarRange(range);

  vtkSmartPointer<vtkImageShiftScale> shifter =
    vtkSmartPointer<vtkImageShiftScale>::New();
  shifter->SetShift(-1.0*range[0]);
  shifter->SetScale(255.0/(range[1]-range[0]));
  shifter->SetOutputScalarTypeToUnsignedChar();
  shifter->SetInputConnection(v16->GetOutputPort());
  shifter->ReleaseDataFlagOff();
  shifter->Update();

  
  vtkSmartPointer<vtkImageViewer2> imageViewer =
    vtkSmartPointer<vtkImageViewer2>::New();
  imageViewer->SetInput(shifter->GetOutput());
  imageViewer->SetColorLevel(127);
  imageViewer->SetColorWindow(255);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  imageViewer->SetupInteractor(iren);
  imageViewer->GetRenderWindow()->SetMultiSamples(0);
  imageViewer->GetRenderWindow()->SetSize(500, 500);

  imageViewer->Render();
  imageViewer->GetRenderer()->ResetCamera();

  imageViewer->Render();    
  
  vtkSmartPointer<vtkSliderRepresentation2D> SliderRepres =
    vtkSmartPointer<vtkSliderRepresentation2D>::New();
  int min = imageViewer->GetSliceMin();
  int max = imageViewer->GetSliceMax();
  SliderRepres->SetMinimumValue(min);
  SliderRepres->SetMaximumValue(max);
  SliderRepres->SetValue(static_cast<int>((min + max) / 2));
  SliderRepres->SetTitleText("Slice");
  SliderRepres->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  SliderRepres->GetPoint1Coordinate()->SetValue(0.3, 0.05);
  SliderRepres->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  SliderRepres->GetPoint2Coordinate()->SetValue(0.7, 0.05);
  SliderRepres->SetSliderLength(0.02);
  SliderRepres->SetSliderWidth(0.03);
  SliderRepres->SetEndCapLength(0.01);
  SliderRepres->SetEndCapWidth(0.03);
  SliderRepres->SetTubeWidth(0.005);
  SliderRepres->SetLabelFormat("%3.0lf");
  SliderRepres->SetTitleHeight(0.02);
  SliderRepres->SetLabelHeight(0.02);

  vtkSmartPointer<vtkSliderWidget> SliderWidget =
    vtkSmartPointer<vtkSliderWidget>::New();
  SliderWidget->SetInteractor(iren);
  SliderWidget->SetRepresentation(SliderRepres);
  SliderWidget->KeyPressActivationOff();
  SliderWidget->SetAnimationModeToAnimate();
  SliderWidget->SetEnabled(true);
  
  vtkSmartPointer<vtkSliderCallback2> SliderCb =
    vtkSmartPointer<vtkSliderCallback2>::New();
  SliderCb->SetImageViewer(imageViewer);
  SliderWidget->AddObserver(vtkCommand::InteractionEvent, SliderCb);  

  imageViewer->SetSlice(static_cast<int>(SliderRepres->GetValue()));

  vtkSmartPointer<vtkContourWidget> ContourWidget =
    vtkSmartPointer<vtkContourWidget>::New();

  vtkSmartPointer<vtkOrientedGlyphContourRepresentation> rep =
    vtkSmartPointer<vtkOrientedGlyphContourRepresentation>::New();
  ContourWidget->SetRepresentation(rep);
  
  vtkSmartPointer<vtkImageActorPointPlacer>  imageActorPointPlacer =
    vtkSmartPointer<vtkImageActorPointPlacer>::New();
  imageActorPointPlacer->SetImageActor(imageViewer->GetImageActor());
  rep->SetPointPlacer(imageActorPointPlacer);
  rep->GetProperty()->SetColor(0,1,0);

  ContourWidget->SetInteractor(iren);
  ContourWidget->SetFollowCursor( followCursor );
  ContourWidget->SetEnabled(true);
  ContourWidget->ProcessEventsOn();

  imageViewer->GetRenderWindow()->SetSize(500, 500);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  //recorder->SetFileName("/tmp/record.log");
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TestImageActorContourWidgetLog);

  // render the image
  //
  iren->Initialize();
  imageViewer->Render();

  if (!disableReplay)
    {
    recorder->EnabledOn();
    recorder->Play();
    }

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();
  
  return EXIT_SUCCESS;
}


