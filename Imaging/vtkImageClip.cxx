/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageClip.cxx
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
#include "vtkImageClip.h"
#include "vtkExtentTranslator.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkImageClip, "1.45");
vtkStandardNewMacro(vtkImageClip);

//----------------------------------------------------------------------------
vtkImageClip::vtkImageClip()
{
  this->ClipData = 0;
  this->Initialized = 0;

  this->OutputWholeExtent[0] =
  this->OutputWholeExtent[2] =
  this->OutputWholeExtent[4] = -VTK_LARGE_INTEGER;

  this->OutputWholeExtent[1] =
  this->OutputWholeExtent[3] =
  this->OutputWholeExtent[5] = VTK_LARGE_INTEGER;
}


//----------------------------------------------------------------------------
void vtkImageClip::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  int idx;
  
  os << indent << "OutputWholeExtent: (" << this->OutputWholeExtent[0]
     << "," << this->OutputWholeExtent[1];
  for (idx = 1; idx < 3; ++idx)
    {
    os << indent << ", " << this->OutputWholeExtent[idx * 2]
       << "," << this->OutputWholeExtent[idx*2 + 1];
    }
  os << ")\n";
  if (this->ClipData)
    {
    os << indent << "ClipDataOn\n";
    }
  else
    {
    os << indent << "ClipDataOff\n";
    }
}
  
//----------------------------------------------------------------------------
void vtkImageClip::SetOutputWholeExtent(int extent[6])
{
  int idx;
  int modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->OutputWholeExtent[idx] != extent[idx])
      {
      this->OutputWholeExtent[idx] = extent[idx];
      modified = 1;
      }
    }
  this->Initialized = 1;
  if (modified)
    {
    this->Modified();
    vtkImageData *output = this->GetOutput();
    if (output)
      {
      output->SetUpdateExtent(extent);
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageClip::SetOutputWholeExtent(int minX, int maxX, 
                                             int minY, int maxY,
                                             int minZ, int maxZ)
{
  int extent[6];
  
  extent[0] = minX;  extent[1] = maxX;
  extent[2] = minY;  extent[3] = maxY;
  extent[4] = minZ;  extent[5] = maxZ;
  this->SetOutputWholeExtent(extent);
}


//----------------------------------------------------------------------------
void vtkImageClip::GetOutputWholeExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->OutputWholeExtent[idx];
    }
}


//----------------------------------------------------------------------------
// Change the WholeExtent
void vtkImageClip::ExecuteInformation(vtkImageData *inData, 
                                      vtkImageData *outData)
{
  int idx, extent[6];
  
  inData->GetWholeExtent(extent);
  if ( ! this->Initialized)
    {
    this->SetOutputWholeExtent(extent);
    }

  // Clip the OutputWholeExtent with the input WholeExtent
  for (idx = 0; idx < 3; ++idx)
    {
    if (this->OutputWholeExtent[idx*2] >= extent[idx*2] && 
        this->OutputWholeExtent[idx*2] <= extent[idx*2+1])
      {
      extent[idx*2] = this->OutputWholeExtent[idx*2];
      }
    if (this->OutputWholeExtent[idx*2+1] >= extent[idx*2] && 
        this->OutputWholeExtent[idx*2+1] <= extent[idx*2+1])
      {
      extent[idx*2+1] = this->OutputWholeExtent[idx*2+1];
      }
    // make usre the order is correct
    if (extent[idx*2] > extent[idx*2+1])
      {
      extent[idx*2] = extent[idx*2+1];
      }
      }
  
  outData->SetWholeExtent(extent);
}


//----------------------------------------------------------------------------
// Sets the output whole extent to be the input whole extent.
void vtkImageClip::ResetOutputWholeExtent()
{
  if ( ! this->GetInput())
    {
    vtkWarningMacro("ResetOutputWholeExtent: No input");
    return;
    }

  this->GetInput()->UpdateInformation();
  this->SetOutputWholeExtent(this->GetInput()->GetWholeExtent());
}



//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
void vtkImageClip::ExecuteData(vtkDataObject *)
{
  int *inExt;
  vtkImageData *outData = this->GetOutput();
  vtkImageData *inData = this->GetInput();
  
  vtkDebugMacro(<<"Executing image clip");

  inExt  = inData->GetExtent(); 

  outData->SetExtent(inExt);
  outData->GetPointData()->PassData(inData->GetPointData());

  if (this->ClipData)
    {
    outData->Crop();
    } 
}




//----------------------------------------------------------------------------
void vtkImageClip::SetOutputWholeExtent(int piece, int numPieces)
{
  vtkImageData *input = this->GetInput();
  vtkImageData *output = this->GetOutput();
  vtkExtentTranslator *translator;
  int ext[6];

  if (input == NULL)
    {
    vtkErrorMacro("We must have an input to set the output extent by piece.");
    return;
    }
  if (output == NULL)
    {
    vtkErrorMacro("We must have an output to set the output extent by piece.");
    return;
    }
  translator = output->GetExtentTranslator();
  if (translator == NULL)
    {
    vtkErrorMacro("Output does not have an extent translator.");
    return;
    }

  input->UpdateInformation();
  input->GetWholeExtent(ext);
  translator->SetWholeExtent(ext);
  translator->SetPiece(piece);
  translator->SetNumberOfPieces(numPieces);
  translator->SetGhostLevel(0);
  translator->PieceToExtent();
  translator->GetExtent(ext);
  this->SetOutputWholeExtent(ext);
}



