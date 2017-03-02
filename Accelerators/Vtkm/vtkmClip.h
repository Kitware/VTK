/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkmClip.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkmClip
 * @brief Clip a dataset using the accelerated vtk-m Clip filter.
 */

#ifndef vtkmClip_h
#define vtkmClip_h

#include "vtkAcceleratorsVTKmModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class VTKACCELERATORSVTKM_EXPORT vtkmClip : public vtkDataSetAlgorithm
{
public:
  static vtkmClip* New();
  vtkTypeMacro(vtkmClip, vtkDataSetAlgorithm)
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * The scalar value to use when clipping the dataset. Values greater than
   * ClipValue are preserved in the output dataset. Default is 0.
   */
  vtkGetMacro(ClipValue, double)
  vtkSetMacro(ClipValue, double)

  /**
   * If true, all input point data arrays will be mapped onto the output
   * dataset. Default is true.
   */
  vtkGetMacro(ComputeScalars, bool)
  vtkSetMacro(ComputeScalars, bool)

protected:
  vtkmClip();
  ~vtkmClip();

  int RequestDataObject(vtkInformation*, vtkInformationVector**,
                        vtkInformationVector*) VTK_OVERRIDE;
  int RequestData(vtkInformation*, vtkInformationVector**,
                  vtkInformationVector*) VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  double ClipValue;
  bool ComputeScalars;

private:
  vtkmClip(const vtkmClip&) VTK_DELETE_FUNCTION;
  void operator=(const vtkmClip&) VTK_DELETE_FUNCTION;
};

#endif // vtkmClip_h
// VTK-HeaderTest-Exclude: vtkmClip.h
