/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitTextureCoords.cxx
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
#include "vtkImplicitTextureCoords.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkImplicitTextureCoords, "1.40");
vtkStandardNewMacro(vtkImplicitTextureCoords);

// Create object with texture dimension=2 and no r-s-t implicit functions
// defined and FlipTexture turned off.
vtkImplicitTextureCoords::vtkImplicitTextureCoords()
{
  this->RFunction = NULL;
  this->SFunction = NULL;
  this->TFunction = NULL;

  this->FlipTexture = 0;
}

vtkImplicitTextureCoords::~vtkImplicitTextureCoords()
{
  this->SetRFunction(NULL);
  this->SetSFunction(NULL);
  this->SetTFunction(NULL);
}


void vtkImplicitTextureCoords::Execute()
{
  vtkIdType ptId, numPts;
  int tcoordDim;
  vtkFloatArray *newTCoords;
  float min[3], max[3], scale[3];
  float tCoord[3], *tc, *x;
  int i;
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();

  // Initialize
  //
  vtkDebugMacro(<<"Generating texture coordinates from implicit functions...");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( ((numPts=input->GetNumberOfPoints()) < 1) )
    {
    vtkErrorMacro(<< "No input points!");
    return;
    }

  if ( this->RFunction == NULL )
    {
    vtkErrorMacro(<< "No implicit functions defined!");
    return;
    }

  tcoordDim = 1;
  if ( this->SFunction != NULL )
    {
    tcoordDim++;
    if ( this->TFunction != NULL )
      {
      tcoordDim++;
      }
    }
//
// Allocate
//
  tCoord[0] = tCoord[1] = tCoord[2] = 0.0;

  newTCoords = vtkFloatArray::New();
  if ( tcoordDim == 1 ) //force 2D map to be created
    {
    newTCoords->SetNumberOfComponents(2);
    newTCoords->Allocate(2*numPts);
    }
  else
    {
    newTCoords->SetNumberOfComponents(tcoordDim);
    newTCoords->Allocate(tcoordDim*numPts);
    }
//
// Compute implicit function values -> insert as initial texture coordinate
//
  for (i=0; i<3; i++) //initialize min/max values array
    {
    min[i] = VTK_LARGE_FLOAT;
    max[i] = -VTK_LARGE_FLOAT;
    }
  for (ptId=0; ptId<numPts; ptId++) //compute texture coordinates
    {
    x = input->GetPoint(ptId);
    tCoord[0] = this->RFunction->FunctionValue(x);
    if ( this->SFunction )
      {
      tCoord[1] = this->SFunction->FunctionValue(x);
      }
    if ( this->TFunction )
      {
      tCoord[2] = this->TFunction->FunctionValue(x);
      }

    for (i=0; i<tcoordDim; i++)
      {
      if (tCoord[i] < min[i])
        {
        min[i] = tCoord[i];
        }
      if (tCoord[i] > max[i])
        {
        max[i] = tCoord[i];
        }
      }

    newTCoords->InsertTuple(ptId,tCoord);
    }
//
// Scale and shift texture coordinates into (0,1) range, with 0.0 implicit 
// function value equal to texture coordinate value of 0.5
//
  for (i=0; i<tcoordDim; i++)
    {
    scale[i] = 1.0;
    if ( max[i] > 0.0 && min[i] < 0.0 ) //have positive & negative numbers
      {
      if ( max[i] > (-min[i]) )
        {
        scale[i] = 0.499 / max[i]; //scale into 0.5->1
        }
      else
        {
        scale[i] = -0.499 / min[i]; //scale into 0->0.5
        }
      }
    else if ( max[i] > 0.0 ) //have positive numbers only
      {
      scale[i] = 0.499 / max[i]; //scale into 0.5->1.0
      }
    else if ( min[i] < 0.0 ) //have negative numbers only
      {
      scale[i] = -0.499 / min[i]; //scale into 0.0->0.5
      }
    }

  if ( this->FlipTexture )
    {
    for (i=0; i<tcoordDim; i++)
      {
      scale[i] *= (-1.0);
      }
    }
  for (ptId=0; ptId<numPts; ptId++)
    {
    tc = newTCoords->GetTuple(ptId);
    for (i=0; i<tcoordDim; i++)
      {
      tCoord[i] = 0.5 + scale[i] * tc[i];
      }
    newTCoords->InsertTuple(ptId,tCoord);
    }
//
// Update self
//
  output->GetPointData()->CopyTCoordsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
}

void vtkImplicitTextureCoords::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Flip Texture: " << this->FlipTexture << "\n";

  if ( this->RFunction != NULL )
    {
    if ( this->SFunction != NULL )
      {
      if ( this->TFunction != NULL )
        {
        os << indent << "R, S, and T Functions defined\n";
        }
      }
    else
      {
      os << indent << "R and S Functions defined\n";
      }
    }
  else
    {
    os << indent << "R Function defined\n";
    }
}
