/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValuePasses.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkValuePasses.h"
#include "vtkObjectFactory.h"

#include "vtkClearRGBPass.h"
#include "vtkLightsPass.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderState.h"
#include "vtkSequencePass.h"
#include "vtkSmartPointer.h"
#include "vtkValuePass.h"

// ----------------------------------------------------------------------------
class vtkValuePasses::vtkInternals
{
public:
  vtkInternals()
  {
    this->SequencePass = vtkSequencePass::New();
    this->ClearPass = vtkClearRGBPass::New();
    this->LightsPass = vtkLightsPass::New();
    this->ValuePass = vtkValuePass::New();

    vtkSmartPointer<vtkRenderPassCollection> passes=vtkSmartPointer<vtkRenderPassCollection>::New();
    passes->AddItem(this->ClearPass);
    passes->AddItem(this->LightsPass);
    passes->AddItem(this->ValuePass);
    this->SequencePass->SetPasses(passes);
  }

  ~vtkInternals()
  {
    this->SequencePass->Delete();
    this->ClearPass->Delete();
    this->LightsPass->Delete();
    this->ValuePass->Delete();
  }

  vtkSequencePass *SequencePass;
  vtkClearRGBPass *ClearPass;
  vtkLightsPass *LightsPass;
  vtkValuePass *ValuePass;
};

// ============================================================================
vtkStandardNewMacro(vtkValuePasses);

// ----------------------------------------------------------------------------
vtkValuePasses::vtkValuePasses()
{
  this->Internals = new vtkValuePasses::vtkInternals();
}

// ----------------------------------------------------------------------------
vtkValuePasses::~vtkValuePasses()
{
  delete this->Internals;
}

// ----------------------------------------------------------------------------
void vtkValuePasses::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
void vtkValuePasses::SetInputArrayToProcess(int fieldAssociation, const char *name)
{
  this->Internals->ValuePass->SetInputArrayToProcess(fieldAssociation, name);
}

// ----------------------------------------------------------------------------
void vtkValuePasses::SetInputArrayToProcess(int fieldAssociation, int fieldAttributeType)
{
  this->Internals->ValuePass->SetInputArrayToProcess(fieldAssociation, fieldAttributeType);
}

// ----------------------------------------------------------------------------
void vtkValuePasses::SetInputComponentToProcess(int comp)
{
  this->Internals->ValuePass->SetInputComponentToProcess(comp);
}

// ----------------------------------------------------------------------------
void vtkValuePasses::SetScalarRange(double min, double max)
{
  this->Internals->ValuePass->SetScalarRange(min, max);
}

// ----------------------------------------------------------------------------
// Description:
void vtkValuePasses::Render(const vtkRenderState *s)
{
  this->NumberOfRenderedProps=0;

  this->Internals->SequencePass->Render(s);

  this->NumberOfRenderedProps+=
    this->Internals->SequencePass->GetNumberOfRenderedProps();
}
