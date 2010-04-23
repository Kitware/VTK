/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContinuousValueWidgetRepresentation.cxx

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

#include "vtkContinuousValueWidgetRepresentation.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkMath.h"
#include "vtkEvent.h"
#include "vtkInteractorObserver.h"
#include "vtkWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"



//----------------------------------------------------------------------
vtkContinuousValueWidgetRepresentation::vtkContinuousValueWidgetRepresentation()
{
  this->Value = 0;
}

//----------------------------------------------------------------------
vtkContinuousValueWidgetRepresentation::~vtkContinuousValueWidgetRepresentation()
{
}

//----------------------------------------------------------------------
void vtkContinuousValueWidgetRepresentation::PlaceWidget(double *vtkNotUsed(bds[6]))
{
  // Position the handles at the end of the lines
  this->BuildRepresentation();
}

void vtkContinuousValueWidgetRepresentation::SetValue(double)
{

}

//----------------------------------------------------------------------
void vtkContinuousValueWidgetRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Value: " << this->GetValue() << "\n";
}
