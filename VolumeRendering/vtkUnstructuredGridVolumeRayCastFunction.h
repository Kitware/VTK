/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridVolumeRayCastFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
// vtkUnstructuredGridVolumeRayCastMapper vtkUnstructuredGridVolumeRayIntegrator

#ifndef __vtkUnstructuredGridVolumeRayCastFunction_h
#define __vtkUnstructuredGridVolumeRayCastFunction_h

#include "vtkObject.h"

class vtkRenderer;
class vtkVolume;
class vtkUnstructuredGridVolumeRayCastIterator;

class VTK_VOLUMERENDERING_EXPORT vtkUnstructuredGridVolumeRayCastFunction : public vtkObject
{
public:
  vtkTypeMacro(vtkUnstructuredGridVolumeRayCastFunction,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  virtual void Initialize( vtkRenderer *ren, vtkVolume   *vol )=0;
  
  virtual void Finalize( )=0;

 // Description:
  // Returns a new object that will iterate over all the intersections of a
  // ray with the cells of the input.  The calling code is responsible for
  // deleting the returned object.
  virtual vtkUnstructuredGridVolumeRayCastIterator *NewIterator() = 0;
//ETX


protected:
  vtkUnstructuredGridVolumeRayCastFunction() {};
  ~vtkUnstructuredGridVolumeRayCastFunction() {};

private:
  vtkUnstructuredGridVolumeRayCastFunction(const vtkUnstructuredGridVolumeRayCastFunction&);  // Not implemented.
  void operator=(const vtkUnstructuredGridVolumeRayCastFunction&);  // Not implemented.
};

#endif







