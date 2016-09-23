/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRemoveVolumeNonCurrentContext.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Test for releasing graphics resources from a non-current
// render window with vtkGPUVolumeRayCastMapper
// Thanks to Stephan Rademacher for providing the testing code.

#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsReader.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

//-----------------------------------------------------------------------------
static const char * TestRemoveVolumeNonCurrentContextLog =
"# StreamVersion 1\n"
"EnterEvent 263 265 0 0 0 0 0\n"
"MouseMoveEvent 259 229 0 0 0 0 0\n"
"MouseMoveEvent 255 185 0 0 0 0 0\n"
"MouseMoveEvent 255 163 0 0 0 0 0\n"
"MouseMoveEvent 252 163 0 0 0 0 0\n"
"MouseMoveEvent 247 164 0 0 0 0 0\n"
"MouseMoveEvent 244 170 0 0 0 0 0\n"
"MouseMoveEvent 243 178 0 0 0 0 0\n"
"MouseMoveEvent 243 189 0 0 0 0 0\n"
"MouseMoveEvent 243 211 0 0 0 0 0\n"
"MouseMoveEvent 250 233 0 0 0 0 0\n"
"MouseMoveEvent 253 243 0 0 0 0 0\n"
"MouseMoveEvent 256 251 0 0 0 0 0\n"
"MouseMoveEvent 258 255 0 0 0 0 0\n"
"MouseMoveEvent 258 256 0 0 0 0 0\n"
"MouseMoveEvent 258 257 0 0 0 0 0\n"
"MouseMoveEvent 258 262 0 0 0 0 0\n"
"MouseMoveEvent 258 268 0 0 0 0 0\n"
"MouseMoveEvent 258 275 0 0 0 0 0\n"
"MouseMoveEvent 259 282 0 0 0 0 0\n"
"MouseMoveEvent 259 289 0 0 0 0 0\n"
"MouseMoveEvent 260 293 0 0 0 0 0\n"
"MouseMoveEvent 260 296 0 0 0 0 0\n"
"MouseMoveEvent 260 298 0 0 0 0 0\n"
"MouseMoveEvent 261 299 0 0 0 0 0\n"
"LeaveEvent 261 300 0 0 0 0 0\n"
"EnterEvent 263 296 0 0 0 0 0\n"
"MouseMoveEvent 268 283 0 0 0 0 0\n"
"MouseMoveEvent 272 262 0 0 0 0 0\n"
"MouseMoveEvent 278 235 0 0 0 0 0\n"
"MouseMoveEvent 284 208 0 0 0 0 0\n"
"MouseMoveEvent 288 189 0 0 0 0 0\n"
"MouseMoveEvent 291 177 0 0 0 0 0\n"
"MouseMoveEvent 291 178 0 0 0 0 0\n"
"MouseMoveEvent 291 180 0 0 0 0 0\n"
"MouseMoveEvent 291 184 0 0 0 0 0\n"
"MouseMoveEvent 291 188 0 0 0 0 0\n"
"MouseMoveEvent 291 191 0 0 0 0 0\n"
"MouseMoveEvent 291 196 0 0 0 0 0\n"
"MouseMoveEvent 291 199 0 0 0 0 0\n"
"MouseMoveEvent 289 203 0 0 0 0 0\n"
"MouseMoveEvent 289 207 0 0 0 0 0\n"
"MouseMoveEvent 288 213 0 0 0 0 0\n"
"MouseMoveEvent 288 216 0 0 0 0 0\n"
"MouseMoveEvent 287 219 0 0 0 0 0\n"
"MouseMoveEvent 287 222 0 0 0 0 0\n"
"MouseMoveEvent 287 223 0 0 0 0 0\n"
"MouseMoveEvent 287 224 0 0 0 0 0\n"
"LeftButtonPressEvent 287 224 0 0 0 0 0\n"
"StartInteractionEvent 287 224 0 0 0 0 0\n"
"TimerEvent 287 224 0 0 0 0 0\n"
"RenderEvent 287 224 0 0 0 0 0\n"
"TimerEvent 287 224 0 0 0 0 0\n"
"RenderEvent 287 224 0 0 0 0 0\n"
"TimerEvent 287 224 0 0 0 0 0\n"
"RenderEvent 287 224 0 0 0 0 0\n"
"TimerEvent 287 224 0 0 0 0 0\n"
"RenderEvent 287 224 0 0 0 0 0\n"
"TimerEvent 287 224 0 0 0 0 0\n"
"RenderEvent 287 224 0 0 0 0 0\n"
"TimerEvent 287 224 0 0 0 0 0\n"
"RenderEvent 287 224 0 0 0 0 0\n"
"TimerEvent 287 224 0 0 0 0 0\n"
"RenderEvent 287 224 0 0 0 0 0\n"
"TimerEvent 287 224 0 0 0 0 0\n"
"RenderEvent 287 224 0 0 0 0 0\n"
"MouseMoveEvent 287 223 0 0 0 0 0\n"
"InteractionEvent 287 223 0 0 0 0 0\n"
"TimerEvent 287 223 0 0 0 0 0\n"
"RenderEvent 287 223 0 0 0 0 0\n"
"MouseMoveEvent 287 218 0 0 0 0 0\n"
"InteractionEvent 287 218 0 0 0 0 0\n"
"TimerEvent 287 218 0 0 0 0 0\n"
"RenderEvent 287 218 0 0 0 0 0\n"
"MouseMoveEvent 287 212 0 0 0 0 0\n"
"InteractionEvent 287 212 0 0 0 0 0\n"
"TimerEvent 287 212 0 0 0 0 0\n"
"RenderEvent 287 212 0 0 0 0 0\n"
"MouseMoveEvent 287 203 0 0 0 0 0\n"
"InteractionEvent 287 203 0 0 0 0 0\n"
"TimerEvent 287 203 0 0 0 0 0\n"
"RenderEvent 287 203 0 0 0 0 0\n"
"MouseMoveEvent 287 196 0 0 0 0 0\n"
"InteractionEvent 287 196 0 0 0 0 0\n"
"TimerEvent 287 196 0 0 0 0 0\n"
"RenderEvent 287 196 0 0 0 0 0\n"
"TimerEvent 287 196 0 0 0 0 0\n"
"RenderEvent 287 196 0 0 0 0 0\n"
"MouseMoveEvent 287 188 0 0 0 0 0\n"
"InteractionEvent 287 188 0 0 0 0 0\n"
"MouseMoveEvent 287 180 0 0 0 0 0\n"
"InteractionEvent 287 180 0 0 0 0 0\n"
"TimerEvent 287 180 0 0 0 0 0\n"
"RenderEvent 287 180 0 0 0 0 0\n"
"MouseMoveEvent 287 175 0 0 0 0 0\n"
"InteractionEvent 287 175 0 0 0 0 0\n"
"TimerEvent 287 175 0 0 0 0 0\n"
"RenderEvent 287 175 0 0 0 0 0\n"
"TimerEvent 287 175 0 0 0 0 0\n"
"RenderEvent 287 175 0 0 0 0 0\n"
"MouseMoveEvent 287 170 0 0 0 0 0\n"
"InteractionEvent 287 170 0 0 0 0 0\n"
"TimerEvent 287 170 0 0 0 0 0\n"
"RenderEvent 287 170 0 0 0 0 0\n"
"MouseMoveEvent 288 164 0 0 0 0 0\n"
"InteractionEvent 288 164 0 0 0 0 0\n"
"MouseMoveEvent 289 160 0 0 0 0 0\n"
"InteractionEvent 289 160 0 0 0 0 0\n"
"TimerEvent 289 160 0 0 0 0 0\n"
"RenderEvent 289 160 0 0 0 0 0\n"
"TimerEvent 289 160 0 0 0 0 0\n"
"RenderEvent 289 160 0 0 0 0 0\n"
"MouseMoveEvent 290 155 0 0 0 0 0\n"
"InteractionEvent 290 155 0 0 0 0 0\n"
"TimerEvent 290 155 0 0 0 0 0\n"
"RenderEvent 290 155 0 0 0 0 0\n"
"MouseMoveEvent 292 150 0 0 0 0 0\n"
"InteractionEvent 292 150 0 0 0 0 0\n"
"MouseMoveEvent 292 143 0 0 0 0 0\n"
"InteractionEvent 292 143 0 0 0 0 0\n"
"TimerEvent 292 143 0 0 0 0 0\n"
"RenderEvent 292 143 0 0 0 0 0\n"
"TimerEvent 292 143 0 0 0 0 0\n"
"RenderEvent 292 143 0 0 0 0 0\n"
"MouseMoveEvent 294 137 0 0 0 0 0\n"
"InteractionEvent 294 137 0 0 0 0 0\n"
"TimerEvent 294 137 0 0 0 0 0\n"
"RenderEvent 294 137 0 0 0 0 0\n"
"MouseMoveEvent 295 130 0 0 0 0 0\n"
"InteractionEvent 295 130 0 0 0 0 0\n"
"TimerEvent 295 130 0 0 0 0 0\n"
"RenderEvent 295 130 0 0 0 0 0\n"
"MouseMoveEvent 296 126 0 0 0 0 0\n"
"InteractionEvent 296 126 0 0 0 0 0\n"
"TimerEvent 296 126 0 0 0 0 0\n"
"RenderEvent 296 126 0 0 0 0 0\n"
"MouseMoveEvent 299 120 0 0 0 0 0\n"
"InteractionEvent 299 120 0 0 0 0 0\n"
"TimerEvent 299 120 0 0 0 0 0\n"
"RenderEvent 299 120 0 0 0 0 0\n"
"MouseMoveEvent 301 116 0 0 0 0 0\n"
"InteractionEvent 301 116 0 0 0 0 0\n"
"TimerEvent 301 116 0 0 0 0 0\n"
"RenderEvent 301 116 0 0 0 0 0\n"
"MouseMoveEvent 303 110 0 0 0 0 0\n"
"InteractionEvent 303 110 0 0 0 0 0\n"
"TimerEvent 303 110 0 0 0 0 0\n"
"RenderEvent 303 110 0 0 0 0 0\n"
"MouseMoveEvent 305 107 0 0 0 0 0\n"
"InteractionEvent 305 107 0 0 0 0 0\n"
"TimerEvent 305 107 0 0 0 0 0\n"
"RenderEvent 305 107 0 0 0 0 0\n"
"MouseMoveEvent 308 103 0 0 0 0 0\n"
"InteractionEvent 308 103 0 0 0 0 0\n"
"TimerEvent 308 103 0 0 0 0 0\n"
"RenderEvent 308 103 0 0 0 0 0\n"
"MouseMoveEvent 311 99 0 0 0 0 0\n"
"InteractionEvent 311 99 0 0 0 0 0\n"
"TimerEvent 311 99 0 0 0 0 0\n"
"RenderEvent 311 99 0 0 0 0 0\n"
"MouseMoveEvent 314 97 0 0 0 0 0\n"
"InteractionEvent 314 97 0 0 0 0 0\n"
"TimerEvent 314 97 0 0 0 0 0\n"
"RenderEvent 314 97 0 0 0 0 0\n"
"MouseMoveEvent 317 94 0 0 0 0 0\n"
"InteractionEvent 317 94 0 0 0 0 0\n"
"TimerEvent 317 94 0 0 0 0 0\n"
"RenderEvent 317 94 0 0 0 0 0\n"
"MouseMoveEvent 320 93 0 0 0 0 0\n"
"InteractionEvent 320 93 0 0 0 0 0\n"
"TimerEvent 320 93 0 0 0 0 0\n"
"RenderEvent 320 93 0 0 0 0 0\n"
"MouseMoveEvent 324 91 0 0 0 0 0\n"
"InteractionEvent 324 91 0 0 0 0 0\n"
"TimerEvent 324 91 0 0 0 0 0\n"
"RenderEvent 324 91 0 0 0 0 0\n"
"MouseMoveEvent 329 89 0 0 0 0 0\n"
"InteractionEvent 329 89 0 0 0 0 0\n"
"TimerEvent 329 89 0 0 0 0 0\n"
"RenderEvent 329 89 0 0 0 0 0\n"
"MouseMoveEvent 334 88 0 0 0 0 0\n"
"InteractionEvent 334 88 0 0 0 0 0\n"
"TimerEvent 334 88 0 0 0 0 0\n"
"RenderEvent 334 88 0 0 0 0 0\n"
"MouseMoveEvent 338 87 0 0 0 0 0\n"
"InteractionEvent 338 87 0 0 0 0 0\n"
"TimerEvent 338 87 0 0 0 0 0\n"
"RenderEvent 338 87 0 0 0 0 0\n"
"MouseMoveEvent 344 86 0 0 0 0 0\n"
"InteractionEvent 344 86 0 0 0 0 0\n"
"TimerEvent 344 86 0 0 0 0 0\n"
"RenderEvent 344 86 0 0 0 0 0\n"
"MouseMoveEvent 350 85 0 0 0 0 0\n"
"InteractionEvent 350 85 0 0 0 0 0\n"
"TimerEvent 350 85 0 0 0 0 0\n"
"RenderEvent 350 85 0 0 0 0 0\n"
"MouseMoveEvent 355 85 0 0 0 0 0\n"
"InteractionEvent 355 85 0 0 0 0 0\n"
"TimerEvent 355 85 0 0 0 0 0\n"
"RenderEvent 355 85 0 0 0 0 0\n"
"MouseMoveEvent 359 84 0 0 0 0 0\n"
"InteractionEvent 359 84 0 0 0 0 0\n"
"TimerEvent 359 84 0 0 0 0 0\n"
"RenderEvent 359 84 0 0 0 0 0\n"
"MouseMoveEvent 363 84 0 0 0 0 0\n"
"InteractionEvent 363 84 0 0 0 0 0\n"
"TimerEvent 363 84 0 0 0 0 0\n"
"RenderEvent 363 84 0 0 0 0 0\n"
"MouseMoveEvent 367 83 0 0 0 0 0\n"
"InteractionEvent 367 83 0 0 0 0 0\n"
"TimerEvent 367 83 0 0 0 0 0\n"
"RenderEvent 367 83 0 0 0 0 0\n"
"MouseMoveEvent 370 83 0 0 0 0 0\n"
"InteractionEvent 370 83 0 0 0 0 0\n"
"TimerEvent 370 83 0 0 0 0 0\n"
"RenderEvent 370 83 0 0 0 0 0\n"
"MouseMoveEvent 372 83 0 0 0 0 0\n"
"InteractionEvent 372 83 0 0 0 0 0\n"
"TimerEvent 372 83 0 0 0 0 0\n"
"RenderEvent 372 83 0 0 0 0 0\n"
"MouseMoveEvent 374 83 0 0 0 0 0\n"
"InteractionEvent 374 83 0 0 0 0 0\n"
"TimerEvent 374 83 0 0 0 0 0\n"
"RenderEvent 374 83 0 0 0 0 0\n"
"TimerEvent 374 83 0 0 0 0 0\n"
"RenderEvent 374 83 0 0 0 0 0\n"
"MouseMoveEvent 375 83 0 0 0 0 0\n"
"InteractionEvent 375 83 0 0 0 0 0\n"
"TimerEvent 375 83 0 0 0 0 0\n"
"RenderEvent 375 83 0 0 0 0 0\n"
"TimerEvent 375 83 0 0 0 0 0\n"
"RenderEvent 375 83 0 0 0 0 0\n"
"TimerEvent 375 83 0 0 0 0 0\n"
"RenderEvent 375 83 0 0 0 0 0\n"
"TimerEvent 375 83 0 0 0 0 0\n"
"RenderEvent 375 83 0 0 0 0 0\n"
"MouseMoveEvent 375 84 0 0 0 0 0\n"
"InteractionEvent 375 84 0 0 0 0 0\n"
"TimerEvent 375 84 0 0 0 0 0\n"
"RenderEvent 375 84 0 0 0 0 0\n"
"MouseMoveEvent 375 86 0 0 0 0 0\n"
"InteractionEvent 375 86 0 0 0 0 0\n"
"MouseMoveEvent 372 90 0 0 0 0 0\n"
"InteractionEvent 372 90 0 0 0 0 0\n"
"TimerEvent 372 90 0 0 0 0 0\n"
"RenderEvent 372 90 0 0 0 0 0\n"
"TimerEvent 372 90 0 0 0 0 0\n"
"RenderEvent 372 90 0 0 0 0 0\n"
"MouseMoveEvent 370 94 0 0 0 0 0\n"
"InteractionEvent 370 94 0 0 0 0 0\n"
"TimerEvent 370 94 0 0 0 0 0\n"
"RenderEvent 370 94 0 0 0 0 0\n"
"MouseMoveEvent 368 98 0 0 0 0 0\n"
"InteractionEvent 368 98 0 0 0 0 0\n"
"MouseMoveEvent 366 102 0 0 0 0 0\n"
"InteractionEvent 366 102 0 0 0 0 0\n"
"TimerEvent 366 102 0 0 0 0 0\n"
"RenderEvent 366 102 0 0 0 0 0\n"
"TimerEvent 366 102 0 0 0 0 0\n"
"RenderEvent 366 102 0 0 0 0 0\n"
"MouseMoveEvent 362 106 0 0 0 0 0\n"
"InteractionEvent 362 106 0 0 0 0 0\n"
"TimerEvent 362 106 0 0 0 0 0\n"
"RenderEvent 362 106 0 0 0 0 0\n"
"MouseMoveEvent 359 111 0 0 0 0 0\n"
"InteractionEvent 359 111 0 0 0 0 0\n"
"TimerEvent 359 111 0 0 0 0 0\n"
"RenderEvent 359 111 0 0 0 0 0\n"
"MouseMoveEvent 357 117 0 0 0 0 0\n"
"InteractionEvent 357 117 0 0 0 0 0\n"
"TimerEvent 357 117 0 0 0 0 0\n"
"RenderEvent 357 117 0 0 0 0 0\n"
"MouseMoveEvent 353 122 0 0 0 0 0\n"
"InteractionEvent 353 122 0 0 0 0 0\n"
"TimerEvent 353 122 0 0 0 0 0\n"
"RenderEvent 353 122 0 0 0 0 0\n"
"MouseMoveEvent 350 128 0 0 0 0 0\n"
"InteractionEvent 350 128 0 0 0 0 0\n"
"TimerEvent 350 128 0 0 0 0 0\n"
"RenderEvent 350 128 0 0 0 0 0\n"
"MouseMoveEvent 347 133 0 0 0 0 0\n"
"InteractionEvent 347 133 0 0 0 0 0\n"
"TimerEvent 347 133 0 0 0 0 0\n"
"RenderEvent 347 133 0 0 0 0 0\n"
"MouseMoveEvent 343 138 0 0 0 0 0\n"
"InteractionEvent 343 138 0 0 0 0 0\n"
"TimerEvent 343 138 0 0 0 0 0\n"
"RenderEvent 343 138 0 0 0 0 0\n"
"MouseMoveEvent 339 144 0 0 0 0 0\n"
"InteractionEvent 339 144 0 0 0 0 0\n"
"TimerEvent 339 144 0 0 0 0 0\n"
"RenderEvent 339 144 0 0 0 0 0\n"
"MouseMoveEvent 337 148 0 0 0 0 0\n"
"InteractionEvent 337 148 0 0 0 0 0\n"
"TimerEvent 337 148 0 0 0 0 0\n"
"RenderEvent 337 148 0 0 0 0 0\n"
"MouseMoveEvent 335 151 0 0 0 0 0\n"
"InteractionEvent 335 151 0 0 0 0 0\n"
"TimerEvent 335 151 0 0 0 0 0\n"
"RenderEvent 335 151 0 0 0 0 0\n"
"MouseMoveEvent 334 154 0 0 0 0 0\n"
"InteractionEvent 334 154 0 0 0 0 0\n"
"TimerEvent 334 154 0 0 0 0 0\n"
"RenderEvent 334 154 0 0 0 0 0\n"
"MouseMoveEvent 331 156 0 0 0 0 0\n"
"InteractionEvent 331 156 0 0 0 0 0\n"
"TimerEvent 331 156 0 0 0 0 0\n"
"RenderEvent 331 156 0 0 0 0 0\n"
"MouseMoveEvent 329 160 0 0 0 0 0\n"
"InteractionEvent 329 160 0 0 0 0 0\n"
"TimerEvent 329 160 0 0 0 0 0\n"
"RenderEvent 329 160 0 0 0 0 0\n"
"MouseMoveEvent 328 162 0 0 0 0 0\n"
"InteractionEvent 328 162 0 0 0 0 0\n"
"TimerEvent 328 162 0 0 0 0 0\n"
"RenderEvent 328 162 0 0 0 0 0\n"
"MouseMoveEvent 327 164 0 0 0 0 0\n"
"InteractionEvent 327 164 0 0 0 0 0\n"
"TimerEvent 327 164 0 0 0 0 0\n"
"RenderEvent 327 164 0 0 0 0 0\n"
"MouseMoveEvent 326 165 0 0 0 0 0\n"
"InteractionEvent 326 165 0 0 0 0 0\n"
"TimerEvent 326 165 0 0 0 0 0\n"
"RenderEvent 326 165 0 0 0 0 0\n"
"MouseMoveEvent 325 168 0 0 0 0 0\n"
"InteractionEvent 325 168 0 0 0 0 0\n"
"TimerEvent 325 168 0 0 0 0 0\n"
"RenderEvent 325 168 0 0 0 0 0\n"
"MouseMoveEvent 324 170 0 0 0 0 0\n"
"InteractionEvent 324 170 0 0 0 0 0\n"
"TimerEvent 324 170 0 0 0 0 0\n"
"RenderEvent 324 170 0 0 0 0 0\n"
"MouseMoveEvent 322 172 0 0 0 0 0\n"
"InteractionEvent 322 172 0 0 0 0 0\n"
"TimerEvent 322 172 0 0 0 0 0\n"
"RenderEvent 322 172 0 0 0 0 0\n"
"MouseMoveEvent 321 173 0 0 0 0 0\n"
"InteractionEvent 321 173 0 0 0 0 0\n"
"TimerEvent 321 173 0 0 0 0 0\n"
"RenderEvent 321 173 0 0 0 0 0\n"
"MouseMoveEvent 319 173 0 0 0 0 0\n"
"InteractionEvent 319 173 0 0 0 0 0\n"
"TimerEvent 319 173 0 0 0 0 0\n"
"RenderEvent 319 173 0 0 0 0 0\n"
"MouseMoveEvent 318 174 0 0 0 0 0\n"
"InteractionEvent 318 174 0 0 0 0 0\n"
"TimerEvent 318 174 0 0 0 0 0\n"
"RenderEvent 318 174 0 0 0 0 0\n"
"MouseMoveEvent 315 175 0 0 0 0 0\n"
"InteractionEvent 315 175 0 0 0 0 0\n"
"TimerEvent 315 175 0 0 0 0 0\n"
"RenderEvent 315 175 0 0 0 0 0\n"
"MouseMoveEvent 313 175 0 0 0 0 0\n"
"InteractionEvent 313 175 0 0 0 0 0\n"
"TimerEvent 313 175 0 0 0 0 0\n"
"RenderEvent 313 175 0 0 0 0 0\n"
"MouseMoveEvent 312 176 0 0 0 0 0\n"
"InteractionEvent 312 176 0 0 0 0 0\n"
"TimerEvent 312 176 0 0 0 0 0\n"
"RenderEvent 312 176 0 0 0 0 0\n"
"MouseMoveEvent 311 176 0 0 0 0 0\n"
"InteractionEvent 311 176 0 0 0 0 0\n"
"TimerEvent 311 176 0 0 0 0 0\n"
"RenderEvent 311 176 0 0 0 0 0\n"
"TimerEvent 311 176 0 0 0 0 0\n"
"RenderEvent 311 176 0 0 0 0 0\n"
"MouseMoveEvent 310 176 0 0 0 0 0\n"
"InteractionEvent 310 176 0 0 0 0 0\n"
"TimerEvent 310 176 0 0 0 0 0\n"
"RenderEvent 310 176 0 0 0 0 0\n"
"MouseMoveEvent 309 177 0 0 0 0 0\n"
"InteractionEvent 309 177 0 0 0 0 0\n"
"MouseMoveEvent 308 177 0 0 0 0 0\n"
"InteractionEvent 308 177 0 0 0 0 0\n"
"TimerEvent 308 177 0 0 0 0 0\n"
"RenderEvent 308 177 0 0 0 0 0\n"
"TimerEvent 308 177 0 0 0 0 0\n"
"RenderEvent 308 177 0 0 0 0 0\n"
"MouseMoveEvent 308 178 0 0 0 0 0\n"
"InteractionEvent 308 178 0 0 0 0 0\n"
"TimerEvent 308 178 0 0 0 0 0\n"
"RenderEvent 308 178 0 0 0 0 0\n"
"TimerEvent 308 178 0 0 0 0 0\n"
"RenderEvent 308 178 0 0 0 0 0\n"
"TimerEvent 308 178 0 0 0 0 0\n"
"RenderEvent 308 178 0 0 0 0 0\n"
"TimerEvent 308 178 0 0 0 0 0\n"
"RenderEvent 308 178 0 0 0 0 0\n"
"TimerEvent 308 178 0 0 0 0 0\n"
"RenderEvent 308 178 0 0 0 0 0\n"
"TimerEvent 308 178 0 0 0 0 0\n"
"RenderEvent 308 178 0 0 0 0 0\n"
"LeftButtonReleaseEvent 308 178 0 0 0 0 0\n"
"EndInteractionEvent 308 178 0 0 0 0 0\n"
"RenderEvent 308 178 0 0 0 0 0\n"
"MouseMoveEvent 327 178 0 0 0 0 0\n"
"MouseMoveEvent 327 178 0 0 0 0 0\n"
"MouseMoveEvent 374 158 0 0 0 0 0\n"
"MouseMoveEvent 374 158 0 0 0 0 0\n"
"MouseMoveEvent 439 127 0 0 0 0 0\n"
"MouseMoveEvent 439 127 0 0 0 0 0\n"
"LeaveEvent 516 89 0 0 0 0 0\n"
"EnterEvent 447 175 0 0 0 0 0\n"
"MouseMoveEvent 447 175 0 0 0 0 0\n"
"MouseMoveEvent 447 175 0 0 0 0 0\n"
"MouseMoveEvent 404 174 0 0 0 0 0\n"
"MouseMoveEvent 404 174 0 0 0 0 0\n"
"MouseMoveEvent 357 173 0 0 0 0 0\n"
"MouseMoveEvent 357 173 0 0 0 0 0\n"
"MouseMoveEvent 332 173 0 0 0 0 0\n"
"MouseMoveEvent 332 173 0 0 0 0 0\n"
"MouseMoveEvent 311 173 0 0 0 0 0\n"
"MouseMoveEvent 311 173 0 0 0 0 0\n"
"MouseMoveEvent 304 175 0 0 0 0 0\n"
"MouseMoveEvent 304 175 0 0 0 0 0\n"
"MouseMoveEvent 301 177 0 0 0 0 0\n"
"MouseMoveEvent 301 177 0 0 0 0 0\n"
"LeftButtonPressEvent 301 177 0 0 0 0 0\n"
"StartInteractionEvent 301 177 0 0 0 0 0\n"
"LeftButtonReleaseEvent 301 177 0 0 0 0 0\n"
"EndInteractionEvent 301 177 0 0 0 0 0\n"
"RenderEvent 301 177 0 0 0 0 0\n"
"MouseMoveEvent 300 178 0 0 0 0 0\n"
"MouseMoveEvent 300 178 0 0 0 0 0\n"
"MouseMoveEvent 299 179 0 0 0 0 0\n"
"MouseMoveEvent 299 179 0 0 0 0 0\n"
"MouseMoveEvent 292 186 0 0 0 0 0\n"
"MouseMoveEvent 292 186 0 0 0 0 0\n"
"MouseMoveEvent 286 195 0 0 0 0 0\n"
"MouseMoveEvent 286 195 0 0 0 0 0\n"
"MouseMoveEvent 281 202 0 0 0 0 0\n"
"MouseMoveEvent 281 202 0 0 0 0 0\n"
"MouseMoveEvent 277 206 0 0 0 0 0\n"
"MouseMoveEvent 277 206 0 0 0 0 0\n"
"MouseMoveEvent 273 212 0 0 0 0 0\n"
"MouseMoveEvent 273 212 0 0 0 0 0\n"
"MouseMoveEvent 270 218 0 0 0 0 0\n"
"MouseMoveEvent 270 218 0 0 0 0 0\n"
"LeftButtonPressEvent 270 218 0 0 0 0 0\n"
"StartInteractionEvent 270 218 0 0 0 0 0\n"
"MouseMoveEvent 269 219 0 0 0 0 0\n"
"InteractionEvent 269 219 0 0 0 0 0\n"
"TimerEvent 269 219 0 0 0 0 0\n"
"RenderEvent 269 219 0 0 0 0 0\n"
"TimerEvent 269 219 0 0 0 0 0\n"
"RenderEvent 269 219 0 0 0 0 0\n"
"MouseMoveEvent 269 220 0 0 0 0 0\n"
"InteractionEvent 269 220 0 0 0 0 0\n"
"TimerEvent 269 220 0 0 0 0 0\n"
"RenderEvent 269 220 0 0 0 0 0\n"
"TimerEvent 269 220 0 0 0 0 0\n"
"RenderEvent 269 220 0 0 0 0 0\n"
"TimerEvent 269 220 0 0 0 0 0\n"
"RenderEvent 269 220 0 0 0 0 0\n"
"TimerEvent 269 220 0 0 0 0 0\n"
"RenderEvent 269 220 0 0 0 0 0\n"
"TimerEvent 269 220 0 0 0 0 0\n"
"RenderEvent 269 220 0 0 0 0 0\n"
"TimerEvent 269 220 0 0 0 0 0\n"
"RenderEvent 269 220 0 0 0 0 0\n"
"TimerEvent 269 220 0 0 0 0 0\n"
"RenderEvent 269 220 0 0 0 0 0\n"
"TimerEvent 269 220 0 0 0 0 0\n"
"RenderEvent 269 220 0 0 0 0 0\n"
"TimerEvent 269 220 0 0 0 0 0\n"
"RenderEvent 269 220 0 0 0 0 0\n"
"TimerEvent 269 220 0 0 0 0 0\n"
"RenderEvent 269 220 0 0 0 0 0\n"
"MouseMoveEvent 269 219 0 0 0 0 0\n"
"InteractionEvent 269 219 0 0 0 0 0\n"
"TimerEvent 269 219 0 0 0 0 0\n"
"RenderEvent 269 219 0 0 0 0 0\n"
"MouseMoveEvent 269 213 0 0 0 0 0\n"
"InteractionEvent 269 213 0 0 0 0 0\n"
"TimerEvent 269 213 0 0 0 0 0\n"
"RenderEvent 269 213 0 0 0 0 0\n"
"MouseMoveEvent 269 202 0 0 0 0 0\n"
"InteractionEvent 269 202 0 0 0 0 0\n"
"TimerEvent 269 202 0 0 0 0 0\n"
"RenderEvent 269 202 0 0 0 0 0\n"
"MouseMoveEvent 269 189 0 0 0 0 0\n"
"InteractionEvent 269 189 0 0 0 0 0\n"
"TimerEvent 269 189 0 0 0 0 0\n"
"RenderEvent 269 189 0 0 0 0 0\n"
"MouseMoveEvent 269 175 0 0 0 0 0\n"
"InteractionEvent 269 175 0 0 0 0 0\n"
"TimerEvent 269 175 0 0 0 0 0\n"
"RenderEvent 269 175 0 0 0 0 0\n"
"MouseMoveEvent 269 167 0 0 0 0 0\n"
"InteractionEvent 269 167 0 0 0 0 0\n"
"TimerEvent 269 167 0 0 0 0 0\n"
"RenderEvent 269 167 0 0 0 0 0\n"
"MouseMoveEvent 269 160 0 0 0 0 0\n"
"InteractionEvent 269 160 0 0 0 0 0\n"
"TimerEvent 269 160 0 0 0 0 0\n"
"RenderEvent 269 160 0 0 0 0 0\n"
"MouseMoveEvent 269 154 0 0 0 0 0\n"
"InteractionEvent 269 154 0 0 0 0 0\n"
"TimerEvent 269 154 0 0 0 0 0\n"
"RenderEvent 269 154 0 0 0 0 0\n"
"MouseMoveEvent 269 152 0 0 0 0 0\n"
"InteractionEvent 269 152 0 0 0 0 0\n"
"TimerEvent 269 152 0 0 0 0 0\n"
"RenderEvent 269 152 0 0 0 0 0\n"
"MouseMoveEvent 269 150 0 0 0 0 0\n"
"InteractionEvent 269 150 0 0 0 0 0\n"
"TimerEvent 269 150 0 0 0 0 0\n"
"RenderEvent 269 150 0 0 0 0 0\n"
"MouseMoveEvent 270 149 0 0 0 0 0\n"
"InteractionEvent 270 149 0 0 0 0 0\n"
"TimerEvent 270 149 0 0 0 0 0\n"
"RenderEvent 270 149 0 0 0 0 0\n"
"MouseMoveEvent 270 148 0 0 0 0 0\n"
"InteractionEvent 270 148 0 0 0 0 0\n"
"TimerEvent 270 148 0 0 0 0 0\n"
"RenderEvent 270 148 0 0 0 0 0\n"
"TimerEvent 270 148 0 0 0 0 0\n"
"RenderEvent 270 148 0 0 0 0 0\n"
"MouseMoveEvent 270 148 0 0 0 0 0\n"
"InteractionEvent 270 148 0 0 0 0 0\n"
"TimerEvent 270 148 0 0 0 0 0\n"
"RenderEvent 270 148 0 0 0 0 0\n"
"MouseMoveEvent 271 146 0 0 0 0 0\n"
"InteractionEvent 271 146 0 0 0 0 0\n"
"MouseMoveEvent 272 144 0 0 0 0 0\n"
"InteractionEvent 272 144 0 0 0 0 0\n"
"TimerEvent 272 144 0 0 0 0 0\n"
"RenderEvent 272 144 0 0 0 0 0\n"
"TimerEvent 272 144 0 0 0 0 0\n"
"RenderEvent 272 144 0 0 0 0 0\n"
"MouseMoveEvent 273 142 0 0 0 0 0\n"
"InteractionEvent 273 142 0 0 0 0 0\n"
"TimerEvent 273 142 0 0 0 0 0\n"
"RenderEvent 273 142 0 0 0 0 0\n"
"MouseMoveEvent 275 140 0 0 0 0 0\n"
"InteractionEvent 275 140 0 0 0 0 0\n"
"MouseMoveEvent 277 138 0 0 0 0 0\n"
"InteractionEvent 277 138 0 0 0 0 0\n"
"TimerEvent 277 138 0 0 0 0 0\n"
"RenderEvent 277 138 0 0 0 0 0\n"
"TimerEvent 277 138 0 0 0 0 0\n"
"RenderEvent 277 138 0 0 0 0 0\n"
"TimerEvent 277 138 0 0 0 0 0\n"
"RenderEvent 277 138 0 0 0 0 0\n"
"TimerEvent 277 138 0 0 0 0 0\n"
"RenderEvent 277 138 0 0 0 0 0\n"
"TimerEvent 277 138 0 0 0 0 0\n"
"RenderEvent 277 138 0 0 0 0 0\n"
"LeftButtonReleaseEvent 277 138 0 0 0 0 0\n"
"EndInteractionEvent 277 138 0 0 0 0 0\n"
"RenderEvent 277 138 0 0 0 0 0\n"
"KeyPressEvent 277 138 0 0 57 1 9\n"
"CharEvent 277 138 0 0 57 1 9\n"
"KeyReleaseEvent 277 138 0 0 57 1 9\n"
"MouseMoveEvent 277 140 0 0 0 0 9\n"
"MouseMoveEvent 277 140 0 0 0 0 9\n"
"MouseMoveEvent 277 145 0 0 0 0 9\n"
;

//-----------------------------------------------------------------------------
class TestRemoveVolumeNonCurrentContextCallback: public vtkCommand
{
public:

  static TestRemoveVolumeNonCurrentContextCallback *New()
  {
    return new TestRemoveVolumeNonCurrentContextCallback;
  }

  void Execute(vtkObject* caller,
               unsigned long eventId,
               void* vtkNotUsed(callData)) VTK_OVERRIDE
  {
    if (eventId != vtkCommand::KeyPressEvent)
    {
      return;
    }

    vtkRenderWindowInteractor* interactor =
      static_cast<vtkRenderWindowInteractor*>(caller);
    if (interactor == NULL)
    {
      return;
    }

    char* pressedKey = interactor->GetKeySym();

    if (strcmp(pressedKey, "9") == 0)
    {
      renderer2->RemoveAllViewProps();
      renderWindow1->Render();
      renderWindow2->Render();
    }
  }

  vtkRenderer* renderer1;
  vtkRenderer* renderer2;
  vtkRenderWindow* renderWindow1;
  vtkRenderWindow* renderWindow2;
};

//-----------------------------------------------------------------------------
int TestRemoveVolumeNonCurrentContext(int argc, char* argv[])
{
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(
                            argc, argv, "Data/ironProt.vtk");
  vtkNew<vtkStructuredPointsReader> pointsReader;
  pointsReader->SetFileName(volumeFile);
  pointsReader->Update();
  delete[] volumeFile;

  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->AddRGBPoint( 0.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint( 64.0, 1.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(128.0, 0.0, 0.0, 1.0);
  colorTransferFunction->AddRGBPoint(192.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(255.0, 0.0, 0.2, 0.0);

  vtkNew<vtkPiecewiseFunction> opacityTransferFunction;
  opacityTransferFunction->AddPoint(0, 0.0);
  opacityTransferFunction->AddPoint(255, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetColor(colorTransferFunction.GetPointer());
  volumeProperty->SetScalarOpacity(opacityTransferFunction.GetPointer());

  // Create the first renderwindow/renderer/mapper.
  // This is the renderer that will experience problems later on.
  vtkNew<vtkSmartVolumeMapper> smartMapper1;
  smartMapper1->SetInputConnection(pointsReader->GetOutputPort());

  vtkNew<vtkVolume> volume1;
  volume1->SetMapper(smartMapper1.GetPointer());
  volume1->SetProperty(volumeProperty.GetPointer());

  vtkNew<vtkRenderer> renderer1;
  vtkNew<vtkRenderWindow> renderWindow1;
  vtkNew<vtkRenderWindowInteractor> interactor1;
  vtkNew<vtkInteractorStyleTrackballCamera> interactorStyle;
  interactor1->SetInteractorStyle(interactorStyle.GetPointer());

  renderWindow1->SetParentId(0);
  renderWindow1->AddRenderer(renderer1.GetPointer());
  renderWindow1->SetWindowName("Victim");
  renderWindow1->SetSize(500, 300);
  renderWindow1->SetPosition(100, 100);
  interactor1->SetRenderWindow(renderWindow1.GetPointer());

  renderer1->AddVolume(volume1.GetPointer());
  renderer1->SetBackground(1.0, 1.0, 1.0);

  // Create the second renderwindow/renderer/mapper.
  // This is the renderer we later remove all the actors from,
  // triggering the problems in the first renderer
  vtkNew<vtkSmartVolumeMapper> smartMapper2;
  smartMapper2->SetInputConnection(pointsReader->GetOutputPort());

  vtkNew<vtkVolume> volume2;
  volume2->SetMapper(smartMapper2.GetPointer());
  volume2->SetProperty(volumeProperty.GetPointer());

  vtkNew<vtkRenderer> renderer2;
  vtkNew<vtkRenderWindow> renderWindow2;
  vtkNew<vtkRenderWindowInteractor> interactor2;

  renderWindow2->SetParentId(0);
  renderWindow2->AddRenderer(renderer2.GetPointer());
  renderWindow2->SetWindowName("Villain");
  renderWindow2->SetSize(300, 300);
  renderWindow2->SetPosition(650, 100);
  interactor2->SetRenderWindow(renderWindow2.GetPointer());

  renderer2->AddVolume(volume2.GetPointer());
  renderer2->SetBackground(1.0, 1.0, 1.0);

  // Create callback so we can trigger the problem
  vtkNew<TestRemoveVolumeNonCurrentContextCallback> callback;
  callback->renderer1 = renderer1.GetPointer();
  callback->renderer2	= renderer2.GetPointer();
  callback->renderWindow1 = renderWindow1.GetPointer();
  callback->renderWindow2 = renderWindow2.GetPointer();
  interactor1->AddObserver("KeyPressEvent", callback.GetPointer());

  // Let's go
  interactor1->Initialize();
  renderWindow1->Render();
  renderWindow2->Render();
  renderWindow1->MakeCurrent();
//  interactor1->SetKeyEventInformation(0, 0, 0, 0, "9");
//  interactor1->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
//  int retval = vtkTesting::Test(argc, argv, renderWindow1.GetPointer(), 15);
//  if (retval == vtkRegressionTester::DO_INTERACTOR)
//    {
//    interactor1->Start();
//    }
//  return !retval;
  return vtkTesting::InteractorEventLoop(argc, argv,
                                         interactor1.GetPointer(),
                                         TestRemoveVolumeNonCurrentContextLog);
}
