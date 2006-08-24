/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalShiftScale.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTemporalShiftScale - modify the time range/steps of temporal data
// .SECTION Description
// vtkTemporalShiftScale  modify the time range or time steps of
// the data without changing the data itself. The data is not resampled
// by this filter, only the information accompanying the data is modified.

#ifndef __vtkTemporalShiftScale_h
#define __vtkTemporalShiftScale_h

#include "vtkTemporalDataSetAlgorithm.h"

class VTK_HYBRID_EXPORT vtkTemporalShiftScale : public vtkTemporalDataSetAlgorithm
{
public:
  static vtkTemporalShiftScale *New();
  vtkTypeRevisionMacro(vtkTemporalShiftScale, vtkTemporalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Apply a translation to the data.
  vtkSetMacro(Shift, double);
  vtkGetMacro(Shift, double);

  // Description:
  // Apply a scale to the data.  The scale is applied
  // before the translation.
  vtkSetMacro(Scale, double);
  vtkGetMacro(Scale, double);

protected:
  vtkTemporalShiftScale();
  ~vtkTemporalShiftScale();

  double Shift;
  double Scale;
  
  virtual int RequestUpdateExtent (vtkInformation *,
                                   vtkInformationVector **,
                                   vtkInformationVector *);
  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);
  
  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

private:
  vtkTemporalShiftScale(const vtkTemporalShiftScale&);  // Not implemented.
  void operator=(const vtkTemporalShiftScale&);  // Not implemented.
};



#endif



