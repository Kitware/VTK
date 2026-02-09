// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLImageSliceMapper.h"

#include "vtk_glad.h"

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkHardwareSelector.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOverrideAttribute.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkScalarsToColors.h"
#include "vtkShaderProgram.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTexture.h"
#include "vtkTrivialProducer.h"

#include <cmath>

#include "vtkOpenGLError.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLImageSliceMapper);

/**
 * Specialized shader to encode cell/point IDs into an RGB texture.
 * The encoding uses 3 bytes (24 bits) to store the index.
 * The index is calculated based on the texture coordinates and the
 * image dimensions (used as stride).
 */
// ------------------------------------------------------------------------------------------------
// Vertex Shader
// ------------------------------------------------------------------------------------------------
static const char* selVS = R"(
  #version 150
  in vec3 vertexMC; // Incoming World Coordinates (e.g. 0..512)
  in vec2 tcoordMC; // Texture Coordinates (0..1)
  out vec2 tcoordVC;

  uniform mat4 MCDCMatrix; // Combined MVP Matrix

  void main()
  {
    tcoordVC = tcoordMC;
    // Force w=1.0 for the input vector to ensure translation applies
    gl_Position = MCDCMatrix * vec4(vertexMC, 1.0);
  }
)";

// ------------------------------------------------------------------------------------------------
// Fragment Shader
// ------------------------------------------------------------------------------------------------
static const char* selFS = R"(
  #version 150

  in vec2 tcoordVC;
  uniform vec2 imgDims;    // This should be the ID Stride dimensions
  out vec4 fragOutput;

  void main()
  {
    // Calculate index based on texture coordinates
    int i = int(floor(tcoordVC.x * imgDims.x));
    int j = int(floor(tcoordVC.y * imgDims.y));

    // Clamp to valid index range
    i = clamp(i, 0, int(imgDims.x) - 1);
    j = clamp(j, 0, int(imgDims.y) - 1);

    // Calculate linear ID using the correct stride
    // For Cells, imgDims.x is (Width - 1). For Points, it is Width.
    int pixelId = j * int(imgDims.x) + i;

    // Encode the index into RGB
    int idx = pixelId & 0xffffff;
    float r = float((idx) & 0xff) / 255.0;
    float g = float((idx >> 8) & 0xff) / 255.0;
    float b = float((idx >> 16) & 0xff) / 255.0;

    fragOutput = vec4(r, g, b, 1.0);
  }
)";

//------------------------------------------------------------------------------
// Initializes an instance, generates a unique index.
vtkOpenGLImageSliceMapper::vtkOpenGLImageSliceMapper()
{
  // setup the polygon mapper
  {
    vtkNew<vtkPolyData> polydata;
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(4);
    polydata->SetPoints(points);

    vtkNew<vtkCellArray> tris;
    polydata->SetPolys(tris);

    vtkNew<vtkFloatArray> tcoords;
    tcoords->SetNumberOfComponents(2);
    tcoords->SetNumberOfTuples(4);
    polydata->GetPointData()->SetTCoords(tcoords);

    vtkNew<vtkTrivialProducer> prod;
    prod->SetOutput(polydata);
    vtkNew<vtkOpenGLPolyDataMapper> polyDataMapper;
    polyDataMapper->SetInputConnection(prod->GetOutputPort());
    this->PolyDataActor = vtkActor::New();
    this->PolyDataActor->SetMapper(polyDataMapper);
    vtkNew<vtkTexture> texture;
    texture->RepeatOff();
    this->PolyDataActor->SetTexture(texture);
  }

  // setup the backing polygon mapper
  {
    vtkNew<vtkPolyData> polydata;
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(4);
    polydata->SetPoints(points);

    vtkNew<vtkCellArray> tris;
    polydata->SetPolys(tris);

    vtkNew<vtkTrivialProducer> prod;
    prod->SetOutput(polydata);
    vtkNew<vtkOpenGLPolyDataMapper> polyDataMapper;
    polyDataMapper->SetInputConnection(prod->GetOutputPort());
    this->BackingPolyDataActor = vtkActor::New();
    this->BackingPolyDataActor->SetMapper(polyDataMapper);
  }

  // setup the background polygon mapper
  {
    vtkNew<vtkPolyData> polydata;
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(10);
    polydata->SetPoints(points);

    vtkNew<vtkCellArray> tris;
    polydata->SetPolys(tris);

    vtkNew<vtkTrivialProducer> prod;
    prod->SetOutput(polydata);
    vtkNew<vtkOpenGLPolyDataMapper> polyDataMapper;
    polyDataMapper->SetInputConnection(prod->GetOutputPort());
    this->BackgroundPolyDataActor = vtkActor::New();
    this->BackgroundPolyDataActor->SetMapper(polyDataMapper);
  }

  this->RenderWindow = nullptr;
  this->TextureSize[0] = 0;
  this->TextureSize[1] = 0;
  this->TextureBytesPerPixel = 1;

  this->LastOrientation = -1;
  this->LastSliceNumber = VTK_INT_MAX;

  this->SelectionHelper = new vtkOpenGLHelper;
}

//------------------------------------------------------------------------------
vtkOpenGLImageSliceMapper::~vtkOpenGLImageSliceMapper()
{
  this->RenderWindow = nullptr;
  this->BackgroundPolyDataActor->UnRegister(this);
  this->BackingPolyDataActor->UnRegister(this);
  this->PolyDataActor->UnRegister(this);

  delete this->SelectionHelper;
  this->SelectionHelper = nullptr;
}

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkOpenGLImageSliceMapper::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "OpenGL", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
// Release the graphics resources used by this texture.
void vtkOpenGLImageSliceMapper::ReleaseGraphicsResources(vtkWindow* renWin)
{
  this->BackgroundPolyDataActor->ReleaseGraphicsResources(renWin);
  this->BackingPolyDataActor->ReleaseGraphicsResources(renWin);
  this->PolyDataActor->ReleaseGraphicsResources(renWin);

  this->RenderWindow = nullptr;
  this->Modified();
}

//------------------------------------------------------------------------------
// Subdivide the image until the pieces fit into texture memory
void vtkOpenGLImageSliceMapper::RecursiveRenderTexturedPolygon(
  vtkRenderer* ren, vtkImageProperty* property, vtkImageData* input, int extent[6], bool recursive)
{
  int xdim, ydim;
  int imageSize[2];
  int textureSize[2];

  // compute image size and texture size from extent
  this->ComputeTextureSize(extent, xdim, ydim, imageSize, textureSize);

  // Check if we can fit this texture in memory
  if (this->TextureSizeOK(textureSize, ren))
  {
    // We can fit it - render
    this->RenderTexturedPolygon(ren, property, input, extent, recursive);
  }

  // If the texture does not fit, then subdivide and render
  // each half.  Unless the graphics card couldn't handle
  // a texture a small as 256x256, because if it can't handle
  // that, then something has gone horribly wrong.
  else if (textureSize[0] > 256 || textureSize[1] > 256)
  {
    int subExtent[6];
    subExtent[0] = extent[0];
    subExtent[1] = extent[1];
    subExtent[2] = extent[2];
    subExtent[3] = extent[3];
    subExtent[4] = extent[4];
    subExtent[5] = extent[5];

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
    subExtent[idx * 2] = extent[idx * 2];
    subExtent[idx * 2 + 1] = extent[idx * 2] + tsize - 1;
    this->RecursiveRenderTexturedPolygon(ren, property, input, subExtent, true);

    subExtent[idx * 2] = subExtent[idx * 2] + tsize;
    subExtent[idx * 2 + 1] = extent[idx * 2 + 1];
    this->RecursiveRenderTexturedPolygon(ren, property, input, subExtent, true);
  }
}

//------------------------------------------------------------------------------
// Load the given image extent into a texture and render it
void vtkOpenGLImageSliceMapper::RenderTexturedPolygon(
  vtkRenderer* ren, vtkImageProperty* property, vtkImageData* input, int extent[6], bool recursive)
{
  // get the previous texture load time
  vtkMTimeType loadTime = this->LoadTime.GetMTime();

  // the render window, needed for state information
  vtkOpenGLRenderWindow* renWin = static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow());

  bool reuseTexture = true;

  // if context has changed, verify context capabilities
  if (renWin != this->RenderWindow || renWin->GetContextCreationTime() > loadTime)
  {
    this->RenderWindow = renWin;
    reuseTexture = false;
  }

  vtkOpenGLClearErrorMacro();

  // get information about the image
  int xdim, ydim; // orientation of texture wrt input image
  vtkImageSliceMapper::GetDimensionIndices(this->Orientation, xdim, ydim);

  // verify that the orientation and slice has not changed
  bool orientationChanged = (this->Orientation != this->LastOrientation);
  this->LastOrientation = this->Orientation;
  bool sliceChanged = (this->SliceNumber != this->LastSliceNumber);
  this->LastSliceNumber = this->SliceNumber;

  // get the mtime of the property, including the lookup table
  vtkMTimeType propertyMTime = 0;
  if (property)
  {
    propertyMTime = property->GetMTime();
    if (!this->PassColorData)
    {
      vtkScalarsToColors* table = property->GetLookupTable();
      if (table)
      {
        vtkMTimeType mtime = table->GetMTime();
        propertyMTime = std::max(mtime, propertyMTime);
      }
    }
  }

  // need to reload the texture
  if (this->Superclass::GetMTime() > loadTime || propertyMTime > loadTime ||
    input->GetMTime() > loadTime || orientationChanged || sliceChanged || recursive)
  {
    // get the data to load as a texture
    int xsize = this->TextureSize[0];
    int ysize = this->TextureSize[1];
    int bytesPerPixel = this->TextureBytesPerPixel;

    // whether to try to use the input data directly as the texture
    bool reuseData = true;

    // generate the data to be used as a texture
    unsigned char* data = this->MakeTextureData((this->PassColorData ? nullptr : property), input,
      extent, xsize, ysize, bytesPerPixel, reuseTexture, reuseData);

    this->TextureSize[0] = xsize;
    this->TextureSize[1] = ysize;
    this->TextureBytesPerPixel = bytesPerPixel;

    vtkImageData* id = vtkImageData::New();
    id->SetExtent(0, xsize - 1, 0, ysize - 1, 0, 0);
    vtkUnsignedCharArray* uca = vtkUnsignedCharArray::New();
    uca->SetNumberOfComponents(bytesPerPixel);
    // Use size_t to avoid integer overflow for large images
    uca->SetArray(data,
      static_cast<vtkIdType>(static_cast<size_t>(xsize) * static_cast<size_t>(ysize) *
        static_cast<size_t>(bytesPerPixel)),
      reuseData, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
    id->GetPointData()->SetScalars(uca);
    uca->Delete();

    this->PolyDataActor->GetTexture()->SetInputData(id);
    id->Delete();

    if (property->GetInterpolationType() == VTK_NEAREST_INTERPOLATION && !this->ExactPixelMatch)
    {
      this->PolyDataActor->GetTexture()->InterpolateOff();
    }
    else
    {
      this->PolyDataActor->GetTexture()->InterpolateOn();
    }

    this->PolyDataActor->GetTexture()->EdgeClampOn();

    // modify the load time to the current time
    this->LoadTime.Modified();
  }

  vtkPoints* points = this->Points;
  if (this->ExactPixelMatch && this->SliceFacesCamera)
  {
    points = nullptr;
  }

  this->RenderPolygon(this->PolyDataActor, points, extent, ren);

  if (this->Background)
  {
    double ambient = property->GetAmbient();
    double diffuse = property->GetDiffuse();

    double bkcolor[4];
    this->GetBackgroundColor(property, bkcolor);
    vtkProperty* pdProp = this->BackgroundPolyDataActor->GetProperty();
    pdProp->SetAmbient(ambient);
    pdProp->SetDiffuse(diffuse);
    pdProp->SetColor(bkcolor[0], bkcolor[1], bkcolor[2]);
    this->RenderBackground(this->BackgroundPolyDataActor, points, extent, ren);
  }

  vtkOpenGLCheckErrorMacro("failed after RenderTexturedPolygon");
}

//------------------------------------------------------------------------------
// Render the polygon that displays the image data
void vtkOpenGLImageSliceMapper::RenderPolygon(
  vtkActor* actor, vtkPoints* points, const int extent[6], vtkRenderer* ren)
{
  vtkOpenGLClearErrorMacro();

  bool textured = (actor->GetTexture() != nullptr);
  vtkPolyData* poly = vtkPolyDataMapper::SafeDownCast(actor->GetMapper())->GetInput();
  vtkPoints* polyPoints = poly->GetPoints();
  if (this->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION)
  {
    polyPoints->SetDataTypeToDouble();
  }
  vtkCellArray* tris = poly->GetPolys();
  vtkDataArray* polyTCoords = poly->GetPointData()->GetTCoords();

  // do we need to rebuild the cell array?
  int numTris = 2;
  if (points)
  {
    numTris = (points->GetNumberOfPoints() - 2);
  }
  if (tris->GetNumberOfConnectivityIds() != 3 * numTris)
  {
    tris->Initialize();
    tris->AllocateEstimate(numTris, 3);
    // this wacky code below works for 2 and 4 triangles at least
    for (vtkIdType i = 0; i < numTris; i++)
    {
      tris->InsertNextCell(3);
      tris->InsertCellPoint(numTris + 1 - (i + 1) / 2);
      tris->InsertCellPoint(i / 2);
      tris->InsertCellPoint((i % 2 == 0) ? numTris - i / 2 : i / 2 + 1);
    }
    tris->Modified();
  }

  // now rebuild the points/tcoords as needed
  if (!points)
  {
    double coords[12], tcoords[8];
    this->MakeTextureGeometry(extent, coords, tcoords);

    polyPoints->SetNumberOfPoints(4);
    if (textured)
    {
      polyTCoords->SetNumberOfTuples(4);
    }
    for (int i = 0; i < 4; i++)
    {
      polyPoints->SetPoint(i, coords[3 * i], coords[3 * i + 1], coords[3 * i + 2]);
      if (textured)
      {
        polyTCoords->SetTuple(i, &tcoords[2 * i]);
      }
    }
    polyPoints->Modified();
    if (textured)
    {
      polyTCoords->Modified();
    }
  }
  else if (points->GetNumberOfPoints())
  {
    int xdim, ydim;
    vtkImageSliceMapper::GetDimensionIndices(this->Orientation, xdim, ydim);
    double* origin = this->DataOrigin;
    double* spacing = this->DataSpacing;
    double xshift = -(0.5 - extent[2 * xdim]) * spacing[xdim];
    double xscale = this->TextureSize[xdim] * spacing[xdim];
    double yshift = -(0.5 - extent[2 * ydim]) * spacing[ydim];
    double yscale = this->TextureSize[ydim] * spacing[ydim];
    vtkIdType ncoords = points->GetNumberOfPoints();
    double coord[3];
    double tcoord[2];
    double invDirection[9];

    polyPoints->DeepCopy(points);
    if (textured)
    {
      vtkMatrix3x3::Invert(this->DataDirection, invDirection);
      polyTCoords->SetNumberOfTuples(ncoords);
    }

    for (vtkIdType i = 0; i < ncoords; i++)
    {
      if (textured)
      {
        // convert points from 3D model coords to 2D texture coords
        points->GetPoint(i, coord);
        vtkMath::Subtract(coord, origin, coord);
        vtkMatrix3x3::MultiplyPoint(invDirection, coord, coord);
        tcoord[0] = (coord[0] - xshift) / xscale;
        tcoord[1] = (coord[1] - yshift) / yscale;
        polyTCoords->SetTuple(i, tcoord);
      }
    }
    if (textured)
    {
      polyTCoords->Modified();
    }
  }
  else // no polygon to render
  {
    return;
  }

  if (textured)
  {
    actor->GetTexture()->Render(ren);
  }
  actor->GetMapper()->SetClippingPlanes(this->GetClippingPlanes());
  actor->GetMapper()->Render(ren, actor);
  if (textured)
  {
    actor->GetTexture()->PostRender(ren);
  }

  vtkOpenGLCheckErrorMacro("failed after RenderPolygon");
}

//------------------------------------------------------------------------------
// Render a wide black border around the polygon, wide enough to fill
// the entire viewport.
void vtkOpenGLImageSliceMapper::RenderBackground(
  vtkActor* actor, vtkPoints* points, const int extent[6], vtkRenderer* ren)
{
  vtkOpenGLClearErrorMacro();

  vtkPolyData* poly = vtkPolyDataMapper::SafeDownCast(actor->GetMapper())->GetInput();
  vtkPoints* polyPoints = poly->GetPoints();
  if (this->GetOutputPointsPrecision() == vtkAlgorithm::DOUBLE_PRECISION)
  {
    polyPoints->SetDataTypeToDouble();
  }
  vtkCellArray* tris = poly->GetPolys();

  static double borderThickness = 1e6;
  int xdim, ydim;
  vtkImageSliceMapper::GetDimensionIndices(this->Orientation, xdim, ydim);

  if (!points)
  {
    double coords[15], tcoords[10], center[3];
    this->MakeTextureGeometry(extent, coords, tcoords);
    coords[12] = coords[0];
    coords[13] = coords[1];
    coords[14] = coords[2];

    center[0] = 0.25 * (coords[0] + coords[3] + coords[6] + coords[9]);
    center[1] = 0.25 * (coords[1] + coords[4] + coords[7] + coords[10]);
    center[2] = 0.25 * (coords[2] + coords[5] + coords[8] + coords[11]);

    // render 4 sides
    tris->Initialize();
    polyPoints->SetNumberOfPoints(10);
    for (int side = 0; side < 4; side++)
    {
      tris->InsertNextCell(3);
      tris->InsertCellPoint(side);
      tris->InsertCellPoint(side + 5);
      tris->InsertCellPoint(side + 1);
      tris->InsertNextCell(3);
      tris->InsertCellPoint(side + 1);
      tris->InsertCellPoint(side + 5);
      tris->InsertCellPoint(side + 6);
    }

    for (int side = 0; side < 5; side++)
    {
      polyPoints->SetPoint(side, coords[3 * side], coords[3 * side + 1], coords[3 * side + 2]);

      double dx = coords[3 * side + xdim] - center[xdim];
      double sx = (dx >= 0 ? 1 : -1);
      double dy = coords[3 * side + ydim] - center[ydim];
      double sy = (dy >= 0 ? 1 : -1);
      coords[3 * side + xdim] += borderThickness * sx;
      coords[3 * side + ydim] += borderThickness * sy;

      polyPoints->SetPoint(side + 5, coords[3 * side], coords[3 * side + 1], coords[3 * side + 2]);
    }
  }
  else if (points->GetNumberOfPoints())
  {
    vtkIdType ncoords = points->GetNumberOfPoints();
    double coord[3], coord1[3];

    points->GetPoint(ncoords - 1, coord1);
    points->GetPoint(0, coord);
    double dx0 = coord[0] - coord1[0];
    double dy0 = coord[1] - coord1[1];
    double r = sqrt(dx0 * dx0 + dy0 * dy0);
    dx0 /= r;
    dy0 /= r;

    tris->Initialize();
    polyPoints->SetNumberOfPoints(ncoords * 2 + 2);

    for (vtkIdType i = 0; i < ncoords; i++)
    {
      tris->InsertNextCell(3);
      tris->InsertCellPoint(i * 2);
      tris->InsertCellPoint(i * 2 + 1);
      tris->InsertCellPoint(i * 2 + 2);
      tris->InsertNextCell(3);
      tris->InsertCellPoint(i * 2 + 2);
      tris->InsertCellPoint(i * 2 + 1);
      tris->InsertCellPoint(i * 2 + 3);
    }

    for (vtkIdType i = 0; i <= ncoords; i++)
    {
      polyPoints->SetPoint(i * 2, coord);

      points->GetPoint(((i + 1) % ncoords), coord1);
      double dx1 = coord1[0] - coord[0];
      double dy1 = coord1[1] - coord[1];
      r = sqrt(dx1 * dx1 + dy1 * dy1);
      dx1 /= r;
      dy1 /= r;

      double t;
      if (fabs(dx0 + dx1) > fabs(dy0 + dy1))
      {
        t = (dy1 - dy0) / (dx0 + dx1);
      }
      else
      {
        t = (dx0 - dx1) / (dy0 + dy1);
      }
      coord[0] += (t * dx0 + dy0) * borderThickness;
      coord[1] += (t * dy0 - dx0) * borderThickness;

      polyPoints->SetPoint(i * 2 + 1, coord);

      coord[0] = coord1[0];
      coord[1] = coord1[1];
      dx0 = dx1;
      dy0 = dy1;
    }
  }
  else // no polygon to render
  {
    return;
  }

  polyPoints->GetData()->Modified();
  tris->Modified();
  actor->GetMapper()->SetClippingPlanes(this->GetClippingPlanes());
  actor->GetMapper()->Render(ren, actor);

  vtkOpenGLCheckErrorMacro("failed after RenderBackground");
}

//------------------------------------------------------------------------------
void vtkOpenGLImageSliceMapper::ComputeTextureSize(
  const int extent[6], int& xdim, int& ydim, int imageSize[2], int textureSize[2])
{
  // find dimension indices that will correspond to the
  // columns and rows of the 2D texture
  vtkImageSliceMapper::GetDimensionIndices(this->Orientation, xdim, ydim);

  // compute the image dimensions
  imageSize[0] = (extent[xdim * 2 + 1] - extent[xdim * 2] + 1);
  imageSize[1] = (extent[ydim * 2 + 1] - extent[ydim * 2] + 1);

  textureSize[0] = imageSize[0];
  textureSize[1] = imageSize[1];
}

//------------------------------------------------------------------------------
// Determine if a given texture size is supported by the video card
bool vtkOpenGLImageSliceMapper::TextureSizeOK(const int size[2], vtkRenderer* ren)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  // First ask OpenGL what the max texture size is
  GLint maxSize;
  ostate->vtkglGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
  // if it does fit, we will render it later
  return size[0] <= maxSize && size[1] <= maxSize;
}

//------------------------------------------------------------------------------
// New method to render for hardware selection
void vtkOpenGLImageSliceMapper::RenderForSelection(
  vtkRenderer* ren, vtkImageSlice* prop, vtkHardwareSelector* selector)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkImageData* input = this->GetInput();

  if (!renWin || !input)
  {
    return;
  }

  // Setup Shader Program
  if (!this->SelectionHelper->Program)
  {
    this->SelectionHelper->Program = renWin->GetShaderCache()->ReadyShaderProgram(selVS, selFS, "");
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->SelectionHelper->Program);
  }
  vtkShaderProgram* prog = this->SelectionHelper->Program;

  vtkOpenGLCamera* cam = vtkOpenGLCamera::SafeDownCast(ren->GetActiveCamera());

  // Key Matrices
  vtkMatrix4x4* wcdc;
  vtkMatrix4x4* wcvc;
  vtkMatrix3x3* norms;
  vtkMatrix4x4* vcdc;
  cam->GetKeyMatrices(ren, wcvc, norms, vcdc, wcdc);
  vtkMatrix4x4* mcwc = prop->GetMatrix();

  if (!prop->GetIsIdentity())
  {
    vtkNew<vtkMatrix4x4> mcdc;
    vtkMatrix4x4::Multiply4x4(wcdc, mcwc, mcdc);

    if (prog->IsUniformUsed("MCDCMatrix"))
    {
      prog->SetUniformMatrix("MCDCMatrix", mcdc);
    }
  }
  else
  {
    // If Actor is at Identity, Data Space == World Space, so just use WCDC
    if (prog->IsUniformUsed("MCDCMatrix"))
    {
      prog->SetUniformMatrix("MCDCMatrix", wcdc);
    }
  }

  int dims[3];
  input->GetDimensions(dims);
  float idDims[2];

  if (selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    // Cell Picking: Reduce dims by 1
    // Ensure we don't go negative if dims are 0 or 1
    idDims[0] = (dims[0] > 1) ? (float)(dims[0] - 1) : 1.0f;
    idDims[1] = (dims[1] > 1) ? (float)(dims[1] - 1) : 1.0f;
  }
  else
  {
    // Point Picking (default): Use full dims
    idDims[0] = (float)dims[0];
    idDims[1] = (float)dims[1];
  }

  // Pass the dimensions to the shader
  prog->SetUniform2f("imgDims", idDims);

  int wholeExt[6];
  input->GetExtent(wholeExt);

  int* dispExt = this->DisplayExtent;

  double origin[3];
  double spacing[3];
  input->GetOrigin(origin);
  input->GetSpacing(spacing);

  double zVal = origin[2] + (dispExt[4] * spacing[2]);

  // Subtract 0.5 from min and add 0.5 to max to cover the full pixel area.
  // This ensures the picking quad aligns perfectly with the rendered pixels.
  float xMin = (float)(origin[0] + (dispExt[0] - 0.5) * spacing[0]);
  float xMax = (float)(origin[0] + (dispExt[1] + 0.5) * spacing[0]);
  float yMin = (float)(origin[1] + (dispExt[2] - 0.5) * spacing[1]);
  float yMax = (float)(origin[1] + (dispExt[3] + 0.5) * spacing[1]);

  float verts[] = {
    xMin, yMin, (float)zVal, // BL
    xMax, yMin, (float)zVal, // BR
    xMin, yMax, (float)zVal, // TL
    xMax, yMax, (float)zVal  // TR
  };

  // Map the visible "Display Extent" onto the quad.
  // Total dimensions of the full input image
  float wholeWidth = (float)(wholeExt[1] - wholeExt[0] + 1);
  float wholeHeight = (float)(wholeExt[3] - wholeExt[2] + 1);

  // Avoid division by zero
  wholeWidth = (wholeWidth < 1.0f) ? 1.0f : wholeWidth;
  wholeHeight = (wholeHeight < 1.0f) ? 1.0f : wholeHeight;

  // Calculate UV fractions based on the Voxel Edges
  // e.g. If Extent is 0..9 (10 pixels), map 0.0 to 10.0 / 10.0
  float uMin = (float)(dispExt[0] - wholeExt[0]) / wholeWidth;
  float uMax = (float)(dispExt[1] - wholeExt[0] + 1) / wholeWidth;
  float vMin = (float)(dispExt[2] - wholeExt[2]) / wholeHeight;
  float vMax = (float)(dispExt[3] - wholeExt[2] + 1) / wholeHeight;

  float tcoords[] = {
    uMin, vMin, // BL
    uMax, vMin, // BR
    uMin, vMax, // TL
    uMax, vMax  // TR
  };

  // Render the quad
  vtkOpenGLVertexArrayObject* vao = this->SelectionHelper->VAO;
  vao->Bind();

  // Upload the geometry and color data
  vtkNew<vtkOpenGLBufferObject> vbo;
  vbo->Upload(verts, 12, vtkOpenGLBufferObject::ArrayBuffer);
  vao->AddAttributeArray(prog, vbo, "vertexMC", 0, 3 * sizeof(float), VTK_FLOAT, 3, false);

  // Texture coordinates
  vtkNew<vtkOpenGLBufferObject> tbo;
  tbo->Upload(tcoords, 8, vtkOpenGLBufferObject::ArrayBuffer);
  vao->AddAttributeArray(prog, tbo, "tcoordMC", 0, 2 * sizeof(float), VTK_FLOAT, 2, false);

  // Draw
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  vao->Release();

  // Update selector with number of points/cells
  int* inputExtent = this->GetInput()->GetExtent();
  unsigned int const numVoxels = (inputExtent[1] - inputExtent[0] + 1) *
    (inputExtent[3] - inputExtent[2] + 1) * (inputExtent[5] - inputExtent[4] + 1);
  selector->UpdateMaximumPointId(numVoxels);
  selector->UpdateMaximumCellId(numVoxels);
  selector->EndRenderProp();
}

//------------------------------------------------------------------------------
// Set the modelview transform and load the texture
void vtkOpenGLImageSliceMapper::Render(vtkRenderer* ren, vtkImageSlice* prop)
{
  vtkOpenGLClearErrorMacro();

  vtkHardwareSelector* selector = ren->GetSelector();
  // Check if the selector is valid, is associated with cells, and if the current selection pass
  // is one of the cell or point ID passes
  if (selector != nullptr &&
    selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_CELLS &&
    (selector->GetCurrentPass() == vtkHardwareSelector::CELL_ID_LOW24 ||
      selector->GetCurrentPass() == vtkHardwareSelector::CELL_ID_HIGH24 ||
      selector->GetCurrentPass() == vtkHardwareSelector::POINT_ID_LOW24 ||
      selector->GetCurrentPass() == vtkHardwareSelector::POINT_ID_HIGH24))
  {
    // Hardware selection: render cell/point IDs using texture coordinates
    this->RenderForSelection(ren, prop, selector);
    // we are in selection mode, do not render anything else
    return;
  }

  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());

  // update the input information
  vtkImageData* input = this->GetInput();
  input->GetSpacing(this->DataSpacing);
  vtkMatrix3x3::DeepCopy(this->DataDirection, input->GetDirectionMatrix());
  input->GetOrigin(this->DataOrigin);
  vtkInformation* inputInfo = this->GetInputInformation(0, 0);
  inputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->DataWholeExtent);

  vtkMatrix4x4* matrix = this->GetDataToWorldMatrix();
  this->PolyDataActor->SetUserMatrix(matrix);
  this->BackingPolyDataActor->SetUserMatrix(matrix);
  this->BackgroundPolyDataActor->SetUserMatrix(matrix);
  if (prop->GetPropertyKeys())
  {
    this->PolyDataActor->SetPropertyKeys(prop->GetPropertyKeys());
    this->BackingPolyDataActor->SetPropertyKeys(prop->GetPropertyKeys());
    this->BackgroundPolyDataActor->SetPropertyKeys(prop->GetPropertyKeys());
  }

  // and now enable/disable as needed for our render
  //  glDisable(GL_CULL_FACE);
  //  glDisable(GL_COLOR_MATERIAL);

  // do an offset to avoid depth buffer issues
  // this->PolyDataActor->GetMapper()->
  //   SetResolveCoincidentTopology(VTK_RESOLVE_POLYGON_OFFSET);
  // this->PolyDataActor->GetMapper()->
  //   SetRelativeCoincidentTopologyPolygonOffsetParameters(1.0,100);

  // Add all the clipping planes  TODO: really in the mapper
  // int numClipPlanes = this->GetNumberOfClippingPlanes();

  vtkOpenGLState* ostate = renWin->GetState();

  // Whether to write to the depth buffer and color buffer
  ostate->vtkglDepthMask(this->DepthEnable ? GL_TRUE : GL_FALSE); // supported in all
  if (!this->ColorEnable && !this->MatteEnable)
  {
    ostate->vtkglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // supported in all
  }

  // color and lighting related items
  vtkImageProperty* property = prop->GetProperty();
  double opacity = property->GetOpacity();
  double ambient = property->GetAmbient();
  double diffuse = property->GetDiffuse();
  vtkProperty* pdProp = this->PolyDataActor->GetProperty();
  pdProp->SetOpacity(opacity);
  pdProp->SetAmbient(ambient);
  pdProp->SetDiffuse(diffuse);

  // render the backing polygon
  int backing = property->GetBacking();
  double* bcolor = property->GetBackingColor();
  if (backing && (this->MatteEnable || (this->DepthEnable && !this->ColorEnable)))
  {
    // the backing polygon is always opaque
    pdProp = this->BackingPolyDataActor->GetProperty();
    pdProp->SetOpacity(1.0);
    pdProp->SetAmbient(ambient);
    pdProp->SetDiffuse(diffuse);
    pdProp->SetColor(bcolor[0], bcolor[1], bcolor[2]);
    this->RenderPolygon(this->BackingPolyDataActor, this->Points, this->DisplayExtent, ren);
    if (this->Background)
    {
      double bkcolor[4];
      this->GetBackgroundColor(property, bkcolor);
      pdProp = this->BackgroundPolyDataActor->GetProperty();
      pdProp->SetOpacity(1.0);
      pdProp->SetAmbient(ambient);
      pdProp->SetDiffuse(diffuse);
      pdProp->SetColor(bkcolor[0], bkcolor[1], bkcolor[2]);
      this->RenderBackground(this->BackgroundPolyDataActor, this->Points, this->DisplayExtent, ren);
    }
  }

  // render the texture
  if (this->ColorEnable || (!backing && this->DepthEnable))
  {
    this->RecursiveRenderTexturedPolygon(
      ren, property, this->GetInput(), this->DisplayExtent, false);
  }

  // Set the masks back again
  ostate->vtkglDepthMask(GL_TRUE);
  if (!this->ColorEnable && !this->MatteEnable)
  {
    ostate->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  }

  this->TimeToDraw = 0.0001;

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
void vtkOpenGLImageSliceMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
