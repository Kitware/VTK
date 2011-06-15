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
#include "vtkOpenGLExtensionManager.h"
#include "vtkTextureBicubicARB_fp.h"
#include "vtkTimerLog.h"
#include "vtkGarbageCollector.h"
#include "vtkTemplateAliasMacro.h"

#include <math.h>

#include "vtkOpenGL.h"
#include "vtkgl.h" // vtkgl namespace

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLImageSliceMapper);
#endif

//----------------------------------------------------------------------------
// Initializes an instance, generates a unique index.
vtkOpenGLImageSliceMapper::vtkOpenGLImageSliceMapper()
{
  this->Index = 0;
  this->FragmentShaderIndex = 0;
  this->RenderWindow = 0;
  this->TextureSize[0] = 0;
  this->TextureSize[1] = 0;
  this->TextureBytesPerPixel = 1;

  this->LastOrientation = -1;
  this->LastSliceNumber = VTK_INT_MAX;

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
}

//----------------------------------------------------------------------------
// Release the graphics resources used by this texture.
void vtkOpenGLImageSliceMapper::ReleaseGraphicsResources(vtkWindow *renWin)
{
  if (this->Index && renWin && renWin->GetMapped())
    {
    static_cast<vtkRenderWindow *>(renWin)->MakeCurrent();
#ifdef GL_VERSION_1_1
    // free any textures
    if (glIsTexture(this->Index))
      {
      GLuint tempIndex;
      tempIndex = this->Index;
      // NOTE: Sun's OpenGL seems to require disabling of texture
      // before deletion
      glDisable(GL_TEXTURE_2D);
      glDeleteTextures(1, &tempIndex);
      }
    if (this->UseFragmentProgram &&
        vtkgl::IsProgramARB(this->FragmentShaderIndex))
      {
      GLuint tempIndex;
      tempIndex = this->FragmentShaderIndex;
      glDisable(vtkgl::FRAGMENT_PROGRAM_ARB);
      vtkgl::DeleteProgramsARB(1, &tempIndex);
      }
#else
    if (glIsList(this->Index))
      {
      glDeleteLists(this->Index,1);
      }
#endif
    this->TextureSize[0] = 0;
    this->TextureSize[1] = 0;
    this->TextureBytesPerPixel = 1;
    }
  this->Index = 0;
  this->FragmentShaderIndex = 0;
  this->RenderWindow = NULL;
  this->Modified();
}

//----------------------------------------------------------------------------
// Render the backing polygon
void vtkOpenGLImageSliceMapper::RenderBackingPolygon()
{
  static double normals[3][3] =
    { {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0} };
  double *normal = normals[(this->Orientation % 3)];
  vtkPoints *points = this->Points;

  if (!points)
    {
    // set the geometry for the quad to texture
    double coords[12];
    this->MakeTextureGeometry(
      this->GetInput(), this->DisplayExtent, this->Border, coords, 0);

    glBegin(GL_QUADS);
    for (int i = 0; i < 4; i++)
      {
      glNormal3dv(normal);
      glVertex3dv(&coords[i*3]);
      }
    glEnd();
    }
  else if (points->GetNumberOfPoints())
    {
    vtkIdType ncoords = points->GetNumberOfPoints(); 
    double coord[3];

    glBegin((ncoords == 4) ? GL_QUADS : GL_POLYGON);
    for (vtkIdType i = 0; i < ncoords; i++)
      {
      points->GetPoint(i, coord);

      glNormal3dv(normal);
      glVertex3dv(coord);
      }
    glEnd();
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

  vtkOpenGLRenderWindow *renWin =
    static_cast<vtkOpenGLRenderWindow *>(ren->GetRenderWindow());

#ifdef GL_VERSION_1_1
  bool reuseTexture = true;
#else
  bool reuseTexture = false;
#endif

  // if context has changed, verify context capabilities
  if (renWin != this->RenderWindow ||
      renWin->GetContextCreationTime() > loadTime)
    {
    this->CheckOpenGLCapabilities(renWin);
    reuseTexture = false;
    }

  // check whether to use a shader for bicubic interpolation
  bool useFragmentProgram =
    (this->UseFragmentProgram &&
     property->GetInterpolationType() == VTK_CUBIC_INTERPOLATION &&
     !this->ExactPixelMatch);

  // verify that the orientation and slice has not changed
  bool orientationChanged = (this->Orientation != this->LastOrientation);
  this->LastOrientation = this->Orientation;
  bool sliceChanged = (this->SliceNumber != this->LastSliceNumber);
  this->LastSliceNumber = this->SliceNumber;

  // get the mtime of the property, including the lookup table
  unsigned long propertyMTime = 0;
  bool checkerboard = false;
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
      checkerboard = (property->GetCheckerboard() != 0);
      }
    }

  // need to reload the texture
  if (this->vtkImageMapper3D::GetMTime() > loadTime ||
      propertyMTime > loadTime ||
      input->GetMTime() > loadTime ||
      orientationChanged || sliceChanged ||
      renWin != this->RenderWindow ||
      renWin->GetContextCreationTime() > loadTime ||
      recursive)
    {
    // whether to try to use the input data directly as the texture
    bool reuseData = true;

    if (checkerboard)
      {
      reuseData = false;
      }

    // get the data to load as a texture
    int xsize = this->TextureSize[0];
    int ysize = this->TextureSize[1];
    int bytesPerPixel = this->TextureBytesPerPixel;

    unsigned char *data = this->MakeTextureData(
      (this->PassColorData ? 0 : property), input, extent, xsize, ysize,
      bytesPerPixel, reuseTexture, reuseData);

    if (checkerboard)
      {
      this->CheckerboardImage(
        data, xsize, ysize, input->GetSpacing(), property);
      }

    GLuint tempIndex = 0;

#ifdef GL_VERSION_1_1
    if (reuseTexture)
      {
      glBindTexture(GL_TEXTURE_2D, this->Index);
      }
    else
#endif
      {
      // free any old display lists
      this->ReleaseGraphicsResources(renWin);
      this->RenderWindow = renWin;

      // define a display list for this texture
      // get a unique display list id
#ifdef GL_VERSION_1_1
      glGenTextures(1, &tempIndex);
      this->Index = static_cast<long>(tempIndex);
      glBindTexture(GL_TEXTURE_2D, this->Index);
#else
      this->Index = glGenLists(1);
      glDeleteLists(static_cast<GLuint>(this->Index),
                    static_cast<GLsizei>(0));
      glNewList(static_cast<GLuint>(this->Index), GL_COMPILE);
#endif

      renWin->RegisterTextureResource(this->Index);
      }

    GLenum interp = GL_LINEAR;
    if (property->GetInterpolationType() == VTK_NEAREST_INTERPOLATION &&
        !this->ExactPixelMatch)
      {
      interp = GL_NEAREST;
      }

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interp);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interp);

    GLenum wrap = (this->UseClampToEdge ? vtkgl::CLAMP_TO_EDGE : GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

    GLenum format = GL_LUMINANCE;
    int internalFormat = bytesPerPixel;
    switch (bytesPerPixel)
      {
      case 1: format = GL_LUMINANCE; break;
      case 2: format = GL_LUMINANCE_ALPHA; break;
      case 3: format = GL_RGB; break;
      case 4: format = GL_RGBA; break;
      }

#ifdef GL_VERSION_1_1
    // if we are using OpenGL 1.1, force 32 bit textures
    switch (bytesPerPixel)
      {
      case 1: internalFormat = GL_LUMINANCE8; break;
      case 2: internalFormat = GL_LUMINANCE8_ALPHA8; break;
      case 3: internalFormat = GL_RGB8; break;
      case 4: internalFormat = GL_RGBA8; break;
      }
#endif

    if (useFragmentProgram && this->FragmentShaderIndex == 0)
      {
      // Use the the ancient ARB_fragment_program extension, it works
      // reliably even with very old hardware and drivers
      vtkgl::GenProgramsARB(1, &tempIndex);
      this->FragmentShaderIndex = static_cast<long>(tempIndex);

      vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB,
                            this->FragmentShaderIndex);
      const char *prog = vtkTextureBicubicARB_fp;
      vtkgl::ProgramStringARB(vtkgl::FRAGMENT_PROGRAM_ARB,
                              vtkgl::PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<GLsizei>(strlen(prog)), prog);

      GLint erri;
      glGetIntegerv(vtkgl::PROGRAM_ERROR_POSITION_ARB, &erri);
      if (erri != -1)
        {
        vtkErrorMacro("Failed to load bicubic shader program: "
                      << reinterpret_cast<const char *>(
                           glGetString(vtkgl::PROGRAM_ERROR_STRING_ARB)));
        }
      }

#ifdef GL_VERSION_1_1
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    if (reuseTexture)
      {
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                      xsize, ysize, format, GL_UNSIGNED_BYTE,
                      static_cast<const GLvoid *>(data));
      }
    else
#endif
      {
      glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                   xsize, ysize, 0, format,
                   GL_UNSIGNED_BYTE, static_cast<const GLvoid *>(data));
      this->TextureSize[0] = xsize;
      this->TextureSize[1] = ysize;
      this->TextureBytesPerPixel = bytesPerPixel;
      }

#ifndef GL_VERSION_1_1
    glEndList();
#endif
    // modify the load time to the current time
    this->LoadTime.Modified();
    if (!reuseData)
      {
      delete [] data;
      }
    }

  // execute the display list that uses creates the texture
#ifdef GL_VERSION_1_1
  glBindTexture(GL_TEXTURE_2D, this->Index);
#else
  glCallList(static_cast<GLuint>(this->Index));
#endif

  if (useFragmentProgram)
    {
    // Bind the bicubic interpolation fragment program, it will
    // not do anything if modern shader objects are also in play.
    vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB,
                          this->FragmentShaderIndex);
    vtkgl::ProgramLocalParameter4fARB(vtkgl::FRAGMENT_PROGRAM_ARB, 0,
                          static_cast<float>(this->TextureSize[0]),
                          static_cast<float>(this->TextureSize[1]),
                          static_cast<float>(1.0/this->TextureSize[0]),
                          static_cast<float>(1.0/this->TextureSize[1]));
    glEnable(vtkgl::FRAGMENT_PROGRAM_ARB);
    }

  glEnable(GL_TEXTURE_2D);

  // modulate the texture with the fragment for lighting effects
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  static double normals[3][3] =
    { { 1.0, 0.0, 0.0 }, { 0.0, -1.0, 0.0 }, { 0.0, 0.0, 1.0 } };
  double *normal = normals[(this->Orientation % 3)];

  if (!this->Points || (this->ExactPixelMatch && this->SliceFacesCamera))
    {
    // use a full-screen quad if slice faces camera, this ensures that all
    // images showing the same "slice" use exactly the same geometry, which
    // helps to avoid some depth-buffer coincidence issues
    double coords[12], tcoords[8];
    this->MakeTextureGeometry(input, extent, this->Border, coords, tcoords);

    glBegin(GL_QUADS);
    for (int i = 0; i < 4; i++)
      {
      glNormal3dv(normal);
      glTexCoord2dv(&tcoords[i*2]);
      glVertex3dv(&coords[i*3]);
      }
    glEnd();
    }
  else if (this->Points->GetNumberOfPoints())
    {
    vtkPoints *points = this->Points;
    vtkIdType ncoords = points->GetNumberOfPoints(); 
    double coord[3];
    double tcoord[2];
    double *spacing = input->GetSpacing();
    double *origin = input->GetOrigin();
    int xdim, ydim;
    vtkImageSliceMapper::GetDimensionIndices(this->Orientation, xdim, ydim);

    glBegin((ncoords == 4) ? GL_QUADS : GL_POLYGON);
    for (vtkIdType i = 0; i < ncoords; i++)
      {
      points->GetPoint(i, coord);
      tcoord[0] = ((coord[0] - origin[xdim] + 0.5*spacing[xdim])/
                   (this->TextureSize[0]*spacing[xdim]));
      tcoord[1] = ((coord[1] - origin[ydim] + 0.5*spacing[ydim])/
                   (this->TextureSize[ydim]*spacing[ydim]));

      glNormal3dv(normal);
      glTexCoord2dv(tcoord);
      glVertex3dv(coord);
      }
    glEnd();
    }

  if (useFragmentProgram)
    {
    glDisable(vtkgl::FRAGMENT_PROGRAM_ARB);
    }
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
      int powerOfTwo = 1;
      while (powerOfTwo < imageSize[i])
        {
        powerOfTwo <<= 1;
        }
      textureSize[i] = powerOfTwo;
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
#ifdef GL_VERSION_1_1
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

  // if it does fit, we will render it later
  return (params == 0 ? 0 : 1);
#else
  // Otherwise we are version 1.0 and we'll just assume the card
  // can do 1024x1024
  return (size[0] <= 1024 && size[1] <= 1024);
#endif
}

//----------------------------------------------------------------------------
// Set the modelview transform and load the texture
void vtkOpenGLImageSliceMapper::Render(vtkRenderer *ren, vtkImageSlice *prop)
{
  vtkOpenGLRenderWindow *renWin =
    vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());

  if (renWin && (renWin != this->RenderWindow ||
      renWin->GetContextCreationTime() > this->LoadTime.GetMTime()))
    {
    this->CheckOpenGLCapabilities(renWin);
    }

  // time the render
  this->Timer->StartTimer();

  // OpenGL matrices are column-order, not row-order like VTK
  vtkMatrix4x4 *matrix = this->GetDataToWorldMatrix();
  double mat[16];
  vtkMatrix4x4::Transpose(*matrix->Element, mat);

  // insert model transformation
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glMultMatrixd(mat);

  // push a bunch of OpenGL state items, so they can be popped later:
  // GL_ALPHA_TEST, GL_DEPTH_TEST, GL_COLOR_MATERIAL, GL_CULL_FACE,
  // GL_LIGHTING, GL_CLIP_PLANE, GL_TEXTURE_2D
  glPushAttrib(GL_ENABLE_BIT);

  // and now enable/disable as needed for our render
  glDisable(GL_CULL_FACE);
  glDisable(GL_COLOR_MATERIAL);

  // don't accept fragments if they have zero opacity:
  // this will stop the zbuffer from being blocked by totally
  // transparent texture fragments.
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, static_cast<GLclampf>(0));

  // depth peeling
  vtkOpenGLRenderer *oRenderer = static_cast<vtkOpenGLRenderer *>(ren);

  if (oRenderer->GetDepthPeelingHigherLayer())
    {
    GLint uUseTexture = oRenderer->GetUseTextureUniformVariable();
    GLint uTexture = oRenderer->GetTextureUniformVariable();
    vtkgl::Uniform1i(uUseTexture, 1);
    vtkgl::Uniform1i(uTexture, 0); // active texture 0
    }

#ifdef GL_VERSION_1_1
  // do an offset to avoid depth buffer issues
  if (vtkMapper::GetResolveCoincidentTopology() !=
      VTK_RESOLVE_SHIFT_ZBUFFER )
    {
    double f, u;
    glEnable(GL_POLYGON_OFFSET_FILL);
    vtkMapper::GetResolveCoincidentTopologyPolygonOffsetParameters(f,u);
    glPolygonOffset(f,u);
    }
#endif

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
  glDepthMask(this->DepthEnable ? GL_TRUE : GL_FALSE);
  if (!this->ColorEnable && !this->MatteEnable)
    {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    }

  // color and lighting related items
  vtkImageProperty *property = prop->GetProperty();
  double opacity = property->GetOpacity();
  double ambient = property->GetAmbient();
  double diffuse = property->GetDiffuse();

  // render the backing polygon
  int backing = property->GetBacking();
  double *bcolor = property->GetBackingColor();
  if (backing &&
      (this->MatteEnable || (this->DepthEnable && !this->ColorEnable)))
    {
    // the backing polygon is always opaque
    this->RenderColorAndLighting(
      bcolor[0], bcolor[1], bcolor[2], 1.0, ambient, diffuse);
    this->RenderBackingPolygon();
    }

  // render the texture
  if (this->ColorEnable || (!backing && this->DepthEnable))
    {
    this->RenderColorAndLighting(1.0, 1.0, 1.0, opacity, ambient, diffuse);

    this->RecursiveRenderTexturedPolygon(
      ren, property, this->GetInput(), this->DisplayExtent, false);
    }

  // Set the masks back again
  glDepthMask(GL_TRUE);
  if (!this->ColorEnable && !this->MatteEnable)
    {
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

  // pop the following attribs that were changed:
  // GL_ALPHA_TEST, GL_DEPTH_TEST, GL_COLOR_MATERIAL, GL_CULL_FACE,
  // GL_LIGHTING, GL_CLIP_PLANE, GL_TEXTURE_2D
  glPopAttrib();

  // pop transformation matrix
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  this->Timer->StopTimer();
  this->TimeToDraw = this->Timer->GetElapsedTime();
  if (this->TimeToDraw == 0)
    {
    this->TimeToDraw = 0.0001;
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLImageSliceMapper::RenderColorAndLighting(
  double red, double green, double blue,
  double alpha, double ambient, double diffuse)
{
  glColor4f(red, green, blue, alpha);

  if (ambient == 1.0 && diffuse == 0.0)
    {
    glDisable(GL_LIGHTING);
    }
  else
    {
    float color[4];
    color[3] = alpha;
    glEnable(GL_LIGHTING);
    glShadeModel(GL_FLAT);
    color[0] = red*ambient;
    color[1] = green*ambient;
    color[2] = blue*ambient;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
    color[0] = red*diffuse;
    color[1] = green*diffuse;
    color[2] = blue*diffuse;
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
    color[0] = color[1] = color[2] = 0.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLImageSliceMapper::CheckOpenGLCapabilities(
  vtkOpenGLRenderWindow *renWin)
{
  vtkOpenGLExtensionManager *manager = 0;

  if (renWin)
    {
    manager = renWin->GetExtensionManager();
    }

  if (renWin && manager)
    {
    this->UseClampToEdge =
      (manager->ExtensionSupported("GL_VERSION_1_2") ||
       manager->ExtensionSupported("GL_EXT_texture_edge_clamp"));
    this->UsePowerOfTwoTextures =
      !(manager->ExtensionSupported("GL_VERSION_2_0") ||
        manager->ExtensionSupported("GL_ARB_texture_non_power_of_two"));
    this->UseFragmentProgram =
      (manager->ExtensionSupported("GL_VERSION_1_3") &&
       manager->LoadSupportedExtension("GL_ARB_fragment_program"));
    }
  else
    {
    this->UseClampToEdge = false;
    this->UsePowerOfTwoTextures = true;
    this->UseFragmentProgram = false;
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLImageSliceMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
