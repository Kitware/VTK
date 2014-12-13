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

#include "vtkglVBOHelper.h"

#include "vtkCellArray.h"
#include "vtkDepthPeelingPass.h"
#include "vtkFloatArray.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkRenderPass.h"
#include "vtkRenderState.h"
#include "vtkTexture.h"
#include "vtkTextureObject.h"
#include "vtkTexturedActor2D.h"
#include "vtkTimerLog.h"
#include "vtkTranslucentPass.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedCharArray.h"

#include <math.h>
#include <cassert>
#include <list>

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

  this->DepthPeelingPass = 0;
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
    this->LastRenderingUsedDepthPeeling=0;
    // Do not remove this MakeCurrent! Due to Start / End methods on
    // some objects which get executed during a pipeline update,
    // other windows might get rendered since the last time
    // a MakeCurrent was called.
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

  if(this->UseDepthPeeling && !context)
    {
    vtkErrorMacro("OpenGL render window is required.")
    return;
    }

  if(!this->UseDepthPeeling)
    {
    // just alpha blending
    this->LastRenderingUsedDepthPeeling=0;
    this->UpdateTranslucentPolygonalGeometry();
    }
  else   // depth peeling.
    {
    if (!this->DepthPeelingPass)
      {
      this->DepthPeelingPass = vtkDepthPeelingPass::New();
      vtkTranslucentPass *tp = vtkTranslucentPass::New();
      this->DepthPeelingPass->SetTranslucentPass(tp);
      tp->Delete();
      }
    this->DepthPeelingPass->SetMaximumNumberOfPeels(this->MaximumNumberOfPeels);
    this->DepthPeelingPass->SetOcclusionRatio(this->OcclusionRatio);
    vtkRenderState s(this);
    s.SetPropArrayAndCount(this->PropArray, this->PropArrayCount);
    s.SetFrameBuffer(0);
    this->DepthPeelingPass->Render(&s);
    }

  vtkOpenGLCheckErrorMacro("failed after DeviceRenderTranslucentPolygonalGeometry");
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
    glDepthMask(GL_TRUE);
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
  if (w && this->DepthPeelingPass)
    {
    this->DepthPeelingPass->ReleaseGraphicsResources(w);
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

  if (this->DepthPeelingPass)
    {
    this->DepthPeelingPass->Delete();
    this->DepthPeelingPass = 0;
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
