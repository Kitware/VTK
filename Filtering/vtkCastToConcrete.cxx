/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCastToConcrete.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCastToConcrete.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkCastToConcrete, "1.27");
vtkStandardNewMacro(vtkCastToConcrete);

void vtkCastToConcrete::Execute()
{
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();
  
  vtkDebugMacro(<<"Casting to concrete type...");

  output->ShallowCopy(input);
}


void vtkCastToConcrete::ExecuteInformation()
{
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();

  output->CopyInformation(input);
}


