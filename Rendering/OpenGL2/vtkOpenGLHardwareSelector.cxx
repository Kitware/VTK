/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLHardwareSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLHardwareSelector.h"

#include "vtk_glew.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

#include "vtkOpenGLError.h"

//#define vtkOpenGLHardwareSelectorDEBUG
#ifdef vtkOpenGLHardwareSelectorDEBUG
#include "vtkImageImport.h"
#include "vtkNew.h"
#include "vtkPNMWriter.h"
#include "vtkWindows.h" // OK on UNix etc
#include <sstream>
#endif

#define ID_OFFSET 1

namespace
{
void annotate(const std::string& str)
{
  vtkOpenGLRenderUtilities::MarkDebugEvent(str);
}
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLHardwareSelector);

//----------------------------------------------------------------------------
vtkOpenGLHardwareSelector::vtkOpenGLHardwareSelector()
{
#ifdef vtkOpenGLHardwareSelectorDEBUG
  cerr << "=====vtkOpenGLHardwareSelector::vtkOpenGLHardwareSelector" << endl;
#endif
}

//----------------------------------------------------------------------------
vtkOpenGLHardwareSelector::~vtkOpenGLHardwareSelector()
{
#ifdef vtkOpenGLHardwareSelectorDEBUG
  cerr << "=====vtkOpenGLHardwareSelector::~vtkOpenGLHardwareSelector" << endl;
#endif
}

//----------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::PreCapturePass(int pass)
{
  annotate(std::string("Starting pass: ") + this->PassTypeToString(static_cast<PassTypes>(pass)));

  // Disable blending
  vtkOpenGLRenderWindow* rwin =
    static_cast<vtkOpenGLRenderWindow*>(this->Renderer->GetRenderWindow());
  vtkOpenGLState* ostate = rwin->GetState();

  this->OriginalBlending = ostate->GetEnumState(GL_BLEND);
  ostate->vtkglDisable(GL_BLEND);
}

//----------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::PostCapturePass(int pass)
{
  // Restore blending.
  vtkOpenGLRenderWindow* rwin =
    static_cast<vtkOpenGLRenderWindow*>(this->Renderer->GetRenderWindow());
  vtkOpenGLState* ostate = rwin->GetState();

  ostate->SetEnumState(GL_BLEND, this->OriginalBlending);
  annotate(std::string("Pass complete: ") + this->PassTypeToString(static_cast<PassTypes>(pass)));
}

//----------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::BeginSelection()
{
  vtkOpenGLRenderWindow* rwin =
    static_cast<vtkOpenGLRenderWindow*>(this->Renderer->GetRenderWindow());

  this->OriginalMultiSample = rwin->GetMultiSamples();
  rwin->SetMultiSamples(0);

  vtkOpenGLState* ostate = rwin->GetState();
  ostate->ResetFramebufferBindings();

  // render normally to set the zbuffer
  if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    vtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
    ostate->vtkglDisable(GL_BLEND);

    rwin->Render();
    this->Renderer->PreserveDepthBufferOn();
  }

  return this->Superclass::BeginSelection();
}

//----------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::EndSelection()
{
  // render normally to set the zbuffer
  if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    this->Renderer->PreserveDepthBufferOff();
  }

  vtkOpenGLRenderWindow* rwin =
    static_cast<vtkOpenGLRenderWindow*>(this->Renderer->GetRenderWindow());
  rwin->SetMultiSamples(this->OriginalMultiSample);

  return this->Superclass::EndSelection();
}

//----------------------------------------------------------------------------
// just add debug output if compiled with vtkOpenGLHardwareSelectorDEBUG
void vtkOpenGLHardwareSelector::SavePixelBuffer(int passNo)
{
  this->Superclass::SavePixelBuffer(passNo);

#ifdef vtkOpenGLHardwareSelectorDEBUG

  vtkNew<vtkImageImport> ii;
  ii->SetImportVoidPointer(this->PixBuffer[passNo], 1);
  ii->SetDataScalarTypeToUnsignedChar();
  ii->SetNumberOfScalarComponents(3);
  ii->SetDataExtent(this->Area[0], this->Area[2], this->Area[1], this->Area[3], 0, 0);
  ii->SetWholeExtent(this->Area[0], this->Area[2], this->Area[1], this->Area[3], 0, 0);

  // change this to somewhere on your system
  // hardcoded as with MPI/parallel/client server it can be hard to
  // find these images sometimes.
  std::string fname = "C:/Users/ken.martin/Documents/pickbuffer_";

#if defined(_WIN32)
  std::ostringstream toString;
  toString.str("");
  toString.clear();
  toString << GetCurrentProcessId();
  fname += toString.str();
  fname += "_";
#endif
  fname += ('0' + passNo);
  fname += ".pnm";
  vtkNew<vtkPNMWriter> pw;
  pw->SetInputConnection(ii->GetOutputPort());
  pw->SetFileName(fname.c_str());
  pw->Write();
  cerr << "=====vtkOpenGLHardwareSelector wrote " << fname << "\n";
#endif
}

//----------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::BeginRenderProp(vtkRenderWindow*)
{
#ifdef vtkOpenGLHardwareSelectorDEBUG
  cerr << "=====vtkOpenGLHardwareSelector::BeginRenderProp" << endl;
#endif
}

//----------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::BeginRenderProp()
{
  this->InPropRender++;
  if (this->InPropRender != 1)
  {
    return;
  }

  // device specific prep
  vtkRenderWindow* renWin = this->Renderer->GetRenderWindow();
  this->BeginRenderProp(renWin);

  // cout << "In BeginRenderProp" << endl;
  if (this->CurrentPass == ACTOR_PASS)
  {
    int propid = this->PropID;
    if (propid >= 0xfffffe)
    {
      vtkErrorMacro("Too many props. Currently only " << 0xfffffe << " props are supported.");
      return;
    }
    float color[3];
    // Since 0 is reserved for nothing selected, we offset propid by 1.
    propid = propid + 1;
    vtkHardwareSelector::Convert(propid, color);
    this->SetPropColorValue(color);
  }
  else if (this->CurrentPass == PROCESS_PASS)
  {
    float color[3];
    // Since 0 is reserved for nothing selected, we offset propid by 1.
    vtkHardwareSelector::Convert(this->ProcessID + 1, color);
    this->SetPropColorValue(color);
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::EndRenderProp(vtkRenderWindow*)
{
#ifdef vtkOpenGLHardwareSelectorDEBUG
  cerr << "=====vtkOpenGLHardwareSelector::EndRenderProp" << endl;
#endif
}

//----------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::EndRenderProp()
{
  this->Superclass::EndRenderProp();
}

//----------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::RenderCompositeIndex(unsigned int index)
{

  if (index > 0xffffff)
  {
    vtkErrorMacro("Indices > 0xffffff are not supported.");
    return;
  }

  index += ID_OFFSET;

  if (this->CurrentPass == COMPOSITE_INDEX_PASS)
  {
    float color[3];
    vtkHardwareSelector::Convert(static_cast<vtkIdType>(0xffffff & index), color);
    this->SetPropColorValue(color);
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::RenderProcessId(unsigned int processid)
{
  if (this->CurrentPass == PROCESS_PASS && this->UseProcessIdFromData)
  {
    if (processid >= 0xffffff)
    {
      vtkErrorMacro("Invalid id: " << processid);
      return;
    }

    float color[3];
    vtkHardwareSelector::Convert(static_cast<vtkIdType>(processid + 1), color);
    this->SetPropColorValue(color);
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
