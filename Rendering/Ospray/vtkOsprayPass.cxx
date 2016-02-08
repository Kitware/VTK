/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOsprayPass.h"
#include "vtkObjectFactory.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkOsprayWindowNode.h"


// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOsprayPass);

// ----------------------------------------------------------------------------
vtkOsprayPass::vtkOsprayPass()
{
  this->SceneGraph = NULL;
}

// ----------------------------------------------------------------------------
vtkOsprayPass::~vtkOsprayPass()
{
  this->SetSceneGraph(NULL);
}

// ----------------------------------------------------------------------------
void vtkOsprayPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkOsprayPass, SceneGraph, vtkOsprayWindowNode)

// ----------------------------------------------------------------------------
void vtkOsprayPass::Render(const vtkRenderState *s)
{
  (void)s;
  this->NumberOfRenderedProps=0;

  if (this->SceneGraph)
    {
    this->SceneGraph->Build();
    this->SceneGraph->Synchronize();
    this->SceneGraph->Render();
    this->SceneGraph->PostRender();
    }
}
