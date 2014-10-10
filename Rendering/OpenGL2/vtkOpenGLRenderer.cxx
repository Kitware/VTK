/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenGLRenderer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLRenderer.h"

#include "vtkNew.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkFloatArray.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkTrivialProducer.h"
#include "vtkTexturedActor2D.h"

#include "vtkglVBOHelper.h"

#include "vtkCuller.h"
#include "vtkLightCollection.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLCamera.h"
#include "vtkLight.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLTexture.h"
#include "vtkTimerLog.h"
#include "vtkRenderPass.h"
#include "vtkRenderState.h"

#include "vtkTextureObject.h"

#include "vtkOpenGLError.h"

#include <math.h>
#include <cassert>
#include <list>

#include "vtkImageData.h"

class vtkGLPickInfo
{
public:
  unsigned int PickedId;
  unsigned int NumPicked;
  bool PerformedHardwarePick;
  std::map<unsigned int,float> PickValues;
};

vtkStandardNewMacro(vtkOpenGLRenderer);

vtkCxxSetObjectMacro(vtkOpenGLRenderer, Pass, vtkRenderPass);

vtkOpenGLRenderer::vtkOpenGLRenderer()
{
  this->PickInfo = new vtkGLPickInfo;
  this->PickInfo->PickedId = 0;
  this->PickInfo->NumPicked = 0;
  this->PickedZ = 0;

  this->DepthPeelingIsSupported=0;
  this->DepthPeelingIsSupportedChecked=0;

  this->DepthZData = 0;
  this->OpaqueZTexture = NULL;
  this->TranslucentZTexture = NULL;
  this->OpaqueRGBATexture = NULL;
  this->TranslucentRGBATexture = NULL;
  this->CurrentRGBATexture = NULL;
  this->DepthPeelingActor = NULL;

  this->DepthPeelingHigherLayer=0;

  this->BackgroundTexture = 0;
  this->Pass = 0;
}

// Ask lights to load themselves into graphics pipeline.
int vtkOpenGLRenderer::UpdateLights ()
{
  vtkOpenGLClearErrorMacro();

  vtkLight *light;
  float status;
  int count = 0;

  vtkCollectionSimpleIterator sit;
  for(this->Lights->InitTraversal(sit);
      (light = this->Lights->GetNextLight(sit)); )
    {
    status = light->GetSwitch();
    if (status > 0.0)
      {
      count++;
      }
    }

  if( !count )
    {
    vtkDebugMacro(<<"No lights are on, creating one.");
    this->CreateLight();
    }

  for(this->Lights->InitTraversal(sit);
      (light = this->Lights->GetNextLight(sit)); )
    {
    status = light->GetSwitch();

    // if the light is on then define it and bind it.
    if (status > 0.0)
      {
      light->Render(this,0);
      }
    }

  vtkOpenGLCheckErrorMacro("failed after UpdateLights");

  return count;
}

// ----------------------------------------------------------------------------
// Description:
// Is rendering at translucent geometry stage using depth peeling and
// rendering a layer other than the first one? (Boolean value)
// If so, the uniform variables UseTexture and Texture can be set.
// (Used by vtkOpenGLProperty or vtkOpenGLTexture)
int vtkOpenGLRenderer::GetDepthPeelingHigherLayer()
{
  return this->DepthPeelingHigherLayer;
}

// ----------------------------------------------------------------------------
// Concrete open gl render method.
void vtkOpenGLRenderer::DeviceRender(void)
{
  vtkTimerLog::MarkStartEvent("OpenGL Dev Render");

  if(this->Pass!=0)
    {
    vtkRenderState s(this);
    s.SetPropArrayAndCount(this->PropArray, this->PropArrayCount);
    s.SetFrameBuffer(0);
    this->Pass->Render(&s);
    }
  else
    {
    // Do not remove this MakeCurrent! Due to Start / End methods on
    // some objects which get executed during a pipeline update,
    // other windows might get rendered since the last time
    // a MakeCurrent was called.
    this->LastRenderingUsedDepthPeeling=0;
    this->RenderWindow->MakeCurrent();
    vtkOpenGLClearErrorMacro();

    this->UpdateCamera();
    this->UpdateLightGeometry();
    this->UpdateLights();
    this->UpdateGeometry();

    vtkOpenGLCheckErrorMacro("failed after DeviceRender");
    }

  vtkTimerLog::MarkEndEvent("OpenGL Dev Render");
}

vtkOpenGLTexture *vtkOpenGLRendererCreateDepthPeelingTexture(
  int width, int height, int numComponents, bool isDepth)
{
  vtkOpenGLTexture *result = vtkOpenGLTexture::New();

  vtkImageData *id = vtkImageData::New();
  id->SetExtent(0,width-1, 0,height-1, 0,0);

  if (isDepth == true)
    {
    id->AllocateScalars(VTK_FLOAT, numComponents);
    result->SetIsDepthTexture(1);
    }
  else
    {
    id->AllocateScalars(VTK_UNSIGNED_CHAR, numComponents);
    }

  result->SetTextureType(GL_TEXTURE_RECTANGLE);
  result->InterpolateOff();
  result->RepeatOff();
  result->SetInputData(id);
  return result;
}

vtkTextureObject *vtkOpenGLRendererCreateDepthPeelingTextureObject(
  vtkOpenGLRenderWindow *context, int width, int height, int numComponents, bool isDepth, void *initialData)
{
  vtkTextureObject *result = vtkTextureObject::New();
  result->SetContext(context);

  if (isDepth == true)
    {
    if (initialData)
      {
      result->CreateDepthFromRaw(
          width, height, vtkTextureObject::Float32, VTK_FLOAT, initialData);
      }
    else
      {
      result->AllocateDepth(width, height, vtkTextureObject::Float32);
      }
    }
  else
    {
    result->Allocate2D(width, height, numComponents, VTK_UNSIGNED_CHAR);
    }

  result->SetMinificationFilter(vtkTextureObject::Nearest);
  result->SetMagnificationFilter(vtkTextureObject::Nearest);
  result->SetWrapS(vtkTextureObject::ClampToEdge);
  result->SetWrapT(vtkTextureObject::ClampToEdge);
  result->SetWrapR(vtkTextureObject::ClampToEdge);
  return result;
}

// get the texture units for depth peeling
int vtkOpenGLRenderer::GetOpaqueRGBATextureUnit()
{
  if (this->OpaqueRGBATexture)
    {
    return this->OpaqueRGBATexture->GetTextureUnit();
    }
  return -1;
}
int vtkOpenGLRenderer::GetOpaqueZTextureUnit()
{
  if (this->OpaqueZTexture)
    {
    return this->OpaqueZTexture->GetTextureUnit();
    }
  return -1;
}
int vtkOpenGLRenderer::GetTranslucentRGBATextureUnit()
{
  if (this->TranslucentRGBATexture)
    {
    return this->TranslucentRGBATexture->GetTextureUnit();
    }
  return -1;
}
int vtkOpenGLRenderer::GetTranslucentZTextureUnit()
{
  if (this->TranslucentZTexture)
    {
    return this->TranslucentZTexture->GetTextureUnit();
    }
  return -1;
}
int vtkOpenGLRenderer::GetCurrentRGBATextureUnit()
{
  if (this->CurrentRGBATexture)
    {
    return this->CurrentRGBATexture->GetTextureUnit();
    }
  return -1;
}

// ----------------------------------------------------------------------------
// Description:
// Render translucent polygonal geometry. Default implementation just call
// UpdateTranslucentPolygonalGeometry().
// Subclasses of vtkRenderer that can deal with depth peeling must
// override this method.
void vtkOpenGLRenderer::DeviceRenderTranslucentPolygonalGeometry()
{
  vtkOpenGLClearErrorMacro();

  vtkOpenGLRenderWindow *context
    = vtkOpenGLRenderWindow::SafeDownCast(this->RenderWindow);
  if(this->UseDepthPeeling)
    {
    if (!context)
      {
      vtkErrorMacro("OpenGL render window is required.")
      return;
      }

    if(!this->DepthPeelingIsSupportedChecked)
      {
      this->DepthPeelingIsSupportedChecked=1;
      this->DepthPeelingIsSupported = true;
      }
    }

  if(!this->UseDepthPeeling || !this->DepthPeelingIsSupported)
    {
    // just alpha blending
    this->LastRenderingUsedDepthPeeling=0;
    this->UpdateTranslucentPolygonalGeometry();
    }
  else   // depth peeling.
    {
    // get the viewport dimensions
    this->GetTiledSizeAndOrigin(&this->ViewportWidth, &this->ViewportHeight,
                                &this->ViewportX, &this->ViewportY);

    // create textures we need if not done already
    if (this->DepthPeelingActor == NULL)
      {
      this->DepthPeelingActor = vtkTexturedActor2D::New();
      vtkNew<vtkPolyDataMapper2D> mapper;
      vtkNew<vtkPolyData> polydata;
      vtkNew<vtkPoints> points;
      points->SetNumberOfPoints(4);
      points->SetPoint(0, 0, 0, 0);
      points->SetPoint(1, this->ViewportWidth-1, 0, 0);
      points->SetPoint(2, this->ViewportWidth-1, this->ViewportHeight-1, 0);
      points->SetPoint(3, 0, this->ViewportHeight-1, 0);
      polydata->SetPoints(points.Get());

      vtkNew<vtkCellArray> tris;
      tris->InsertNextCell(3);
      tris->InsertCellPoint(0);
      tris->InsertCellPoint(1);
      tris->InsertCellPoint(2);
      tris->InsertNextCell(3);
      tris->InsertCellPoint(0);
      tris->InsertCellPoint(2);
      tris->InsertCellPoint(3);
      polydata->SetPolys(tris.Get());

      vtkNew<vtkTrivialProducer> prod;
      prod->SetOutput(polydata.Get());

      // Set some properties.
      mapper->SetInputConnection(prod->GetOutputPort());
      this->DepthPeelingActor->SetMapper(mapper.Get());
      }

    // has the size changed?
    if (this->OpaqueRGBATexture && (
          this->OpaqueRGBATexture->GetWidth() != static_cast<unsigned int>(this->ViewportWidth) ||
          this->OpaqueRGBATexture->GetHeight() != static_cast<unsigned int>(this->ViewportHeight)))
      {
      delete this->DepthZData;
      this->DepthZData = 0;

      this->OpaqueZTexture->UnRegister(this);
      this->OpaqueZTexture = 0;

      this->OpaqueRGBATexture->UnRegister(this);
      this->OpaqueRGBATexture = 0;

      this->TranslucentRGBATexture->UnRegister(this);
      this->TranslucentRGBATexture = 0;

      this->CurrentRGBATexture->UnRegister(this);
      this->CurrentRGBATexture = 0;

      vtkPolyData *polydata = vtkPolyDataMapper2D::SafeDownCast(this->DepthPeelingActor->GetMapper())->GetInput();
      polydata->GetPoints()->SetPoint(1, this->ViewportWidth-1, 0, 0);
      polydata->GetPoints()->SetPoint(2, this->ViewportWidth-1, this->ViewportHeight-1, 0);
      polydata->GetPoints()->SetPoint(3, 0, this->ViewportHeight-1, 0);
      polydata->GetPoints()->Modified();
      }

    // create textures we need if not done already
    if (this->OpaqueZTexture == NULL)
      {
      this->OpaqueZTexture = vtkOpenGLRendererCreateDepthPeelingTextureObject(
        context, this->ViewportWidth, this->ViewportHeight, 1, true, NULL);
      this->OpaqueRGBATexture = vtkOpenGLRendererCreateDepthPeelingTextureObject(
        context, this->ViewportWidth, this->ViewportHeight, 4, false, NULL);
      this->TranslucentRGBATexture = vtkOpenGLRendererCreateDepthPeelingTextureObject(
        context, this->ViewportWidth, this->ViewportHeight, 4, false, NULL);
      this->CurrentRGBATexture = vtkOpenGLRendererCreateDepthPeelingTextureObject(
        context, this->ViewportWidth, this->ViewportHeight, 4, false, NULL);
      this->DepthZData = new std::vector<float>(this->ViewportWidth * this->ViewportHeight, 0.0);
      }

    this->TranslucentZTexture = vtkOpenGLRendererCreateDepthPeelingTextureObject(
      context, this->ViewportWidth, this->ViewportHeight, 1, true, &((*this->DepthZData)[0]));

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
 //  glClearColor(0.0,0.0,0.0,0.0); // always clear to black
   // glClearDepth(static_cast<GLclampf>(1.0));
#ifdef GL_MULTISAMPLE
    glDisable(GL_MULTISAMPLE);
#endif
    glDisable(GL_BLEND);

    // Get opaqueRGBA and opaqueZ
    this->OpaqueRGBATexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
      this->ViewportX, this->ViewportY,
      this->ViewportWidth, this->ViewportHeight);
    this->OpaqueRGBATexture->Deactivate(); // deactivate & unbind to save texture resources
    this->OpaqueZTexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
      this->ViewportX, this->ViewportY,
      this->ViewportWidth, this->ViewportHeight);

    this->TranslucentZTexture->Activate();
    this->OpaqueZTexture->Activate();

    // Do render loop until complete
    unsigned int threshold=
      static_cast<unsigned int>(this->ViewportWidth*this->ViewportHeight*OcclusionRatio);

#if GL_ES_VERSION_2_0 != 1
    GLuint queryId;
    glGenQueries(1,&queryId);
#endif

    bool done = false;
    GLuint nbPixels = 0;
    int peelCount = 0;
    while(!done)
      {
      // clear the zbuffer and color buffers
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // render the translucent geometry
      this->LastRenderingUsedDepthPeeling=1;
#if GL_ES_VERSION_2_0 != 1
      glBeginQuery(GL_SAMPLES_PASSED,queryId);
#endif
      this->UpdateTranslucentPolygonalGeometry();

      // update translucentZ
      this->TranslucentZTexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
          this->ViewportX, this->ViewportY,
          this->ViewportWidth, this->ViewportHeight);

#if GL_ES_VERSION_2_0 != 1
      glEndQuery(GL_SAMPLES_PASSED);
      glGetQueryObjectuiv(queryId,GL_QUERY_RESULT,&nbPixels);
      if (nbPixels <= threshold)
        {
        done = true;
        }
#endif
      peelCount++;
      if(this->MaximumNumberOfPeels && peelCount >= this->MaximumNumberOfPeels)
        {
        done = true;
        }
      //cerr << "Pass " << peelCount << " pixels Drawn " << nbPixels << "\n";

      // blend the last two peels together
      if (peelCount > 1)
        {
        this->CurrentRGBATexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
          this->ViewportX, this->ViewportY,
          this->ViewportWidth, this->ViewportHeight);
        this->LastRenderingUsedDepthPeeling = 2;
        // take the TranslucentRGBA texture and blend it with the current frame buffer
        this->DepthPeelingActor->RenderOverlay(this);
        }

      // update translucent RGBA
      this->TranslucentRGBATexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
        this->ViewportX, this->ViewportY,
        this->ViewportWidth, this->ViewportHeight);
      }

    // unload the textures we are done with
    this->CurrentRGBATexture->Deactivate();
    this->OpaqueZTexture->Deactivate();
    this->TranslucentZTexture->UnRegister(this);
    this->TranslucentZTexture = 0;

    // blend in OpaqueRGBA
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    this->LastRenderingUsedDepthPeeling = 3;
    this->OpaqueRGBATexture->Activate();
    this->DepthPeelingActor->RenderOverlay(this);

    // unload the last two textures
    this->TranslucentRGBATexture->Deactivate();
    this->OpaqueRGBATexture->Deactivate();

    // done
    this->LastRenderingUsedDepthPeeling=1;
    }

  vtkOpenGLCheckErrorMacro("failed after DeviceRenderTranslucentPolygonalGeometry");
}


// ----------------------------------------------------------------------------
// Description:
// Render a peel layer. If there is no more GPU RAM to save the texture,
// return false otherwise returns true. Also if layer==0 and no prop have
// been rendered (there is no translucent geometry), it returns false.
// \pre positive_layer: layer>=0
int vtkOpenGLRenderer::RenderPeel(int vtkNotUsed(layer))
{
  // assert("pre: positive_layer" && layer>=0);
  return 0;
}



// ----------------------------------------------------------------------------
void vtkOpenGLRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PickedId" << this->PickInfo->PickedId<< "\n";
  os << indent << "NumPicked" << this->PickInfo->NumPicked<< "\n";
  os << indent << "PickedZ " << this->PickedZ << "\n";
  os << indent << "Pass:";
  if(this->Pass!=0)
    {
      os << "exists" << endl;
    }
  else
    {
      os << "null" << endl;
    }
}


void vtkOpenGLRenderer::Clear(void)
{
  vtkOpenGLClearErrorMacro();

  GLbitfield  clear_mask = 0;

  if (! this->Transparent())
    {
    if (this->IsPicking)
      {
      glClearColor(0.0,0.0,0.0,0.0);
      }
    else
      {
      glClearColor( static_cast<GLclampf>(this->Background[0]),
                    static_cast<GLclampf>(this->Background[1]),
                    static_cast<GLclampf>(this->Background[2]),
                    static_cast<GLclampf>(0.0));
      }
    clear_mask |= GL_COLOR_BUFFER_BIT;
    }

  if (!this->GetPreserveDepthBuffer())
    {
    //glClearDepth(static_cast<GLclampf>(1.0));
    clear_mask |= GL_DEPTH_BUFFER_BIT;
    }

  vtkDebugMacro(<< "glClear\n");
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glClear(clear_mask);

  // If gradient background is turned on, draw it now.
  if (!this->IsPicking && !this->Transparent() &&
      (this->GradientBackground || this->TexturedBackground))
    {
    int size[2];
    size[0] = this->GetSize()[0];
    size[1] = this->GetSize()[1];

    double tile_viewport[4];
    this->GetRenderWindow()->GetTileViewport(tile_viewport);

    vtkNew<vtkTexturedActor2D> actor;
    vtkNew<vtkPolyDataMapper2D> mapper;
    vtkNew<vtkPolyData> polydata;
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(4);
    points->SetPoint(0, 0, 0, 0);
    points->SetPoint(1, size[0], 0, 0);
    points->SetPoint(2, size[0], size[1], 0);
    points->SetPoint(3, 0, size[1], 0);
    polydata->SetPoints(points.Get());

    vtkNew<vtkCellArray> tris;
    tris->InsertNextCell(3);
    tris->InsertCellPoint(0);
    tris->InsertCellPoint(1);
    tris->InsertCellPoint(2);
    tris->InsertNextCell(3);
    tris->InsertCellPoint(0);
    tris->InsertCellPoint(2);
    tris->InsertCellPoint(3);
    polydata->SetPolys(tris.Get());

    vtkNew<vtkTrivialProducer> prod;
    prod->SetOutput(polydata.Get());

    // Set some properties.
    mapper->SetInputConnection(prod->GetOutputPort());
    actor->SetMapper(mapper.Get());

    if(this->TexturedBackground && this->BackgroundTexture)
      {
      this->BackgroundTexture->InterpolateOn();
      actor->SetTexture(this->BackgroundTexture);

      vtkNew<vtkFloatArray> tcoords;
      float tmp[2];
      tmp[0] = 0;
      tmp[1] = 0;
      tcoords->SetNumberOfComponents(2);
      tcoords->SetNumberOfTuples(4);
      tcoords->SetTuple(0,tmp);
      tmp[0] = 1.0;
      tcoords->SetTuple(1,tmp);
      tmp[1] = 1.0;
      tcoords->SetTuple(2,tmp);
      tmp[0] = 0.0;
      tcoords->SetTuple(3,tmp);
      polydata->GetPointData()->SetTCoords(tcoords.Get());
      }
    else // gradient
      {
      vtkNew<vtkUnsignedCharArray> colors;
      float tmp[4];
      tmp[0] = this->Background[0]*255;
      tmp[1] = this->Background[1]*255;
      tmp[2] = this->Background[2]*255;
      tmp[3] = 255;
      colors->SetNumberOfComponents(4);
      colors->SetNumberOfTuples(4);
      colors->SetTuple(0,tmp);
      colors->SetTuple(1,tmp);
      tmp[0] = this->Background2[0]*255;
      tmp[1] = this->Background2[1]*255;
      tmp[2] = this->Background2[2]*255;
      colors->SetTuple(2,tmp);
      colors->SetTuple(3,tmp);
      polydata->GetPointData()->SetScalars(colors.Get());
      }

    glDisable(GL_DEPTH_TEST);
    actor->RenderOverlay(this);
    }

  glEnable(GL_DEPTH_TEST);

  vtkOpenGLCheckErrorMacro("failed after Clear");
}

void vtkOpenGLRenderer::StartPick(unsigned int vtkNotUsed(pickFromSize))
{
  vtkOpenGLClearErrorMacro();

  /*
  int size[2];
  size[0] = this->GetSize()[0];
  size[1] = this->GetSize()[1];

  // Create the FBO
  glGenFramebuffers(1, &this->PickInfo->PickingFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, this->PickInfo->PickingFBO);

  // Create the texture object for the primitive information buffer
  glGenTextures(1, &this->PickInfo->PickingTexture);
  glBindTexture(GL_TEXTURE_2D, this->PickInfo->PickingTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, size[0], size[1],
              0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
              this->PickInfo->PickingTexture, 0);

  // Create the texture object for the depth buffer
  glGenTextures(1, &this->PickInfo->DepthTexture);
  glBindTexture(GL_TEXTURE_2D, this->PickInfo->DepthTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size[0], size[1],
              0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
              this->PickInfo->DepthTexture, 0);

  // Disable reading to avoid problems with older GPUs
  glReadBuffer(GL_NONE);

  // Verify that the FBO is correct
  GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

  if (Status != GL_FRAMEBUFFER_COMPLETE)
    {
    printf("FB error, status: 0x%x\n", Status);
    return;
    }

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->PickInfo->PickingFBO);
  */

  // Do not remove this MakeCurrent! Due to Start / End methods on
  // some objects which get executed during a pipeline update,
  // other windows might get rendered since the last time
  // a MakeCurrent was called.
  this->RenderWindow->MakeCurrent();
  this->RenderWindow->IsPickingOn();
  this->IsPicking = 1;
  this->PickInfo->PerformedHardwarePick = false;
  this->PickInfo->PickValues.clear();
  this->PickInfo->NumPicked = 0;
  this->PickInfo->PickedId = 0;

  this->Clear();

  vtkOpenGLCheckErrorMacro("failed after StartPick");
}

void vtkOpenGLRenderer::ReleaseGraphicsResources(vtkWindow *w)
{
  if (w && this->Pass)
    {
    this->Pass->ReleaseGraphicsResources(w);
    }
}

void vtkOpenGLRenderer::UpdatePickId()
{
  this->CurrentPickId++;
}


void vtkOpenGLRenderer::DevicePickRender()
{
  // Do not remove this MakeCurrent! Due to Start / End methods on
  // some objects which get executed during a pipeline update,
  // other windows might get rendered since the last time
  // a MakeCurrent was called.
  this->RenderWindow->MakeCurrent();
  vtkOpenGLClearErrorMacro();

  this->UpdateCamera();
  this->UpdateLightGeometry();
  this->UpdateLights();

  this->PickGeometry();

  this->PickInfo->PerformedHardwarePick = true;

  vtkOpenGLCheckErrorMacro("failed after DevicePickRender");
}


void vtkOpenGLRenderer::DonePick()
{
  if (this->PickInfo->PerformedHardwarePick)
    {
    glFlush();

    unsigned char *pixBuffer = this->GetRenderWindow()->GetPixelData(
      this->PickX1, this->PickY1, this->PickX2, this->PickY2, 0);
  //    (this->GetRenderWindow()->GetSwapBuffers() == 1) ? 0 : 1);

    // for debugging save out the image
    // FILE * pFile;
    // pFile = fopen ("myfile.ppm", "wb");
    // fwrite (pixBuffer , sizeof(unsigned char), 3*((int)this->PickY2-(int)this->PickY1+1)*((int)this->PickX2-(int)this->PickX1+1), pFile);
    // fclose (pFile);

    float *depthBuffer = this->GetRenderWindow()->GetZbufferData(
      this->PickX1, this->PickY1, this->PickX2, this->PickY2);

    // read the color and z buffer values for the region
    // to see what hits we have
    this->PickInfo->PickValues.clear();
    unsigned char *pb = pixBuffer;
    float *dbPtr = depthBuffer;
    for (int y = this->PickY1; y <= this->PickY2; y++)
      {
      for (int x = this->PickX1; x <= this->PickX2; x++)
        {
        unsigned char rgb[3];
        rgb[0] = *pb++;
        rgb[1] = *pb++;
        rgb[2] = *pb++;
        int val = 0;
        val |= rgb[2];
        val = val << 8;
        val |= rgb[1];
        val = val << 8;
        val |= rgb[0];
        if (val > 0)
          {
          if (this->PickInfo->PickValues.find(val) == this->PickInfo->PickValues.end())
            {
            this->PickInfo->PickValues.insert(std::pair<unsigned int,float>(val,*dbPtr));
            }
          }
        dbPtr++;
        }
      }

    this->PickInfo->NumPicked = (unsigned int)this->PickInfo->PickValues.size();

    this->PickInfo->PickedId = 0;
    std::map<unsigned int,float>::const_iterator dvItr =
      this->PickInfo->PickValues.begin();
    this->PickedZ = 1.0;
    for ( ; dvItr != this->PickInfo->PickValues.end(); dvItr++)
      {
      if(dvItr->second < this->PickedZ)
        {
        this->PickedZ = dvItr->second;
        this->PickInfo->PickedId = dvItr->first - 1;
        }
      }
    }

  // Restore the default framebuffer
  //glBindFramebuffer(GL_FRAMEBUFFER, 0);

  this->RenderWindow->IsPickingOff();
  this->IsPicking = 0;
}

double vtkOpenGLRenderer::GetPickedZ()
{
  return this->PickedZ;
}

unsigned int vtkOpenGLRenderer::GetPickedId()
{
  return static_cast<unsigned int>(this->PickInfo->PickedId);
}

vtkOpenGLRenderer::~vtkOpenGLRenderer()
{
  delete this->PickInfo;

  if(this->Pass != NULL)
    {
    this->Pass->UnRegister(this);
    this->Pass = NULL;
    }

  if (this->DepthZData)
    {
    delete this->DepthZData;
    this->DepthZData = 0;
    }
  if (this->OpaqueZTexture)
    {
    this->OpaqueZTexture->UnRegister(this);
    this->OpaqueZTexture = NULL;
    }
  if (this->TranslucentZTexture)
    {
    this->TranslucentZTexture->UnRegister(this);
    this->TranslucentZTexture = NULL;
    }
  if (this->OpaqueRGBATexture)
    {
    this->OpaqueRGBATexture->UnRegister(this);
    this->OpaqueRGBATexture = NULL;
    }
  if (this->TranslucentRGBATexture)
    {
    this->TranslucentRGBATexture->UnRegister(this);
    this->TranslucentRGBATexture = NULL;
    }
  if (this->CurrentRGBATexture)
    {
    this->CurrentRGBATexture->UnRegister(this);
    this->CurrentRGBATexture = NULL;
    }
  if (this->DepthPeelingActor)
    {
    this->DepthPeelingActor->UnRegister(this);
    this->DepthPeelingActor = NULL;
    }
}

unsigned int vtkOpenGLRenderer::GetNumPickedIds()
{
  return static_cast<unsigned int>(this->PickInfo->NumPicked);
}

int vtkOpenGLRenderer::GetPickedIds(unsigned int atMost,
                                    unsigned int *callerBuffer)
{
  if (this->PickInfo->PickValues.empty())
    {
    return 0;
    }

  unsigned int max = (atMost < this->PickInfo->NumPicked) ? atMost : this->PickInfo->NumPicked;

  unsigned int k = 0;
  unsigned int *optr = callerBuffer;
  std::map<unsigned int,float>::const_iterator dvItr =
    this->PickInfo->PickValues.begin();
  this->PickedZ = 1.0;
  for ( ; dvItr != this->PickInfo->PickValues.end() && k < max; dvItr++)
    {
    *optr = static_cast<unsigned int>(dvItr->first);
    optr++;
    }
  return k;
}
