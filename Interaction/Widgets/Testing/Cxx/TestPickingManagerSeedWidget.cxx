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
// It measures the performances using the Picking manager into different modes:
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
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSeedRepresentation.h"
#include "vtkSeedWidget.h"
#include "vtkSmartPointer.h"
#include "vtkSphereHandleRepresentation.h"
#include "vtkStdString.h"
#include "vtkTimerLog.h"
#include "vtksys/FStream.hxx"

// STL includes
#include <fstream>
#include <iostream>
#include <list>

const char eventLogTestPickingManagerSeedWidget[] = "# StreamVersion 1 \n"
                                                    "EnterEvent 570 160 0 0 0 0 0 i\n"
                                                    "MouseMoveEvent 570 160 0 0 0 0 0 i\n"
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
                                                    "MouseMoveEvent 353 212 0 0 0 0 0 i\n"
                                                    "KeyPressEvent 353 212 0 0 32 1 space i\n"
                                                    "CharEvent 353 212 0 0 32 1 space i\n"
                                                    "KeyReleaseEvent 420 372 0 0 32 1 space i\n"
                                                    "MouseMoveEvent 284 134 0 0 0 0 0 i\n";

//------------------------------------------------------------------------------
// Press 'Ctrl' to switch the activation of the Picking Manager.
// Press 'o' to switch the activation of the optimization based on the render
// events.
class vtkPickingManagerCallback : public vtkCommand
{
public:
  static vtkPickingManagerCallback* New() { return new vtkPickingManagerCallback; }

  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkRenderWindowInteractor* iren = static_cast<vtkRenderWindowInteractor*>(caller);

    // Enable/Disable the PickingManager
    if ((vtkStdString(iren->GetKeySym()) == "Control_L" ||
          vtkStdString(iren->GetKeySym()) == "Control_R") &&
      iren->GetPickingManager())
    {
      if (!iren->GetPickingManager()->GetEnabled())
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
    else if (vtkStdString(iren->GetKeySym()) == "o" && iren->GetPickingManager())
    {
      if (!iren->GetPickingManager()->GetOptimizeOnInteractorEvents())
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

  vtkPickingManagerCallback() = default;
};

//------------------------------------------------------------------------------
// Press 'Space' to reorganize the cube of seeds
class vtkPMSCubeCallback : public vtkCommand
{
public:
  static vtkPMSCubeCallback* New() { return new vtkPMSCubeCallback; }

  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkRenderWindowInteractor* iren = static_cast<vtkRenderWindowInteractor*>(caller);

    // Reorganize the cube
    if (vtkStdString(iren->GetKeySym()) == "space")
    {
      const int baseCube = static_cast<int>(pow(this->Seeds.size(), 1. / 3.) / 2 + 0.5);
      std::list<vtkSmartPointer<vtkHandleWidget> >::iterator it = this->Seeds.begin();

      for (int i = -baseCube; i < baseCube; ++i)
      {
        for (int j = -baseCube; j < baseCube; ++j)
        {
          for (int k = -baseCube; k < baseCube; ++k)
          {
            vtkSphereHandleRepresentation* newHandleRep =
              vtkSphereHandleRepresentation::SafeDownCast((*it)->GetRepresentation());

            double pos[3] = { static_cast<double>(i), static_cast<double>(j),
              static_cast<double>(k) };
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
// Each time a render event occurs, the corresponding elapsed time is written.
class vtkPMSRecordPerfCallback : public vtkCommand
{
public:
  static vtkPMSRecordPerfCallback* New() { return new vtkPMSRecordPerfCallback; }

  vtkPMSRecordPerfCallback()
  {
    this->performanceReport.open("pickingManagerPerfs.txt");
    this->logTime = vtkTimerLog::New();
  }

  ~vtkPMSRecordPerfCallback() override
  {
    if (this->performanceReport.is_open())
    {
      this->performanceReport << "\n";
      this->performanceReport.close();
    }

    this->logTime->Delete();
  }

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long, void*) override
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

  vtksys::ofstream performanceReport;
  vtkTimerLog* logTime;

private:
  vtkPMSRecordPerfCallback(const vtkPMSRecordPerfCallback&) = delete;
  void operator=(const vtkPMSRecordPerfCallback&) = delete;
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
  renWin->AddRenderer(ren1);

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkInteractorStyleTrackballCamera> irenStyle;
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(irenStyle);

  /*--------------------------------------------------------------------------*/
  // PICKING MANAGER
  /*--------------------------------------------------------------------------*/
  // Callback to switch between the managed and non-managed mode of the
  // Picking Manager
  vtkNew<vtkPickingManagerCallback> callMode;
  iren->AddObserver(vtkCommand::KeyPressEvent, callMode);

  /*--------------------------------------------------------------------------*/
  // SEEDS
  /*--------------------------------------------------------------------------*/
  // Representations
  double pos[3] = { 0, 0, 0 };
  vtkNew<vtkSphereHandleRepresentation> handle;
  // handle->SetHandleSize(15.0);
  handle->GetProperty()->SetRepresentationToWireframe();
  handle->GetProperty()->SetColor(1, 1, 1);

  vtkNew<vtkSeedRepresentation> seedRepresentation;
  seedRepresentation->SetHandleRepresentation(handle);

  // Settings
  vtkNew<vtkSeedWidget> seedWidget;
  seedWidget->SetRepresentation(seedRepresentation);
  seedWidget->SetInteractor(iren);
  seedWidget->EnabledOn();

  // Create a cube full of seeds
  // base correspond to the side of the cube --> (2*base)^3 seeds
  const int baseCube = 2;
  std::list<vtkSmartPointer<vtkHandleWidget> > seeds;
  for (int i = -baseCube; i < baseCube; ++i)
  {
    for (int j = -baseCube; j < baseCube; ++j)
    {
      for (int k = -baseCube; k < baseCube; ++k)
      {
        vtkHandleWidget* newHandle = seedWidget->CreateNewHandle();
        newHandle->SetEnabled(1);
        vtkSphereHandleRepresentation* newHandleRep =
          vtkSphereHandleRepresentation::SafeDownCast(newHandle->GetRepresentation());

        pos[0] = i;
        pos[1] = j;
        pos[2] = k;
        newHandleRep->GetProperty()->SetRepresentationToWireframe();
        newHandleRep->GetProperty()->SetColor(1, 1, 1);
        newHandleRep->SetWorldPosition(pos);

        seeds.push_back(newHandle);
      }
    }
  }

  seedWidget->CompleteInteraction();

  // Callback to reorganize the cube when space is pressed
  vtkNew<vtkPMSCubeCallback> reorganizeCallback;
  reorganizeCallback->Seeds = seeds;
  iren->AddObserver(vtkCommand::KeyPressEvent, reorganizeCallback);

  /*--------------------------------------------------------------------------*/
  // Rendering
  /*--------------------------------------------------------------------------*/
  // Add the actors to the renderer, set the background and size
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(600, 600);

  // Record
  // iren->GetPickingManager()->EnabledOff();
  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLogTestPickingManagerSeedWidget);

  // render the image
  iren->Initialize();
  double extent[6] = { -7, 7, -7, 7, -1, 1 };
  ren1->ResetCamera(extent);
  renWin->Render();

  // Performance Measurements
  // Callback to write the rendering running time given different configurations
  // vtkNew<vtkPMSRecordPerfCallback> writePerfsCallback;
  // iren->AddObserver(vtkCommand::RenderEvent, writePerfsCallback);
  // writePerfsCallback->logTime->StartTimer();

  recorder->Play();
  recorder->Off();

  // writePerfsCallback->logTime->StopTimer();

  iren->Start();

  return EXIT_SUCCESS;
}
