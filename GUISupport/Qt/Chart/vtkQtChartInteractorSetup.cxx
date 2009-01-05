/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartInteractorSetup.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/// \file vtkQtChartInteractorSetup.cxx
/// \date March 11, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartInteractorSetup.h"

#include "vtkQtChartArea.h"
#include "vtkQtChartInteractor.h"
#include "vtkQtChartMouseFunction.h"
#include "vtkQtChartMousePan.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartMouseZoom.h"


vtkQtChartMouseSelection *vtkQtChartInteractorSetup::createDefault(
    vtkQtChartArea *area)
{
  // Create a new interactor and add it to the chart area.
  vtkQtChartInteractor *interactor = new vtkQtChartInteractor(area);
  area->setInteractor(interactor);

  // Set up the mouse buttons. Start with pan on the right button.
  interactor->addFunction(Qt::RightButton, new vtkQtChartMousePan(interactor));

  // Add the zoom functionality to the middle button since the middle
  // button usually has the wheel, which is used for zooming.
  interactor->addFunction(Qt::MidButton, new vtkQtChartMouseZoom(interactor));
  interactor->addFunction(Qt::MidButton, new vtkQtChartMouseZoomX(interactor),
      Qt::ControlModifier);
  interactor->addFunction(Qt::MidButton, new vtkQtChartMouseZoomY(interactor),
      Qt::AltModifier);
  interactor->addFunction(Qt::MidButton,
      new vtkQtChartMouseZoomBox(interactor), Qt::ShiftModifier);

  // Add zoom functionality to the wheel.
  interactor->addWheelFunction(new vtkQtChartMouseZoom(interactor));
  interactor->addWheelFunction(new vtkQtChartMouseZoomX(interactor),
      Qt::ControlModifier);
  interactor->addWheelFunction(new vtkQtChartMouseZoomY(interactor),
      Qt::AltModifier);

  // Add selection to the left button.
  vtkQtChartMouseSelection *selection =
      new vtkQtChartMouseSelection(interactor);
  interactor->addFunction(Qt::LeftButton, selection);

  return selection;
}

vtkQtChartMouseSelection *vtkQtChartInteractorSetup::createSplitZoom(
    vtkQtChartArea *area)
{
  // Create a new interactor and add it to the chart area.
  vtkQtChartInteractor *interactor = new vtkQtChartInteractor(area);
  area->setInteractor(interactor);

  // Set up the mouse buttons. Start with pan on the left button.
  interactor->addFunction(Qt::LeftButton, new vtkQtChartMousePan(interactor));

  // Add selection to the left button as well.
  vtkQtChartMouseSelection *selection =
      new vtkQtChartMouseSelection(interactor);
  interactor->addFunction(Qt::LeftButton, selection);

  // Add the zoom box functionality to the right button.
  interactor->addFunction(Qt::RightButton,
      new vtkQtChartMouseZoomBox(interactor));

  // Add the rest of the zoom capability to the middle button.
  interactor->addFunction(Qt::MidButton, new vtkQtChartMouseZoom(interactor));
  interactor->addFunction(Qt::MidButton, new vtkQtChartMouseZoomX(interactor),
      Qt::ControlModifier);
  interactor->addFunction(Qt::MidButton, new vtkQtChartMouseZoomY(interactor),
      Qt::AltModifier);

  // Add zoom functionality to the wheel.
  interactor->addWheelFunction(new vtkQtChartMouseZoom(interactor));
  interactor->addWheelFunction(new vtkQtChartMouseZoomX(interactor),
      Qt::ControlModifier);
  interactor->addWheelFunction(new vtkQtChartMouseZoomY(interactor),
      Qt::AltModifier);

  return selection;
}


