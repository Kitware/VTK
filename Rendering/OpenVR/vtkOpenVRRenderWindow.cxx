/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRRenderWindow.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

Parts Copyright Valve Coproration from hellovr_opengl_main.cpp
under their BSD license found here:
https://github.com/ValveSoftware/openvr/blob/master/LICENSE

=========================================================================*/
#include "vtkOpenVRRenderWindow.h"

#include "vtkIdList.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkRendererCollection.h"
#include "vtkStringOutputWindow.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkTransform.h"
#include "vtkFloatArray.h"
#include "vtkPointData.h"
#include "vtkOpenGLTexture.h"
#include "vtkTextureObject.h"
#include "vtkShaderProgram.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkOpenVRCamera.h"

#include <cmath>
#include <sstream>

#include "vtkOpenGLError.h"


// this internal class is used to load models
// such as for the trackers and controllers and to
// render them in the scene
class vtkOpenVRModel : public vtkObject
{
public:
  static vtkOpenVRModel *New();
  vtkTypeMacro(vtkOpenVRModel,vtkObject);

  bool Build(vtkOpenVRRenderWindow *win);
  void Render(vtkOpenVRRenderWindow *win,
    const vr::TrackedDevicePose_t &pose);

  const std::string & GetName() const { return this->ModelName; }
  void SetName( const std::string & modelName ) { this->ModelName = modelName;};

  // show the model
  void SetShow(bool v) { this->Show = v;};
  bool GetShow() { return this->Show;};


  void ReleaseGraphicsResources(vtkRenderWindow *win);

  vr::RenderModel_t *RawModel;

protected:
  vtkOpenVRModel();
  ~vtkOpenVRModel();

  std::string ModelName;

  bool Show;
  bool Loaded;

  vr::RenderModel_TextureMap_t *RawTexture;
  vtkOpenGLHelper ModelHelper;
  vtkOpenGLVertexBufferObject *ModelVBO;
  vtkNew<vtkTextureObject> TextureObject;
  vtkNew<vtkMatrix4x4> PoseMatrix;
};

vtkStandardNewMacro(vtkOpenVRModel);

vtkOpenVRModel::vtkOpenVRModel()
{
  this->RawModel = NULL;
  this->RawTexture = NULL;
  this->Loaded = false;
  this->ModelVBO = vtkOpenGLVertexBufferObject::New();
};

vtkOpenVRModel::~vtkOpenVRModel()
{
  this->ModelVBO->Delete();
  this->ModelVBO = 0;
}

void vtkOpenVRModel::ReleaseGraphicsResources(vtkRenderWindow *win)
{
  this->ModelVBO->ReleaseGraphicsResources();
  this->ModelHelper.ReleaseGraphicsResources(win);
  this->TextureObject->ReleaseGraphicsResources(win);
}

bool vtkOpenVRModel::Build(vtkOpenVRRenderWindow *win)
{
  this->ModelVBO->Upload(
    this->RawModel->rVertexData,
    this->RawModel->unVertexCount,
    vtkOpenGLBufferObject::ArrayBuffer);

  this->ModelHelper.IBO->Upload(
    this->RawModel->rIndexData,
    this->RawModel->unTriangleCount * 3,
    vtkOpenGLBufferObject::ElementArrayBuffer);
  this->ModelHelper.IBO->IndexCount = this->RawModel->unTriangleCount * 3;

  this->ModelHelper.Program = win->GetShaderCache()->ReadyShaderProgram(

    // vertex shader -- use normals?? yes?
    "//VTK::System::Dec\n"
    "uniform mat4 matrix;\n"
    "attribute vec4 position;\n"
//    "attribute vec3 v3NormalIn;\n"
    "attribute vec2 v2TexCoordsIn;\n"
    "out vec2 v2TexCoord;\n"
    "void main()\n"
    "{\n"
    " v2TexCoord = v2TexCoordsIn;\n"
    " gl_Position = matrix * vec4(position.xyz, 1);\n"
    "}\n",

    //fragment shader
    "//VTK::System::Dec\n"
    "//VTK::Output::Dec\n"
    "uniform sampler2D diffuse;\n"
    "in vec2 v2TexCoord;\n"
    "out vec4 outputColor;\n"
    "void main()\n"
    "{\n"
    "   gl_FragData[0] = texture( diffuse, v2TexCoord);\n"
    "}\n",

    // geom shader
    ""
    );

  vtkShaderProgram *program = this->ModelHelper.Program;
  this->ModelHelper.VAO->Bind();
  if (!this->ModelHelper.VAO->AddAttributeArray(program,
        this->ModelVBO,
        "position",
        offsetof( vr::RenderModel_Vertex_t, vPosition ),
        sizeof(vr::RenderModel_Vertex_t),
        VTK_FLOAT, 3, false))
  {
      vtkErrorMacro(<< "Error setting position in shader VAO.");
  }
  if (!this->ModelHelper.VAO->AddAttributeArray(program,
        this->ModelVBO,
        "v2TexCoordsIn",
        offsetof( vr::RenderModel_Vertex_t, rfTextureCoord ),
        sizeof(vr::RenderModel_Vertex_t),
        VTK_FLOAT, 2, false))
  {
      vtkErrorMacro(<< "Error setting tcoords in shader VAO.");
  }

  // create and populate the texture
  this->TextureObject->SetContext(win);
  this->TextureObject->Create2DFromRaw(
    this->RawTexture->unWidth, this->RawTexture->unHeight,
    4,  VTK_UNSIGNED_CHAR,
    const_cast<void *>(static_cast<const void *const>(
      this->RawTexture->rubTextureMapData)));
  this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
  this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);

  this->TextureObject->SetMinificationFilter(vtkTextureObject::LinearMipmapLinear);
  this->TextureObject->SetGenerateMipmap(true);

  return true;
}

void vtkOpenVRModel::Render(
  vtkOpenVRRenderWindow *win,
  const vr::TrackedDevicePose_t &pose)
{
  // do we not have the model loaded? Keep trying it is async
  if (!this->RawModel)
  {
      // start loading the model if we didn't find one
    if (vr::VRRenderModels()->LoadRenderModel_Async(
        this->GetName().c_str(), &this->RawModel)
        > vr::EVRRenderModelError::VRRenderModelError_Loading)
    {
      vtkErrorMacro("Unable to load render model " << this->GetName() );
      return; // move on to the next tracked device
    }
  }

  // if we have the model try loading the texture
  if (this->RawModel && !this->RawTexture)
  {
    if (vr::VRRenderModels( )->LoadTexture_Async(
        this->RawModel->diffuseTextureId, &this->RawTexture )
        > vr::EVRRenderModelError::VRRenderModelError_Loading)
    {
      vtkErrorMacro(<< "Unable to load render texture for render model "
        << this->GetName() );
    }

    // if this call succeeded and we have the texture
    // then build the VTK structures
    if (this->RawTexture)
    {
      if ( !this->Build(win) )
      {
        vtkErrorMacro( "Unable to create GL model from render model " << this->GetName() );
      }
      vr::VRRenderModels()->FreeRenderModel( this->RawModel );
      vr::VRRenderModels()->FreeTexture( this->RawTexture );
      this->Loaded = true;
    }
  }

  if (this->Loaded)
  {
    // render the model
    win->GetShaderCache()->ReadyShaderProgram(this->ModelHelper.Program);
    this->ModelHelper.VAO->Bind();
    this->ModelHelper.IBO->Bind();

    this->TextureObject->Activate();
    this->ModelHelper.Program->SetUniformi("diffuse",
      this->TextureObject->GetTextureUnit());

    vtkRenderer *ren;
    vtkCollectionSimpleIterator rit;
    win->GetRenderers()->InitTraversal(rit);
    ren = win->GetRenderers()->GetNextRenderer(rit);
    if (ren)
    {
      vtkOpenVRCamera *cam =
        static_cast<vtkOpenVRCamera *>(ren->GetActiveCamera());

      double elems[16];
      for (int j = 0; j < 3; j++)
      {
        for (int i = 0; i < 4; i++)
        {
          elems[j+i*4] = pose.mDeviceToAbsoluteTracking.m[j][i];
        }
      }
      elems[3] = 0.0;
      elems[7] = 0.0;
      elems[11] = 0.0;
      elems[15] = 1.0;

      vtkMatrix4x4* tcdc;
      cam->GetTrackingToDCMatrix(tcdc);

      vtkMatrix4x4::Multiply4x4(
        elems,
        (double *)(tcdc->Element),
        (double *)(this->PoseMatrix->Element));

      this->ModelHelper.Program->SetUniformMatrix("matrix", this->PoseMatrix.Get());
    }

    glDrawElements( GL_TRIANGLES,
      static_cast<GLsizei>(this->ModelHelper.IBO->IndexCount),
      GL_UNSIGNED_SHORT, 0 );
    this->TextureObject->Deactivate();
  }
}

//=========================================================
//
// Here starts the main vtkOpenVRRenderWindow class
//
vtkStandardNewMacro(vtkOpenVRRenderWindow);

vtkOpenVRRenderWindow::vtkOpenVRRenderWindow()
{
  this->StereoCapableWindow = 1;
  this->StereoRender = 1;
  this->Size[0] = 640; // we double this for two eyes
  this->Size[1] = 720;
  this->Position[0] = 700;
  this->Position[1] = 100;
  this->OpenVRRenderModels = NULL;
  this->HMD = NULL;
  this->DistortionVBO = vtkOpenGLVertexBufferObject::New();
  this->HMDTransform = vtkTransform::New();
  this->ContextId = 0;
  this->WindowId = 0;
  memset(this->TrackedDeviceToRenderModel, 0, sizeof(this->TrackedDeviceToRenderModel));
}

vtkOpenVRRenderWindow::~vtkOpenVRRenderWindow()
{
  this->Finalize();

  vtkRenderer *ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ( (ren = this->Renderers->GetNextRenderer(rit)) )
  {
    ren->SetRenderWindow(NULL);
  }
  this->DistortionVBO->Delete();
  this->DistortionVBO = 0;
  this->HMDTransform->Delete();
  this->HMDTransform = 0;
}

// ----------------------------------------------------------------------------
void vtkOpenVRRenderWindow::ReleaseGraphicsResources(vtkRenderWindow *renWin)
{
  this->Superclass::ReleaseGraphicsResources(renWin);
  this->DistortionVBO->ReleaseGraphicsResources();
  this->Distortion.ReleaseGraphicsResources(renWin);
  for( std::vector< vtkOpenVRModel * >::iterator i = this->VTKRenderModels.begin();
       i != this->VTKRenderModels.end(); i++ )
  {
    (*i)->ReleaseGraphicsResources(renWin);
  }
}

// Purpose: Helper to get a string from a tracked device property and turn it
//      into a std::string
std::string vtkOpenVRRenderWindow::GetTrackedDeviceString(
  vr::IVRSystem *pHmd,
  vr::TrackedDeviceIndex_t unDevice,
  vr::TrackedDeviceProperty prop,
  vr::TrackedPropertyError *peError)
{
  uint32_t unRequiredBufferLen =
    pHmd->GetStringTrackedDeviceProperty( unDevice, prop, NULL, 0, peError );
  if( unRequiredBufferLen == 0 )
  {
    return "";
  }

  char *pchBuffer = new char[ unRequiredBufferLen ];
  unRequiredBufferLen =
    pHmd->GetStringTrackedDeviceProperty( unDevice, prop, pchBuffer, unRequiredBufferLen, peError );
  std::string sResult = pchBuffer;
  delete [] pchBuffer;
  return sResult;
}

// Purpose: Finds a render model we've already loaded or loads a new one
vtkOpenVRModel *vtkOpenVRRenderWindow::FindOrLoadRenderModel(
  const char *pchRenderModelName )
{
  vtkOpenVRModel *pRenderModel = NULL;
  for( std::vector< vtkOpenVRModel * >::iterator i = this->VTKRenderModels.begin();
       i != this->VTKRenderModels.end(); i++ )
  {
    if( !stricmp( (*i)->GetName().c_str(), pchRenderModelName ) )
    {
      pRenderModel = *i;
      return pRenderModel;
    }
  }

  // create the model
  pRenderModel = vtkOpenVRModel::New();
  pRenderModel->SetName(pchRenderModelName);

  // start loading the model if we didn't find one
  if (vr::VRRenderModels()->LoadRenderModel_Async(
      pRenderModel->GetName().c_str(), &pRenderModel->RawModel)
      > vr::EVRRenderModelError::VRRenderModelError_Loading)
  {
    vtkErrorMacro("Unable to load render model " << pRenderModel->GetName() );
    pRenderModel->Delete();
    return NULL; // move on to the next tracked device
  }

  pRenderModel->SetShow(true);
  this->VTKRenderModels.push_back( pRenderModel );

  return pRenderModel;
}

void vtkOpenVRRenderWindow::RenderModels()
{
  bool bIsInputCapturedByAnotherProcess =
    this->HMD->IsInputFocusCapturedByAnotherProcess();

  // for each device
  for (uint32_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1;
       unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++ )
  {
    // is it not connected?
    if (!this->HMD->IsTrackedDeviceConnected( unTrackedDevice ) )
    {
      continue;
    }
    // do we not have amodel loaded yet? try loading one
    if (!this->TrackedDeviceToRenderModel[ unTrackedDevice ])
    {
      std::string sRenderModelName =
        this->GetTrackedDeviceString(this->HMD, unTrackedDevice, vr::Prop_RenderModelName_String );
      vtkOpenVRModel *pRenderModel = this->FindOrLoadRenderModel( sRenderModelName.c_str() );
      if( pRenderModel )
      {
        this->TrackedDeviceToRenderModel[ unTrackedDevice ] = pRenderModel;
      }
    }
    // if we still have no model or it is not set to show
    if( !this->TrackedDeviceToRenderModel[ unTrackedDevice ]
      || !this->TrackedDeviceToRenderModel[ unTrackedDevice ]->GetShow())
    {
      continue;
    }
    // is the model's pose not valid?
    const vr::TrackedDevicePose_t &pose = this->TrackedDevicePose[ unTrackedDevice ];
    if( !pose.bPoseIsValid )
    {
      continue;
    }

    if( bIsInputCapturedByAnotherProcess &&
        this->HMD->GetTrackedDeviceClass( unTrackedDevice ) == vr::TrackedDeviceClass_Controller )
    {
      continue;
    }

    this->TrackedDeviceToRenderModel[ unTrackedDevice ]->Render(this, pose);
  }
}

void vtkOpenVRRenderWindow::Clean()
{
  /* finish OpenGL rendering */
  if (this->OwnContext && this->ContextId)
  {
    this->MakeCurrent();
    this->ReleaseGraphicsResources(this);
  }

  this->ContextId = NULL;
}

// ----------------------------------------------------------------------------
void vtkOpenVRRenderWindow::MakeCurrent()
{
  SDL_GL_MakeCurrent(this->WindowId, this->ContextId);
}

// ----------------------------------------------------------------------------
// Description:
// Tells if this window is the current OpenGL context for the calling thread.
bool vtkOpenVRRenderWindow::IsCurrent()
{
  return this->ContextId!=0 && this->ContextId==SDL_GL_GetCurrentContext();
}


// ----------------------------------------------------------------------------
void vtkOpenVRRenderWindow::SetSize(int x, int y)
{
  static int resizing = 0;
  if ((this->Size[0] != x) || (this->Size[1] != y))
  {
    this->Superclass::SetSize(x, y);

    if (this->Interactor)
    {
      this->Interactor->SetSize(x, y);
    }

    if (this->Mapped)
    {
      if (!resizing)
      {
        resizing = 1;
        SDL_SetWindowSize(this->WindowId, this->Size[0], this->Size[1]);
        resizing = 0;
      }
    }
  }
}

// Get the size of the whole screen.
int *vtkOpenVRRenderWindow::GetScreenSize(void)
{
  return this->Size;
}


void vtkOpenVRRenderWindow::SetPosition(int x, int y)
{
  static int resizing = 0;

  if ((this->Position[0] != x) || (this->Position[1] != y))
  {
    this->Modified();
    this->Position[0] = x;
    this->Position[1] = y;
    if (this->Mapped)
    {
      if (!resizing)
      {
        resizing = 1;
        SDL_SetWindowPosition(this->WindowId,x,y);
        resizing = 0;
      }
    }
  }
}

void vtkOpenVRRenderWindow::UpdateHMDMatrixPose()
{
  if (!this->HMD)
  {
    return;
  }
  vr::VRCompositor()->WaitGetPoses(this->TrackedDevicePose,
    vr::k_unMaxTrackedDeviceCount, NULL, 0 );

  // update the camera values based on the pose
  if ( this->TrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid )
  {
    vtkRenderer *ren;
    vtkCollectionSimpleIterator rit;
    this->Renderers->InitTraversal(rit);
    while ( (ren = this->Renderers->GetNextRenderer(rit)) )
    {
      vtkOpenVRCamera *cam = static_cast<vtkOpenVRCamera *>(ren->GetActiveCamera());
      this->HMDTransform->Identity();

      // get the position and orientation of the HMD
      vr::TrackedDevicePose_t &tdPose =
        this->TrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd];
      double pos[3];
      for (int i = 0; i < 3; i++)
      {
        pos[i] = tdPose.mDeviceToAbsoluteTracking.m[i][3];
      }

      double distance = cam->GetDistance();
      double *trans = cam->GetTranslation();

      for (int i = 0; i < 3; i++)
      {
        pos[i] = pos[i]*distance - trans[i];
      }

      double ortho[3][3];
      for (int i = 0; i < 3; i++)
      {
        ortho[0][i] = tdPose.mDeviceToAbsoluteTracking.m[0][i];
        ortho[1][i] = tdPose.mDeviceToAbsoluteTracking.m[1][i];
        ortho[2][i] = tdPose.mDeviceToAbsoluteTracking.m[2][i];
      }
      if (vtkMath::Determinant3x3(ortho) < 0)
      {
        ortho[0][2] = -ortho[0][2];
        ortho[1][2] = -ortho[1][2];
        ortho[2][2] = -ortho[2][2];
      }
      double wxyz[4];
      vtkMath::Matrix3x3ToQuaternion(ortho, wxyz);

      // calc the return value wxyz
     double mag = sqrt( wxyz[1] * wxyz[1] + wxyz[2] * wxyz[2] + wxyz[3] * wxyz[3] );
      if ( mag != 0.0 )
      {
        wxyz[0] = 2.0 * vtkMath::DegreesFromRadians( atan2( mag, wxyz[0] ) );
        wxyz[1] /= mag;
        wxyz[2] /= mag;
        wxyz[3] /= mag;
      }
      else
      {
        wxyz[0] = 0.0;
        wxyz[1] = 0.0;
        wxyz[2] = 0.0;
        wxyz[3] = 1.0;
      }

      this->HMDTransform->RotateWXYZ(wxyz[0], wxyz[1], wxyz[2], wxyz[3]);
      cam->SetFocalPoint(0,0,-1);
      cam->SetPosition(0,0,0);
      cam->SetViewUp(0,1,0);
      cam->ApplyTransform(this->HMDTransform);
      double *dop = cam->GetDirectionOfProjection();
      cam->SetFocalPoint(
        pos[0] + dop[0]*distance,
        pos[1] + dop[1]*distance,
        pos[2] + dop[2]*distance);
      cam->SetPosition(pos[0], pos[1], pos[2]);
    }
  }
}

void vtkOpenVRRenderWindow::Render()
{
  this->UpdateHMDMatrixPose();
  this->vtkRenderWindow::Render();
}

void vtkOpenVRRenderWindow::StereoUpdate()
{
  // camera handles what we need
}

void vtkOpenVRRenderWindow::StereoMidpoint()
{
  // render the left eye models
  this->RenderModels();

  glBindFramebuffer( GL_FRAMEBUFFER, 0 );
  glDisable( GL_MULTISAMPLE );

  glBindFramebuffer(GL_READ_FRAMEBUFFER, this->LeftEyeDesc.m_nRenderFramebufferId);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->LeftEyeDesc.m_nResolveFramebufferId );

  glBlitFramebuffer( 0, 0, this->RenderWidth, this->RenderHeight, 0, 0, this->RenderWidth, this->RenderHeight,
    GL_COLOR_BUFFER_BIT,
    GL_LINEAR );

  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0 );
}

void  vtkOpenVRRenderWindow::StereoRenderComplete()
{
  // render the right eye models
  this->RenderModels();

  glBindFramebuffer( GL_FRAMEBUFFER, 0 );
  glDisable( GL_MULTISAMPLE );

  glBindFramebuffer(GL_READ_FRAMEBUFFER, this->RightEyeDesc.m_nRenderFramebufferId );
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->RightEyeDesc.m_nResolveFramebufferId );

  glBlitFramebuffer( 0, 0, this->RenderWidth, this->RenderHeight, 0, 0, this->RenderWidth, this->RenderHeight,
    GL_COLOR_BUFFER_BIT,
    GL_LINEAR  );

  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0 );
}

// End the rendering process and display the image.
void vtkOpenVRRenderWindow::Frame(void)
{
  this->MakeCurrent();
  if (!this->AbortRender && this->DoubleBuffer && this->SwapBuffers)
  {
    // for now as fast as possible
    if ( this->HMD )
    {
      vr::Texture_t leftEyeTexture = {(void*)this->LeftEyeDesc.m_nResolveTextureId, vr::API_OpenGL, vr::ColorSpace_Gamma };
      vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture );
      vr::Texture_t rightEyeTexture = {(void*)this->RightEyeDesc.m_nResolveTextureId, vr::API_OpenGL, vr::ColorSpace_Gamma };
      vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture );
    }
    this->RenderDistortion();

    SDL_GL_SwapWindow( this->WindowId );
  }
}

// setup the Shader, VBO/IBO/VAO for the lens distortion pass
// THis pass is not actually part of rendering to the OPenVR
// device. It is just for simulating the user's view on the
// screen. It could be replaced with a more simple bitblt
// for a very tiny performance improvement.
void vtkOpenVRRenderWindow::SetupDistortion()
{
  GLushort iLensGridSegmentCountH = 43;
  GLushort iLensGridSegmentCountV = 43;

  float w = (float)( 1.0/float(iLensGridSegmentCountH-1));
  float h = (float)( 1.0/float(iLensGridSegmentCountV-1));

  float u, v = 0;

  std::vector<float> vbodata(0);

  //left eye distortion verts
  float Xoffset = -1;
  for( int y=0; y < iLensGridSegmentCountV; y++ )
  {
    v = 1-y*h;
    for( int x=0; x < iLensGridSegmentCountH; x++ )
    {
      u = x*w;
      vbodata.push_back( Xoffset+u );
      vbodata.push_back( -1+2*y*h );

      vr::DistortionCoordinates_t dc0 = this->HMD->ComputeDistortion(vr::Eye_Left, u, v);

      vbodata.push_back( dc0.rfRed[0] );
      vbodata.push_back( 1 - dc0.rfRed[1] );
      vbodata.push_back( dc0.rfGreen[0] );
      vbodata.push_back( 1 - dc0.rfGreen[1] );
      vbodata.push_back( dc0.rfBlue[0] );
      vbodata.push_back( 1 - dc0.rfBlue[1] );
    }
  }

  //right eye distortion verts
  Xoffset = 0;
  for( int y=0; y < iLensGridSegmentCountV; y++ )
  {
    v = 1-y*h;
    for( int x=0; x < iLensGridSegmentCountH; x++ )
    {
      u = x*w;
      vbodata.push_back( Xoffset+u );
      vbodata.push_back( -1+2*y*h );

      vr::DistortionCoordinates_t dc0 = this->HMD->ComputeDistortion( vr::Eye_Right, u, v );

      vbodata.push_back( dc0.rfRed[0] );
      vbodata.push_back( 1 - dc0.rfRed[1] );
      vbodata.push_back( dc0.rfGreen[0] );
      vbodata.push_back( 1 - dc0.rfGreen[1] );
      vbodata.push_back( dc0.rfBlue[0] );
      vbodata.push_back( 1 - dc0.rfBlue[1] );
    }
  }

  this->DistortionVBO->Upload(vbodata, vtkOpenGLBufferObject::ArrayBuffer);

  std::vector<GLushort> vIndices;
  GLushort a,b,c,d;

  GLushort offset = 0;
  for( GLushort y=0; y < iLensGridSegmentCountV-1; y++ )
  {
    for( GLushort x=0; x < iLensGridSegmentCountH-1; x++ )
    {
      a = iLensGridSegmentCountH*y+x +offset;
      b = iLensGridSegmentCountH*y+x+1 +offset;
      c = (y+1) * iLensGridSegmentCountH+x+1 +offset;
      d = (y+1) * iLensGridSegmentCountH+x +offset;
      vIndices.push_back( a );
      vIndices.push_back( b );
      vIndices.push_back( c );

      vIndices.push_back( a );
      vIndices.push_back( c );
      vIndices.push_back( d );
    }
  }

  offset = (iLensGridSegmentCountH)*(iLensGridSegmentCountV);
  for( GLushort y=0; y < iLensGridSegmentCountV-1; y++ )
  {
    for( GLushort x=0; x < iLensGridSegmentCountH-1; x++ )
    {
      a = iLensGridSegmentCountH*y+x +offset;
      b = iLensGridSegmentCountH*y+x+1 +offset;
      c = (y+1)*iLensGridSegmentCountH+x+1 +offset;
      d = (y+1)*iLensGridSegmentCountH+x +offset;
      vIndices.push_back( a );
      vIndices.push_back( b );
      vIndices.push_back( c );

      vIndices.push_back( a );
      vIndices.push_back( c );
      vIndices.push_back( d );
    }
  }

  this->Distortion.IBO->Upload(vIndices, vtkOpenGLBufferObject::ElementArrayBuffer);
  this->Distortion.IBO->IndexCount = vIndices.size();

  this->Distortion.Program = this->GetShaderCache()->ReadyShaderProgram(
    // vertex shader
    "//VTK::System::Dec\n"
    "attribute vec4 position;\n"
    "attribute vec2 redTC;\n"
    "attribute vec2 greenTC;\n"
    "attribute vec2 blueTC;\n"
    "noperspective out vec2 v2UVred;\n"
    "noperspective out vec2 v2UVgreen;\n"
    "noperspective out vec2 v2UVblue;\n"
    "void main()\n"
    "{\n"
    " v2UVred = redTC;\n"
    " v2UVgreen = greenTC;\n"
    " v2UVblue = blueTC;\n"
    " gl_Position = position;\n"
    "}\n",
    // fragment shader
    "//VTK::System::Dec\n"
    "//VTK::Output::Dec\n"
    "uniform sampler2D mytexture;\n"
    "noperspective  in vec2 v2UVred;\n"
    "noperspective  in vec2 v2UVgreen;\n"
    "noperspective  in vec2 v2UVblue;\n"

    "void main()\n"
    "{\n"
    " float fBoundsCheck = ( (dot( vec2( lessThan( v2UVgreen.xy, vec2(0.05, 0.05)) ), vec2(1.0, 1.0))+dot( vec2( greaterThan( v2UVgreen.xy, vec2( 0.95, 0.95)) ), vec2(1.0, 1.0))) );\n"
    " if( fBoundsCheck > 1.0 )\n"
    " { gl_FragData[0] = vec4( 0, 0, 0, 1.0 ); }\n"
    " else\n"
    " {\n"
    "   float red = texture(mytexture, v2UVred).x;\n"
    "   float green = texture(mytexture, v2UVgreen).y;\n"
    "   float blue = texture(mytexture, v2UVblue).z;\n"
    "   gl_FragData[0] = vec4( red, green, blue, 1.0  ); }\n"
    "}\n",

    // geom shader
    ""
    );

  vtkShaderProgram *program = this->Distortion.Program;
  this->Distortion.VAO->Bind();
  if (!this->Distortion.VAO->AddAttributeArray(program,
        this->DistortionVBO,
        "position", 0, 8*sizeof(float),
        VTK_FLOAT, 2, false))
  {
      vtkErrorMacro(<< "Error setting position in shader VAO.");
  }
  if (!this->Distortion.VAO->AddAttributeArray(program,
        this->DistortionVBO,
        "redTC", 2*sizeof(float), 8*sizeof(float),
        VTK_FLOAT, 2, false))
  {
      vtkErrorMacro(<< "Error setting redTC in shader VAO.");
  }
  if (!this->Distortion.VAO->AddAttributeArray(program,
        this->DistortionVBO,
        "greenTC", 4*sizeof(float), 8*sizeof(float),
        VTK_FLOAT, 2, false))
  {
      vtkErrorMacro(<< "Error setting greenTC in shader VAO.");
  }
  if (!this->Distortion.VAO->AddAttributeArray(program,
        this->DistortionVBO,
        "blueTC", 6*sizeof(float), 8*sizeof(float),
        VTK_FLOAT, 2, false))
  {
      vtkErrorMacro(<< "Error setting blueTC in shader VAO.");
  }
}

void vtkOpenVRRenderWindow::RenderDistortion()
{
  glDisable(GL_DEPTH_TEST);
  glViewport( 0, 0, this->Size[0]*2, this->Size[1] );

  this->GetShaderCache()->ReadyShaderProgram(this->Distortion.Program);
  this->Distortion.VAO->Bind();
  this->Distortion.IBO->Bind();

  //render left lens (first half of index array )
  glBindTexture(GL_TEXTURE_2D, this->LeftEyeDesc.m_nResolveTextureId );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
  glDrawElements( GL_TRIANGLES,
    static_cast<GLsizei>(this->Distortion.IBO->IndexCount/2), GL_UNSIGNED_SHORT, 0 );

  //render right lens (second half of index array )
  glBindTexture(GL_TEXTURE_2D, this->RightEyeDesc.m_nResolveTextureId  );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
  glDrawElements( GL_TRIANGLES,
    static_cast<GLsizei>(this->Distortion.IBO->IndexCount/2),
    GL_UNSIGNED_SHORT, (const void *)(this->Distortion.IBO->IndexCount/2*sizeof(unsigned short)) );
}

bool vtkOpenVRRenderWindow::CreateFrameBuffer( int nWidth, int nHeight, FramebufferDesc &framebufferDesc )
{
  glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId );
  glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);

  glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
  glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
  if (this->GetMultiSamples())
  {
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, nWidth, nHeight );
  }
  else
  {
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, nWidth, nHeight );
  }
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId );

  glGenTextures(1, &framebufferDesc.m_nRenderTextureId );
  if (this->GetMultiSamples())
  {
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId );
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, nWidth, nHeight, true);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId, 0);
  }
  else
  {
    glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nRenderTextureId );
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nRenderTextureId, 0);
  }
  glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId );
  glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);

  glGenTextures(1, &framebufferDesc.m_nResolveTextureId );
  glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0);

  // check FBO status
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    return false;
  }

  glBindFramebuffer( GL_FRAMEBUFFER, 0 );

  return true;
}


// Initialize the rendering window.
void vtkOpenVRRenderWindow::Initialize (void)
{
  if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER ) < 0 )
  {
    vtkErrorMacro("SDL could not initialize! SDL Error: " <<  SDL_GetError());
    return;
  }

  // Loading the SteamVR Runtime
  vr::EVRInitError eError = vr::VRInitError_None;
  this->HMD = vr::VR_Init( &eError, vr::VRApplication_Scene );

  if ( eError != vr::VRInitError_None )
  {
    this->HMD = NULL;
    char buf[1024];
    sprintf_s( buf, sizeof( buf ), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription( eError ) );
    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "VR_Init Failed", buf, NULL );
    return;
  }

  this->OpenVRRenderModels = (vr::IVRRenderModels *)
    vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, &eError );
  if( !this->OpenVRRenderModels )
  {
    this->HMD = NULL;
    vr::VR_Shutdown();

    char buf[1024];
    sprintf_s( buf, sizeof( buf ), "Unable to get render model interface: %s", vr::VR_GetVRInitErrorAsEnglishDescription( eError ) );
    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "VR_Init Failed", buf, NULL );
    return;
  }

  Uint32 unWindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
  //Uint32 unWindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN;

  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
  //SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

  SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 0 );
  SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 0 );

  this->WindowId = SDL_CreateWindow( this->WindowName,
    this->Position[0], this->Position[1],
    this->Size[0]*2, this->Size[1],
    unWindowFlags );
  if (this->WindowId == NULL)
  {
    vtkErrorMacro("Window could not be created! SDL Error: " <<  SDL_GetError());
    return;
  }

  this->ContextId = SDL_GL_CreateContext(this->WindowId);
  if (this->ContextId == NULL)
  {
    vtkErrorMacro("OpenGL context could not be created! SDL Error: " <<  SDL_GetError() );
    return;
  }

  this->OpenGLInit();

  // make sure vsync is off
  if ( SDL_GL_SetSwapInterval( 0 ) < 0 )
  {
    vtkErrorMacro("Warning: Unable to set VSync! SDL Error: " << SDL_GetError() );
    return;
  }

  m_strDriver = "No Driver";
  m_strDisplay = "No Display";

  m_strDriver = GetTrackedDeviceString( this->HMD,
    vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String );
  m_strDisplay = GetTrackedDeviceString( this->HMD,
    vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String );

  std::string strWindowTitle = "VTK - " + m_strDriver + " " + m_strDisplay;
  this->SetWindowName(strWindowTitle.c_str());
  SDL_SetWindowTitle( this->WindowId, this->WindowName );

  this->HMD->GetRecommendedRenderTargetSize(
    &this->RenderWidth, &this->RenderHeight );

  this->CreateFrameBuffer( this->RenderWidth, this->RenderHeight, this->LeftEyeDesc );
  this->CreateFrameBuffer( this->RenderWidth, this->RenderHeight, this->RightEyeDesc );

  this->SetupDistortion();

  if ( !vr::VRCompositor() )
  {
    vtkErrorMacro("Compositor initialization failed." );
    return;
  }
}

void vtkOpenVRRenderWindow::Finalize (void)
{
  this->Clean();

  if( this->HMD )
  {
    vr::VR_Shutdown();
    this->HMD = NULL;
  }

  for( std::vector< vtkOpenVRModel * >::iterator i = this->VTKRenderModels.begin();
       i != this->VTKRenderModels.end(); i++ )
  {
    (*i)->Delete();
  }
  this->VTKRenderModels.clear();

  if( this->ContextId )
  {
  }

  if( this->WindowId )
  {
    SDL_DestroyWindow( this->WindowId );
    this->WindowId = NULL;
  }

  SDL_Quit();
}

void vtkOpenVRRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->ContextId << "\n";
  os << indent << "Window Id: " << this->WindowId << "\n";
}

// Begin the rendering process.
void vtkOpenVRRenderWindow::Start(void)
{
  // if the renderer has not been initialized, do so now
  if (!this->ContextId)
  {
    this->Initialize();
  }

  // set the current window
  this->MakeCurrent();
}
