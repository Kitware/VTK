/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkProp.h"
#include "vtkObjectFactory.h"
#include "vtkAssemblyPaths.h"
#include "vtkCommand.h"


//----------------------------------------------------------------------------
vtkProp* vtkProp::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkProp");
  if(ret)
    {
    return (vtkProp*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkProp;
}

// Creates an Prop with the following defaults: visibility on.
vtkProp::vtkProp()
{
  this->Visibility = 1;  // ON

  this->Pickable   = 1;
  this->PickTag = 0;
  this->Dragable   = 1;
  
  this->AllocatedRenderTime = 10.0;
  this->EstimatedRenderTime = 0.0;
  this->RenderTimeMultiplier = 1.0;

  this->Paths = NULL;
}

vtkProp::~vtkProp()
{
  if ( this->Paths )
    {
    this->Paths->Delete();
    }
}

// This method is invoked when an instance of vtkProp (or subclass, 
// e.g., vtkActor) is picked by vtkPicker.
void vtkProp::SetPickMethod(void (*f)(void *), void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->PickTag);
  this->PickTag = this->AddObserver(vtkCommand::PickEvent,cbc);
}

// Set a method to delete user arguments for PickMethod.
void vtkProp::SetPickMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->PickTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

// This method is invoked if the prop is picked.
void vtkProp::Pick()
{
  this->InvokeEvent(vtkCommand::PickEvent,NULL);
}

// Shallow copy of vtkProp.
void vtkProp::ShallowCopy(vtkProp *prop)
{
  this->Visibility = prop->GetVisibility();
  this->Pickable   = prop->GetPickable();
  this->Dragable   = prop->GetDragable();
}

void vtkProp::InitPathTraversal()
{
  if ( this->Paths == NULL )
    {
    this->Paths = vtkAssemblyPaths::New();
    vtkAssemblyPath *path = vtkAssemblyPath::New();
    path->AddNode(this,NULL);
    this->BuildPaths(this->Paths,path);
    path->Delete();
    }
  this->Paths->InitTraversal();
}

vtkAssemblyPath *vtkProp::GetNextPath()
{
  if ( ! this->Paths)
    {
    return NULL;
    }
  return this->Paths->GetNextItem();
}

// This method is used in conjunction with the assembly object to build a copy
// of the assembly hierarchy. This hierarchy can then be traversed for 
// rendering, picking or other operations.
void vtkProp::BuildPaths(vtkAssemblyPaths *paths, vtkAssemblyPath *path)
{
  // This is a leaf node in the assembly hierarchy so we
  // copy the path in preparation to assingning it to paths.
  vtkAssemblyPath *childPath = vtkAssemblyPath::New();
  childPath->ShallowCopy(path);

  // We can add this path to the list of paths
  paths->AddItem(childPath);
  childPath->Delete(); //okay, reference counting
}

void vtkProp::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent);

  os << indent << "Dragable: " << (this->Dragable ? "On\n" : "Off\n");
  os << indent << "Pickable: " << (this->Pickable ? "On\n" : "Off\n");

  os << indent << "AllocatedRenderTime: " 
     << this->AllocatedRenderTime << endl;
  os << indent << "EstimatedRenderTime: " 
     << this->EstimatedRenderTime << endl;
  os << indent << "RenderTimeMultiplier: " 
     << this->RenderTimeMultiplier << endl;
  os << indent << "Visibility: " << (this->Visibility ? "On\n" : "Off\n");
}




