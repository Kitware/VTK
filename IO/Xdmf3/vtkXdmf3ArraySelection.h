/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3Common.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXdmf3ArraySelection - helper to identify requested arrays with
// .SECTION Description

// This file is a helper for the vtkXdmf3Reader and vtkXdmf3Writer and
// not intended to be part of VTK public API
// VTK-HeaderTest-Exclude: vtkXdmf3ArraySelection.h

#ifndef __vtkXdmf3ArraySelection_h
#define __vtkXdmf3ArraySelection_h

#include "vtkIOXdmf3Module.h" // For export macro

#include <map> //for superclass template
#include <string> //for superclass's content type

class vtkXdmf3ArraySelection : public std::map<std::string, bool>
{
public:
  void Merge(const vtkXdmf3ArraySelection& other);
  void AddArray(const char* name, bool status=true);
  bool ArrayIsEnabled(const char* name);
  bool HasArray(const char* name);
  int GetArraySetting(const char* name);
  void SetArrayStatus(const char* name, bool status);
  const char* GetArrayName(int index);
  int GetNumberOfArrays();
};

#endif //# __vtkXdmf3ArraySelection_h
