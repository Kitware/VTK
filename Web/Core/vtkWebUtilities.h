/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWebUtilities
 * @brief   collection of utility functions for ParaView Web.
 *
 * vtkWebUtilities consolidates miscellaneous utility functions useful for
 * Python scripts designed for ParaView Web.
*/

#ifndef vtkWebUtilities_h
#define vtkWebUtilities_h

#include "vtkObject.h"
#include "vtkWebCoreModule.h" // needed for exports
#include <string>

class vtkDataSet;

class VTKWEBCORE_EXPORT vtkWebUtilities : public vtkObject
{
public:
  static vtkWebUtilities* New();
  vtkTypeMacro(vtkWebUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static std::string WriteAttributesToJavaScript(int field_type, vtkDataSet*);
  static std::string WriteAttributeHeadersToJavaScript(
    int field_type, vtkDataSet*);

  //@{
  /**
   * This method is similar to the ProcessRMIs() method on the GlobalController
   * except that it is Python friendly in the sense that it will release the
   * Python GIS lock, so when run in a thread, this will trully work in the
   * background without locking the main one.
   */
  static void ProcessRMIs();
  static void ProcessRMIs(int reportError, int dont_loop=0);
  //@}

protected:
  vtkWebUtilities();
  ~vtkWebUtilities();

private:
  vtkWebUtilities(const vtkWebUtilities&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWebUtilities&) VTK_DELETE_FUNCTION;

};

#endif
// VTK-HeaderTest-Exclude: vtkWebUtilities.h
