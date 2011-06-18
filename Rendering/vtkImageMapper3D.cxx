/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapper3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageMapper3D.h"

#include "vtkRenderer.h"
#include "vtkImageSlice.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPlane.h"
#include "vtkAbstractTransform.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkGraphicsFactory.h"
#include "vtkTemplateAliasMacro.h"

//----------------------------------------------------------------------------
vtkImageMapper3D::vtkImageMapper3D()
{
  // Build a default greyscale lookup table
  this->DefaultLookupTable = vtkLookupTable::New();
  this->DefaultLookupTable->SetRampToLinear();
  this->DefaultLookupTable->SetValueRange(0.0, 1.0);
  this->DefaultLookupTable->SetSaturationRange(0.0, 0.0);
  this->DefaultLookupTable->SetAlphaRange(1.0, 1.0);
  this->DefaultLookupTable->Build();
  this->DefaultLookupTable->SetVectorModeToRGBColors();

  this->Border = 0;

  this->SlicePlane = vtkPlane::New();
  this->SliceFacesCamera = 0;
  this->SliceAtFocalPoint = 0;

  this->DataToWorldMatrix = vtkMatrix4x4::New();
  this->CurrentProp = 0;
  this->CurrentRenderer = 0;

  this->MatteEnable = true;
  this->ColorEnable = true;
  this->DepthEnable = true;

  this->DataOrigin[0] = 0.0;
  this->DataOrigin[1] = 0.0;
  this->DataOrigin[2] = 0.0;

  this->DataSpacing[0] = 1.0;
  this->DataSpacing[1] = 1.0;
  this->DataSpacing[2] = 1.0;

  this->DataWholeExtent[0] = 0;
  this->DataWholeExtent[1] = 0;
  this->DataWholeExtent[2] = 0;
  this->DataWholeExtent[3] = 0;
  this->DataWholeExtent[4] = 0;
  this->DataWholeExtent[5] = 0;
}

//----------------------------------------------------------------------------
vtkImageMapper3D::~vtkImageMapper3D()
{
  if (this->DefaultLookupTable)
    {
    this->DefaultLookupTable->Delete();
    }
  if (this->SlicePlane)
    {
    this->SlicePlane->Delete();
    }
  if (this->DataToWorldMatrix)
    {
    this->DataToWorldMatrix->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkImageMapper3D::SetInput(vtkImageData *input)
{
  if (input)
    {
    this->SetInputConnection(0, input->GetProducerPort());
    }
  else
    {
    // Setting a NULL input removes the connection.
    this->SetInputConnection(0, 0);
    }
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageMapper3D::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return 0;
    }
  return vtkImageData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
void vtkImageMapper3D::ReleaseGraphicsResources(vtkWindow *)
{
  // see subclass for implementation
}

//----------------------------------------------------------------------------
void vtkImageMapper3D::Render(vtkRenderer *, vtkImageSlice *)
{
  // see subclass for implementation
}

//----------------------------------------------------------------------------
int vtkImageMapper3D::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                this->DataWholeExtent);
    inInfo->Get(vtkDataObject::SPACING(), this->DataSpacing);
    inInfo->Get(vtkDataObject::ORIGIN(), this->DataOrigin);

    return 1;
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkImageMapper3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "SlicePlane: " << this->SlicePlane << "\n";
  os << indent << "SliceAtFocalPoint: "
     << (this->SliceAtFocalPoint ? "On\n" : "Off\n");
  os << indent << "SliceFacesCamera: "
     << (this->SliceFacesCamera ? "On\n" : "Off\n");
  os << indent << "Border: " << (this->Border ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
vtkDataObject *vtkImageMapper3D::GetDataObjectInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return 0;
    }
  return this->GetInputDataObject(0, 0);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkImageMapper3D::GetDataSetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return 0;
    }
  return vtkDataSet::SafeDownCast(this->GetInputDataObject(0, 0));
}

//----------------------------------------------------------------------------
int vtkImageMapper3D::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageMapper3D::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
static
vtkRenderer *vtkImageMapper3DFindRenderer(vtkProp *prop, int &count)
{
  vtkRenderer *ren = 0;

  int n = prop->GetNumberOfConsumers();
  for (int i = 0; i < n; i++)
    {
    vtkObjectBase *o = prop->GetConsumer(i);
    vtkProp3D *a = 0;
    if ( (ren = vtkRenderer::SafeDownCast(o)) )
      {
      count++;
      }
    else if ( (a = vtkProp3D::SafeDownCast(o)) )
      {
      ren = vtkImageMapper3DFindRenderer(a, count);
      }
    }

  return ren;
}

//----------------------------------------------------------------------------
static
void vtkImageMapper3DComputeMatrix(vtkProp *prop, double mat[16])
{
  vtkMatrix4x4 *propmat = prop->GetMatrix();
  vtkMatrix4x4::DeepCopy(mat, propmat);

  int n = prop->GetNumberOfConsumers();
  for (int i = 0; i < n; i++)
    {
    vtkObjectBase *o = prop->GetConsumer(i);
    vtkProp3D *a = 0;
    if ( (a = vtkProp3D::SafeDownCast(o)) )
      {
      vtkImageMapper3DComputeMatrix(a, mat);
      if (a->IsA("vtkAssembly") || a->IsA("vtkImageStack"))
        {
        vtkMatrix4x4::Multiply4x4(mat, *propmat->Element, mat);
        }
      }
    }
}

//----------------------------------------------------------------------------
vtkRenderer *vtkImageMapper3D::GetCurrentRenderer()
{
  vtkImageSlice *prop = this->CurrentProp;
  vtkRenderer *ren = this->CurrentRenderer;
  int count = 0;

  if (ren)
    {
    return ren;
    }

  if (!prop)
    {
    return 0;
    }

  ren = vtkImageMapper3DFindRenderer(prop, count);

  if (count > 1)
    {
    vtkErrorMacro("Cannot follow camera, mapper is associated with"
                  "multiple renderers");
    ren = 0;
    }

  return ren;
}

//----------------------------------------------------------------------------
vtkMatrix4x4 *vtkImageMapper3D::GetDataToWorldMatrix()
{
  vtkProp3D *prop = this->CurrentProp;

  if (prop)
    {
    if (this->CurrentRenderer)
      {
      this->DataToWorldMatrix->DeepCopy(prop->GetMatrix());
      }
    else
      {
      double mat[16];
      vtkImageMapper3DComputeMatrix(prop, mat);
      this->DataToWorldMatrix->DeepCopy(mat);
      }
    }

  return this->DataToWorldMatrix;
}

//----------------------------------------------------------------------------
// Subdivide the image until the pieces fit into texture memory
void vtkImageMapper3D::RecursiveRenderTexturedPolygon(
  vtkRenderer *ren, vtkImageProperty *property,
  vtkImageData *input, int extent[6], bool recursive)
{
  int xdim, ydim;
  int imageSize[2];
  int textureSize[2];

  // compute image size and texture size from extent
  this->ComputeTextureSize(
    extent, xdim, ydim, imageSize, textureSize);

  // Check if we can fit this texture in memory
  if (this->TextureSizeOK(textureSize))
    {
    // We can fit it - render
    this->RenderTexturedPolygon(
      ren, property, input, extent, recursive);
    }

  // If the texture does not fit, then subdivide and render
  // each half.  Unless the graphics card couldn't handle
  // a texture a small as 256x256, because if it can't handle
  // that, then something has gone horribly wrong.
  else if (textureSize[0] > 256 || textureSize[1] > 256)
    {
    int subExtent[6];
    subExtent[0] = extent[0]; subExtent[1] = extent[1];
    subExtent[2] = extent[2]; subExtent[3] = extent[3];
    subExtent[4] = extent[4]; subExtent[5] = extent[5];

    // Which is larger, x or y?
    int idx = ydim;
    int tsize = textureSize[1];
    if (textureSize[0] > textureSize[1])
      {
      idx = xdim;
      tsize = textureSize[0];
      }

    // Divide size by two
    tsize /= 2;

    // Render each half recursively
    subExtent[idx*2] = extent[idx*2];
    subExtent[idx*2 + 1] = extent[idx*2] + tsize - 1;
    this->RecursiveRenderTexturedPolygon(
      ren, property, input, subExtent, true);

    subExtent[idx*2] = subExtent[idx*2] + tsize;
    subExtent[idx*2 + 1] = extent[idx*2 + 1];
    this->RecursiveRenderTexturedPolygon(
      ren, property, input, subExtent, true);
    }
}

//----------------------------------------------------------------------------
void vtkImageMapper3D::RenderTexturedPolygon(
  vtkRenderer *, vtkImageProperty *, vtkImageData *, int [6], bool)
{
  // implemented in subclasses
}

//----------------------------------------------------------------------------
bool vtkImageMapper3D::TextureSizeOK(const int [2])
{
  // implemented in subclasses
  return true;
}

//----------------------------------------------------------------------------
// Convert char data without changing format
static
void vtkImageMapperCopy(
  const unsigned char *inPtr, unsigned char *outPtr, const int extent[6],
  int numComp, int inIncY, int inIncZ, int outIncY, int outIncZ)
{
  // number of values per row of input image
  int rowLength = extent[1] - extent[0] + 1;
  if (rowLength <= 0)
    {
    return;
    }

  // loop through the data and copy it for the texture
  if (numComp == 1)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          *outPtr++ = *inPtr++;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 2)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          outPtr[0] = inPtr[0];
          outPtr[1] = inPtr[1];
          outPtr += 2;
          inPtr += 2;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 3)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          outPtr[0] = inPtr[0];
          outPtr[1] = inPtr[1];
          outPtr[2] = inPtr[2];
          outPtr += 3;
          inPtr += 3;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else // if (numComp == 4)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          outPtr[0] = inPtr[0];
          outPtr[1] = inPtr[1];
          outPtr[2] = inPtr[2];
          outPtr[3] = inPtr[3];
          outPtr += 4;
          inPtr += numComp;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
}

//----------------------------------------------------------------------------
// Convert char data to RGBA
static
void vtkImageMapperConvertToRGBA(
  unsigned char *inPtr, unsigned char *outPtr, const int extent[6],
  int numComp, int inIncY, int inIncZ, int outIncY, int outIncZ)
{
  // number of values per row of input image
  int rowLength = extent[1] - extent[0] + 1;
  if (rowLength <= 0)
    {
    return;
    }

  unsigned char alpha = 255;

  // loop through the data and copy it for the texture
  if (numComp == 1)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          unsigned char val = inPtr[0];
          outPtr[0] = val;
          outPtr[1] = val;
          outPtr[2] = val;
          outPtr[3] = alpha;
          outPtr += 4;
          inPtr++;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 2)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          unsigned char val = inPtr[0];
          unsigned char a = inPtr[1];
          outPtr[0] = val;
          outPtr[1] = val;
          outPtr[2] = val;
          outPtr[3] = a;
          outPtr += 4;
          inPtr += 2;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 3)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          outPtr[0] = inPtr[0];
          outPtr[1] = inPtr[1];
          outPtr[2] = inPtr[2];
          outPtr[3] = alpha;
          outPtr += 4;
          inPtr += 3;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else // if (numComp == 4)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          outPtr[0] = inPtr[0];
          outPtr[1] = inPtr[1];
          outPtr[2] = inPtr[2];
          outPtr[3] = inPtr[3];
          outPtr += 4;
          inPtr += numComp;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
}

//----------------------------------------------------------------------------
// Convert data to unsigned char

template<class F>
inline F vtkImageMapperClamp(F x, F xmin, F xmax)
{
  // do not change this code: it compiles into min/max opcodes
  x = (x > xmin ? x : xmin);
  x = (x < xmax ? x : xmax);
  return x;
}

template<class F, class T>
void vtkImageMapperShiftScale(
  const T *inPtr, unsigned char *outPtr, const int extent[6],
  int numComp, int inIncY, int inIncZ, int outIncY, int outIncZ,
  F shift, F scale)
{
  const F vmin = static_cast<F>(0);
  const F vmax = static_cast<F>(255);

  // number of values per row of input image
  int rowLength = (extent[1] - extent[0] + 1);
  if (rowLength <= 0)
    {
    return;
    }

  unsigned char alpha = 255;

  // loop through the data and copy it for the texture
  if (numComp == 1)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          // Pixel operation
          F val = (inPtr[0] + shift)*scale;
          val = vtkImageMapperClamp(val, vmin, vmax);
          unsigned char cval = static_cast<unsigned char>(val + 0.5);
          outPtr[0] = cval;
          outPtr[1] = cval;
          outPtr[2] = cval;
          outPtr[3] = alpha;
          outPtr += 4;
          inPtr++;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 2)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          // Pixel operation
          F val = (inPtr[0] + shift)*scale;
          val = vtkImageMapperClamp(val, vmin, vmax);
          unsigned char cval = static_cast<unsigned char>(val + 0.5);
          val = (inPtr[1] + shift)*scale;
          val = vtkImageMapperClamp(val, vmin, vmax);
          unsigned char aval = static_cast<unsigned char>(val + 0.5);
          outPtr[0] = cval;
          outPtr[1] = cval;
          outPtr[2] = cval;
          outPtr[3] = aval;
          outPtr += 4;
          inPtr += 2;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 3)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          // Pixel operation
          F r = (inPtr[0] + shift)*scale;
          F g = (inPtr[1] + shift)*scale;
          F b = (inPtr[2] + shift)*scale;
          r = vtkImageMapperClamp(r, vmin, vmax);
          g = vtkImageMapperClamp(g, vmin, vmax);
          b = vtkImageMapperClamp(b, vmin, vmax);
          outPtr[0] = static_cast<unsigned char>(r + 0.5);
          outPtr[1] = static_cast<unsigned char>(g + 0.5);
          outPtr[2] = static_cast<unsigned char>(b + 0.5);
          outPtr[3] = alpha;
          outPtr += 4;
          inPtr += 3;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else // if (numComp == 4)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          // Pixel operation
          F r = (inPtr[0] + shift)*scale;
          F g = (inPtr[1] + shift)*scale;
          F b = (inPtr[2] + shift)*scale;
          F a = (inPtr[3] + shift)*scale;
          r = vtkImageMapperClamp(r, vmin, vmax);
          g = vtkImageMapperClamp(g, vmin, vmax);
          b = vtkImageMapperClamp(b, vmin, vmax);
          a = vtkImageMapperClamp(a, vmin, vmax);
          outPtr[0] = static_cast<unsigned char>(r + 0.5);
          outPtr[1] = static_cast<unsigned char>(g + 0.5);
          outPtr[2] = static_cast<unsigned char>(b + 0.5);
          outPtr[3] = static_cast<unsigned char>(a + 0.5);
          outPtr += 4;
          inPtr += numComp;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageMapper3D::ConvertImageScalarsToRGBA(
  void *inPtr, unsigned char *outPtr, const int extent[6],
  int numComp, int inIncY, int inIncZ, int outIncY, int outIncZ,
  int scalarType, double scalarRange[2])
{
  double shift = -scalarRange[0];
  double scale = 255.0;

  if (scalarRange[0] < scalarRange[1])
    {
    scale /= (scalarRange[1] - scalarRange[0]);
    }
  else
    {
    scale = 1e+32;
    }

  // Check if the data can be simply copied
  if (scalarType == VTK_UNSIGNED_CHAR &&
      static_cast<int>(shift*scale) == 0 &&
      static_cast<int>((255 + shift)*scale) == 255)
    {
    vtkImageMapperConvertToRGBA(static_cast<unsigned char *>(inPtr),
                                outPtr, extent, numComp,
                                inIncY, inIncZ, outIncY, outIncZ);
    }
  else
    {
    switch (scalarType)
      {
      vtkTemplateAliasMacro(
        vtkImageMapperShiftScale(static_cast<VTK_TT*>(inPtr),
                                 outPtr, extent, numComp,
                                 inIncY, inIncZ, outIncY, outIncZ,
                                 shift, scale));
      default:
        vtkGenericWarningMacro(
          "ConvertImageScalarsToRGBA: Unknown input ScalarType");
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageMapper3D::ApplyLookupTableToImageScalars(
  void *inPtr, unsigned char *outPtr, const int extent[6],
  int numComp, int inIncY, int inIncZ, int outIncY, int outIncZ,
  int scalarType, vtkScalarsToColors *lookupTable)
{
  // number of values per row of input image
  int rowLength = extent[1] - extent[0] + 1;
  int scalarSize = vtkDataArray::GetDataTypeSize(scalarType);

  // convert incY from continuous increment to regular increment
  outIncY += 4*rowLength;
  inIncY += numComp*rowLength;
  inIncY *= scalarSize;
  inIncZ *= scalarSize;

  // loop through the data and copy it for the texture
  for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
    {
    for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
      {
      if (numComp == 1)
        {
        lookupTable->MapScalarsThroughTable(
          inPtr, outPtr, scalarType, rowLength, numComp, VTK_RGBA);
        }
      else
        {
        lookupTable->MapVectorsThroughTable(
          inPtr, outPtr, scalarType, rowLength, numComp, VTK_RGBA);
        }

      outPtr += outIncY;
      inPtr = static_cast<void *>(static_cast<char *>(inPtr) + inIncY);
      }
    outPtr += outIncZ;
    inPtr = static_cast<void *>(static_cast<char *>(inPtr) + inIncZ);
    }
}

//----------------------------------------------------------------------------
void vtkImageMapper3D::CheckerboardRGBA(
  unsigned char *data, int xsize, int ysize,
  double originx, double originy, double spacingx, double spacingy)
{
  static double tol = 7.62939453125e-06;
  static double maxval = 2147483647;
  static double minval = -2147483647;

  originx += 1.0 + tol;
  originy += 1.0 + tol;

  originx = (originx > minval ? originx : minval);
  originx = (originx < maxval ? originx : maxval);
  originy = (originy > minval ? originy : minval);
  originy = (originy < maxval ? originy : maxval);

  spacingx = fabs(spacingx);
  spacingy = fabs(spacingy);

  spacingx = (spacingx < maxval ? spacingx : maxval);
  spacingy = (spacingy < maxval ? spacingy : maxval);
  spacingx = (spacingx != 0 ? spacingx : maxval);
  spacingy = (spacingy != 0 ? spacingy : maxval);

  int xn = static_cast<int>(spacingx + tol);
  int yn = static_cast<int>(spacingy + tol);
  double fx = spacingx - xn;
  double fy = spacingy - yn;

  int state = 0;
  int tmpstate = ~state;
  double spacing2x = 2*spacingx;
  double spacing2y = 2*spacingy;
  originx -= ceil(originx/spacing2x)*spacing2x;
  while (originx < 0) { originx += spacing2x; }
  originy -= ceil(originy/spacing2y)*spacing2y;
  while (originy < 0) { originy += spacing2y; }
  double tmporiginx = originx - spacingx;
  originx = (tmporiginx < 0 ? originx : tmporiginx);
  state = (tmporiginx < 0 ? state : tmpstate);
  tmpstate = ~state;
  double tmporiginy = originy - spacingy;
  originy = (tmporiginy < 0 ? originy : tmporiginy);
  state = (tmporiginy < 0 ? state : tmpstate);
  
  int xm = static_cast<int>(originx);
  int savexm = xm;
  int ym = static_cast<int>(originy);
  double gx = originx - xm;
  double savegx = gx;
  double gy = originy - ym;

  int inc = 4;
  data += (inc - 1);
  for (int j = 0; j < ysize;)
    {
    double tmpy = gy - 1.0;
    gy = (tmpy < 0 ? gy : tmpy);
    int yextra = (tmpy >= 0);
    ym += yextra;
    int ry = ysize - j;
    ym = (ym < ry ? ym : ry);
    j += ym;

    for (; ym; --ym)
      {
      tmpstate = state;
      xm = savexm;
      gx = savegx;

      for (int i = 0; i < xsize;)
        {
        double tmpx = gx - 1.0;
        gx = (tmpx < 0 ? gx : tmpx);
        int xextra = (tmpx >= 0);
        xm += xextra;
        int rx = xsize - i;
        xm = (xm < rx ? xm : rx);
        i += xm;
        if ( (tmpstate & xm) ) 
          {
          do
            {
            *data = 0;
            data += inc;
            }
          while (--xm);
          }
        data += inc*xm;
        xm = xn;
        tmpstate = ~tmpstate;
        gx += fx;
        }
      }

   ym = yn;
   state = ~state;
   gy += fy;
   }  
}

//----------------------------------------------------------------------------
// Given an image and an extent that describes a single slice, this method
// will return a contiguous block of unsigned char data that can be loaded
// into a texture.
// The values of xsize, ysize, bytesPerPixel, and reuseTexture must be
// pre-loaded with the current texture size and depth, with subTexture
// set to 1 if only a subTexture is to be generated.
// When the method returns, these values will be set to the dimensions
// of the data that was produced, and subTexture will remain set to 1
// if xsize,ysize describe a subtexture size.
// If subTexture is not set to one upon return, then xsize,ysize will
// describe the full texture size, with the assumption that the full
// texture must be reloaded.
// If reuseData is false upon return, then the returned array must be
// freed after use with delete [].
unsigned char *vtkImageMapper3D::MakeTextureData(
  vtkImageProperty *property, vtkImageData *input, int extent[6],
  int &xsize, int &ysize, int &bytesPerPixel, bool &reuseTexture,
  bool &reuseData)
{
  int xdim, ydim;
  int imageSize[2];
  int textureSize[2];

  // compute image size and texture size from extent
  this->ComputeTextureSize(
    extent, xdim, ydim, imageSize, textureSize);

  // number of components
  int numComp = input->GetNumberOfScalarComponents();
  int scalarType = input->GetScalarType();
  int textureBytesPerPixel = 4;

  // lookup table and window/level
  double colorWindow = 255.0;
  double colorLevel = 127.5;
  vtkScalarsToColors *lookupTable = 0;

  if (property)
    {
    colorWindow = property->GetColorWindow();
    colorLevel = property->GetColorLevel();
    lookupTable = property->GetLookupTable();
    }

  // check if the input is pre-formatted as colors
  int inputIsColors = false;
  if (lookupTable == 0 && scalarType == VTK_UNSIGNED_CHAR &&
      colorLevel == 127.5 && colorWindow == 255.0)
    {
    inputIsColors = true;
    if (reuseData && numComp < 4)
      {
      textureBytesPerPixel = numComp;
      }
    }

  // reuse texture if texture size has not changed
  if (xsize == textureSize[0] && ysize == textureSize[1] &&
      bytesPerPixel == textureBytesPerPixel && reuseTexture)
    {
    // if texture is reused, only reload the image portion
    xsize = imageSize[0];
    ysize = imageSize[1];
    }
  else
    {
    xsize = textureSize[0];
    ysize = textureSize[1];
    bytesPerPixel = textureBytesPerPixel;
    reuseTexture = false;
    }

  // if the image is already of the desired size and type
  if (xsize == imageSize[0] && ysize == imageSize[1])
    {
    // Check if the data needed for the texture is a contiguous region
    // of the input data: this requires that xdim = 0 and ydim = 1
    // OR xextent = 1 pixel and xdim = 1 and ydim = 2
    // OR xdim = 0 and ydim = 2 and yextent = 1 pixel.
    // In addition the corresponding x display extents must match the
    // extent of the data
    int *dataExtent = input->GetExtent();

    if ( (xdim == 0 && ydim == 1 &&
          extent[0] == dataExtent[0] && extent[1] == dataExtent[1]) ||
         (xdim == 1 && ydim == 2 && dataExtent[0] == dataExtent[1] &&
          extent[2] == dataExtent[2] && extent[3] == dataExtent[3]) ||
         (xdim == 0 && ydim == 2 && dataExtent[2] == dataExtent[3] &&
          extent[0] == dataExtent[0] && extent[1] == dataExtent[1]) )
      {
      // if contiguous and correct data type, use data as-is
      if (inputIsColors && reuseData)
        {
        return static_cast<unsigned char *>(
          input->GetScalarPointerForExtent(extent));
        }
      }
    }

  // could not directly use input data, so allocate a new array
  reuseData = false;

  unsigned char *outPtr = new unsigned char [ysize*xsize*bytesPerPixel];

  // output increments
  vtkIdType outIncY = bytesPerPixel*(xsize - imageSize[0]);
  vtkIdType outIncZ = 0;
  if (ydim == 2)
    {
    outIncZ = outIncY;
    outIncY = 0;
    }

  // input pointer and increments
  vtkIdType inIncX, inIncY, inIncZ;
  void *inPtr = input->GetScalarPointerForExtent(extent);
  input->GetContinuousIncrements(extent, inIncX, inIncY, inIncZ);

  // convert Window/Level to a scalar range
  double range[2];
  range[0] = colorLevel - 0.5*colorWindow;
  range[1] = colorLevel + 0.5*colorWindow;

  // reformat the data for use as a texture
  if (lookupTable)
    {
    // apply a lookup table
    if (property && !property->GetUseLookupTableScalarRange())
      {
      // no way to do this without modifying the table
      lookupTable->SetRange(range);
      }

    this->ApplyLookupTableToImageScalars(inPtr, outPtr, extent, numComp,
                                         inIncY, inIncZ, outIncY, outIncZ,
                                         scalarType, lookupTable);
    }
  else if (!inputIsColors) // no lookup table, do a shift/scale calculation
    {
    this->ConvertImageScalarsToRGBA(inPtr, outPtr, extent, numComp,
                                    inIncY, inIncZ, outIncY, outIncZ,
                                    scalarType, range);
    }
  else // just copy the data
    {
    vtkImageMapperCopy(static_cast<unsigned char *>(inPtr),
                       outPtr, extent, numComp,
                       inIncY, inIncZ, outIncY, outIncZ);
    }

  return outPtr;
}

//----------------------------------------------------------------------------
// Compute the coords and tcoords for the image
void vtkImageMapper3D::MakeTextureGeometry(
  vtkImageData *input, const int extent[6], int border,
  double coords[12], double tcoords[8])
{
  int xdim, ydim;
  int imageSize[2];
  int textureSize[2];

  // compute image size and texture size from extent
  this->ComputeTextureSize(
    extent, xdim, ydim, imageSize, textureSize);

  // get spacing/origin for the quad coordinates
  double *spacing = input->GetSpacing();
  double *origin = input->GetOrigin();

  // compute the world coordinates of the quad
  coords[0] = extent[0]*spacing[0] + origin[0];
  coords[1] = extent[2]*spacing[1] + origin[1];
  coords[2] = extent[4]*spacing[2] + origin[2];

  coords[3] = extent[1]*spacing[0] + origin[0];
  coords[4] = extent[2 + (xdim == 1)]*spacing[1] + origin[1];
  coords[5] = extent[4]*spacing[2] + origin[2];

  coords[6] = extent[1]*spacing[0] + origin[0];
  coords[7] = extent[3]*spacing[1] + origin[1];
  coords[8] = extent[5]*spacing[2] + origin[2];

  coords[9] = extent[0]*spacing[0] + origin[0];
  coords[10] = extent[2 + (ydim == 1)]*spacing[1] + origin[1];
  coords[11] = extent[5]*spacing[2] + origin[2];

  // stretch the geometry one half-pixel
  if (border)
    {
    coords[xdim] -= 0.5*spacing[xdim];
    coords[ydim] -= 0.5*spacing[ydim];
    coords[3 + xdim] += 0.5*spacing[xdim];
    coords[3 + ydim] -= 0.5*spacing[ydim];
    coords[6 + xdim] += 0.5*spacing[xdim];
    coords[6 + ydim] += 0.5*spacing[ydim];
    coords[9 + xdim] -= 0.5*spacing[xdim];
    coords[9 + ydim] += 0.5*spacing[ydim];
    }

  if (tcoords)
    {
    // compute the tcoords
    double textureBorder = 0.5*(border == 0);

    tcoords[0] = textureBorder/textureSize[0];
    tcoords[1] = textureBorder/textureSize[1];

    tcoords[2] = (imageSize[0] - textureBorder)/textureSize[0];
    tcoords[3] = tcoords[1];

    tcoords[4] = tcoords[2];
    tcoords[5] = (imageSize[1] - textureBorder)/textureSize[1];

    tcoords[6] = tcoords[0];
    tcoords[7] = tcoords[5];
    }
}

//----------------------------------------------------------------------------
void vtkImageMapper3D::ComputeTextureSize(
  const int extent[6], int &xdim, int &ydim,
  int imageSize[2], int textureSize[2])
{
  // find dimension indices that will correspond to the
  // columns and rows of the 2D texture
  xdim = 1;
  ydim = 2;
  if (extent[0] != extent[1])
    {
    xdim = 0;
    if (extent[2] != extent[3])
      {
      ydim = 1;
      }
    }

  // compute the image dimensions
  imageSize[0] = (extent[xdim*2+1] - extent[xdim*2] + 1);
  imageSize[1] = (extent[ydim*2+1] - extent[ydim*2] + 1);

  textureSize[0] = imageSize[0];
  textureSize[1] = imageSize[1];
}

//----------------------------------------------------------------------------
void vtkImageMapper3D::GetSlicePlaneInDataCoords(
  vtkMatrix4x4 *propMatrix, double normal[4])
{
  double point[3];
  this->SlicePlane->GetNormal(normal);
  this->SlicePlane->GetOrigin(point);

  // The plane has a transform, though most people forget
  vtkAbstractTransform *planeTransform = this->SlicePlane->GetTransform();
  if (planeTransform)
    {
    planeTransform->TransformNormalAtPoint(point, normal, normal);
    planeTransform->TransformPoint(point, point);
    }

  // Convert to a homogeneous normal in data coords
  normal[3] = -vtkMath::Dot(point, normal);

  // Transform to data coordinates
  if (propMatrix)
    {
    double mat[16];
    vtkMatrix4x4::Transpose(*propMatrix->Element, mat);
    vtkMatrix4x4::MultiplyPoint(mat, normal, normal);
    }

  // Normalize the "normal" part for good measure
  double l = vtkMath::Norm(normal);
  normal[0] /= l;
  normal[1] /= l;
  normal[2] /= l;
  normal[3] /= l;
}
