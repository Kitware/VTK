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
/**
 * @class   vtkTemporalShiftScale
 * @brief   modify the time range/steps of temporal data
 *
 * vtkTemporalShiftScale  modify the time range or time steps of
 * the data without changing the data itself. The data is not resampled
 * by this filter, only the information accompanying the data is modified.
 *
 * @par Thanks:
 * Ken Martin (Kitware) and John Bidiscombe of
 * CSCS - Swiss National Supercomputing Centre
 * for creating and contributing this class.
 * For related material, please refer to :
 * John Biddiscombe, Berk Geveci, Ken Martin, Kenneth Moreland, David Thompson,
 * "Time Dependent Processing in a Parallel Pipeline Architecture",
 * IEEE Visualization 2007.
*/

#ifndef vtkTemporalShiftScale_h
#define vtkTemporalShiftScale_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkAlgorithm.h"


class VTKFILTERSHYBRID_EXPORT vtkTemporalShiftScale: public vtkAlgorithm
{
public:
  static vtkTemporalShiftScale *New();
  vtkTypeMacro(vtkTemporalShiftScale, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Apply a translation to the data before scaling.
   * To convert T{5,100} to T{0,1} use Preshift=-5, Scale=1/95, PostShift=0
   * To convert T{5,105} to T{5,10} use Preshift=-5, Scale=5/100, PostShift=5
   */
  vtkSetMacro(PreShift, double);
  vtkGetMacro(PreShift, double);
  //@}

  //@{
  /**
   * Apply a translation to the time
   */
  vtkSetMacro(PostShift, double);
  vtkGetMacro(PostShift, double);
  //@}

  //@{
  /**
   * Apply a scale to the time.
   */
  vtkSetMacro(Scale, double);
  vtkGetMacro(Scale, double);
  //@}

  //@{
  /**
   * If Periodic is true, requests for time will be wrapped around so that
   * the source appears to be a periodic time source. If data exists for times
   * {0,N-1}, setting periodic to true will cause time 0 to be produced when time
   * N, 2N, 2N etc is requested. This effectively gives the source the ability to
   * generate time data indefinitely in a loop.
   * When combined with Shift/Scale, the time becomes periodic in the
   * shifted and scaled time frame of reference.
   * Note: Since the input time may not start at zero, the wrapping of time
   * from the end of one period to the start of the next, will subtract the
   * initial time - a source with T{5..6} repeated periodicaly will have output
   * time {5..6..7..8} etc.
   */
  vtkSetMacro(Periodic, int);
  vtkGetMacro(Periodic, int);
  vtkBooleanMacro(Periodic, int);
  //@}

  //@{
  /**
   * if Periodic time is enabled, this flag determines if the last time step is the same
   * as the first. If PeriodicEndCorrection is true, then it is assumed that the input
   * data goes from 0-1 (or whatever scaled/shifted actual time) and time 1 is the
   * same as time 0 so that steps will be 0,1,2,3...N,1,2,3...N,1,2,3 where step N
   * is the same as 0 and step 0 is not repeated. When this flag is false
   * the data is assumed to be literal and output is of the form 0,1,2,3...N,0,1,2,3...
   * By default this flag is ON
   */
  vtkSetMacro(PeriodicEndCorrection, int);
  vtkGetMacro(PeriodicEndCorrection, int);
  vtkBooleanMacro(PeriodicEndCorrection, int);
  //@}

  //@{
  /**
   * if Periodic time is enabled, this controls how many time periods time is reported
   * for. A filter cannot output an infinite number of time steps and therefore a finite
   * number of periods is generated when reporting time.
   */
  vtkSetMacro(MaximumNumberOfPeriods, double);
  vtkGetMacro(MaximumNumberOfPeriods, double);
  //@}

protected:
  vtkTemporalShiftScale();
  ~vtkTemporalShiftScale();

  double PreShift;
  double PostShift;
  double Scale;
  int    Periodic;
  int    PeriodicEndCorrection;
  double MaximumNumberOfPeriods;
  //
  double InRange[2];
  double OutRange[2];
  double PeriodicRange[2];
  int    PeriodicN;
  double TempMultiplier;

  /**
   * see vtkAlgorithm for details
   */
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

  virtual int RequestUpdateExtent (vtkInformation *,
                                   vtkInformationVector **,
                                   vtkInformationVector *);

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info);


  virtual int RequestDataObject(vtkInformation *,
                                vtkInformationVector **,
                                vtkInformationVector *);


  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

  double ForwardConvert(double T0);
  double BackwardConvert(double T1);

private:
  vtkTemporalShiftScale(const vtkTemporalShiftScale&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTemporalShiftScale&) VTK_DELETE_FUNCTION;
};



#endif



