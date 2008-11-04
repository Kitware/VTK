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

typedef vtkstd::map<vtkVariant,vtkIdType> Counts;
typedef vtkstd::map<vtkVariant,double> PDF;

vtkCxxRevisionMacro(vtkContingencyStatistics, "1.32");
vtkStandardNewMacro(vtkContingencyStatistics);

// ----------------------------------------------------------------------
vtkContingencyStatistics::vtkContingencyStatistics()
{
  this->AssessNames->SetNumberOfValues( 3 );
  this->AssessNames->SetValue( 0, "P" );
  this->AssessNames->SetValue( 1, "Py|x" );
  this->AssessNames->SetValue( 2, "Px|y" );

  this->AssessParameters = vtkStringArray::New();
  this->AssessParameters->SetNumberOfValues( 3 );
  this->AssessParameters->SetValue( 0, "P" );
  this->AssessParameters->SetValue( 1, "Py|x" );
  this->AssessParameters->SetValue( 2, "Px|y" );
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

  int numDoubles = 3;
  vtkStdString doubleNames[] = { "P",
                                 "Py|x",
                                 "Px|y" };
  
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

  // Calculate marginal counts (marginal PDFs are calculated at storage time to avoid redundant summations)
  vtkstd::map<vtkStdString,vtkstd::pair<vtkStdString,vtkStdString> > marginalToPair;
  vtkstd::map<vtkStdString,Counts> marginalCounts;
  double inv_n = 1. / this->SampleSize;
  vtkVariant x, y;
  for ( int i = 0; i < nRow; ++ i )
    {
    vtkStdString c1 = inMeta->GetValueByName( i, "Variable X" ).ToString();
    vtkStdString c2 = inMeta->GetValueByName( i, "Variable Y" ).ToString();
    if ( c1 == "" || c2 == "" )
      {
      // Clean up previously defined marginal probabilities and information entropies
      inMeta->RemoveRow( i );
      }
    else
      {
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
      vtkIdType c = inMeta->GetValueByName( i, "Cardinality" ).ToInt();
      
      if ( marginalToPair[c1].first == c1 && marginalToPair[c1].second == c2  )
        {
        if ( x != "" )
          {
          marginalCounts[c1][x] += c;
          }
        }

      if ( marginalToPair[c2].first == c1 && marginalToPair[c2].second == c2  )
        {
        if ( y != "" )
          {
          marginalCounts[c2][y] += c;
          }
        }
      }
    }

  // Append marginal probabilities to the model table
  vtkstd::map<vtkStdString,PDF> marginalPDFs;
  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues( 8 );
  double p;

  for ( vtkstd::map<vtkStdString,Counts>::iterator sit = marginalCounts.begin();
        sit != marginalCounts.end(); ++ sit )
      {
      for ( Counts::iterator xit = sit->second.begin(); 
            xit != sit->second.end(); ++ xit )
        {
        // Calculate and retain marginal PDF
        p = inv_n * xit->second;
        marginalPDFs[sit->first][xit->first] = p;

        // Insert marginal cardinalities and probabilities
        row->SetValue( 0, sit->first );    // variable name
        row->SetValue( 1, "" );            // empty entry for the second variable name
        row->SetValue( 2, xit->first );    // variable value
        row->SetValue( 3, "" );            // empty entry for the second variable value
        row->SetValue( 4, xit->second );   // marginal cardinality
        row->SetValue( 5, p );             // marginal probability
        row->SetValue( 6, -1. );           // invalid entry for the first conditional probability
        row->SetValue( 7, -1. );           // invalid entry for the second conditional probability
        inMeta->InsertNextRow( row );
        }
      }

  // Calculate PDFs and information entropies
  double* doubleVals = new double[numDoubles]; // P(x,y), P(y|x), P(x|y)
  
  typedef vtkstd::map<vtkStdString,vtkstd::map<vtkStdString,double> > Entropies;
  Entropies *H = new Entropies[numDoubles];  // H(X,Y), H(Y|X), H(X|Y)
  vtkstd::map<vtkStdString,vtkstd::map<vtkStdString,vtkIdType> > sanityChecks;

  for ( int i = 0; i < nRow; ++ i )
    {
    vtkStdString c1 = inMeta->GetValueByName( i, "Variable X" ).ToString();
    vtkStdString c2 = inMeta->GetValueByName( i, "Variable Y" ).ToString();
    if ( c1 == "" || c2 == "" )
      {
      continue;
      }

    vtkIdType c = inMeta->GetValueByName( i, "Cardinality" ).ToInt();
    doubleVals[0] = inv_n * c;

    x = inMeta->GetValueByName( i, "x" );
    y = inMeta->GetValueByName( i, "y" );

    if ( x != "" && y != "" )
      {
      sanityChecks[c1][c2] += c;

      doubleVals[1] = doubleVals[0] / marginalPDFs[c1][x];
      doubleVals[2] = doubleVals[0] / marginalPDFs[c2][y];
      
      for ( int j = 0; j < numDoubles; ++ j )
        {
        // Update information entropies
        H[j][c1][c2] -= doubleVals[0] * log( doubleVals[j] );

        // Store conditional probabilities -- but not information entropies
        inMeta->SetValueByName( i, doubleNames[j], doubleVals[j] );
        }
      }
    }

  // Store information entropies
  for ( Entropies::iterator eit = H[0].begin(); eit != H[0].end(); ++ eit )
      {
      for ( vtkstd::map<vtkStdString,double>::iterator xit = eit->second.begin(); 
            xit != eit->second.end(); ++ xit )
        {
        row->SetValue( 0, eit->first );                           // X variable name
        row->SetValue( 1, xit->first );                           // Y variable name
        row->SetValue( 2, "" );                                   // empty entry for the X variable value
        row->SetValue( 3, "" );                                   // empty entry for the Y variable value
        row->SetValue( 4, sanityChecks[eit->first][xit->first] ); // must be equal to sample size
        row->SetValue( 5, xit->second );                          // H(X,Y)
        row->SetValue( 6, H[1][eit->first][xit->first] );         // H(Y|X)
        row->SetValue( 7, H[2][eit->first][xit->first] );         // H(X|Y)
        inMeta->InsertNextRow( row );
        }
      }

  row->Delete();
  delete [] H;
  delete [] doubleVals;
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
      
  // Values
  vtkVariant x, y;

  // PDFs
  vtkstd::map<vtkVariant,PDF> pdf[3];

  // Joint CDF 
  double cdf = 0.;

  vtkIdType nRowP = inMeta->GetNumberOfRows();
  for ( int i = 0; i < nRowP; ++ i )
    {
    vtkStdString c1 = inMeta->GetValueByName( i, "Variable X" ).ToString();
    vtkStdString c2 = inMeta->GetValueByName( i, "Variable Y" ).ToString();

    if ( c1 == "" || c2 == "" )
      {
      continue;
      }

    if ( c1 == varNameX && c2 == varNameY )
      {
      x = inMeta->GetValueByName( i, "x" );
      y = inMeta->GetValueByName( i, "y" );

      if ( x != "" && y != "" ) // NB: the latter implies the former if the meta table is well-formed...
        {
        for ( int j = 0; j < 3; ++ j )
          {
          pdf[j][x][y] = inMeta->GetValueByName( i, columnNames->GetValue( j ) ).ToDouble();

          if ( ! j )
            {
            cdf += pdf[j][x][y];
            }
          }
        } // if ( x != "" && y != "" )
      else if ( x == "" && y == "" ) // NB: the former implies the latter if the meta table is well-formed...
        {
        // If row contains information entropies, check cardinalities (cf. sanityChecks above)
        if ( inMeta->GetValueByName( i, "Cardinality" ) != this->SampleSize )
          {
          vtkWarningMacro( "Incorrect information entropy for column pair:"
                           << varNameX.c_str()
                           << ","
                           << varNameY.c_str()
                           << "). Ignoring it." );
          
          dfunc = 0;
          return;
          }
        } // else if ( x == "" && y == "" )
      } // if ( c1 == varNameX && c2 == varNameY )
    } // for ( int i = 0; i < nRowP; ++ i )

  if ( fabs( cdf - 1. ) > 1.e-6 )
    {
    vtkWarningMacro( "Incorrect CDF for column pair:"
                     << varNameX.c_str()
                     << ","
                     << varNameY.c_str()
                     << "). Ignoring it." );
    
    dfunc = 0;
    return;
    }

  dfunc = new BivariateContingenciesFunctor( valsX,
                                             valsY,
                                             pdf[0],
                                             pdf[1],
                                             pdf[2] );
}
