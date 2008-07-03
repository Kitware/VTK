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

vtkCxxRevisionMacro(vtkDescriptiveStatistics, "1.33");
vtkStandardNewMacro(vtkDescriptiveStatistics);

// ----------------------------------------------------------------------
vtkDescriptiveStatistics::vtkDescriptiveStatistics()
{
  this->SignedDeviations = 1;
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
void vtkDescriptiveStatistics::ExecuteLearn( vtkTable* inData,
                                             vtkTable* outMeta,
                                             bool finalize )
{
  vtkIdType nCol = inData->GetNumberOfColumns();
  if ( ! nCol )
    {
    this->SampleSize = 0;
    return;
    }

  this->SampleSize = inData->GetNumberOfRows();
  if ( ! this->SampleSize )
    {
    return;
    }

  if ( ! this->Internals->SelectedColumns.size() )
    {
    return;
    }

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable" );
  outMeta->AddColumn( stringCol );
  stringCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Minimum" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "Maximum" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

  if ( finalize )
    {
    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Mean" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Standard Deviation" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Variance" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Skewness" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sample Kurtosis" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "G2 Kurtosis" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();
    }
  else
    {
    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum x" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum x2" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum x3" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "Sum x4" );
    outMeta->AddColumn( doubleCol );
    doubleCol->Delete();

    vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
    idTypeCol->SetName( "Cardinality" );
    outMeta->AddColumn( idTypeCol );
    idTypeCol->Delete();
    }

  for ( vtkstd::set<vtkStdString>::iterator it = this->Internals->SelectedColumns.begin(); 
        it != this->Internals->SelectedColumns.end(); ++ it )
    {
    vtkStdString col = *it;
    if ( ! inData->GetColumnByName( col ) )
      {
      vtkWarningMacro( "InData table does not have a column "<<col.c_str()<<". Ignoring it." );
      continue;
      }

    double minVal = inData->GetValueByName( 0, col ).ToDouble();
    double maxVal = minVal;

    double val  = 0.;
    double val2 = 0.;
    double sum1 = 0.;
    double sum2 = 0.;
    double sum3 = 0.;
    double sum4 = 0.;
    for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
      {
      val  = inData->GetValueByName( r, col ).ToDouble();
      val2 = val * val;
      sum1 += val;
      sum2 += val2;
      sum3 += val2 * val;
      sum4 += val2 * val2;
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

    if ( finalize )
      {
      double sd;
      double G2;
      this->CalculateFromSums( this->SampleSize, sum1, sum2, sum3, sum4, sd, G2 );

      row->SetNumberOfValues( 9 );
      row->SetValue( 0, col );
      row->SetValue( 1, minVal );
      row->SetValue( 2, maxVal );
      row->SetValue( 3, sum1 );
      row->SetValue( 4, sd );
      row->SetValue( 5, sum2 );
      row->SetValue( 6, sum3 );
      row->SetValue( 7, sum4 );
      row->SetValue( 8, G2 );
      }
    else
      {
      row->SetNumberOfValues( 8 );
      row->SetValue( 0, col );
      row->SetValue( 1, minVal );
      row->SetValue( 2, maxVal );
      row->SetValue( 3, sum1 );
      row->SetValue( 4, sum2 );
      row->SetValue( 5, sum3 );
      row->SetValue( 6, sum4 );
      row->SetValue( 7, this->SampleSize );
      }

    outMeta->InsertNextRow( row );

    row->Delete();
    }

  return;
}

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::ExecuteValidate( vtkTable*,
                                                vtkTable*,
                                                vtkTable* )
{
  // Not implemented for this statistical engine
}

// ----------------------------------------------------------------------
class DataArrayDeviantFunctor : public vtkDescriptiveStatistics::DeviantFunctor
{
public:
  vtkDataArray* Array;
};

class SignedDataArrayDeviantFunctor : public DataArrayDeviantFunctor
{
public:
  SignedDataArrayDeviantFunctor( vtkDataArray* arr, double nominal, double stdev )
    {
    this->Array = arr;
    this->Nominal = nominal;
    this->Deviation = stdev;
    }
  virtual ~SignedDataArrayDeviantFunctor() { }
  virtual double operator() ( vtkIdType row )
    {
    return ( this->Array->GetTuple( row )[0] - this->Nominal ) / this->Deviation;
    }
};

class UnsignedDataArrayDeviantFunctor : public DataArrayDeviantFunctor
{
public:
  UnsignedDataArrayDeviantFunctor( vtkDataArray* arr, double nominal, double stdev )
    {
    this->Array = arr;
    this->Nominal = nominal;
    this->Deviation = stdev;
    }
  virtual ~UnsignedDataArrayDeviantFunctor() { }
  virtual double operator() ( vtkIdType row )
    {
    return fabs ( this->Array->GetTuple( row )[0] - this->Nominal ) / this->Deviation;
    }
};

// ----------------------------------------------------------------------
class AbstractArrayDeviantFunctor : public vtkDescriptiveStatistics::DeviantFunctor
{
public:
  vtkAbstractArray* Array;
};

// When the deviation is 0, we can't normalize. Instead, a non-zero value (1)
// is returned only when the nominal value is matched exactly.
class ZedDeviationDeviantFunctor : public AbstractArrayDeviantFunctor
{
public:
  ZedDeviationDeviantFunctor( vtkAbstractArray* arr, double nominal )
    {
    this->Array = arr;
    this->Nominal = nominal;
    }
  virtual ~ZedDeviationDeviantFunctor() { }
  virtual double operator() ( vtkIdType row )
    {
    return ( this->Array->GetVariantValue( row ).ToDouble() == this->Nominal ) ? 0. : 1.;
    }
};

class SignedAbstractArrayDeviantFunctor : public AbstractArrayDeviantFunctor
{
public:
  SignedAbstractArrayDeviantFunctor( vtkAbstractArray* arr, double nominal, double stdev )
    {
    this->Array = arr;
    this->Nominal = nominal;
    this->Deviation = stdev;
    }
  virtual ~SignedAbstractArrayDeviantFunctor() { }
  virtual double operator() ( vtkIdType row )
    {
    return ( this->Array->GetVariantValue( row ).ToDouble() - this->Nominal ) / this->Deviation;
    }
};

class UnsignedAbstractArrayDeviantFunctor : public AbstractArrayDeviantFunctor
{
public:
  UnsignedAbstractArrayDeviantFunctor( vtkAbstractArray* arr, double nominal, double stdev )
    {
    this->Array = arr;
    this->Nominal = nominal;
    this->Deviation = stdev;
    }
  virtual ~UnsignedAbstractArrayDeviantFunctor() { }
  virtual double operator() ( vtkIdType row )
    {
    return fabs ( this->Array->GetVariantValue( row ).ToDouble() - this->Nominal ) / this->Deviation;
    }
};

// ----------------------------------------------------------------------
void vtkDescriptiveStatistics::ExecuteAssess( vtkTable* inData,
                                              vtkTable* inMeta,
                                              vtkTable* outData,
                                              vtkTable* outMeta )
{
  vtkIdType nColD = inData->GetNumberOfColumns();
  if ( ! nColD )
    {
    return;
    }

  vtkIdType nRowD = inData->GetNumberOfRows();
  if ( ! nRowD )
    {
    return;
    }

  vtkIdType nColP = inMeta->GetNumberOfColumns();
  if ( nColP < 3 )
    {
    vtkWarningMacro( "Parameter table has " 
                     << nColP
                     << " < 3 columns. Doing nothing." );
    return;
    }

  vtkIdType nRowP = inMeta->GetNumberOfRows();
  if ( ! nRowP )
    {
    return;
    }

  if ( ! this->Internals->SelectedColumns.size() )
    {
    return;
    }

  // Loop over rows of the parameter table looking for columns that
  // specify the mean and standard deviation of some requested table column.
  for ( int i = 0; i < nRowP; ++ i )
    {
    // Is the parameter one that's been requested?
    vtkStdString colName = inMeta->GetValue( i, 0 ).ToString();
    vtkstd::set<vtkStdString>::iterator it =
      this->Internals->SelectedColumns.find( colName );
    if ( it == this->Internals->SelectedColumns.end() )
      { // Have parameter values. But user doesn't want it... skip it.
      continue;
      }
    double nominal = inMeta->GetValueByName( i, "Mean" ).ToDouble();
    double deviation = inMeta->GetValueByName( i, "Standard Deviation" ).ToDouble();

    // Does the requested array exist in the input inData?
    vtkAbstractArray* arr;
    if ( ! ( arr = inData->GetColumnByName( colName ) ) )
      { // User requested it. InMeta table has it. But inData doesn't... whine
      vtkWarningMacro(
        "InData table does not have a column "
        << colName.c_str() << ". Ignoring it." );
      continue;
      }
    vtkDataArray* darr = vtkDataArray::SafeDownCast( arr );

    // Create the outData column
    vtkDoubleArray* relativeDeviations = vtkDoubleArray::New();
    vtksys_ios::ostringstream devColName;
    devColName << "Relative Deviation of " << colName;
    relativeDeviations->SetName( devColName.str().c_str() );
    relativeDeviations->SetNumberOfTuples( nRowD );

    DeviantFunctor* dfunc = 0;
    if ( deviation == 0. )
      {
      dfunc = new ZedDeviationDeviantFunctor( arr, nominal );
      }
    else
      {
      if ( darr )
        {
        if ( this->GetSignedDeviations() )
          {
          dfunc = new SignedDataArrayDeviantFunctor( darr, nominal, deviation );
          }
        else
          {
          dfunc = new UnsignedDataArrayDeviantFunctor( darr, nominal, deviation );
          }
        }
      else
        {
        if ( this->GetSignedDeviations() )
          {
          dfunc = new SignedAbstractArrayDeviantFunctor( arr, nominal, deviation );
          }
        else
          {
          dfunc = new UnsignedAbstractArrayDeviantFunctor( arr, nominal, deviation );
          }
        }
      }

    // Compute the deviation of each entry for the column
    double dev;
    for ( vtkIdType r = 0; r < nRowD; ++ r )
      {
      dev = (*dfunc)( r );
      relativeDeviations->SetValue( r, dev );
      }
    delete dfunc;

    // Add the column to the outData
    outData->AddColumn( relativeDeviations );
    relativeDeviations->Delete();
    }

  outMeta->ShallowCopy( inMeta );
}

// ----------------------------------------------------------------------
int vtkDescriptiveStatistics::CalculateFromSums( int n, 
                                                 double& s1,
                                                 double& s2,
                                                 double& s3,
                                                 double& s4,
                                                 double& sd,
                                                 double& G2 )
{
  if ( n < 1 ) 
    {
    return -1;
    }

  double nd = static_cast<double>( n );

  // (unbiased) estimation of the mean
  s1 /= nd;

  if ( n == 1 )
    {
    s2 = 0.;
    sd = 0.;
    s3 = 0.;
    s4 = 0.;
    G2 = 0.;
    return 1;
    }

  // (unbiased) estimation of the variance
  double nm1 = nd - 1.;
  double s1p2 = s1 * s1;
  double var = ( s2 - s1p2 * nd ) / nm1;

  if ( var > 0. )
    {
    // sample estimation of the kurtosis "excess"
    s4 = ( s4 / nd - 4. * s1 * s3 / nd + 6. * s1p2 * s2 / nd - 3. * s1p2 * s1p2 )
      / ( var * var ) - 3.;
    
    // sample estimation of the skewness
    s3 = ( s3 / nd - 3. * s1 * s2 / nd + 2. * s1p2 * s1 ) 
      / pow( var, 1.5 );

    s2 = var;
    sd = sqrt( s2 );
    }
  else
    {
    s2 = var;
    sd = 0.;
    s3 = 0.;
    s4 = 0.;
    G2 = 0.;
    return 1;
    }

  // G2 estimation of the kurtosis "excess"
  if ( n > 3 )
    {
    G2 = ( ( nd + 1. ) * s4 + 6. ) * nm1 / ( ( nd - 2. ) * ( nd - 3. ) );
    return 0;
    }
  else
    {
    G2 = s4;
    return 1;
    }
}


