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
class vtkStatisticsAlgorithmPrivate;

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

  // Description:
  // Let the user know whether the full statistical model (when available) was
  // indeed derived from the underlying minimal model.
  // NB: It may be, or not be, a problem that a full model was not derived. For
  // instance, when doing parallel calculations, one only wants to derive the full
  // model after all partial calculations have completed. On the other hand, one
  // can also directly provide a full model, that was previously calculated or
  // guessed, and not derive a new one; in this case, IsFullModelDerived() will
  // always return false, but this does not mean that the full model is invalid 
  // (nor does it mean that it is valid).
  virtual int IsFullModelDerived() {return this->FullWasDerived;}

//BTX
  // Description:
  // Set the name of a parameter of the Assess option
  void SetAssessParameter( vtkIdType id, vtkStdString name );

  // Description:
  // Get the name of a parameter of the Assess option
  vtkStdString GetAssessParameter( vtkIdType id );

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
  virtual void SelectAssessFunctor( vtkTable* outData, 
                                    vtkDataObject* inMeta,
                                    vtkStringArray* rowNames,
                                    AssessFunctor*& dfunc ) = 0;
//ETX

  // Description:
  // Add or remove a column from the current analysis request.
  // Once all the column status values are set, call RequestSelectedColumns()
  // before selecting another set of columns for a different analysis request.
  // The way that columns selections are used varies from algorithm to algorithm.
  //
  // Note: the set of selected columns is maintained in vtkStatisticsAlgorithmPrivate::Buffer
  // until RequestSelectedColumns() is called, at which point the set is appended
  // to vtkStatisticsAlgorithmPrivate::Requests.
  // If there are any columns in vtkStatisticsAlgorithmPrivate::Buffer at the time
  // RequestData() is called, RequestSelectedColumns() will be called and the
  // selection added to the list of requests.
  virtual void SetColumnStatus( const char* namCol, int status );

  // Description:
  // Set the the status of each and every column in the current request to OFF (0).
  virtual void ResetAllColumnStates();

  // Description:
  // Use the current column status values to produce a new request for statistics
  // to be produced when RequestData() is called. See SetColumnStatus() for more information.
  virtual int RequestSelectedColumns();

  // Description:
  // Empty the list of current requests.
  virtual void ResetRequests();

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
                             vtkDataObject* ) = 0;
  virtual void ExecuteDerive( vtkDataObject* ) = 0;
  virtual void ExecuteAssess( vtkTable*,
                              vtkDataObject*,
                              vtkTable*,
                              vtkDataObject* ) = 0; 

  vtkIdType NumberOfVariables;
  bool Learn;
  bool Derive;
  bool Assess;
  bool FullWasDerived;
  vtkStringArray* AssessParameters;
  vtkStringArray* AssessNames;
  vtkStatisticsAlgorithmPrivate* Internals;

private:
  vtkStatisticsAlgorithm(const vtkStatisticsAlgorithm&); // Not implemented
  void operator=(const vtkStatisticsAlgorithm&);   // Not implemented
};

#endif

