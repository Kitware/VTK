/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedTerrainPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProjectedTerrainPath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPriorityQueue.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkProjectedTerrainPath, "1.2");
vtkStandardNewMacro(vtkProjectedTerrainPath);

// Begin vtkProjectedTerrainPath class implementation----------------------------
//
vtkProjectedTerrainPath::vtkProjectedTerrainPath()
{
  this->ProjectionMode = SIMPLE_PROJECTION;
  this->HeightOffset = 10.0;
  this->HeightTolerance = 10.0;
  this->SubdivisionFactor = 1000;
  this->LineError = NULL;
}

vtkProjectedTerrainPath::~vtkProjectedTerrainPath()
{
}

int vtkProjectedTerrainPath::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // get the info objects
//  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
//  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
//  vtkImageData *input = vtkImageData::SafeDownCast(
//    inInfo->Get(vtkDataObject::DATA_OBJECT()));
//  vtkPolyData *output = vtkPolyData::SafeDownCast(
//    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  return 1;
}

int vtkProjectedTerrainPath::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

void vtkProjectedTerrainPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
 
  os << indent << "Projection Mode: ";
  if ( this->ProjectionMode == SIMPLE_PROJECTION )
    {
    os << "Simple Projection\n";
    }
  else if ( this->ProjectionMode == NONOCCLUDED_PROJECTION )
    {
    os << "Non-occluded Projection\n";
    }
  else //if ( this->ProjectionMode == HUG_PROJECTION )
    {
    os << "Hug Projection\n";
    }
  
  os << indent << "Height Offset: " << this->HeightOffset << "\n";
  os << indent << "Height Tolerance: " << this->HeightTolerance << "\n";
  os << indent << "Subdivision Factor: " << this->SubdivisionFactor << "\n";
  
}
