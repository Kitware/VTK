/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVertexBufferObjectGroup.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLVertexBufferObjectGroup.h"

#include <cassert>

#include "vtkObjectFactory.h"
#include "vtkDataArray.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexBufferObjectCache.h"
#include "vtkViewport.h"
#include "vtkShaderProgram.h"

// STL headers
#include <algorithm>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLVertexBufferObjectGroup);

typedef std::map<std::string, vtkOpenGLVertexBufferObject *>::iterator vboIter;

// typedef std::map<const std::string, vtkDataArray *>::iterator arrayIter;
typedef std::map<std::string, std::vector<vtkDataArray *> >::iterator arrayIter;

// ----------------------------------------------------------------------------
vtkOpenGLVertexBufferObjectGroup::vtkOpenGLVertexBufferObjectGroup()
{
}

// ----------------------------------------------------------------------------
vtkOpenGLVertexBufferObjectGroup::~vtkOpenGLVertexBufferObjectGroup()
{
  for (vboIter i = this->UsedVBOs.begin(); i != this->UsedVBOs.end(); ++i)
  {
    i->second->Delete();
  }
  this->UsedVBOs.clear();
}

int vtkOpenGLVertexBufferObjectGroup::GetNumberOfComponents(
  const char *attribute)
{
  vboIter i = this->UsedVBOs.find(attribute);
  if (i != this->UsedVBOs.end())
  {
    return i->second->NumberOfComponents;
  }
  return 0;
}

int vtkOpenGLVertexBufferObjectGroup::GetNumberOfTuples(
  const char *attribute)
{
  vboIter i = this->UsedVBOs.find(attribute);
  if (i != this->UsedVBOs.end())
  {
    return i->second->NumberOfTuples;
  }
  return 0;
}

vtkOpenGLVertexBufferObject *vtkOpenGLVertexBufferObjectGroup::GetVBO(
  const char *attribute)
{
  vboIter i = this->UsedVBOs.find(attribute);
  if (i != this->UsedVBOs.end())
  {
    return i->second;
  }
  return NULL;
}

void vtkOpenGLVertexBufferObjectGroup::CacheDataArray(
  const char *attribute,
  vtkDataArray *da,
  vtkViewport *vp,
  int destType)
{
  vtkOpenGLRenderWindow *renWin =
    vtkOpenGLRenderWindow::SafeDownCast(vp->GetVTKWindow());
  vtkOpenGLVertexBufferObjectCache *cache = renWin->GetVBOCache();
  this->CacheDataArray(attribute, da, cache, destType);
}

void vtkOpenGLVertexBufferObjectGroup::CacheDataArray(
  const char *attribute,
  vtkDataArray *da,
  vtkOpenGLVertexBufferObjectCache *cache,
  int destType)
{
  // empty array, clear out entry if needed
  arrayIter diter = this->UsedDataArrays.find(attribute);
  if (!da || da->GetNumberOfTuples() == 0)
  {
    if (diter != this->UsedDataArrays.end())
    {
      vboIter viter = this->UsedVBOs.find(attribute);
      if (viter != this->UsedVBOs.end())
      {
        viter->second->UnRegister(this);
        this->UsedVBOs.erase(viter);
      }
      std::vector<vtkDataArray *> &vec = diter->second;
      for (size_t j = 0; j < vec.size(); j++)
      {
        if (vec[j])
        {
          vec[j]->Delete();
        }
      }
      diter->second.clear();
      this->UsedDataArrays.erase(diter);
    }
    return;
  }

  // non null data array
  // if anything changed in VBO shape then recreate
  std::vector<vtkDataArray *> &arrays = this->UsedDataArrays[attribute];
  vboIter viter = this->UsedVBOs.find(attribute);
  if (arrays.size() != 1 || arrays[0] != da
      || viter == this->UsedVBOs.end()
      || viter->second->NumberOfComponents !=
        static_cast<unsigned int>(da->GetNumberOfComponents())
      || viter->second->DataType != destType
      || viter->second->NumberOfTuples !=
        static_cast<unsigned int>(da->GetNumberOfTuples()))
  {
    if (viter != this->UsedVBOs.end())
    {
      viter->second->UnRegister(this);
      this->UsedVBOs.erase(viter);
    }
    for (size_t j = 0; j < arrays.size(); j++)
    {
      if (arrays[j])
      {
        arrays[j]->Delete();
      }
    }
    arrays.clear();
    da->Register(this);
    arrays.push_back(da);

    // Get VBO for used array
    vtkOpenGLVertexBufferObject* vbo = cache->GetVBO(da, destType);

    // Unregister former VBO
    this->UsedVBOs[attribute] = vbo;
  }
}

void vtkOpenGLVertexBufferObjectGroup::AppendDataArray(
  const char *attribute,
  vtkDataArray *da,
  int destType)
{
  if (!da)
  {
    return;
  }

  std::vector<vtkDataArray *> &arrays = this->UsedDataArrays[attribute];
  if (std::find(arrays.begin(), arrays.end(), da) == arrays.end())
  {
    da->Register(this);
    arrays.push_back(da);

    // make sure we have a VBO for this array
    if (this->UsedVBOs.find(attribute) == this->UsedVBOs.end())
    {
      vtkOpenGLVertexBufferObject* vbo = vtkOpenGLVertexBufferObject::New();
      vbo->InitVBO(da, destType);
      this->UsedVBOs[attribute] = vbo;
    }
  }
}

void vtkOpenGLVertexBufferObjectGroup::ReleaseGraphicsResources(vtkWindow *)
{
  for (vboIter i = this->UsedVBOs.begin(); i != this->UsedVBOs.end(); ++i)
  {
    i->second->Delete();
  }
  this->UsedVBOs.clear();
}

void vtkOpenGLVertexBufferObjectGroup::AddAllAttributesToVAO(
  vtkShaderProgram *program,
  vtkOpenGLVertexArrayObject *vao)
{
  for (vboIter i = this->UsedVBOs.begin(); i != this->UsedVBOs.end(); ++i)
  {
    std::string dataShaderName = i->first;
    if (program->IsAttributeUsed(dataShaderName.c_str()))
    {
      vtkOpenGLVertexBufferObject* vbo = i->second;
      if (!vao->AddAttributeArray(
        program, vbo, dataShaderName,
        0, // offset see assert later in this file
        (vbo->DataType == VTK_UNSIGNED_CHAR))) //TODO: fix tweak. true for colors.
      {
        vtkErrorMacro(<< "Error setting '" << dataShaderName << "' in shader VAO.");
      }
    }
  }
}

void vtkOpenGLVertexBufferObjectGroup::ClearAllDataArrays()
{
  for (arrayIter i = this->UsedDataArrays.begin();
       i != this->UsedDataArrays.end(); ++i)
  {
    std::vector<vtkDataArray *> &vec = i->second;
    for (size_t j = 0; j < vec.size(); j++)
    {
      if (vec[j])
      {
        vec[j]->Delete();
      }
    }
    i->second.clear();
  }
  this->UsedDataArrays.clear();
}

void vtkOpenGLVertexBufferObjectGroup::ClearAllVBOs()
{
  for (vboIter i = this->UsedVBOs.begin(); i != this->UsedVBOs.end(); ++i)
  {
    i->second->Delete();
  }
  this->UsedVBOs.clear();
}

void vtkOpenGLVertexBufferObjectGroup::BuildAllVBOs(vtkViewport *vp)
{
  vtkOpenGLRenderWindow *renWin =
    vtkOpenGLRenderWindow::SafeDownCast(vp->GetVTKWindow());
  vtkOpenGLVertexBufferObjectCache *cache = renWin->GetVBOCache();
  this->BuildAllVBOs(cache);
}

// ----------------------------------------------------------------------------
void vtkOpenGLVertexBufferObjectGroup::BuildAllVBOs(
  vtkOpenGLVertexBufferObjectCache *)
{
  // free any VBOs for unused attributes
  for (vboIter i = this->UsedVBOs.begin(); i != this->UsedVBOs.end();)
  {
    if (this->UsedDataArrays.find(i->first) == this->UsedDataArrays.end())
    {
      i->second->UnRegister(this);
      vboIter toErase = i;
      ++i;
      this->UsedVBOs.erase(toErase);
    }
    else
    {
      ++i;
    }
  }

  // now push data to VBOs as needed
  for (arrayIter i = this->UsedDataArrays.begin();
       i != this->UsedDataArrays.end(); ++i)
  {
    std::string attribute = i->first;
    std::vector<vtkDataArray *> &vec = i->second;
    vtkOpenGLVertexBufferObject *vbo = this->UsedVBOs[attribute];

    // append arrays if vbo empty
    if (vbo->NumberOfTuples == 0)
    {
      if (vec.size() == 1)
      {
        vbo->UploadDataArray(vec[0]);
      }
      else
      {
        for (size_t j = 0; j < vec.size(); j++)
        {
          // only append if needed?
          vbo->AppendDataArray(vec[j]);
        }
        vbo->UploadVBO();
      }
    }
  }

  // Upload updated VBOs
  for (vboIter i = this->UsedVBOs.begin(); i != this->UsedVBOs.end(); ++i)
  {
    vtkOpenGLVertexBufferObject* vbo = i->second;
    if(vbo->GetMTime() > vbo->GetUploadTime())
    {
      vbo->UploadVBO();
    }
  }

  this->ClearAllDataArrays();
}

// ----------------------------------------------------------------------------
void vtkOpenGLVertexBufferObjectGroup::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
