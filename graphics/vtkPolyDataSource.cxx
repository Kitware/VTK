/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkPolyDataSource.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkPolyDataSource* vtkPolyDataSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPolyDataSource");
  if(ret)
    {
    return (vtkPolyDataSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPolyDataSource;
}




//----------------------------------------------------------------------------
vtkPolyDataSource::vtkPolyDataSource()
{
  this->vtkSource::SetNthOutput(0, vtkPolyData::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
  this->ExecutePiece = this->ExecuteNumberOfPieces = 0;
}

//----------------------------------------------------------------------------
vtkPolyData *vtkPolyDataSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkPolyData *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
void vtkPolyDataSource::SetOutput(vtkPolyData *output)
{
  this->vtkSource::SetNthOutput(0, output);
}


//----------------------------------------------------------------------------
void vtkPolyDataSource::ComputeInputUpdateExtents(vtkDataObject *data)
{
  int piece, numPieces;
  vtkPolyData *output = (vtkPolyData *)data;
  int idx;

  output->GetUpdateExtent(piece, numPieces);
  
  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
    {
    return;
    }
  
  // just copy the Update extent as default behavior.
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx])
      {
      this->Inputs[idx]->SetUpdateExtent(piece, numPieces);
      }
    }
  
  // Save the piece so execute can use this information.
  this->ExecutePiece = piece;
  this->ExecuteNumberOfPieces = numPieces;
}

  








