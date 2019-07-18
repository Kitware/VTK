/*=========================================================================

  Prograxq:   Visualization Toolkit
  Module:    vtkOSPRayPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtk_glew.h>

#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkLightsPass.h"
#include "vtkObjectFactory.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOSPRayViewNodeFactory.h"
#include "vtkOverlayPass.h"
#include "vtkOpaquePass.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkSequencePass.h"
#include "vtkVolumetricPass.h"

#include "RTWrapper/RTWrapper.h"

class vtkOSPRayPassInternals : public vtkRenderPass
{
public:
  static vtkOSPRayPassInternals *New();
  vtkTypeMacro(vtkOSPRayPassInternals,vtkRenderPass);
  vtkOSPRayPassInternals()
  {
    this->Factory = 0;
  }
  ~vtkOSPRayPassInternals()
  {
    this->Factory->Delete();
  }
  void Render(const vtkRenderState *s) override
  {
    this->Parent->RenderInternal(s);
  }

  vtkOSPRayViewNodeFactory *Factory;
  vtkOSPRayPass *Parent;

  // OpenGL-based display
  GLuint fullscreenQuadProgram = 0;
  GLuint fullscreenColorTextureLocation = 0;
  GLuint fullscreenDepthTextureLocation = 0;
  GLuint fullscreenVAO = 0;
};

int vtkOSPRayPass::RTDeviceRefCount = 0;

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayPassInternals);

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayPass);

// ----------------------------------------------------------------------------
vtkOSPRayPass::vtkOSPRayPass()
{
  this->SceneGraph = nullptr;

  vtkOSPRayPass::RTInit();

  vtkOSPRayViewNodeFactory *vnf = vtkOSPRayViewNodeFactory::New();
  this->Internal = vtkOSPRayPassInternals::New();
  this->Internal->Factory = vnf;
  this->Internal->Parent = this;

  this->CameraPass = vtkCameraPass::New();
  this->LightsPass = vtkLightsPass::New();
  this->SequencePass = vtkSequencePass::New();
  this->VolumetricPass = vtkVolumetricPass::New();
  this->OverlayPass = vtkOverlayPass::New();

  this->RenderPassCollection = vtkRenderPassCollection::New();
  this->RenderPassCollection->AddItem(this->LightsPass);
  this->RenderPassCollection->AddItem(this->Internal);
  this->RenderPassCollection->AddItem(this->OverlayPass);

  this->SequencePass->SetPasses(this->RenderPassCollection);
  this->CameraPass->SetDelegatePass(this->SequencePass);

  this->PreviousType = "none";
}

// ----------------------------------------------------------------------------
vtkOSPRayPass::~vtkOSPRayPass()
{
  this->SetSceneGraph(nullptr);
  this->Internal->Delete();
  this->Internal = 0;
  if (this->CameraPass)
  {
    this->CameraPass->Delete();
    this->CameraPass = 0;
  }
  if (this->LightsPass)
  {
    this->LightsPass->Delete();
    this->LightsPass = 0;
  }
  if (this->SequencePass)
  {
    this->SequencePass->Delete();
    this->SequencePass = 0;
  }
  if (this->VolumetricPass)
  {
    this->VolumetricPass->Delete();
    this->VolumetricPass = 0;
  }
  if (this->OverlayPass)
  {
    this->OverlayPass->Delete();
    this->OverlayPass = 0;
  }
  if (this->RenderPassCollection)
  {
    this->RenderPassCollection->Delete();
    this->RenderPassCollection = 0;
  }
  vtkOSPRayPass::RTShutdown();
}

// ----------------------------------------------------------------------------
void vtkOSPRayPass::RTInit()
{
  if (RTDeviceRefCount == 0)
  {
    rtwInit();
  }
  RTDeviceRefCount++;
}

// ----------------------------------------------------------------------------
void vtkOSPRayPass::RTShutdown()
{
  --RTDeviceRefCount;
  if (RTDeviceRefCount == 0)
  {
    rtwShutdown();
  }
}

// ----------------------------------------------------------------------------
void vtkOSPRayPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkOSPRayPass, SceneGraph, vtkOSPRayRendererNode)

// ----------------------------------------------------------------------------
void vtkOSPRayPass::Render(const vtkRenderState *s)
{
  vtkRenderer *ren = s->GetRenderer();
  if (ren)
  {
    std::string type = vtkOSPRayRendererNode::GetRendererType(ren);
    if (this->PreviousType != type && this->SceneGraph)
    {
      this->SceneGraph->Delete();
      this->SceneGraph = nullptr;
    }
    if (!this->SceneGraph)
    {
      this->SceneGraph = vtkOSPRayRendererNode::SafeDownCast
        (this->Internal->Factory->CreateNode(ren));
    }
    this->PreviousType = type;
  }

  this->CameraPass->Render(s);
}

// ----------------------------------------------------------------------------
void vtkOSPRayPass::RenderInternal(const vtkRenderState *s)
{
  this->NumberOfRenderedProps=0;

  if (this->SceneGraph)
  {

    this->SceneGraph->TraverseAllPasses();

    vtkRenderer *ren = s->GetRenderer();
    vtkOSPRayRendererNode* oren = vtkOSPRayRendererNode::SafeDownCast(this->SceneGraph->GetViewNodeFor(ren));
    if (oren->GetBackend() == nullptr)
        return;
    // copy the result to the window

    vtkRenderWindow *rwin =
      vtkRenderWindow::SafeDownCast(ren->GetVTKWindow());
    int viewportX, viewportY;
    int viewportWidth, viewportHeight;
    int right = 0;
    if (rwin)
    {
      if (rwin->GetStereoRender() == 1)
      {
        if (rwin->GetStereoType() == VTK_STEREO_CRYSTAL_EYES)
        {
          vtkCamera *camera = ren->GetActiveCamera();
          if (camera)
          {
            if (!camera->GetLeftEye())
            {
              right = 1;
            }
          }
        }
      }
    }
    ren->GetTiledSizeAndOrigin(&viewportWidth,&viewportHeight, &viewportX,&viewportY);


    int layer = ren->GetLayer();
    if (layer == 0)
    {
        const int colorTexGL = this->SceneGraph->GetColorBufferTextureGL();
        const int depthTexGL = this->SceneGraph->GetDepthBufferTextureGL();

        vtkOpenGLRenderWindow* windowOpenGL = vtkOpenGLRenderWindow::SafeDownCast(rwin);

        if (colorTexGL != 0 && depthTexGL != 0 && windowOpenGL != nullptr)
        {
            windowOpenGL->MakeCurrent();

            // Init OpenGL display resources
            if (!this->Internal->fullscreenVAO)
                glGenVertexArrays(1, &this->Internal->fullscreenVAO);

            if (!this->Internal->fullscreenQuadProgram)
            {
                const GLchar* vertexShader =
                    "#version 330\n"
                    "void main() {}";

                const GLchar* geometryShader =
                    "#version 330 core\n"
                    "layout(points) in;"
                    "layout(triangle_strip, max_vertices = 4) out;"
                    "out vec2 texcoord;"
                    "void main() {"
                    "gl_Position = vec4( 1.0, 1.0, 0.0, 1.0 ); texcoord = vec2( 1.0, 1.0 ); EmitVertex();"
                    "gl_Position = vec4(-1.0, 1.0, 0.0, 1.0 ); texcoord = vec2( 0.0, 1.0 ); EmitVertex();"
                    "gl_Position = vec4( 1.0,-1.0, 0.0, 1.0 ); texcoord = vec2( 1.0, 0.0 ); EmitVertex();"
                    "gl_Position = vec4(-1.0,-1.0, 0.0, 1.0 ); texcoord = vec2( 0.0, 0.0 ); EmitVertex();"
                    "EndPrimitive();"
                    "}";

                const GLchar* fragmentShader =
                    "#version 330\n"
                    "uniform sampler2D colorTexture;"
                    "uniform sampler2D depthTexture;"
                    "in vec2 texcoord;"
                    "out vec4 color;"
                    "void main() {"
                    "	color = texture(colorTexture, texcoord);"
                    "   gl_FragDepth = texture(depthTexture, texcoord).r;"
                    "}";

                GLuint vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
                glShaderSource(vertexShaderHandle, 1, &vertexShader, 0);
                glCompileShader(vertexShaderHandle);

                GLuint geometryShaderHandle = glCreateShader(GL_GEOMETRY_SHADER);
                glShaderSource(geometryShaderHandle, 1, &geometryShader, 0);
                glCompileShader(geometryShaderHandle);

                GLuint fragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
                glShaderSource(fragmentShaderHandle, 1, &fragmentShader, 0);
                glCompileShader(fragmentShaderHandle);

                this->Internal->fullscreenQuadProgram = glCreateProgram();
                glAttachShader(this->Internal->fullscreenQuadProgram, vertexShaderHandle);
                glAttachShader(this->Internal->fullscreenQuadProgram, geometryShaderHandle);
                glAttachShader(this->Internal->fullscreenQuadProgram, fragmentShaderHandle);
                glLinkProgram(this->Internal->fullscreenQuadProgram);

                this->Internal->fullscreenColorTextureLocation = glGetUniformLocation(this->Internal->fullscreenQuadProgram, "colorTexture");
                this->Internal->fullscreenDepthTextureLocation = glGetUniformLocation(this->Internal->fullscreenQuadProgram, "depthTexture");
            }

            // Set viewport
            glViewport(viewportX, viewportY, viewportWidth, viewportHeight);

            // Save previous program
            int program;
            glGetIntegerv(GL_CURRENT_PROGRAM, &program);

            // Display texture
            glUseProgram(this->Internal->fullscreenQuadProgram);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, colorTexGL);
            glUniform1i(this->Internal->fullscreenColorTextureLocation, 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, depthTexGL);
            glUniform1i(this->Internal->fullscreenDepthTextureLocation, 1);

            glBindVertexArray(this->Internal->fullscreenVAO);
            glDrawArrays(GL_POINTS, 0, 1);

            // Restore state
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glUseProgram(program);
            glBindVertexArray(0);
        }
        else
        {
            rwin->SetZbufferData(
                viewportX, viewportY,
                viewportX + viewportWidth - 1,
                viewportY + viewportHeight - 1,
                this->SceneGraph->GetZBuffer());
            rwin->SetRGBACharPixelData(
                viewportX, viewportY,
                viewportX + viewportWidth - 1,
                viewportY + viewportHeight - 1,
                this->SceneGraph->GetBuffer(),
                0, vtkOSPRayRendererNode::GetCompositeOnGL(ren), right);
        }
    }
    else
    {
      float *ontoZ = rwin->GetZbufferData
        (viewportX,  viewportY,
         viewportX+viewportWidth-1,
         viewportY+viewportHeight-1);
      unsigned char *ontoRGBA = rwin->GetRGBACharPixelData
        (viewportX,  viewportY,
         viewportX+viewportWidth-1,
         viewportY+viewportHeight-1,
         0, right);
      oren->WriteLayer(ontoRGBA, ontoZ, viewportWidth, viewportHeight, layer);
      rwin->SetZbufferData(
         viewportX,  viewportY,
         viewportX+viewportWidth-1,
         viewportY+viewportHeight-1,
         ontoZ);
      rwin->SetRGBACharPixelData(
         viewportX,  viewportY,
         viewportX+viewportWidth-1,
         viewportY+viewportHeight-1,
         ontoRGBA,
         0, vtkOSPRayRendererNode::GetCompositeOnGL(ren), right);
      delete[] ontoZ;
      delete[] ontoRGBA;
    }
  }
}

// ----------------------------------------------------------------------------
bool vtkOSPRayPass::IsBackendAvailable(const char *choice)
{
  std::set<RTWBackendType> bends = rtwGetAvailableBackends();
  if (!strcmp(choice, "OSPRay raycaster"))
  {
    return (bends.find(RTW_BACKEND_OSPRAY) != bends.end());
  }
  if (!strcmp(choice, "OSPRay pathtracer"))
  {
    return (bends.find(RTW_BACKEND_OSPRAY) != bends.end());
  }
  if (!strcmp(choice, "OptiX pathtracer"))
  {
    return (bends.find(RTW_BACKEND_VISRTX) != bends.end());
  }
  return false;
}
