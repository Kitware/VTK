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

#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"

#include "vtkgl.h"
#include "vtkOpenGLError.h"

// Description:
// Internal state and helper methods.
class vtkOpenGLHardwareSelector::vtkInternals
{
public:
  vtkOpenGLRenderWindow *Context;
  bool MultisampleSupport;
  bool OriginalMultisample;
  bool OriginalLighting;
  bool OriginalBlending;

  vtkInternals() :
    Context(NULL),
    MultisampleSupport(false),
    OriginalMultisample(false),
    OriginalLighting(false),
    OriginalBlending(false)
    {}

  // Description:
  // Set the rendering context and load the required
  // extensions.
  void SetContext(vtkRenderWindow *context)
  {
    if (this->Context != context)
    {
      this->MultisampleSupport = false;
      this->Context = vtkOpenGLRenderWindow::SafeDownCast(context);
      if (this->Context)
      {
        vtkOpenGLExtensionManager *manager
           = this->Context->GetExtensionManager();

        // don't need any of the functions so don't bother
        // to load the extension, but do make sure enums are
        // defined.
        this->MultisampleSupport
          = manager->ExtensionSupported("GL_ARB_multisample")==1;
      }
    }
  }

  // Description:
  // Enable/disable lighting
  void EnableLighting(bool mode)
  {
    if (mode)
    {
      glEnable(GL_LIGHTING);
    }
    else
    {
      glDisable(GL_LIGHTING);
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
        glEnable(vtkgl::MULTISAMPLE);
      }
      else
      {
        glDisable(vtkgl::MULTISAMPLE);
      }
    }
  }

  // Description:
  // Check if multisample is enabled.
  bool QueryMultisampling()
  {
    if (this->MultisampleSupport && glIsEnabled(vtkgl::MULTISAMPLE))
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
vtkStandardNewMacro(vtkOpenGLHardwareSelector);

//----------------------------------------------------------------------------
vtkOpenGLHardwareSelector::vtkOpenGLHardwareSelector()
{
  #ifdef vtkOpenGLHardwareSelectorDEBUG
  cerr << "=====vtkOpenGLHardwareSelector::vtkOpenGLHardwareSelector" << endl;
  #endif
  this->Internals = new vtkInternals;
}

//----------------------------------------------------------------------------
vtkOpenGLHardwareSelector::~vtkOpenGLHardwareSelector()
{
  #ifdef vtkOpenGLHardwareSelectorDEBUG
  cerr << "=====vtkOpenGLHardwareSelector::~vtkOpenGLHardwareSelector" << endl;
  #endif
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::BeginRenderProp(vtkRenderWindow *context)
{
  #ifdef vtkOpenGLHardwareSelectorDEBUG
  cerr << "=====vtkOpenGLHardwareSelector::BeginRenderProp" << endl;
  #endif

  this->Internals->SetContext(context);

  // Disable multisample, lighting, and blending.
  this->Internals->OriginalMultisample = this->Internals->QueryMultisampling();
  this->Internals->EnableMultisampling(false);

  this->Internals->OriginalLighting = this->Internals->QueryLighting();
  this->Internals->EnableLighting(false);

  this->Internals->OriginalBlending = this->Internals->QueryBlending();
  this->Internals->EnableBlending(false);
}

//----------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::EndRenderProp(vtkRenderWindow *)
{
  #ifdef vtkOpenGLHardwareSelectorDEBUG
  cerr << "=====vtkOpenGLHardwareSelector::EndRenderProp" << endl;
  #endif

  // Restore multisample, lighting, and blending.
  this->Internals->EnableMultisampling(this->Internals->OriginalMultisample);
  this->Internals->EnableLighting(this->Internals->OriginalLighting);
  this->Internals->EnableBlending(this->Internals->OriginalBlending);
}

//----------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os
   << indent << "MultisampleSupport: "
   << this->Internals->MultisampleSupport
   << endl;
}
