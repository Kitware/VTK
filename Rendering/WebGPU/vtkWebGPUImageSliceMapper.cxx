// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUImageSliceMapper.h"

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkInformation.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTexture.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUImageSliceMapper);

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkWebGPUImageSliceMapper::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
vtkWebGPUImageSliceMapper::vtkWebGPUImageSliceMapper()
{
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(4);
  this->Quad->SetPoints(points);

  vtkNew<vtkCellArray> triangles;
  triangles->InsertNextCell({ 0, 1, 2 });
  triangles->InsertNextCell({ 0, 2, 3 });
  this->Quad->SetPolys(triangles);

  vtkNew<vtkFloatArray> tcoords;
  tcoords->SetNumberOfComponents(2);
  tcoords->SetNumberOfTuples(4);
  this->Quad->GetPointData()->SetTCoords(tcoords);

  // The factory hands back the WebGPU implementations of these, the same way it
  // handed back this mapper.
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(this->Quad);
  this->ProxyActor->SetMapper(mapper);
  this->ProxyActor->SetTexture(this->Texture);
  // An image slice is not lit: its colours come from the property's colour
  // window/level and lookup table, which are already baked into the texture.
  this->ProxyActor->GetProperty()->SetAmbient(1.0);
  this->ProxyActor->GetProperty()->SetDiffuse(0.0);
  this->ProxyActor->GetProperty()->SetSpecular(0.0);
}

//------------------------------------------------------------------------------
vtkWebGPUImageSliceMapper::~vtkWebGPUImageSliceMapper() = default;

//------------------------------------------------------------------------------
void vtkWebGPUImageSliceMapper::ReleaseGraphicsResources(vtkWindow* window)
{
  this->ProxyActor->ReleaseGraphicsResources(window);
  this->Texture->ReleaseGraphicsResources(window);
  this->BuildTime.Modified();
  this->BuiltExtent[1] = this->BuiltExtent[0] - 1;
  this->Superclass::ReleaseGraphicsResources(window);
}

//------------------------------------------------------------------------------
void vtkWebGPUImageSliceMapper::BuildTexturedQuad(vtkImageSlice* prop)
{
  vtkImageData* input = this->GetInput();
  vtkImageProperty* property = prop->GetProperty();

  int extent[6];
  std::copy(this->DisplayExtent, this->DisplayExtent + 6, extent);

  // MakeTextureData reports back through these what it produced, and reads them
  // to decide what it can avoid doing. reuseData must go in as true: it is what
  // lets the method keep the input's own component count instead of expanding to
  // RGBA, and passing false leaves it reporting four bytes per pixel while the
  // copy it performs writes the input's three - which shears every row.
  int xsize = this->TextureSize[0];
  int ysize = this->TextureSize[1];
  int bytesPerPixel = this->TextureBytesPerPixel;
  bool reuseTexture = true;
  bool reuseData = true;
  unsigned char* data = this->MakeTextureData(
    property, input, extent, xsize, ysize, bytesPerPixel, reuseTexture, reuseData);
  if (data == nullptr || xsize <= 0 || ysize <= 0 || bytesPerPixel <= 0)
  {
    vtkErrorMacro(<< "Could not build texture data for the image slice.");
    return;
  }

  vtkNew<vtkImageData> textureImage;
  textureImage->SetDimensions(xsize, ysize, 1);
  vtkNew<vtkUnsignedCharArray> scalars;
  scalars->SetNumberOfComponents(bytesPerPixel);
  // When reuseData is set the block belongs to the input image and must not be
  // freed; otherwise MakeTextureData allocated it for us and the array takes it
  // over.
  scalars->SetArray(data, static_cast<vtkIdType>(xsize) * ysize * bytesPerPixel,
    /*save=*/reuseData ? 1 : 0);
  textureImage->GetPointData()->SetScalars(scalars);
  this->Texture->SetInputData(textureImage);
  this->Texture->SetInterpolate(property->GetInterpolationType() != VTK_NEAREST_INTERPOLATION);
  this->Texture->RepeatOff();
  this->Texture->EdgeClampOn();

  // The quad that carries the texture, in data coordinates.
  double coords[12];
  double tcoords[8];
  this->MakeTextureGeometry(extent, coords, tcoords);

  vtkPoints* points = this->Quad->GetPoints();
  auto* quadTCoords = vtkFloatArray::SafeDownCast(this->Quad->GetPointData()->GetTCoords());
  for (int i = 0; i < 4; ++i)
  {
    points->SetPoint(i, coords[3 * i], coords[3 * i + 1], coords[3 * i + 2]);
    quadTCoords->SetTypedComponent(i, 0, static_cast<float>(tcoords[2 * i]));
    quadTCoords->SetTypedComponent(i, 1, static_cast<float>(tcoords[2 * i + 1]));
  }
  points->Modified();
  quadTCoords->Modified();
  this->Quad->Modified();

  this->TextureSize[0] = xsize;
  this->TextureSize[1] = ysize;
  this->TextureBytesPerPixel = bytesPerPixel;
  std::copy(extent, extent + 6, this->BuiltExtent);
  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkWebGPUImageSliceMapper::Render(vtkRenderer* renderer, vtkImageSlice* prop)
{
  vtkImageData* input = this->GetInput();
  if (input == nullptr)
  {
    return;
  }
  vtkImageProperty* property = prop->GetProperty();
  if (property == nullptr)
  {
    return;
  }
  // vtkImageStack renders its slices in three passes and asks for one of matte,
  // colour or depth at a time. Only the colour pass draws anything here; the
  // other two exist to feed OpenGL depth peeling.
  if (!this->ColorEnable)
  {
    return;
  }

  // The base class helpers below read these, and they follow the input.
  input->GetSpacing(this->DataSpacing);
  input->GetOrigin(this->DataOrigin);
  vtkMatrix3x3::DeepCopy(this->DataDirection, input->GetDirectionMatrix());
  if (vtkInformation* inputInfo = this->GetInputInformation(0, 0))
  {
    inputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->DataWholeExtent);
  }

  const bool extentChanged =
    !std::equal(this->DisplayExtent, this->DisplayExtent + 6, this->BuiltExtent);
  if (extentChanged || this->BuildTime < input->GetMTime() ||
    this->BuildTime < property->GetMTime() || this->BuildTime < this->GetMTime())
  {
    this->BuildTexturedQuad(prop);
  }

  // Where the slice sits, and how it is shaded, come from the prop.
  this->ProxyActor->SetUserMatrix(this->GetDataToWorldMatrix());
  this->ProxyActor->GetProperty()->SetOpacity(property->GetOpacity());
  if (prop->GetPropertyKeys())
  {
    this->ProxyActor->SetPropertyKeys(prop->GetPropertyKeys());
  }

  if (this->ProxyActor->HasTranslucentPolygonalGeometry())
  {
    this->ProxyActor->RenderTranslucentPolygonalGeometry(renderer);
  }
  else
  {
    this->ProxyActor->RenderOpaqueGeometry(renderer);
  }
  this->TimeToDraw = 0.0001;
}

//------------------------------------------------------------------------------
void vtkWebGPUImageSliceMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ProxyActor: " << this->ProxyActor << '\n';
  os << indent << "Texture: " << this->Texture << '\n';
}
VTK_ABI_NAMESPACE_END
