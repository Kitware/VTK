/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPassThroughFilter.h
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
// .NAME vtkPassThroughFilter - Filter which shallow copies it's input to it's output
// .SECTION Description
// This filter shallow copies it's input to it's output. It is normally
// used by PVSources with multiple outputs as the VTK filter in the
// dummy connection objects at each output.

#ifndef __vtkPassThroughFilter_h
#define __vtkPassThroughFilter_h

#include "vtkDataSetToDataSetFilter.h"

class vtkFieldData;

class VTK_EXPORT vtkPassThroughFilter : public vtkDataSetToDataSetFilter
{
public:
  vtkTypeRevisionMacro(vtkPassThroughFilter,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a new vtkPassThroughFilter.
  static vtkPassThroughFilter *New();


protected:

  vtkPassThroughFilter() {};
  virtual ~vtkPassThroughFilter() {};

  void Execute();

private:
  vtkPassThroughFilter(const vtkPassThroughFilter&);  // Not implemented.
  void operator=(const vtkPassThroughFilter&);  // Not implemented.
};

#endif


