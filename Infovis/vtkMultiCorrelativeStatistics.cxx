#include "vtkMultiCorrelativeStatistics.h"

#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkStatisticsAlgorithmPrivate.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <vtkstd/map>
#include <vtkstd/vector>
#include <vtksys/ios/sstream>

#define VTK_MULTICORRELATIVE_KEYCOLUMN1 "Column1"
#define VTK_MULTICORRELATIVE_KEYCOLUMN2 "Column2"
#define VTK_MULTICORRELATIVE_ENTRIESCOL "Entries"

vtkCxxRevisionMacro(vtkMultiCorrelativeStatistics,"1.1");
vtkStandardNewMacro(vtkMultiCorrelativeStatistics);

vtkMultiCorrelativeStatistics::vtkMultiCorrelativeStatistics()
{
}

vtkMultiCorrelativeStatistics::~vtkMultiCorrelativeStatistics()
{
}

int vtkMultiCorrelativeStatistics::FillInputPortInformation( int port, vtkInformation* info )
{
  // Override the parent class for port 1 (Learn)
  int stat = this->Superclass::FillInputPortInformation( port, info );
  if ( port == 1 )
    {
    info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet" );
    }
  return stat;
}

int vtkMultiCorrelativeStatistics::FillOutputPortInformation( int port, vtkInformation* info )
{
  // Override the parent class for port 1 (Learn)
  int stat = this->Superclass::FillOutputPortInformation( port, info );
  if ( port == 1 )
    {
    info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet" );
    }
  return stat;
}

void vtkMultiCorrelativeStatistics::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

void vtkMultiCorrelativeStatistics::ExecuteLearn(
  vtkTable* inData, vtkDataObject* outMetaDO )
{
  vtkMultiBlockDataSet* outMeta = vtkMultiBlockDataSet::SafeDownCast( outMetaDO );
  if ( ! outMeta )
    {
    return;
    }

  vtkstd::set<vtkstd::set<vtkStdString> >::iterator reqIt;
  vtkstd::set<vtkStdString>::iterator colIt;
  vtkstd::set<vtkstd::pair<vtkStdString,vtkDataArray*> > allColumns;
  vtkstd::map<vtkstd::pair<vtkIdType,vtkIdType>,vtkIdType> colPairs;
  vtkstd::map<vtkstd::pair<vtkIdType,vtkIdType>,vtkIdType>::iterator cpIt;
  vtkstd::map<vtkStdString,vtkIdType> colNameToIdx;
  vtkstd::vector<vtkDataArray*> colPtrs;
  vtkStringArray* ocol1 = vtkStringArray::New();
  vtkStringArray* ocol2 = vtkStringArray::New();
  ocol1->SetName( VTK_MULTICORRELATIVE_KEYCOLUMN1 );
  ocol2->SetName( VTK_MULTICORRELATIVE_KEYCOLUMN2 );

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
        allColumns.insert( vtkstd::pair<vtkStdString,vtkDataArray*>( *colIt, arr ) );
        }
      }
    }
  // Now make a map from input column name to output column index (colNameToIdx):
  vtkIdType i = 0;
  vtkIdType m = static_cast<vtkIdType>( allColumns.size() );
  vtkstd::set<vtkstd::pair<vtkStdString,vtkDataArray*> >::iterator acIt;
  vtkStdString empty;
  ocol1->InsertNextValue( "Sample Size" );
  ocol2->InsertNextValue( empty );
  for ( acIt = allColumns.begin(); acIt != allColumns.end(); ++ acIt )
    {
    colNameToIdx[acIt->first] = i ++;
    colPtrs.push_back( acIt->second );
    ocol1->InsertNextValue( acIt->second->GetName() );
    ocol2->InsertNextValue( empty );
    }

  // Get a list of column pairs (across all requests) for which we'll compute sums of squares.
  // This keeps us from computing the same covariance entry multiple times if several requests
  // contain common pairs of columns.
  i = m - 1;
  // For each request:
  for ( reqIt = this->Internals->Requests.begin(); reqIt != this->Internals->Requests.end(); ++ reqIt )
    {
    // For each column in the request:
    for ( colIt = reqIt->begin(); colIt != reqIt->end(); ++ colIt )
      {
      vtkstd::map<vtkStdString,vtkIdType>::iterator idxIt = colNameToIdx.find( *colIt );
      // Ignore invalid column names
      if ( idxIt != colNameToIdx.end() )
        {
        vtkIdType colA = idxIt->second;
        vtkStdString colAName = idxIt->first;
        vtkstd::set<vtkStdString>::iterator colIt2;
        for ( colIt2 = colIt; colIt2 != reqIt->end(); ++ colIt2 )
          {
          idxIt = colNameToIdx.find( *colIt2 );
          // Ignore invalid column names
          if ( idxIt != colNameToIdx.end() )
            { // Note that other requests may have inserted this entry.
            vtkstd::pair<vtkIdType,vtkIdType> entry( colA, idxIt->second );
            if ( colPairs.find( entry ) == colPairs.end() )
              { // point to the offset in mucov (below) for this column-pair sum:
              colPairs[entry] = ++ i;
              ocol1->InsertNextValue( colAName );
              ocol2->InsertNextValue( idxIt->first );
              }
            }
          }
        }
      }
    }

  // Now (finally!) compute the covariance and column sums.
  // This uses the on-line algorithms for computing centered
  // moments and covariances from Philippe's SAND2008-6212 report.
  vtkIdType n = inData->GetNumberOfRows();
  double* x;
  vtkstd::vector<double> v( m, 0. ); // Values (v) for one observation
  //vtkstd::vector<double> mucov( m + colPairs.size(), 0. ); // mean (mu) followed by covariance (cov) values
  vtkDoubleArray* mucov = vtkDoubleArray::New();
  mucov->SetName( VTK_MULTICORRELATIVE_ENTRIESCOL );
  mucov->SetNumberOfTuples( 1 + m + colPairs.size() ); // sample size followed by mean (mu) followed by covariance (cov) values
  mucov->FillComponent( 0, 0. );
  double* rv = mucov->GetPointer( 0 );
  *rv = static_cast<double>( n );
  ++ rv; // skip SampleSize entry
  for ( i = 0; i < n; ++ i )
    {
    // First fetch column values
    for ( vtkIdType j = 0; j < m; ++ j, ++ x )
      {
      v[j] = colPtrs[j]->GetTuple(i)[0];
      //cout << colPtrs[j]->GetName() << ": " << v[j] << " j=" << j << "\n";
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
  /*
  i = 0;
  cpIt = colPairs.begin();
  for ( vtkstd::vector<double>::iterator dit = mucov.begin(); dit != mucov.end(); ++ dit, ++ i )
    {
    cout << i << ": " << *dit;
    if ( i >= m )
      {
      cout << " (" << cpIt->first.first << "," << cpIt->first.second << ":" << cpIt->second << ")";
      ++ cpIt;
      }
    cout << "\n";
    }
    */

  vtkTable* sparseCov = vtkTable::New();
  sparseCov->AddColumn( ocol1 );
  sparseCov->AddColumn( ocol2 );
  sparseCov->AddColumn( mucov );
  ocol1->Delete();
  ocol2->Delete();
  mucov->Delete();
  outMeta->SetNumberOfBlocks( 1 );
  outMeta->SetBlock( 0, sparseCov );
  outMeta->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Raw Sparse Covariance Data" );
  sparseCov->Delete();

  this->SetSampleSize( n );
}

void vtkMultiCorrelativeCholesky( vtkstd::vector<double*>& a, vtkIdType m )
{
  // Some macros to make the Cholevsky decomposition algorithm legible:
#ifdef A
#  undef A
#endif
#ifdef L
#  undef L
#endif
#define A(i,j) ( j >= i ? a[j][i] : a[i][j] )
#define L(i,j) a[j][i + 1]

  double tmp;
  for ( vtkIdType i = 0; i < m; ++ i )
    {
    //cout << "L(" << i << "," << i << ") is a[" << i << "][" << i + 1 << "] = sqrt(\n";
    //cout << "  A(" << i << "," << i << ") is a[" << i << "][" << i << "] " << A(i,i) << "\n";
    L(i,i) = A(i,i);
    for ( vtkIdType k = 0; k < i; ++ k )
      {
      //cout << "  - L(" << i << "," << k << ")^2 is a[" << k << "][" << i + 1 << "]^2 " << L(i,k) << "^2\n";
      tmp = L(i,k);
      L(i,i) -= tmp * tmp;
      }
    L(i,i) = sqrt( L(i,i) );
    //cout << "  ) = " << L(i,i) << "\n";
    //cout << "--\n";
    for ( vtkIdType j = i + 1; j < m; ++ j )
      {
      //cout << "L(" << j << "," << i << ") is a[" << i << "][" << j + 1 << "] = (\n";
      L(j,i) = A(j,i);
      //cout << "  A(" << j << "," << i << ") is a[" << i << "][" << j << "] " << A(j,i) << "\n";
      for ( vtkIdType k = 0; k < i; ++ k )
        {
        //cout << "  - L(" << j << "," << k << ") is a[" << k << "][" << j + 1 << "] " << L(j,k) << " *\n";
        //cout << "    L(" << i << "," << k << ") is a[" << k << "][" << i + 1 << "] " << L(i,k) << "\n";
        L(j,i) -= L(j,k) * L(i,k);
        }
      L(j,i) /= L(i,i);
      //cout << "  ) / L(" << i << "," << i << ") is a[" << i << "][" << i + 1 << "] " << L(i,i) << "\n";
      //cout << "  = " << L(j,i) << "\n";
      //cout << "--\n";
      }
    }
}

void vtkMultiCorrelativeStatistics::ExecuteDerive( vtkDataObject* outMetaDO )
{
  vtkMultiBlockDataSet* outMeta = vtkMultiBlockDataSet::SafeDownCast( outMetaDO );
  vtkTable* sparseCov;
  vtkStringArray* ocol1;
  vtkStringArray* ocol2;
  vtkDoubleArray* mucov;
  if (
    ! outMeta || outMeta->GetNumberOfBlocks() < 1 ||
    ! ( sparseCov = vtkTable::SafeDownCast( outMeta->GetBlock( 0 ) ) ) ||
    ! ( ocol1 = vtkStringArray::SafeDownCast( sparseCov->GetColumnByName( VTK_MULTICORRELATIVE_KEYCOLUMN1 ) ) ) ||
    ! ( ocol2 = vtkStringArray::SafeDownCast( sparseCov->GetColumnByName( VTK_MULTICORRELATIVE_KEYCOLUMN2 ) ) ) ||
    ! ( mucov = vtkDoubleArray::SafeDownCast( sparseCov->GetColumnByName( VTK_MULTICORRELATIVE_ENTRIESCOL ) ) )
    )
    {
    return;
    }

  vtkstd::set<vtkstd::set<vtkStdString> >::iterator reqIt;
  vtkstd::set<vtkStdString>::iterator colIt;
  vtkstd::set<vtkstd::pair<vtkStdString,vtkDataArray*> > allColumns;
  vtkstd::map<vtkstd::pair<vtkIdType,vtkIdType>,vtkIdType> colPairs;
  vtkstd::map<vtkstd::pair<vtkIdType,vtkIdType>,vtkIdType>::iterator cpIt;
  vtkstd::map<vtkStdString,vtkIdType> colNameToIdx;
  vtkstd::vector<vtkDataArray*> colPtrs;
  // Reconstruct information about the computed sums from the raw data.
  // The first entry is always the sample size
  double n = mucov->GetValue( 0 );
  vtkIdType m = 0;
  vtkIdType i;
  vtkIdType nmucov = mucov->GetNumberOfTuples();
  for ( i = 1; i < nmucov && ocol2->GetValue( i ).empty(); ++ i, ++ m )
    {
    colNameToIdx[ocol1->GetValue( i )] = m;
    }
  for ( ; i < nmucov; ++ i )
    {
    vtkstd::pair<vtkIdType,vtkIdType> idxs( colNameToIdx[ocol1->GetValue(i)], colNameToIdx[ocol2->GetValue(i)] );
    colPairs[idxs] = i - 1;
    }
  double* rv = mucov->GetPointer( 0 ) + 1;
  double* x = rv;

  // Create an output table for each request and fill it in using the mucov array (the first table in
  // outMeta and which is presumed to exist upon entry to ExecuteDerive).
  // Note that these tables are normalized by the number of samples.
  outMeta->SetNumberOfBlocks( 1 + static_cast<int>( this->Internals->Requests.size() ) );
  // For each request:
  i = 0;
  double scale = 1. / ( this->GetSampleSize() - 1 );
  for ( reqIt = this->Internals->Requests.begin(); reqIt != this->Internals->Requests.end(); ++ reqIt, ++ i )
    {
    vtkStringArray* colNames = vtkStringArray::New();
    colNames->SetName( "Column" );
    vtkDoubleArray* colAvgs = vtkDoubleArray::New();
    colAvgs->SetName( "Column Averages" );
    vtkstd::vector<vtkDoubleArray*> covCols;
    vtkstd::vector<double*> covPtrs;
    vtkstd::vector<int> covIdxs;
    vtksys_ios::ostringstream reqNameStr;
    reqNameStr << "Cov(";
    // For each column in the request:
    for ( colIt = reqIt->begin(); colIt != reqIt->end(); ++ colIt )
      {
      vtkstd::map<vtkStdString,vtkIdType>::iterator idxIt = colNameToIdx.find( *colIt );
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
      }
    reqNameStr << ")";
    covCols.push_back( colAvgs );
    colNames->InsertNextValue( "Cholesky" ); // Need extra row for lower-triangular Cholesky decomposition
    // We now have the total number of columns in the output.
    // Allocate memory for the correct number of rows and fill in values.
    vtkIdType reqCovSize = colNames->GetNumberOfTuples();
    colAvgs->SetNumberOfTuples( reqCovSize );
    vtkTable* covariance = vtkTable::New();
    outMeta->SetBlock( i + 1, covariance );
    outMeta->GetMetaData( static_cast<unsigned>( i + 1 ) )->Set( vtkCompositeDataSet::NAME(), reqNameStr.str().c_str() );
    covariance->Delete(); // outMeta now owns covariance
    covariance->AddColumn( colNames );
    covariance->AddColumn( colAvgs );
    colNames->Delete();
    colAvgs->Delete();
    vtkIdType j = 0;
    for ( vtkstd::vector<vtkDoubleArray*>::iterator arrIt = covCols.begin(); arrIt != covCols.end(); ++ arrIt, ++ j )
      {
      (*arrIt)->SetNumberOfTuples( reqCovSize );
      (*arrIt)->FillComponent( 0, 0. );
      x = (*arrIt)->GetPointer( 0 );
      covPtrs.push_back( x );
      if ( *arrIt != colAvgs )
        { // column is part of covariance matrix
        covariance->AddColumn( *arrIt );
        (*arrIt)->Delete();
        for ( vtkIdType k = 0; k <= j; ++ k, ++ x )
          {
          *x = rv[colPairs[vtkstd::pair<vtkIdType,vtkIdType>( covIdxs[k], covIdxs[j] )]] * scale;
          }
        }
      else
        { // column holds averages
        for ( vtkIdType k = 0; k < reqCovSize - 1; ++ k, ++ x )
          {
          *x = rv[covIdxs[k]];
          }
        *x = static_cast<double>( n );
        }
      }
    vtkMultiCorrelativeCholesky( covPtrs, reqCovSize - 1 );
    }
}

void vtkMultiCorrelativeStatistics::ExecuteAssess(
  vtkTable*, vtkDataObject*, vtkTable*, vtkDataObject* )
{
}

void vtkMultiCorrelativeStatistics::SelectAssessFunctor(
  vtkTable* inData, vtkDataObject* inMeta,
  vtkStringArray* rowNames, vtkStringArray* columnNames, AssessFunctor*& dfunc )
{
  (void)inData;
  (void)inMeta;
  (void)rowNames;
  (void)columnNames;
  (void)dfunc;
}

