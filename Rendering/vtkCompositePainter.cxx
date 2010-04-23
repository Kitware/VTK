/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositePainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositePainter.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkObjectFactory.h"
#include "vtkPainterDeviceAdapter.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkCellData.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
# include "vtkOpenGL.h"
#endif
vtkStandardNewMacro(vtkCompositePainter);
vtkInformationKeyMacro(vtkCompositePainter, COLOR_LEAVES, Integer);
//----------------------------------------------------------------------------
vtkCompositePainter::vtkCompositePainter()
{
  this->ColorLeaves = 0;
  this->OutputData = 0;
}

//----------------------------------------------------------------------------
vtkCompositePainter::~vtkCompositePainter()
{
}

//----------------------------------------------------------------------------
void vtkCompositePainter::ProcessInformation(vtkInformation* info)
{
  this->Superclass::ProcessInformation(info);
  if (info->Has(COLOR_LEAVES()))
    {
    this->SetColorLeaves(info->Get(COLOR_LEAVES()));
    }
  else
    {
    this->SetColorLeaves(0);
    }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCompositePainter::GetOutput()
{
  return this->OutputData? this->OutputData : this->GetInput();
}

//----------------------------------------------------------------------------
void vtkCompositePainter::RenderInternal(vtkRenderer* renderer,
                                         vtkActor* actor,
                                         unsigned long typeflags,
                                         bool forceCompileOnly)
{
  vtkPainterDeviceAdapter* device = renderer->GetRenderWindow()->
    GetPainterDeviceAdapter();

  vtkCompositeDataSet* input = vtkCompositeDataSet::SafeDownCast(this->GetInput());
  if (!input || !this->DelegatePainter)
    {
      this->Superclass::RenderInternal(renderer, actor, typeflags,
                                       forceCompileOnly);
    return;
    }

  //turn off antialising and lighting so that the colors we draw will be the
  //colors we read back
  int origMultisample = device->QueryMultisampling();
  int origLighting = device->QueryLighting();
  int origBlending = device->QueryBlending();

  if (this->ColorLeaves)
    {
    device->MakeMultisampling(0);
    device->MakeLighting(0);
    device->MakeBlending(0);
    }

  vtkCompositeDataIterator* iter = input->NewIterator();
  unsigned int index=1; // start from 1 since 0 cannot be a color.
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
    iter->GoToNextItem(), index++)
    {
    vtkDataObject* dobj = iter->GetCurrentDataObject();
    if (dobj)
      {
      if (this->ColorLeaves)
        {
        unsigned char params[4] = {255, 255, 0, 255};
        params[0] = static_cast<unsigned char>(index & 0x0000ff);
        params[1] = static_cast<unsigned char>((index & 0x00ff00)>>8);
        params[2] = static_cast<unsigned char>((index & 0xff0000)>>16);
        params[3] = 255;
        float color[4];
        color[0] = params[0]/255.0;
        color[1] = params[1]/255.0;
        color[2] = params[2]/255.0;
        color[3] = params[3]/255.0;
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
        }

      this->DelegatePainter->SetInput(dobj);
      this->OutputData = dobj;
      this->Superclass::RenderInternal(renderer, actor, typeflags,
                                       forceCompileOnly);
      this->OutputData = 0;
      }
    }
  iter->Delete();

  if (this->ColorLeaves)
    {
    //reset lighting back to the default
    device->MakeBlending(origBlending);
    device->MakeLighting(origLighting);
    device->MakeMultisampling(origMultisample);
    }
}

//-----------------------------------------------------------------------------
void vtkCompositePainter::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);

  vtkGarbageCollectorReport(collector, this->OutputData, "Output");
}


//----------------------------------------------------------------------------
void vtkCompositePainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ColorLeaves: " << this->ColorLeaves << endl;
}

