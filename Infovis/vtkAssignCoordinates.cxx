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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkAssignCoordinates.h"

#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

vtkCxxRevisionMacro(vtkAssignCoordinates, "1.1");
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
}


int vtkAssignCoordinates::RequestData(vtkInformation *vtkNotUsed(request),
                            vtkInformationVector **inputVector,
                            vtkInformationVector *outputVector)
{

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkGraph *output = vtkGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
    
  // Do a shallow copy of the input to the output
  // and then create new points on the output
  output->ShallowCopy(input);
  vtkPoints* newPoints = vtkPoints::New();
  newPoints->DeepCopy(input->GetPoints());
  output->SetPoints(newPoints);
  newPoints->Delete();
    
  // I need at least one coordinate array
  if (!this->XCoordArrayName || strlen(XCoordArrayName) == 0)
    {
    return 0;
    }
    
  // Okay now check for coordinate arrays
  vtkDataArray* XArray = output->GetVertexData()->GetArray(this->XCoordArrayName);
  
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
    YArray = output->GetVertexData()->GetArray(this->YCoordArrayName);
    
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
    ZArray = output->GetVertexData()->GetArray(this->ZCoordArrayName);
    
    // Does the array exist at all?  
    if (ZArray == NULL)
      {
      vtkErrorMacro("Could not find array named " << this->ZCoordArrayName);
      return 0;
      }
    }
          
  // Generate the points, either x,0,0 or x,y,0 or x,y,z
  vtkPoints* pts = output->GetPoints();
  for (int i=0; i< input->GetNumberOfVertices(); i++)
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
