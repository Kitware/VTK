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
  Copyright 2010 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/

#include "vtkToolkits.h"

#include "vtkContingencyStatistics.h"
#include "vtkStatisticsAlgorithmPrivate.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#ifdef VTK_USE_GNU_R
#include <vtkRInterface.h>
#endif // VTK_USE_GNU_R
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtksys/stl/map>
#include <vtksys/stl/vector>

#include <vtksys/ios/sstream>

#define VTK_STATISTICS_NUMBER_OF_VARIABLES 2

typedef vtksys_stl::map<vtkStdString,vtkIdType> Counts;
typedef vtksys_stl::map<vtkStdString,double> PDF;

vtkStandardNewMacro(vtkContingencyStatistics);

// ----------------------------------------------------------------------
vtkContingencyStatistics::vtkContingencyStatistics()
{
  // This engine has 2 primary tables: summary and contingency table
  this->NumberOfPrimaryTables = 2;

  this->AssessNames->SetNumberOfValues( 4 );
  this->AssessNames->SetValue( 0, "P" );
  this->AssessNames->SetValue( 1, "Py|x" );
  this->AssessNames->SetValue( 2, "Px|y" );
  this->AssessNames->SetValue( 3, "PMI" );

  this->AssessParameters = vtkStringArray::New();
  this->AssessParameters->SetNumberOfValues( 4 );
  this->AssessParameters->SetValue( 0, "P" );
  this->AssessParameters->SetValue( 1, "Py|x" );
  this->AssessParameters->SetValue( 2, "Px|y" );
  this->AssessParameters->SetValue( 3, "PMI" );
}

// ----------------------------------------------------------------------
vtkContingencyStatistics::~vtkContingencyStatistics()
{
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::Learn( vtkTable* inData,
                                      vtkTable* vtkNotUsed( inParameters ),
                                      vtkMultiBlockDataSet* outMeta )
{
  if ( ! inData )
    {
    return;
    }

  if ( ! outMeta )
    {
    return;
    }

  // Summary table: assigns a unique key to each (variable X,variable Y) pair
  vtkTable* summaryTab = vtkTable::New();

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable X" );
  summaryTab->AddColumn( stringCol );
  stringCol->Delete();

  stringCol = vtkStringArray::New();
  stringCol->SetName( "Variable Y" );
  summaryTab->AddColumn( stringCol );
  stringCol->Delete();

  // The actual contingency table, indexed by the key of the summary
  vtkTable* contingencyTab = vtkTable::New();

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Key" );
  contingencyTab->AddColumn( idTypeCol );
  idTypeCol->Delete();

  stringCol = vtkStringArray::New();
  stringCol->SetName( "x" );
  contingencyTab->AddColumn( stringCol );
  stringCol->Delete();

  stringCol = vtkStringArray::New();
  stringCol->SetName( "y" );
  contingencyTab->AddColumn( stringCol );
  stringCol->Delete();

  idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName( "Cardinality" );
  contingencyTab->AddColumn( idTypeCol );
  idTypeCol->Delete();

  // Row to be used to insert into summary table
  vtkVariantArray* row2 = vtkVariantArray::New();
  row2->SetNumberOfValues( 2 );

  // Row to be used to insert into contingency table
  vtkVariantArray* row4 = vtkVariantArray::New();
  row4->SetNumberOfValues( 4 );

  // Insert first row which will always contain the data set cardinality, with key -1
  // NB: The cardinality is calculated in derive mode ONLY, and is set to an invalid value of -1 in
  // learn mode to make it clear that it is not a correct value. This is an issue of database
  // normalization: including the cardinality to the other counts can lead to inconsistency, in particular
  // when the input meta table is calculated by something else than the learn mode (e.g., is specified
  // by the user).
  vtkStdString zString = vtkStdString( "" );
  row4->SetValue( 0, -1 );
  row4->SetValue( 1, zString );
  row4->SetValue( 2, zString );
  row4->SetValue( 3, -1 );
  contingencyTab->InsertNextRow( row4 );

  // Loop over requests
  vtkIdType nRow = inData->GetNumberOfRows();
  for ( vtksys_stl::set<vtksys_stl::set<vtkStdString> >::const_iterator rit = this->Internals->Requests.begin();
        rit != this->Internals->Requests.end(); ++ rit )
    {
    // Each request contains only one pair of column of interest (if there are others, they are ignored)
    vtksys_stl::set<vtkStdString>::const_iterator it = rit->begin();
    vtkStdString colX = *it;
    if ( ! inData->GetColumnByName( colX ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << colX.c_str()
                       << ". Ignoring this pair." );
      continue;
      }

    ++ it;
    vtkStdString colY = *it;
    if ( ! inData->GetColumnByName( colY ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << colY.c_str()
                       << ". Ignoring this pair." );
      continue;
      }

    // Create entry in summary for pair (colX,colY) and set its index to be the key
    // for (colX,colY) values in the contingency table
    row2->SetValue( 0, colX );
    row2->SetValue( 1, colY );
    row4->SetValue( 0, summaryTab->GetNumberOfRows() );
    summaryTab->InsertNextRow( row2 );

    vtkAbstractArray* valsX = inData->GetColumnByName( colX );
    vtkAbstractArray* valsY = inData->GetColumnByName( colY );

    // Calculate contingency table
    vtksys_stl::map<vtkStdString,Counts> contingencyTable;
    for ( vtkIdType r = 0; r < nRow; ++ r )
      {
      ++ contingencyTable
        [valsX->GetVariantValue( r ).ToString()]
        [valsY->GetVariantValue( r ).ToString()];
      }

    // Store contingency table
    for ( vtksys_stl::map<vtkStdString,Counts>::iterator mit = contingencyTable.begin();
          mit != contingencyTable.end(); ++ mit )
      {
      row4->SetValue( 1, mit->first );
      for ( Counts::iterator dit = mit->second.begin(); dit != mit->second.end(); ++ dit )
        {
        row4->SetValue( 2, dit->first );
        row4->SetValue( 3, dit->second );

        contingencyTab->InsertNextRow( row4 );
        }
      }
    }

  // Finally set blocks of the output meta port
  outMeta->SetNumberOfBlocks( 2 );
  outMeta->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Summary" );
  outMeta->SetBlock( 0, summaryTab );
  outMeta->GetMetaData( static_cast<unsigned>( 1 ) )->Set( vtkCompositeDataSet::NAME(), "Contingency Table" );
  outMeta->SetBlock( 1, contingencyTab );

  // Clean up
  summaryTab->Delete();
  contingencyTab->Delete();
  row2->Delete();
  row4->Delete();
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::Derive( vtkMultiBlockDataSet* inMeta )
{
  if ( ! inMeta || inMeta->GetNumberOfBlocks() < 2 )
    {
    return;
    }

  vtkTable* summaryTab = vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) );
  if ( ! summaryTab  )
    {
    return;
    }

  vtkTable* contingencyTab = vtkTable::SafeDownCast( inMeta->GetBlock( 1 ) );
  if ( ! contingencyTab )
    {
    return;
    }

  int nEntropy = 3;
  vtkStdString entropyNames[] = { "H(X,Y)",
                                  "H(Y|X)",
                                  "H(X|Y)" };

  // Create table for derived meta statistics
  vtkIdType nRowSumm = summaryTab->GetNumberOfRows();
  vtkDoubleArray* doubleCol;
  for ( int j = 0; j < nEntropy; ++ j )
    {
    if ( ! summaryTab->GetColumnByName( entropyNames[j] ) )
      {
      doubleCol = vtkDoubleArray::New();
      doubleCol->SetName( entropyNames[j] );
      doubleCol->SetNumberOfTuples( nRowSumm );
      summaryTab->AddColumn( doubleCol );
      doubleCol->Delete();
      }
    }

  // Create columns of derived statistics
  int nDerivedVals = 4;
  vtkStdString derivedNames[] = { "P",
                                  "Py|x",
                                  "Px|y",
                                  "PMI" };

  vtkIdType nRowCont = contingencyTab->GetNumberOfRows();
  for ( int j = 0; j < nDerivedVals; ++ j )
    {
    if ( ! contingencyTab->GetColumnByName( derivedNames[j] ) )
      {
      doubleCol = vtkDoubleArray::New();
      doubleCol->SetName( derivedNames[j] );
      doubleCol->SetNumberOfTuples( nRowCont );
      contingencyTab->AddColumn( doubleCol );
      doubleCol->Delete();
      }
    }

  // Downcast columns to typed arrays for efficient data access
  vtkStringArray* varX = vtkStringArray::SafeDownCast( summaryTab->GetColumnByName( "Variable X" ) );
  vtkStringArray* varY = vtkStringArray::SafeDownCast( summaryTab->GetColumnByName( "Variable Y" ) );
  vtkIdTypeArray* keys = vtkIdTypeArray::SafeDownCast( contingencyTab->GetColumnByName( "Key" ) );
  vtkStringArray* valx = vtkStringArray::SafeDownCast( contingencyTab->GetColumnByName( "x" ) );
  vtkStringArray* valy = vtkStringArray::SafeDownCast( contingencyTab->GetColumnByName( "y" ) );
  vtkIdTypeArray* card = vtkIdTypeArray::SafeDownCast( contingencyTab->GetColumnByName( "Cardinality" ) );
  vtkDoubleArray** derivedCols = new vtkDoubleArray*[nDerivedVals];
  for ( int j = 0; j < nDerivedVals; ++ j )
    {
    derivedCols[j] = vtkDoubleArray::SafeDownCast( contingencyTab->GetColumnByName( derivedNames[j] ) );

    if ( ! derivedCols[j] )
      {
      vtkErrorMacro("Empty model column(s). Cannot derive model.\n");
      return;
      }
    }

  // Temporary counters, used to check that all pairs of variables have indeed the same number of observations
  vtksys_stl::map<vtkIdType,vtkIdType> cardinalities;

  // Calculate marginal counts (marginal PDFs are calculated at storage time to avoid redundant summations)
  vtksys_stl::map<vtkStdString,vtksys_stl::pair<vtkStdString,vtkStdString> > marginalToPair;
  vtksys_stl::map<vtkStdString,Counts> marginalCounts;
  vtkStdString x, y, c1, c2;
  vtkIdType key, c;
  for ( int r = 1; r < nRowCont; ++ r ) // Skip first row which contains data set cardinality
    {
    // Find the pair of variables to which the key corresponds
    key = keys->GetValue( r );

    if ( key < 0 || key >= nRowSumm )
      {
      vtkErrorMacro( "Inconsistent input: dictionary does not have a row "
                     <<  key
                     <<". Cannot derive model." );
      return;
      }

    c1 = varX->GetValue( key );
    c2 = varY->GetValue( key );

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

    x = valx->GetValue( r );
    y = valy->GetValue( r );
    c = card->GetValue( r );
    cardinalities[key] += c;

    if ( marginalToPair[c1].first == c1 && marginalToPair[c1].second == c2  )
      {
      marginalCounts[c1][x] += c;
      }

    if ( marginalToPair[c2].first == c1 && marginalToPair[c2].second == c2  )
      {
      marginalCounts[c2][y] += c;
      }
    }

  // Data set cardinality: unknown yet, pick the cardinality of the first pair and make sure all other pairs
  // have the same cardinality.
  vtkIdType n = cardinalities[0];
  for ( vtksys_stl::map<vtkIdType,vtkIdType>::iterator iit = cardinalities.begin();
        iit != cardinalities.end(); ++ iit )
    {
    if ( iit->second != n )
      {
      vtkErrorMacro( "Inconsistent input: variable pairs do not have equal cardinalities: "
                     << iit->first
                     << " != "
                     << n
                     <<". Cannot derive model." );
      return;
      }
    }

  // We have a unique value for the cardinality and can henceforth proceed
  contingencyTab->SetValueByName( 0, "Cardinality", n );

  // Fill cardinality row (0) with invalid values for derived statistics
  for ( int i = 0; i < nDerivedVals; ++ i )
    {
    contingencyTab->SetValueByName( 0, derivedNames[i], -1. );
    }

  // Resize output meta so marginal PDF tables can be appended
  unsigned int nBlocks = inMeta->GetNumberOfBlocks();
  inMeta->SetNumberOfBlocks( nBlocks + static_cast<unsigned int>( marginalCounts.size() ) );

  // Rows of the marginal PDF tables contain:
  // 0: variable value
  // 1: marginal cardinality
  // 2: marginal probability
  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues( 3 );

  // Add marginal PDF tables as new blocks to the meta output starting at block nBlock
  // NB: block nBlock is kept for information entropy
  double inv_n = 1. / n;
  vtksys_stl::map<vtkStdString,PDF> marginalPDFs;
  for ( vtksys_stl::map<vtkStdString,Counts>::iterator sit = marginalCounts.begin();
        sit != marginalCounts.end(); ++ sit, ++ nBlocks )
    {
    vtkTable* marginalTab = vtkTable::New();

    vtkStringArray* stringCol = vtkStringArray::New();
    stringCol->SetName( sit->first.c_str() );
    marginalTab->AddColumn( stringCol );
    stringCol->Delete();

    vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
    idTypeCol->SetName( "Cardinality" );
    marginalTab->AddColumn( idTypeCol );
    idTypeCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName( "P" );
    marginalTab->AddColumn( doubleCol );
    doubleCol->Delete();

    double p;
    for ( Counts::iterator xit = sit->second.begin();
          xit != sit->second.end(); ++ xit )
      {
      // Calculate and retain marginal PDF
      p = inv_n * xit->second;
      marginalPDFs[sit->first][xit->first] = p;

      // Insert marginal cardinalities and probabilities
      row->SetValue( 0, xit->first );    // variable value
      row->SetValue( 1, xit->second );   // marginal cardinality
      row->SetValue( 2, p );             // marginal probability
      marginalTab->InsertNextRow( row );
      }

    // Add marginal PDF block
    inMeta->GetMetaData( nBlocks )->Set( vtkCompositeDataSet::NAME(), sit->first.c_str() );
    inMeta->SetBlock( nBlocks, marginalTab );

    // Clean up
    marginalTab->Delete();
    }

  // Container for derived values
  double* derivedVals = new double[nDerivedVals];

  // Container for information entropies
  typedef vtksys_stl::map<vtkIdType,double> Entropies;
  Entropies *H = new Entropies[nEntropy];

  // Calculate joint and conditional PDFs, and information entropies
  double p1, p2;
  for ( int r = 1; r < nRowCont; ++ r ) // Skip first row which contains data set cardinality
    {
    // Find the pair of variables to which the key corresponds
    key = keys->GetValue( r );

    // Paranoid check: this test is not necessary since it has already been performed above
    // and the DB should not have been corrupted since. However, it does not cost much anyway.
    if ( key < 0 || key >= nRowSumm )
      {
      vtkErrorMacro( "Inconsistent input: dictionary does not have a row "
                     <<  key
                     <<". Cannot derive model." );
      return;
      }

    // Get values
    c1 = varX->GetValue( key );
    c2 = varY->GetValue( key );

    // Get primary statistics for (c1,c2) pair
    x = valx->GetValue( r );
    y = valy->GetValue( r );
    c = card->GetValue( r );

    // Calculate P(c1,c2)
    derivedVals[0] = inv_n * c;

    // Get marginal PDF values and their product
    p1 = marginalPDFs[c1][x];
    p2 = marginalPDFs[c2][y];

    // Calculate P(c2|c1)
    derivedVals[1] = derivedVals[0] / p1;

    // Calculate P(c1|c2)
    derivedVals[2] = derivedVals[0] / p2;

    // Store P(c1,c2), P(c2|c1), P(c1|c2) and use them to update H(X,Y), H(Y|X), H(X|Y)
    for ( int j = 0; j < nEntropy; ++ j )
      {
      // Store probabilities
      derivedCols[j]->SetValue( r, derivedVals[j] );

      // Update information entropies
      H[j][key] -= derivedVals[0] * log( derivedVals[j] );
      }

    // Calculate and store PMI(c1,c2)
    derivedVals[3] = log( derivedVals[0] / ( p1 * p2 ) );
    derivedCols[3]->SetValue( r, derivedVals[3] );
    }

  // Store information entropies
  for ( Entropies::iterator eit = H[0].begin(); eit != H[0].end(); ++ eit )
    {
    summaryTab->SetValueByName( eit->first, entropyNames[0], eit->second );      // H(X,Y)
    summaryTab->SetValueByName( eit->first, entropyNames[1], H[1][eit->first] ); // H(Y|X)
    summaryTab->SetValueByName( eit->first, entropyNames[2], H[2][eit->first] ); // H(X|Y)
    }

  // Clean up
  row->Delete();
  delete [] H;
  delete [] derivedCols;
  delete [] derivedVals;
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::Test( vtkTable* inData,
                                     vtkMultiBlockDataSet* inMeta,
                                     vtkTable* outMeta )
{
  if ( ! inMeta )
    {
    return;
    }

  vtkTable* summaryTab = vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) );
  if ( ! summaryTab )
    {
    return;
    }

  vtkTable* contingencyTab = vtkTable::SafeDownCast( inMeta->GetBlock( 1 ) );
  if ( ! contingencyTab  )
    {
    return;
    }

  if ( ! outMeta )
    {
    return;
    }

  if ( ! inData )
    {
    return;
    }

  // The test table, indexed by the key of the summary
  vtkTable* testTab = vtkTable::New();

  // Prepare columns for the test:
  // 0: dimension
  // 1: Chi square statistic
  // 2: Chi square statistic with Yates correction
  // 3: Chi square p-value
  // 4: Chi square with Yates correction p-value
  // NB: These are not added to the output table yet, for they will be filled individually first
  //     in order that R be invoked only once.
  vtkIdTypeArray* dimCol = vtkIdTypeArray::New();
  dimCol->SetName( "d" );

  vtkDoubleArray* chi2Col = vtkDoubleArray::New();
  chi2Col->SetName( "Chi2" );

  vtkDoubleArray* chi2yCol = vtkDoubleArray::New();
  chi2yCol->SetName( "Chi2 Yates" );

  // Downcast columns to typed arrays for efficient data access
  vtkStringArray* varX = vtkStringArray::SafeDownCast( summaryTab->GetColumnByName( "Variable X" ) );
  vtkStringArray* varY = vtkStringArray::SafeDownCast( summaryTab->GetColumnByName( "Variable Y" ) );
  vtkIdTypeArray* keys = vtkIdTypeArray::SafeDownCast( contingencyTab->GetColumnByName( "Key" ) );
  vtkStringArray* valx = vtkStringArray::SafeDownCast( contingencyTab->GetColumnByName( "x" ) );
  vtkStringArray* valy = vtkStringArray::SafeDownCast( contingencyTab->GetColumnByName( "y" ) );
  vtkIdTypeArray* card = vtkIdTypeArray::SafeDownCast( contingencyTab->GetColumnByName( "Cardinality" ) );

  // Loop over requests
  vtkIdType nRowSumm = summaryTab->GetNumberOfRows();
  vtkIdType nRowCont = contingencyTab->GetNumberOfRows();
  for ( vtksys_stl::set<vtksys_stl::set<vtkStdString> >::const_iterator rit = this->Internals->Requests.begin();
        rit != this->Internals->Requests.end(); ++ rit )
    {
    // Each request contains only one pair of column of interest (if there are others, they are ignored)
    vtksys_stl::set<vtkStdString>::const_iterator it = rit->begin();
    vtkStdString varNameX = *it;
    if ( ! inData->GetColumnByName( varNameX ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << varNameX.c_str()
                       << ". Ignoring this pair." );
      continue;
      }

    ++ it;
    vtkStdString varNameY = *it;
    if ( ! inData->GetColumnByName( varNameY ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << varNameY.c_str()
                       << ". Ignoring this pair." );
      continue;
      }

    // Find the summary key to which the pair (colX,colY) corresponds
    vtkIdType pairKey = -1;
    for ( vtkIdType r = 0; r < nRowSumm && pairKey == -1; ++ r )
      {
      if ( varX->GetValue( r ) == varNameX
           &&
           varY->GetValue( r ) == varNameY )
        {
        pairKey = r;
        }
      }
    if ( pairKey < 0 || pairKey >= nRowSumm )
      {
      vtkErrorMacro( "Inconsistent input: dictionary does not have a row "
                     << pairKey
                     <<". Cannot test." );
      return;
      }

    // Start by fetching joint counts

    // Sanity check: make sure all counts sum to grand total
    vtkIdType n = card->GetValue( 0 );
    vtkIdType sumij = 0;

    // Loop over parameters table until the requested variables are found
    vtksys_stl::map<vtkStdString,Counts> oij;
    vtkStdString x, y;
    vtkIdType key, c;
    for ( int r = 1; r < nRowCont; ++ r ) // Skip first row which contains data set cardinality
      {
      // Find the pair of variables to which the key corresponds
      key = keys->GetValue( r );

      // Only use entries in the contingency table that correspond to values we are interested in
      if ( key != pairKey )
        {
        continue;
        }

      // Fill PDF and update CDF
      x = valx->GetValue( r );
      y = valy->GetValue( r );
      c = card->GetValue( r );
      oij[x][y] = c;
      sumij += c;
      } // for ( int r = 1; r < nRowCont; ++ r )

    // Sanity check: verify that sum = grand total
    if ( sumij != n )
      {
      vtkWarningMacro( "Inconsistent sum of counts and grand total for column pair "
                       << varNameX.c_str()
                       << ","
                       << varNameY.c_str()
                       << "): "
                       << sumij
                       << " <> "
                       << n
                       << ". Cannot test." );

      return;
      }

    // Now search for relevant marginal counts
    Counts ek[2];
    int foundCount = 0;
    for ( unsigned int b = 2; b < inMeta->GetNumberOfBlocks()  && foundCount < 2; ++ b )
      {
      const char* name = inMeta->GetMetaData( static_cast<unsigned>( b ) )->Get( vtkCompositeDataSet::NAME() );
      int foundIndex = -1;
      if ( ! strcmp( name, varNameX ) )
        {
        // Found the marginal count of X
        foundIndex = 0;
        ++ foundCount;
        }
      else if ( ! strcmp( name, varNameY ) )
        {
        // Found the marginal count of Y
        foundIndex = 1;
        ++ foundCount;
        }

      if ( foundIndex > -1 )
        {
        // One relevant PDF was found
        vtkTable* marginalTab = vtkTable::SafeDownCast( inMeta->GetBlock( b ) );

        // Downcast columns to appropriate arrays for efficient data access
        vtkStringArray* vals = vtkStringArray::SafeDownCast( marginalTab->GetColumnByName( name ) );
        vtkIdTypeArray* marg = vtkIdTypeArray::SafeDownCast( marginalTab->GetColumnByName( "Cardinality" ) );

        // Now iterate over all entries and fill count map
        vtkIdType nRow = marginalTab->GetNumberOfRows();
        for ( vtkIdType r = 0; r < nRow; ++ r )
          {
          ek[foundIndex][vals->GetValue( r )] = marg->GetValue( r );
          }
        }
      }

    // Eliminating the case where one or both marginal counts are not provided in the model
    if ( ! ek[0].size() )
      {
      vtkErrorMacro( "Incomplete input: missing marginal count for "
                     << varNameX.c_str()
                     <<". Cannot test." );
      return;
      }

    if ( ! ek[1].size() )
      {
      vtkErrorMacro( "Incomplete input: missing marginal count for "
                     << varNameY.c_str()
                     <<". Cannot test." );
      return;
      }

    // Now that we have all we need, let us calculate the test statistic

    // We must iterate over all possible independent instances, which might result in
    // an impossibly too large double loop, even if the actual occurrence table is
    // sparse. C'est la vie.
    double eij, delta;
    double chi2  = 0; // chi square test statistic
    double chi2y = 0; // chi square test statistic with Yates correction
    for ( Counts::iterator xit = ek[0].begin(); xit != ek[0].end(); ++ xit )
      {
      for ( Counts::iterator yit = ek[1].begin(); yit != ek[1].end(); ++ yit )
        {
        // Expected count
        eij = static_cast<double>( xit->second * yit->second ) / n;

        // Discrepancy
        delta = eij - oij[xit->first][yit->first];

        // Chi square contribution
        chi2 += delta * delta / eij;

        // Chi square contribution with Yates correction
        delta = fabs( delta ) - .5;
        chi2y += delta * delta / eij;
        } // yit
      } // xit

    // Degrees of freedom
    vtkIdType d = ( ek[0].size() - 1 ) * ( ek[1].size() - 1 );

    // Insert variable name and calculated Jarque-Bera statistic
    // NB: R will be invoked only once at the end for efficiency
    dimCol->InsertNextTuple1( d );
    chi2Col->InsertNextTuple1( chi2 );
    chi2yCol->InsertNextTuple1( chi2y );
    } // rit

  // Now, add the already prepared columns to the output table
  testTab->AddColumn( dimCol );
  testTab->AddColumn( chi2Col );
  testTab->AddColumn( chi2yCol );

  // Last phase: compute the p-values or assign invalid value if they cannot be computed
  vtkDoubleArray* testChi2Col = 0;
  vtkDoubleArray* testChi2yCol = 0;
  bool calculatedP = false;

  // If available, use R to obtain the p-values for the Chi square distribution with required DOFs
#ifdef VTK_USE_GNU_R
  // Prepare VTK - R interface
  vtkRInterface* ri = vtkRInterface::New();

  // Use the calculated DOFs and Chi square statistics as inputs to the Chi square function
  ri->AssignVTKDataArrayToRVariable( dimCol, "d" );
  ri->AssignVTKDataArrayToRVariable( chi2Col, "chi2" );
  ri->AssignVTKDataArrayToRVariable( chi2yCol, "chi2y" );

  // Now prepare R script and calculate the p-values (in a single R script evaluation for efficiency)
  vtksys_ios::ostringstream rs;
  rs << "p<-c();"
     << "py<-c();"
     << "for(i in 1:"
     << dimCol->GetNumberOfTuples()
     << "){"
     << "p<-c(p,1-pchisq(chi2[i],d[i]));"
     << "py<-c(py,1-pchisq(chi2y[i],d[i]))"
     << "}";
  ri->EvalRscript( rs.str().c_str() );

  // Retrieve the p-values
  testChi2Col = vtkDoubleArray::SafeDownCast( ri->AssignRVariableToVTKDataArray( "p" ) );
  testChi2yCol = vtkDoubleArray::SafeDownCast( ri->AssignRVariableToVTKDataArray( "py" ) );
  if ( ! testChi2Col || ! testChi2yCol
       || testChi2Col->GetNumberOfTuples() != dimCol->GetNumberOfTuples()
       || testChi2yCol->GetNumberOfTuples() != dimCol->GetNumberOfTuples() )
    {
    vtkWarningMacro( "Something went wrong with the R calculations. Reported p-values will be invalid." );
    }
  else
    {
    // Test values have been calculated by R: the test column can be added to the output table
    testTab->AddColumn( testChi2Col );
    testTab->AddColumn( testChi2yCol );
    calculatedP = true;
    }

  // Clean up
  ri->Delete();
#endif // VTK_USE_GNU_R

  // Use the invalid value of -1 for p-values if R is absent or there was an R error
  if ( ! calculatedP )
    {
    // Test columns must be created first
    testChi2Col = vtkDoubleArray::New(); // Chi square p-value
    testChi2yCol = vtkDoubleArray::New(); // Chi square with Yates correction p-value

    // Fill this column
    vtkIdType n = dimCol->GetNumberOfTuples();
    testChi2Col->SetNumberOfTuples( n );
    testChi2yCol->SetNumberOfTuples( n );
    for ( vtkIdType r = 0; r < n; ++ r )
      {
      testChi2Col->SetTuple1( r, -1 );
      testChi2yCol->SetTuple1( r, -1 );
      }

    // Now add the column of invalid values to the output table
    testTab->AddColumn( testChi2Col );
    testTab->AddColumn( testChi2yCol );

    // Clean up
    testChi2Col->Delete();
    testChi2yCol->Delete();
    }

  // The test column name can only be set after the column has been obtained from R
  testChi2Col->SetName( "P" );
  testChi2yCol->SetName( "P Yates" );

  // Finally set output table to test table
  outMeta->ShallowCopy( testTab );

  // Clean up
  dimCol->Delete();
  chi2Col->Delete();
  chi2yCol->Delete();
  testTab->Delete();
}


// ----------------------------------------------------------------------
class BivariateContingenciesAndInformationFunctor : public vtkStatisticsAlgorithm::AssessFunctor
{
public:
  vtkAbstractArray* DataX;
  vtkAbstractArray* DataY;
  vtksys_stl::map<vtkStdString,PDF> PdfX_Y;
  vtksys_stl::map<vtkStdString,PDF> PdfYcX;
  vtksys_stl::map<vtkStdString,PDF> PdfXcY;
  vtksys_stl::map<vtkStdString,PDF> PmiX_Y;

  BivariateContingenciesAndInformationFunctor( vtkAbstractArray* valsX,
                                               vtkAbstractArray* valsY,
                                               const vtksys_stl::map<vtkStdString,PDF> parameters[4] )
  {
    this->DataX = valsX;
    this->DataY = valsY;
    this->PdfX_Y = parameters[0];
    this->PdfYcX = parameters[1];
    this->PdfXcY = parameters[2];
    this->PmiX_Y = parameters[3];
  }
  virtual ~BivariateContingenciesAndInformationFunctor() { }
  virtual void operator() ( vtkVariantArray* result,
                            vtkIdType id )
  {
    vtkStdString x = this->DataX->GetVariantValue( id ).ToString();
    vtkStdString y = this->DataY->GetVariantValue( id ).ToString();

    result->SetNumberOfValues( 4 );
    result->SetValue( 0, this->PdfX_Y[x][y] );
    result->SetValue( 1, this->PdfYcX[x][y] );
    result->SetValue( 2, this->PdfXcY[x][y] );
    result->SetValue( 3, this->PmiX_Y[x][y] );
  }
};

// ----------------------------------------------------------------------
void vtkContingencyStatistics::Assess( vtkTable* inData,
                                       vtkMultiBlockDataSet* inMeta,
                                       vtkTable* outData )
{
  if ( ! inData )
    {
    return;
    }

  if ( ! inMeta )
    {
    return;
    }

  vtkTable* summaryTab = vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) );
  if ( ! summaryTab )
    {
    return;
    }

  // Downcast columns to string arrays for efficient data access
  vtkStringArray* varX = vtkStringArray::SafeDownCast( summaryTab->GetColumnByName( "Variable X" ) );
  vtkStringArray* varY = vtkStringArray::SafeDownCast( summaryTab->GetColumnByName( "Variable Y" ) );

  // Loop over requests
  vtkIdType nRowSumm = summaryTab->GetNumberOfRows();
  vtkIdType nRowData = inData->GetNumberOfRows();
  for ( vtksys_stl::set<vtksys_stl::set<vtkStdString> >::const_iterator rit = this->Internals->Requests.begin();
        rit != this->Internals->Requests.end(); ++ rit )
    {
    // Each request contains only one pair of column of interest (if there are others, they are ignored)
    vtksys_stl::set<vtkStdString>::const_iterator it = rit->begin();
    vtkStdString varNameX = *it;
    if ( ! inData->GetColumnByName( varNameX ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << varNameX.c_str()
                       << ". Ignoring this pair." );
      continue;
      }

    ++ it;
    vtkStdString varNameY = *it;
    if ( ! inData->GetColumnByName( varNameY ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << varNameY.c_str()
                       << ". Ignoring this pair." );
      continue;
      }

    // Find the summary key to which the pair (colX,colY) corresponds
    vtkIdType pairKey = -1;
    for ( vtkIdType r = 0; r < nRowSumm && pairKey == -1; ++ r )
      {
      if ( varX->GetValue( r ) == varNameX
           &&
           varY->GetValue( r ) == varNameY )
        {
        pairKey = r;
        }
      }
    if ( pairKey < 0 || pairKey >= nRowSumm )
      {
      vtkErrorMacro( "Inconsistent input: dictionary does not have a row "
                     << pairKey
                     <<". Cannot derive model." );
      return;
      }

    vtkStringArray* varNames = vtkStringArray::New();
    varNames->SetNumberOfValues( VTK_STATISTICS_NUMBER_OF_VARIABLES );
    varNames->SetValue( 0, varNameX );
    varNames->SetValue( 1, varNameY );

    // Store names to be able to use SetValueByName which is faster than SetValue
    int nv = this->AssessNames->GetNumberOfValues();
    vtkStdString* names = new vtkStdString[nv];
    for ( int v = 0; v < nv; ++ v )
      {
      vtksys_ios::ostringstream assessColName;
      assessColName << this->AssessNames->GetValue( v )
                    << "("
                    << varNameX
                    << ","
                    << varNameY
                    << ")";

      names[v] = assessColName.str().c_str();

      vtkDoubleArray* assessValues = vtkDoubleArray::New();
      assessValues->SetName( names[v] );
      assessValues->SetNumberOfTuples( nRowData );
      outData->AddColumn( assessValues );
      assessValues->Delete();
      }

    // Select assess functor
    AssessFunctor* dfunc;
    this->SelectAssessFunctor( outData,
                               inMeta,
                               pairKey,
                               varNames,
                               dfunc );

    if ( ! dfunc )
      {
      // Functor selection did not work. Do nothing.
      vtkWarningMacro( "AssessFunctors could not be allocated for column pair ("
                       << varNameX.c_str()
                       << ","
                       << varNameY.c_str()
                       << "). Ignoring it." );
      delete [] names;
      continue;
      }
    else
      {
      // Assess each entry of the column
      vtkVariantArray* assessResult = vtkVariantArray::New();
      for ( vtkIdType r = 0; r < nRowData; ++ r )
        {
        (*dfunc)( assessResult, r );
        for ( int v = 0; v < nv; ++ v )
          {
          outData->SetValueByName( r, names[v], assessResult->GetValue( v ) );
          }
        }

      assessResult->Delete();
      }

    // Clean up
    delete dfunc;
    delete [] names;
    varNames->Delete(); // Do not delete earlier! Otherwise, dfunc will be wrecked
    } // rit
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::SelectAssessFunctor( vtkTable* vtkNotUsed( outData ),
                                                    vtkDataObject* vtkNotUsed( inMetaDO ),
                                                    vtkStringArray* vtkNotUsed( rowNames ),
                                                    AssessFunctor*& vtkNotUsed( dfunc ) )
{
  // This method is not implemented for contingency statistics, as its API does not allow
  // for the passing of necessary parameters.
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::SelectAssessFunctor( vtkTable* outData,
                                                    vtkMultiBlockDataSet* inMeta,
                                                    vtkIdType pairKey,
                                                    vtkStringArray* rowNames,
                                                    AssessFunctor*& dfunc )
{
  vtkTable* contingencyTab = vtkTable::SafeDownCast( inMeta->GetBlock( 1 ) );
  if ( ! contingencyTab  )
    {
    return;
    }

  vtkStdString varNameX = rowNames->GetValue( 0 );
  vtkStdString varNameY = rowNames->GetValue( 1 );

  // Grab the data for the requested variables
  vtkAbstractArray* valsX = outData->GetColumnByName( varNameX );
  vtkAbstractArray* valsY = outData->GetColumnByName( varNameY );
  if ( ! valsX || ! valsY )
    {
    dfunc = 0;
    return;
    }

  // Downcast columns to appropriate arrays for efficient data access
  vtkIdTypeArray* keys = vtkIdTypeArray::SafeDownCast( contingencyTab->GetColumnByName( "Key" ) );
  vtkStringArray* valx = vtkStringArray::SafeDownCast( contingencyTab->GetColumnByName( "x" ) );
  vtkStringArray* valy = vtkStringArray::SafeDownCast( contingencyTab->GetColumnByName( "y" ) );

  int np = this->AssessParameters->GetNumberOfValues();
  vtksys_stl::vector<vtkDoubleArray *> para;
  for ( int p = 0; p < np; ++ p )
    {
    para.push_back( vtkDoubleArray::SafeDownCast( contingencyTab->GetColumnByName( this->AssessParameters->GetValue( p ) ) ) );
    if ( ! para[p] )
      {
      dfunc = 0;
      return;
      }
    }

  // parameter maps:
  // 0: PDF(X,Y)
  // 1: PDF(Y|X)
  // 2: PDF(X|Y)
  // 3: PMI(X,Y)
  vtksys_stl::map<vtkStdString,PDF> paraMap[4];

  // Sanity check: joint CDF
  double cdf = 0.;

  // Loop over parameters table until the requested variables are found
  vtkIdType nRowCont = contingencyTab->GetNumberOfRows();
  vtkStdString x, y;
  vtkIdType key;
  double v;
  for ( int r = 1; r < nRowCont; ++ r ) // Skip first row which contains data set cardinality
    {
    // Find the pair of variables to which the key corresponds
    key = keys->GetValue( r );

    if ( key != pairKey )
      {
      continue;
      }

    x = valx->GetValue( r );
    y = valy->GetValue( r );

    for ( int p = 0; p < np; ++ p )
      {
      v = para[p]->GetValue( r );
      paraMap[p][x][y] = v;

      // Sanity check: update CDF
      if ( ! p )
        {
        cdf += v;
        }
      }
    } // for ( int r = 1; r < nRowCont; ++ r )

  // Sanity check: verify that CDF = 1
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

  dfunc = new BivariateContingenciesAndInformationFunctor( valsX,
                                                           valsY,
                                                           paraMap );
}
