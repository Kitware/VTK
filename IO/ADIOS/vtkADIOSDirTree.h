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
/**
 * @class   vtkADIOSDirTree
 * @brief   A directory tree structure holding ADIOS data
*/

#ifndef vtkADIOSDirTree_h
#define vtkADIOSDirTree_h

#include <map>
#include <string>
#include <vector>

#include "vtkIndent.h"

#include "ADIOSReader.h"
#include "ADIOSScalar.h"
#include "ADIOSVarInfo.h"

class vtkADIOSDirTree
{
public:
  vtkADIOSDirTree(const std::string& name);
  vtkADIOSDirTree(const ADIOS::Reader &reader);
  ~vtkADIOSDirTree();

  const std::string& GetName() const { return this->Name; }
  void PrintSelf(std::ostream& os, vtkIndent indent) const;

  /**
   * Access a subdirectory
   */
  const vtkADIOSDirTree* GetDir(const std::string& dirName) const;

  //@{
  /**
   * Access variables by name
   */
  const ADIOS::Scalar* GetScalar(const std::string& varName) const;
  const ADIOS::VarInfo* GetArray(const std::string& varName) const;
  //@}

  //@{
  /**
   * Access variables all at once
   */
  void GetScalars(std::vector<const ADIOS::Scalar*>& vars) const;
  void GetArrays(std::vector<const ADIOS::VarInfo*>& vars) const;
  //@}

private:
  vtkADIOSDirTree* BuildPath(const std::vector<std::string>& path,
    size_t startIdx, size_t numComponents);

  const vtkADIOSDirTree* GetDir(const std::vector<std::string>& path,
    size_t pIdx) const;

  const std::string Name;
  std::map<std::string, const ADIOS::Scalar*> Scalars;
  std::map<std::string, const ADIOS::VarInfo*> Arrays;
  std::map<std::string, vtkADIOSDirTree*> SubDirs;
};

#endif
// VTK-HeaderTest-Exclude: vtkADIOSDirTree.h
