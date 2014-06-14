/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageSliceMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLImageSliceMapper.h"

#include <GL/glew.h>

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkImageSlice.h"
#include "vtkImageProperty.h"
#include "vtkDataArray.h"
#include "vtkLookupTable.h"
#include "vtkPoints.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"
#include "vtkMapper.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkTimerLog.h"
#include "vtkGarbageCollector.h"
#include "vtkTemplateAliasMacro.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include "vtkTexture.h"
#include "vtkPointData.h"
#include "vtkVBOPolyDataMapper.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkTrivialProducer.h"
#include "vtkActor.h"
#include "vtkProperty.h"

#include <math.h>

#include "vtkOpenGLError.h"

vtkStandardNewMacro(vtkOpenGLImageSliceMapper);

//----------------------------------------------------------------------------
// Initializes an instance, generates a unique index.
vtkOpenGLImageSliceMapper::vtkOpenGLImageSliceMapper()
{
  // setup the polygon mapper
  {
  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(4);
  polydata->SetPoints(points.Get());

  vtkNew<vtkCellArray> tris;
  polydata->SetPolys(tris.Get());

  vtkNew<vtkFloatArray> tcoords;
  tcoords->SetNumberOfComponents(2);
  tcoords->SetNumberOfTuples(4);
  polydata->GetPointData()->SetTCoords(tcoords.Get());

  vtkNew<vtkTrivialProducer> prod;
  prod->SetOutput(polydata.Get());
  vtkNew<vtkVBOPolyDataMapper> polyDataMapper;
  polyDataMapper->SetInputConnection(prod->GetOutputPort());
  this->PolyDataActor = vtkActor::New();
  this->PolyDataActor->SetMapper(polyDataMapper.Get());
  vtkNew<vtkTexture> texture;
  texture->RepeatOff();
  this->PolyDataActor->SetTexture(texture.Get());
  }

  // setup the backing polygon mapper
  {
  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(4);
  polydata->SetPoints(points.Get());

  vtkNew<vtkCellArray> tris;
  polydata->SetPolys(tris.Get());

  vtkNew<vtkTrivialProducer> prod;
  prod->SetOutput(polydata.Get());
  vtkNew<vtkVBOPolyDataMapper> polyDataMapper;
  polyDataMapper->SetInputConnection(prod->GetOutputPort());
  this->BackingPolyDataActor = vtkActor::New();
  this->BackingPolyDataActor->SetMapper(polyDataMapper.Get());
  }

  // setup the background polygon mapper
  {
  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(10);
  polydata->SetPoints(points.Get());

  vtkNew<vtkCellArray> tris;
  polydata->SetPolys(tris.Get());

  vtkNew<vtkTrivialProducer> prod;
  prod->SetOutput(polydata.Get());
  vtkNew<vtkVBOPolyDataMapper> polyDataMapper;
  polyDataMapper->SetInputConnection(prod->GetOutputPort());
  this->BackgroundPolyDataActor = vtkActor::New();
  this->BackgroundPolyDataActor->SetMapper(polyDataMapper.Get());
  }

  this->TextureIndex = 0;
  this->BackgroundTextureIndex = 0;
  this->FragmentShaderIndex = 0;
  this->RenderWindow = 0;
  this->TextureSize[0] = 0;
  this->TextureSize[1] = 0;
  this->TextureBytesPerPixel = 1;

  this->LastOrientation = -1;
  this->LastSliceNumber = VTK_INT_MAX;

  this->LoadCount = 0;

  this->UseClampToEdge = false;
  this->UsePowerOfTwoTextures = true;

  // Use GL_ARB_fragment_program, which is an extension to OpenGL 1.3
  // that is compatible with very old drivers and hardware, and is still
  // fully supported on modern hardware.  The only caveat is that it is
  // automatically disabled if any modern shaders (e.g. depth peeling)
  // are simultaneously loaded, so it will not interfere with them.
  this->UseFragmentProgram = false;
}

//----------------------------------------------------------------------------
vtkOpenGLImageSliceMapper::~vtkOpenGLImageSliceMapper()
{
  this->RenderWindow = NULL;
  this->BackgroundPolyDataActor->UnRegister(this);
  this->BackingPolyDataActor->UnRegister(this);
  this->PolyDataActor->UnRegister(this);
}

//----------------------------------------------------------------------------
// Release the graphics resources used by this texture.
void vtkOpenGLImageSliceMapper::ReleaseGraphicsResources(vtkWindow *renWin)
{
  this->BackgroundPolyDataActor->ReleaseGraphicsResources(renWin);
  this->BackingPolyDataActor->ReleaseGraphicsResources(renWin);
  this->PolyDataActor->ReleaseGraphicsResources(renWin);

  this->TextureIndex = 0;
  this->BackgroundTextureIndex = 0;
  this->FragmentShaderIndex = 0;
  this->RenderWindow = NULL;
  this->Modified();
}

//----------------------------------------------------------------------------
// Subdivide the image until the pieces fit into texture memory
void vtkOpenGLImageSliceMapper::RecursiveRenderTexturedPolygon(
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
// Load the given image extent into a texture and render it
void vtkOpenGLImageSliceMapper::RenderTexturedPolygon(
  vtkRenderer *ren, vtkImageProperty *property,
  vtkImageData *input, int extent[6], bool recursive)
{
  // get the previous texture load time
  unsigned long loadTime = this->LoadTime.GetMTime();

  // the render window, needed for state information
  vtkOpenGLRenderWindow *renWin =
    static_cast<vtkOpenGLRenderWindow *>(ren->GetRenderWindow());

  bool reuseTexture = true;

  // if context has changed, verify context capabilities
  if (renWin != this->RenderWindow ||
      renWin->GetContextCreationTime() > loadTime)
    {
    // force two initial loads for each new context
    this->LoadCount = 0;
    this->CheckOpenGLCapabilities(renWin);
    reuseTexture = false;
    }

  vtkOpenGLClearErrorMacro();

  // get information about the image
  int xdim, ydim; // orientation of texture wrt input image
  vtkImageSliceMapper::GetDimensionIndices(this->Orientation, xdim, ydim);

  // check whether to use a shader for bicubic interpolation
  bool checkerboard = (property && property->GetCheckerboard());
  bool cubicInterpolation = (property &&
    property->GetInterpolationType() == VTK_CUBIC_INTERPOLATION);
  bool useFragmentProgram = false;
    // (this->UseFragmentProgram &&
    //  (!this->ExactPixelMatch || !this->SliceFacesCamera) &&
    //  (cubicInterpolation || checkerboard));

  // verify that the orientation and slice has not changed
  bool orientationChanged = (this->Orientation != this->LastOrientation);
  this->LastOrientation = this->Orientation;
  bool sliceChanged = (this->SliceNumber != this->LastSliceNumber);
  this->LastSliceNumber = this->SliceNumber;

  // get the mtime of the property, including the lookup table
  unsigned long propertyMTime = 0;
  if (property)
    {
    propertyMTime = property->GetMTime();
    if (!this->PassColorData)
      {
      vtkScalarsToColors *table = property->GetLookupTable();
      if (table)
        {
        unsigned long mtime = table->GetMTime();
        if (mtime > propertyMTime)
          {
          propertyMTime = mtime;
          }
        }
      }
    }

  // need to reload the texture
  if (this->vtkImageMapper3D::GetMTime() > loadTime ||
      propertyMTime > loadTime ||
      input->GetMTime() > loadTime ||
      orientationChanged || sliceChanged ||
      this->LoadCount < 2 || recursive)
    {
    this->LoadCount++;

    // get the data to load as a texture
    int xsize;
    int ysize;
    int bytesPerPixel;

    // whether to try to use the input data directly as the texture
    bool reuseData = true;

    // generate the data to be used as a texture
    unsigned char *data = this->MakeTextureData(
      (this->PassColorData ? 0 : property), input, extent, xsize, ysize,
      bytesPerPixel, reuseTexture, reuseData);

    this->TextureSize[0] = xsize;
    this->TextureSize[1] = ysize;
    this->TextureBytesPerPixel = bytesPerPixel;

    vtkImageData *id = vtkImageData::New();
    id->SetExtent(0,xsize-1,0,ysize-1,0,0);
    vtkUnsignedCharArray *uca = vtkUnsignedCharArray::New();
    uca->SetNumberOfComponents(bytesPerPixel);
    uca->SetArray(data,xsize*ysize*bytesPerPixel,!reuseData);
    id->GetPointData()->SetScalars(uca);

    this->PolyDataActor->GetTexture()->SetInputData(id);

    if (property->GetInterpolationType() == VTK_NEAREST_INTERPOLATION &&
        !this->ExactPixelMatch)
      {
      this->PolyDataActor->GetTexture()->InterpolateOff();
      }
    else
      {
      this->PolyDataActor->GetTexture()->InterpolateOn();
      }

    if (this->UseClampToEdge)
      {
      this->PolyDataActor->GetTexture()->EdgeClampOn();
      }
    else
      {
      this->PolyDataActor->GetTexture()->EdgeClampOff();
      }

    // modify the load time to the current time
    this->LoadTime.Modified();
    }

  vtkPoints *points = this->Points;
  if (this->ExactPixelMatch && this->SliceFacesCamera)
    {
    points = 0;
    }

  this->RenderPolygon(this->PolyDataActor, points, extent, ren);

  if (this->Background)
    {
    double ambient = property->GetAmbient();
    double diffuse = property->GetDiffuse();

    double bkcolor[4];
    this->GetBackgroundColor(property, bkcolor);
    vtkProperty *pdProp = this->BackgroundPolyDataActor->GetProperty();
    pdProp->SetAmbient(ambient);
    pdProp->SetDiffuse(diffuse);
    pdProp->SetColor(bkcolor[0], bkcolor[1], bkcolor[2]);
    this->RenderBackground(this->BackgroundPolyDataActor, points, extent, ren);
    }

  if (useFragmentProgram)
    {
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
    }

  vtkOpenGLCheckErrorMacro("failed after RenderTexturedPolygon");
}

//----------------------------------------------------------------------------
// Render the polygon that displays the image data
void vtkOpenGLImageSliceMapper::RenderPolygon(
  vtkActor *actor, vtkPoints *points, const int extent[6], vtkRenderer *ren)
{
  vtkOpenGLClearErrorMacro();

  bool textured = (actor->GetTexture() != NULL);
  vtkPolyData *poly = vtkPolyDataMapper::SafeDownCast(actor->GetMapper())->GetInput();
  vtkPoints *polyPoints = poly->GetPoints();
  vtkCellArray *tris = poly->GetPolys();
  vtkDataArray *polyTCoords = poly->GetPointData()->GetTCoords();

  if (!points)
    {
    double coords[12], tcoords[8];
    this->MakeTextureGeometry(extent, coords, tcoords);

    tris->Initialize();
    tris->InsertNextCell(3);
    tris->InsertCellPoint(0);
    tris->InsertCellPoint(1);
    tris->InsertCellPoint(2);
    tris->InsertNextCell(3);
    tris->InsertCellPoint(0);
    tris->InsertCellPoint(2);
    tris->InsertCellPoint(3);

    polyPoints->SetNumberOfPoints(4);
    if (textured)
      {
      polyTCoords->SetNumberOfTuples(4);
      }
    for (int i = 0; i < 4; i++)
      {
      polyPoints->SetPoint(i, coords[3*i], coords[3*i+1], coords[3*i+2]);
      if (textured)
        {
        polyTCoords->SetTuple(i,&tcoords[2*i]);
        }
      }
    }
  else if (points->GetNumberOfPoints())
    {
    int xdim, ydim;
    vtkImageSliceMapper::GetDimensionIndices(this->Orientation, xdim, ydim);
    double *origin = this->DataOrigin;
    double *spacing = this->DataSpacing;
    double xshift = origin[xdim] - (0.5 - extent[2*xdim])*spacing[xdim];
    double xscale = this->TextureSize[xdim]*spacing[xdim];
    double yshift = origin[ydim] - (0.5 - extent[2*ydim])*spacing[ydim];
    double yscale = this->TextureSize[ydim]*spacing[ydim];
    vtkIdType ncoords = points->GetNumberOfPoints();
    double coord[3];
    double tcoord[2];

    polyPoints->DeepCopy(points);
    if (textured)
      {
      polyTCoords->SetNumberOfTuples(ncoords);
      }

    tris->Initialize();
    tris->Allocate(4*(ncoords-2));
    for (vtkIdType i = 0; i < ncoords; i++)
      {
      if (textured)
        {
        points->GetPoint(i, coord);
        tcoord[0] = (coord[0] - xshift)/xscale;
        tcoord[1] = (coord[1] - yshift)/yscale;
        polyTCoords->SetTuple(i,tcoord);
        }
      if (i >= 2)
        {
        tris->InsertNextCell(3);
        tris->InsertCellPoint(ncoords - (i+1)/2);
        tris->InsertCellPoint(i/2 - 1);
        tris->InsertCellPoint((i % 2 == 0) ? ncoords - 1 - i/2 : i/2);
        }
      }
    }

  if (textured)
    {
    actor->GetTexture()->Render(ren);
    }
  actor->GetMapper()->Render(ren, actor);

  vtkOpenGLCheckErrorMacro("failed after RenderPolygon");
}

//----------------------------------------------------------------------------
// Render a wide black border around the polygon, wide enough to fill
// the entire viewport.
void vtkOpenGLImageSliceMapper::RenderBackground(
  vtkActor *actor, vtkPoints *points, const int extent[6], vtkRenderer *ren)
{
  vtkOpenGLClearErrorMacro();

  vtkPolyData *poly = vtkPolyDataMapper::SafeDownCast(actor->GetMapper())->GetInput();
  vtkPoints *polyPoints = poly->GetPoints();
  vtkCellArray *tris = poly->GetPolys();

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

    center[0] = 0.25*(coords[0] + coords[3] + coords[6] + coords[9]);
    center[1] = 0.25*(coords[1] + coords[4] + coords[7] + coords[10]);
    center[2] = 0.25*(coords[2] + coords[5] + coords[8] + coords[11]);

    // render 4 sides
    tris->Initialize();
    polyPoints->SetNumberOfPoints(10);
    for (int side = 0; side < 4; side++)
      {
      tris->InsertNextCell(3);
      tris->InsertCellPoint(side);
      tris->InsertCellPoint(side+5);
      tris->InsertCellPoint(side+1);
      tris->InsertNextCell(3);
      tris->InsertCellPoint(side+1);
      tris->InsertCellPoint(side+5);
      tris->InsertCellPoint(side+6);
      }

    for (int side = 0; side < 5; side++)
      {
      polyPoints->SetPoint(side, coords[3*side], coords[3*side+1], coords[3*side+2]);

      double dx = coords[3*side+xdim] - center[xdim];
      double sx = (dx >= 0 ? 1 : -1);
      double dy = coords[3*side+ydim] - center[ydim];
      double sy = (dy >= 0 ? 1 : -1);
      coords[3*side+xdim] += borderThickness*sx;
      coords[3*side+ydim] += borderThickness*sy;

      polyPoints->SetPoint(side+5, coords[3*side], coords[3*side+1], coords[3*side+2]);
      }
    }
  else if (points->GetNumberOfPoints())
    {
    vtkIdType ncoords = points->GetNumberOfPoints();
    double coord[3], coord1[3];

    points->GetPoint(ncoords-1, coord1);
    points->GetPoint(0, coord);
    double dx0 = coord[0] - coord1[0];
    double dy0 = coord[1] - coord1[1];
    double r = sqrt(dx0*dx0 + dy0*dy0);
    dx0 /= r;
    dy0 /= r;

    tris->Initialize();
    polyPoints->SetNumberOfPoints(ncoords*2+2);

    for (vtkIdType i = 0; i < ncoords; i++)
      {
      tris->InsertNextCell(3);
      tris->InsertCellPoint(i*2);
      tris->InsertCellPoint(i*2+1);
      tris->InsertCellPoint(i*2+2);
      tris->InsertNextCell(3);
      tris->InsertCellPoint(i*2+2);
      tris->InsertCellPoint(i*2+1);
      tris->InsertCellPoint(i*2+3);
      }

    for (vtkIdType i = 0; i <= ncoords; i++)
      {
      polyPoints->SetPoint(i*2,coord);

      points->GetPoint(((i + 1) % ncoords), coord1);
      double dx1 = coord1[0] - coord[0];
      double dy1 = coord1[1] - coord[1];
      r = sqrt(dx1*dx1 + dy1*dy1);
      dx1 /= r;
      dy1 /= r;

      double t;
      if (fabs(dx0 + dx1) > fabs(dy0 + dy1))
        {
        t = (dy1 - dy0)/(dx0 + dx1);
        }
      else
        {
        t = (dx0 - dx1)/(dy0 + dy1);
        }
      coord[0] += (t*dx0 + dy0)*borderThickness;
      coord[1] += (t*dy0 - dx0)*borderThickness;

      polyPoints->SetPoint(i*2+1,coord);

      coord[0] = coord1[0];
      coord[1] = coord1[1];
      dx0 = dx1;
      dy0 = dy1;
      }
    }

  actor->GetMapper()->Render(ren, actor);

  vtkOpenGLCheckErrorMacro("failed after RenderBackground");
}

//----------------------------------------------------------------------------
void vtkOpenGLImageSliceMapper::BindFragmentProgram(
  vtkRenderer *ren, vtkImageProperty *property)
{
  vtkOpenGLClearErrorMacro();

  int xdim, ydim, zdim; // orientation of texture wrt input image
  vtkImageSliceMapper::GetDimensionIndices(this->Orientation, xdim, ydim);
  zdim = 3 - xdim - ydim; // they sum to three
  double *spacing = this->DataSpacing;
  double *origin = this->DataOrigin;
  int *extent = this->DisplayExtent;

  // Bind the bicubic interpolation fragment program, it will
  // not do anything if modern shader objects are also in play.
  glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
                        this->FragmentShaderIndex);

  // checkerboard information
  double checkSpacing[2], checkOffset[2];
  property->GetCheckerboardSpacing(checkSpacing);
  property->GetCheckerboardOffset(checkOffset);

  // transformation to permute texture-oriented coords to data coords
  double mat[16];
  vtkMatrix4x4::Identity(mat);
  mat[0] = mat[5] = mat[10] = 0.0;
  mat[4*xdim] = mat[1+4*ydim] = 1.0;
  int dimsep = ydim - xdim + 3*(xdim > ydim);
  mat[2+4*zdim] = (((dimsep % 3) == 1) ? 1.0 : -1.0);
  mat[4*zdim+3] = origin[zdim] + spacing[zdim]*extent[2*zdim];

  // checkerboard uses view coordinates
  vtkMatrix4x4 *m = this->GetDataToWorldMatrix();
  vtkMatrix4x4 *c = ren->GetActiveCamera()->GetViewTransformMatrix();
  vtkMatrix4x4::Multiply4x4(*m->Element, mat, mat);
  vtkMatrix4x4::Multiply4x4(*c->Element, mat, mat);

  // first parameter: texture size needed for bicubic interpolator
  glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0,
    static_cast<float>(this->TextureSize[0]),
    static_cast<float>(this->TextureSize[1]),
    static_cast<float>(1.0/this->TextureSize[0]),
    static_cast<float>(1.0/this->TextureSize[1]));

  // second parameter: scale and offset for converting texture coords
  // into the input image's data coords
  glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1,
    static_cast<float>(this->TextureSize[0]*spacing[xdim]),
    static_cast<float>(this->TextureSize[1]*spacing[ydim]),
    static_cast<float>(origin[xdim] +
                       spacing[xdim]*(extent[2*xdim] - 0.5)),
    static_cast<float>(origin[ydim] +
                       spacing[ydim]*(extent[2*ydim] - 0.5)));

  // third parameter: scale and offset for converting data coords into
  // checkboard square indices, for checkerboarding
  glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2,
    static_cast<float>(0.5/checkSpacing[0]),
    static_cast<float>(0.5/checkSpacing[1]),
    static_cast<float>(-0.5*checkOffset[0]),
    static_cast<float>(-0.5*checkOffset[1]));

  // fourth, fifth param: first two rows of the transformation matrix
  // from data coords to camera coords (including a pre-translation of
  // z from zero to the z position of the slice, since the texture coords
  // are 2D and do not provide the z position)
  glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 3,
    static_cast<float>(mat[0]), static_cast<float>(mat[1]),
    static_cast<float>(mat[2]), static_cast<float>(mat[3]));
  glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 4,
    static_cast<float>(mat[4]), static_cast<float>(mat[5]),
    static_cast<float>(mat[6]), static_cast<float>(mat[7]));

  vtkOpenGLCheckErrorMacro("failed after BindFragmentProgram");
}

//----------------------------------------------------------------------------
vtkStdString vtkOpenGLImageSliceMapper::BuildFragmentProgram(
  vtkImageProperty *property)
{
  vtkStdString prog =
    "!!ARBfp1.0\n"
    "\n";

  // parameters needed for cubic interpolation:
  // texdim is texture size {width, height, 1.0/width, 1.0/height}
  // parameters needed for checkerboarding:
  // todata is for converting tex coords to VTK data coords
  // togrid converts transformed data coords to checkerboard squares
  // mx, my are first two rows of matrix for transforming data coords
  prog.append(
    "PARAM texdim = program.local[0];\n"
    "PARAM todata = program.local[1];\n"
    "PARAM togrid = program.local[2];\n"
    "PARAM mx = program.local[3];\n"
    "PARAM my = program.local[4];\n"
    "TEMP coord, coord2;\n"
    "TEMP c, c1, c2;\n"
    "TEMP weightx, weighty;\n"
    "\n");

  // checkerboard
  if (property->GetCheckerboard())
    {
    prog.append(
    "# generate a checkerboard pattern\n"
    "MOV coord.xyzw, {0, 0, 0, 1};\n"
    "MAD coord.xy, fragment.texcoord.xyxy, todata.xyxy, todata.zwzw;\n"
    "DP4 coord2.x, coord, mx;\n"
    "DP4 coord2.y, coord, my;\n"
    "MAD coord.xy, coord2.xyxy, togrid.xyxy, togrid.zwzw;\n"
    "FRC coord.xy, coord;\n"
    "SUB coord.xy, coord, {0.5, 0.5, 0.5, 0.5};\n"
    "MUL coord.x, coord.x, coord.y;\n"
    "KIL coord.x;\n"
    "\n");
    }

  // interpolate
  if (property->GetInterpolationType() == VTK_CUBIC_INTERPOLATION)
    {
    // create a bicubic interpolation program
    prog.append(
    "# compute the {rx, ry, fx, fy} fraction vector\n"
    "MAD coord, fragment.texcoord.xyxy, texdim.xyxy, {0.5, 0.5, 0.5, 0.5};\n"
    "FRC coord, coord;\n"
    "SUB coord.xy, {1, 1, 1, 1}, coord;\n"
    "\n"
    "# compute the x weights\n"
    "MAD weightx, coord.zzxx, {0.5, 1.5, 1.5, 0.5}, {0,-1,-1, 0};\n"
    "MAD weightx, weightx, coord.xzxz, {0,-1,-1, 0};\n"
    "MUL weightx, weightx, -coord.xxzz;\n"
    "\n"
    "# compute the y weights\n"
    "MAD weighty, coord.wwyy, {0.5, 1.5, 1.5, 0.5}, {0,-1,-1, 0};\n"
    "MAD weighty, weighty, coord.ywyw, {0,-1,-1, 0};\n"
    "MUL weighty, weighty, -coord.yyww;\n"
    "\n"
    "# get the texture coords for the coefficients\n"
    "ADD coord, coord.xyxy, {-2,-2,-1,-2};\n"
    "MAD coord, coord, texdim.zwzw, fragment.texcoord.xyxy;\n"
    "MAD coord2, texdim.zwzw, {2, 0, 2, 0}, coord;\n"
    "\n");

    // loop through the rows of the kernel
    for (int i = 0; i < 4; i++)
      {
      prog.append(
        "# do a row of texture lookups and weights\n"
        "TEX c2, coord.xyzw, texture, 2D;\n"
        "MUL c1, c2, weightx.xxxx;\n"
        "TEX c2, coord.zwxy, texture, 2D;\n"
        "MAD c1, c2, weightx.yyyy, c1;\n"
        "TEX c2, coord2.xyzw, texture, 2D;\n"
        "MAD c1, c2, weightx.zzzz, c1;\n"
        "TEX c2, coord2.zwxy, texture, 2D;\n"
        "MAD c1, c2, weightx.wwww, c1;\n");

      // choose the y weight for current row
      static const char *rowsum[4] = {
        "MUL c, weighty.xxxx, c1;\n\n",
        "MAD c, weighty.yyyy, c1, c;\n\n",
        "MAD c, weighty.zzzz, c1, c;\n\n",
        "MAD c, weighty.wwww, c1, c;\n\n"
        };

      prog.append(rowsum[i]);

      if (i < 3)
        {
        prog.append(
        "# advance y coord to next row\n"
        "ADD coord.yw, coord, texdim.wwww;\n"
        "ADD coord2.yw, coord2, texdim.wwww;\n"
        "\n");
        }
      }
    }
  else
    {
    // use currently set texture interpolation
    prog.append(
    "# interpolate the texture\n"
    "TEX c, fragment.texcoord, texture, 2D;\n"
    "\n");
    }

  // modulate the fragment color with the texture
  prog.append(
    "# output the color\n"
    "MUL result.color, fragment.color, c;\n"
    "\n");

  // end program
  prog.append(
    "END\n");

  return prog;
}

//----------------------------------------------------------------------------
void vtkOpenGLImageSliceMapper::ComputeTextureSize(
  const int extent[6], int &xdim, int &ydim,
  int imageSize[2], int textureSize[2])
{
  // find dimension indices that will correspond to the
  // columns and rows of the 2D texture
  vtkImageSliceMapper::GetDimensionIndices(this->Orientation, xdim, ydim);

  // compute the image dimensions
  imageSize[0] = (extent[xdim*2+1] - extent[xdim*2] + 1);
  imageSize[1] = (extent[ydim*2+1] - extent[ydim*2] + 1);

  if (this->UsePowerOfTwoTextures)
    {
    // find the target size of the power-of-two texture
    for (int i = 0; i < 2; i++)
      {
      textureSize[i] = vtkMath::NearestPowerOfTwo(imageSize[i]);
      }
    }
  else
    {
    textureSize[0] = imageSize[0];
    textureSize[1] = imageSize[1];
    }
}

//----------------------------------------------------------------------------
// Determine if a given texture size is supported by the video card
bool vtkOpenGLImageSliceMapper::TextureSizeOK(const int size[2])
{
  vtkOpenGLClearErrorMacro();

  // First ask OpenGL what the max texture size is
  GLint maxSize;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
  if (size[0] > maxSize || size[1] > maxSize)
    {
    return 0;
    }

  // Test a proxy texture to see if it fits in memory
  glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA8, size[0], size[1],
               0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  GLint params = 0;
  glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,
                           &params);

  vtkOpenGLCheckErrorMacro("failed after TextureSizeOK");

  // if it does fit, we will render it later
  return (params == 0 ? 0 : 1);
}

//----------------------------------------------------------------------------
// Set the modelview transform and load the texture
void vtkOpenGLImageSliceMapper::Render(vtkRenderer *ren, vtkImageSlice *prop)
{
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();

  vtkOpenGLClearErrorMacro();

  vtkOpenGLRenderWindow *renWin =
    vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());

  if (renWin && (renWin != this->RenderWindow ||
      renWin->GetContextCreationTime() > this->LoadTime.GetMTime()))
    {
    this->CheckOpenGLCapabilities(renWin);
    }

  // time the render
  this->Timer->StartTimer();

  // update the input information
  vtkImageData *input = this->GetInput();
  input->GetSpacing(this->DataSpacing);
  input->GetOrigin(this->DataOrigin);
  vtkInformation *inputInfo = this->GetInputInformation(0, 0);
  inputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                 this->DataWholeExtent);

  matrix = this->GetDataToWorldMatrix();
  this->PolyDataActor->SetUserMatrix(matrix);
  this->BackingPolyDataActor->SetUserMatrix(matrix);
  this->BackgroundPolyDataActor->SetUserMatrix(matrix);


  // and now enable/disable as needed for our render
  //  glDisable(GL_CULL_FACE);
  //  glDisable(GL_COLOR_MATERIAL);

  // do an offset to avoid depth buffer issues
  if (vtkMapper::GetResolveCoincidentTopology() !=
      VTK_RESOLVE_SHIFT_ZBUFFER )
    {
    double f, u;
    glEnable(GL_POLYGON_OFFSET_FILL);
    vtkMapper::GetResolveCoincidentTopologyPolygonOffsetParameters(f,u);
    glPolygonOffset(f,u);
    }

  // Add all the clipping planes
  int numClipPlanes = this->GetNumberOfClippingPlanes();
  if (numClipPlanes > 6)
    {
    vtkErrorMacro(<< "OpenGL has a limit of 6 clipping planes");
    }

  for (int i = 0; i < 6; i++)
    {
    GLenum clipPlaneId = static_cast<GLenum>(GL_CLIP_PLANE0+i);
    if (i < numClipPlanes)
      {
      double planeEquation[4];
      this->GetClippingPlaneInDataCoords(matrix, i, planeEquation);
      glClipPlane(clipPlaneId, planeEquation);
      glEnable(clipPlaneId);
      }
    else
      {
      glDisable(clipPlaneId);
      }
    }

  // Whether to write to the depth buffer and color buffer
  glDepthMask(this->DepthEnable ? GL_TRUE : GL_FALSE); // supported in all
  if (!this->ColorEnable && !this->MatteEnable)
    {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // supported in all
    }

  // color and lighting related items
  vtkImageProperty *property = prop->GetProperty();
  double opacity = property->GetOpacity();
  double ambient = property->GetAmbient();
  double diffuse = property->GetDiffuse();
  vtkProperty *pdProp = this->PolyDataActor->GetProperty();
  pdProp->SetOpacity(opacity);
  pdProp->SetAmbient(ambient);
  pdProp->SetDiffuse(diffuse);

  // render the backing polygon
  int backing = property->GetBacking();
  double *bcolor = property->GetBackingColor();
  if (backing &&
      (this->MatteEnable || (this->DepthEnable && !this->ColorEnable)))
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
  glDepthMask(GL_TRUE);
  if (!this->ColorEnable && !this->MatteEnable)
    {
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

  this->Timer->StopTimer();
  this->TimeToDraw = this->Timer->GetElapsedTime();
  if (this->TimeToDraw == 0)
    {
    this->TimeToDraw = 0.0001;
    }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//----------------------------------------------------------------------------
void vtkOpenGLImageSliceMapper::CheckOpenGLCapabilities(vtkOpenGLRenderWindow*)
{
  this->UseClampToEdge = true;
  this->UsePowerOfTwoTextures = true;
  this->UseFragmentProgram = true;
}

//----------------------------------------------------------------------------
void vtkOpenGLImageSliceMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
