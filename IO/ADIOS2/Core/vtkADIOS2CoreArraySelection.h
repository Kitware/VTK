/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3ArraySelection.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkADIOS2ArraySelection
 * @brief   helper to identify requested arrays with
 *
 * Used to mark arrays that reader can optionally read in.
 * Needed for ParaView GUI usage.
 *
 * This file is a helper for the vtkADIOS2CoreImageReader and not intended to be
 * part of VTK public API
 */

#ifndef vtkADIOS2CoreArraySelection_h
#define vtkADIOS2CoreArraySelection_h

#include "vtkIOADIOS2Module.h" // For export macro

#include <map>    //for superclass template
#include <string> //for superclass's content type

#ifdef _MSC_VER
#pragma warning(push)           // save
#pragma warning(disable : 4251) // needs to have dll-interface to be used by clients of class
#endif
class VTKIOADIOS2_EXPORT vtkADIOS2ArraySelection : public std::map<std::string, bool>
{
public:
  /**
   * add a new array to the set, with a particular status
   */
  void AddArray(const char* name, bool status = true);

  /**
   * test if a particular array is enablled or not
   */
  bool ArrayIsEnabled(const char* name);

  /**
   * check if a particular array is in the map yet or not
   */
  bool HasArray(const char* name);

  //@{
  /**
   * get/set user choice of whether a particular array is to be loaded
   */
  void SetArrayStatus(const char* name, bool status);
  int GetArrayStatus(const char* name);
  //@}

  /**
   * get name of a particular array
   */
  const char* GetArrayName(int index);

  /**
   * get number of arrays in the map
   */
  int GetNumberOfArrays();
};
#ifdef _MSC_VER
#pragma warning(pop) // restore
#endif

#endif //# vtkADIOS2CoreArraySelection_h
// VTK-HeaderTest-Exclude: vtkADIOS2CoreArraySelection.h
