/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QTestMainWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "QTestMainWindow.h"

#include "vtkRegressionTestImage.h"

QTestMainWindow::QTestMainWindow(vtkRenderWindow* renWin, int ac, char** av) :
  QMainWindow(),
  RegressionImageResult(false),
  RenderWindow(renWin),
  argc(ac),
  argv(av)
{
}

bool QTestMainWindow::regressionImageResult() const
{
  return this->RegressionImageResult;
}

void QTestMainWindow::captureImage()
{
  this->RegressionImageResult = vtkRegressionTestImage(RenderWindow);
  this->close();
}
