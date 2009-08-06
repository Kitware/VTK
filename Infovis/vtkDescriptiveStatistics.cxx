/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDescriptiveStatistics.cxx

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

#include "vtkToolkits.h"

#include "vtkDescriptiveStatistics.h"
#include "vtkStatisticsAlgorithmPrivate.h"

#include "vtkDataObjectCollection.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/set>
#include <vtksys/ios/sstream> 

vtkCxxRevisionMacro(vtkDescriptiveStatistics, "1.85");
vtkStandardNewMacro(vtkDescriptiveStatistics);

// ----------------------------------------------------------------------
vtkDescriptiveStatistics::vtkDescriptiveStatistics()
{
  this->AssessNames->SetNumberOfValues( 1 );
  this->AssessNames->SetValue( 0, "d" ); // relative deviation, i.e., when unsigned, 1D Mahlanobis distance

  this->AssessParameters = vtkStringArray::New();
  this->AssessParameters->SetNumberOfValues( 2 );
  this->AssessParameters->SetValue( 0, "Mean" );
  this->AssessParameters->SetValue( 1, "Standard Deviation" );
  this->SignedDeviations = 0; // By default, use unsigned deviation (1D Mahlanobis distance)
}

// ----------------------------------------------------------------------
vtkDescriptiveStatistics::~vtkDescriptiveStatistics()
{
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "SignedDeviations: " << this->SignedDeviations << "\n";
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::SetNominalParameter( const char* name ) 
{ 
  this->SetAssessOptionParameter( 0, name ); 
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::SetDeviationParameter( const char* name ) 
{ 
  this->SetAssessOptionParameter( 1, name ); 
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::Aggregate( vtkDataObjectCollection* inMetaColl,
                                          vtkDataObject* outMetaDO )
{
  vtkTable* outMeta = vtkTable::SafeDownCast( outMetaDO );
  if ( ! outMeta ) 
    { 
    return; 
    } 

  // Get hold of the first model (data object) in the collection
  vtkCollectionSimpleIterator it;
  inMetaColl->InitTraversal( it );
  vtkDataObject *inMetaDO = inMetaColl->GetNextDataObject( it );

  // Verify that the model is indeed contained in a table
  vtkTable* inMeta = vtkTable::SafeDownCast( inMetaDO );
  if ( ! inMeta ) 
    { 
    return; 
    }

  vtkIdType nRow = inMeta->GetNumberOfRows();
  if ( ! nRow )
    {
    // No statistics were calculated.
    return;
    }

  // Use this first model to initialize the aggregated one
  outMeta->DeepCopy( inMeta );

  // Now, loop over all remaining models and update aggregated each time
  while ( ( inMetaDO = inMetaColl->GetNextDataObject( it ) ) )
    {
    // Verify that the model is indeed contained in a table
    inMeta = vtkTable::SafeDownCast( inMetaDO );
    if ( ! inMeta ) 
      { 
      return; 
      }
    
    if ( inMeta->GetNumberOfRows() != nRow )
      {
      // Models do not match
      return;
      }

    // Iterate over all model rows
    for ( int r = 0; r < nRow; ++ r )
      {
      // Verify that variable names match each other
      if ( inMeta->GetValueByName( r, "Variable" ) != outMeta->GetValueByName( r, "Variable" ) )
        {
        // Models do not match
        return;
        }

      // Get aggregated statistics
      int n = outMeta->GetValueByName( r, "Cardinality" ).ToInt();
      double min = outMeta->GetValueByName( r, "Minimum" ).ToDouble();
      double max = outMeta->GetValueByName( r, "Maximum" ).ToDouble();
      double mean = outMeta->GetValueByName( r, "Mean" ).ToDouble();
      double M2 = outMeta->GetValueByName( r, "M2" ).ToDouble();
      double M3 = outMeta->GetValueByName( r, "M3" ).ToDouble();
      double M4 = outMeta->GetValueByName( r, "M4" ).ToDouble();
      
      // Get current model statistics
      int n_c = inMeta->GetValueByName( r, "Cardinality" ).ToInt();
      double min_c = inMeta->GetValueByName( r, "Minimum" ).ToDouble();
      double max_c = inMeta->GetValueByName( r, "Maximum" ).ToDouble();
      double mean_c = inMeta->GetValueByName( r, "Mean" ).ToDouble();
      double M2_c = inMeta->GetValueByName( r, "M2" ).ToDouble();
      double M3_c = inMeta->GetValueByName( r, "M3" ).ToDouble();
      double M4_c = inMeta->GetValueByName( r, "M4" ).ToDouble();
      
      // Update global statics
      int N = n + n_c; 

      if ( min_c < min )
        {
        outMeta->SetValueByName( r, "Minimum", min_c );
        }

      if ( max_c > max )
        {
        outMeta->SetValueByName( r, "Maximum", max_c );
        }

      double delta = mean_c - mean;
      double delta_sur_N = delta / static_cast<double>( N );
      double delta2_sur_N2 = delta_sur_N * delta_sur_N;

      int n2 = n * n;
      int n_c2 = n_c * n_c;
      int prod_n = n * n_c;
 
      M4 += M4_c 
        + prod_n * ( n2 - prod_n + n_c2 ) * delta * delta_sur_N * delta2_sur_N2
        + 6. * ( n2 * M2_c + n_c2 * M2 ) * delta2_sur_N2
        + 4. * ( n * M3_c - n_c * M3 ) * delta_sur_N;

      M3 += M3_c 
        + prod_n * ( n - n_c ) * delta * delta2_sur_N2
        + 3. * ( n * M2_c - n_c * M2 ) * delta_sur_N;

      M2 += M2_c 
        + prod_n * delta * delta_sur_N;

      mean += n_c * delta_sur_N;

      // Store updated model
      outMeta->SetValueByName( r, "Cardinality", N );
      outMeta->SetValueByName( r, "Mean", mean );
      outMeta->SetValueByName( r, "M2", M2 );
      outMeta->SetValueByName( r, "M3", M3 );
      outMeta->SetValueByName( r, "M4", M4 );
      }
    }

  return;
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::Learn( vtkTable* inData,
                                      vtkTable* vtkNotUsed( inParameters ),
                                      vtkDataObject* outMetaDO )
{
  vtkTable* outMeta = vtkTable::SafeDownCast( outMetaDO );
  if ( ! outMeta ) 
    { 
    return; 
    } 

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable" );
  outMeta->AddColumn( stringCol );
  stringCol->Delete();

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Cardinality" );
  outMeta->AddColumn( idTypeCol );
  idTypeCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Minimum" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Maximum" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Mean" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "M2" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "M3" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "M4" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  vtkIdType nRow = inData->GetNumberOfRows();
  if ( ! nRow )
    {
    return;
    }

  if ( ! this->Internals->Requests.size() )
    {
    return;
    }

  vtkIdType nCol = inData->GetNumberOfColumns();
  if ( ! nCol )
    {
    return;
    }
  
  // Loop over requests
  for ( vtkstd::set<vtkstd::set<vtkStdString> >::iterator rit = this->Internals->Requests.begin(); 
        rit != this->Internals->Requests.end(); ++ rit )
    {
    // Each request contains only one column of interest (if there are others, they are ignored)
    vtkstd::set<vtkStdString>::iterator it = rit->begin();
    vtkStdString varName = *it;
    if ( ! inData->GetColumnByName( varName ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << varName.c_str()
                       << ". Ignoring it." );
      continue;
      }

    double minVal = inData->GetValueByName( 0, varName ).ToDouble();
    double maxVal = minVal;
    double mean = 0.;
    double mom2 = 0.;
    double mom3 = 0.;
    double mom4 = 0.;

    double n, inv_n, val, delta, A, B;
    for ( vtkIdType r = 0; r < nRow; ++ r )
      {
      n = r + 1.;
      inv_n = 1. / n;

      val = inData->GetValueByName( r, varName ).ToDouble();
      delta = val - mean;

      A = delta * inv_n; 
      mean += A;
      mom4 += A * ( A * A * delta * r * ( n * ( n - 3. ) + 3. ) + 6. * A * mom2 - 4. * mom3  );

      B = val - mean;
      mom3 += A * ( B * delta * ( n - 2. ) - 3. * mom2 );
      mom2 += delta * B;

      if ( val < minVal )
        {
        minVal = val;
        }
      else if ( val > maxVal )
        {
        maxVal = val;
        }
      }

    vtkVariantArray* row = vtkVariantArray::New();

    row->SetNumberOfValues( 8 );

    row->SetValue( 0, varName );
    row->SetValue( 1, nRow );
    row->SetValue( 2, minVal );
    row->SetValue( 3, maxVal );
    row->SetValue( 4, mean );
    row->SetValue( 5, mom2 );
    row->SetValue( 6, mom3 );
    row->SetValue( 7, mom4 );

    outMeta->InsertNextRow( row );

    row->Delete();
    }

  return;
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::Derive( vtkDataObject* inMetaDO )
{
  vtkTable* inMeta = vtkTable::SafeDownCast( inMetaDO ); 
  if ( ! inMeta ) 
    { 
    return; 
    } 

  vtkIdType nCol = inMeta->GetNumberOfColumns();
  if ( nCol < 8 )
    {
    return;
    }

  vtkIdType nRow = inMeta->GetNumberOfRows();
  if ( ! nRow )
    {
    return;
    }

  int numDoubles = 7;
  vtkStdString doubleNames[] = { "Standard Deviation",
                                 "Variance",
                                 "g1 Skewness",
                                 "G1 Skewness",
                                 "g2 Kurtosis",
                                 "G2 Kurtosis",
                                 "Sum" };

  vtkDoubleArray* doubleCol;
  for ( int j = 0; j < numDoubles; ++ j )
    {
    if ( ! inMeta->GetColumnByName( doubleNames[j] ) )
      {
      doubleCol = vtkDoubleArray::New();
      doubleCol->SetName( doubleNames[j] );
      doubleCol->SetNumberOfTuples( nRow );
      inMeta->AddColumn( doubleCol );
      doubleCol->Delete();
      }
    }

  // Storage for standard deviation, variance, skewness, G1, kurtosis, G2, sum
  double* derivedVals = new double[numDoubles]; 

  for ( int i = 0; i < nRow; ++ i )
    {
    double mom2 = inMeta->GetValueByName( i, "M2" ).ToDouble();
    double mom3 = inMeta->GetValueByName( i, "M3" ).ToDouble();
    double mom4 = inMeta->GetValueByName( i, "M4" ).ToDouble();

    int numSamples = inMeta->GetValueByName(i, "Cardinality" ).ToInt();

    if ( numSamples == 1 || mom2 < 1.e-150 )
      {
      derivedVals[0] = 0.;
      derivedVals[1] = 0.;
      derivedVals[2] = 0.;
      derivedVals[3] = 0.;
      derivedVals[4] = 0.;
      derivedVals[5] = 0.;
      }
    else
      {
      double n = static_cast<double>( numSamples );
      double inv_n = 1. / n;
      double nm1 = n - 1.;

      derivedVals[1] = mom2 / nm1;
      derivedVals[0] = sqrt( derivedVals[1] );

      double var_inv = 1. / derivedVals[1];
      double nvar_inv = var_inv * inv_n;
      derivedVals[2] = nvar_inv * sqrt( var_inv ) * mom3;
      derivedVals[4] = nvar_inv * var_inv * mom4 - 3.;
      if ( n > 2 )
        {
        // G1 skewness estimate
        double nm2 = nm1 - 1.;
        derivedVals[3] = ( n * n ) / ( nm1 * nm2 ) * derivedVals[2];
 
        if ( n > 3 )
          { 
          // G2 kurtosis estimate
          derivedVals[5] = ( ( n + 1. ) * derivedVals[4] + 6. ) * nm1 / ( nm2 * ( nm1 - 2. ) );
          }
        else
          {
          derivedVals[5] = derivedVals[4];
          }
        }
      else
        {
        derivedVals[3] = derivedVals[2];
        derivedVals[5] = derivedVals[4];
        }
      }

    // Sum
    derivedVals[6] = numSamples * inMeta->GetValueByName( i, "Mean" ).ToDouble();

    for ( int j = 0; j < numDoubles; ++ j )
      {
      inMeta->SetValueByName( i, doubleNames[j], derivedVals[j] );
      }
    }

  delete [] derivedVals;
}

// ----------------------------------------------------------------------
class TableColumnDeviantFunctor : public vtkStatisticsAlgorithm::AssessFunctor
{
public:
  vtkDataArray* Data;
  double Nominal;
  double Deviation;
};

// When the deviation is 0, we can't normalize. Instead, a non-zero value (1)
// is returned only when the nominal value is matched exactly.
class ZedDeviationDeviantFunctor : public TableColumnDeviantFunctor
{
public:
  ZedDeviationDeviantFunctor( vtkDataArray* vals, 
                              double nominal )
  {
    this->Data = vals;
    this->Nominal = nominal;
  }
  virtual ~ZedDeviationDeviantFunctor() { }
  virtual void operator() ( vtkVariantArray* result,
                            vtkIdType id )
  {
    result->SetNumberOfValues( 1 );
    result->SetValue( 0, ( this->Data->GetTuple1( id ) == this->Nominal ) ? 0. : 1. );
  }
};

class SignedTableColumnDeviantFunctor : public TableColumnDeviantFunctor
{
public:
  SignedTableColumnDeviantFunctor( vtkDataArray* vals, 
                                   double nominal, 
                                   double deviation )
  {
    this->Data = vals;
    this->Nominal = nominal;
    this->Deviation = deviation;
  }
  virtual ~SignedTableColumnDeviantFunctor() { }
  virtual void operator() ( vtkVariantArray* result,
                            vtkIdType id )
  {
    result->SetNumberOfValues( 1 );
    result->SetValue( 0, ( this->Data->GetTuple1( id ) - this->Nominal ) / this->Deviation );
  }
};

class UnsignedTableColumnDeviantFunctor : public TableColumnDeviantFunctor
{
public:
  UnsignedTableColumnDeviantFunctor( vtkDataArray* vals, 
                                     double nominal, 
                                     double deviation )
  {
    this->Data = vals;
    this->Nominal = nominal;
    this->Deviation = deviation;
  }
  virtual ~UnsignedTableColumnDeviantFunctor() { }
  virtual void operator() ( vtkVariantArray* result,
                            vtkIdType id )
  {
    result->SetNumberOfValues( 1 );
    result->SetValue( 0, fabs ( this->Data->GetTuple1( id ) - this->Nominal ) / this->Deviation );
  }
};

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::SelectAssessFunctor( vtkTable* outData,
                                                    vtkDataObject* inMetaDO,
                                                    vtkStringArray* rowNames,
                                                    AssessFunctor*& dfunc )
{
  vtkTable* inMeta = vtkTable::SafeDownCast( inMetaDO ); 
  if ( ! inMeta ) 
    { 
    return; 
    }

  vtkStdString varName = rowNames->GetValue( 0 );

  // Downcast meta columns to string arrays for efficient data access
  vtkStringArray* vars = vtkStringArray::SafeDownCast( inMeta->GetColumnByName( "Variable" ) );
  if ( ! vars )
    {
    dfunc = 0;
    return;
    }

  // Loop over parameters table until the requested variable is found
  vtkIdType nRowP = inMeta->GetNumberOfRows();
  for ( int r = 0; r < nRowP; ++ r )
    {
    if ( vars->GetValue( r ) == varName )
      {
      // Grab the data for the requested variable
      vtkAbstractArray* arr = outData->GetColumnByName( varName );
      if ( ! arr )
        {
        dfunc = 0;
        return;
        }
      
      // For descriptive statistics, type must be convertible to DataArray (e.g., StringArrays do not fit here).
      vtkDataArray* vals = vtkDataArray::SafeDownCast( arr );
      if ( ! vals )
        {
        dfunc = 0;
        return;
        }

      double nominal   = inMeta->GetValueByName( r, this->AssessParameters->GetValue( 0 ) ).ToDouble();
      double deviation = inMeta->GetValueByName( r, this->AssessParameters->GetValue( 1 ) ).ToDouble();

      if ( deviation == 0. )
        {
        dfunc = new ZedDeviationDeviantFunctor( vals, nominal );
        }
      else
        {
        if ( this->GetSignedDeviations() )
          {
          dfunc = new SignedTableColumnDeviantFunctor( vals, nominal, deviation );
          }
        else
          {
          dfunc = new UnsignedTableColumnDeviantFunctor( vals, nominal, deviation );
          }
        }

      return;
      }
    }

  // The variable of interest was not found in the parameter table
  dfunc = 0;
}
