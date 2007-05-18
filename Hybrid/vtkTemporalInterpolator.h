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
// vtkTemporalInterpolator interpolates between two time steps to
// produce new data for an arbitrary T.
// vtkTemporalInterpolator has two modes of operation. The default
// mode is to produce a continuous range of time values as output
// which enables a filter downstream to request Any value of T within
// the range. The interpolator will produce the requested T.
// The second mode of operation is enabled by setting
// DiscreteTimeStepInterval to a non zero value. When this mode is
// activated, the filter will report a finite number of Time steps
// separated by deltaT between the original range of values.
// This mode is useful when a dataset of N time steps has one (or more)
// missing datasets for certain T values and you simply wish to smooth
// over the missing steps but otherwise use the original data.

#ifndef __vtkTemporalInterpolator_h
#define __vtkTemporalInterpolator_h

#include "vtkTemporalDataSetAlgorithm.h"

class vtkSimpleInterpolator;
class vtkDataSet;
/*
class VTK_HYBRID_EXPORT vtkTemporalInterpolator : public vtkDataObjectAlgorithm
{
public:
  static vtkTemporalInterpolator *New();
  vtkTypeRevisionMacro(vtkTemporalInterpolator, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
*/
class VTK_HYBRID_EXPORT vtkTemporalInterpolator : public vtkTemporalDataSetAlgorithm
{
public:
  static vtkTemporalInterpolator *New();
  vtkTypeRevisionMacro(vtkTemporalInterpolator, vtkTemporalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If you require a discrete number of outputs steps, to be 
  // generated from an input source - for example, you required
  // N steps separated by T, then set DiscreteTimeStepInterval to T
  // and you will get TIME_RANGE/DiscreteTimeStepInterval steps
  // This is a useful option to use if you have a dataset with one
  // missing time step and wish to 'fill-in' the missing data
  // with an interpolated value from the steps either side
  vtkSetMacro(DiscreteTimeStepInterval, double);
  vtkGetMacro(DiscreteTimeStepInterval, double);

protected:
  vtkTemporalInterpolator();
  ~vtkTemporalInterpolator();

  double DiscreteTimeStepInterval;
/*
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  virtual int RequestDataObject(vtkInformation *,
                                vtkInformationVector **,
                                vtkInformationVector *);
*/
  virtual int RequestUpdateExtent(vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *,
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



