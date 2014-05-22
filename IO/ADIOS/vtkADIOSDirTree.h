/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkADIOSDirTree.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkADIOSDirTree - A directory tree structure holding ADIOS data

#ifndef __vtkADIOSDirTree_h
#define __vtkADIOSDirTree_h

#include <string>
#include <vector>
#include <map>

#include <vtkIndent.h>

#include "ADIOSScalar.h"
#include "ADIOSVarInfo.h"
#include "ADIOSReader.h"

struct vtkADIOSDirTree
{
  std::map<std::string, const ADIOSScalar*> Scalars;
  std::map<std::string, const ADIOSVarInfo*> Arrays;
  std::map<std::string, vtkADIOSDirTree> SubDirs;

  void PrintSelf(std::ostream& os, vtkIndent indent) const;

  // Description:
  // Cosntruct a directory tree form pre-cached ADIOS data
  void BuildDirTree(const ADIOSReader &reader);

  // Description:
  // Retrieve a directory object
  vtkADIOSDirTree* GetDir(const std::string& path, size_t numDrop = 0,
    bool createPath = false)
  {
    std::vector<std::string> tok;
    Tokenize(path, tok);
    return GetDir(tok, numDrop, createPath);
  }

  vtkADIOSDirTree* GetDir(const std::vector<std::string>& path,
    size_t numDrop, bool createPath);

  const vtkADIOSDirTree* GetDir(const std::string& path,
    size_t numDrop = 0) const
  {
    std::vector<std::string> tok;
    Tokenize(path, tok);
    return GetDir(tok, numDrop);
  }
  const vtkADIOSDirTree* GetDir(const std::vector<std::string>& path,
    size_t numDrop = 0) const;

  // Description:
  // Access variables by name
  const ADIOSVarInfo* operator[](const std::string& name) const;

  const ADIOSScalar* GetScalar(const std::string& name) const;

  // Description:
  // A helper function to split a string into delimited components
  static void Tokenize(const std::string& path, std::vector<std::string>& split,
    char delim='/');
};

#endif
// VTK-HeaderTest-Exclude: vtkADIOSDirTree.h
