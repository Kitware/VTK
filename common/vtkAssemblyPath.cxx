/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssemblyPath.cxx
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
#include "vtkAssemblyPath.h"
#include "vtkAssemblyNode.h"
#include "vtkTransform.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkAssemblyPath* vtkAssemblyPath::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkAssemblyPath");
  if(ret)
    {
    return (vtkAssemblyPath*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkAssemblyPath;
}

vtkAssemblyPath::vtkAssemblyPath()
{
  this->Transform = vtkTransform::New();
  this->Transform->PreMultiply();  
  this->TransformedProp = NULL;
}

vtkAssemblyPath::~vtkAssemblyPath()
{
  this->Transform->Delete();
  if ( this->TransformedProp != NULL )
    {
    this->TransformedProp->Delete();
    }
}

void vtkAssemblyPath::AddNode(vtkProp *p, vtkMatrix4x4 *m)
{
  vtkAssemblyNode *n = vtkAssemblyNode::New();
  n->SetProp(p);
  n->SetMatrix(m); //really a copy because we're gonna compute with it
  this->AddNode(n);
  n->Delete(); //ok reference counted
}

void vtkAssemblyPath::AddNode(vtkAssemblyNode *n) 
{
  // First add the node to the list
  this->vtkCollection::AddItem((vtkObject *)n);
  
  // Grab the matrix, if any, and concatenate it
  this->Transform->Push(); //keep in synch with list of nodes
  vtkMatrix4x4 *matrix;
  if ( (matrix = n->GetMatrix()) != NULL )
    {
    this->Transform->Concatenate(matrix);
    this->Transform->GetMatrix(matrix); //replace previous matrix
    }
}

vtkAssemblyNode *vtkAssemblyPath::GetNextNode() 
{ 
  return (vtkAssemblyNode *)(this->GetNextItemAsObject());
}

vtkAssemblyNode *vtkAssemblyPath::GetFirstNode() 
{ 
  if ( this->Top == NULL )
    {
    return NULL;
    }
  else
    {
    return (vtkAssemblyNode *)(this->Top->Item);
    }
}

vtkAssemblyNode *vtkAssemblyPath::GetLastNode() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return (vtkAssemblyNode *)(this->Bottom->Item);
    }
}

void vtkAssemblyPath::DeleteLastNode()
{
  vtkAssemblyNode *node = this->GetLastNode();
  this->vtkCollection::RemoveItem(node);
  
  this->Transform->Pop();
}

void vtkAssemblyPath::ShallowCopy(vtkAssemblyPath *path)
{
  vtkAssemblyNode *node;

  this->RemoveAllItems();
  for ( path->InitTraversal(); (node = path->GetNextNode()); )
    {
    this->vtkCollection::AddItem((vtkObject *)node);
    }
}

unsigned long vtkAssemblyPath::GetMTime()
{
  unsigned long mtime=this->vtkCollection::GetMTime();
  unsigned long nodeMTime;
  vtkAssemblyNode *node;
  
  for ( this->InitTraversal(); (node = this->GetNextNode()); )
    {
    nodeMTime = node->GetMTime();
    if ( nodeMTime > mtime )
      {
      mtime = nodeMTime;
      }
    }
  return mtime;
}

void vtkAssemblyPath::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkCollection::PrintSelf(os,indent);
}


