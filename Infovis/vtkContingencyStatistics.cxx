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
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/map>

#include <vtksys/ios/sstream>

typedef vtkstd::map<vtkStdString,vtkIdType> Counts;
typedef vtkstd::map<vtkStdString,double> PDF;

vtkCxxRevisionMacro(vtkContingencyStatistics, "1.44");
vtkStandardNewMacro(vtkContingencyStatistics);

// ----------------------------------------------------------------------
vtkContingencyStatistics::vtkContingencyStatistics()
{
  this->AssessNames->SetNumberOfValues( 4 );
  this->AssessNames->SetValue( 0, "P" );
  this->AssessNames->SetValue( 1, "Py|x" );
  this->AssessNames->SetValue( 2, "Px|y" );
  this->AssessNames->SetValue( 3, "PMI" );

  this->AssessParameters = vtkStringArray::New();
  this->AssessParameters->SetNumberOfValues( 3 );
  this->AssessParameters->SetValue( 0, "P" );
  this->AssessParameters->SetValue( 1, "Py|x" );
  this->AssessParameters->SetValue( 2, "Px|y" );

  this->CalculatePointwiseInformation = true;
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
  os << indent << "CalculatePointwiseInformation: " << this->CalculatePointwiseInformation << endl;
}

// ----------------------------------------------------------------------
int vtkContingencyStatistics::FillInputPortInformation( int port, vtkInformation* info )
{
  // Override the parent class for Meta port 1
  int res; 
  if ( port == 1 )
    {
    info->Set( vtkAlgorithm::INPUT_IS_OPTIONAL(), 1 );
    info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet" );

    res = 1;
    }
  else
    {
    res = this->Superclass::FillInputPortInformation( port, info );
    }

  return res;
}

// ----------------------------------------------------------------------
int vtkContingencyStatistics::FillOutputPortInformation( int port, vtkInformation* info )
{
  // Override the parent class for Meta port 1
  int res = this->Superclass::FillOutputPortInformation( port, info );
  if ( port == 1 )
    {
    info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet" );
    }
  
  return res;
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::ExecuteLearn( vtkTable* inData,
                                             vtkDataObject* outMetaDO )
{
  vtkMultiBlockDataSet* outMeta = vtkMultiBlockDataSet::SafeDownCast( outMetaDO );
  if ( ! outMeta )
    {
    return;
    }

  vtkIdType n = inData->GetNumberOfRows();
  if ( n <= 0 )
    {
    return;
    }

  if ( inData->GetNumberOfColumns() <= 0 )
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

  vtkVariantArray* row2 = vtkVariantArray::New();
  row2->SetNumberOfValues( 2 );

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
  row4->SetValue( 1,  zString );
  row4->SetValue( 2, zString );
  row4->SetValue( 3, -1 );
  contingencyTab->InsertNextRow( row4 );

  typedef vtkstd::map<vtkStdString,vtkIdType> Distribution;

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

    // Create entry in summary for pair (colX,colY) and set its index to be the key
    // for (colX,colY) values in the contingency table
    row2->SetValue( 0, colX );
    row2->SetValue( 1, colY );

    row4->SetValue( 0, summaryTab->GetNumberOfRows() );

    summaryTab->InsertNextRow( row2 );

    vtkAbstractArray* valsX = inData->GetColumnByName( colX );
    vtkAbstractArray* valsY = inData->GetColumnByName( colY );

    vtkstd::map<vtkStdString,Distribution> contingencyTable;
    for ( vtkIdType r = 0; r < n; ++ r )
      {
      ++ contingencyTable
        [valsX->GetVariantValue( r ).ToString()]
        [valsY->GetVariantValue( r ).ToString()];
      }

    for ( vtkstd::map<vtkStdString,Distribution>::iterator mit = contingencyTable.begin(); 
          mit != contingencyTable.end(); ++ mit )
      {
      row4->SetValue( 1, mit->first );
      for ( Distribution::iterator dit = mit->second.begin(); dit != mit->second.end(); ++ dit )
        {
        row4->SetValue( 2, dit->first );
        row4->SetValue( 3, dit->second );

        contingencyTab->InsertNextRow( row4 );
        }
      }
    }

  // Finally set blocks of the output meta information
  outMeta->SetNumberOfBlocks( 2 );
  outMeta->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Summary" );
  outMeta->SetBlock( 0, summaryTab );
  outMeta->GetMetaData( static_cast<unsigned>( 1 ) )->Set( vtkCompositeDataSet::NAME(), "Contingency Table" );
  outMeta->SetBlock( 1, contingencyTab );

  // Clean up
  summaryTab->Delete();
  row2->Delete();
  contingencyTab->Delete();
  row4->Delete();

  return;
}

// ----------------------------------------------------------------------
void vtkContingencyStatistics::ExecuteDerive( vtkDataObject* inMetaDO )
{
  vtkMultiBlockDataSet* inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
  if ( ! inMeta || inMeta->GetNumberOfBlocks() < 2 )
    {
    return;
    }

  vtkTable* summaryTab;
  if ( ! ( summaryTab = vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) ) ) 
       || summaryTab->GetNumberOfColumns() < 2 )
    {
    return;
    }

  vtkIdType nRowSumm = summaryTab->GetNumberOfRows();
  if ( nRowSumm <= 0 )
    {
    return;
    }

  // Add columns for joint and conditional probabilities
  int numDoubles = 3;
  vtkStdString entropyNames[] = { "H(X,Y)",
                                  "H(Y|X)",
                                  "H(X|Y)" };
  
  vtkDoubleArray* doubleCol;
  for ( int j = 0; j < numDoubles; ++ j )
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

  vtkTable* contingencyTab;
  if ( ! ( contingencyTab = vtkTable::SafeDownCast( inMeta->GetBlock( 1 ) ) )
       || contingencyTab->GetNumberOfColumns() < 4 )
    {
    return;
    }

  vtkIdType nRowCont = contingencyTab->GetNumberOfRows();
  if ( nRowCont <= 0 )
    {
    return;
    }

  // Add columns for joint and conditional probabilities
  vtkStdString doubleNames[] = { "P",
                                 "Py|x",
                                 "Px|y" };
  
  for ( int j = 0; j < numDoubles; ++ j )
    {
    if ( ! contingencyTab->GetColumnByName( doubleNames[j] ) )
      {
      doubleCol = vtkDoubleArray::New();
      doubleCol->SetName( doubleNames[j] );
      doubleCol->SetNumberOfTuples( nRowCont );
      contingencyTab->AddColumn( doubleCol );
      doubleCol->Delete();
      }
    }

  // Downcast columns to string arrays for efficient data access
  vtkStringArray* varX = vtkStringArray::SafeDownCast( summaryTab->GetColumnByName( "Variable X" ) );
  vtkStringArray* varY = vtkStringArray::SafeDownCast( summaryTab->GetColumnByName( "Variable Y" ) );

  vtkIdTypeArray* keys = vtkIdTypeArray::SafeDownCast( contingencyTab->GetColumnByName( "Key" ) );
  vtkStringArray* valx = vtkStringArray::SafeDownCast( contingencyTab->GetColumnByName( "x" ) );
  vtkStringArray* valy = vtkStringArray::SafeDownCast( contingencyTab->GetColumnByName( "y" ) );
  vtkIdTypeArray* card = vtkIdTypeArray::SafeDownCast( contingencyTab->GetColumnByName( "Cardinality" ) );
  vtkDoubleArray* prob[3];
  for ( int j = 0; j < numDoubles; ++ j )
    {
    prob[j] = vtkDoubleArray::SafeDownCast( contingencyTab->GetColumnByName( doubleNames[j] ) );
    }

  if ( ! varX || ! varX || ! keys || ! valx || ! valy || ! card 
       || ! prob[0] || ! prob[1] || ! prob[2] )
    {
    vtkErrorMacro("Empty model column(s). Cannot derive model.\n");
    return;
    }

  // Temporary counters, used to check that all pairs of variables have indeed the same number of observations
  vtkstd::map<vtkIdType,vtkIdType> cardinality;

  // Calculate marginal counts (marginal PDFs are calculated at storage time to avoid redundant summations)
  vtkstd::map<vtkStdString,vtkstd::pair<vtkStdString,vtkStdString> > marginalToPair;
  vtkstd::map<vtkStdString,Counts> marginalCounts;
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
    cardinality[key] += c;

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
  vtkIdType n = cardinality[0];
  for ( vtkstd::map<vtkIdType,vtkIdType>::iterator iit = cardinality.begin();
        iit != cardinality.end(); ++ iit )
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
  double inv_n = 1. / n;
  contingencyTab->SetValueByName( 0, "Cardinality", n );

  // Complete cardinality row (0) with invalid values for derived statistics
  for ( int i = 0; i < numDoubles; ++ i )
    {
    contingencyTab->SetValueByName( 0, doubleNames[i], -1. );
    }

  
  // Resize output meta so marginal PDF tables can be appended
  unsigned int nBlocks = inMeta->GetNumberOfBlocks();
  inMeta->SetNumberOfBlocks( nBlocks + static_cast<unsigned int>(marginalCounts.size()) );

  // Rows of the marginal PDF tables contain:
  // 0: variable value
  // 1: marginal cardinality
  // 2: marginal probability
  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues( 3 );

  // Add marginal PDF tables as new blocks to the meta output starting at block nBlock + 1
  // NB: block nBlock is kept for information entropy
  vtkstd::map<vtkStdString,PDF> marginalPDFs;
  for ( vtkstd::map<vtkStdString,Counts>::iterator sit = marginalCounts.begin();
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

      marginalTab->Delete();
      }


  // Container for P(x,y), P(y|x), P(x|y)
  double* doubleVals = new double[numDoubles]; 
  
  // Container for H(X,Y), H(Y|X), H(X|Y)
  typedef vtkstd::map<vtkIdType,double> Entropies;
  Entropies *H = new Entropies[numDoubles];

  // Calculate joint and conditional PDFs, and information entropies
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

    c1 = varX->GetValue( key );
    c2 = varY->GetValue( key );

    x = valx->GetValue( r );
    y = valy->GetValue( r );
    c = card->GetValue( r );
    doubleVals[0] = inv_n * c;

    doubleVals[1] = doubleVals[0] / marginalPDFs[c1][x];
    doubleVals[2] = doubleVals[0] / marginalPDFs[c2][y];

    for ( int j = 0; j < numDoubles; ++ j )
      {
      // Store conditional probabilities
      prob[j]->SetValue( r, doubleVals[j] );

      // Update information entropies
      H[j][key] -= doubleVals[0] * log( doubleVals[j] );
      }
    }

  // Store information entropies
  for ( Entropies::iterator eit = H[0].begin(); eit != H[0].end(); ++ eit )
      {
      summaryTab->SetValueByName( eit->first, entropyNames[0], eit->second ); // H(X,Y)
      summaryTab->SetValueByName( eit->first, entropyNames[1], H[1][eit->first] ); // H(Y|X)
      summaryTab->SetValueByName( eit->first, entropyNames[2], H[2][eit->first] ); // H(X|Y)
      }

  // Clean up
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
  vtkstd::map<vtkStdString,PDF> PdfXY;
  vtkstd::map<vtkStdString,PDF> PdfYcondX;
  vtkstd::map<vtkStdString,PDF> PdfXcondY;

  BivariateContingenciesFunctor( vtkAbstractArray* valsX,
                                 vtkAbstractArray* valsY,
                                 const vtkstd::map<vtkStdString,PDF>& pdfXY,
                                 const vtkstd::map<vtkStdString,PDF>& pdfYcondX,
                                 const vtkstd::map<vtkStdString,PDF>& pdfXcondY )
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
    vtkStdString x = this->DataX->GetVariantValue( id ).ToString();
    vtkStdString y = this->DataY->GetVariantValue( id ).ToString();

    result->SetNumberOfValues( 3 );
    result->SetValue( 0, this->PdfXY[x][y] );
    result->SetValue( 1, this->PdfYcondX[x][y] );
    result->SetValue( 2, this->PdfXcondY[x][y] );
  }
};

// ----------------------------------------------------------------------
class BivariateContingenciesAndInformationFunctor : public vtkStatisticsAlgorithm::AssessFunctor
{
public:
  vtkAbstractArray* DataX;
  vtkAbstractArray* DataY;
  PDF PdfX;
  PDF PdfY;
  vtkstd::map<vtkStdString,PDF> PdfXY;
  vtkstd::map<vtkStdString,PDF> PdfYcondX;
  vtkstd::map<vtkStdString,PDF> PdfXcondY;

  BivariateContingenciesAndInformationFunctor( vtkAbstractArray* valsX,
                                               vtkAbstractArray* valsY,
                                               PDF pdfX,
                                               PDF pdfY,
                                               const vtkstd::map<vtkStdString,PDF>& pdfXY,
                                               const vtkstd::map<vtkStdString,PDF>& pdfYcondX,
                                               const vtkstd::map<vtkStdString,PDF>& pdfXcondY )
  {
    this->DataX = valsX;
    this->DataY = valsY;
    this->PdfX = pdfX;
    this->PdfY = pdfY;
    this->PdfXY = pdfXY;
    this->PdfYcondX = pdfYcondX;
    this->PdfXcondY = pdfXcondY;
  }
  virtual ~BivariateContingenciesAndInformationFunctor() { }
  virtual void operator() ( vtkVariantArray* result,
                            vtkIdType id )
  {
    vtkStdString x = this->DataX->GetVariantValue( id ).ToString();
    vtkStdString y = this->DataY->GetVariantValue( id ).ToString();

    result->SetNumberOfValues( 4 );
    result->SetValue( 0, this->PdfXY[x][y] );
    result->SetValue( 1, this->PdfYcondX[x][y] );
    result->SetValue( 2, this->PdfXcondY[x][y] );
    result->SetValue( 3, log ( this->PdfXY[x][y] / ( this->PdfX[x] * this->PdfY[y] ) ) );
  }
};

// ----------------------------------------------------------------------
void vtkContingencyStatistics::ExecuteAssess( vtkTable* inData,
                                              vtkDataObject* inMetaDO,
                                              vtkTable* outData,
                                              vtkDataObject* vtkNotUsed( outMeta ) )
{
  if ( ! inData || inData->GetNumberOfColumns() <= 0 )
    {
    return;
    }

  vtkIdType nRowData = inData->GetNumberOfRows();
  if ( nRowData <= 0 )
    {
    return;
    }

  vtkMultiBlockDataSet* inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
  if ( ! inMeta || inMeta->GetNumberOfBlocks() < 2 )
    {
    return;
    }

  vtkTable* summaryTab;
  if ( ! ( summaryTab = vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) ) ) 
       || summaryTab->GetNumberOfColumns() < 2 )
    {
    return;
    }

  vtkIdType nRowSumm = summaryTab->GetNumberOfRows();
  if ( nRowSumm <= 0 )
    {
    return;
    }

  // Downcast columns to string arrays for efficient data access
  vtkStringArray* varX = vtkStringArray::SafeDownCast( summaryTab->GetColumnByName( "Variable X" ) );
  vtkStringArray* varY = vtkStringArray::SafeDownCast( summaryTab->GetColumnByName( "Variable Y" ) );

  // Loop over pairs of columns of interest
  for ( vtkstd::set<vtkstd::pair<vtkStdString,vtkStdString> >::iterator it = this->Internals->Selection.begin(); 
        it != this->Internals->Selection.end(); ++ it )
    {
    vtkStdString varNameX = it->first;
    if ( ! inData->GetColumnByName( varNameX ) )
      {
      vtkWarningMacro( "InData table does not have a column "
                       << varNameX.c_str()
                       << ". Ignoring this pair." );
      continue;
      }

    vtkStdString varNameY = it->second;
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
    varNames->SetNumberOfValues( this->NumberOfVariables );
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

    delete dfunc;
    delete [] names;
    varNames->Delete(); // Do not delete earlier! Otherwise, dfunc will be wrecked
    }
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
  vtkTable* contingencyTab;
  if ( ! ( contingencyTab = vtkTable::SafeDownCast( inMeta->GetBlock( 1 ) ) )
       || contingencyTab->GetNumberOfColumns() < 7 )
    {
    return;
    }

  vtkIdType nRowCont = contingencyTab->GetNumberOfRows();
  if ( nRowCont <= 0 )
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
      
  // Downcast columns to string arrays for efficient data access
  vtkIdTypeArray* keys = vtkIdTypeArray::SafeDownCast( contingencyTab->GetColumnByName( "Key" ) );
  vtkStringArray* valx = vtkStringArray::SafeDownCast( contingencyTab->GetColumnByName( "x" ) );
  vtkStringArray* valy = vtkStringArray::SafeDownCast( contingencyTab->GetColumnByName( "y" ) );

  int np = this->AssessParameters->GetNumberOfValues();
  vtkDoubleArray* para[3];
  for ( int p = 0; p < np; ++ p )
    {
    para[p] = vtkDoubleArray::SafeDownCast( contingencyTab->GetColumnByName( this->AssessParameters->GetValue( p ) ) );
    if ( ! para[p] )
      {
      dfunc = 0;
      return;
      }
    }

  // PDFs
  vtkstd::map<vtkStdString,PDF> pdf[3];
  PDF pdfX;
  PDF pdfY;

  // Joint CDF 
  double cdf = 0.;

  // Loop over parameters table until the requested variables are found 
  vtkStdString x, y;
  vtkIdType key;
  double v;
  bool infos = this->GetCalculatePointwiseInformation();
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
      pdf[p][x][y] += v;

      if ( ! p )
        {
        cdf += v;
        if ( infos )
          {
          pdfX[x] += v; 
          pdfY[y] += v; 
          }
        }
      }
    } // for ( int r = 1; r < nRowCont; ++ r )

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

  if ( infos )
    {
    dfunc = new BivariateContingenciesAndInformationFunctor( valsX,
                                                             valsY,
                                                             pdfX,
                                                             pdfY,
                                                             pdf[0],
                                                             pdf[1],
                                                             pdf[2] );
    }
  else
    {
    dfunc = new BivariateContingenciesFunctor( valsX,
                                               valsY,
                                               pdf[0],
                                               pdf[1],
                                               pdf[2] );
    }
}
