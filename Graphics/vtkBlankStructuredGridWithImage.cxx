/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlankStructuredGridWithImage.cxx
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
#include "vtkBlankStructuredGridWithImage.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkBlankStructuredGridWithImage, "1.5");
vtkStandardNewMacro(vtkBlankStructuredGridWithImage);

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkBlankStructuredGridWithImage::SetBlankingInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(1, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkImageData *vtkBlankStructuredGridWithImage::GetBlankingInput()
{
  if (this->NumberOfInputs < 2)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[1]);
}

void vtkBlankStructuredGridWithImage::Execute()
{
  vtkStructuredGrid *grid = this->GetInput();
  vtkStructuredGrid *output = this->GetOutput();
  vtkImageData *image = this->GetBlankingInput();
  int gridDims[3], imageDims[3];

  vtkDebugMacro(<< "Adding image blanking");
  
  // Perform error checking
  grid->GetDimensions(gridDims);
  image->GetDimensions(imageDims);
  if ( gridDims[0] != imageDims[0] || gridDims[1] != imageDims[1] ||
       gridDims[2] != imageDims[2] )
    {
    vtkErrorMacro(<< "Blanking dimensions must be identical with grid dimensions");
    return;
    }
  
  if ( image->GetScalarType() != VTK_UNSIGNED_CHAR ||
       image->GetNumberOfScalarComponents() != 1 )
    {
    vtkErrorMacro(<<"This filter requires unsigned char images with one component");
    return;
    }
  
  // Get the image, set it as the blanking array.
  unsigned char *data = (unsigned char *)image->GetScalarPointer();
  vtkUnsignedCharArray *dataArray = vtkUnsignedCharArray::New();
  dataArray->SetArray(data, gridDims[0]*gridDims[1]*gridDims[2], 1);

  output->CopyStructure(grid);
  output->GetPointData()->PassData(grid->GetPointData());
  output->GetCellData()->PassData(grid->GetCellData());
  output->SetPointVisibility(dataArray);
  output->BlankingOn();
  
  dataArray->Delete();
}


//----------------------------------------------------------------------------
void vtkBlankStructuredGridWithImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
