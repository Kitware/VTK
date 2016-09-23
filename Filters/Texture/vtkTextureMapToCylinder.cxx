/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToCylinder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextureMapToCylinder.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkOBBTree.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkTextureMapToCylinder);

// Create object with cylinder axis parallel to z-axis (points (0,0,-0.5)
// and (0,0,0.5)). The PreventSeam ivar is set to true. The cylinder is
// automatically generated.
vtkTextureMapToCylinder::vtkTextureMapToCylinder()
{
  this->Point1[0] = 0.0;
  this->Point1[1] = 0.0;
  this->Point1[2] = -0.5;

  this->Point2[0] = 0.0;
  this->Point2[1] = 0.0;
  this->Point2[2] = 0.5;

  this->AutomaticCylinderGeneration = 1;
  this->PreventSeam = 1;
}

int vtkTextureMapToCylinder::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkFloatArray *newTCoords;
  vtkIdType numPts=input->GetNumberOfPoints();
  vtkIdType ptId;
  int i;
  double x[3], tc[2], thetaX, thetaY, closest[3], v[3];
  double axis[3], vP[3], vec[3];

  vtkDebugMacro(<<"Generating Cylindrical Texture Coordinates");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( numPts < 1 )
  {
    vtkErrorMacro(<<"Can't generate texture coordinates without points");
    return 1;
  }

  if ( this->AutomaticCylinderGeneration )
  {
    vtkPoints *pts=vtkPoints::New(); pts->SetNumberOfPoints(numPts);
    double corner[3], max[3], mid[3], min[3], size[3], l;
    vtkOBBTree *OBB = vtkOBBTree::New();

    for ( ptId=0; ptId < numPts; ptId++ )
    {
      input->GetPoint(ptId, x);
      pts->SetPoint(ptId,x);
    }

    OBB->ComputeOBB(pts,corner,max,mid,min,size);
    pts->Delete();
    OBB->Delete();

    for ( i=0; i < 3; i++)
    {
      l = (mid[i] + min[i])/2.0;
      this->Point1[i] = corner[i] + l;
      this->Point2[i] = corner[i] + max[i] + l;
    }

    vtkDebugMacro(<<"Cylinder axis computed as \tPoint1: ("
                  << this->Point1[0] <<", " << this->Point1[1] <<", "
                  << this->Point1[2] <<")\n\t\t\t\tPoint2: ("
                  << this->Point2[0] <<", " << this->Point2[1] <<", "
                  << this->Point2[2] <<")");
  }

  //compute axis which is theta (angle measure) origin
  for ( i=0; i < 3; i++ )
  {
    axis[i] = this->Point2[i] - this->Point1[i];
  }
  if ( vtkMath::Norm(axis) == 0.0 )
  {
    vtkErrorMacro(<<"Bad cylinder axis");
    return 1;
  }

  v[0] = 1.0; v[1] = v[2] = 0.0;
  vtkMath::Cross(axis,v,vP);
  if ( vtkMath::Norm(vP) == 0.0 )
  {//must be prependicular
    v[1] = 1.0; v[0] = v[2] = 0.0;
    vtkMath::Cross(axis,v,vP);
  }
  vtkMath::Cross(vP,axis,vec);
  if ( vtkMath::Normalize(vec) == 0.0 )
  {
    vtkErrorMacro(<<"Bad cylinder axis");
    return 1;
  }
  newTCoords = vtkFloatArray::New();
  newTCoords->SetName("Texture Coordinates");
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(2*numPts);

  //loop over all points computing spherical coordinates
  for ( ptId=0; ptId < numPts; ptId++ )
  {
    input->GetPoint(ptId, x);
    vtkLine::DistanceToLine(x,this->Point1,this->Point2,tc[1],closest);

    for (i=0; i < 3; i++)
    {
      v[i] = x[i] - closest[i];
    }
    vtkMath::Normalize(v);

    thetaX = acos ((double)vtkMath::Dot(v,vec));
    vtkMath::Cross(vec,v,vP);
    thetaY = vtkMath::Dot(axis,vP); //not really interested in angle, just +/- sign

    if ( this->PreventSeam )
    {
      tc[0] = thetaX / vtkMath::Pi();
    }
    else
    {
      tc[0] = thetaX / (2.0*vtkMath::Pi());
      if ( thetaY < 0.0 )
      {
        tc[0] = 1.0 - tc[0];
      }
    }

    newTCoords->InsertTuple(ptId,tc);
  }

  output->GetPointData()->CopyTCoordsOff();
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  return 1;
}

void vtkTextureMapToCylinder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Automatic Cylinder Generation: " <<
                  (this->AutomaticCylinderGeneration ? "On\n" : "Off\n");
  os << indent << "Prevent Seam: " <<
                  (this->PreventSeam ? "On\n" : "Off\n");
  os << indent << "Point1: (" << this->Point1[0] << ", "
                              << this->Point1[1] << ", "
                              << this->Point1[2] << ")\n";
  os << indent << "Point2: (" << this->Point2[0] << ", "
                              << this->Point2[1] << ", "
                              << this->Point2[2] << ")\n";
}
