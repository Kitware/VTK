/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitTextureCoords.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkImplicitTextureCoords.h"

// Description:
// Create object with texture dimension=2 and no r-s-t implicit functions
// defined and FlipTexture turned off.
vtkImplicitTextureCoords::vtkImplicitTextureCoords()
{
  this->RFunction = NULL;
  this->SFunction = NULL;
  this->TFunction = NULL;

  this->FlipTexture = 0;
}

void vtkImplicitTextureCoords::Execute()
{
  int ptId, numPts, tcoordDim;
  vtkFloatTCoords *newTCoords;
  float min[3], max[3], scale[3];
  float tCoord[3], *tc, *x;
  int i;
  vtkDataSet *input=this->Input;
  //
  // Initialize
  //
  vtkDebugMacro(<<"Generating texture coordinates from implicit functions...");

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

  if ( tcoordDim == 1 ) //force 2D map to be created
    {
    newTCoords = vtkFloatTCoords::New();
    newTCoords->Allocate(numPts,2);
    }
  else
    {
    newTCoords = vtkFloatTCoords::New();
    newTCoords->Allocate(numPts,tcoordDim);
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
    if ( this->SFunction ) tCoord[1] = this->SFunction->FunctionValue(x);
    if ( this->TFunction ) tCoord[2] = this->TFunction->FunctionValue(x);

    for (i=0; i<tcoordDim; i++)
      {
      if (tCoord[i] < min[i]) min[i] = tCoord[i];
      if (tCoord[i] > max[i]) max[i] = tCoord[i];
      }

    newTCoords->InsertTCoord(ptId,tCoord);
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
      if ( max[i] > (-min[i]) ) scale[i] = 0.499 / max[i]; //scale into 0.5->1
      else scale[i] = -0.499 / min[i]; //scale into 0->0.5
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

  if ( this->FlipTexture ) for (i=0; i<tcoordDim; i++) scale[i] *= (-1.0);
  for (ptId=0; ptId<numPts; ptId++)
    {
    tc = newTCoords->GetTCoord(ptId);
    for (i=0; i<tcoordDim; i++) tCoord[i] = 0.5 + scale[i] * tc[i];
    newTCoords->InsertTCoord(ptId,tCoord);
    }
//
// Update self
//
  this->Output->GetPointData()->CopyTCoordsOff();
  this->Output->GetPointData()->PassData(input->GetPointData());

  this->Output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
}

void vtkImplicitTextureCoords::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

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

// Description:
// Update input to this filter and the filter itself. Note that we are 
// overloading this method because the output is an abstract dataset type.
// This requires special treatment.
void vtkImplicitTextureCoords::Update()
{
  int doit = 0;
  
  // make sure output has been created
  if ( !this->Output )
    {
    vtkErrorMacro(<< "No output has been created...need to set input");
    return;
    }

  // make sure input is available
  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Updating = 0;

  if ( this->Input->GetMTime() > this->ExecuteTime ||
       this->GetMTime() > this->ExecuteTime )
    {
    doit = 1;
    }
  if (this->RFunction && this->RFunction->GetMTime() > this->ExecuteTime)
    {
    doit = 1;
    }
  if (this->SFunction && this->SFunction->GetMTime() > this->ExecuteTime)
    {
    doit = 1;
    }
  if (this->TFunction && this->TFunction->GetMTime() > this->ExecuteTime)
    {
    doit = 1;
    }
  
  if (doit)
    {
    if ( this->Input->GetDataReleased() )
      {
      this->Input->ForceUpdate();
      }

    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    // copy topological/geometric structure from input
    this->Output->CopyStructure(this->Input);
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
}

