/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3ArraySelection.h
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
// Used by ParaView GUI to mark arrays, sets, and blocks that reader
// can optionally read in. Note: we use it for more than just arrays but
// Paraview code requires "Array" name in the API.
//
// This file is a helper for the vtkXdmf3Reader and not intended to be
// part of VTK public API
// VTK-HeaderTest-Exclude: vtkXdmf3ArraySelection.h

#ifndef vtkXdmf3ArraySelection_h
#define vtkXdmf3ArraySelection_h

#include "vtkIOXdmf3Module.h" // For export macro

#include <map> //for superclass template
#include <string> //for superclass's content type

class VTKIOXDMF3_EXPORT vtkXdmf3ArraySelection
  : public std::map<std::string, bool>
{
public:
  // Description:
  // used in parallel to send of combine sets
  void Merge(const vtkXdmf3ArraySelection& other);

  // Description:
  // add a new array to the set, with a particular status
  void AddArray(const char* name, bool status=true);

  // Description:
  // test if a particular array is enablled or not
  bool ArrayIsEnabled(const char* name);

  // Description:
  // check if a particular array is in the set yet or not
  bool HasArray(const char* name);

  // Description:
  // get/set user choice of whether a particular array is to be loaded
  void SetArrayStatus(const char* name, bool status);
  int GetArraySetting(const char* name);

  // Description:
  // get string name of a particular array
  const char* GetArrayName(int index);

  // Description:
  // get number of arrays in the set
  int GetNumberOfArrays();
};

#endif //# vtkXdmf3ArraySelection_h
