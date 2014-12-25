/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureUnitManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtk_glew.h"
#include "vtkTextureUnitManager.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"

#include <cassert>

vtkStandardNewMacro(vtkTextureUnitManager);

// ----------------------------------------------------------------------------
vtkTextureUnitManager::vtkTextureUnitManager()
{
  this->Context=0;
  this->NumberOfTextureUnits=0;
  this->TextureUnits=0;
}

// ----------------------------------------------------------------------------
vtkTextureUnitManager::~vtkTextureUnitManager()
{
  this->DeleteTable();
  this->Context=0;
}

// ----------------------------------------------------------------------------
// Description:
// Delete the allocation table and check if it is not called before
// all the texture units have been released.
void vtkTextureUnitManager::DeleteTable()
{
  if(this->TextureUnits!=0)
    {
    size_t i=0;
    size_t c=this->NumberOfTextureUnits;
    bool valid=true;
    while(valid && i<c)
      {
      valid = this->TextureUnits[i] > 0 ? false : true;
      ++i;
      }
    if(!valid)
      {
      vtkErrorMacro(<<"the texture unit is deleted but some texture units have not been released: Id="<<i);
      }
    delete[] this->TextureUnits;
    this->TextureUnits=0;
    this->NumberOfTextureUnits=0;
    }
}

// ----------------------------------------------------------------------------
void vtkTextureUnitManager::SetContext(vtkOpenGLRenderWindow *context)
{
  if(this->Context!=context)
    {
    if(this->Context!=0)
      {
      this->DeleteTable();
      }
    this->Context=context;
    if(this->Context!=0)
      {
      glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &this->NumberOfTextureUnits);
      if(this->NumberOfTextureUnits > 0)
        {
        this->TextureUnits = new unsigned int[this->NumberOfTextureUnits];
        size_t i=0;
        size_t c=this->NumberOfTextureUnits;
        while(i<c)
          {
          this->TextureUnits[i]=0;
          ++i;
          }
        }
      }
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
// Description:
// Number of texture units supported by the OpenGL context.
int vtkTextureUnitManager::GetNumberOfTextureUnits()
{
  return this->NumberOfTextureUnits;
}

// ----------------------------------------------------------------------------
// Description:
// Reserve a texture unit. It returns its number.
// It returns -1 if the allocation failed (because there is no more
// texture unit left).
// \post valid_result: result==-1 || result>=0 && result<this->GetNumberOfTextureUnits())
// \post allocated: result==-1 || this->IsAllocated(result)
int vtkTextureUnitManager::Allocate()
{
  bool found=false;
  size_t i=0;
  size_t c=this->NumberOfTextureUnits;
  while(!found && i<c)
    {
    found = (this->TextureUnits[i] > 0 ? false : true);
    ++i;
    }

  int result;
  if(found)
    {
    result=static_cast<int>(i-1);
    this->TextureUnits[result] += 1;
    }
  else
    {
    int leastUsed = 0;
    for (int j = 0; j < this->NumberOfTextureUnits; ++j)
      {
      if (this->TextureUnits[j] < this->TextureUnits[leastUsed])
        {
        leastUsed = j;
        }
      }
    this->TextureUnits[leastUsed] += 1;
    result = leastUsed;
    }

  assert("post: valid_result" && (result==-1 || (result>=0 && result<this->GetNumberOfTextureUnits())));
  assert("post: allocated" && (result==-1 || this->IsAllocated(result)));
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Tell if texture unit `textureUnitId' is already allocated.
// \pre valid_id_range : textureUnitId>=0 && textureUnitId<this->GetNumberOfTextureUnits()
bool vtkTextureUnitManager::IsAllocated(int textureUnitId)
{
  assert("pre: valid_textureUnitId_range" && textureUnitId>=0 && textureUnitId<this->GetNumberOfTextureUnits());
  return (this->TextureUnits[textureUnitId] > 0 ? true : false);
}

// ----------------------------------------------------------------------------
// Description:
// Release a texture unit.
// \pre valid_id: textureUnitId>=0 && textureUnitId<this->GetNumberOfTextureUnits()
// \pre allocated_id: this->IsAllocated(textureUnitId)
void vtkTextureUnitManager::Free(int textureUnitId)
{
  assert("pre: valid_textureUnitId" && (textureUnitId>=0 && textureUnitId<this->GetNumberOfTextureUnits()));
//  assert("pre: allocated_textureUnitId" && this->IsAllocated(textureUnitId));

  if (this->TextureUnits[textureUnitId] >= 1)
    {
    this->TextureUnits[textureUnitId] -= 1;
    }
  else
    {
    this->TextureUnits[textureUnitId] = 0;
    }
}

// ----------------------------------------------------------------------------
void vtkTextureUnitManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Context: ";
  if(this->Context!=0)
    {
    os << static_cast<void *>(this->Context) <<endl;
    }
  else
    {
    os << "none" << endl;
    }
}
