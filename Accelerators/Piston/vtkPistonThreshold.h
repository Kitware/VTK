/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonThreshold.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPistonThreshold
 * @brief    A filter that contours on the GPU
 *
 * This filter uses LANL's Piston library to isocontour on the GPU.
*/

#ifndef vtkPistonThreshold_h
#define vtkPistonThreshold_h

#include "vtkPistonAlgorithm.h"

class VTKACCELERATORSPISTON_EXPORT vtkPistonThreshold : public vtkPistonAlgorithm
{
public:
  vtkTypeMacro(vtkPistonThreshold,vtkPistonAlgorithm);
  static vtkPistonThreshold *New();
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Choose the lower value of the threshold.
   */
  vtkSetMacro(MinValue, float);
  vtkGetMacro(MinValue, float);
  //@}

  //@{
  /**
   * Choose the upper value of the threshold.
   */
  vtkSetMacro(MaxValue, float);
  vtkGetMacro(MaxValue, float);
  //@}

protected:
  vtkPistonThreshold();
  ~vtkPistonThreshold();

  /**
   * Method that does the actual calculation.
   */
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);


  float MinValue;
  float MaxValue;

private:
  vtkPistonThreshold(const vtkPistonThreshold&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPistonThreshold&) VTK_DELETE_FUNCTION;

};

#endif
