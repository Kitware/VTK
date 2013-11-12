/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPickingManagerSeedWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*==============================================================================

  Library: MSVTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

//
// This example tests the PickingManager using a scene full of seed widgets.
// It measures the performances using the Picking manager into differents mode:
// * Disabled
// * Enabled
// * With/Without cache optimization
//
// The test depends on:
// * vtkSeedWidget
// * vtkSphereHandleRepresentation
//
// By default the Picking Manager is enabled.
// Press 'Ctrl' to switch the activation of the Picking Manager.
// Press 'o' to enable/disable the Optimization on render events.
// Press 'Space' to restore the cube

// VTK includes
#include "vtkCommand.h"
#include "vtkHandleWidget.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPickingManager.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSeedRepresentation.h"
#include "vtkSeedWidget.h"
#include "vtkSmartPointer.h"
#include "vtkSphereHandleRepresentation.h"
#include "vtkStdString.h"
#include "vtkTimerLog.h"

// STL includes
#include <iostream>
#include <fstream>
#include <list>

const char eventLogTestPickingManagerSeedWidget[] =
  "# StreamVersion 1 \n"
  "EnterEvent 570 160 0 0 0 0 0 i\n"
  "MouseMoveEvent 570 160 0 0 0 0 0 i\n"
  "MouseMoveEvent 473 164 0 0 0 0 0 i\n"
  "MouseMoveEvent 442 166 0 0 0 0 0 i\n"
  "MouseMoveEvent 415 168 0 0 0 0 0 i\n"
  "MouseMoveEvent 398 171 0 0 0 0 0 i\n"
  "MouseMoveEvent 376 173 0 0 0 0 0 i\n"
  "MouseMoveEvent 372 173 0 0 0 0 0 i\n"
  "MouseMoveEvent 371 173 0 0 0 0 0 i\n"
  "MouseMoveEvent 370 173 0 0 0 0 0 i\n"
  "MouseMoveEvent 370 171 0 0 0 0 0 i\n"
  "MouseMoveEvent 369 169 0 0 0 0 0 i\n"
  "MouseMoveEvent 368 169 0 0 0 0 0 i\n"
  "MouseMoveEvent 368 168 0 0 0 0 0 i\n"
  "MouseMoveEvent 368 167 0 0 0 0 0 i\n"
  "MouseMoveEvent 367 167 0 0 0 0 0 i\n"
  "MouseMoveEvent 367 166 0 0 0 0 0 i\n"
  "MouseMoveEvent 366 165 0 0 0 0 0 i\n"
  "MouseMoveEvent 366 164 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 366 164 0 0 0 0 0 i\n"
  "StartInteractionEvent 366 164 0 0 0 0 0 i\n"
  "MouseMoveEvent 365 165 0 0 0 0 0 i\n"
  "MouseMoveEvent 327 185 0 0 0 0 0 i\n"
  "MouseMoveEvent 321 187 0 0 0 0 0 i\n"
  "MouseMoveEvent 319 189 0 0 0 0 0 i\n"
  "MouseMoveEvent 318 190 0 0 0 0 0 i\n"
  "MouseMoveEvent 316 190 0 0 0 0 0 i\n"
  "MouseMoveEvent 314 190 0 0 0 0 0 i\n"
  "MouseMoveEvent 313 191 0 0 0 0 0 i\n"
  "MouseMoveEvent 312 191 0 0 0 0 0 i\n"
  "MouseMoveEvent 311 191 0 0 0 0 0 i\n"
  "MouseMoveEvent 310 191 0 0 0 0 0 i\n"
  "MouseMoveEvent 309 191 0 0 0 0 0 i\n"
  "MouseMoveEvent 308 191 0 0 0 0 0 i\n"
  "MouseMoveEvent 307 191 0 0 0 0 0 i\n"
  "MouseMoveEvent 306 190 0 0 0 0 0 i\n"
  "MouseMoveEvent 305 190 0 0 0 0 0 i\n"
  "MouseMoveEvent 303 190 0 0 0 0 0 i\n"
  "MouseMoveEvent 302 190 0 0 0 0 0 i\n"
  "MouseMoveEvent 301 190 0 0 0 0 0 i\n"
  "MouseMoveEvent 300 189 0 0 0 0 0 i\n"
  "MouseMoveEvent 299 189 0 0 0 0 0 i\n"
  "MouseMoveEvent 298 189 0 0 0 0 0 i\n"
  "MouseMoveEvent 296 188 0 0 0 0 0 i\n"
  "MouseMoveEvent 295 188 0 0 0 0 0 i\n"
  "MouseMoveEvent 294 188 0 0 0 0 0 i\n"
  "MouseMoveEvent 293 188 0 0 0 0 0 i\n"
  "MouseMoveEvent 291 188 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 291 188 0 0 0 0 0 i\n"
  "EndInteractionEvent 291 188 0 0 0 0 0 i\n"
  "MouseMoveEvent 308 205 0 0 0 0 0 i\n"
  "MouseMoveEvent 363 268 0 0 0 0 0 i\n"
  "MouseMoveEvent 376 316 0 0 0 0 0 i\n"
  "MouseMoveEvent 383 343 0 0 0 0 0 i\n"
  "MouseMoveEvent 392 375 0 0 0 0 0 i\n"
  "MouseMoveEvent 394 386 0 0 0 0 0 i\n"
  "MouseMoveEvent 396 399 0 0 0 0 0 i\n"
  "MouseMoveEvent 398 421 0 0 0 0 0 i\n"
  "MouseMoveEvent 398 430 0 0 0 0 0 i\n"
  "MouseMoveEvent 398 437 0 0 0 0 0 i\n"
  "MouseMoveEvent 396 441 0 0 0 0 0 i\n"
  "MouseMoveEvent 396 445 0 0 0 0 0 i\n"
  "MouseMoveEvent 395 445 0 0 0 0 0 i\n"
  "MouseMoveEvent 395 446 0 0 0 0 0 i\n"
  "MouseMoveEvent 395 445 0 0 0 0 0 i\n"
  "MouseMoveEvent 395 443 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 395 443 0 0 0 0 0 i\n"
  "StartInteractionEvent 395 443 0 0 0 0 0 i\n"
  "MouseMoveEvent 395 442 0 0 0 0 0 i\n"
  "MouseMoveEvent 393 435 0 0 0 0 0 i\n"
  "MouseMoveEvent 392 433 0 0 0 0 0 i\n"
  "MouseMoveEvent 392 432 0 0 0 0 0 i\n"
  "MouseMoveEvent 390 430 0 0 0 0 0 i\n"
  "MouseMoveEvent 390 429 0 0 0 0 0 i\n"
  "MouseMoveEvent 389 427 0 0 0 0 0 i\n"
  "MouseMoveEvent 389 426 0 0 0 0 0 i\n"
  "MouseMoveEvent 388 426 0 0 0 0 0 i\n"
  "MouseMoveEvent 387 425 0 0 0 0 0 i\n"
  "MouseMoveEvent 387 424 0 0 0 0 0 i\n"
  "MouseMoveEvent 386 423 0 0 0 0 0 i\n"
  "MouseMoveEvent 385 421 0 0 0 0 0 i\n"
  "MouseMoveEvent 384 420 0 0 0 0 0 i\n"
  "MouseMoveEvent 384 419 0 0 0 0 0 i\n"
  "MouseMoveEvent 384 418 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 384 418 0 0 0 0 0 i\n"
  "EndInteractionEvent 384 418 0 0 0 0 0 i\n"
  "MouseMoveEvent 380 400 0 0 0 0 0 i\n"
  "MouseMoveEvent 368 362 0 0 0 0 0 i\n"
  "MouseMoveEvent 350 314 0 0 0 0 0 i\n"
  "MouseMoveEvent 337 295 0 0 0 0 0 i\n"
  "MouseMoveEvent 315 265 0 0 0 0 0 i\n"
  "MouseMoveEvent 301 251 0 0 0 0 0 i\n"
  "MouseMoveEvent 296 246 0 0 0 0 0 i\n"
  "MouseMoveEvent 293 242 0 0 0 0 0 i\n"
  "MouseMoveEvent 292 241 0 0 0 0 0 i\n"
  "MouseMoveEvent 291 240 0 0 0 0 0 i\n"
  "MouseMoveEvent 291 239 0 0 0 0 0 i\n"
  "MouseMoveEvent 292 239 0 0 0 0 0 i\n"
  "MouseMoveEvent 293 239 0 0 0 0 0 i\n"
  "MouseMoveEvent 296 242 0 0 0 0 0 i\n"
  "MouseMoveEvent 297 244 0 0 0 0 0 i\n"
  "MouseMoveEvent 297 248 0 0 0 0 0 i\n"
  "MouseMoveEvent 297 249 0 0 0 0 0 i\n"
  "MouseMoveEvent 297 251 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 297 251 0 0 0 0 0 i\n"
  "MouseMoveEvent 297 252 0 0 0 0 0 i\n"
  "MouseMoveEvent 298 253 0 0 0 0 0 i\n"
  "MouseMoveEvent 301 259 0 0 0 0 0 i\n"
  "MouseMoveEvent 301 263 0 0 0 0 0 i\n"
  "MouseMoveEvent 302 265 0 0 0 0 0 i\n"
  "MouseMoveEvent 305 278 0 0 0 0 0 i\n"
  "MouseMoveEvent 307 285 0 0 0 0 0 i\n"
  "MouseMoveEvent 311 301 0 0 0 0 0 i\n"
  "MouseMoveEvent 314 311 0 0 0 0 0 i\n"
  "MouseMoveEvent 316 320 0 0 0 0 0 i\n"
  "MouseMoveEvent 321 336 0 0 0 0 0 i\n"
  "MouseMoveEvent 327 353 0 0 0 0 0 i\n"
  "MouseMoveEvent 330 365 0 0 0 0 0 i\n"
  "MouseMoveEvent 332 371 0 0 0 0 0 i\n"
  "MouseMoveEvent 332 376 0 0 0 0 0 i\n"
  "MouseMoveEvent 335 381 0 0 0 0 0 i\n"
  "MouseMoveEvent 335 382 0 0 0 0 0 i\n"
  "MouseMoveEvent 336 384 0 0 0 0 0 i\n"
  "MouseMoveEvent 337 386 0 0 0 0 0 i\n"
  "MouseMoveEvent 337 389 0 0 0 0 0 i\n"
  "MouseMoveEvent 337 391 0 0 0 0 0 i\n"
  "MouseMoveEvent 338 395 0 0 0 0 0 i\n"
  "MouseMoveEvent 338 397 0 0 0 0 0 i\n"
  "MouseMoveEvent 339 399 0 0 0 0 0 i\n"
  "MouseMoveEvent 339 401 0 0 0 0 0 i\n"
  "MouseMoveEvent 339 402 0 0 0 0 0 i\n"
  "MouseMoveEvent 338 401 0 0 0 0 0 i\n"
  "MouseMoveEvent 336 401 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 336 401 0 0 0 0 0 i\n"
  "MouseMoveEvent 333 395 0 0 0 0 0 i\n"
  "MouseMoveEvent 332 392 0 0 0 0 0 i\n"
  "MouseMoveEvent 330 383 0 0 0 0 0 i\n"
  "MouseMoveEvent 328 377 0 0 0 0 0 i\n"
  "MouseMoveEvent 325 361 0 0 0 0 0 i\n"
  "MouseMoveEvent 323 353 0 0 0 0 0 i\n"
  "MouseMoveEvent 322 346 0 0 0 0 0 i\n"
  "MouseMoveEvent 319 335 0 0 0 0 0 i\n"
  "MouseMoveEvent 318 325 0 0 0 0 0 i\n"
  "MouseMoveEvent 318 317 0 0 0 0 0 i\n"
  "MouseMoveEvent 317 315 0 0 0 0 0 i\n"
  "MouseMoveEvent 317 311 0 0 0 0 0 i\n"
  "MouseMoveEvent 317 308 0 0 0 0 0 i\n"
  "MouseMoveEvent 317 307 0 0 0 0 0 i\n"
  "MouseMoveEvent 317 306 0 0 0 0 0 i\n"
  "MouseMoveEvent 317 305 0 0 0 0 0 i\n"
  "MouseMoveEvent 317 304 0 0 0 0 0 i\n"
  "MouseMoveEvent 317 302 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 317 302 0 0 0 0 0 i\n"
  "MouseMoveEvent 316 302 0 0 0 0 0 i\n"
  "MouseMoveEvent 314 302 0 0 0 0 0 i\n"
  "MouseMoveEvent 310 303 0 0 0 0 0 i\n"
  "MouseMoveEvent 289 308 0 0 0 0 0 i\n"
  "MouseMoveEvent 271 316 0 0 0 0 0 i\n"
  "MouseMoveEvent 243 324 0 0 0 0 0 i\n"
  "MouseMoveEvent 221 334 0 0 0 0 0 i\n"
  "MouseMoveEvent 211 341 0 0 0 0 0 i\n"
  "MouseMoveEvent 205 345 0 0 0 0 0 i\n"
  "MouseMoveEvent 201 348 0 0 0 0 0 i\n"
  "MouseMoveEvent 196 352 0 0 0 0 0 i\n"
  "MouseMoveEvent 192 356 0 0 0 0 0 i\n"
  "MouseMoveEvent 189 357 0 0 0 0 0 i\n"
  "MouseMoveEvent 189 358 0 0 0 0 0 i\n"
  "MouseMoveEvent 188 359 0 0 0 0 0 i\n"
  "MouseMoveEvent 188 360 0 0 0 0 0 i\n"
  "MouseMoveEvent 188 361 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 188 361 0 0 0 0 0 i\n"
  "MouseMoveEvent 188 360 0 0 0 0 0 i\n"
  "MouseMoveEvent 190 355 0 0 0 0 0 i\n"
  "MouseMoveEvent 192 350 0 0 0 0 0 i\n"
  "MouseMoveEvent 194 348 0 0 0 0 0 i\n"
  "MouseMoveEvent 198 345 0 0 0 0 0 i\n"
  "MouseMoveEvent 204 340 0 0 0 0 0 i\n"
  "MouseMoveEvent 207 337 0 0 0 0 0 i\n"
  "MouseMoveEvent 217 336 0 0 0 0 0 i\n"
  "MouseMoveEvent 224 328 0 0 0 0 0 i\n"
  "MouseMoveEvent 226 326 0 0 0 0 0 i\n"
  "MouseMoveEvent 228 324 0 0 0 0 0 i\n"
  "MouseMoveEvent 231 321 0 0 0 0 0 i\n"
  "MouseMoveEvent 232 320 0 0 0 0 0 i\n"
  "MouseMoveEvent 235 317 0 0 0 0 0 i\n"
  "MouseMoveEvent 237 315 0 0 0 0 0 i\n"
  "MouseMoveEvent 240 309 0 0 0 0 0 i\n"
  "MouseMoveEvent 244 303 0 0 0 0 0 i\n"
  "MouseMoveEvent 248 299 0 0 0 0 0 i\n"
  "MouseMoveEvent 249 298 0 0 0 0 0 i\n"
  "MouseMoveEvent 252 296 0 0 0 0 0 i\n"
  "MouseMoveEvent 252 295 0 0 0 0 0 i\n"
  "MouseMoveEvent 254 295 0 0 0 0 0 i\n"
  "MouseMoveEvent 255 294 0 0 0 0 0 i\n"
  "MouseMoveEvent 256 294 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 256 294 0 0 0 0 0 i\n"
  "MouseMoveEvent 254 294 0 0 0 0 0 i\n"
  "MouseMoveEvent 252 294 0 0 0 0 0 i\n"
  "MouseMoveEvent 250 293 0 0 0 0 0 i\n"
  "MouseMoveEvent 246 293 0 0 0 0 0 i\n"
  "MouseMoveEvent 244 292 0 0 0 0 0 i\n"
  "MouseMoveEvent 228 292 0 0 0 0 0 i\n"
  "MouseMoveEvent 218 292 0 0 0 0 0 i\n"
  "MouseMoveEvent 185 292 0 0 0 0 0 i\n"
  "MouseMoveEvent 179 292 0 0 0 0 0 i\n"
  "MouseMoveEvent 160 292 0 0 0 0 0 i\n"
  "MouseMoveEvent 155 292 0 0 0 0 0 i\n"
  "MouseMoveEvent 152 292 0 0 0 0 0 i\n"
  "MouseMoveEvent 146 292 0 0 0 0 0 i\n"
  "MouseMoveEvent 142 294 0 0 0 0 0 i\n"
  "MouseMoveEvent 140 294 0 0 0 0 0 i\n"
  "MouseMoveEvent 139 294 0 0 0 0 0 i\n"
  "MouseMoveEvent 138 294 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 138 294 0 0 0 0 0 i\n"
  "MouseMoveEvent 139 294 0 0 0 0 0 i\n"
  "MouseMoveEvent 141 294 0 0 0 0 0 i\n"
  "MouseMoveEvent 142 294 0 0 0 0 0 i\n"
  "MouseMoveEvent 147 291 0 0 0 0 0 i\n"
  "MouseMoveEvent 161 287 0 0 0 0 0 i\n"
  "MouseMoveEvent 167 284 0 0 0 0 0 i\n"
  "MouseMoveEvent 171 283 0 0 0 0 0 i\n"
  "MouseMoveEvent 176 280 0 0 0 0 0 i\n"
  "MouseMoveEvent 197 277 0 0 0 0 0 i\n"
  "MouseMoveEvent 207 275 0 0 0 0 0 i\n"
  "MouseMoveEvent 222 271 0 0 0 0 0 i\n"
  "MouseMoveEvent 258 263 0 0 0 0 0 i\n"
  "MouseMoveEvent 289 255 0 0 0 0 0 i\n"
  "MouseMoveEvent 300 251 0 0 0 0 0 i\n"
  "MouseMoveEvent 320 241 0 0 0 0 0 i\n"
  "MouseMoveEvent 325 239 0 0 0 0 0 i\n"
  "MouseMoveEvent 327 238 0 0 0 0 0 i\n"
  "MouseMoveEvent 327 236 0 0 0 0 0 i\n"
  "MouseMoveEvent 327 233 0 0 0 0 0 i\n"
  "MouseMoveEvent 327 232 0 0 0 0 0 i\n"
  "MouseMoveEvent 327 231 0 0 0 0 0 i\n"
  "MouseMoveEvent 327 229 0 0 0 0 0 i\n"
  "MouseMoveEvent 326 229 0 0 0 0 0 i\n"
  "MouseMoveEvent 326 231 0 0 0 0 0 i\n"
  "MouseMoveEvent 328 233 0 0 0 0 0 i\n"
  "MouseMoveEvent 329 233 0 0 0 0 0 i\n"
  "MouseMoveEvent 333 233 0 0 0 0 0 i\n"
  "MouseMoveEvent 334 232 0 0 0 0 0 i\n"
  "MouseMoveEvent 335 227 0 0 0 0 0 i\n"
  "MouseMoveEvent 336 224 0 0 0 0 0 i\n"
  "MouseMoveEvent 338 210 0 0 0 0 0 i\n"
  "MouseMoveEvent 338 208 0 0 0 0 0 i\n"
  "MouseMoveEvent 337 200 0 0 0 0 0 i\n"
  "MouseMoveEvent 334 194 0 0 0 0 0 i\n"
  "MouseMoveEvent 333 193 0 0 0 0 0 i\n"
  "MouseMoveEvent 333 192 0 0 0 0 0 i\n"
  "MouseMoveEvent 332 191 0 0 0 0 0 i\n"
  "MouseMoveEvent 332 190 0 0 0 0 0 i\n"
  "MiddleButtonPressEvent 332 190 0 0 0 0 0 i\n"
  "StartInteractionEvent 332 190 0 0 0 0 0 i\n"
  "MouseMoveEvent 332 191 0 0 0 0 0 i\n"
  "MouseMoveEvent 332 213 0 0 0 0 0 i\n"
  "MouseMoveEvent 332 214 0 0 0 0 0 i\n"
  "MouseMoveEvent 332 215 0 0 0 0 0 i\n"
  "MiddleButtonReleaseEvent 332 215 0 0 0 0 0 i\n"
  "EndInteractionEvent 332 215 0 0 0 0 0 i\n"
  "MouseMoveEvent 348 216 0 0 0 0 0 i\n"
  "MouseMoveEvent 351 216 0 0 0 0 0 i\n"
  "MouseMoveEvent 353 216 0 0 0 0 0 i\n"
  "MouseMoveEvent 357 218 0 0 0 0 0 i\n"
  "MouseMoveEvent 359 220 0 0 0 0 0 i\n"
  "MouseMoveEvent 361 222 0 0 0 0 0 i\n"
  "MouseMoveEvent 363 224 0 0 0 0 0 i\n"
  "MouseMoveEvent 365 226 0 0 0 0 0 i\n"
  "MouseMoveEvent 369 232 0 0 0 0 0 i\n"
  "MouseMoveEvent 372 238 0 0 0 0 0 i\n"
  "MouseMoveEvent 372 239 0 0 0 0 0 i\n"
  "MouseMoveEvent 372 238 0 0 0 0 0 i\n"
  "MouseMoveEvent 371 235 0 0 0 0 0 i\n"
  "RightButtonPressEvent 372 236 0 0 0 0 0 i\n"
  "StartInteractionEvent 372 236 0 0 0 0 0 i\n"
  "MouseMoveEvent 371 234 0 0 0 0 0 i\n"
  "MouseMoveEvent 368 226 0 0 0 0 0 i\n"
  "MouseMoveEvent 367 225 0 0 0 0 0 i\n"
  "MouseMoveEvent 367 225 0 0 0 0 0 i\n"
  "MouseMoveEvent 366 224 0 0 0 0 0 i\n"
  "MouseMoveEvent 366 223 0 0 0 0 0 i\n"
  "MouseMoveEvent 365 222 0 0 0 0 0 i\n"
  "MouseMoveEvent 365 221 0 0 0 0 0 i\n"
  "MouseMoveEvent 365 220 0 0 0 0 0 i\n"
  "MouseMoveEvent 365 219 0 0 0 0 0 i\n"
  "MouseMoveEvent 365 218 0 0 0 0 0 i\n"
  "MouseMoveEvent 365 217 0 0 0 0 0 i\n"
  "MouseMoveEvent 364 217 0 0 0 0 0 i\n"
  "MouseMoveEvent 364 216 0 0 0 0 0 i\n"
  "MouseMoveEvent 364 215 0 0 0 0 0 i\n"
  "MouseMoveEvent 364 214 0 0 0 0 0 i\n"
  "MouseMoveEvent 364 213 0 0 0 0 0 i\n"
  "MouseMoveEvent 364 212 0 0 0 0 0 i\n"
  "MouseMoveEvent 364 211 0 0 0 0 0 i\n"
  "MouseMoveEvent 364 210 0 0 0 0 0 i\n"
  "MouseMoveEvent 364 209 0 0 0 0 0 i\n"
  "RightButtonReleaseEvent 364 209 0 0 0 0 0 i\n"
  "EndInteractionEvent 364 209 0 0 0 0 0 i\n"
  "MouseMoveEvent 363 209 0 0 0 0 0 i\n"
  "MouseMoveEvent 361 206 0 0 0 0 0 i\n"
  "MouseMoveEvent 359 204 0 0 0 0 0 i\n"
  "MouseMoveEvent 357 203 0 0 0 0 0 i\n"
  "MouseMoveEvent 354 199 0 0 0 0 0 i\n"
  "MouseMoveEvent 352 195 0 0 0 0 0 i\n"
  "MouseMoveEvent 350 193 0 0 0 0 0 i\n"
  "MouseMoveEvent 348 191 0 0 0 0 0 i\n"
  "MouseMoveEvent 346 188 0 0 0 0 0 i\n"
  "MouseMoveEvent 345 188 0 0 0 0 0 i\n"
  "LeftButtonPressEvent 345 188 0 0 0 0 0 i\n"
  "StartInteractionEvent 345 188 0 0 0 0 0 i\n"
  "MouseMoveEvent 344 187 0 0 0 0 0 i\n"
  "MouseMoveEvent 338 186 0 0 0 0 0 i\n"
  "MouseMoveEvent 337 185 0 0 0 0 0 i\n"
  "MouseMoveEvent 335 184 0 0 0 0 0 i\n"
  "MouseMoveEvent 334 183 0 0 0 0 0 i\n"
  "MouseMoveEvent 332 182 0 0 0 0 0 i\n"
  "MouseMoveEvent 331 182 0 0 0 0 0 i\n"
  "MouseMoveEvent 329 181 0 0 0 0 0 i\n"
  "MouseMoveEvent 327 180 0 0 0 0 0 i\n"
  "MouseMoveEvent 326 179 0 0 0 0 0 i\n"
  "MouseMoveEvent 325 179 0 0 0 0 0 i\n"
  "MouseMoveEvent 324 180 0 0 0 0 0 i\n"
  "MouseMoveEvent 324 179 0 0 0 0 0 i\n"
  "MouseMoveEvent 323 179 0 0 0 0 0 i\n"
  "MouseMoveEvent 323 178 0 0 0 0 0 i\n"
  "MouseMoveEvent 322 178 0 0 0 0 0 i\n"
  "MouseMoveEvent 321 177 0 0 0 0 0 i\n"
  "MouseMoveEvent 319 177 0 0 0 0 0 i\n"
  "MouseMoveEvent 319 176 0 0 0 0 0 i\n"
  "MouseMoveEvent 317 175 0 0 0 0 0 i\n"
  "MouseMoveEvent 316 174 0 0 0 0 0 i\n"
  "MouseMoveEvent 315 172 0 0 0 0 0 i\n"
  "MouseMoveEvent 314 171 0 0 0 0 0 i\n"
  "MouseMoveEvent 312 170 0 0 0 0 0 i\n"
  "MouseMoveEvent 311 168 0 0 0 0 0 i\n"
  "MouseMoveEvent 310 166 0 0 0 0 0 i\n"
  "MouseMoveEvent 309 164 0 0 0 0 0 i\n"
  "MouseMoveEvent 309 163 0 0 0 0 0 i\n"
  "MouseMoveEvent 307 161 0 0 0 0 0 i\n"
  "MouseMoveEvent 306 160 0 0 0 0 0 i\n"
  "MouseMoveEvent 305 158 0 0 0 0 0 i\n"
  "MouseMoveEvent 305 157 0 0 0 0 0 i\n"
  "MouseMoveEvent 304 155 0 0 0 0 0 i\n"
  "MouseMoveEvent 303 154 0 0 0 0 0 i\n"
  "MouseMoveEvent 302 152 0 0 0 0 0 i\n"
  "MouseMoveEvent 301 150 0 0 0 0 0 i\n"
  "MouseMoveEvent 300 149 0 0 0 0 0 i\n"
  "MouseMoveEvent 299 148 0 0 0 0 0 i\n"
  "MouseMoveEvent 298 147 0 0 0 0 0 i\n"
  "MouseMoveEvent 296 145 0 0 0 0 0 i\n"
  "MouseMoveEvent 295 144 0 0 0 0 0 i\n"
  "MouseMoveEvent 293 142 0 0 0 0 0 i\n"
  "MouseMoveEvent 291 140 0 0 0 0 0 i\n"
  "MouseMoveEvent 290 139 0 0 0 0 0 i\n"
  "MouseMoveEvent 288 138 0 0 0 0 0 i\n"
  "MouseMoveEvent 286 136 0 0 0 0 0 i\n"
  "MouseMoveEvent 286 135 0 0 0 0 0 i\n"
  "MouseMoveEvent 285 135 0 0 0 0 0 i\n"
  "MouseMoveEvent 284 134 0 0 0 0 0 i\n"
  "MouseMoveEvent 283 134 0 0 0 0 0 i\n"
  "MouseMoveEvent 283 133 0 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 283 133 0 0 0 0 0 i\n"
  "EndInteractionEvent 283 133 0 0 0 0 0 i\n"
  "MouseMoveEvent 284 134 0 0 0 0 0 i\n"
  "MouseMoveEvent 287 135 0 0 0 0 0 i\n"
  "MouseMoveEvent 289 135 0 0 0 0 0 i\n"
  "MouseMoveEvent 291 137 0 0 0 0 0 i\n"
  "MouseMoveEvent 296 140 0 0 0 0 0 i\n"
  "MouseMoveEvent 304 148 0 0 0 0 0 i\n"
  "MouseMoveEvent 310 150 0 0 0 0 0 i\n"
  "MouseMoveEvent 314 153 0 0 0 0 0 i\n"
  "MouseMoveEvent 318 157 0 0 0 0 0 i\n"
  "MouseMoveEvent 325 166 0 0 0 0 0 i\n"
  "MouseMoveEvent 330 173 0 0 0 0 0 i\n"
  "MouseMoveEvent 341 187 0 0 0 0 0 i\n"
  "MouseMoveEvent 345 195 0 0 0 0 0 i\n"
  "MouseMoveEvent 353 212 0 0 0 0 0 i\n"
  "KeyPressEvent 353 212 0 0 32 1 space i\n"
  "CharEvent 353 212 0 0 32 1 space i\n"
  "KeyReleaseEvent 420 372 0 0 32 1 space i\n"
  "MouseMoveEvent 284 134 0 0 0 0 0 i\n"
  ;

//------------------------------------------------------------------------------
// Press 'Ctrl' to switch the activation of the Picking Manager.
// Press 'o' to switch the activation of the optimization based on the render
// events.
class vtkPickingManagerCallback : public vtkCommand
{
public:
  static vtkPickingManagerCallback *New()
    {return new vtkPickingManagerCallback;}

  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
    vtkRenderWindowInteractor *iren =
      static_cast<vtkRenderWindowInteractor*>(caller);

    // Enable/Disable the PickingManager
    if((vtkStdString(iren->GetKeySym()) == "Control_L" ||
       vtkStdString(iren->GetKeySym()) == "Control_R") &&
       iren->GetPickingManager())
      {
      if(!iren->GetPickingManager()->GetEnabled())
        {
        std::cout << "PickingManager ON !" << std::endl;
        iren->GetPickingManager()->EnabledOn();
        }
      else
        {
        std::cout << "PickingManager OFF !" << std::endl;
        iren->GetPickingManager()->EnabledOff();
        }
      }
    // Enable/Disable the Optimization on render events.
    else if (vtkStdString(iren->GetKeySym()) == "o" &&
             iren->GetPickingManager())
      {
      if(!iren->GetPickingManager()->GetOptimizeOnInteractorEvents())
        {
        std::cout << "Optimization on Interactor events ON !" << std::endl;
        iren->GetPickingManager()->SetOptimizeOnInteractorEvents(1);
        }
      else
        {
        std::cout << "Optimization on Interactor events OFF !" << std::endl;
        iren->GetPickingManager()->SetOptimizeOnInteractorEvents(0);
        }
      }
    }

  vtkPickingManagerCallback() {}
};

//------------------------------------------------------------------------------
// Press 'Space' to reorganize the cube of seeds
class vtkPMSCubeCallback : public vtkCommand
{
public:
  static vtkPMSCubeCallback *New()
  { return new vtkPMSCubeCallback; }

  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkRenderWindowInteractor *iren =
      static_cast<vtkRenderWindowInteractor*>(caller);

    // Reorganize the cube
    if(vtkStdString(iren->GetKeySym()) == "space")
      {
      const int baseCube =
        static_cast<int>(pow(this->Seeds.size(), 1./3.) / 2 + 0.5);
      std::list<vtkSmartPointer<vtkHandleWidget> >::iterator it =
        this->Seeds.begin();

      for(int i=-baseCube; i<baseCube; ++i)
        {
        for(int j=-baseCube; j<baseCube; ++j)
          {
          for(int k=-baseCube; k<baseCube; ++k)
            {
            vtkSphereHandleRepresentation* newHandleRep =
                vtkSphereHandleRepresentation::SafeDownCast(
                  (*it)->GetRepresentation());

            double pos[3] = {static_cast<double>(i),
                             static_cast<double>(j),
                             static_cast<double>(k)};
            newHandleRep->SetWorldPosition(pos);

            ++it;
            }
          }
        }
      }
  }

  std::list<vtkSmartPointer<vtkHandleWidget> > Seeds;
};

//------------------------------------------------------------------------------
// Write timerlog in file
// Each time a render event occurs, the correspoding elapsed time is written.
class vtkPMSRecordPerfCallback : public vtkCommand
{
public:
  static vtkPMSRecordPerfCallback *New()
  { return new vtkPMSRecordPerfCallback; }

  vtkPMSRecordPerfCallback()
  {
    this->performanceReport.open("pickingManagerPerfs.txt");
    this->logTime = vtkTimerLog::New();
  }

  ~vtkPMSRecordPerfCallback()
  {
    if (this->performanceReport.is_open())
      {
      this->performanceReport << "\n";
      this->performanceReport.close();
      }

    this->logTime->Delete();
  }

  virtual void Execute(vtkObject* vtkNotUsed(caller), unsigned long, void*)
  {
    // vtkRenderWindowInteractor *iren =
    //   static_cast<vtkRenderWindowInteractor*>(caller);

    // Compute elapsed time
    double elapsedTime;
    this->logTime->StopTimer();
    elapsedTime = this->logTime->GetElapsedTime();

    if (this->performanceReport.is_open())
      {
      // FPS Measurement
      /*
      this->performanceReport << "; " <<
        iren->GetRenderWindow()->GetRenderers()->
          GetFirstRenderer()->GetLastRenderTimeInSeconds();
      */

      // Write delta time
      this->performanceReport << "; " << elapsedTime;
      }

    // Re-start timer
    this->logTime->StartTimer();
  }

  std::ofstream performanceReport;
  vtkTimerLog* logTime;

private:
  vtkPMSRecordPerfCallback(const vtkPMSRecordPerfCallback&);  //Not implemented
  void operator=(const vtkPMSRecordPerfCallback&);  //Not implemented
};

//------------------------------------------------------------------------------
// Test Picking Manager with a lot of seeds
//------------------------------------------------------------------------------
int TestPickingManagerSeedWidget(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkNew<vtkRenderer> ren1;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkInteractorStyleTrackballCamera> irenStyle;
  iren->SetRenderWindow(renWin.GetPointer());
  iren->SetInteractorStyle(irenStyle.GetPointer());

  /*--------------------------------------------------------------------------*/
  // PICKING MANAGER
  /*--------------------------------------------------------------------------*/
  // Callback to switch between the managed and non-managed mode of the
  // Picking Manager
  vtkNew<vtkPickingManagerCallback> callMode;
  iren->AddObserver(vtkCommand::KeyPressEvent, callMode.GetPointer());

  /*--------------------------------------------------------------------------*/
  // SEEDS
  /*--------------------------------------------------------------------------*/
  // Representations
  double pos[3] = {0, 0, 0};
  vtkNew<vtkSphereHandleRepresentation> handle;
  //handle->SetHandleSize(15.0);
  handle->GetProperty()->SetRepresentationToWireframe();
  handle->GetProperty()->SetColor(1,1,1);

  vtkNew<vtkSeedRepresentation> seedRepresentation;
  seedRepresentation->SetHandleRepresentation(handle.GetPointer());

    // Settings
  vtkNew<vtkSeedWidget> seedWidget;
  seedWidget->SetRepresentation(seedRepresentation.GetPointer());
  seedWidget->SetInteractor(iren.GetPointer());
  seedWidget->EnabledOn();

  // Create a cube full of seeds
  // base correspond to the side of the cube --> (2*base)^3 seeds
  const int baseCube = 2;
  std::list <vtkSmartPointer<vtkHandleWidget> > seeds;
  for(int i=-baseCube; i<baseCube; ++i)
    {
    for(int j=-baseCube; j<baseCube; ++j)
      {
      for(int k=-baseCube; k<baseCube; ++k)
        {
        vtkHandleWidget* newHandle = seedWidget->CreateNewHandle();
        newHandle->SetEnabled(1);
        vtkSphereHandleRepresentation* newHandleRep =
            vtkSphereHandleRepresentation::SafeDownCast(
              newHandle->GetRepresentation());

        pos[0] = i;
        pos[1] = j;
        pos[2] = k;
        newHandleRep->GetProperty()->SetRepresentationToWireframe();
        newHandleRep->GetProperty()->SetColor(1,1,1);
        newHandleRep->SetWorldPosition(pos);

        seeds.push_back(newHandle);
        }
      }
    }

  seedWidget->CompleteInteraction();

  // Callback to reorganize the cube when space is pressed
  vtkNew<vtkPMSCubeCallback> reorganizeCallback;
  reorganizeCallback->Seeds = seeds;
  iren->AddObserver(vtkCommand::KeyPressEvent, reorganizeCallback.GetPointer());

  /*--------------------------------------------------------------------------*/
  // Rendering
  /*--------------------------------------------------------------------------*/
  // Add the actors to the renderer, set the background and size
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(600, 600);

  // Record
  //iren->GetPickingManager()->EnabledOff();
  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren.GetPointer());
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLogTestPickingManagerSeedWidget);

  // render the image
  iren->Initialize();
  double extent[6] = {-7, 7, -7, 7, -1, 1};
  ren1->ResetCamera(extent);
  renWin->Render();

  // Performance Measurements
  // Callback to write the rendering running time given different configurations
  // vtkNew<vtkPMSRecordPerfCallback> writePerfsCallback;
  // iren->AddObserver(vtkCommand::RenderEvent, writePerfsCallback.GetPointer());
  // writePerfsCallback->logTime->StartTimer();

  recorder->Play();
  recorder->Off();

  // writePerfsCallback->logTime->StopTimer();

  iren->Start();

  return EXIT_SUCCESS;
}
