/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLogLookupTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLogLookupTable - map scalars into colors using log (base 10) scale
// .SECTION Description
// This class is an empty shell.  Use vtkLookupTable with SetScaleToLog10()
// instead.
//
// .SECTION See Also
// vtkLookupTable

#ifndef __vtkLogLookupTable_h
#define __vtkLogLookupTable_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkLookupTable.h"

class VTKRENDERINGCORE_EXPORT vtkLogLookupTable : public vtkLookupTable
{
public:
  static vtkLogLookupTable *New();

  vtkTypeMacro(vtkLogLookupTable,vtkLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkLogLookupTable(int sze=256, int ext=256);
  ~vtkLogLookupTable() {};
private:
  vtkLogLookupTable(const vtkLogLookupTable&);  // Not implemented.
  void operator=(const vtkLogLookupTable&);  // Not implemented.
};

#endif
