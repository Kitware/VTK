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
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
// .NAME vtkStatisticsAlgorithm - Base class for statistics algorithms
//
// .SECTION Description
// All statistics algorithms can conceptually be operated with several operations:
// * Learn: given an input data set, calculate a minimal statistical model (e.g.,
//   sums, raw moments, joint probabilities).
// * Derive: given an input minimal statistical model, derive the full model
//   (e.g., descriptive statistics, quantiles, correlations, conditional
//    probabilities).
//   NB: It may be, or not be, a problem that a full model was not derived. For
//   instance, when doing parallel calculations, one only wants to derive the full
//   model after all partial calculations have completed. On the other hand, one
//   can also directly provide a full model, that was previously calculated or
//   guessed, and not derive a new one.
// * Assess: given an input data set, input statistics, and some form of
//   threshold, assess a subset of the data set.
// * Test: perform at least one statistical test.
// Therefore, a vtkStatisticsAlgorithm has the following ports
// * 3 optional input ports:
//   * Data (vtkTable)
//   * Parameters to the learn operation (vtkTable)
//   * Input model (vtkMultiBlockDataSet)
// * 3 output ports:
//   * Data (input annotated with assessments when the Assess operation is ON).
//   * Output model (identical to the the input model when Learn operation is OFF).
//   * Output of statistical tests. Some engines do not offer such tests yet, in
//     which case this output will always be empty even when the Test operation is ON.
//
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
// for implementing this class.
// Updated by Philippe Pebay, Kitware SAS 2012

#ifndef __vtkStatisticsAlgorithm_h
#define __vtkStatisticsAlgorithm_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkTableAlgorithm.h"

class vtkDataObjectCollection;
class vtkMultiBlockDataSet;
class vtkStdString;
class vtkStringArray;
class vtkVariant;
class vtkVariantArray;
class vtkStatisticsAlgorithmPrivate;

class VTKFILTERSSTATISTICS_EXPORT vtkStatisticsAlgorithm : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkStatisticsAlgorithm, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // enumeration values to specify input port types
  enum InputPorts
    {
    INPUT_DATA = 0,         //!< Port 0 is for learn data
    LEARN_PARAMETERS = 1,   //!< Port 1 is for learn parameters (initial guesses, etc.)
    INPUT_MODEL = 2         //!< Port 2 is for a priori models
    };

  // Description:
  // enumeration values to specify output port types
  enum OutputIndices
    {
    OUTPUT_DATA  = 0,       //!< Output 0 mirrors the input data, plus optional assessment columns
    OUTPUT_MODEL = 1,       //!< Output 1 contains any generated model
#ifndef VTK_LEGACY_REMOVE
    ASSESSMENT   = 2,       //!< This is an old, deprecated name for OUTPUT_TEST.
#endif
    OUTPUT_TEST  = 2        //!< Output 2 contains result of statistical test(s)
    };
//ETX

  // Description:
  // A convenience method for setting learn input parameters (if one is expected or allowed).
  // It is equivalent to calling SetInputConnection( 1, params );
  virtual void SetLearnOptionParameterConnection( vtkAlgorithmOutput* params )
    { this->SetInputConnection( vtkStatisticsAlgorithm::LEARN_PARAMETERS, params ); }

  // Description:
  // A convenience method for setting learn input parameters (if one is expected or allowed).
  // It is equivalent to calling SetInputData( 1, params );
  virtual void SetLearnOptionParameters( vtkDataObject* params )
    { this->SetInputData( vtkStatisticsAlgorithm::LEARN_PARAMETERS, params ); }

  // Description:
  // A convenience method for setting the input model connection (if one is expected or allowed).
  // It is equivalent to calling SetInputConnection( 2, model );
  virtual void SetInputModelConnection( vtkAlgorithmOutput* model )
    { this->SetInputConnection( vtkStatisticsAlgorithm::INPUT_MODEL, model ); }

  // Description:
  // A convenience method for setting the input model (if one is expected or allowed).
  // It is equivalent to calling SetInputData( 2, model );
  virtual void SetInputModel( vtkDataObject* model )
    { this->SetInputData( vtkStatisticsAlgorithm::INPUT_MODEL, model ); }

  // Description:
  // Set/Get the Learn operation.
  vtkSetMacro( LearnOption, bool );
  vtkGetMacro( LearnOption, bool );

  // Description:
  // Set/Get the Derive operation.
  vtkSetMacro( DeriveOption, bool );
  vtkGetMacro( DeriveOption, bool );

  // Description:
  // Set/Get the Assess operation.
  vtkSetMacro( AssessOption, bool );
  vtkGetMacro( AssessOption, bool );

  // Description:
  // Set/Get the Test operation.
  vtkSetMacro( TestOption, bool );
  vtkGetMacro( TestOption, bool );

  // Description:
  // Set/Get the number of tables in the primary model.
  vtkSetMacro( NumberOfPrimaryTables, vtkIdType );
  vtkGetMacro( NumberOfPrimaryTables, vtkIdType );

  // Description:
  // Set/get assessment names.
  virtual void SetAssessNames( vtkStringArray* );
  vtkGetObjectMacro(AssessNames,vtkStringArray);

//BTX
  // Description:
  // A base class for a functor that assesses data.
  class AssessFunctor {
  public:
    virtual void operator() ( vtkVariantArray*,
                              vtkIdType ) = 0;
    virtual ~AssessFunctor() { }
  };
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

  // Description:
  // Return the number of requests.
  // This does not include any request that is in the column-status buffer
  // but for which RequestSelectedColumns() has not yet been called (even though
  // it is possible this request will be honored when the filter is run -- see SetColumnStatus()
  // for more information).
  virtual vtkIdType GetNumberOfRequests();

  // Description:
  // Return the number of columns for a given request.
  virtual vtkIdType GetNumberOfColumnsForRequest( vtkIdType request );

  // Description:
  // Provide the name of the \a c-th column for the \a r-th request.
  //
  // For the version of this routine that returns an integer,
  // if the request or column does not exist because \a r or \a c is out of bounds,
  // this routine returns 0 and the value of \a columnName is unspecified.
  // Otherwise, it returns 1 and the value of \a columnName is set.
  //
  // For the version of this routine that returns const char*,
  // if the request or column does not exist because \a r or \a c is out of bounds,
  // the routine returns NULL. Otherwise it returns the column name.
  // This version is not thread-safe.
  virtual const char* GetColumnForRequest( vtkIdType r, vtkIdType c );
  //BTX
  virtual int GetColumnForRequest( vtkIdType r, vtkIdType c, vtkStdString& columnName );
  //ETX

  // Description:
  // Convenience method to create a request with a single column name \p namCol in a single
  // call; this is the preferred method to select columns, ensuring selection consistency
  // (a single column per request).
  // Warning: no name checking is performed on \p namCol; it is the user's
  // responsibility to use valid column names.
  void AddColumn( const char* namCol );

  // Description:
  // Convenience method to create a request with a single column name pair
  //  (\p namColX, \p namColY) in a single call; this is the preferred method to select
  // columns pairs, ensuring selection consistency (a pair of columns per request).
  //
  // Unlike SetColumnStatus(), you need not call RequestSelectedColumns() after AddColumnPair().
  //
  // Warning: \p namColX and \p namColY are only checked for their validity as strings;
  // no check is made that either are valid column names.
  void AddColumnPair( const char* namColX, const char* namColY );

  // Description:
  // A convenience method (in particular for access from other applications) to
  // set parameter values of Learn mode.
  // Return true if setting of requested parameter name was excuted, false otherwise.
  // NB: default method (which is sufficient for most statistics algorithms) does not
  // have any Learn parameters to set and always returns false.
  virtual bool SetParameter( const char* parameter,
                             int index,
                             vtkVariant value );

  // Description:
  // Given a collection of models, calculate aggregate model
  virtual void Aggregate( vtkDataObjectCollection*,
                          vtkMultiBlockDataSet* ) = 0;

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
  // Execute the calculations required by the Learn option, given some input Data
  virtual void Learn( vtkTable*,
                      vtkTable*,
                      vtkMultiBlockDataSet* ) = 0;

  // Description:
  // Execute the calculations required by the Derive option.
  virtual void Derive( vtkMultiBlockDataSet* ) = 0;

  // Description:
  // Execute the calculations required by the Assess option.
  virtual void Assess( vtkTable*,
                       vtkMultiBlockDataSet*,
                       vtkTable* ) = 0;

  // Description:
  // A convenience implementation for generic assessment with variable number of variables.
  void Assess( vtkTable*,
               vtkMultiBlockDataSet*,
               vtkTable*,
               int );

  // Description:
  // Execute the calculations required by the Test option.
  virtual void Test( vtkTable*,
                     vtkMultiBlockDataSet*,
                     vtkTable* ) = 0;

  //BTX
  // Description:
  // A pure virtual method to select the appropriate assessment functor.
  virtual void SelectAssessFunctor( vtkTable* outData,
                                    vtkDataObject* inMeta,
                                    vtkStringArray* rowNames,
                                    AssessFunctor*& dfunc ) = 0;
  //ETX

  int NumberOfPrimaryTables;
  bool LearnOption;
  bool DeriveOption;
  bool AssessOption;
  bool TestOption;
  vtkStringArray* AssessNames;
  vtkStatisticsAlgorithmPrivate* Internals;

private:
  vtkStatisticsAlgorithm(const vtkStatisticsAlgorithm&); // Not implemented
  void operator=(const vtkStatisticsAlgorithm&);   // Not implemented
};

#endif
