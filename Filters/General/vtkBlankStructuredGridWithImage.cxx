/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlankStructuredGridWithImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBlankStructuredGridWithImage.h"

#include "vtkCellData.h"
#include "vtkExecutive.h"
#include "vtkFieldData.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkBlankStructuredGridWithImage);

//----------------------------------------------------------------------------
vtkBlankStructuredGridWithImage::vtkBlankStructuredGridWithImage()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkBlankStructuredGridWithImage::~vtkBlankStructuredGridWithImage()
{
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkBlankStructuredGridWithImage::SetBlankingInputData(vtkImageData *input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkImageData *vtkBlankStructuredGridWithImage::GetBlankingInput()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }

  return vtkImageData::SafeDownCast(
    this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
int vtkBlankStructuredGridWithImage::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *imageInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkStructuredGrid *grid = vtkStructuredGrid::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData *image = vtkImageData::SafeDownCast(
    imageInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkStructuredGrid *output = vtkStructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int gridDims[3], imageDims[3];

  vtkDebugMacro(<< "Adding image blanking");

  // Perform error checking
  grid->GetDimensions(gridDims);
  image->GetDimensions(imageDims);
  if ( gridDims[0] != imageDims[0] || gridDims[1] != imageDims[1] ||
       gridDims[2] != imageDims[2] )
    {
    vtkErrorMacro("Blanking dimensions must be identical with grid dimensions. "
                  "Blanking dimensions are " << imageDims[0] << " "
                  << imageDims[1] << " " << imageDims[2]
                  << ". Grid dimensions are " << gridDims[0] << " "
                  << gridDims[1] << " " << gridDims[2] << ".");
    return 1;
    }

  if ( image->GetScalarType() != VTK_UNSIGNED_CHAR ||
       image->GetNumberOfScalarComponents() != 1 )
    {
    vtkErrorMacro(<<"This filter requires unsigned char images with one component");
    return 1;
    }

  // Get the image, set it as the blanking array.
  unsigned char *data=static_cast<unsigned char *>(image->GetScalarPointer());
  vtkUnsignedCharArray *dataArray = vtkUnsignedCharArray::New();
  dataArray->SetArray(data, gridDims[0]*gridDims[1]*gridDims[2], 1);

  output->CopyStructure(grid);
  output->GetPointData()->PassData(grid->GetPointData());
  output->GetCellData()->PassData(grid->GetCellData());
  output->SetPointVisibilityArray(dataArray);

  dataArray->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkBlankStructuredGridWithImage::FillInputPortInformation(
  int port, vtkInformation *info)
{
  if (port == 0)
    {
    return this->Superclass::FillInputPortInformation(port, info);
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkBlankStructuredGridWithImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
