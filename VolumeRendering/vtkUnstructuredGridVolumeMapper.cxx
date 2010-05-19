/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridVolumeMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridVolumeMapper.h"

#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkUnstructuredGrid.h"


// Construct a vtkUnstructuredGridVolumeMapper with empty scalar input and
// clipping off.
vtkUnstructuredGridVolumeMapper::vtkUnstructuredGridVolumeMapper()
{
  this->BlendMode = vtkUnstructuredGridVolumeMapper::COMPOSITE_BLEND;
}

vtkUnstructuredGridVolumeMapper::~vtkUnstructuredGridVolumeMapper()
{  
}

void vtkUnstructuredGridVolumeMapper::SetInput( vtkDataSet *genericInput )
{
  vtkUnstructuredGrid *input = 
    vtkUnstructuredGrid::SafeDownCast( genericInput );
  
  if ( input )
    {
    this->SetInput( input );
    }
  else
    {
    vtkErrorMacro("The SetInput method of this mapper requires vtkUnstructuredGrid as input");
    }
}

void vtkUnstructuredGridVolumeMapper::SetInput( vtkUnstructuredGrid *input )
{
  if(input)
    {
    this->SetInputConnection(0, input->GetProducerPort());
    }
  else
    {
    // Setting a NULL input removes the connection.
    this->SetInputConnection(0, 0);
    }
}

vtkUnstructuredGrid *vtkUnstructuredGridVolumeMapper::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return 0;
    }
  return vtkUnstructuredGrid::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
}


// Print the vtkUnstructuredGridVolumeMapper
void vtkUnstructuredGridVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Blend Mode: " << this->BlendMode << endl;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridVolumeMapper::FillInputPortInformation(
  int vtkNotUsed( port ), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

