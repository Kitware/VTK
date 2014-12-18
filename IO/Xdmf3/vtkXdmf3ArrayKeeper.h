/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3ArrayKeeper.h
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
// .NAME vtkXdmf3ArrayKeeper - LRU cache of XDMF Arrays
// .SECTION Description
// vtkXdmf3ArrayKeeper maintains the in memory cache of recently used XdmfArrays.
// Each array that is loaded from XDMF is put in the cache and/or marked with the
// current timestep. A release method frees arrays that have not been recently
// used.
//
// This file is a helper for the vtkXdmf3Reader and not intended to be
// part of VTK public API
// VTK-HeaderTest-Exclude: vtkXdmf3ArrayKeeper.h

#ifndef vtkXdmf3ArrayKeeper_h
#define vtkXdmf3ArrayKeeper_h

#include "vtkIOXdmf3Module.h" // For export macro
#include <map>

class XdmfArray;

class VTKIOXDMF3_EXPORT vtkXdmf3ArrayKeeper
  : public std::map<XdmfArray *, unsigned int>
{
public:
  //Description:
  //Constructor
  vtkXdmf3ArrayKeeper();

  //Description:
  //Destructor
  ~vtkXdmf3ArrayKeeper();

  //Description:
  //Call to mark arrays that will be accessed with a new timestamp
  void BumpGeneration();

  //Description:
  //Call whenever you a new XDMF array is accessed.
  void Insert(XdmfArray *val);

  //Description:
  //Call to free all open arrays that are currently open but not in use.
  //Force argument frees all arrays.
  void Release(bool force);

private:
  unsigned int generation;
};

#endif //vtkXdmf3ArrayKeeper_h
