/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValuePass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkValuePass.h"

#include "vtkClearRGBPass.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationStringKey.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkRenderer.h"
#include "vtkRenderState.h"
#include "vtkSmartPointer.h"
#include "vtkRenderbuffer.h"
#include "vtkRenderWindow.h"
#include "vtkFloatArray.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkFrameBufferObject2.h"

#include <cassert>


vtkStandardNewMacro(vtkValuePass);

vtkInformationKeyMacro(vtkValuePass, RENDER_VALUES, Integer);

vtkInformationKeyMacro(vtkValuePass, SCALAR_MODE, Integer);
vtkInformationKeyMacro(vtkValuePass, ARRAY_MODE, Integer);
vtkInformationKeyMacro(vtkValuePass, ARRAY_ID, Integer);
vtkInformationKeyMacro(vtkValuePass, ARRAY_NAME, String);
vtkInformationKeyMacro(vtkValuePass, ARRAY_COMPONENT, Integer);
vtkInformationKeyMacro(vtkValuePass, SCALAR_RANGE, DoubleVector);

class vtkValuePass::vtkInternals
{
public:
  int FieldAssociation;
  int FieldAttributeType;
  std::string FieldName;
  bool FieldNameSet;
  int Component;
  double ScalarRange[2];
  bool ScalarRangeSet;
  vtkSmartPointer<vtkClearRGBPass> ClearPass;

  vtkInternals()
  : ClearPass(vtkSmartPointer<vtkClearRGBPass>::New())
    {
    this->FieldAssociation = 0;
    this->FieldAttributeType = 0;
    this->FieldName = "";
    this->FieldNameSet = false;
    this->Component = 0;
    this->ScalarRange[0] = 0.0;
    this->ScalarRange[1] = -1.0;
    this->ScalarRangeSet = false;
    }
};

// ----------------------------------------------------------------------------
vtkValuePass::vtkValuePass()
: RenderingMode(1)
, ValueRenderBO(NULL)
, ValueFrameBO(NULL)
, ValuePassResourcesAllocated(false)
{
  this->Internals = new vtkInternals();

  this->Size[0] = 0;
  this->Size[1] = 0;
}

// ----------------------------------------------------------------------------
vtkValuePass::~vtkValuePass()
{
  delete this->Internals;
}

// ----------------------------------------------------------------------------
void vtkValuePass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
void vtkValuePass::SetInputArrayToProcess(int fieldAssociation,
  const char *name)
{
  if (!this->Internals->FieldNameSet ||
      this->Internals->FieldAssociation != fieldAssociation ||
      this->Internals->FieldName.compare(name) != 0)
    {
    this->Internals->FieldAssociation = fieldAssociation;
    this->Internals->FieldName = name;
    this->Internals->FieldNameSet = true;
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
void vtkValuePass::SetInputArrayToProcess(int fieldAssociation,
  int fieldAttributeType)
{
  if (this->Internals->FieldAssociation != fieldAssociation ||
      this->Internals->FieldAttributeType != fieldAttributeType ||
      this->Internals->FieldNameSet)
    {
    this->Internals->FieldAssociation = fieldAssociation;
    this->Internals->FieldAttributeType = fieldAttributeType;
    this->Internals->FieldNameSet = false;
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
void vtkValuePass::SetInputComponentToProcess(int component)
{
  if (this->Internals->Component != component)
    {
    this->Internals->Component = component;
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
void vtkValuePass::SetScalarRange(double min, double max)
{
  if (this->Internals->ScalarRange[0] != min ||
      this->Internals->ScalarRange[1] != max)
    {
    this->Internals->ScalarRange[0] = min;
    this->Internals->ScalarRange[1] = max;
    this->Internals->ScalarRangeSet = (max > min);
    this->Modified();
    }
}


// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkValuePass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  this->BeginPass(s->GetRenderer());

  this->Internals->ClearPass->Render(s);

  this->NumberOfRenderedProps=0;
  this->RenderOpaqueGeometry(s);

  // vtkFrameBufferObject2 is not supported
  //s->SetFrameBuffer(this->ValueFrameBO);

  this->EndPass();
}

// ----------------------------------------------------------------------------
// Description:
// Opaque pass with key checking.
// \pre s_exists: s!=0
void vtkValuePass::RenderOpaqueGeometry(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  std::vector<int> scalarVisibilities;

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
    {
    vtkProp *p = s->GetPropArray()[i];

    // Cache scalar visibility state and turn it on
    vtkActor *a = vtkActor::SafeDownCast(p);
    if (a)
      {
      scalarVisibilities.push_back(a->GetMapper()->GetScalarVisibility());
      a->GetMapper()->ScalarVisibilityOn();
      }

    vtkSmartPointer<vtkInformation> keys = p->GetPropertyKeys();
    if (!keys)
      {
      keys.TakeReference(vtkInformation::New());
      }
    keys->Set(vtkValuePass::RENDER_VALUES(), this->RenderingMode);
    keys->Set(vtkValuePass::SCALAR_MODE(), this->Internals->FieldAssociation);
    keys->Set(vtkValuePass::ARRAY_MODE(), this->Internals->FieldNameSet);
    keys->Set(vtkValuePass::ARRAY_ID(), this->Internals->FieldAttributeType);
    keys->Set(vtkValuePass::ARRAY_NAME(), this->Internals->FieldName.c_str());
    keys->Set(vtkValuePass::ARRAY_COMPONENT(), this->Internals->Component);
    keys->Set(vtkValuePass::SCALAR_RANGE(), this->Internals->ScalarRange, 2);
    p->SetPropertyKeys(keys);

    int rendered =
      p->RenderOpaqueGeometry(s->GetRenderer());
    this->NumberOfRenderedProps += rendered;
    ++i;
    }

  // Remove set keys
  i = 0;
  while (i < c)
    {
    vtkProp *p = s->GetPropArray()[i];

    // Restore scalar visibility state
    vtkActor *a = vtkActor::SafeDownCast(p);
    if (a)
      {
      a->GetMapper()->SetScalarVisibility(scalarVisibilities[i]);
      }

    vtkInformation *keys = p->GetPropertyKeys();
    keys->Remove(vtkValuePass::RENDER_VALUES());
    keys->Remove(vtkValuePass::SCALAR_MODE());
    keys->Remove(vtkValuePass::ARRAY_MODE());
    keys->Remove(vtkValuePass::ARRAY_ID());
    keys->Remove(vtkValuePass::ARRAY_NAME());
    keys->Remove(vtkValuePass::ARRAY_COMPONENT());
    keys->Remove(vtkValuePass::SCALAR_RANGE());
    p->SetPropertyKeys(keys);
    ++i;
    }
}

//------------------------------------------------------------------------------
void vtkValuePass::BeginPass(vtkRenderer* ren)
{
  switch(this->RenderingMode)
  {
  case vtkValuePass::FLOATING_POINT:
    // Allocate if necessary and bind frame buffer.
    this->InitializeFloatingPointMode(ren);
    this->ValueFrameBO->Bind(GL_DRAW_FRAMEBUFFER);
    break;

  case vtkValuePass::INVERTIBLE_LUT:
  default:
    // Cleanup in case FLOATING_POINT was active.
    this->ReleaseFloatingPointMode(ren);
    break;
  }
}

//------------------------------------------------------------------------------
void vtkValuePass::EndPass()
{
  switch(this->RenderingMode)
  {
  case vtkValuePass::FLOATING_POINT:
    // Unbind the float FBO and glReadPixels to host side.
    this->ValueFrameBO->UnBind(GL_DRAW_FRAMEBUFFER);
    break;

  case vtkValuePass::INVERTIBLE_LUT:
  default:
    // Nothing to do in this mode.
    break;
  }
}

//------------------------------------------------------------------------------
void vtkValuePass::InitializeFloatingPointMode(vtkRenderer* ren)
{
  if (this->ValuePassResourcesAllocated)
    return;

  vtkRenderWindow* renWin = ren->GetRenderWindow();
  if (!this->IsFloatFBOSupported(renWin))
    return;

  // Allocate FBO's Color attachment target
  int* size = ren->GetSize();
  this->Size[0] = size[0];
  this->Size[1] = size[1];

  this->ValueRenderBO = vtkRenderbuffer::New();
  this->ValueRenderBO->SetContext(renWin);
  this->ValueRenderBO->CreateColorAttachment(this->Size[0], this->Size[1]);

  // Initialize the FBO into which the float value pass is rendered.
  this->ValueFrameBO = vtkFrameBufferObject2::New();
  this->ValueFrameBO->SetContext(renWin);
  this->ValueFrameBO->Bind(GL_FRAMEBUFFER);
  this->ValueFrameBO->InitializeViewport(this->Size[0], this->Size[1]);
  /* GL_COLOR_ATTACHMENT0 */
  this->ValueFrameBO->AddColorAttachment(GL_FRAMEBUFFER, 0, this->ValueRenderBO);

  // Verify FBO
  if(!this->ValueFrameBO->CheckFrameBufferStatus(GL_FRAMEBUFFER))
    {
    vtkErrorMacro("Failed to attach FBO.");
    this->ReleaseFloatingPointMode(ren);
    }

  this->ValueFrameBO->UnBind(GL_FRAMEBUFFER);
  this->ValuePassResourcesAllocated = true;
}

//-----------------------------------------------------------------------------
void vtkValuePass::ReleaseFloatingPointMode(vtkRenderer* ren)
{
  if (!this->ValuePassResourcesAllocated)
    return;

  vtkRenderWindow* renWin = ren->GetRenderWindow();
  renWin->MakeCurrent();

  // Cleanup FBO (grahpics resources cleaned internally)
  this->ValueFrameBO->Delete();
  this->ValueFrameBO = NULL;

  this->ValueRenderBO->Delete();
  this->ValueRenderBO = NULL;

  this->ValuePassResourcesAllocated = false;
}

//-----------------------------------------------------------------------------
bool vtkValuePass::IsFloatFBOSupported(vtkRenderWindow *renWin)
{
  vtkOpenGLRenderWindow *context = vtkOpenGLRenderWindow::SafeDownCast(renWin);
  if (!context)
    {
    vtkErrorMacro(<< "Support for " << renWin->GetClassName()
      << " not implemented");
    return false;
    }

#if GL_ES_VERSION_2_0 != 1
  bool contextSupport = vtkOpenGLRenderWindow::GetContextSupportsOpenGL32();
  if (!contextSupport)
    {
    vtkWarningMacro(<< "Context does not support OpenGL core profile 3.2. "
      << " Will check extension support.");
    }

  bool extSupport = glewIsSupported("GL_EXT_framebuffer_object") &&
    glewIsSupported("GL_ARB_texture_float");
  if (!extSupport)
    {
    vtkWarningMacro(<< "EXT_framebuffer_object or ARB_texture_float not"
      << " supported.");
    }

  return contextSupport || extSupport;
#else
  return true;
#endif
}

//------------------------------------------------------------------------------
vtkFloatArray* vtkValuePass::GetFloatImageData(vtkRenderer* ren)
{
  vtkRenderWindow* renWin = ren->GetRenderWindow();
  renWin->MakeCurrent();

  //Allocate output array.
  vtkFloatArray* pixels = vtkFloatArray::New();
  pixels->SetNumberOfValues(this->Size[0] * this->Size[1] /* * numComponents */);

  // Prepare and bind value texture and FBO.
  this->ValueFrameBO->Bind(GL_READ_FRAMEBUFFER);

  GLint originalReadBuff;
  glGetIntegerv(GL_READ_BUFFER, &originalReadBuff);
  glReadBuffer(GL_COLOR_ATTACHMENT0);

  // Calling pack alignment ensures any window size can be grabbed.
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);

  glReadPixels(0, 0, this->Size[0], this->Size[1], GL_RED, GL_FLOAT,
    pixels->GetVoidPointer(0));

  glReadBuffer(originalReadBuff);
  this->ValueFrameBO->UnBind(GL_READ_FRAMEBUFFER);

  vtkOpenGLCheckErrorMacro("Failed to read pixels from OpenGL buffer!");
  return pixels;
}

//-------------------------------------------------------------------------------
std::vector<int> vtkValuePass::GetFloatImageExtents(vtkRenderer* ren)
{
  vtkRenderWindow* renWin = ren->GetRenderWindow();
  renWin->MakeCurrent();
  int* size = ren->GetSize();

  std::vector<int> ext;
  ext.reserve(6);
  ext.push_back(0); ext.push_back(this->Size[0] - 1);
  ext.push_back(0); ext.push_back(this->Size[1] - 1);
  ext.push_back(0); ext.push_back(0);

  return ext;
}
