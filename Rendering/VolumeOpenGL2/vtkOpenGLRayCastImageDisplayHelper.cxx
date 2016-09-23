/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRayCastImageDisplayHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLRayCastImageDisplayHelper.h"

#include "vtkObjectFactory.h"
#include "vtkVolume.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkFixedPointRayCastImage.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkTextureObject.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkShaderProgram.h"

#include "vtkOpenGLHelper.h"

#include "vtkOpenGLError.h"

#include <cmath>

#include "vtkTextureObjectVS.h"  // a pass through shader

vtkStandardNewMacro(vtkOpenGLRayCastImageDisplayHelper);

//----------------------------------------------------------------------------
// Construct a new vtkOpenGLRayCastImageDisplayHelper with default values
vtkOpenGLRayCastImageDisplayHelper::vtkOpenGLRayCastImageDisplayHelper()
{
  this->TextureObject = vtkTextureObject::New();
  this->ShaderProgram = NULL;
}

//----------------------------------------------------------------------------
// Destruct a vtkOpenGLRayCastImageDisplayHelper - clean up any memory used
vtkOpenGLRayCastImageDisplayHelper::~vtkOpenGLRayCastImageDisplayHelper()
{
  if (this->TextureObject)
  {
    this->TextureObject->Delete();
    this->TextureObject = 0;
  }
  if (this->ShaderProgram)
  {
    delete this->ShaderProgram;
    this->ShaderProgram = 0;
  }
}

//----------------------------------------------------------------------------
// imageMemorySize   is how big the texture is - this is always a power of two
//
// imageViewportSize is how big the renderer viewport is in pixels
//
// imageInUseSize    is the rendered image - equal or smaller than imageMemorySize
//                   and imageViewportSize
//
// imageOrigin       is the starting pixel of the imageInUseSize image on the
//                   the imageViewportSize viewport
//
void vtkOpenGLRayCastImageDisplayHelper::RenderTexture( vtkVolume *vol,
                                                        vtkRenderer *ren,
                                                        vtkFixedPointRayCastImage *image,
                                                        float requestedDepth )
{
  this->RenderTextureInternal( vol, ren, image->GetImageMemorySize(), image->GetImageViewportSize(),
                       image->GetImageInUseSize(), image->GetImageOrigin(),
                       requestedDepth, VTK_UNSIGNED_SHORT, image->GetImage() );
}

//----------------------------------------------------------------------------
void vtkOpenGLRayCastImageDisplayHelper::RenderTexture( vtkVolume *vol,
                                                        vtkRenderer *ren,
                                                        int imageMemorySize[2],
                                                        int imageViewportSize[2],
                                                        int imageInUseSize[2],
                                                        int imageOrigin[2],
                                                        float requestedDepth,
                                                        unsigned char *image )
{
  this->RenderTextureInternal( vol, ren, imageMemorySize, imageViewportSize,
                               imageInUseSize, imageOrigin, requestedDepth,
                               VTK_UNSIGNED_CHAR, static_cast<void *>(image) );
}

//----------------------------------------------------------------------------
void vtkOpenGLRayCastImageDisplayHelper::RenderTexture( vtkVolume *vol,
                                                        vtkRenderer *ren,
                                                        int imageMemorySize[2],
                                                        int imageViewportSize[2],
                                                        int imageInUseSize[2],
                                                        int imageOrigin[2],
                                                        float requestedDepth,
                                                        unsigned short *image )
{
  this->RenderTextureInternal( vol, ren, imageMemorySize, imageViewportSize,
                               imageInUseSize, imageOrigin, requestedDepth,
                               VTK_UNSIGNED_SHORT, static_cast<void *>(image) );
}

//----------------------------------------------------------------------------
void vtkOpenGLRayCastImageDisplayHelper::RenderTextureInternal( vtkVolume *vol,
                                                                vtkRenderer *ren,
                                                                int imageMemorySize[2],
                                                                int imageViewportSize[2],
                                                                int imageInUseSize[2],
                                                                int imageOrigin[2],
                                                                float requestedDepth,
                                                                int imageScalarType,
                                                                void *image )
{
  vtkOpenGLClearErrorMacro();

  // Set the context
  vtkOpenGLRenderWindow *ctx =
    vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  this->TextureObject->SetContext(ctx);

  float offsetX, offsetY;

  float depth;
  if ( requestedDepth > 0.0 && requestedDepth <= 1.0 )
  {
    depth = requestedDepth*2.0 - 1.0;
  }
  else
  {
    // Pass the center of the volume through the world to view function
    // of the renderer to get the z view coordinate to use for the
    // view to world transformation of the image bounds. This way we
    // will draw the image at the depth of the center of the volume
    ren->SetWorldPoint( vol->GetCenter()[0],
                        vol->GetCenter()[1],
                        vol->GetCenter()[2],
                        1.0 );
    ren->WorldToDisplay();
    depth = ren->GetDisplayPoint()[2];
  }

  // Don't write into the Zbuffer - just use it for comparisons
  glDepthMask( 0 );

  this->TextureObject->SetMinificationFilter(vtkTextureObject::Linear);
  this->TextureObject->SetMagnificationFilter(vtkTextureObject::Linear);

  if ( imageScalarType == VTK_UNSIGNED_CHAR )
  {
    this->TextureObject->Create2DFromRaw(
      imageMemorySize[0], imageMemorySize[1], 4,
      VTK_UNSIGNED_CHAR, static_cast<unsigned char *>(image));
  }
  else
  {
    this->TextureObject->Create2DFromRaw(
      imageMemorySize[0], imageMemorySize[1], 4,
      VTK_UNSIGNED_SHORT, static_cast<unsigned short *>(image));
  }

  offsetX = .5 / static_cast<float>(imageMemorySize[0]);
  offsetY = .5 / static_cast<float>(imageMemorySize[1]);

  float tcoords[8];
  tcoords[0]  = 0.0 + offsetX;
  tcoords[1]  = 0.0 + offsetY;
  tcoords[2]  =
    (float)imageInUseSize[0]/(float)imageMemorySize[0] - offsetX;
  tcoords[3]  = offsetY;
  tcoords[4]  =
    (float)imageInUseSize[0]/(float)imageMemorySize[0] - offsetX;
  tcoords[5]  =
    (float)imageInUseSize[1]/(float)imageMemorySize[1] - offsetY;
  tcoords[6]  = offsetX;
  tcoords[7]  =
    (float)imageInUseSize[1]/(float)imageMemorySize[1] - offsetY;

  float verts[12] = {
    2.0f*imageOrigin[0]/imageViewportSize[0] - 1.0f,
    2.0f*imageOrigin[1]/imageViewportSize[1] - 1.0f, depth,
    2.0f*(imageOrigin[0]+imageInUseSize[0]) /
    imageViewportSize[0] - 1.0f,
    2.0f*imageOrigin[1]/imageViewportSize[1] - 1.0f, depth,
    2.0f*(imageOrigin[0]+imageInUseSize[0]) /
    imageViewportSize[0] - 1.0f,
    2.0f*(imageOrigin[1]+imageInUseSize[1]) /
    imageViewportSize[1] - 1.0f, depth,
    2.0f*imageOrigin[0]/imageViewportSize[0] - 1.0f,
    2.0f*(imageOrigin[1]+imageInUseSize[1]) /
    imageViewportSize[1] - 1.0f, depth};

  if (!this->ShaderProgram)
  {
    this->ShaderProgram = new vtkOpenGLHelper;

    // build the shader source code
    std::string VSSource = vtkTextureObjectVS;
    std::string FSSource =
      "//VTK::System::Dec\n"
      "//VTK::Output::Dec\n"
      "varying vec2 tcoordVC;\n"
      "uniform sampler2D source;\n"
      "uniform float scale;\n"
      "void main(void)\n"
      "{\n"
      "  gl_FragData[0] = texture2D(source,tcoordVC)*scale;\n"
      "}\n";
    std::string GSSource;

    // compile and bind it if needed
    vtkShaderProgram *newShader =
      ctx->GetShaderCache()->ReadyShaderProgram(VSSource.c_str(),
                                         FSSource.c_str(),
                                         GSSource.c_str());

    // if the shader changed reinitialize the VAO
    if (newShader != this->ShaderProgram->Program)
    {
      this->ShaderProgram->Program = newShader;
      this->ShaderProgram->VAO->ShaderProgramChanged(); // reset the VAO as the shader has changed
    }

    this->ShaderProgram->ShaderSourceTime.Modified();
  }
  else
  {
    ctx->GetShaderCache()->ReadyShaderProgram(this->ShaderProgram->Program);
  }

  glEnable(GL_BLEND);

  // backup current GL blend state
  GLint blendSrcA = GL_SRC_ALPHA;
  GLint blendDstA = GL_ONE;
  GLint blendSrcC = GL_SRC_ALPHA;
  GLint blendDstC = GL_ONE;
  glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcA);
  glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstA);
  glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcC);
  glGetIntegerv(GL_BLEND_DST_RGB, &blendDstC);

  if (this->PreMultipliedColors)
  {
    // make the blend function correct for textures premultiplied by alpha.
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  }

  // bind and activate this texture
  this->TextureObject->Activate();
  int sourceId = this->TextureObject->GetTextureUnit();
  this->ShaderProgram->Program->SetUniformi("source",sourceId);
  this->ShaderProgram->Program->SetUniformf("scale",this->PixelScale);
  vtkOpenGLRenderUtilities::RenderQuad(verts, tcoords, this->ShaderProgram->Program,
    this->ShaderProgram->VAO);
  this->TextureObject->Deactivate();

  // restore GL blend state
  glBlendFuncSeparate(blendSrcC,blendDstC,blendSrcA,blendDstA);

  vtkOpenGLCheckErrorMacro("failed after RenderTextureInternal");
}

//----------------------------------------------------------------------------
void vtkOpenGLRayCastImageDisplayHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkOpenGLRayCastImageDisplayHelper::ReleaseGraphicsResources(vtkWindow *win)
{
  this->TextureObject->ReleaseGraphicsResources(win);
  if (this->ShaderProgram)
  {
    this->ShaderProgram->ReleaseGraphicsResources(win);
    delete this->ShaderProgram;
    this->ShaderProgram = NULL;
  }
}
