/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridVolumeRayCastFunction.h
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

// .NAME vtkUnstructuredGridVolumeRayCastFunction - a superclass for ray casting functions

// .SECTION Description
// vtkUnstructuredGridVolumeRayCastFunction is a superclass for ray casting functions that 
// can be used within a vtkUnstructuredGridVolumeRayCastMapper. 

// .SECTION See Also

#ifndef __vtkUnstructuredGridVolumeRayCastFunction_h
#define __vtkUnstructuredGridVolumeRayCastFunction_h

#include "vtkObject.h"

class vtkRenderer;
class vtkVolume;

class VTK_RENDERING_EXPORT vtkUnstructuredGridVolumeRayCastFunction : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkUnstructuredGridVolumeRayCastFunction,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkUnstructuredGridVolumeRayCastFunction() {};
  ~vtkUnstructuredGridVolumeRayCastFunction() {};

private:
  vtkUnstructuredGridVolumeRayCastFunction(const vtkUnstructuredGridVolumeRayCastFunction&);  // Not implemented.
  void operator=(const vtkUnstructuredGridVolumeRayCastFunction&);  // Not implemented.
};

#endif







