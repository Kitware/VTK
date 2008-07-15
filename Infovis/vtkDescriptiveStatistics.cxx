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

vtkCxxRevisionMacro(vtkDescriptiveStatistics, "1.37");
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
                                             vtkTable* outMeta )
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
  doubleCol->SetName( "g1 Skewness" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();
  
  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "G1 Skewness" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();
  
  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "g2 Kurtosis" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();
  
  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName( "G2 Kurtosis" );
  outMeta->AddColumn( doubleCol );
  doubleCol->Delete();

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
    double mean = 0.;
    double mom2 = 0.;
    double mom3 = 0.;
    double mom4 = 0.;

    double val, delta, n, A, B;
    double inv_n;
    for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
      {
      val  = inData->GetValueByName( r, col ).ToDouble();

      n = r + 1.;
      inv_n = 1. / n;
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

    double nm1, nm2, variance, skewness, G1, kurtosis, G2;
    if ( this->SampleSize == 1 || mom2 < 1.e-150 )
      {
      variance = 0.;
      skewness = 0.;
      G1 = 0.;
      kurtosis = 0.;
      G2 = 0.;
      }
    else
      {
      nm1 = this->SampleSize - 1.;
      variance = mom2 / nm1;
      double var_inv = 1. / variance;
      double nvar_inv = var_inv * inv_n;
      skewness = nvar_inv * sqrt( var_inv ) * mom3;
      kurtosis = nvar_inv * var_inv * mom4 - 3.;
      if ( n > 2 )
        {
        // G1 skewness estimate
        nm2 = nm1 - 1.;
        G1 = ( n * n ) / ( nm1 * nm2 ) * skewness;
 
        if ( n > 3 )
          { 
          // G2 kurtosis estimate
          G2 = ( ( n + 1. ) * kurtosis + 6. ) * nm1 / ( nm2 * ( nm1 - 2. ) );
          }
        else
          {
          G2 = kurtosis;
          }
        }
      else
        {
        G1 = skewness;
        G2 = kurtosis;
        }
      }

    vtkVariantArray* row = vtkVariantArray::New();

    row->SetNumberOfValues( 10 );
    row->SetValue( 0, col );
    row->SetValue( 1, minVal );
    row->SetValue( 2, maxVal );
    row->SetValue( 3, mean );
    row->SetValue( 4, sqrt( fabs( variance ) ) );
    row->SetValue( 5, variance );
    row->SetValue( 6, skewness );
    row->SetValue( 7, G1 );
    row->SetValue( 8, kurtosis );
    row->SetValue( 9, G2 );

    outMeta->InsertNextRow( row );

    row->Delete();
    }

  return;
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
                                              vtkTable* vtkNotUsed( outMeta ) )
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

  return;
}
