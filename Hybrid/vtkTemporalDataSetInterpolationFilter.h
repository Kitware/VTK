/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalDataSetInterpolationFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTemporalDataSetInterpolationFilter - Interpolate datasets between time values to produce a new dataset
// .SECTION Description
// vtkTemporalDataSetInterpolationFilter is a filter which interpolates 
// between (usually 2) datasets to produce a new dataset. 
//
// The filter can operate either one of two ways - depending on what type 
// of input it receives.
// If a vtkTemporalDataSet is input, the filter will take the timesteps it 
// requires from the input and use them for interpolation. If the time 
// steps required are not present, the filter will request the time steps 
// it needs and pass the request upstream during the RequestUpdateExtent 
// pass of the pipeline. The Upstream filter should be capable of producing
// the N requested timesteps. The interpolation filter will then use them to
// generate a single output for the Time T requested in the RequestData pass.
//
// If the input to the filter is not a vtkTemporalDataSet, then the filter
// will force the upstream pipeline to loop N times and generate N DataSets
// which will be cached internally and used for interpolation purposes.
// Note that this method only works if you use a modified 
// StreamingDemandDrivenPipeline and not the Composite executive. Integrating
// this functionalitry into the compositePipeline is a work in progress.
// 
// Note that in both methods of operation, this filter produces only a single
// timestep of output data. In future version it will hopefully be capable
// of producing multiple steps of interpolated data at once.
//
// Notes:
// If the input provides TIME_STEPS 0,1,2...N and the TimeStepInterval is
// set to 0.1, this filter will output TIME_STEPS 0.0, 0.1, 0.2....N
// (But only one timestep is produced at a time).
// 

#ifndef __vtkTemporalDataSetInterpolationFilter_h
#define __vtkTemporalDataSetInterpolationFilter_h

#include "vtkTemporalDataSetAlgorithm.h"
#include <vtkstd/vector>

class vtkDataSet;
class vtkDataSetCache;
class vtkSimpleInterpolator;
//BTX
class VTDIF_stdinternals;
//ETX
class VTK_HYBRID_EXPORT vtkTemporalDataSetInterpolationFilter : 
  public vtkTemporalDataSetAlgorithm
{
public:
//BTX
  // Description:
  // Enums to control the type of interpolation to use.
  enum {INTERPOLATION_TYPE_LINEAR=0,
        INTERPOLATION_TYPE_SPLINE
  };
//ETX
public:
  static vtkTemporalDataSetInterpolationFilter *New();
  vtkTypeRevisionMacro(vtkTemporalDataSetInterpolationFilter,
    vtkTemporalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify which type of function to use for interpolation. By default
  // linear interpolation (SetInterpolationFunctionToLinear()) is used
  vtkSetMacro(InterpolationType,int);
  vtkGetMacro(InterpolationType,int);
  void SetInterpolationTypeToLinear()
    {this->SetInterpolationType(INTERPOLATION_TYPE_LINEAR);}
  void SetInterpolationTypeToSpline()
    {this->SetInterpolationType(INTERPOLATION_TYPE_SPLINE);}
  
  // Description:
  // Specify how many points will be used for spline interpolation. 
  // Note that for linear interpolation two are always required, for spline, 
  // more must be used (the maximum is determined by memory constraints
  // but typically 5 should be enough).
  // When data for time 't' is requested, the input dataset will be 
  // fetched at N time points- 1/2 on each side of the requested 't' if
  // available.
  vtkSetMacro(NumberOfSplineInterpolationPoints,int);
  vtkGetMacro(NumberOfSplineInterpolationPoints,int);
  
  // Description:
  // Set/Get the timestep. This value will be overridden if a downstream filter is 
  // also a temporal filter and requests some other T value. Additionally
  // The timestep T is only meaningful if you already know the TimeSteps{...}
  // that are produced - they are output in the Information of the filter.
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // Description:
  // Set/Get the timestep as a real time value. The input data will be interpolated
  // from N datasets if the exact 't' value does not correspond to an input time
  // value. This has not been implemented fully yet, so use only for debugging purposes
  // until it is ready.
  vtkSetMacro(TimeValue, double);
  vtkGetMacro(TimeValue, double);

  // Description:
  // Set the time resolution of the output data. If the input data is 
  // at times T=1,2...N (separated by 1T), setting the TimeStepInterval 
  // to 0.1T will produce 10xN+1 steps between the originals.
  vtkSetMacro(TimeStepInterval, double);
  vtkGetMacro(TimeStepInterval, double);

  // Description:
  // Get the range of valid timestep index values. 
  // This can be used by the ParaView GUI.
  vtkGetVector2Macro(TimeStepRange, int);

  vtkExecutive *CreateDefaultExecutive();

protected:
   vtkTemporalDataSetInterpolationFilter();
  ~vtkTemporalDataSetInterpolationFilter();

  //
  // Provide the DataObject information for inputs and outputs
  //
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  //
  // Process the usual suspects
  //
  virtual int ProcessRequest(vtkInformation* request, 
                             vtkInformationVector** inputVector, 
                             vtkInformationVector* outputVector);

  //
  // Handle the Information request
  //
  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector, 
                                 vtkInformationVector *outputVector);

  //
  // Create the data
  //
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  
  //
  // Compute input time steps given output ones
  //
  virtual int RequestUpdateExtent(vtkInformation* request,
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector);

  //
  // Given a particular timestep requested on the output,
  // compute the required input time steps (should be time values)
  //
  virtual int ComputeInputTimeValues(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  // Description: (when looping a simple pipeline - not used if input is temporal)
  // The Algorithm receives this ModifyRequest from the executive before sending
  // REQUEST_DATA. 
  // If all the timesteps we want are already cached - then stop the REQUEST_DATA 
  // from being sent Upstream - otherwise the filter upstream will update a second
  // time and we don't need it to. Once we have interpolated, put the request back
  // otherwise the pipeline will fall over next time around (the request is a
  // static vtkInformation object essentially)
  //
  // Note : I don't like doing this, but it works.
  virtual int ModifyRequest(vtkInformation* request, int when);

protected:
  // Description:
  // General interpolation routine for any tiype on input data. This is
  // called recursively when heirarchical/multigroup data is encountered
  vtkDataObject *InterpolateDataObject(vtkDataObject **input, int N);

  // Description:
  // Root level interpolation for a concrete dataset object.
  // Point/Cell data and points are interpolated.
  // Needs improving if connectivity is to be handled
  vtkDataSet    *InterpolateDataSet(vtkDataSet **input, int N);

  // Description:
  // Interpolate a single vtkDataArray. Called from the Interpolation routine on the 
  // points and pointdata/celldata
  vtkDataArray *InterpolateDataArray(double *T, vtkDataArray **arrays, vtkIdType Ni, vtkIdType N);

  // Description:
  // Called juse before interpolation to ensure each data arrayhas the same 
  // number of tuples
  bool VerifyArrays(vtkDataArray **arrays, int N);

  //
  // Data used within the class
  //

  // The number of timesteps that the input can provide
  int                     NumberOfInputTimeSteps;
  // The actual timestep we are currently requesting from the input
  int                     RequestedInputTimeStep;
  // The Number of Output timesteps that we can provide
  int                     NumberOfOutputTimeSteps;
  // The timestep requested by the SetTimeStep member (avoid using)
  int                     TimeStep;
  // The requested real time (timestep)
  double                  TimeValue;
  // The timestep we are actually delivering differs from this->TimeStep if the
  // downstream pipeline had a different time step requested
  int                     ActualTimeStep;
  // The same as {0, NumberOfOutputTimeSteps-1}
  int                     TimeStepRange[2];
  // The real time interval between steps in the output
  double                  TimeStepInterval;
  // Interpolation
  int                     InterpolationType;
  int                     NumberOfSplineInterpolationPoints;
  // Variables used during looping
  int                     DataLoopingFlag;
  // The first input time step index we need for the requested output time
  int                     FirstLoopIndex;
  // The last input time step index we need for the requested output time
  int                     LastLoopIndex;
  // Caching for interpolation purposes
  vtkDataSetCache        *DataCache;
  int                     SuppressDataUpdate;
  int                     SuppressedDataUpdate;

//BTX
  // Keep these handy to make checks easier
  VTDIF_stdinternals     *Internals;
//ETX

private:
  vtkTemporalDataSetInterpolationFilter(const vtkTemporalDataSetInterpolationFilter&);  // Not implemented.
  void operator=(const vtkTemporalDataSetInterpolationFilter&);        // Not implemented.
};

#endif
