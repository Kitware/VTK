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
#include "vtkObjectFactory.h"

#include "vtkActor.h"
#include "vtkCompositePainter.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkPainterPolyDataMapper.h"
#include "vtkProp.h"
#include "vtkRenderer.h"
#include "vtkRenderState.h"
#include "vtkSmartPointer.h"
#include "vtkValuePainter.h"

#include "vtkPolyData.h"

// ----------------------------------------------------------------------------
class vtkValuePass::vtkInternals
{
public:
  vtkNew<vtkCompositePainter> CompositePainter;
  vtkNew<vtkValuePainter> ValuePainter;

  vtkInternals()
  {
    this->CompositePainter->SetDelegatePainter(this->ValuePainter.GetPointer());
  }

  ~vtkInternals()
  {
  }
};

// ============================================================================
vtkStandardNewMacro(vtkValuePass);

// ----------------------------------------------------------------------------
vtkValuePass::vtkValuePass()
{
  this->Internals = new vtkValuePass::vtkInternals();
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
void vtkValuePass::SetInputArrayToProcess(int fieldAssociation, const char *name)
{
  this->Internals->ValuePainter->SetInputArrayToProcess(fieldAssociation, name);
}

// ----------------------------------------------------------------------------
void vtkValuePass::SetInputArrayToProcess(int fieldAssociation, int fieldAttributeType)
{
  this->Internals->ValuePainter->SetInputArrayToProcess(fieldAssociation, fieldAttributeType);
}

// ----------------------------------------------------------------------------
void vtkValuePass::SetInputComponentToProcess(int comp)
{
  this->Internals->ValuePainter->SetInputComponentToProcess(comp);
}

// ----------------------------------------------------------------------------
void vtkValuePass::SetScalarRange(double min, double max)
{
  this->Internals->ValuePainter->SetScalarRange(min, max);
}

// ----------------------------------------------------------------------------
// Description:
// renders geometry in direct value mode
void vtkValuePass::Render(const vtkRenderState *s)
{
  this->NumberOfRenderedProps=0;

  int c=s->GetPropArrayCount();
  int i=0;
  while(i<c)
    {
    vtkProp *p=s->GetPropArray()[i];
    vtkActor *actor = vtkActor::SafeDownCast(p);
    if (actor)
      {
      vtkPainterPolyDataMapper *mapper = vtkPainterPolyDataMapper::SafeDownCast(actor->GetMapper());
      if (mapper)
        {
        //backup
        vtkPainter *oldP = mapper->GetPainter();
        if (oldP)
          {
          oldP->Register(NULL);
          }

        //swap in
        mapper->SetPainter(this->Internals->CompositePainter.GetPointer());
        vtkInformation *iv = this->Internals->CompositePainter->GetInformation();
        vtkInformation *akeys = actor->GetPropertyKeys();
        if (iv && akeys)
          {
          if (akeys->Has(vtkValuePainter::SCALAR_MODE()))
            {
            iv->Set(vtkValuePainter::SCALAR_MODE(), akeys->Get(vtkValuePainter::SCALAR_MODE()));
            }
          if (akeys->Has(vtkValuePainter::ARRAY_NAME()))
            {
            iv->Set(vtkValuePainter::ARRAY_NAME(), akeys->Get(vtkValuePainter::ARRAY_NAME()));
            }
          if (akeys->Has(vtkValuePainter::ARRAY_ID()))
            {
            iv->Set(vtkValuePainter::ARRAY_ID(), akeys->Get(vtkValuePainter::ARRAY_ID()));
            }
          if (akeys->Has(vtkValuePainter::ARRAY_COMPONENT()))
            {
            iv->Set(vtkValuePainter::ARRAY_COMPONENT(), akeys->Get(vtkValuePainter::ARRAY_COMPONENT()));
            }
          }

        //render
        int rendered=
          p->RenderFilteredOpaqueGeometry(s->GetRenderer(),s->GetRequiredKeys());
        this->NumberOfRenderedProps+=rendered;

        //restore
        mapper->SetPainter(oldP);
        if (oldP)
          {
          oldP->UnRegister(NULL);
          }
        }
      }
    ++i;
    }
}
