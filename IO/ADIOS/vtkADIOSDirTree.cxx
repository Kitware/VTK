/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkvtkADIOSDirTree.cxx

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
void vtkADIOSDirTree::Tokenize(const std::string& str,
  std::vector<std::string> &tok, char d)
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

//----------------------------------------------------------------------------
void vtkADIOSDirTree::PrintSelf(std::ostream& os, vtkIndent indent) const
{
  typedef std::map<std::string, const ADIOSScalar*>::const_iterator ScalMapIt;
  typedef std::map<std::string, const ADIOSVarInfo*>::const_iterator VarMapIt;
  typedef std::map<std::string, vtkADIOSDirTree>::const_iterator DirMapIt;
  vtkIndent indent2 = indent.GetNextIndent();

  if(this->Scalars.size() > 0)
    {
    for(ScalMapIt i = this->Scalars.begin(); i != this->Scalars.end(); ++i)
      {
      os << indent << "Scalar: " << i->first << std::endl;
      }
    }

  if(this->Arrays.size() > 0)
    {
    for(VarMapIt i = this->Arrays.begin(); i != this->Arrays.end(); ++i)
      {
      os << indent << "Array: " << i->first << std::endl;
      }
    }

  for(DirMapIt i = this->SubDirs.begin(); i != this->SubDirs.end(); ++i)
    {
    os << indent << i->first << '/' << std::endl;
    i->second.PrintSelf(os, indent2);
    }
}

//----------------------------------------------------------------------------
void vtkADIOSDirTree::BuildDirTree(const ADIOSReader &reader)
{
  std::vector<std::string> fullPath;

  // Pupulate scalars
  typedef std::vector<ADIOSScalar*>::const_iterator ScalIt;
  const std::vector<ADIOSScalar*>& scalars = reader.GetScalars();
  for(ScalIt i = scalars.begin(); i != scalars.end(); ++i)
    {
    Tokenize((*i)->GetName(), fullPath);
    vtkADIOSDirTree *subDir = this->GetDir(fullPath, 1, true);
    subDir->Scalars[fullPath[fullPath.size()-1]] = *i;
    }

  // Pupulate arrays
  typedef std::vector<ADIOSVarInfo*>::const_iterator VarIt;
  const std::vector<ADIOSVarInfo*>& arrays = reader.GetArrays();
  for(VarIt i = arrays.begin(); i != arrays.end(); ++i)
    {
    Tokenize((*i)->GetName(), fullPath);
    vtkADIOSDirTree *subDir = this->GetDir(fullPath, 1, true);
    subDir->Arrays[fullPath[fullPath.size()-1]] = *i;
    }
}

//----------------------------------------------------------------------------
vtkADIOSDirTree* vtkADIOSDirTree::GetDir(const std::vector<std::string>& path,
  size_t numDrop, bool createPath)
{
  typedef std::map<std::string, vtkADIOSDirTree> SubDirMap;

  vtkADIOSDirTree* curTree = this;
  for(size_t i = 0; i < path.size() - numDrop; ++i)
    {
    const std::string &name = path[i];
    SubDirMap::iterator subDir = curTree->SubDirs.find(name);
    if(subDir == curTree->SubDirs.end())
      {
      if(!createPath)
        {
        return NULL;
        }
      subDir = curTree->SubDirs.insert(
        SubDirMap::value_type(name, vtkADIOSDirTree())).first;
      }
    curTree = &subDir->second;
    }
  return curTree;
}

//----------------------------------------------------------------------------
const vtkADIOSDirTree* vtkADIOSDirTree::GetDir(
  const std::vector<std::string>& path, size_t numDrop) const
{
  typedef std::map<std::string, vtkADIOSDirTree> SubDirMap;

  const vtkADIOSDirTree* curTree = this;
  for(size_t i = 0; i < path.size() - numDrop; ++i)
    {
    const std::string &name = path[i];
    SubDirMap::const_iterator subDir = curTree->SubDirs.find(name);
    if(subDir == curTree->SubDirs.end())
      {
      return NULL;
      }
    curTree = &subDir->second;
    }
  return curTree;
}

//----------------------------------------------------------------------------
const ADIOSVarInfo* vtkADIOSDirTree::operator[](
  const std::string& varName) const
{
  std::map<std::string, const ADIOSVarInfo*>::const_iterator i;

  if((i = this->Arrays.find(varName)) != this->Arrays.end())
    {
    return i->second;
    }
  return NULL;
}

//----------------------------------------------------------------------------
const ADIOSScalar* vtkADIOSDirTree::GetScalar(const std::string& varName) const
{
  std::map<std::string, const ADIOSScalar*>::const_iterator i;

  if((i = this->Scalars.find(varName)) != this->Scalars.end())
    {
    return i->second;
    }
  return NULL;
}
