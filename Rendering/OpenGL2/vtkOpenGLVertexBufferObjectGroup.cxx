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

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexBufferObjectCache.h"
#include "vtkShaderProgram.h"
#include "vtkViewport.h"

// STL headers
#include <algorithm>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLVertexBufferObjectGroup);

typedef std::map<std::string, vtkOpenGLVertexBufferObject*>::iterator vboIter;

// typedef std::map<const std::string, vtkDataArray *>::iterator arrayIter;
typedef std::map<std::string, std::vector<vtkDataArray*> >::iterator arrayIter;

// ----------------------------------------------------------------------------
vtkOpenGLVertexBufferObjectGroup::vtkOpenGLVertexBufferObjectGroup() = default;

// ----------------------------------------------------------------------------
vtkOpenGLVertexBufferObjectGroup::~vtkOpenGLVertexBufferObjectGroup()
{
  for (vboIter i = this->UsedVBOs.begin(); i != this->UsedVBOs.end(); ++i)
  {
    i->second->Delete();
  }
  this->UsedVBOs.clear();
}

int vtkOpenGLVertexBufferObjectGroup::GetNumberOfComponents(const char* attribute)
{
  vboIter i = this->UsedVBOs.find(attribute);
  if (i != this->UsedVBOs.end())
  {
    return i->second->GetNumberOfComponents();
  }
  return 0;
}

int vtkOpenGLVertexBufferObjectGroup::GetNumberOfTuples(const char* attribute)
{
  vboIter i = this->UsedVBOs.find(attribute);
  if (i != this->UsedVBOs.end())
  {
    return i->second->GetNumberOfTuples();
  }
  return 0;
}

vtkOpenGLVertexBufferObject* vtkOpenGLVertexBufferObjectGroup::GetVBO(const char* attribute)
{
  vboIter i = this->UsedVBOs.find(attribute);
  if (i != this->UsedVBOs.end())
  {
    return i->second;
  }
  return nullptr;
}

void vtkOpenGLVertexBufferObjectGroup::RemoveAttribute(const char* attribute)
{
  // empty array, means delete any existing entries
  arrayIter diter = this->UsedDataArrays.find(attribute);
  if (diter != this->UsedDataArrays.end())
  {
    vboIter viter = this->UsedVBOs.find(attribute);
    if (viter != this->UsedVBOs.end())
    {
      viter->second->UnRegister(this);
      this->UsedVBOs.erase(viter);
      this->Modified();
    }
    std::vector<vtkDataArray*>& vec = diter->second;
    for (size_t j = 0; j < vec.size(); j++)
    {
      if (vec[j])
      {
        vec[j]->Delete();
      }
    }
    diter->second.clear();
    this->UsedDataArrays.erase(diter);

    // rebuild the map for this attribute
    this->UsedDataArrayMaps[attribute].clear();
    std::vector<vtkDataArray*>& arrays = this->UsedDataArrays[attribute];
    vtkIdType totalOffset = 0;
    for (vtkDataArray* arr : arrays)
    {
      this->UsedDataArrayMaps[attribute][arr] = totalOffset;
      totalOffset += arr->GetNumberOfTuples();
    }
    this->UsedDataArraySizes[attribute] = totalOffset;
  }
}

void vtkOpenGLVertexBufferObjectGroup::CacheDataArray(
  const char* attribute, vtkDataArray* da, vtkViewport* vp, int destType)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(vp->GetVTKWindow());
  vtkOpenGLVertexBufferObjectCache* cache = renWin->GetVBOCache();
  this->CacheDataArray(attribute, da, cache, destType);
}

void vtkOpenGLVertexBufferObjectGroup::CacheDataArray(
  const char* attribute, vtkDataArray* da, vtkOpenGLVertexBufferObjectCache* cache, int destType)
{
  // empty array, means delete any existing entries
  if (!da || da->GetNumberOfTuples() == 0)
  {
    this->RemoveAttribute(attribute);
    return;
  }

  // OK we have a data array
  arrayIter diter = this->UsedDataArrays.find(attribute);

  // if a different array is already setup for this attribute
  // clear it out. Remember that UsedArrays is cleared
  // after upload. So if there is another array here
  // it means the same attribute has been set twice.
  // so we delete the prior setting, last one wins.
  if (diter != this->UsedDataArrays.end() && (diter->second.size() != 1 || diter->second[0] != da))
  {
    for (size_t j = 0; j < diter->second.size(); j++)
    {
      if (diter->second[j])
      {
        diter->second[j]->Delete();
      }
    }
    diter->second.clear();
    this->UsedDataArrayMaps[attribute].clear();
  }

  // make sure we add this DA to our list of arrays
  da->Register(this);
  std::vector<vtkDataArray*>& vec = this->UsedDataArrays[attribute];
  vec.push_back(da);
  this->UsedDataArrayMaps[attribute][da] = 0;
  this->UsedDataArraySizes[attribute] = da->GetNumberOfTuples();

  // get the VBO for this DA
  vtkOpenGLVertexBufferObject* vbo = cache->GetVBO(da, destType);
  vboIter viter = this->UsedVBOs.find(attribute);

  // if this VBO is the same as previous for this attribute
  // then just return
  if (viter != this->UsedVBOs.end() && viter->second == vbo)
  {
    vbo->UnRegister(this); // GetVBO increments the ref count
    return;
  }

  this->Modified();

  // if this VBO is different from the prior VBO for this
  // attribute then free the prior VBO
  if (viter != this->UsedVBOs.end())
  {
    viter->second->UnRegister(this);
    this->UsedVBOs.erase(viter);
  }

  // store the VBO and upload
  this->UsedVBOs[attribute] = vbo;
}

// On a composite poly data with N blocks this method
// gets called N times. So it is import that it not
// be order N itself because then the total time
// becomes N*N.  So we maintain a map structure of
// the used data arrays specifically to speed up this
// method.
//
// With a 8196 block dataset this method was consuming
// 75% of the CPU time as N*N. Using the map it now
// consumes only 1.3% of the CPU time.
//
bool vtkOpenGLVertexBufferObjectGroup::ArrayExists(
  const char* attribute, vtkDataArray* da, vtkIdType& offset, vtkIdType& totalOffset)
{
  totalOffset = offset = 0;
  if (!da)
  {
    return true;
  }

  // attribute does not exist
  auto mi = this->UsedDataArrayMaps.find(attribute);
  if (mi == this->UsedDataArrayMaps.end())
  {
    return false;
  }

  // attribute exists
  totalOffset = this->UsedDataArraySizes[attribute];

  // is the da already in it?
  auto di = mi->second.find(da);
  if (di == mi->second.end())
  {
    // no return false
    return false;
  }

  // yes, set the offset and return
  offset = di->second;
  return true;
}

void vtkOpenGLVertexBufferObjectGroup::AppendDataArray(
  const char* attribute, vtkDataArray* da, int destType)
{
  if (!da)
  {
    return;
  }

  std::vector<vtkDataArray*>& arrays = this->UsedDataArrays[attribute];
  da->Register(this);
  arrays.push_back(da);
  this->UsedDataArrayMaps[attribute][da] = this->UsedDataArraySizes[attribute];
  this->UsedDataArraySizes[attribute] += da->GetNumberOfTuples();

  // make sure we have a VBO for this array
  // we do not use the cache when appending
  if (this->UsedVBOs.find(attribute) == this->UsedVBOs.end())
  {
    vtkOpenGLVertexBufferObject* vbo = vtkOpenGLVertexBufferObject::New();
    vbo->SetDataType(destType);
    this->UsedVBOs[attribute] = vbo;
  }
}

void vtkOpenGLVertexBufferObjectGroup::ReleaseGraphicsResources(vtkWindow*)
{
  for (vboIter i = this->UsedVBOs.begin(); i != this->UsedVBOs.end(); ++i)
  {
    i->second->Delete();
  }
  this->UsedVBOs.clear();
}

void vtkOpenGLVertexBufferObjectGroup::AddAllAttributesToVAO(
  vtkShaderProgram* program, vtkOpenGLVertexArrayObject* vao)
{
  for (vboIter i = this->UsedVBOs.begin(); i != this->UsedVBOs.end(); ++i)
  {
    std::string dataShaderName = i->first;
    if (program->IsAttributeUsed(dataShaderName.c_str()))
    {
      vtkOpenGLVertexBufferObject* vbo = i->second;
      if (!vao->AddAttributeArray(program, vbo, dataShaderName,
            0,                                          // offset see assert later in this file
            (vbo->GetDataType() == VTK_UNSIGNED_CHAR))) // TODO: fix tweak. true for colors.
      {
        vtkErrorMacro(<< "Error setting '" << dataShaderName << "' in shader VAO.");
      }
    }
  }
}

void vtkOpenGLVertexBufferObjectGroup::ClearAllDataArrays()
{
  for (arrayIter i = this->UsedDataArrays.begin(); i != this->UsedDataArrays.end(); ++i)
  {
    std::vector<vtkDataArray*>& vec = i->second;
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
  this->UsedDataArrayMaps.clear();
  this->UsedDataArraySizes.clear();
}

void vtkOpenGLVertexBufferObjectGroup::ClearAllVBOs()
{
  for (vboIter i = this->UsedVBOs.begin(); i != this->UsedVBOs.end(); ++i)
  {
    i->second->Delete();
  }
  this->UsedVBOs.clear();
}

void vtkOpenGLVertexBufferObjectGroup::BuildAllVBOs(vtkViewport* vp)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(vp->GetVTKWindow());
  vtkOpenGLVertexBufferObjectCache* cache = renWin->GetVBOCache();
  this->BuildAllVBOs(cache);
}

// ----------------------------------------------------------------------------
void vtkOpenGLVertexBufferObjectGroup::BuildAllVBOs(vtkOpenGLVertexBufferObjectCache*)
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

  // we always upload appended data :-(
  for (arrayIter i = this->UsedDataArrays.begin(); i != this->UsedDataArrays.end(); ++i)
  {
    std::string attribute = i->first;
    std::vector<vtkDataArray*>& vec = i->second;
    vtkOpenGLVertexBufferObject* vbo = this->UsedVBOs[attribute];

    if (vec.size() > 1)
    {
      for (size_t j = 0; j < vec.size(); j++)
      {
        // only append if needed?
        vbo->AppendDataArray(vec[j]);
      }
      vbo->UploadVBO();
    }
  }

  // for everything else we upload based on mtimes
  for (arrayIter i = this->UsedDataArrays.begin(); i != this->UsedDataArrays.end(); ++i)
  {
    std::string attribute = i->first;
    std::vector<vtkDataArray*>& vec = i->second;
    vtkOpenGLVertexBufferObject* vbo = this->UsedVBOs[attribute];

    if (vec.size() == 1 && vec[0]->GetMTime() > vbo->GetUploadTime())
    {
      vbo->UploadDataArray(vec[0]);
    }
  }

  // Upload updated VBOs
  for (vboIter i = this->UsedVBOs.begin(); i != this->UsedVBOs.end(); ++i)
  {
    vtkOpenGLVertexBufferObject* vbo = i->second;
    if (vbo->GetMTime() > vbo->GetUploadTime())
    {
      vbo->UploadVBO();
    }
  }

  this->ClearAllDataArrays();
}

//----------------------------------------------------------------------------
vtkMTimeType vtkOpenGLVertexBufferObjectGroup::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();

  for (vboIter i = this->UsedVBOs.begin(); i != this->UsedVBOs.end(); ++i)
  {
    vtkMTimeType time = i->second->GetMTime();
    if (time > mTime)
    {
      mTime = time;
    }
  }

  return mTime;
}

// ----------------------------------------------------------------------------
void vtkOpenGLVertexBufferObjectGroup::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
