/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitImageRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitImageRepresentation.h"

#include "vtkActor.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCutter.h"
#include "vtkFeatureEdges.h"
#include "vtkImageData.h"
#include "vtkImageMapToColors.h"
#include "vtkImageReslice.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTexture.h"
#include "vtkTextureMapToPlane.h"

#include <iostream>

vtkStandardNewMacro(vtkImplicitImageRepresentation);
vtkCxxSetObjectMacro(vtkImplicitImageRepresentation, ColorMap, vtkImageMapToColors);

//------------------------------------------------------------------------------
vtkImplicitImageRepresentation::vtkImplicitImageRepresentation()
{
  this->UserControlledLookupTable = false;
  this->TextureInterpolate = true;
  this->ResliceInterpolate = VTK_LINEAR_RESLICE;
  this->OriginalWindow = 1.0;
  this->OriginalLevel = 0.5;

  this->ImageData = nullptr;
  this->Reslice = vtkImageReslice::New();
  this->Reslice->TransformInputSamplingOff();
  this->ResliceAxes = vtkMatrix4x4::New();
  this->ColorMap = vtkImageMapToColors::New();
  this->Texture = vtkTexture::New();
  this->LookupTable = nullptr;

  // Setup the image / texture pipeline
  this->GenerateTexturePlane();
}

//------------------------------------------------------------------------------
vtkImplicitImageRepresentation::~vtkImplicitImageRepresentation()
{
  if (this->LookupTable)
  {
    this->LookupTable->UnRegister(this);
  }
  if (this->ImageData)
  {
    this->ImageData = nullptr;
  }
  this->Reslice->Delete();
  this->ResliceAxes->Delete();
  this->ColorMap->Delete();
  this->Texture->Delete();
  this->TextureMapToPlane->Delete();
}

//------------------------------------------------------------------------------
void vtkImplicitImageRepresentation::PlaceImage(vtkAlgorithmOutput* aout)
{
  vtkImageData* img =
    vtkImageData::SafeDownCast(aout->GetProducer()->GetOutputDataObject(aout->GetIndex()));

  this->PlaceImage(img);
  this->Reslice->SetInputConnection(aout);
}

//------------------------------------------------------------------------------
void vtkImplicitImageRepresentation::PlaceImage(vtkImageData* img)
{
  this->ImageData = img;
  if (!this->ImageData)
  {
    // If nullptr is passed, remove any reference that Reslice had
    // on the old ImageData
    //
    this->Reslice->SetInputData(nullptr);
    return;
  }

  // Place the widget
  double bounds[6];
  img->GetBounds(bounds);
  this->Superclass::PlaceWidget(bounds);

  // Now update the pipeline
  double range[2];
  this->ImageData->GetScalarRange(range);

  if (!this->UserControlledLookupTable)
  {
    this->LookupTable->SetTableRange(range[0], range[1]);
    this->LookupTable->Build();
  }

  this->OriginalWindow = range[1] - range[0];
  this->OriginalLevel = 0.5 * (range[0] + range[1]);

  if (fabs(this->OriginalWindow) < 0.001)
  {
    this->OriginalWindow = 0.001 * (this->OriginalWindow < 0.0 ? -1 : 1);
  }
  if (fabs(this->OriginalLevel) < 0.001)
  {
    this->OriginalLevel = 0.001 * (this->OriginalLevel < 0.0 ? -1 : 1);
  }

  this->Reslice->SetInputData(img);
  int interpolate = this->ResliceInterpolate;
  this->ResliceInterpolate = -1; // Force change
  this->SetResliceInterpolate(interpolate);

  this->ColorMap->SetInputConnection(this->Reslice->GetOutputPort());

  this->Texture->SetInputConnection(this->ColorMap->GetOutputPort());
  this->Texture->SetInterpolate(this->TextureInterpolate);
}

//------------------------------------------------------------------------------
void vtkImplicitImageRepresentation::SetLookupTable(vtkLookupTable* table)
{
  if (this->LookupTable != table)
  {
    // to avoid destructor recursion
    vtkLookupTable* temp = this->LookupTable;
    this->LookupTable = table;
    if (temp != nullptr)
    {
      temp->UnRegister(this);
    }
    if (this->LookupTable != nullptr)
    {
      this->LookupTable->Register(this);
    }
    else // create a default lut
    {
      this->LookupTable = this->CreateDefaultLookupTable();
    }
  }

  this->ColorMap->SetLookupTable(this->LookupTable);
  this->Texture->SetLookupTable(this->LookupTable);

  if (this->ImageData && !this->UserControlledLookupTable)
  {
    double range[2];
    this->ImageData->GetScalarRange(range);

    this->LookupTable->SetTableRange(range[0], range[1]);
    this->LookupTable->Build();

    this->OriginalWindow = range[1] - range[0];
    this->OriginalLevel = 0.5 * (range[0] + range[1]);
  }
}

//------------------------------------------------------------------------------
void vtkImplicitImageRepresentation::SetResliceInterpolate(int i)
{
  if (this->ResliceInterpolate == i)
  {
    return;
  }
  this->ResliceInterpolate = i;
  this->Modified();

  if (!this->Reslice)
  {
    return;
  }

  if (i == VTK_NEAREST_RESLICE)
  {
    this->Reslice->SetInterpolationModeToNearestNeighbor();
  }
  else if (i == VTK_LINEAR_RESLICE)
  {
    this->Reslice->SetInterpolationModeToLinear();
  }
  else
  {
    this->Reslice->SetInterpolationModeToCubic();
  }
  this->Texture->SetInterpolate(this->TextureInterpolate);
}

//------------------------------------------------------------------------------
void vtkImplicitImageRepresentation::CreateDefaultProperties()
{
  // Use what's defined in the superclass
  this->Superclass::CreateDefaultProperties();

  // Plane properties need to modified for best appearance due to texture
  this->PlaneProperty->SetAmbient(1.0);
  this->PlaneProperty->SetAmbientColor(1.0, 1.0, 1.0);
  this->PlaneProperty->SetOpacity(1.0);
  this->CutActor->SetProperty(this->PlaneProperty);

  this->SelectedPlaneProperty->SetAmbient(1.0);
  this->SelectedPlaneProperty->SetAmbientColor(0.0, 1.0, 0.0);
  this->SelectedPlaneProperty->SetOpacity(1.0);
}

//------------------------------------------------------------------------------
vtkLookupTable* vtkImplicitImageRepresentation::CreateDefaultLookupTable()
{
  vtkLookupTable* lut = vtkLookupTable::New();
  lut->Register(this);
  lut->Delete();
  lut->SetNumberOfColors(256);
  lut->SetHueRange(0, 0);
  lut->SetSaturationRange(0, 0);
  lut->SetValueRange(0, 1);
  lut->SetAlphaRange(1, 1);
  lut->Build();
  return lut;
}

//------------------------------------------------------------------------------
void vtkImplicitImageRepresentation::GenerateTexturePlane()
{
  this->TextureMapToPlane = vtkTextureMapToPlane::New();
  this->TextureMapToPlane->AutomaticPlaneGenerationOff();
  this->TextureMapToPlane->SetOrigin(this->PlaneSource->GetOrigin());
  this->TextureMapToPlane->SetPoint1(this->PlaneSource->GetPoint1());
  this->TextureMapToPlane->SetPoint2(this->PlaneSource->GetPoint2());

  // Modify superclasses' pipeline to add in texture mapping
  this->TextureMapToPlane->SetInputConnection(this->Cutter->GetOutputPort());
  this->CutMapper->SetInputConnection(this->TextureMapToPlane->GetOutputPort());
  this->Edges->SetInputConnection(this->Cutter->GetOutputPort());

  this->SetResliceInterpolate(this->ResliceInterpolate);

  this->LookupTable = this->CreateDefaultLookupTable();

  this->ColorMap->SetLookupTable(this->LookupTable);
  this->ColorMap->SetOutputFormatToRGBA();
  this->ColorMap->PassAlphaToOutputOn();

  this->Texture->SetQualityTo32Bit();
  this->Texture->SetColorMode(VTK_COLOR_MODE_DEFAULT);
  this->Texture->SetInterpolate(this->TextureInterpolate);
  this->Texture->RepeatOff();
  this->Texture->SetLookupTable(this->LookupTable);

  // Note using the superclasses' actor for texturing, this may mean
  // modifying the pipeline.
  this->CutActor->SetTexture(this->Texture);
}

//------------------------------------------------------------------------------
void vtkImplicitImageRepresentation::UpdatePlane()
{
  if (!this->Reslice || !this->ImageData)
  {
    return;
  }

  // Calculate appropriate pixel spacing for the reslicing
  //
  vtkAlgorithm* inpAlg = this->Reslice->GetInputAlgorithm();
  inpAlg->UpdateInformation();
  vtkInformation* outInfo = inpAlg->GetOutputInformation(0);
  double spacing[3];
  outInfo->Get(vtkDataObject::SPACING(), spacing);
  double origin[3];
  outInfo->Get(vtkDataObject::ORIGIN(), origin);
  int extent[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);

  int i;

  for (i = 0; i < 3; i++)
  {
    if (extent[2 * i] > extent[2 * i + 1])
    {
      vtkErrorMacro("Invalid extent [" << extent[0] << ", " << extent[1] << ", " << extent[2]
                                       << ", " << extent[3] << ", " << extent[4] << ", "
                                       << extent[5] << "]."
                                       << " Perhaps the input data is empty?");
      break;
    }
  }

  // Update texture coordinate generation
  this->TextureMapToPlane->SetOrigin(this->PlaneSource->GetOrigin());
  this->TextureMapToPlane->SetPoint1(this->PlaneSource->GetPoint1());
  this->TextureMapToPlane->SetPoint2(this->PlaneSource->GetPoint2());

  // Get the plane axes and related informaion
  double planeAxis1[3], planeAxis2[3];
  this->PlaneSource->GetAxis1(planeAxis1);
  this->PlaneSource->GetAxis2(planeAxis2);

  // The x,y dimensions of the plane
  //
  double planeSizeX = vtkMath::Normalize(planeAxis1);
  double planeSizeY = vtkMath::Normalize(planeAxis2);

  double normal[3];
  this->PlaneSource->GetNormal(normal);

  // Generate the slicing matrix
  //
  this->ResliceAxes->Identity();
  for (i = 0; i < 3; i++)
  {
    this->ResliceAxes->SetElement(0, i, planeAxis1[i]);
    this->ResliceAxes->SetElement(1, i, planeAxis2[i]);
    this->ResliceAxes->SetElement(2, i, normal[i]);
  }

  double planeOrigin[4];
  this->PlaneSource->GetOrigin(planeOrigin);

  planeOrigin[3] = 1.0;

  this->ResliceAxes->Transpose();
  this->ResliceAxes->SetElement(0, 3, planeOrigin[0]);
  this->ResliceAxes->SetElement(1, 3, planeOrigin[1]);
  this->ResliceAxes->SetElement(2, 3, planeOrigin[2]);

  this->Reslice->SetResliceAxes(this->ResliceAxes);

  double spacingX = fabs(planeAxis1[0] * spacing[0]) + fabs(planeAxis1[1] * spacing[1]) +
    fabs(planeAxis1[2] * spacing[2]);

  double spacingY = fabs(planeAxis2[0] * spacing[0]) + fabs(planeAxis2[1] * spacing[1]) +
    fabs(planeAxis2[2] * spacing[2]);

  // Pad extent up to a power of two for efficient texture mapping

  // make sure we're working with valid values
  double realExtentX = (spacingX == 0) ? VTK_INT_MAX : planeSizeX / spacingX;

  int extentX;
  // Sanity check the input data:
  // * if realExtentX is too large, extentX will wrap
  // * if spacingX is 0, things will blow up.
  if (realExtentX > (VTK_INT_MAX >> 1))
  {
    vtkErrorMacro(<< "Invalid X extent: " << realExtentX);
    extentX = 0;
  }
  else
  {
    extentX = 1;
    while (extentX < realExtentX)
    {
      extentX = extentX << 1;
    }
  }

  // make sure extentY doesn't wrap during padding
  double realExtentY = (spacingY == 0) ? VTK_INT_MAX : planeSizeY / spacingY;

  int extentY;
  if (realExtentY > (VTK_INT_MAX >> 1))
  {
    vtkErrorMacro(<< "Invalid Y extent: " << realExtentY);
    extentY = 0;
  }
  else
  {
    extentY = 1;
    while (extentY < realExtentY)
    {
      extentY = extentY << 1;
    }
  }

  double outputSpacingX = (extentX == 0) ? 1.0 : planeSizeX / extentX;
  double outputSpacingY = (extentY == 0) ? 1.0 : planeSizeY / extentY;
  this->Reslice->SetOutputSpacing(outputSpacingX, outputSpacingY, 1);
  this->Reslice->SetOutputOrigin(0.5 * outputSpacingX, 0.5 * outputSpacingY, 0);
  this->Reslice->SetOutputExtent(0, extentX - 1, 0, extentY - 1, 0, 0);
}

//------------------------------------------------------------------------------
void vtkImplicitImageRepresentation::BuildRepresentation()
{
  // Make sure we're in a valid state
  if (!this->Renderer || !this->Renderer->GetRenderWindow())
  {
    return;
  }

  // Build the geometry
  this->Superclass::BuildRepresentation();

  // Now setup the pipeline
  this->UpdatePlane();
}

//------------------------------------------------------------------------------
void vtkImplicitImageRepresentation::SetCropPlaneToBoundingBox(bool val)
{
  if (this->CropPlaneToBoundingBox == val)
  {
    return;
  }

  this->CropPlaneToBoundingBox = val;
  if (val)
  {
    this->TextureMapToPlane->SetInputConnection(this->Cutter->GetOutputPort());
    this->CutMapper->SetInputConnection(this->TextureMapToPlane->GetOutputPort());
    this->Edges->SetInputConnection(this->Cutter->GetOutputPort());
  }
  else
  {
    this->CutMapper->SetInputConnection(this->PlaneSource->GetOutputPort());
    this->Edges->SetInputConnection(this->PlaneSource->GetOutputPort());
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkImplicitImageRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Reslice Interpolate: " << this->ResliceInterpolate << "\n";
  os << indent
     << "User Controlled Lookup Table: " << (this->UserControlledLookupTable ? "On\n" : "Off\n");

  os << indent
     << "User Controlled Lookup Table: " << (this->UserControlledLookupTable ? "On\n" : "Off\n");
  if (this->LookupTable)
  {
    os << indent << "LookupTable:\n";
    this->LookupTable->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "LookupTable: (none)\n";
  }

  if (this->ColorMap)
  {
    os << indent << "ColorMap:\n";
    this->ColorMap->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "ColorMap: (none)\n";
  }
}
