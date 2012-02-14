/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRContourFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkAMRContourFilter.h"
#include "vtkObjectFactory.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

//
// Standard methods
//
vtkStandardNewMacro( vtkAMRContourFilter );

vtkAMRContourFilter::vtkAMRContourFilter()
{
   this->SetNumberOfOutputPorts( 1 );
}

//------------------------------------------------------------------------------
vtkAMRContourFilter::~vtkAMRContourFilter()
{
}

//------------------------------------------------------------------------------
void vtkAMRContourFilter::PrintSelf( std::ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
int vtkAMRContourFilter::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(
   vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHierarchicalBoxDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRContourFilter::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRContourFilter::RequestData( vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  // TODO: implement this
  return( 1 );
}


