/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContingencyStatistics.cxx

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

#include "vtkContingencyStatistics.h"
#include "vtkBivariateStatisticsAlgorithmPrivate.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/map>

#include <vtksys/ios/sstream>

typedef vtkstd::map<vtkVariant,double> PDF;

vtkCxxRevisionMacro(vtkContingencyStatistics, "1.28");
vtkStandardNewMacro(vtkContingencyStatistics);

// ----------------------------------------------------------------------
vtkContingencyStatistics::vtkContingencyStatistics()
{
  this->AssessNames->SetNumberOfValues( 3 );
  this->AssessNames->SetValue( 0, "P" );
  this->AssessNames->SetValue( 1, "Py|x" );
  this->AssessNames->SetValue( 2, "Px|y" );

  this->AssessParameters = vtkStringArray::New();
  this->AssessParameters->SetNumberOfValues( 1 );
  this->AssessParameters->SetValue( 0, "P" );
}

// ----------------------------------------------------------------------
vtkContingencyStatistics::~vtkContingencyStatistics()
{
  this->AssessParameters->Delete();
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
 }

// ----------------------------------------------------------------------
void vtkContingencyStatistics::ExecuteLearn( vtkTable* inData,
                                             vtkTable* outMeta )
{
  if ( ! this->SampleSize )
    {
    return;
    }

  if ( ! inData->GetNumberOfColumns() )
    {
    this->SampleSize = 0;
    return;
    }

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable X" );
  outMeta->AddColumn( stringCol );
  stringCol->Delete();

  stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable Y" );
  outMeta->AddColumn( stringCol );
  stringCol->Delete();

  vtkVariantArray* variantCol = vtkVariantArray::New();
  variantCol->SetName( "x" );
  outMeta->AddColumn( variantCol );
  variantCol->Delete();

  variantCol = vtkVariantArray::New();
  variantCol->SetName( "y" );
  outMeta->AddColumn( variantCol );
  variantCol->Delete();

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Cardinality" );
  outMeta->AddColumn( idTypeCol );
  idTypeCol->Delete();

  vtkVariantArray* row = vtkVariantArray::New();

  row->SetNumberOfValues( 5 );

  typedef vtkstd::map<vtkVariant,vtkIdType,vtkVariantLessThan> Distribution;

  for ( vtkstd::set<vtkstd::pair<vtkStdString,vtkStdString> >::iterator it = this->Internals->Selection.begin(); 
        it != this->Internals->Selection.end(); ++ it )
    {
    vtkStdString colX = it->first;
    if ( ! inData->GetColumnByName( colX ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << colX.c_str() 
                       <<". Ignoring this pair." );
      continue;
      }

    vtkStdString colY = it->second;
    if ( ! inData->GetColumnByName( colY ) )
      {
      vtkWarningMacro( "InData table does not have a column " 
                       << colY.c_str()
                       << ". Ignoring this pair." );
      continue;
      }

    row->SetValue( 0, colX );
    row->SetValue( 1, colY );

    vtkstd::map<vtkVariant,Distribution,vtkVariantLessThan> conTable;
    for ( vtkIdType r = 0; r < this->SampleSize; ++ r )
      {
      ++ conTable[inData->GetValueByName( r, colX )][inData->GetValueByName( r, colY )];
      }

    for ( vtkstd::map<vtkVariant,Distribution,vtkVariantLessThan>::iterator mit = conTable.begin(); 
          mit != conTable.end(); ++ mit )
      {
      row->SetValue( 2, mit->first );
      for ( Distribution::iterator dit = mit->second.begin(); dit != mit->second.end(); ++ dit )
        {
        row->SetValue( 3, dit->first );
        row->SetValue( 4, dit->second );

        outMeta->InsertNextRow( row );
        }
      }
    }
  row->Delete();

  return;
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::ExecuteDerive( vtkTable* inMeta )
{
  vtkIdType nCol = inMeta->GetNumberOfColumns();
  if ( nCol < 5 )
    {
    return;
    }

  vtkIdType nRow = inMeta->GetNumberOfRows();
  if ( ! nRow )
    {
    return;
    }

  vtkStdString doubleName( "P" );

  vtkDoubleArray* doubleCol;
  if ( ! inMeta->GetColumnByName( doubleName ) )
    {
    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( doubleName );
    doubleCol->SetNumberOfTuples( nRow );
    inMeta->AddColumn( doubleCol );
    doubleCol->Delete();
    }

  typedef vtkstd::map<vtkVariant,vtkstd::pair<int,double> > VariantToStats;
  vtkstd::map<vtkStdString,VariantToStats> marginals;
  vtkstd::map<vtkStdString,vtkstd::pair<vtkStdString,vtkStdString> > marginalToPair;

  for ( int i = 0; i < nRow; ++ i )
    {
    vtkStdString c1 = inMeta->GetValueByName( i, "Variable X" ).ToString();
    vtkStdString c2 = inMeta->GetValueByName( i, "Variable Y" ).ToString();
    if ( c1 == "" || c2 == "" )
      {
      // Clean up previously defined marginal probabilities
      inMeta->RemoveRow( i );
      }
    else
      {
      double doubleVal;
      double inv_n = 1. / this->SampleSize;
      vtkVariant x, y;
      int c;
      
      if ( marginalToPair.find( c1 ) == marginalToPair.end() )
        { 
        // c1 has not yet been used as a key... add it with (c1,c2) as the corresponding pair
        marginalToPair[c1].first = c1;
        marginalToPair[c1].second = c2;
        }

      if ( marginalToPair.find( c2 ) == marginalToPair.end() )
        { 
        // c2 has not yet been used as a key... add it with (c1,c2) as the corresponding pair
        marginalToPair[c2].first = c1;
        marginalToPair[c2].second = c2;
        }

      x = inMeta->GetValueByName( i, "x" );
      y = inMeta->GetValueByName( i, "y" );
      c = inMeta->GetValueByName( i, "Cardinality" ).ToInt();
      
      doubleVal = inv_n * c;

      if ( marginalToPair[c1].first == c1 && marginalToPair[c1].second == c2  )
        {
        marginals[c1][x].first  += c;
        marginals[c1][x].second += doubleVal;
        }

      if ( marginalToPair[c2].first == c1 && marginalToPair[c2].second == c2  )
        {
        marginals[c2][y].first  += c;
        marginals[c2][y].second += doubleVal;
        }

      inMeta->SetValueByName( i, doubleName, doubleVal );
      }
    }

  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues( 6 );

  for ( vtkstd::map<vtkStdString,VariantToStats>::iterator sit = marginals.begin();
        sit != marginals.end(); ++ sit )
      {
      for ( VariantToStats::iterator xit = sit->second.begin(); 
            xit != sit->second.end(); ++ xit )
        {
        // Insert marginal cardinalities and probabilities
        row->SetValue( 0, sit->first );         // variable name
        row->SetValue( 1, "" );                 // empty entry for the second variable name
        row->SetValue( 2, xit->first );         // variable value
        row->SetValue( 3, "" );                 // empty entry for the second variable value
        row->SetValue( 4, xit->second.first );  // marginal cardinality
        row->SetValue( 5, xit->second.second ); // marginal probability
        inMeta->InsertNextRow( row );
        }
      }

  row->Delete();
}

// ----------------------------------------------------------------------
class BivariateContingenciesFunctor : public vtkStatisticsAlgorithm::AssessFunctor
{
public:
  vtkAbstractArray* DataX;
  vtkAbstractArray* DataY;
  vtkstd::map<vtkVariant,PDF> PdfXY;
  vtkstd::map<vtkVariant,PDF> PdfYcondX;
  vtkstd::map<vtkVariant,PDF> PdfXcondY;

  BivariateContingenciesFunctor( vtkAbstractArray* valsX,
                                 vtkAbstractArray* valsY,
                                 const vtkstd::map<vtkVariant,PDF>& pdfXY,
                                 const vtkstd::map<vtkVariant,PDF>& pdfYcondX,
                                 const vtkstd::map<vtkVariant,PDF>& pdfXcondY )
  {
    this->DataX = valsX;
    this->DataY = valsY;
    this->PdfXY = pdfXY;
    this->PdfYcondX = pdfYcondX;
    this->PdfXcondY = pdfXcondY;
  }
  virtual ~BivariateContingenciesFunctor() { }
  virtual void operator() ( vtkVariantArray* result,
                            vtkIdType id )
  {
    vtkVariant x = this->DataX->GetVariantValue( id );
    vtkVariant y = this->DataY->GetVariantValue( id );

    result->SetNumberOfValues( 3 );
    result->SetValue( 0, this->PdfXY[x][y] );
    result->SetValue( 1, this->PdfYcondX[x][y] );
    result->SetValue( 2, this->PdfXcondY[x][y] );
  }
};

// ----------------------------------------------------------------------
void vtkContingencyStatistics::SelectAssessFunctor( vtkTable* inData,
                                                    vtkTable* inMeta,
                                                    vtkStringArray* rowNames,
                                                    vtkStringArray* columnNames,
                                                    AssessFunctor*& dfunc )
{
  vtkStdString varNameX = rowNames->GetValue( 0 );
  vtkStdString varNameY = rowNames->GetValue( 1 );

  // Grab the data for the requested variables
  vtkAbstractArray* valsX = inData->GetColumnByName( varNameX );
  vtkAbstractArray* valsY = inData->GetColumnByName( varNameY );
  if ( ! valsX || ! valsY )
    {
    dfunc = 0;
    return;
    }
      
  // PDFs
  PDF pdfX, pdfY;
  vtkstd::map<vtkVariant,PDF> pdfXY;

  // CDFs
  double sX  = 0.;
  double sY  = 0.;
  double sXY = 0.;

  // information entropies
  double HYcondX = 0.;
  double HXcondY = 0.;
  double HXY     = 0.;

  double p;
  vtkStdString paramName = columnNames->GetValue( 0 );
  vtkIdType nRowP = inMeta->GetNumberOfRows();
  for ( int i = 0; i < nRowP; ++ i )
    {
    vtkStdString c1 = inMeta->GetValueByName( i, "Variable X" ).ToString();
    vtkStdString c2 = inMeta->GetValueByName( i, "Variable Y" ).ToString();

    if ( c1 == varNameX ) 
      {
      if ( c2 == varNameY )
        {
        // (c1,c2) = (colX,colY): populate bivariate pdf for (colX,colY)
        p = inMeta->GetValueByName( i, paramName ).ToDouble();
        pdfXY[inMeta->GetValueByName( i, "x" )][inMeta->GetValueByName( i, "y" )] = p;
        sXY += p;
        }
      else if ( c2 == "" )
        {
        // (c1,c2) = (colX,""): populate marginal pdf for colX
        p = inMeta->GetValueByName( i, paramName ).ToDouble();
        pdfX[inMeta->GetValueByName( i, "x" )] = p;
        sX += p;
        }
      }
    else if ( c1 == varNameY && c2 == "" ) 
      {
      // (c1,c2) = (colY,""): populate marginal pdf for colY
      p = inMeta->GetValueByName( i, paramName ).ToDouble();
      pdfY[inMeta->GetValueByName( i, "x" )] = p;
      sY += p;
      }
    }

  if ( fabs( sX - 1. ) > 1.e-6 || fabs( sY - 1. ) > 1.e-6 || fabs( sXY - 1. ) > 1.e-6 )
    {
    vtkWarningMacro( "Incorrect parameters for column pair:"
                     << " model CDFs do not all sum to 1." );
    
    dfunc = 0;
    return;
    }

  // Calculate conditional PDFs and information entropies
  vtkstd::map<vtkVariant,PDF> pdfYcondX;
  vtkstd::map<vtkVariant,PDF> pdfXcondY;
  vtkVariant x,y;

  for ( vtkstd::map<vtkVariant,PDF>::iterator xit = pdfXY.begin();
        xit != pdfXY.end(); ++ xit )
    {
    x = xit->first;
    for ( vtkstd::map<vtkVariant,double>::iterator yit = xit->second.begin(); 
          yit != xit->second.end(); ++ yit )
      {
      y = yit->first;
      p = yit->second;
      pdfYcondX[x][y] = p / pdfX[x];
      pdfXcondY[x][y] = p / pdfY[y];
      
      HXY -= p * log( p );
      HYcondX -= p * log( pdfYcondX[x][y] );
      HXcondY -= p * log( pdfXcondY[x][y] );
      }
    }

  dfunc = new BivariateContingenciesFunctor( valsX,
                                             valsY,
                                             pdfXY,
                                             pdfYcondX,
                                             pdfXcondY );
}
