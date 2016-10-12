/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPSphereSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPSphereSource.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLargeInteger.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>

vtkStandardNewMacro(vtkPSphereSource);

//----------------------------------------------------------------------------
int vtkPSphereSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkIdType i, j, numOffset;
  int jStart, jEnd;
  vtkIdType numPts, numPolys;
  vtkPoints *newPoints;
  vtkFloatArray *newNormals;
  vtkCellArray *newPolys;
  float x[3], n[3], deltaPhi, deltaTheta, phi, theta, radius, norm;
  float startTheta, endTheta, startPhi, endPhi;
  vtkIdType base, thetaResolution, phiResolution;
  int numPoles = 0;
  vtkIdType pts[3];
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  int piece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // I want to modify the ivars resoultion start theta and end theta,
  // so I will make local copies of them.  THese might be able to be merged
  // with the other copies of them, ...
  int localThetaResolution = this->ThetaResolution;
  float localStartTheta = this->StartTheta;
  float localEndTheta = this->EndTheta;

  while (localEndTheta < localStartTheta)
  {
    localEndTheta += 360.0;
  }
  deltaTheta = (localEndTheta - localStartTheta) / localThetaResolution;

  // Change the ivars based on pieces.
  vtkIdType start, end;
  start = piece * localThetaResolution / numPieces;
  end = (piece+1) * localThetaResolution / numPieces;
  localEndTheta = localStartTheta + (float)(end) * deltaTheta;
  localStartTheta = localStartTheta + (float)(start) * deltaTheta;
  localThetaResolution = end - start;

  //
  // Set things up; allocate memory
  //

  vtkDebugMacro("PSphereSource Executing");

  numPts = this->PhiResolution * localThetaResolution + 2;
  // creating triangles
  numPolys = this->PhiResolution * 2 * localThetaResolution;

  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(3*numPts);

  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys, 3));
  //
  // Create sphere
  //
  // Create north pole if needed
  if ( this->StartPhi <= 0.0 )
  {
    x[0] = this->Center[0];
    x[1] = this->Center[1];
    x[2] = this->Center[2] + this->Radius;
    newPoints->InsertPoint(numPoles,x);

    x[0] = x[1] = 0.0; x[2] = 1.0;
    newNormals->InsertTuple(numPoles,x);
    numPoles++;
  }

  // Create south pole if needed
  if ( this->EndPhi >= 180.0 )
  {
    x[0] = this->Center[0];
    x[1] = this->Center[1];
    x[2] = this->Center[2] - this->Radius;
    newPoints->InsertPoint(numPoles,x);

    x[0] = x[1] = 0.0; x[2] = -1.0;
    newNormals->InsertTuple(numPoles,x);
    numPoles++;
  }

  // Check data, determine increments, and convert to radians
  startTheta = (localStartTheta < localEndTheta ? localStartTheta
                : localEndTheta);
  startTheta *= vtkMath::Pi() / 180.0;
  endTheta = (localEndTheta > localStartTheta ? localEndTheta
              : localStartTheta);
  endTheta *= vtkMath::Pi() / 180.0;

  startPhi = (this->StartPhi < this->EndPhi ? this->StartPhi : this->EndPhi);
  startPhi *= vtkMath::Pi() / 180.0;
  endPhi = (this->EndPhi > this->StartPhi ? this->EndPhi : this->StartPhi);
  endPhi *= vtkMath::Pi() / 180.0;

  phiResolution = this->PhiResolution - numPoles;
  deltaPhi = (endPhi - startPhi) / (this->PhiResolution - 1);
  thetaResolution = localThetaResolution;
  if (fabs(localStartTheta - localEndTheta) < 360.0)
  {
    ++localThetaResolution;
  }
  deltaTheta = (endTheta - startTheta) / thetaResolution;

  jStart = (this->StartPhi <= 0.0 ? 1 : 0);
  jEnd = (this->EndPhi >= 180.0 ? this->PhiResolution - 1
        : this->PhiResolution);

  // Create intermediate points
  for (i=0; i < localThetaResolution; i++)
  {
    theta = localStartTheta * vtkMath::Pi() / 180.0 + i*deltaTheta;

    for (j=jStart; j<jEnd; j++)
    {
      phi = startPhi + j*deltaPhi;
      radius = this->Radius * sin((double)phi);
      n[0] = radius * cos((double)theta);
      n[1] = radius * sin((double)theta);
      n[2] = this->Radius * cos((double)phi);
      x[0] = n[0] + this->Center[0];
      x[1] = n[1] + this->Center[1];
      x[2] = n[2] + this->Center[2];
      newPoints->InsertNextPoint(x);

      if ( (norm = vtkMath::Norm(n)) == 0.0 )
      {
        norm = 1.0;
      }
      n[0] /= norm; n[1] /= norm; n[2] /= norm;
      newNormals->InsertNextTuple(n);
    }
  }

  // Generate mesh connectivity
  base = phiResolution * localThetaResolution;

  if (fabs(localStartTheta - localEndTheta) < 360.0)
  {
    --localThetaResolution;
  }

  if ( this->StartPhi <= 0.0 )  // around north pole
  {
    for (i=0; i < localThetaResolution; i++)
    {
      pts[0] = phiResolution*i + numPoles;
      pts[1] = (phiResolution*(i+1) % base) + numPoles;
      pts[2] = 0;
      newPolys->InsertNextCell(3, pts);
    }
  }

  if ( this->EndPhi >= 180.0 ) // around south pole
  {
    numOffset = phiResolution - 1 + numPoles;

    for (i=0; i < localThetaResolution; i++)
    {
      pts[0] = phiResolution*i + numOffset;
      pts[2] = ((phiResolution*(i+1)) % base) + numOffset;
      pts[1] = numPoles - 1;
      newPolys->InsertNextCell(3, pts);
    }
  }

  // bands in-between poles
  for (i=0; i < localThetaResolution; i++)
  {
    for (j=0; j < (phiResolution-1); j++)
    {
      pts[0] = phiResolution*i + j + numPoles;
      pts[1] = pts[0] + 1;
      pts[2] = ((phiResolution*(i+1)+j) % base) + numPoles + 1;
      newPolys->InsertNextCell(3, pts);

      pts[1] = pts[2];
      pts[2] = pts[1] - 1;
      newPolys->InsertNextCell(3, pts);
    }
  }
  //
  // Update ourselves and release memeory
  //
  newPoints->Squeeze();
  output->SetPoints(newPoints);
  newPoints->Delete();

  newNormals->Squeeze();
  output->GetPointData()->SetNormals(newNormals);
  newNormals->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  return 1;
}

//----------------------------------------------------------------------------
unsigned long vtkPSphereSource::GetEstimatedMemorySize()
{
  vtkLargeInteger sz;
  vtkLargeInteger sz2;
  unsigned long thetaResolution = this->ThetaResolution;
  vtkInformation *outInfo = this->GetExecutive()->GetOutputInformation(0);
  int numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (numPieces)
  {
    thetaResolution /= numPieces;
  }

  if (thetaResolution < 1)
  {
    thetaResolution = 1;
  }

  // ignore poles
  sz = thetaResolution;
  sz = sz * (this->PhiResolution + 1);
  sz2 = thetaResolution;
  sz2 = sz2 * this->PhiResolution * 2;
  sz = sz * 3 * sizeof(float);
  sz2 = sz2 * 4 * sizeof(int);

  sz = sz + sz2;

  // convert to kibibytes (1024 bytes)
  sz >>= 10;

  return sz.CastToUnsignedLong();
}

void vtkPSphereSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
