/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunctionShiftScale.h
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

// .NAME vtkPiecewiseFunctionShiftScale -
// 
// .SECTION Description

#ifndef __vtkPiecewiseFunctionShiftScale_h
#define __vtkPiecewiseFunctionShiftScale_h

#include "vtkPiecewiseFunctionToPiecewiseFunctionFilter.h"
#include "vtkPiecewiseFunction.h"

class VTK_FILTERING_EXPORT vtkPiecewiseFunctionShiftScale : public vtkPiecewiseFunctionToPiecewiseFunctionFilter
{
public:
  static vtkPiecewiseFunctionShiftScale *New();
  vtkTypeRevisionMacro(vtkPiecewiseFunctionShiftScale, vtkPiecewiseFunctionToPiecewiseFunctionFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  vtkSetObjectMacro(Input, vtkPiecewiseFunction);
  vtkGetObjectMacro(Input, vtkPiecewiseFunction);
  
  vtkSetMacro(PositionShift, float);
  vtkSetMacro(PositionScale, float);
  vtkSetMacro(ValueShift, float);
  vtkSetMacro(ValueScale, float);
  
  vtkGetMacro(PositionShift, float);
  vtkGetMacro(PositionScale, float);
  vtkGetMacro(ValueShift, float);
  vtkGetMacro(ValueScale, float);
  
protected:
  vtkPiecewiseFunctionShiftScale();
  ~vtkPiecewiseFunctionShiftScale();
  
  void Execute();
  
  vtkPiecewiseFunction *Input;
  
  float PositionShift;
  float PositionScale;
  float ValueShift;
  float ValueScale;
  
private:
  vtkPiecewiseFunctionShiftScale(const vtkPiecewiseFunctionShiftScale&);  // Not implemented
  void operator=(const vtkPiecewiseFunctionShiftScale&);  // Not implemented
};

#endif
