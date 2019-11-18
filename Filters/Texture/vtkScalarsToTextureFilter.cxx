/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarsToTextureFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkScalarsToTextureFilter.h"

#include "vtkDataObject.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkResampleToImage.h"
#include "vtkScalarsToColors.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTextureMapToPlane.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkScalarsToTextureFilter);

//-----------------------------------------------------------------------------
vtkScalarsToTextureFilter::vtkScalarsToTextureFilter()
{
  this->SetNumberOfOutputPorts(2);
  this->TextureDimensions[0] = this->TextureDimensions[1] = 128;
}

//-----------------------------------------------------------------------------
void vtkScalarsToTextureFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Texture dimensions: " << this->TextureDimensions[0] << "x"
     << this->TextureDimensions[1] << '\n'
     << indent << "Transfer function:\n";
  this->TransferFunction->PrintSelf(os, indent.GetNextIndent());
}

//-----------------------------------------------------------------------------
int vtkScalarsToTextureFilter::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return 1;
  }
  return this->Superclass::FillOutputPortInformation(port, info);
}

//-----------------------------------------------------------------------------
int vtkScalarsToTextureFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo0 = outputVector->GetInformationObject(0);
  vtkInformation* outInfo1 = outputVector->GetInformationObject(1);

  // get and check input
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    vtkErrorMacro("Input polydata is null.");
    return 0;
  }

  vtkDataArray* array = this->GetInputArrayToProcess(0, inputVector);
  if (!array)
  {
    vtkErrorMacro("No array to process.");
    return 0;
  }

  // get the name of array to process
  const char* arrayName = array->GetName();

  // get the output
  vtkPolyData* outputGeometry =
    vtkPolyData::SafeDownCast(outInfo0->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* outputTexture =
    vtkImageData::SafeDownCast(outInfo1->Get(vtkDataObject::DATA_OBJECT()));

  // generate texture coords
  vtkNew<vtkTextureMapToPlane> texMap;
  texMap->SetInputData(input);
  texMap->Update();
  vtkPolyData* pdTex = vtkPolyData::SafeDownCast(texMap->GetOutput());

  // Deep copy the poly data to first output, as it will be modified just after
  outputGeometry->DeepCopy(pdTex);

  // overwrite position with texture coordinates
  vtkDataArray* tcoords = pdTex->GetPointData()->GetTCoords();
  vtkPoints* pts = pdTex->GetPoints();
  for (vtkIdType i = 0; i < pts->GetNumberOfPoints(); i++)
  {
    double p[3];
    tcoords->GetTuple(i, p);
    p[2] = 0.0;
    pts->SetPoint(i, p);
  }
  pts->Modified();

  // generate texture image
  vtkNew<vtkResampleToImage> resample;
  resample->UseInputBoundsOff();
  resample->SetSamplingBounds(0.0, 1.0, 0.0, 1.0, 0.0, 0.0);
  resample->SetSamplingDimensions(
    std::max(1, this->TextureDimensions[0]), std::max(1, this->TextureDimensions[1]), 1);
  resample->SetInputDataObject(pdTex);
  resample->Update();

  outputTexture->ShallowCopy(resample->GetOutput());

  // compute RGBA through lookup table
  if (this->UseTransferFunction)
  {
    vtkDataArray* scalars = outputTexture->GetPointData()->GetArray(arrayName);
    vtkSmartPointer<vtkScalarsToColors> stc = this->TransferFunction;
    if (stc.Get() == nullptr)
    {
      // use a default lookup table
      double* range = scalars->GetRange();
      vtkNew<vtkLookupTable> lut;
      lut->SetTableRange(range[0], range[1]);
      lut->Build();
      stc = lut.Get();
    }

    vtkUnsignedCharArray* colors = stc->MapScalars(scalars, VTK_COLOR_MODE_DEFAULT, -1);
    colors->SetName("RGBA");
    outputTexture->GetPointData()->SetScalars(colors);
    colors->Delete();
  }

  return 1;
}

// ----------------------------------------------------------------------------
int vtkScalarsToTextureFilter::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(1);

  int extent[6] = { 0, this->TextureDimensions[0] - 1, 0, this->TextureDimensions[1] - 1, 0, 0 };

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  outInfo->Set(vtkDataObject::ORIGIN(), 0.0, 0.0, 0.0);
  outInfo->Set(vtkDataObject::SPACING(), 1.0 / extent[1], 1.0 / extent[3], 0.0);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);

  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
vtkScalarsToColors* vtkScalarsToTextureFilter::GetTransferFunction()
{
  return this->TransferFunction;
}

//-----------------------------------------------------------------------------
void vtkScalarsToTextureFilter::SetTransferFunction(vtkScalarsToColors* stc)
{
  if (this->TransferFunction.Get() != stc)
  {
    this->TransferFunction = stc;
    this->Modified();
  }
}
