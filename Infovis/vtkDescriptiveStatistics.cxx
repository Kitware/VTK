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
#include "vtkUnivariateStatisticsAlgorithmPrivate.h"

#include "vtkDoubleArray.h"
#include "vtkIDTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/set>
#include <vtksys/ios/sstream> 

vtkCxxRevisionMacro(vtkDescriptiveStatistics, "1.62");
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
  this->AssessParameters->Delete();
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
  this->SetAssessParameter( 0, name ); 
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::SetDeviationParameter( const char* name ) 
{ 
  this->SetAssessParameter( 1, name ); 
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::ExecuteLearn( vtkTable* inData,
                                             vtkDataObject* outMetaDO )
{
  vtkTable* outMeta = vtkTable::SafeDownCast( outMetaDO );
  if ( ! outMeta ) 
    { 
    return; 
    } 

  vtkIdType nRow = inData->GetNumberOfRows();
  if ( ! nRow )
    {
    return;
    }

  if ( ! this->Internals->Selection.size() )
    {
    return;
    }

  vtkIdType nCol = inData->GetNumberOfColumns();
  if ( ! nCol )
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

  for ( vtkstd::set<vtkStdString>::iterator it = this->Internals->Selection.begin(); 
        it != this->Internals->Selection.end(); ++ it )
    {
    vtkStdString colName = *it;
    if ( ! inData->GetColumnByName( colName ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << colName.c_str()
                       << ". Ignoring it." );
      continue;
      }

    double minVal = inData->GetValueByName( 0, colName ).ToDouble();
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

      val = inData->GetValueByName( r, colName ).ToDouble();
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

    row->SetValue( 0, colName );
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
void vtkDescriptiveStatistics::ExecuteDerive( vtkDataObject* inMetaDO )
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

  int numDoubles = 6;
  vtkStdString doubleNames[] = { "Standard Deviation", 
                                 "Variance",
                                 "g1 Skewness",
                                 "G1 Skewness",
                                 "g2 Kurtosis",
                                 "G2 Kurtosis" };

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

  double* doubleVals = new double[numDoubles]; // std, variance, skewness, G1, kurtosis, G2

  for ( int i = 0; i < nRow; ++ i )
    {
    vtkStdString colName = inMeta->GetValueByName( i, "Variable" ).ToString();
    double mom2 = inMeta->GetValueByName( i, "M2" ).ToDouble();
    double mom3 = inMeta->GetValueByName( i, "M3" ).ToDouble();
    double mom4 = inMeta->GetValueByName( i, "M4" ).ToDouble();

    int numSamples = inMeta->GetValueByName(i, "Cardinality" ).ToInt();
    if ( numSamples == 1 || mom2 < 1.e-150 )
      {
      doubleVals[0] = 0.;
      doubleVals[1] = 0.;
      doubleVals[2] = 0.;
      doubleVals[3] = 0.;
      doubleVals[4] = 0.;
      doubleVals[5] = 0.;
      }
    else
      {
      double n = static_cast<double>( numSamples );
      double inv_n = 1. / n;
      double nm1 = n - 1.;

      doubleVals[1] = mom2 / nm1;
      doubleVals[0] = sqrt( doubleVals[1] );

      double var_inv = 1. / doubleVals[1];
      double nvar_inv = var_inv * inv_n;
      doubleVals[2] = nvar_inv * sqrt( var_inv ) * mom3;
      doubleVals[4] = nvar_inv * var_inv * mom4 - 3.;
      if ( n > 2 )
        {
        // G1 skewness estimate
        double nm2 = nm1 - 1.;
        doubleVals[3] = ( n * n ) / ( nm1 * nm2 ) * doubleVals[2];
 
        if ( n > 3 )
          { 
          // G2 kurtosis estimate
          doubleVals[5] = ( ( n + 1. ) * doubleVals[4] + 6. ) * nm1 / ( nm2 * ( nm1 - 2. ) );
          }
        else
          {
          doubleVals[5] = doubleVals[4];
          }
        }
      else
        {
        doubleVals[3] = doubleVals[2];
        doubleVals[5] = doubleVals[4];
        }
      }

    for ( int j = 0; j < numDoubles; ++ j )
      {
      inMeta->SetValueByName( i, doubleNames[j], doubleVals[j] );
      }
    }

  delete [] doubleVals;
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

  // Create the outData columns 
  int nv = this->AssessNames->GetNumberOfValues(); 
  for ( int v = 0; v < nv; ++ v ) 
    { 
    vtksys_ios::ostringstream assessColName; 
    assessColName << this->AssessNames->GetValue( v ) 
                  << "(" 
                  << varName 
                  << ")";
 
    vtkDoubleArray* assessValues = vtkDoubleArray::New(); 
    assessValues->SetName( assessColName.str().c_str() ); 
    assessValues->SetNumberOfTuples( outData->GetNumberOfRows() ); 
    outData->AddColumn( assessValues ); 
    assessValues->Delete(); 
    } 
 
  // Loop over parameters table until the requested variable is found
  vtkIdType nRowP = inMeta->GetNumberOfRows();
  for ( int i = 0; i < nRowP; ++ i )
    {
    if ( inMeta->GetValueByName( i, "Variable" ).ToString() == varName )
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

      double nominal   = inMeta->GetValueByName( i, this->AssessParameters->GetValue( 0 ) ).ToDouble();
      double deviation = inMeta->GetValueByName( i, this->AssessParameters->GetValue( 1 ) ).ToDouble();

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
