/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToPiston.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataSetToPiston
 * @brief   converts a DataSet to a PistonDataObject
 *
 * Converts vtkDataSets that reside on the CPU into piston data that
 * resides on the GPU. Afterward vtkPistonAlgorithms will processed
 * it there.
 *
 * @sa
 * vtkPistonToDataSet
*/

#ifndef vtkDataSetToPiston_h
#define vtkDataSetToPiston_h

#include "vtkPistonAlgorithm.h"

class vtkDataSet;

class VTKACCELERATORSPISTON_EXPORT vtkDataSetToPiston : public vtkPistonAlgorithm
{
public:
  static vtkDataSetToPiston *New();
  vtkTypeMacro(vtkDataSetToPiston,vtkPistonAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkDataSetToPiston();
  ~vtkDataSetToPiston();

  /**
   * Method that does the actual calculation. Funnels down to ExecuteData.
   */
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  /**
   * Overridden to say that we require vtkDataSet inputs
   */
  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkDataSetToPiston(const vtkDataSetToPiston&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataSetToPiston&) VTK_DELETE_FUNCTION;
};

#endif
