/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkADIOSDirTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <map>
#include <stdexcept>

#include "vtkADIOSDirTree.h"

//----------------------------------------------------------------------------
namespace {
void Tokenize(const std::string& str, std::vector<std::string> &tok,
  char d = '/')
{
  tok.clear();
  if(str.empty())
    {
    return;
    }

  size_t posPrev;
  for(posPrev = -1; str[posPrev+1] == d; ++posPrev); // Ignore leading delims
  size_t posCur;
  while((posCur = str.find(d, posPrev+1)) != std::string::npos)
    {
      if(posCur - posPrev > 1) // Only acknowledge non-empty path components
        {
        tok.push_back(str.substr(posPrev+1, posCur-posPrev-1));
        }
      posPrev = posCur;
    }

  if(posPrev != str.size()-1) // Only add teh last component if it's non-empty
    {
    tok.push_back(str.substr(posPrev+1, str.size()-posPrev-1));
    }
}
}

//----------------------------------------------------------------------------
void vtkADIOSDirTree::PrintSelf(std::ostream& os, vtkIndent indent) const
{
  vtkIndent indent2 = indent.GetNextIndent();

  os << indent << '"' << this->GetName() << '"' << std::endl;
  std::map<std::string, const ADIOS::Scalar*>::const_iterator s;
  for(s = this->Scalars.begin(); s != this->Scalars.end(); ++s)
    {
    os << indent2 << "S: " << s->first << std::endl;
    }

  std::map<std::string, const ADIOS::VarInfo*>::const_iterator a;
  for(a = this->Arrays.begin(); a != this->Arrays.end(); ++a)
    {
    os << indent2 << "A: " << a->first << std::endl;
    }

  std::map<std::string, vtkADIOSDirTree*>::const_iterator d;
  for(d = this->SubDirs.begin(); d != this->SubDirs.end(); ++d)
    {
    d->second->PrintSelf(os, indent2);
    }
}

//----------------------------------------------------------------------------
vtkADIOSDirTree::vtkADIOSDirTree(const std::string& name)
: Name(name)
{
}

//----------------------------------------------------------------------------
vtkADIOSDirTree::vtkADIOSDirTree(const ADIOS::Reader &reader)
: Name("")
{
  // Populate scalars
  std::vector<const ADIOS::Scalar*>::const_iterator s;
  const std::vector<const ADIOS::Scalar*>& scalars = reader.GetScalars();
  for(s = scalars.begin(); s != scalars.end(); ++s)
    {
    std::vector<std::string> path;
    Tokenize((*s)->GetName(), path);

    vtkADIOSDirTree *d = this->BuildPath(path, 0, path.size()-1);
    d->Scalars[*path.rbegin()] = *s;
    const_cast<ADIOS::Scalar*>(*s)->SetName(*path.rbegin());
    }

  // Populate arrays
  std::vector<const ADIOS::VarInfo*>::const_iterator a;
  const std::vector<const ADIOS::VarInfo*>& arrays = reader.GetArrays();
  for(a = arrays.begin(); a != arrays.end(); ++a)
    {
    std::vector<std::string> path;
    Tokenize((*a)->GetName(), path);

    vtkADIOSDirTree *d = this->BuildPath(path, 0, path.size()-1);
    d->Arrays[*path.rbegin()] = *a;
    const_cast<ADIOS::VarInfo*>(*a)->SetName(*path.rbegin());
    }
}

//----------------------------------------------------------------------------
vtkADIOSDirTree::~vtkADIOSDirTree()
{
  std::map<std::string, vtkADIOSDirTree*>::iterator d;
  for(d = this->SubDirs.begin(); d != this->SubDirs.end(); ++d)
    {
    delete d->second;
    }
}

//----------------------------------------------------------------------------
const vtkADIOSDirTree* vtkADIOSDirTree::GetDir(
  const std::string& dirName) const
{
  std::vector<std::string> path;
  Tokenize(dirName, path);

  return path.size() == 0 ? this : this->GetDir(path, 0);
}

//----------------------------------------------------------------------------
const vtkADIOSDirTree* vtkADIOSDirTree::GetDir(
  const std::vector<std::string>& path, size_t pIdx) const
{
  if(pIdx == path.size())
    {
    return this;
    }
  else
    {
    std::map<std::string, vtkADIOSDirTree*>::const_iterator i =
      this->SubDirs.find(path[pIdx]);

    return i == this->SubDirs.end() ? NULL : i->second->GetDir(path, pIdx+1);
    }
}

//----------------------------------------------------------------------------
const ADIOS::VarInfo* vtkADIOSDirTree::GetArray(
  const std::string& varName) const
{
  std::map<std::string, const ADIOS::VarInfo*>::const_iterator i =
    this->Arrays.find(varName);

  return i == this->Arrays.end() ? NULL : i->second;
}

//----------------------------------------------------------------------------
const ADIOS::Scalar* vtkADIOSDirTree::GetScalar(
  const std::string& varName) const
{
  std::map<std::string, const ADIOS::Scalar*>::const_iterator i =
    this->Scalars.find(varName);

  return i == this->Scalars.end() ? NULL : i->second;
}

//----------------------------------------------------------------------------
void vtkADIOSDirTree::GetScalars(
  std::vector<const ADIOS::Scalar*>& vars) const
{
  vars.clear();
  vars.reserve(this->Scalars.size());
  std::map<std::string, const ADIOS::Scalar*>::const_iterator i;
  for(i = this->Scalars.begin(); i != this->Scalars.end(); ++i)
    {
    vars.push_back(i->second);
    }
}

//----------------------------------------------------------------------------
void vtkADIOSDirTree::GetArrays(
  std::vector<const ADIOS::VarInfo*>& vars) const
{
  vars.clear();
  vars.reserve(this->Arrays.size());
  std::map<std::string, const ADIOS::VarInfo*>::const_iterator i;
  for(i = this->Arrays.begin(); i != this->Arrays.end(); ++i)
    {
    vars.push_back(i->second);
    }
}

//----------------------------------------------------------------------------
vtkADIOSDirTree* vtkADIOSDirTree::BuildPath(
  const std::vector<std::string>& path, size_t startIdx,
  size_t numComponents)
{
  if(numComponents == 0)
    {
    return this;
    }

  const std::string& name = path[startIdx];
  vtkADIOSDirTree*& d = this->SubDirs[name];
  if(!d)
    {
    d = new vtkADIOSDirTree(name);
    }
  return d->BuildPath(path, startIdx+1, numComponents-1);
}
