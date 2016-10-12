/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridVolumeRayIntegrator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkUnstructuredGridVolumeRayIntegrator
 * @brief   a superclass for volume ray integration functions
 *
 *
 *
 * vtkUnstructuredGridVolumeRayIntegrator is a superclass for ray
 * integration functions that can be used within a
 * vtkUnstructuredGridVolumeRayCastMapper.
 *
 * @sa
 * vtkUnstructuredGridVolumeRayCastMapper
 * vtkUnstructuredGridVolumeRayCastFunction
*/

#ifndef vtkUnstructuredGridVolumeRayIntegrator_h
#define vtkUnstructuredGridVolumeRayIntegrator_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkObject.h"

class vtkVolume;
class vtkDoubleArray;
class vtkDataArray;

class VTKRENDERINGVOLUME_EXPORT vtkUnstructuredGridVolumeRayIntegrator : public vtkObject
{
public:
  vtkTypeMacro(vtkUnstructuredGridVolumeRayIntegrator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Set up the integrator with the given properties and scalars.
   */
  virtual void Initialize(vtkVolume *volume,
                          vtkDataArray* scalars) = 0;

  /**
   * Given a set of intersections (defined by the three arrays), compute
   * the peicewise integration of the array in front to back order.
   * /c intersectionLengths holds the lengths of each peicewise segment.
   * /c nearIntersections and /c farIntersections hold the scalar values at
   * the front and back of each segment.  /c color should contain the RGBA
   * value of the volume in front of the segments passed in, and the result
   * will be placed back into /c color.
   */
  virtual void Integrate(vtkDoubleArray *intersectionLengths,
                         vtkDataArray *nearIntersections,
                         vtkDataArray *farIntersections,
                         float color[4]) = 0;

protected:
  vtkUnstructuredGridVolumeRayIntegrator();
  ~vtkUnstructuredGridVolumeRayIntegrator();

private:
  vtkUnstructuredGridVolumeRayIntegrator(const vtkUnstructuredGridVolumeRayIntegrator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnstructuredGridVolumeRayIntegrator&) VTK_DELETE_FUNCTION;
};

#endif

