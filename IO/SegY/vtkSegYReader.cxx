/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegYReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSegYReader.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkSegYReaderInternal.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"

#include <algorithm>
#include <iostream>
#include <iterator>

vtkStandardNewMacro(vtkSegYReader);

//-----------------------------------------------------------------------------
vtkSegYReader::vtkSegYReader()
{
  this->SetNumberOfInputPorts(0);
  this->Reader = new vtkSegYReaderInternal();
  this->FileName = nullptr;
  this->Is3D = false;
  this->Force2D = false;
  std::fill(this->DataOrigin, this->DataOrigin + 3, 0.0);
  std::fill(this->DataSpacing[0], this->DataSpacing[0] + 3, 1.0);
  std::fill(this->DataSpacing[1], this->DataSpacing[1] + 3, 1.0);
  std::fill(this->DataSpacing[2], this->DataSpacing[2] + 3, 1.0);
  std::fill(this->DataSpacingSign, this->DataSpacingSign + 3, 1);
  std::fill(this->DataExtent, this->DataExtent + 6, 0);

  this->XYCoordMode = VTK_SEGY_SOURCE;
  this->StructuredGrid = 1;
  this->XCoordByte = 73;
  this->YCoordByte = 77;

  this->VerticalCRS = VTK_SEGY_VERTICAL_HEIGHTS;
}

//-----------------------------------------------------------------------------
vtkSegYReader::~vtkSegYReader()
{
  delete this->Reader;
  delete[] this->FileName;
}

//-----------------------------------------------------------------------------
void vtkSegYReader::SetXYCoordModeToSource()
{
  this->SetXYCoordMode(VTK_SEGY_SOURCE);
}

//-----------------------------------------------------------------------------
void vtkSegYReader::SetXYCoordModeToCDP()
{
  this->SetXYCoordMode(VTK_SEGY_CDP);
}

//-----------------------------------------------------------------------------
void vtkSegYReader::SetXYCoordModeToCustom()
{
  this->SetXYCoordMode(VTK_SEGY_CUSTOM);
}

//-----------------------------------------------------------------------------
void vtkSegYReader::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkSegYReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    return 0;
  }

  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!output)
  {
    return 0;
  }

  this->Reader->SetVerticalCRS(this->VerticalCRS);
  switch (this->XYCoordMode)
  {
    case VTK_SEGY_SOURCE:
    {
      this->Reader->SetXYCoordBytePositions(72, 76);
      break;
    }
    case VTK_SEGY_CDP:
    {
      this->Reader->SetXYCoordBytePositions(180, 184);
      break;
    }
    case VTK_SEGY_CUSTOM:
    {
      this->Reader->SetXYCoordBytePositions(this->XCoordByte - 1, this->YCoordByte - 1);
      break;
    }
    default:
    {
      vtkErrorMacro(<< "Unknown value for XYCoordMode " << this->XYCoordMode);
      return 1;
    }
  }
  this->Reader->LoadTraces(this->DataExtent);
  this->UpdateProgress(0.5);
  if (this->Is3D && !this->StructuredGrid)
  {
    vtkImageData* imageData = vtkImageData::SafeDownCast(output);
    this->Reader->ExportData(
      imageData, this->DataExtent, this->DataOrigin, this->DataSpacing, this->DataSpacingSign);
  }
  else
  {
    vtkStructuredGrid* grid = vtkStructuredGrid::SafeDownCast(output);
    this->Reader->ExportData(grid, this->DataExtent, this->DataOrigin, this->DataSpacing);
    grid->Squeeze();
  }
  this->Reader->In.close();
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSegYReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Invalid output information object");
    return 0;
  }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->DataExtent, 6);
  if (this->Is3D && !this->StructuredGrid)
  {
    double spacing[3] = { vtkMath::Norm(this->DataSpacing[0]), vtkMath::Norm(this->DataSpacing[1]),
      vtkMath::Norm(this->DataSpacing[2]) };
    outInfo->Set(vtkDataObject::ORIGIN(), this->DataOrigin, 3);
    outInfo->Set(vtkDataObject::SPACING(), spacing, 3);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkSegYReader::RequestDataObject(vtkInformation*,
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  if (!this->FileName)
  {
    vtkErrorMacro("Requires valid input file name");
    return 0;
  }

  if (this->Reader->In.is_open())
  {
    this->Reader->In.seekg(0, this->Reader->In.beg);
  }
  else
  {
    this->Reader->In.open(this->FileName, std::ios::binary);
  }
  if (!this->Reader->In)
  {
    vtkErrorMacro("File not found:" << this->FileName);
    return 0;
  }
  this->Is3D = this->Reader->Is3DComputeParameters(
    this->DataExtent, this->DataOrigin, this->DataSpacing, this->DataSpacingSign, this->Force2D);
  const char* outputTypeName =
    (this->Is3D && !this->StructuredGrid) ? "vtkImageData" : "vtkStructuredGrid";

  if (!output || !output->IsA(outputTypeName))
  {
    vtkDataSet* newOutput = nullptr;
    if (this->Is3D && !this->StructuredGrid)
    {
      newOutput = vtkImageData::New();
    }
    else
    {
      newOutput = vtkStructuredGrid::New();
    }
    info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    newOutput->Delete();
  }
  return 1;
}
