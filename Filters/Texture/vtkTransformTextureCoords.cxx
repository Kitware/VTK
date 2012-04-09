/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformTextureCoords.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransformTextureCoords.h"

#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkTransformTextureCoords);

// Create instance with Origin (0.5,0.5,0.5); Position (0,0,0); and Scale
// set to (1,1,1). Rotation of the texture coordinates is turned off.
vtkTransformTextureCoords::vtkTransformTextureCoords()
{
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.5;
  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
  this->Scale[0] = this->Scale[1] = this->Scale[2] = 1.0;

  this->FlipR = 0;
  this->FlipS = 0;
  this->FlipT = 0;
}

// Incrementally change the position of the texture map (i.e., does a
// translate or shift of the texture coordinates).
void vtkTransformTextureCoords::AddPosition (double dPX, double dPY, double dPZ)
{
  double position[3];

  position[0] = this->Position[0] + dPX;
  position[1] = this->Position[1] + dPY;
  position[2] = this->Position[2] + dPZ;
  
  this->SetPosition(position);
}

void vtkTransformTextureCoords::AddPosition(double deltaPosition[3])
{ 
  this->AddPosition (deltaPosition[0], deltaPosition[1], deltaPosition[2]);
}

int vtkTransformTextureCoords::RequestData(
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

  vtkDataArray *inTCoords=input->GetPointData()->GetTCoords();
  vtkDataArray *newTCoords;
  vtkIdType numPts=input->GetNumberOfPoints(), ptId;
  int i, j, texDim;
  vtkTransform *transform;
  vtkMatrix4x4 *matrix;
  double TC[3], newTC[3];

  vtkDebugMacro(<<"Transforming texture coordinates...");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( inTCoords == NULL || numPts < 1 )
    {
    vtkErrorMacro(<<"No texture coordinates to transform");
    return 1;
    }
  transform = vtkTransform::New();
  matrix = vtkMatrix4x4::New();

  // create same type as input
  texDim = inTCoords->GetNumberOfComponents();
  newTCoords = inTCoords->NewInstance();
  newTCoords->SetNumberOfComponents(inTCoords->GetNumberOfComponents());
  newTCoords->Allocate(numPts*texDim);

  // just pretend texture coordinate is 3D point and use transform object to
  // manipulate
  transform->PostMultiply();
  // shift back to origin
  transform->Translate(-this->Origin[0], -this->Origin[1], -this->Origin[2]);

  // scale
  transform->Scale(this->Scale[0], this->Scale[1], this->Scale[2]);

  // rotate about z, then x, then y
  if ( this->FlipT )
    {
    transform->RotateZ(180.0);
    }
  if ( this->FlipR )
    {
    transform->RotateX(180.0);
    }
  if ( this->FlipS )
    {
    transform->RotateY(180.0);
    }

  // move back from origin and translate
  transform->Translate(this->Origin[0] + this->Position[0],
                      this->Origin[1] + this->Position[1],
                      this->Origin[2] + this->Position[2]);

  matrix->DeepCopy(transform->GetMatrix());

  newTC[0] = newTC[1] = newTC[2] = 0.0;
  newTC[0] = newTC[1] = newTC[2] = 0.0;

  int abort=0;
  int progressInterval = numPts/20+1;
  
  for (ptId=0; ptId < numPts && !abort; ptId++)
    {
    if ( !(ptId % progressInterval) )
      {
      this->UpdateProgress((double)ptId/numPts);
      abort = this->GetAbortExecute();
      }

    inTCoords->GetTuple(ptId, TC);
    for (i=0; i<texDim; i++)
      {
      newTC[i] = matrix->Element[i][3];
      for (j=0; j<texDim; j++)
        {
        newTC[i] += matrix->Element[i][j] * TC[j];
        }
      }

    newTCoords->InsertTuple(ptId,newTC);
    }

  // Update self
  //
  output->GetPointData()->CopyTCoordsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
  matrix->Delete();
  transform->Delete();

  return 1;
}

void vtkTransformTextureCoords::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Scale: (" 
     << this->Scale[0] << ", " 
     << this->Scale[1] << ", " 
     << this->Scale[2] << ")\n";

  os << indent << "Position: (" 
     << this->Position[0] << ", " 
     << this->Position[1] << ", " 
     << this->Position[2] << ")\n";

  os << indent << "Origin: (" 
     << this->Origin[0] << ", " 
     << this->Origin[1] << ", " 
     << this->Origin[2] << ")\n";

  os << indent << "FlipR: " << (this->FlipR ? "On\n" : "Off\n");
  os << indent << "FlipS: " << (this->FlipS ? "On\n" : "Off\n");
  os << indent << "FlipT: " << (this->FlipT ? "On\n" : "Off\n");
}
