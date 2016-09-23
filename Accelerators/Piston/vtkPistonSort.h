/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonSort.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPistonSort
 * @brief    NVidia thrust filter example.
 *
 * An example filter that operates on the GPU and produces a result that
 * can be processed by another piston filter in the pipeline.
 * The implementation simply calls thrust::sort on the scalar array which
 * keeps the same data type while producing a result which is verifiably
 * correct.
*/

#ifndef vtkPistonSort_h
#define vtkPistonSort_h

#include "vtkPistonAlgorithm.h"

class VTKACCELERATORSPISTON_EXPORT vtkPistonSort : public vtkPistonAlgorithm
{
public:
  vtkTypeMacro(vtkPistonSort,vtkPistonAlgorithm);
  static vtkPistonSort *New();
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPistonSort() {VTK_LEGACY_BODY(vtkPistonSort::vtkPistonSort, "VTK 6.3");}
  ~vtkPistonSort() {}

  /**
   * Method that does the actual calculation.
   */
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);


private:
  vtkPistonSort(const vtkPistonSort&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPistonSort&) VTK_DELETE_FUNCTION;

};

#endif
