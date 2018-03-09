/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegY3DReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageData.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSegYReader.h"
#include "vtkSmartPointer.h"
#include "vtkSegY3DReader.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"

#include <algorithm>
#include <iostream>
#include <iterator>


vtkStandardNewMacro(vtkSegY3DReader);

//-----------------------------------------------------------------------------
vtkSegY3DReader::vtkSegY3DReader()
{
  this->SetNumberOfInputPorts(0);
  this->Reader = new vtkSegYReader();
  this->FileName = nullptr;
  this->Is3D = false;
  this->DataOrigin[0] = this->DataOrigin[1] = this->DataOrigin[2] = 0.0;
  this->DataSpacing[0] = this->DataSpacing[1] = this->DataSpacing[2] = 1.0;
  this->DataExtent[0] = this->DataExtent[2] = this->DataExtent[4] = 0;
  this->DataExtent[1] = this->DataExtent[3] = this->DataExtent[5] = 0;

  this->XYCoordMode = VTK_SEGY_SOURCE;
  this->XCoordByte = 73;
  this->YCoordByte = 77;

  this->VerticalCRS = VTK_SEGY_VERTICAL_HEIGHTS;

}

//-----------------------------------------------------------------------------
vtkSegY3DReader::~vtkSegY3DReader()
{
  delete this->Reader;
}

//-----------------------------------------------------------------------------
void vtkSegY3DReader::SetXYCoordModeToSource()
{
  this->SetXYCoordMode(VTK_SEGY_SOURCE);
}

//-----------------------------------------------------------------------------
void vtkSegY3DReader::SetXYCoordModeToCDP()
{
  this->SetXYCoordMode(VTK_SEGY_CDP);
}

//-----------------------------------------------------------------------------
void vtkSegY3DReader::SetXYCoordModeToCustom()
{
  this->SetXYCoordMode(VTK_SEGY_CUSTOM);
}


//-----------------------------------------------------------------------------
void vtkSegY3DReader::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}


//-----------------------------------------------------------------------------
int vtkSegY3DReader::RequestData(vtkInformation* vtkNotUsed(request),
                               vtkInformationVector** vtkNotUsed(inputVector),
                               vtkInformationVector* outputVector)
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

  if (this->Is3D)
  {
    this->Reader->LoadTraces();
    this->Reader->ExportData3D(vtkImageData::SafeDownCast(output),
                               this->DataExtent, this->DataOrigin, this->DataSpacing);
  }
  else
  {
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
        this->Reader->SetXYCoordBytePositions(this->XCoordByte - 1,
                                              this->YCoordByte - 1);
        break;
      }
    default:
      {
        vtkErrorMacro(<< "Unknown value for XYCoordMode " << this->XYCoordMode);
        return 1;
      }
    }
    vtkStructuredGrid* grid = vtkStructuredGrid::SafeDownCast(output);
    this->Reader->SetVerticalCRS(this->VerticalCRS);
    this->Reader->LoadTraces();
    this->Reader->ExportData2D(grid);
    grid->Squeeze();
  }
  this->Reader->In.close();
  std::cout << "RequestData" << std::endl;
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSegY3DReader::RequestInformation(vtkInformation * vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if (this->Is3D)
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    if (!outInfo)
    {
      vtkErrorMacro("Invalid output information object");
      return 0;
    }

    std::cout << "DataExtent: ";
    std::copy(this->DataExtent, this->DataExtent + 6, std::ostream_iterator<int>(std::cout, " "));
    std::cout << "\nDataOrigin: ";
    std::copy(this->DataOrigin, this->DataOrigin + 3, std::ostream_iterator<double>(std::cout, " "));
    std::cout << "\nDataSpacing: ";
    std::copy(this->DataSpacing, this->DataSpacing + 3, std::ostream_iterator<double>(std::cout, " "));
    std::cout << std::endl;
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                 this->DataExtent, 6);
    outInfo->Set(vtkDataObject::ORIGIN(), this->DataOrigin, 3);
    outInfo->Set(vtkDataObject::SPACING(), this->DataSpacing, 3);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkSegY3DReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  const char* outputTypeName = this->Is3D ? "vtkImageData" : "vtkStructuredGrid";
  info->Set(vtkDataObject::DATA_TYPE_NAME(), outputTypeName);
  std::cout << "FillOutputPortInformation" << std::endl;
  return 1;
}

//----------------------------------------------------------------------------
int vtkSegY3DReader::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  if (!this->FileName)
  {
    vtkErrorMacro("Requires valid input file name") ;
    return 0;
  }

  this->Reader->In.open(this->FileName, std::ifstream::binary);
  if (!this->Reader->In)
  {
    std::cerr << "File not found:" << this->FileName << std::endl;
    return 0;
  }
  this->Is3D = this->Reader->Is3DComputeParameters(
    this->DataExtent, this->DataOrigin, this->DataSpacing);
  const char* outputTypeName = this->Is3D ? "vtkImageData" : "vtkStructuredGrid";

  if (!output || !output->IsA(outputTypeName))
  {
    vtkDataSet* newOutput = nullptr;
    if (this->Is3D)
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
  std::cout << "RequestDataObject" << std::endl;
  return 1;
}
