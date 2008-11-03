/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkStatisticsAlgorithm.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
// .NAME vtkStatisticsAlgorithm - Base class for statistics algorithms
//
// .SECTION Description
// All statistics algorithms can conceptually be operated with several options:
// * Learn: given an input data set, calculate a minimal statistical model (e.g., 
//   sums, raw moments, joint probabilities).
// * Derive: given an input minimal statistical model, derive the full model 
//   (e.g., descriptive statistics, quantiles, correlations, conditional
//    probabilities).
// * Assess: given an input data set, input statistics, and some form of 
//   threshold, assess a subset of the data set. 
// Therefore, a vtkStatisticsAlgorithm has the following vtkTable ports
// * 2 input ports:
//   * Data (mandatory)
//   * Input model (optional) 
// * 3 output port (called Output):
//   * Data (annotated with assessments when the Assess option is ON).
//   * Output model (identical to the the input model when Learn option is OFF).
//   * Meta information about the model and/or the overall fit of the data to the
//     model; is filled only when the Assess option is ON.
//
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkStatisticsAlgorithm_h
#define __vtkStatisticsAlgorithm_h

#include "vtkTableAlgorithm.h"

class vtkStdString;
class vtkStringArray;
class vtkVariantArray;

class VTK_INFOVIS_EXPORT vtkStatisticsAlgorithm : public vtkTableAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkStatisticsAlgorithm, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // A convenience method for setting the statistics table input.
  // NB: This is mainly for the benefit of the VTK client/server 
  // layer, vanilla VTKcode should use, e.g:
  //
  // stats_algo2->SetInputConnection( 1, stats_algo1->output() );
  //
  virtual void SetInputStatisticsConnection( vtkAlgorithmOutput* );

  // Description:
  // Set the number of variables.
  vtkSetMacro( NumberOfVariables, vtkIdType );

  // Description:
  // Get the number of variables.
  vtkGetMacro( NumberOfVariables, vtkIdType );

  // Description:
  // Set the sample size.
  vtkSetMacro( SampleSize, vtkIdType );

  // Description:
  // Get the sample size.
  vtkGetMacro( SampleSize, vtkIdType );

  // Description:
  // Set the Learn option.
  vtkSetMacro( Learn, bool );

  // Description:
  // Get the Learn option.
  vtkGetMacro( Learn, bool );

  // Description:
  // Set the Derive option.
  vtkSetMacro( Derive, bool );

  // Description:
  // Get the Derive option.
  vtkGetMacro( Derive, bool );

  // Description:
  // Set the Assess option.
  vtkSetMacro( Assess, bool );

  // Description:
  // Get the Assess option.
  vtkGetMacro( Assess, bool );

//BTX
  // Description:
  // Set the name of a parameter of the Assess option
  void SetAssessParameter( vtkIdType id, vtkStdString name );

  // Description:
  // Get the name of a parameter of the Assess option
  vtkStdString SetAssessParameter( vtkIdType id );

  // Description:
  // A base class for a functor that assesses data.
  class AssessFunctor {
  public:
    virtual void operator() ( vtkVariantArray*,
                              vtkIdType ) = 0;
    virtual ~AssessFunctor() { }
  };

  // Description:
  // A pure virtual method to select the appropriate assessment functor.
  virtual void SelectAssessFunctor( vtkTable* inData, 
                                    vtkTable* inMeta,
                                    vtkStringArray* rowNames,
                                    vtkStringArray* columnNames,
                                    AssessFunctor*& dfunc ) = 0;
//ETX

protected:
  vtkStatisticsAlgorithm();
  ~vtkStatisticsAlgorithm();

  virtual int FillInputPortInformation( int port, vtkInformation* info );
  virtual int FillOutputPortInformation( int port, vtkInformation* info );

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector* );

  // Description:
  // Execute the required calculations in the specified execution modes
  virtual void ExecuteLearn( vtkTable*,
                             vtkTable* ) = 0;
  virtual void ExecuteDerive( vtkTable* ) = 0;
  virtual void ExecuteAssess( vtkTable*,
                              vtkTable*,
                              vtkTable*,
                              vtkTable* ) = 0; 

  vtkIdType NumberOfVariables;
  vtkIdType SampleSize;
  bool Learn;
  bool Derive;
  bool Assess;
  vtkStringArray* AssessParameters;
  vtkStringArray* AssessNames;

private:
  vtkStatisticsAlgorithm(const vtkStatisticsAlgorithm&); // Not implemented
  void operator=(const vtkStatisticsAlgorithm&);   // Not implemented
};

#endif

