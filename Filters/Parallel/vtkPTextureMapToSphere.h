/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPTextureMapToSphere.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPTextureMapToSphere
 * @brief   generate texture coordinates by mapping points to sphere
 *
 * vtkPTextureMapToSphere inherits from vtkTextureMapToSphere to handle multi-processing
 * environment.
 *
 * @sa
 * vtkTextureMapToPlane vtkTextureMapToCylinder
 * vtkTransformTexture vtkThresholdTextureCoords
 * vtkTextureMapToSphere
 */

#ifndef vtkPTextureMapToSphere_h
#define vtkPTextureMapToSphere_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkTextureMapToSphere.h"

class vtkDataSet;
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPTextureMapToSphere : public vtkTextureMapToSphere
{
public:
  vtkTypeMacro(vtkPTextureMapToSphere, vtkTextureMapToSphere);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPTextureMapToSphere* New();

protected:
  vtkPTextureMapToSphere();
  ~vtkPTextureMapToSphere() override = default;

  virtual void ComputeCenter(vtkDataSet* dataSet) override;

  vtkMultiProcessController* Controller;

private:
  vtkPTextureMapToSphere(const vtkPTextureMapToSphere&) = delete;
  void operator=(const vtkPTextureMapToSphere&) = delete;
};

#endif
