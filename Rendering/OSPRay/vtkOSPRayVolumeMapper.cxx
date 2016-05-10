/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayVolumeMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOSPRayVolumeMapper.h"

#include "vtkObjectFactory.h"
#include "vtkOSPRayPass.h"
#include "vtkSmartPointer.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"

vtkStandardNewMacro(vtkOSPRayVolumeMapper)

// ----------------------------------------------------------------------------
vtkOSPRayVolumeMapper::vtkOSPRayVolumeMapper()
{
}

// ----------------------------------------------------------------------------
vtkOSPRayVolumeMapper::~vtkOSPRayVolumeMapper()
{
}

// ----------------------------------------------------------------------------
void vtkOSPRayVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
void vtkOSPRayVolumeMapper::Render(vtkRenderer *ren,
                             vtkVolume *vol)
{
  vtkRenderPass* oldPass = ren->GetPass();
  vtkSmartPointer<vtkRenderer> tmpRen = vtkSmartPointer<vtkRenderer>::New();
  tmpRen->SetRenderWindow(ren->GetRenderWindow());
  tmpRen->SetActiveCamera(ren->GetActiveCamera());
  tmpRen->SetBackground(ren->GetBackground());
  tmpRen->AddVolume(vol);
  vtkSmartPointer<vtkOSPRayPass> ospray=vtkSmartPointer<vtkOSPRayPass>::New();
  tmpRen->SetPass(ospray);
  tmpRen->Render();
  tmpRen->SetErase(0);
}
