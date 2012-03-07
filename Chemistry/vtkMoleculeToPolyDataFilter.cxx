/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMoleculeToPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMoleculeToPolyDataFilter.h"

#include "vtkInformation.h"
#include "vtkMolecule.h"


//----------------------------------------------------------------------------
vtkMoleculeToPolyDataFilter::vtkMoleculeToPolyDataFilter()
{
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkMoleculeToPolyDataFilter::~vtkMoleculeToPolyDataFilter()
{
}

//----------------------------------------------------------------------------
vtkMolecule * vtkMoleculeToPolyDataFilter::GetInput()
{
  return vtkMolecule::SafeDownCast(this->Superclass::GetInput(0));
}

//----------------------------------------------------------------------------
int vtkMoleculeToPolyDataFilter::FillInputPortInformation(int port,
                                                          vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMolecule");
  return 1;
}

//----------------------------------------------------------------------------
void vtkMoleculeToPolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
