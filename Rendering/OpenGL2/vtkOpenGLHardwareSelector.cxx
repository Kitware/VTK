/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGL2HardwareSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGL2HardwareSelector.h"

#include <GL/glew.h>

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGL2RenderWindow.h"

#include "vtkOpenGLError.h"

#define ID_OFFSET 1

// Description:
// Internal state and helper methods.
class vtkOpenGL2HardwareSelector::vtkInternals
{
public:
  vtkOpenGL2RenderWindow *Context;
  bool MultisampleSupport;
  bool OriginalMultisample;
  bool OriginalBlending;

  vtkInternals() :
    Context(NULL),
    MultisampleSupport(false),
    OriginalMultisample(false),
    OriginalBlending(false)
    {}

  // Description:
  // Set the rendering context and load the required
  // extensions.
  void SetContext(vtkRenderWindow *context)
    {
    if (this->Context != context)
      {
      this->MultisampleSupport = true;
      }
    }

  // Description:
  // Check if lighting is enabled.
  bool QueryLighting()
    {
    if (glIsEnabled(GL_LIGHTING))
      {
      return true;
      }
    else
      {
      return false;
      }
    }

  // Description:
  // Enable/disable multisampling.
  void EnableMultisampling(bool mode)
    {
    if (this->MultisampleSupport)
      {
      if (mode)
        {
        glEnable(GL_MULTISAMPLE);
        }
      else
        {
        glDisable(GL_MULTISAMPLE);
        }
      }
    }

  // Description:
  // Check if multisample is enabled.
  bool QueryMultisampling()
    {
    if (this->MultisampleSupport && glIsEnabled(GL_MULTISAMPLE))
      {
      return true;
      }
    else
      {
      return false;
      }
    }

  // Description:
  // Enable/Disable blending
  void EnableBlending(bool mode)
    {
    if (mode)
      {
      glEnable(GL_BLEND);
      }
    else
      {
      glDisable(GL_BLEND);
      }
    }

  // Description:
  // Check if blending is enabled.
  bool QueryBlending()
    {
    if (glIsEnabled(GL_BLEND))
      {
      return true;
      }
    else
      {
      return false;
      }
    }
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGL2HardwareSelector);

//----------------------------------------------------------------------------
vtkOpenGL2HardwareSelector::vtkOpenGL2HardwareSelector()
{
  #ifdef vtkOpenGL2HardwareSelectorDEBUG
  cerr << "=====vtkOpenGL2HardwareSelector::vtkOpenGL2HardwareSelector" << endl;
  #endif
  this->Internals = new vtkInternals;
}

//----------------------------------------------------------------------------
vtkOpenGL2HardwareSelector::~vtkOpenGL2HardwareSelector()
{
  #ifdef vtkOpenGL2HardwareSelectorDEBUG
  cerr << "=====vtkOpenGL2HardwareSelector::~vtkOpenGL2HardwareSelector" << endl;
  #endif
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkOpenGL2HardwareSelector::BeginRenderProp(vtkRenderWindow *context)
{
  #ifdef vtkOpenGL2HardwareSelectorDEBUG
  cerr << "=====vtkOpenGL2HardwareSelector::BeginRenderProp" << endl;
  #endif

  this->Internals->SetContext(context);

  // Disable multisample, lighting, and blending.
  this->Internals->OriginalMultisample = this->Internals->QueryMultisampling();
  this->Internals->EnableMultisampling(false);

  this->Internals->OriginalBlending = this->Internals->QueryBlending();
  this->Internals->EnableBlending(false);
}

//----------------------------------------------------------------------------
void vtkOpenGL2HardwareSelector::EndRenderProp(vtkRenderWindow *)
{
  #ifdef vtkOpenGL2HardwareSelectorDEBUG
  cerr << "=====vtkOpenGL2HardwareSelector::EndRenderProp" << endl;
  #endif

  // Restore multisample, lighting, and blending.
  this->Internals->EnableMultisampling(this->Internals->OriginalMultisample);
  this->Internals->EnableBlending(this->Internals->OriginalBlending);
}

//----------------------------------------------------------------------------
void vtkOpenGL2HardwareSelector::BeginRenderProp()
{
  this->InPropRender++;
  if (this->InPropRender != 1)
    {
    return;
    }

  // device specific prep
  vtkRenderWindow *renWin = this->Renderer->GetRenderWindow();
  this->BeginRenderProp(renWin);

  //cout << "In BeginRenderProp" << endl;
  //glFinish();
  if (this->CurrentPass == ACTOR_PASS)
    {
    int propid = this->PropID;
    if (propid >= 0xfffffe)
      {
      vtkErrorMacro("Too many props. Currently only " << 0xfffffe
        << " props are supported.");
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
void vtkOpenGL2HardwareSelector::RenderCompositeIndex(unsigned int index)
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
    vtkHardwareSelector::Convert(static_cast<int>(0xffffff & index), color);
    this->SetPropColorValue(color);
    }
}

//----------------------------------------------------------------------------
// TODO: make inline
void vtkOpenGL2HardwareSelector::RenderAttributeId(vtkIdType attribid)
{
  if (attribid < 0)
    {
    vtkErrorMacro("Invalid id: " << attribid);
    return;
    }

  this->MaxAttributeId = (attribid > this->MaxAttributeId)? attribid :
    this->MaxAttributeId;

  if (this->CurrentPass < ID_LOW24 || this->CurrentPass > ID_HIGH16)
    {
    return;
    }

  // 0 is reserved.
  attribid += ID_OFFSET;

  for (int cc=0; cc < 3; cc++)
    {
    int words24 = (0xffffff & attribid);
    attribid = attribid >> 24;
    if ((this->CurrentPass - ID_LOW24) == cc)
      {
      float color[3];
      vtkHardwareSelector::Convert(words24, color);
      this->SetPropColorValue(color);
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkOpenGL2HardwareSelector::RenderProcessId(unsigned int processid)
{
  if (this->CurrentPass == PROCESS_PASS && this->UseProcessIdFromData)
    {
    if (processid >= 0xffffff)
      {
      vtkErrorMacro("Invalid id: " << processid);
      return;
      }

    float color[3];
    vtkHardwareSelector::Convert(
      static_cast<int>(processid + 1), color);
    this->SetPropColorValue(color);
    }
}



//----------------------------------------------------------------------------
void vtkOpenGL2HardwareSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os
   << indent << "MultisampleSupport: "
   << this->Internals->MultisampleSupport
   << endl;
}
