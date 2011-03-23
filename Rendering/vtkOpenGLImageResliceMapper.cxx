/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageResliceMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLImageResliceMapper.h"

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkImageSlice.h"
#include "vtkImageProperty.h"
#include "vtkDataArray.h"
#include "vtkLookupTable.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"
#include "vtkMapper.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkImageResliceToColors.h"
#include "vtkTimerLog.h"
#include "vtkGarbageCollector.h"
#include "vtkTemplateAliasMacro.h"

#include <math.h>

#include "vtkOpenGL.h"
#include "vtkgl.h" // vtkgl namespace

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLImageResliceMapper);
#endif

//----------------------------------------------------------------------------
// Initializes an instance, generates a unique index.
vtkOpenGLImageResliceMapper::vtkOpenGLImageResliceMapper()
{
  this->Index = 0;
  this->RenderWindow = 0;
  this->TextureSize[0] = 0;
  this->TextureSize[1] = 0;
  this->TextureBytesPerPixel = 1;

  this->UseClampToEdge = false;
  this->UsePowerOfTwoTextures = true;
}

//----------------------------------------------------------------------------
vtkOpenGLImageResliceMapper::~vtkOpenGLImageResliceMapper()
{
  this->RenderWindow = NULL;
}

//----------------------------------------------------------------------------
// Release the graphics resources used by this texture.
void vtkOpenGLImageResliceMapper::ReleaseGraphicsResources(vtkWindow *renWin)
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
  this->RenderWindow = NULL;
  this->Modified();
}

//----------------------------------------------------------------------------
// Load the given image extent into a texture and render it
void vtkOpenGLImageResliceMapper::InternalLoad(
  vtkRenderer *ren, vtkProp3D *vtkNotUsed(prop), vtkImageProperty *property,
  vtkImageData *input, int extent[6], bool recursive)
{
  vtkOpenGLRenderWindow *renWin =
    static_cast<vtkOpenGLRenderWindow *>(ren->GetRenderWindow());

  // Get the mtime of the property, including the lookup table
  unsigned long propertyMTime = 0;
  if (property)
    {
    propertyMTime = property->GetMTime();
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

  // get the previous load time
  unsigned long loadTime = this->LoadTime.GetMTime();

  // need to reload the texture
  if (this->vtkImageMapper3D::GetMTime() > loadTime ||
      propertyMTime > loadTime ||
      this->WorldToDataMatrix->GetMTime() > loadTime ||
      input->GetMTime() > loadTime ||
      renWin != this->RenderWindow ||
      renWin->GetContextCreationTime() > loadTime ||
      recursive)
    {
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

    // get the data to load as a texture
    int xsize = this->TextureSize[0];
    int ysize = this->TextureSize[1];
    int bytesPerPixel = this->TextureBytesPerPixel;
    bool releaseData;

    unsigned char *data = this->MakeTextureData(
      0, input, extent, xsize, ysize, bytesPerPixel, reuseTexture,
      releaseData);

    this->MakeTextureCutGeometry(input, extent, this->Border,
      this->Coords, this->TCoords, this->NCoords);

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
      glNewList (static_cast<GLuint>(this->Index), GL_COMPILE);
#endif

      static_cast<vtkOpenGLRenderWindow *>(ren->GetRenderWindow())
        ->RegisterTextureResource(this->Index);
      }

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
    if (releaseData)
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

  // don't accept fragments if they have zero opacity:
  // this will stop the zbuffer from be blocked by totally
  // transparent texture fragments.
  glAlphaFunc(GL_GREATER, static_cast<GLclampf>(0));
  glEnable(GL_ALPHA_TEST);

  // now bind it
  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

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

  // set the OpenGL state and draw the quad
  glDisable(GL_CULL_FACE);
  glDisable(GL_COLOR_MATERIAL);

  double opacity = property->GetOpacity();
  double ambient = property->GetAmbient();
  double diffuse = property->GetDiffuse();

  if (ambient == 1.0 && diffuse == 0.0)
    {
    glDisable(GL_LIGHTING);
    }
  else
    {
    float color[4];
    color[3] = opacity;
    glEnable(GL_LIGHTING);
    glShadeModel(GL_FLAT);
    color[0] = color[1] = color[2] = ambient;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
    color[0] = color[1] = color[2] = diffuse;
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
    color[0] = color[1] = color[2] = 0.0;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
    }

  glColor4f(1.0, 1.0, 1.0, opacity);

  // Add all the clipping planes
  int numClipPlanes = this->GetNumberOfClippingPlanes();
  if (numClipPlanes > 6)
    {
    vtkErrorMacro(<< "OpenGL has a limit of 6 clipping planes");
    numClipPlanes = 6;
    }

  int i;
  for (i = 0; i < numClipPlanes; i++)
    {
    double planeEquation[4];
    this->GetClippingPlaneInDataCoords(this->SliceToWorldMatrix,
                                       i, planeEquation);
    GLenum clipPlaneId = static_cast<GLenum>(GL_CLIP_PLANE0+i);
    glEnable(clipPlaneId);
    glClipPlane(clipPlaneId, planeEquation);
    }

  if (this->NCoords)
    {
    glBegin((this->NCoords == 4) ? GL_QUADS : GL_POLYGON);
    for (i = 0; i < this->NCoords; i++)
      {
      glTexCoord2dv(&this->TCoords[i*2]);
      glVertex3dv(&this->Coords[i*3]);
      }
    glEnd();
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    GLenum clipPlaneId = static_cast<GLenum>(GL_CLIP_PLANE0+i);
    glDisable(clipPlaneId);
    }

  if (ambient == 1.0 && diffuse == 0.0)
    {
    glEnable(GL_LIGHTING);
    }
}

//----------------------------------------------------------------------------
// Determine if a given texture size is supported by the video card
bool vtkOpenGLImageResliceMapper::TextureSizeOK(const int size[2])
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
// Load the texture and the geometry
void vtkOpenGLImageResliceMapper::Load(
  vtkRenderer *ren, vtkProp3D *prop, vtkImageProperty *property)
{
  vtkImageResliceToColors *reslice = this->ImageReslice;
  vtkScalarsToColors *lookupTable = this->DefaultLookupTable;

  reslice->SetInput(this->GetInput());

  if (property)
    {
    double colorWindow = property->GetColorWindow();
    double colorLevel = property->GetColorLevel();
    if (property->GetLookupTable())
      {
      lookupTable = property->GetLookupTable();
      if (!property->GetUseLookupTableScalarRange())
        {
        lookupTable->SetRange(colorLevel - 0.5*colorWindow,
                              colorLevel + 0.5*colorWindow);
        }
      }
    else
      {
      lookupTable->SetRange(colorLevel - 0.5*colorWindow,
                            colorLevel + 0.5*colorWindow);
      }
    }
  else
    {
    lookupTable->SetRange(0, 255);
    }

  reslice->SetLookupTable(lookupTable);
  reslice->UpdateWholeExtent();

  this->RecursiveLoad(
    ren, prop, property, reslice->GetOutput(),
    reslice->GetOutputExtent(), false);
}

//----------------------------------------------------------------------------
// Set the modelview transform and load the texture
void vtkOpenGLImageResliceMapper::Render(vtkRenderer *ren, vtkImageSlice *prop)
{
  vtkImageProperty *property = prop->GetProperty();

  // set the matrices
  this->UpdateWorldToDataMatrix(prop);
  this->UpdateSliceToWorldMatrix(ren->GetActiveCamera());

  // set the reslice spacing/origin/extent/axes
  this->UpdateResliceInformation(ren);

  // set the reslice bits related to the property
  this->UpdateResliceInterpolation(property);

  // the hefty glPushAttrib call
  glPushAttrib(GL_ENABLE_BIT);

  // for picking
  glDepthMask(GL_TRUE);

  // transpose VTK matrix to create OpenGL matrix
  double mat[16];
  vtkMatrix4x4::Transpose(*this->SliceToWorldMatrix->Element, mat);

  // insert model transformation
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glMultMatrixd(mat);

  // time the render
  this->Timer->StartTimer();

  // render the texture
  this->Load(ren, prop, property);

  this->Timer->StopTimer();
  this->TimeToDraw = this->Timer->GetElapsedTime();
  if (this->TimeToDraw == 0)
    {
    this->TimeToDraw = 0.0001;
    }

  // pop transformation matrix
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glPopAttrib();
}

//----------------------------------------------------------------------------
void vtkOpenGLImageResliceMapper::CheckOpenGLCapabilities(
  vtkOpenGLRenderWindow *renWin)
{
  vtkOpenGLExtensionManager *manager = 0;

  if (renWin)
    {
    manager = renWin->GetExtensionManager();
    }

  if (renWin && manager)
    {
    this->UsePowerOfTwoTextures =
      !(manager->ExtensionSupported("GL_VERSION_2_0") ||
        manager->ExtensionSupported("GL_ARB_texture_non_power_of_two"));

    this->UseClampToEdge =
      (manager->ExtensionSupported("GL_VERSION_1_2") ||
       manager->ExtensionSupported("GL_EXT_texture_edge_clamp"));
    }
  else
    {
    this->UsePowerOfTwoTextures = true;
    this->UseClampToEdge = false;
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLImageResliceMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
