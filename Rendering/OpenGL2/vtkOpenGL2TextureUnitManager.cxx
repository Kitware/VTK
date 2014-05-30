/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGL2TextureUnitManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGL2TextureUnitManager.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGL2RenderWindow.h"
#include "vtkOpenGLHardwareSupport.h"

#include <cassert>

vtkStandardNewMacro(vtkOpenGL2TextureUnitManager);

// ----------------------------------------------------------------------------
vtkOpenGL2TextureUnitManager::vtkOpenGL2TextureUnitManager()
{
  this->Context=0;
  this->NumberOfTextureUnits=0;
  this->TextureUnits=0;
}

// ----------------------------------------------------------------------------
vtkOpenGL2TextureUnitManager::~vtkOpenGL2TextureUnitManager()
{
  this->DeleteTable();
  this->Context=0;
}

// ----------------------------------------------------------------------------
// Description:
// Delete the allocation table and check if it is not called before
// all the texture units have been released.
void vtkOpenGL2TextureUnitManager::DeleteTable()
{
  if(this->TextureUnits!=0)
    {
    size_t i=0;
    size_t c=this->NumberOfTextureUnits;
    bool valid=true;
    while(valid && i<c)
      {
      valid=!this->TextureUnits[i];
      ++i;
      }
    if(!valid)
      {
      vtkErrorMacro(<<"the texture unit is deleted but not some texture unit has not been released: Id="<<i);
      }
    delete[] this->TextureUnits;
    this->TextureUnits=0;
    this->NumberOfTextureUnits=0;
    }
}

// ----------------------------------------------------------------------------
void vtkOpenGL2TextureUnitManager::SetContext(vtkOpenGL2RenderWindow *context)
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
      vtkOpenGLHardwareSupport *info=context->GetHardwareSupport();
      this->NumberOfTextureUnits=info->GetNumberOfTextureUnits();
      if(this->NumberOfTextureUnits>0)
        {
        this->TextureUnits=new bool[this->NumberOfTextureUnits];
        size_t i=0;
        size_t c=this->NumberOfTextureUnits;
        while(i<c)
          {
          this->TextureUnits[i]=false;
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
int vtkOpenGL2TextureUnitManager::GetNumberOfTextureUnits()
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
int vtkOpenGL2TextureUnitManager::Allocate()
{
  bool found=false;
  size_t i=0;
  size_t c=this->NumberOfTextureUnits;
  while(!found && i<c)
    {
    found=!this->TextureUnits[i];
    ++i;
    }

  int result;
  if(found)
    {
    result=static_cast<int>(i-1);
    this->TextureUnits[result]=true;
    }
  else
    {
    result=-1;
    }

  assert("post: valid_result" && (result==-1 || (result>=0 && result<this->GetNumberOfTextureUnits())));
  assert("post: allocated" && (result==-1 || this->IsAllocated(result)));
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Tell if texture unit `textureUnitId' is already allocated.
// \pre valid_id_range : textureUnitId>=0 && textureUnitId<this->GetNumberOfTextureUnits()
bool vtkOpenGL2TextureUnitManager::IsAllocated(int textureUnitId)
{
  assert("pre: valid_textureUnitId_range" && textureUnitId>=0 && textureUnitId<this->GetNumberOfTextureUnits());
  return this->TextureUnits[textureUnitId];
}

// ----------------------------------------------------------------------------
// Description:
// Release a texture unit.
// \pre valid_id: textureUnitId>=0 && textureUnitId<this->GetNumberOfTextureUnits()
// \pre allocated_id: this->IsAllocated(textureUnitId)
void vtkOpenGL2TextureUnitManager::Free(int textureUnitId)
{
  assert("pre: valid_textureUnitId" && (textureUnitId>=0 && textureUnitId<this->GetNumberOfTextureUnits()));
  assert("pre: allocated_textureUnitId" && this->IsAllocated(textureUnitId));

  this->TextureUnits[textureUnitId]=false;
}

// ----------------------------------------------------------------------------
void vtkOpenGL2TextureUnitManager::PrintSelf(ostream& os, vtkIndent indent)
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
