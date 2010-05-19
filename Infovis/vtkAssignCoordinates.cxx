/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssignCoordinates.cxx

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
#include "vtkAssignCoordinates.h"

#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkAssignCoordinates);

vtkAssignCoordinates::vtkAssignCoordinates()
{
  this->XCoordArrayName = 0;
  this->YCoordArrayName = 0;
  this->ZCoordArrayName = 0;
  
  this->Jitter = false;
}

vtkAssignCoordinates::~vtkAssignCoordinates()
{
  if(this->XCoordArrayName!=0)
    {
    delete[] this->XCoordArrayName;
    }
  if(this->YCoordArrayName!=0)
    {
    delete[] this->YCoordArrayName;
    }
  if(this->ZCoordArrayName!=0)
    {
    delete[] this->ZCoordArrayName;
    }
}

int vtkAssignCoordinates::RequestData(vtkInformation *vtkNotUsed(request),
                            vtkInformationVector **inputVector,
                            vtkInformationVector *outputVector)
{

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
    
  // Do a shallow copy of the input to the output
  output->ShallowCopy(input);
  
  // Create new points on the output
  vtkDataSetAttributes *data = 0;
  vtkPoints* pts = vtkPoints::New();
  if (vtkPointSet::SafeDownCast(input))
    {
    vtkPointSet *psInput = vtkPointSet::SafeDownCast(input);
    vtkPointSet *psOutput = vtkPointSet::SafeDownCast(output);
    pts->DeepCopy(psInput->GetPoints());
    psOutput->SetPoints(pts);
    pts->Delete();
    data = psOutput->GetPointData();
    }
  else if (vtkGraph::SafeDownCast(input))
    {
    vtkGraph *graphInput = vtkGraph::SafeDownCast(input);
    vtkGraph *graphOutput = vtkGraph::SafeDownCast(output);
    pts->DeepCopy(graphInput->GetPoints());
    graphOutput->SetPoints(pts);
    pts->Delete();
    data = graphOutput->GetVertexData();
    }
  else
    {
    vtkErrorMacro(<<"Input must be graph or point set.");
    return 0;
    }
    
  // I need at least one coordinate array
  if (!this->XCoordArrayName || strlen(XCoordArrayName) == 0)
    {
    return 0;
    }
    
  // Okay now check for coordinate arrays
  vtkDataArray* XArray = data->GetArray(this->XCoordArrayName);
  
  // Does the array exist at all?  
  if (XArray == NULL)
    {
    vtkErrorMacro("Could not find array named " << this->XCoordArrayName);
    return 0;
    }
    
  // Y coordinate array
  vtkDataArray* YArray = 0;
  if (this->YCoordArrayName && strlen(YCoordArrayName) > 0)
    {
    YArray = data->GetArray(this->YCoordArrayName);
    
    // Does the array exist at all?  
    if (YArray == NULL)
      {
      vtkErrorMacro("Could not find array named " << this->YCoordArrayName);
      return 0;
      }
    }
    
  // Z coordinate array
  vtkDataArray* ZArray = 0;
  if (this->ZCoordArrayName && strlen(ZCoordArrayName) > 0)
    {
    ZArray = data->GetArray(this->ZCoordArrayName);
    
    // Does the array exist at all?  
    if (ZArray == NULL)
      {
      vtkErrorMacro("Could not find array named " << this->ZCoordArrayName);
      return 0;
      }
    }
          
  // Generate the points, either x,0,0 or x,y,0 or x,y,z
  int numPts = pts->GetNumberOfPoints();
  for (int i = 0; i < numPts; i++)
    {
    double rx,ry,rz;
    if (Jitter)
      {
      rx = vtkMath::Random()-.5;
      ry = vtkMath::Random()-.5;
      rz = vtkMath::Random()-.5;
      rx *= .02;
      ry *= .02;
      rz *= .02;
      }
    else
      {
      rx = ry = rz = 0;
      }
    if (YArray)
      {
      if (ZArray)
        {
        pts->SetPoint(i, XArray->GetTuple1(i)+rx, 
                         YArray->GetTuple1(i)+ry, 
                         ZArray->GetTuple1(i)+rz);
        }
      else
        {
        pts->SetPoint(i, XArray->GetTuple1(i)+rx, 
                         YArray->GetTuple1(i)+ry, 0);
        }
      }
    else
      {
      pts->SetPoint(i, XArray->GetTuple1(i)+rx, 0, 0);
      }
    }
    
    return 1;
} 

int vtkAssignCoordinates::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // This algorithm may accept a vtkPointSet or vtkGraph.
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;  
}

void vtkAssignCoordinates::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "XCoordArrayName: " 
     << (this->XCoordArrayName ? this->XCoordArrayName : "(none)") << endl;
     
  os << indent << "YCoordArrayName: " 
     << (this->YCoordArrayName ? this->YCoordArrayName : "(none)") << endl;
     
  os << indent << "ZCoordArrayName: " 
     << (this->ZCoordArrayName ? this->ZCoordArrayName : "(none)") << endl;
     
  os << indent << "Jitter: " 
     << (this->Jitter ? "True" : "False") << endl;
}
