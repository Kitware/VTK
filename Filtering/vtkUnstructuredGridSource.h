/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridSource.h
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
// .NAME vtkUnstructuredGridSource - abstract class whose subclasses generate unstructured grid data
// .SECTION Description
// vtkUnstructuredGridSource is an abstract class whose subclasses generate 
// unstructured grid data.

// .SECTION See Also
// vtkUnstructuredGridReader

#ifndef __vtkUnstructuredGridSource_h
#define __vtkUnstructuredGridSource_h

#include "vtkSource.h"
#include "vtkUnstructuredGrid.h"

class VTK_FILTERING_EXPORT vtkUnstructuredGridSource : public vtkSource
{
public:
  vtkTypeRevisionMacro(vtkUnstructuredGridSource,vtkSource);

  // Description:
  // Get the output of this source.
  vtkUnstructuredGrid *GetOutput();
  vtkUnstructuredGrid *GetOutput(int idx)
    {return (vtkUnstructuredGrid *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkUnstructuredGrid *output);
  
protected:
  vtkUnstructuredGridSource();
  ~vtkUnstructuredGridSource() {};
  
  // Since the Outputs[0] has the same UpdateExtent format
  // as the generic DataObject we can copy the UpdateExtent
  // as a default behavior.
  void ComputeInputUpdateExtents(vtkDataObject *output);
  
private:
  vtkUnstructuredGridSource(const vtkUnstructuredGridSource&);  // Not implemented.
  void operator=(const vtkUnstructuredGridSource&);  // Not implemented.
};

#endif


