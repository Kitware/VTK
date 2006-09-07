/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTemporalInterpolator - interpolate temporal datasets
// .SECTION Description
// vtkTemporalInterpolator convert a discrete time series to respond to
// continuous temporal requests.

#ifndef __vtkTemporalInterpolator_h
#define __vtkTemporalInterpolator_h

#include "vtkTemporalDataSetAlgorithm.h"

class vtkSimpleInterpolator;
class vtkDataSet;

class VTK_HYBRID_EXPORT vtkTemporalInterpolator : public vtkTemporalDataSetAlgorithm
{
public:
  static vtkTemporalInterpolator *New();
  vtkTypeRevisionMacro(vtkTemporalInterpolator, vtkTemporalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkTemporalInterpolator();
  ~vtkTemporalInterpolator();

  virtual int RequestUpdateExtent (vtkInformation *,
                                   vtkInformationVector **,
                                   vtkInformationVector *);
  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);
  
  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

  // Description:
  // General interpolation routine for any tiype on input data. This is
  // called recursively when heirarchical/multigroup data is encountered
  vtkDataObject *InterpolateDataObject(vtkDataObject *in1, 
                                       vtkDataObject *in2,
                                       double ratio);

  // Description:
  // Root level interpolation for a concrete dataset object.
  // Point/Cell data and points are interpolated.
  // Needs improving if connectivity is to be handled
  vtkDataSet *InterpolateDataSet(vtkDataSet *in1, 
                                 vtkDataSet *in2,
                                 double ratio);

  // Description:
  // Interpolate a single vtkDataArray. Called from the Interpolation routine
  // on the points and pointdata/celldata
  vtkDataArray *InterpolateDataArray(double ratio, vtkDataArray **arrays, 
                                     vtkIdType N);

  // Description:
  // Called juse before interpolation to ensure each data arrayhas the same 
  // number of tuples
  bool VerifyArrays(vtkDataArray **arrays, int N);

private:
  vtkTemporalInterpolator(const vtkTemporalInterpolator&);  // Not implemented.
  void operator=(const vtkTemporalInterpolator&);  // Not implemented.
};



#endif



