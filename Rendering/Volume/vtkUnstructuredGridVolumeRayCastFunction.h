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

/**
 * @class   vtkUnstructuredGridVolumeRayCastFunction
 * @brief   a superclass for ray casting functions
 *
 *
 * vtkUnstructuredGridVolumeRayCastFunction is a superclass for ray casting functions that
 * can be used within a vtkUnstructuredGridVolumeRayCastMapper.
 *
 * @sa
 * vtkUnstructuredGridVolumeRayCastMapper vtkUnstructuredGridVolumeRayIntegrator
*/

#ifndef vtkUnstructuredGridVolumeRayCastFunction_h
#define vtkUnstructuredGridVolumeRayCastFunction_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkObject.h"

class vtkRenderer;
class vtkVolume;
class vtkUnstructuredGridVolumeRayCastIterator;

class VTKRENDERINGVOLUME_EXPORT vtkUnstructuredGridVolumeRayCastFunction : public vtkObject
{
public:
  vtkTypeMacro(vtkUnstructuredGridVolumeRayCastFunction,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual void Initialize( vtkRenderer *ren, vtkVolume   *vol )=0;

  virtual void Finalize( )=0;

 /**
  * Returns a new object that will iterate over all the intersections of a
  * ray with the cells of the input.  The calling code is responsible for
  * deleting the returned object.
  */
  VTK_NEWINSTANCE
  virtual vtkUnstructuredGridVolumeRayCastIterator *NewIterator() = 0;

protected:
  vtkUnstructuredGridVolumeRayCastFunction() {}
  ~vtkUnstructuredGridVolumeRayCastFunction() {}

private:
  vtkUnstructuredGridVolumeRayCastFunction(const vtkUnstructuredGridVolumeRayCastFunction&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnstructuredGridVolumeRayCastFunction&) VTK_DELETE_FUNCTION;
};

#endif







