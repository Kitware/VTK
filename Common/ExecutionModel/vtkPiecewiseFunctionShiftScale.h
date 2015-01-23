/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunctionShiftScale.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPiecewiseFunctionShiftScale -
//
// .SECTION Description

#ifndef vtkPiecewiseFunctionShiftScale_h
#define vtkPiecewiseFunctionShiftScale_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkPiecewiseFunctionAlgorithm.h"

class vtkPiecewiseFunction;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkPiecewiseFunctionShiftScale : public vtkPiecewiseFunctionAlgorithm
{
public:
  static vtkPiecewiseFunctionShiftScale *New();
  vtkTypeMacro(vtkPiecewiseFunctionShiftScale, vtkPiecewiseFunctionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetMacro(PositionShift, double);
  vtkSetMacro(PositionScale, double);
  vtkSetMacro(ValueShift, double);
  vtkSetMacro(ValueScale, double);

  vtkGetMacro(PositionShift, double);
  vtkGetMacro(PositionScale, double);
  vtkGetMacro(ValueShift, double);
  vtkGetMacro(ValueScale, double);

protected:
  vtkPiecewiseFunctionShiftScale();
  ~vtkPiecewiseFunctionShiftScale();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  double PositionShift;
  double PositionScale;
  double ValueShift;
  double ValueScale;

private:
  vtkPiecewiseFunctionShiftScale(const vtkPiecewiseFunctionShiftScale&);  // Not implemented
  void operator=(const vtkPiecewiseFunctionShiftScale&);  // Not implemented
};

#endif
