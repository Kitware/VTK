#include "vtkMultiCorrelativeStatistics.h"
#include "vtkMultiCorrelativeStatisticsAssessFunctor.h"

#include "vtkDataObject.h"
#include "vtkDataObjectCollection.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOrderStatistics.h"
#include "vtkStatisticsAlgorithmPrivate.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtksys/stl/map>
#include <vtksys/stl/vector>
#include <vtksys/ios/sstream>

#define VTK_MULTICORRELATIVE_KEYCOLUMN1 "Column1"
#define VTK_MULTICORRELATIVE_KEYCOLUMN2 "Column2"
#define VTK_MULTICORRELATIVE_ENTRIESCOL "Entries"
#define VTK_MULTICORRELATIVE_AVERAGECOL "Mean"
#define VTK_MULTICORRELATIVE_COLUMNAMES "Column"

vtkStandardNewMacro(vtkMultiCorrelativeStatistics);

// ----------------------------------------------------------------------
vtkMultiCorrelativeStatistics::vtkMultiCorrelativeStatistics()
{
  this->AssessNames->SetNumberOfValues( 1 );
  this->AssessNames->SetValue( 0, "d^2" ); // Squared Mahalanobis distance
  this->MedianAbsoluteDeviation = false;
}

// ----------------------------------------------------------------------
vtkMultiCorrelativeStatistics::~vtkMultiCorrelativeStatistics()
{
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeStatistics::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
static void vtkMultiCorrelativeInvertCholesky( vtksys_stl::vector<double*>& chol, vtksys_stl::vector<double>& inv )
{
  vtkIdType m = static_cast<vtkIdType>( chol.size() );
  inv.resize( m * ( m + 1 ) / 2 );

  vtkIdType i, j, k;
  for ( i = 0; i < m; ++ i )
    {
    vtkIdType rsi = ( i * ( i + 1 ) ) / 2; // start index of row i in inv.
    inv[rsi + i] = 1. / chol[i][i];
    for ( j = i; j > 0; )
      {
      inv[rsi + (-- j)] = 0.;
      for ( k = j; k < i; ++ k )
        {
        vtkIdType rsk = ( k * ( k + 1 ) ) / 2;
        inv[rsi + j] -= chol[k][i] * inv[rsk + j];
        }
      inv[rsi + j] *= inv[rsi + i];
      }
    }
  // The result, stored in \a inv as a lower-triangular, row-major matrix, is
  // the inverse of the Cholesky decomposition given as input (stored as a
  // rectangular, column-major matrix in \a chol). Note that the super-diagonal
  // entries of \a chol are not zero as you would expect... we just cleverly
  // don't reference them.
}

// ----------------------------------------------------------------------
static void vtkMultiCorrelativeTransposeTriangular( vtksys_stl::vector<double>& a, vtkIdType m )
{
  vtksys_stl::vector<double> b( a.begin(), a.end() );
  double* bp = &b[0];
  vtkIdType i, j;
  a.clear();
  double* v;
  for ( i = 0; i < m; ++ i )
    {
    v = bp + ( i * ( i + 3 ) ) / 2; // pointer to i-th entry along diagonal (i.e., a(i,i)).
    for ( j = i; j < m; ++ j )
      {
      a.push_back( *v );
      v += ( j + 1 ); // move down one row
      }
    }

  // Now, if a had previously contained: [ A B C D E F G H I J ], representing the
  // lower triangular matrix: A          or the upper triangular: A B D G
  // (row-major order)        B C           (column-major order)    C E H
  //                          D E F                                   F I
  //                          G H I J                                   J
  // It now contains [ A B D G C E H F I J ], representing
  // upper triangular matrix : A B D G   or the lower triangular: A
  // (row-major order)           C E H      (column-major order)  B C
  //                               F I                            D E F
  //                                 J                            G H I J
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeAssessFunctor::operator () ( vtkVariantArray* result, vtkIdType row )
{
  vtkIdType m = static_cast<vtkIdType>( this->Columns.size() );
  vtkIdType i, j;
  this->Tuple = this->EmptyTuple; // initialize Tuple to 0.0
  double* x = &this->Tuple[0];
  double* y;
  double* ci = &this->Factor[0];
  double v;
  for ( i = 0; i < m; ++ i )
    {
    v = this->Columns[i]->GetTuple( row )[0] - this->Center[i];
    y = x + i;
    for ( j = i; j < m; ++ j, ++ ci, ++ y )
      {
      (*y) += (*ci) * v;
      }
    }
  double r = 0.;
  y = x;
  for ( i = 0; i < m; ++ i, ++ y )
    {
    r += (*y) * (*y);
    }

  result->SetNumberOfValues( 1 );
  // To report cumulance values instead of relative deviation, use this:
  // result->SetValue( 0, exp( -0.5 * r ) * pow( 0.5 * r, 0.5 * m - 2.0 ) * ( 0.5 * ( r + m ) - 1.0 ) / this->Normalization );
  result->SetValue( 0, r );
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeStatistics::Aggregate( vtkDataObjectCollection* inMetaColl,
                                               vtkMultiBlockDataSet* outMeta )
{
  if ( ! outMeta )
    {
    return;
    }

  // Get hold of the first model (data object) in the collection
  vtkCollectionSimpleIterator it;
  inMetaColl->InitTraversal( it );
  vtkDataObject *inMetaDO = inMetaColl->GetNextDataObject( it );

  // Verify that the first input model is indeed contained in a multiblock data set
  vtkMultiBlockDataSet* inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
  if ( ! inMeta )
    {
    return;
    }

  // Verify that the first covariance matrix is indeed contained in a table
  vtkTable* inCov = vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) );
  if ( ! inCov )
    {
    return;
    }

  vtkIdType nRow = inCov->GetNumberOfRows();
  if ( ! nRow )
    {
    // No statistics were calculated.
    return;
    }

  // Use this first model to initialize the aggregated one
  vtkTable* outCov = vtkTable::New();
  outCov->DeepCopy( inCov );

  // Now, loop over all remaining models and update aggregated each time
  while ( ( inMetaDO = inMetaColl->GetNextDataObject( it ) ) )
    {
    // Verify that the current model is indeed contained in a multiblock data set
    inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
    if ( ! inMeta )
      {
      outCov->Delete();

      return;
      }

    // Verify that the current covariance matrix is indeed contained in a table
    inCov = vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) );
    if ( ! inCov )
      {
      outCov->Delete();

      return;
      }

    if ( inCov->GetNumberOfRows() != nRow )
      {
      // Models do not match
      outCov->Delete();

      return;
      }

    // Iterate over all model rows
    int inN, outN;
    double muFactor = 0.;
    double covFactor = 0.;
    std::vector<double> inMu, outMu;
    int j = 0;
    int k = 0;
    for ( int r = 0; r < nRow; ++ r )
      {
      // Verify that variable names match each other
      if ( inCov->GetValueByName( r, VTK_MULTICORRELATIVE_KEYCOLUMN1 ) != outCov->GetValueByName( r, VTK_MULTICORRELATIVE_KEYCOLUMN1 )
           || inCov->GetValueByName( r, VTK_MULTICORRELATIVE_KEYCOLUMN2 ) != outCov->GetValueByName( r, VTK_MULTICORRELATIVE_KEYCOLUMN2 ) )
        {
        // Models do not match
        outCov->Delete();

        return;
        }

      // Update each model parameter
      if ( inCov->GetValueByName( r, VTK_MULTICORRELATIVE_KEYCOLUMN1 ).ToString() == "Cardinality" )
        {
        // Cardinality
        inN = inCov->GetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL ).ToInt();
        outN = outCov->GetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL ).ToInt();
        int totN = inN + outN;
        outCov->SetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL, totN );
        muFactor = static_cast<double>( inN ) / totN;
        covFactor = static_cast<double>( inN ) * outN / totN;
        }
      else if ( inCov->GetValueByName( r, VTK_MULTICORRELATIVE_KEYCOLUMN2 ).ToString() == "" )
        {
        // Mean
        inMu.push_back( inCov->GetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL ).ToDouble() );
        outMu.push_back( outCov->GetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL ).ToDouble() );
        outCov->SetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL, outMu.back() + ( inMu.back() - outMu.back() ) * muFactor );
        }
      else
        {
        // M XY
        double inCovEntry = inCov->GetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL ).ToDouble();
        double outCovEntry = outCov->GetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL ).ToDouble();
        outCov->SetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL, inCovEntry + outCovEntry + ( inMu[j] - outMu[j] ) * ( inMu[k] - outMu[k] ) * covFactor );
        ++ k;
        if ( k > j )
          {
          ++ j;
          k = 0;
          }
        }
      }
    }

  // Replace covariance block of output model with updated one
  outMeta->SetBlock( 0, outCov );

  // Clean up
  outCov->Delete();
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeStatistics::Learn( vtkTable* inData,
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

  vtkTable* sparseCov = vtkTable::New();

  vtkStringArray* col1 = vtkStringArray::New();
  col1->SetName( VTK_MULTICORRELATIVE_KEYCOLUMN1 );
  sparseCov->AddColumn( col1 );
  col1->Delete();

  vtkStringArray* col2 = vtkStringArray::New();
  col2->SetName( VTK_MULTICORRELATIVE_KEYCOLUMN2 );
  sparseCov->AddColumn( col2 );
  col2->Delete();

  vtkDoubleArray* col3 = vtkDoubleArray::New();
  col3->SetName( VTK_MULTICORRELATIVE_ENTRIESCOL );
  sparseCov->AddColumn( col3 );
  col3->Delete();

  vtksys_stl::set<vtksys_stl::set<vtkStdString> >::const_iterator reqIt;
  vtksys_stl::set<vtkStdString>::const_iterator colIt;
  vtksys_stl::set<vtksys_stl::pair<vtkStdString,vtkDataArray*> > allColumns;
  vtksys_stl::map<vtksys_stl::pair<vtkIdType,vtkIdType>,vtkIdType> colPairs;
  vtksys_stl::map<vtksys_stl::pair<vtkIdType,vtkIdType>,vtkIdType>::iterator cpIt;
  vtksys_stl::map<vtkStdString,vtkIdType> colNameToIdx;
  vtksys_stl::vector<vtkDataArray*> colPtrs;

  // Populate a vector with pointers to columns of interest (i.e., columns from the input dataset
  // which have some statistics requested) and create a map from column names into this vector.
  // The first step is to create a set so that the vector entries will be sorted by name.
  for ( reqIt = this->Internals->Requests.begin(); reqIt != this->Internals->Requests.end(); ++ reqIt )
    {
    for ( colIt = reqIt->begin(); colIt != reqIt->end(); ++ colIt )
      {
      // Ignore invalid column names
      vtkDataArray* arr = vtkDataArray::SafeDownCast( inData->GetColumnByName( colIt->c_str() ) );
      if ( arr )
        {
        allColumns.insert( vtksys_stl::pair<vtkStdString,vtkDataArray*>( *colIt, arr ) );
        }
      }
    }

  // Now make a map from input column name to output column index (colNameToIdx):
  vtkIdType i = 0;
  vtkIdType m = static_cast<vtkIdType>( allColumns.size() );
  vtksys_stl::set<vtksys_stl::pair<vtkStdString,vtkDataArray*> >::const_iterator acIt;
  vtkStdString empty;
  col1->InsertNextValue( "Cardinality" );
  col2->InsertNextValue( empty );
  for ( acIt = allColumns.begin(); acIt != allColumns.end(); ++ acIt )
    {
    colNameToIdx[acIt->first] = i ++;
    colPtrs.push_back( acIt->second );
    col1->InsertNextValue( acIt->second->GetName() );
    col2->InsertNextValue( empty );
    }

  // Get a list of column pairs (across all requests) for which sums of squares will be computed.
  // This keeps us from computing the same covariance entry multiple times if several requests
  // contain common pairs of columns.
  i = m;

  // Loop over requests
  vtkIdType nRow = inData->GetNumberOfRows();
  for ( reqIt = this->Internals->Requests.begin(); reqIt != this->Internals->Requests.end(); ++ reqIt )
    {
    // For each column in the request:
    for ( colIt = reqIt->begin(); colIt != reqIt->end(); ++ colIt )
      {
      vtksys_stl::map<vtkStdString,vtkIdType>::iterator idxIt = colNameToIdx.find( *colIt );
      // Ignore invalid column names
      if ( idxIt != colNameToIdx.end() )
        {
        vtkIdType colA = idxIt->second;
        vtkStdString colAName = idxIt->first;
        vtksys_stl::set<vtkStdString>::const_iterator colIt2;
        for ( colIt2 = colIt; colIt2 != reqIt->end(); ++ colIt2 )
          {
          idxIt = colNameToIdx.find( *colIt2 );
          // Ignore invalid column names
          if ( idxIt != colNameToIdx.end() )
            { // Note that other requests may have inserted this entry.
            vtksys_stl::pair<vtkIdType,vtkIdType> entry( colA, idxIt->second );
            if ( colPairs.find( entry ) == colPairs.end() )
              {
              // Point to the offset in col3 (below) for this column-pair sum:
              colPairs[entry] = -1;
              }
            }
          }
        }
      }
    }

  // Now insert the column pairs into col1 and col2 in the order in which they'll be evaluated.
  for ( cpIt = colPairs.begin(); cpIt != colPairs.end(); ++ cpIt )
    {
    cpIt->second = i ++;
    col1->InsertNextValue( colPtrs[cpIt->first.first]->GetName() );
    col2->InsertNextValue( colPtrs[cpIt->first.second]->GetName() );
    }

  // Now (finally!) compute the covariance and column sums.
  // This uses the on-line algorithms for computing centered
  // moments and covariances from Philippe's SAND2008-6212 report.
  double* x;
  vtksys_stl::vector<double> v( m, 0. ); // Values (v) for one observation

  // Storage pattern in primary statistics column:
  //  Row 0: cardinality of sample
  //  Rows 1 to m - 1: means of each variable
  //  Rows m to m + colPairs.size(): variances/covariances for each pair of variables
  col3->SetNumberOfTuples( 1 + m + colPairs.size() );
  col3->FillComponent( 0, 0. );

  // Retrieve pointer to values and skip Cardinality entry
  double* rv = col3->GetPointer( 0 );
  *rv = static_cast<double>( nRow );
  ++ rv;

  if ( this->MedianAbsoluteDeviation )
    {
    // Computes the Median
    vtkNew<vtkTable> medianTable;
    this->ComputeMedian(inData, medianTable.GetPointer());
    // Sets the median
    x = rv;
    for ( vtkIdType j = 0; j < m; ++ j, ++ x )
      {
      *x = medianTable->GetValue(1, j+1).ToDouble();
      }

    // Computes the MAD inData (Median Absolute Deviation)
    vtkNew<vtkTable> inDataMAD;
    vtkIdType l = 0;
    // Iterate over column pairs
    for ( cpIt = colPairs.begin(); cpIt != colPairs.end(); ++ cpIt, ++ x , ++ l )
      {
      vtkIdType j = cpIt->first.first;
      vtkIdType k = cpIt->first.second;

      vtksys_ios::ostringstream nameStr;
      nameStr << "Cov{" << j << "," << k << "}";
      vtkNew<vtkDoubleArray> col;
      col->SetNumberOfTuples( nRow );
      col->SetName( nameStr.str().c_str() );
      inDataMAD->AddColumn( col.GetPointer() );
      // Iterate over rows
      for ( i = 0; i < nRow; ++ i )
        {
        inDataMAD->SetValue(i, l, (int)fabs(
            ( colPtrs[j]->GetTuple(i)[0] - rv[j] ) *
            ( colPtrs[k]->GetTuple(i)[0] - rv[k] )
          ));
        }
      }
    // Computes the MAD matrix
    vtkNew<vtkTable> MADTable;
    this->ComputeMedian( inDataMAD.GetPointer(), MADTable.GetPointer() );
    // Sets the MAD
    x = rv + m;
    // Iterate over column pairs
    for ( l = 0, cpIt = colPairs.begin(); cpIt != colPairs.end(); ++ cpIt, ++ x, ++ l )
      {
      *x = MADTable->GetValue(1, l+1).ToDouble();
      }
    }
  else
    {
    // Iterate over rows
    for ( i = 0; i < nRow; ++ i )
      {
      // First fetch column values
      for ( vtkIdType j = 0; j < m; ++ j )
        {
        v[j] = colPtrs[j]->GetTuple(i)[0];
        }
      // Update column products. Equation 3.12 from the SAND report.
      x = rv + m;
      for ( cpIt = colPairs.begin(); cpIt != colPairs.end(); ++ cpIt, ++ x )
        {
        // cpIt->first is a pair of indices into colPtrs used to specify (u,v) or (s,t)
        // cpIt->first.first is the index of u or s
        // cpIt->first.second is the index of v or t
        *x +=
          ( v[cpIt->first.first] - rv[cpIt->first.first] ) * // \delta_{u,2,1} = s - \mu_{u,1}
          ( v[cpIt->first.second] - rv[cpIt->first.second] ) * // \delta_{v,2,1} = t - \mu_{v,1}
          i / ( i + 1. ); // \frac{n_1 n_2}{n_1 + n_2} = \frac{n_1}{n_1 + 1}
        }
      // Update running column averages. Equation 1.1 from the SAND report.
      x = rv;
      for ( vtkIdType j = 0; j < m; ++ j, ++ x )
        {
        *x += ( v[j] - *x ) / ( i + 1 );
        }
      }
    }

  outMeta->SetNumberOfBlocks( 1 );
  outMeta->SetBlock( 0, sparseCov );
  outMeta->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Raw Sparse Covariance Data" );
  sparseCov->Delete();
}

// ----------------------------------------------------------------------
static void vtkMultiCorrelativeCholesky( vtksys_stl::vector<double*>& a, vtkIdType m )
{
  // First define some macros to make the Cholevsky decomposition algorithm legible:
#ifdef A
#  undef A
#endif
#ifdef L
#  undef L
#endif
#define A(i,j) ( j >= i ? a[j][i] : a[i][j] )
#define L(i,j) a[j][i + 1]

  // Then perform decomposition
  double tmp;
  for ( vtkIdType i = 0; i < m; ++ i )
    {
    L(i,i) = A(i,i);
    for ( vtkIdType k = 0; k < i; ++ k )
      {
      tmp = L(i,k);
      L(i,i) -= tmp * tmp;
      }
    L(i,i) = sqrt( L(i,i) );
    for ( vtkIdType j = i + 1; j < m; ++ j )
      {
      L(j,i) = A(j,i);
      for ( vtkIdType k = 0; k < i; ++ k )
        {
        L(j,i) -= L(j,k) * L(i,k);
        }
      L(j,i) /= L(i,i);
      }
    }
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeStatistics::Derive( vtkMultiBlockDataSet* outMeta )
{
  vtkTable* sparseCov;
  vtkStringArray* col1;
  vtkStringArray* col2;
  vtkDoubleArray* col3;
  if (
    ! outMeta ||
    ! ( sparseCov = vtkTable::SafeDownCast( outMeta->GetBlock( 0 ) ) ) ||
    ! ( col1 = vtkStringArray::SafeDownCast( sparseCov->GetColumnByName( VTK_MULTICORRELATIVE_KEYCOLUMN1 ) ) ) ||
    ! ( col2 = vtkStringArray::SafeDownCast( sparseCov->GetColumnByName( VTK_MULTICORRELATIVE_KEYCOLUMN2 ) ) ) ||
    ! ( col3 = vtkDoubleArray::SafeDownCast( sparseCov->GetColumnByName( VTK_MULTICORRELATIVE_ENTRIESCOL ) ) )
    )
    {
    return;
    }

  vtksys_stl::set<vtksys_stl::set<vtkStdString> >::const_iterator reqIt;
  vtksys_stl::set<vtkStdString>::const_iterator colIt;
  vtksys_stl::set<vtksys_stl::pair<vtkStdString,vtkDataArray*> > allColumns;
  vtksys_stl::map<vtksys_stl::pair<vtkIdType,vtkIdType>,vtkIdType> colPairs;
  vtksys_stl::map<vtksys_stl::pair<vtkIdType,vtkIdType>,vtkIdType>::iterator cpIt;
  vtksys_stl::map<vtkStdString,vtkIdType> colNameToIdx;
  vtksys_stl::vector<vtkDataArray*> colPtrs;
  // Reconstruct information about the computed sums from the raw data.
  // The first entry is always the sample size
  double n = col3->GetValue( 0 );
  vtkIdType m = 0;
  vtkIdType i;
  vtkIdType ncol3 = col3->GetNumberOfTuples();
  for ( i = 1; i < ncol3 && col2->GetValue( i ).empty(); ++ i, ++ m )
    {
    colNameToIdx[col1->GetValue( i )] = m;
    }
  for ( ; i < ncol3; ++ i )
    {
    vtksys_stl::pair<vtkIdType,vtkIdType> idxs( colNameToIdx[col1->GetValue(i)], colNameToIdx[col2->GetValue(i)] );
    colPairs[idxs] = i - 1;
    }
  double* rv = col3->GetPointer( 0 ) + 1;

  // Create an output table for each request and fill it in using the col3 array (the first table in
  // outMeta and which is presumed to exist upon entry to Derive).
  // Note that these tables are normalized by the number of samples.
  outMeta->SetNumberOfBlocks( 1 + static_cast<unsigned int>( this->Internals->Requests.size() ) );

  // Keep track of last current block
  unsigned int b = 1;

  // Loop over requests
  double scale = 1. / ( n - 1 ); // n -1 for unbiased variance estimators
  for ( reqIt = this->Internals->Requests.begin();
        reqIt != this->Internals->Requests.end(); ++ reqIt, ++ b )
    {
    vtkStringArray* colNames = vtkStringArray::New();
    colNames->SetName( VTK_MULTICORRELATIVE_COLUMNAMES );
    vtkDoubleArray* colAvgs = vtkDoubleArray::New();
    colAvgs->SetName( VTK_MULTICORRELATIVE_AVERAGECOL );
    vtksys_stl::vector<vtkDoubleArray*> covCols;
    vtksys_stl::vector<double*> covPtrs;
    vtksys_stl::vector<int> covIdxs;
    vtksys_ios::ostringstream reqNameStr;
    reqNameStr << "Cov(";
    // For each column in the request:
    for ( colIt = reqIt->begin(); colIt != reqIt->end(); ++ colIt )
      {
      vtksys_stl::map<vtkStdString,vtkIdType>::iterator idxIt = colNameToIdx.find( *colIt );
      // Ignore invalid column names
      if ( idxIt != colNameToIdx.end() )
        {
        // Create a new column for the covariance matrix output
        covIdxs.push_back( idxIt->second );
        colNames->InsertNextValue( *colIt );
        vtkDoubleArray* arr = vtkDoubleArray::New();
        arr->SetName( colIt->c_str() );
        covCols.push_back( arr );

        if ( colIt == reqIt->begin() )
          {
          reqNameStr << *colIt;
          }
        else
          {
          reqNameStr << "," << *colIt;
          }
        }
      } // colIt

    reqNameStr << ")";
    covCols.push_back( colAvgs );
    colNames->InsertNextValue( "Cholesky" ); // Need extra row for lower-triangular Cholesky decomposition

    // We now have the total number of columns in the output
    // Allocate memory for the correct number of rows and fill in values
    vtkIdType reqCovSize = colNames->GetNumberOfTuples();
    colAvgs->SetNumberOfTuples( reqCovSize );

    // Prepare covariance table and store it as last current block
    vtkTable* covariance = vtkTable::New();
    covariance->AddColumn( colNames );
    covariance->AddColumn( colAvgs );
    outMeta->GetMetaData( b )->Set( vtkCompositeDataSet::NAME(), reqNameStr.str().c_str() );
    outMeta->SetBlock( b, covariance );

    // Clean up
    covariance->Delete();
    colNames->Delete();
    colAvgs->Delete();

    vtkIdType j = 0;
    for ( vtksys_stl::vector<vtkDoubleArray*>::iterator arrIt = covCols.begin(); arrIt != covCols.end(); ++ arrIt, ++ j )
      {
      (*arrIt)->SetNumberOfTuples( reqCovSize );
      (*arrIt)->FillComponent( 0, 0. );
      double* x = (*arrIt)->GetPointer( 0 );
      covPtrs.push_back( x );
      if ( *arrIt != colAvgs )
        {
        // Column is part of covariance matrix
        covariance->AddColumn( *arrIt );
        (*arrIt)->Delete();
        for ( vtkIdType k = 0; k <= j; ++ k, ++ x )
          {
          *x = rv[colPairs[vtksys_stl::pair<vtkIdType,vtkIdType>( covIdxs[k], covIdxs[j] )]] * scale;
          }
        }
      else // if ( *arrIt != colAvgs )
        {
        // Column holds averages
        for ( vtkIdType k = 0; k < reqCovSize - 1; ++ k, ++ x )
          {
          *x = rv[covIdxs[k]];
          }
        *x = static_cast<double>( n );
        }
      } // arrIt, j
    vtkMultiCorrelativeCholesky( covPtrs, reqCovSize - 1 );
    } //  reqIt, b
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeStatistics::Assess( vtkTable* inData,
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

  // For each request, add a column to the output data related to the probability
  // of observing each input datum with respect to the model in the request
  // NB: Column names of the metadata and input data are assumed to match
  // The output columns will be named "this->AssessNames->GetValue(0)(A,B,C)",
  // where "A", "B", and "C" are the column names specified in the per-request metadata tables.
  vtkIdType nRow = inData->GetNumberOfRows();
  int nb = static_cast<int>( inMeta->GetNumberOfBlocks() );
  AssessFunctor* dfunc = 0;
  for ( int req = 1; req < nb; ++ req )
    {
    vtkTable* reqModel = vtkTable::SafeDownCast( inMeta->GetBlock( req ) );
    if ( ! reqModel )
      {
      // Silently skip invalid entries
      // NB: The assessValues column is left in output data even when empty
      continue;
      }

    this->SelectAssessFunctor( inData,
                               reqModel,
                               0,
                               dfunc );
    vtkMultiCorrelativeAssessFunctor* mcfunc = static_cast<vtkMultiCorrelativeAssessFunctor*>( dfunc );
    if ( ! mcfunc )
      {
      vtkWarningMacro( "Request "
                       << req - 1
                       << " could not be accommodated. Skipping." );
      delete dfunc;
      continue;
      }

    // Create the outData columns
    int nv = this->AssessNames->GetNumberOfValues();
    vtkStdString* names = new vtkStdString[nv];
    for ( int v = 0; v < nv; ++ v )
      {
      vtksys_ios::ostringstream assessColName;
      assessColName << this->AssessNames->GetValue( v )
                    << "(";
      for ( int i = 0; i < mcfunc->GetNumberOfColumns(); ++ i )
        {
        if ( i > 0 )
          {
          assessColName << ",";
          }
        assessColName << mcfunc->GetColumn( i )->GetName();
        }
      assessColName << ")";

      // Storing names to be able to use SetValueByName which is faster than SetValue
      vtkDoubleArray* assessValues = vtkDoubleArray::New();
      names[v] = assessColName.str().c_str();
      assessValues->SetName( names[v] );
      assessValues->SetNumberOfTuples( nRow );
      outData->AddColumn( assessValues );
      assessValues->Delete();
      }

    // Assess each entry of the column
    vtkVariantArray* assessResult = vtkVariantArray::New();
    for ( vtkIdType r = 0; r < nRow; ++ r )
      {
      (*dfunc)( assessResult, r );
      for ( int v = 0; v < nv; ++ v )
        {
        outData->SetValueByName( r, names[v], assessResult->GetValue( v ) );
        }
      }

    assessResult->Delete();

    delete dfunc;
    delete [] names;
    }
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeStatistics::ComputeMedian(vtkTable* inData, vtkTable* outData)
{
  vtkOrderStatistics* orderStats = this->CreateOrderStatisticsInstance();
  vtkNew<vtkTable> inOrderStats;
  orderStats->SetInputData( vtkStatisticsAlgorithm::INPUT_DATA,
    inOrderStats.GetPointer() );
  for (vtkIdType i = 0; i < inData->GetNumberOfColumns(); ++i )
    {
    inOrderStats->AddColumn( inData->GetColumn(i) );
    orderStats->AddColumn( inData->GetColumn(i)->GetName() );
    }
  orderStats->SetNumberOfIntervals(2);
  orderStats->SetLearnOption(true);
  orderStats->SetDeriveOption(true);
  orderStats->SetTestOption(false);
  orderStats->SetAssessOption(false);
  orderStats->Update();

  // Get the Median
  vtkMultiBlockDataSet* outputOrderStats = vtkMultiBlockDataSet::SafeDownCast(
    orderStats->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  outData->ShallowCopy( vtkTable::SafeDownCast(
    outputOrderStats->GetBlock( outputOrderStats->GetNumberOfBlocks() - 1) ) );

  orderStats->Delete();
}

// ----------------------------------------------------------------------
vtkOrderStatistics* vtkMultiCorrelativeStatistics::CreateOrderStatisticsInstance()
{
  return vtkOrderStatistics::New();
}

// ----------------------------------------------------------------------
vtkMultiCorrelativeAssessFunctor* vtkMultiCorrelativeAssessFunctor::New()
{
  return new vtkMultiCorrelativeAssessFunctor;
}

// ----------------------------------------------------------------------
bool vtkMultiCorrelativeAssessFunctor::Initialize( vtkTable* inData,
                                                   vtkTable* reqModel,
                                                   bool cholesky )
{
  vtkDoubleArray* avgs = vtkDoubleArray::SafeDownCast( reqModel->GetColumnByName( VTK_MULTICORRELATIVE_AVERAGECOL ) );
  if ( ! avgs )
    {
    vtkGenericWarningMacro( "Multicorrelative request without a \"" VTK_MULTICORRELATIVE_AVERAGECOL "\" column" );
    return false;
    }
  vtkStringArray* name = vtkStringArray::SafeDownCast( reqModel->GetColumnByName( VTK_MULTICORRELATIVE_COLUMNAMES ) );
  if ( ! name )
    {
    vtkGenericWarningMacro( "Multicorrelative request without a \"" VTK_MULTICORRELATIVE_COLUMNAMES "\" column" );
    return false;
    }

  // Storage for input data columns
  vtksys_stl::vector<vtkDataArray*> cols;

  // Storage for Cholesky matrix columns
  // NB: Only the lower triangle is significant
  vtksys_stl::vector<double*> chol;
  vtkIdType m = reqModel->GetNumberOfColumns() - 2;
  vtkIdType i;
  for ( i = 0; i < m ; ++ i )
    {
    vtkStdString colname( name->GetValue( i ) );
    vtkDataArray* arr = vtkDataArray::SafeDownCast( inData->GetColumnByName( colname.c_str() ) );
    if ( ! arr )
      {
      vtkGenericWarningMacro( "Multicorrelative input data needs a \"" << colname.c_str() << "\" column" );
      return false;
      }
    cols.push_back( arr );
    vtkDoubleArray* dar = vtkDoubleArray::SafeDownCast( reqModel->GetColumnByName( colname.c_str() ) );
    if ( ! dar )
      {
      vtkGenericWarningMacro( "Multicorrelative request needs a \"" << colname.c_str() << "\" column" );
      return false;
      }
    chol.push_back( dar->GetPointer( 1 ) );
    }

  // OK, if we made it this far, we will succeed
  this->Columns = cols;
  this->Center = avgs->GetPointer( 0 );
  this->Tuple.resize( m );
  this->EmptyTuple = vtksys_stl::vector<double>( m, 0. );
  if ( cholesky )
    {
    // Store the inverse of chol in this->Factor, F
    vtkMultiCorrelativeInvertCholesky( chol, this->Factor );
    // transpose F to make it easier to use in the () operator
    vtkMultiCorrelativeTransposeTriangular( this->Factor, m );
    }
#if 0
  // Compute the normalization factor to turn X * F * F' * X' into a cumulance
  if ( m % 2 == 0 )
    {
    this->Normalization = 1.0;
    for ( i = m / 2 - 1; i > 1; -- i )
      {
      this->Normalization *= i;
      }
    }
  else
    {
    this->Normalization = sqrt( vtkMath::Pi() ) / ( 1 << ( m / 2 ) );
    for ( i = m - 2; i > 1; i -= 2 )
      {
      this->Normalization *= i;
      }
    }
#endif // 0

  return true;
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeStatistics::SelectAssessFunctor( vtkTable* inData,
                                                         vtkDataObject* inMetaDO,
                                                         vtkStringArray* vtkNotUsed(rowNames),
                                                         AssessFunctor*& dfunc )
{
  dfunc = 0;
  vtkTable* reqModel = vtkTable::SafeDownCast( inMetaDO );
  if ( ! reqModel )
    {
    return;
    }

  vtkMultiCorrelativeAssessFunctor* mcfunc = vtkMultiCorrelativeAssessFunctor::New();
  if ( ! mcfunc->Initialize( inData, reqModel ) )
    {
    delete mcfunc;
    }
  dfunc = mcfunc;
}
