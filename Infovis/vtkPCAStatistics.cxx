#include "vtkPCAStatistics.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiCorrelativeStatisticsAssessFunctor.h"
#include "vtkObjectFactory.h"
#ifdef VTK_USE_GNU_R
#include <vtkRInterface.h>
#endif // VTK_USE_GNU_R
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <map>
#include <vector>
#include <vtksys/ios/sstream>

#include "alglib/svd.h"

// To Do:
// - Add option to pre-multiply EigenVectors by normalization coeffs
// - In vtkPCAAssessFunctor, pre-multiply EigenVectors by normalization coeffs (if req)
// -

#define VTK_PCA_NORMCOLUMN "PCA Cov Norm"
#define VTK_PCA_COMPCOLUMN "PCA"

vtkStandardNewMacro(vtkPCAStatistics);

const char* vtkPCAStatistics::NormalizationSchemeEnumNames[NUM_NORMALIZATION_SCHEMES + 1] =
{
  "None",
  "TriangleSpecified",
  "DiagonalSpecified",
  "DiagonalVariance",
  "InvalidNormalizationScheme"
};

const char* vtkPCAStatistics::BasisSchemeEnumNames[NUM_BASIS_SCHEMES + 1] =
{
  "FullBasis",
  "FixedBasisSize",
  "FixedBasisEnergy",
  "InvalidBasisScheme"
};

// ----------------------------------------------------------------------
void vtkPCAStatistics::GetEigenvalues(int request, vtkDoubleArray* eigenvalues)
{
  vtkSmartPointer<vtkMultiBlockDataSet> outputMetaDS =
      vtkMultiBlockDataSet::SafeDownCast(
        this->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

  if(!outputMetaDS)
    {
    vtkErrorMacro(<<"NULL dataset pointer!");
    }

  vtkSmartPointer<vtkTable> outputMeta =
    vtkTable::SafeDownCast( outputMetaDS->GetBlock( request + 1 ) );

  if(!outputMetaDS)
    {
    vtkErrorMacro(<<"NULL table pointer!");
    }

  vtkDoubleArray* meanCol = vtkDoubleArray::SafeDownCast(outputMeta->GetColumnByName("Mean"));
  vtkStringArray* rowNames = vtkStringArray::SafeDownCast(outputMeta->GetColumnByName("Column"));

  eigenvalues->SetNumberOfComponents(1);

  // Get values
  int eval = 0;
  for(vtkIdType i = 0; i < meanCol->GetNumberOfTuples(); i++)
    {
    std::stringstream ss;
    ss << "PCA " << eval;

    std::string rowName = rowNames->GetValue(i);
    if(rowName.compare(ss.str()) == 0)
      {
      eigenvalues->InsertNextValue(meanCol->GetValue(i));
      eval++;
      }
    }

}

// ----------------------------------------------------------------------
double vtkPCAStatistics::GetEigenvalue(int request, int i)
{
  vtkSmartPointer<vtkDoubleArray> eigenvalues =
    vtkSmartPointer<vtkDoubleArray>::New();
  this->GetEigenvalues(request, eigenvalues);
  return eigenvalues->GetValue(i);
}

// ----------------------------------------------------------------------
void vtkPCAStatistics::GetEigenvalues(vtkDoubleArray* eigenvalues)
{
  this->GetEigenvalues(0, eigenvalues);
}

// ----------------------------------------------------------------------
double vtkPCAStatistics::GetEigenvalue(int i)
{
  return this->GetEigenvalue(0,i);
}

// ----------------------------------------------------------------------
void vtkPCAStatistics::GetEigenvectors(int request, vtkDoubleArray* eigenvectors)
{
  // Count eigenvalues
  vtkSmartPointer<vtkDoubleArray> eigenvalues =
    vtkSmartPointer<vtkDoubleArray>::New();
  this->GetEigenvalues(request, eigenvalues);
  vtkIdType numberOfEigenvalues = eigenvalues->GetNumberOfTuples();

  vtkSmartPointer<vtkMultiBlockDataSet> outputMetaDS =
      vtkMultiBlockDataSet::SafeDownCast(
        this->GetOutputDataObject( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

  if(!outputMetaDS)
    {
    vtkErrorMacro(<<"NULL dataset pointer!");
    }

  vtkSmartPointer<vtkTable> outputMeta =
    vtkTable::SafeDownCast( outputMetaDS->GetBlock( request + 1 ) );

  if(!outputMeta)
    {
    vtkErrorMacro(<<"NULL table pointer!");
    }

  vtkDoubleArray* meanCol = vtkDoubleArray::SafeDownCast(outputMeta->GetColumnByName("Mean"));
  vtkStringArray* rowNames = vtkStringArray::SafeDownCast(outputMeta->GetColumnByName("Column"));

  eigenvectors->SetNumberOfComponents(numberOfEigenvalues);

  // Get vectors
  int eval = 0;
  for(vtkIdType i = 0; i < meanCol->GetNumberOfTuples(); i++)
    {
    std::stringstream ss;
    ss << "PCA " << eval;

    std::string rowName = rowNames->GetValue(i);
    if(rowName.compare(ss.str()) == 0)
      {
      std::vector<double> eigenvector;
      for(int val = 0; val < numberOfEigenvalues; val++)
        {
        // The first two columns will always be "Column" and "Mean", so start with the next one
        vtkDoubleArray* currentCol = vtkDoubleArray::SafeDownCast(outputMeta->GetColumn(val+2));
        eigenvector.push_back(currentCol->GetValue(i));
        }

      eigenvectors->InsertNextTupleValue(&eigenvector.front());
      eval++;
      }
    }

}

// ----------------------------------------------------------------------
void vtkPCAStatistics::GetEigenvectors(vtkDoubleArray* eigenvectors)
{
  this->GetEigenvectors(0, eigenvectors);
}

// ----------------------------------------------------------------------
void vtkPCAStatistics::GetEigenvector(int request, int i, vtkDoubleArray* eigenvector)
{
  vtkSmartPointer<vtkDoubleArray> eigenvectors =
    vtkSmartPointer<vtkDoubleArray>::New();
  this->GetEigenvectors(request, eigenvectors);

  double* evec = new double[eigenvectors->GetNumberOfComponents()];
  eigenvectors->GetTupleValue(i, evec);

  eigenvector->Reset();
  eigenvector->Squeeze();
  eigenvector->SetNumberOfComponents(eigenvectors->GetNumberOfComponents());
  eigenvector->InsertNextTupleValue(evec);
  delete[] evec;
}

// ----------------------------------------------------------------------
void vtkPCAStatistics::GetEigenvector(int i, vtkDoubleArray* eigenvector)
{
  this->GetEigenvector(0, i, eigenvector);
}

// ======================================================== vtkPCAAssessFunctor
class vtkPCAAssessFunctor : public vtkMultiCorrelativeAssessFunctor
{
public:
  static vtkPCAAssessFunctor* New();

  vtkPCAAssessFunctor() { }
  virtual ~vtkPCAAssessFunctor() { }
  virtual bool InitializePCA(
                             vtkTable* inData, vtkTable* reqModel,
                             int normScheme, int basisScheme, int basisSize, double basisEnergy );

  virtual void operator () ( vtkVariantArray* result, vtkIdType row );

  std::vector<double> EigenValues;
  std::vector<std::vector<double> > EigenVectors;
  vtkIdType BasisSize;
};

// ----------------------------------------------------------------------
vtkPCAAssessFunctor* vtkPCAAssessFunctor::New()
{
  return new vtkPCAAssessFunctor;
}

// ----------------------------------------------------------------------
bool vtkPCAAssessFunctor::InitializePCA( vtkTable* inData,
                                         vtkTable* reqModel,
                                         int normScheme,
                                         int basisScheme,
                                         int fixedBasisSize,
                                         double fixedBasisEnergy )
{
  if ( ! this->vtkMultiCorrelativeAssessFunctor::Initialize( inData,
                                                             reqModel,
                                                             false /* no Cholesky decomp */ ) )
    {
    return false;
    }

  // Put the PCA basis into a matrix form we can use.
  vtkIdType m = reqModel->GetNumberOfColumns() - 2;
  vtkDoubleArray* evalm = vtkDoubleArray::SafeDownCast( reqModel->GetColumnByName( VTK_MULTICORRELATIVE_AVERAGECOL ) );
  if ( ! evalm )
    {
    vtkGenericWarningMacro( "No \"" VTK_MULTICORRELATIVE_AVERAGECOL "\" column in request." );
    return false;
    }

  // Check that the derived model includes additional rows specifying the
  // normalization as required.
  vtkIdType mrmr = reqModel->GetNumberOfRows(); // actual number of rows
  vtkIdType ermr; // expected number of rows
  switch ( normScheme )
    {
    case vtkPCAStatistics::NONE:
      // m+1 covariance/Cholesky rows, m eigenvector rows, no normalization factors
      ermr = 2 * m + 1;
      break;
    case vtkPCAStatistics::DIAGONAL_SPECIFIED:
    case vtkPCAStatistics::DIAGONAL_VARIANCE:
      // m+1 covariance/Cholesky rows, m eigenvector rows, 1 normalization factor row
      ermr = 2 * m + 2;
      break;
    case vtkPCAStatistics::TRIANGLE_SPECIFIED:
      // m+1 covariance/Cholesky rows, m eigenvector rows, m normalization factor rows
      ermr = 3 * m + 1;
      break;
    case vtkPCAStatistics::NUM_NORMALIZATION_SCHEMES:
    default:
      {
      vtkGenericWarningMacro( "The normalization scheme specified (" << normScheme << ") is invalid." );
      return false;
      }
      break;
    }

  // Allow derived classes to add rows, but never allow fewer than required.
  if ( mrmr < ermr )
    {
    vtkGenericWarningMacro(
                           "Expected " << ( 2 * m + 1 ) << " or more columns in request but found "
                           << reqModel->GetNumberOfRows() << "." );
    return false;
    }

  // OK, we got this far; we should succeed.
  vtkIdType i, j;
  double eigSum = 0.;
  for ( i = 0; i < m; ++ i )
    {
    double eigVal = evalm->GetValue( m + 1 + i );
    eigSum += eigVal;
    this->EigenValues.push_back( eigVal );
    }
  this->BasisSize = -1;
  switch ( basisScheme )
    {
    case vtkPCAStatistics::NUM_BASIS_SCHEMES:
    default:
      vtkGenericWarningMacro( "Unknown basis scheme " << basisScheme << ". Using FULL_BASIS." );
      // fall through
    case vtkPCAStatistics::FULL_BASIS:
      this->BasisSize = m;
      break;
    case vtkPCAStatistics::FIXED_BASIS_SIZE:
      this->BasisSize = fixedBasisSize;
      break;
    case vtkPCAStatistics::FIXED_BASIS_ENERGY:
      {
      double frac = 0.;
      for ( i = 0; i < m; ++ i )
        {
        frac += this->EigenValues[i] / eigSum;
        if ( frac > fixedBasisEnergy )
          {
          this->BasisSize = i + 1;
          break;
          }
        }
      if ( this->BasisSize < 0 )
        { // OK, it takes all the eigenvectors to approximate that well...
        this->BasisSize = m;
        }
      }
      break;
    }

  // FIXME: Offer mode to include normalization factors (none,diag,triang)?
  // Could be done here by pre-multiplying this->EigenVectors by factors.
  for ( i = 0; i < this->BasisSize; ++ i )
    {
    std::vector<double> evec;
    for ( j = 0; j < m; ++ j )
      {
      evec.push_back( reqModel->GetValue( m + 1 + i, j + 2 ).ToDouble() );
      }
    this->EigenVectors.push_back( evec );
    }
  return true;
}

// ----------------------------------------------------------------------
void vtkPCAAssessFunctor::operator () ( vtkVariantArray* result, vtkIdType row )
{
  vtkIdType i;
  result->SetNumberOfValues( this->BasisSize );
  std::vector<std::vector<double> >::iterator it;
  vtkIdType m = this->GetNumberOfColumns();
  for ( i = 0; i < m; ++ i )
    {
    this->Tuple[i] = this->Columns[i]->GetTuple( row )[0] - this->Center[i];
    }
  i = 0;
  for ( it = this->EigenVectors.begin(); it != this->EigenVectors.end(); ++ it, ++ i )
    {
    double cv = 0.;
    std::vector<double>::iterator tvit;
    std::vector<double>::iterator evit = this->Tuple.begin();
    for ( tvit = it->begin(); tvit != it->end(); ++ tvit, ++ evit )
      {
      cv += (*evit) * (*tvit);
      }
    result->SetValue( i, cv );
    }
}

// ======================================================== vtkPCAStatistics
vtkPCAStatistics::vtkPCAStatistics()
{
  this->SetNumberOfInputPorts( 4 ); // last port is for normalization coefficients.
  this->NormalizationScheme = NONE;
  this->BasisScheme = FULL_BASIS;
  this->FixedBasisSize = -1;
  this->FixedBasisEnergy = 1.;
}

// ----------------------------------------------------------------------
vtkPCAStatistics::~vtkPCAStatistics()
{
}

// ----------------------------------------------------------------------
void vtkPCAStatistics::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "NormalizationScheme: " << this->GetNormalizationSchemeName( this->NormalizationScheme ) << "\n";
  os << indent << "BasisScheme: " << this->GetBasisSchemeName( this->BasisScheme ) << "\n";
  os << indent << "FixedBasisSize: " << this->FixedBasisSize << "\n";
  os << indent << "FixedBasisEnergy: " << this->FixedBasisEnergy << "\n";
}

// ----------------------------------------------------------------------
bool vtkPCAStatistics::SetParameter( const char* parameter,
                                     int vtkNotUsed( index ),
                                     vtkVariant value )
{
  if ( ! strcmp( parameter, "NormalizationScheme" ) )
    {
    this->SetNormalizationScheme( value.ToInt() );

    return true;
    }

  if ( ! strcmp( parameter, "BasisScheme" ) )
    {
    this->SetBasisScheme( value.ToInt() );

    return true;
    }

  if ( ! strcmp( parameter, "FixedBasisSize" ) )
    {
    this->SetFixedBasisSize( value.ToInt() );

    return true;
    }

  if ( ! strcmp( parameter, "FixedBasisEnergy" ) )
    {
    this->SetFixedBasisEnergy( value.ToDouble() );

    return true;
    }

  return false;
}

// ----------------------------------------------------------------------
const char* vtkPCAStatistics::GetNormalizationSchemeName( int schemeIndex )
{
  if ( schemeIndex < 0 || schemeIndex > NUM_NORMALIZATION_SCHEMES )
    {
    return vtkPCAStatistics::NormalizationSchemeEnumNames[NUM_NORMALIZATION_SCHEMES];
    }
  return vtkPCAStatistics::NormalizationSchemeEnumNames[schemeIndex];
}

// ----------------------------------------------------------------------
void vtkPCAStatistics::SetNormalizationSchemeByName( const char* schemeName )
{
  for ( int i = 0; i < NUM_NORMALIZATION_SCHEMES; ++ i )
    {
    if ( ! strcmp( vtkPCAStatistics::NormalizationSchemeEnumNames[i], schemeName ) )
      {
      this->SetNormalizationScheme( i );
      return;
      }
    }
  vtkErrorMacro( "Invalid normalization scheme name \"" << schemeName << "\" provided." );
}


// ----------------------------------------------------------------------
vtkTable* vtkPCAStatistics::GetSpecifiedNormalization()
{
  return vtkTable::SafeDownCast( this->GetInputDataObject( 3, 0 ) );
}

void vtkPCAStatistics::SetSpecifiedNormalization( vtkTable* normSpec )
{
  this->SetInput( 3, normSpec );
}

// ----------------------------------------------------------------------
const char* vtkPCAStatistics::GetBasisSchemeName( int schemeIndex )
{
  if ( schemeIndex < 0 || schemeIndex > NUM_BASIS_SCHEMES )
    {
    return vtkPCAStatistics::BasisSchemeEnumNames[NUM_BASIS_SCHEMES];
    }
  return vtkPCAStatistics::BasisSchemeEnumNames[schemeIndex];
}

// ----------------------------------------------------------------------
void vtkPCAStatistics::SetBasisSchemeByName( const char* schemeName )
{
  for ( int i = 0; i < NUM_BASIS_SCHEMES; ++ i )
    {
    if ( ! strcmp( vtkPCAStatistics::BasisSchemeEnumNames[i], schemeName ) )
      {
      this->SetBasisScheme( i );
      return;
      }
    }
  vtkErrorMacro( "Invalid basis scheme name \"" << schemeName << "\" provided." );
}

// ----------------------------------------------------------------------
int vtkPCAStatistics::FillInputPortInformation( int port, vtkInformation* info )
{
  if ( port == 3 )
    {
    info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable" );
    info->Set( vtkAlgorithm::INPUT_IS_OPTIONAL(), 1 );
    return 1;
    }
  return this->Superclass::FillInputPortInformation( port, info );
}

// ----------------------------------------------------------------------
static void vtkPCAStatisticsNormalizeSpec( vtkVariantArray* normData,
                                           ap::real_2d_array& cov,
                                           vtkTable* normSpec,
                                           vtkTable* reqModel,
                                           bool triangle )
{
  vtkIdType i, j;
  vtkIdType m = reqModel->GetNumberOfColumns() - 2;
  std::map<vtkStdString,vtkIdType> colNames;
  // Get a list of columns of interest for this request
  for ( i = 0; i < m; ++ i )
    {
    colNames[ reqModel->GetColumn( i + 2 )->GetName() ] = i;
    }
  // Turn normSpec into a useful array.
  std::map<std::pair<vtkIdType,vtkIdType>,double> factor;
  vtkIdType n = normSpec->GetNumberOfRows();
  for ( vtkIdType r = 0; r < n; ++ r )
    {
    std::map<vtkStdString,vtkIdType>::iterator it;
    if ( ( it = colNames.find( normSpec->GetValue( r, 0 ).ToString() ) ) == colNames.end() )
      {
      continue;
      }
    i = it->second;
    if ( ( it = colNames.find( normSpec->GetValue( r, 1 ).ToString() ) ) == colNames.end() )
      {
      continue;
      }
    j = it->second;
    if ( j < i )
      {
      vtkIdType tmp = i;
      i = j;
      j = tmp;
      }
    factor[std::pair<vtkIdType,vtkIdType>( i, j )] = normSpec->GetValue( r, 2 ).ToDouble();
    }
  // Now normalize cov, recording any missing factors along the way.
  vtksys_ios::ostringstream missing;
  bool gotMissing = false;
  std::map<std::pair<vtkIdType,vtkIdType>,double>::iterator fit;
  if ( triangle )
    { // Normalization factors are provided for the upper triangular portion of the covariance matrix.
    for ( i = 0; i < m; ++ i )
      {
      for ( j = i; j < m; ++ j )
        {
        double v;
        fit = factor.find( std::pair<vtkIdType,vtkIdType>( i, j ) );
        if ( fit == factor.end() )
          {
          v = 1.;
          gotMissing = true;
          missing
            << "(" << reqModel->GetColumn( i + 2 )->GetName()
            << "," << reqModel->GetColumn( j + 2 )->GetName()
            << ") ";
          }
        else
          {
          v = fit->second;
          }
        normData->InsertNextValue( v );
        cov( i, j ) /= v;
        if ( i != j )
          { // don't normalize diagonal entries twice
          cov( j, i ) /= v;
          }
        }
      }
    }
  else
    { // Only diagonal normalization factors are supplied. Off-diagonals are the product of diagonals.
    for ( i = 0; i < m; ++ i )
      {
      double v;
      double vsq;
      fit = factor.find( std::pair<vtkIdType,vtkIdType>( i, i ) );
      if ( fit == factor.end() )
        {
        vsq = v = 1.;
        gotMissing = true;
        missing
          << "(" << reqModel->GetColumn( i + 2 )->GetName()
          << "," << reqModel->GetColumn( i + 2 )->GetName()
          << ") ";
        }
      else
        {
        vsq = fit->second;
        v = sqrt( vsq );
        }
      normData->InsertNextValue( vsq );
      // normalization factor applied up and to the left.
      for ( j = 0; j < i; ++ j )
        {
        cov( i, j ) /= v;
        cov( j, i ) /= v;
        }
      // normalization factor applied down and to the right.
      for ( j = i + 1; j < m; ++ j )
        {
        cov( i, j ) /= v;
        cov( j, i ) /= v;
        }
      cov( i, i ) /= vsq;
      }
    }
  if ( gotMissing )
    {
    vtkGenericWarningMacro(
                           "The following normalization factors were expected but not provided: "
                           << missing.str().c_str() );
    }
}

// ----------------------------------------------------------------------
static void vtkPCAStatisticsNormalizeVariance( vtkVariantArray* normData,
                                               ap::real_2d_array& cov )
{
  vtkIdType i, j;
  vtkIdType m = cov.gethighbound( 0 ) - cov.getlowbound( 0 ) + 1;
  for ( i = 0; i < m; ++ i )
    {
    normData->InsertNextValue( cov( i, i ) );
    double norm = sqrt( cov( i, i ) );
    // normalization factor applied down and to the right.
    for ( j = i + 1; j < m; ++ j )
      {
      cov( i, j ) /= norm;
      cov( j, i ) /= norm;
      }
    // normalization factor applied up and to the left.
    for ( j = 0; j < i; ++ j )
      {
      cov( i, j ) /= norm;
      cov( j, i ) /= norm;
      }
    cov( i, i ) = 1.;
    }
}

// ----------------------------------------------------------------------
void vtkPCAStatistics::Derive( vtkMultiBlockDataSet* inMeta )
{
  if ( ! inMeta )
    {
    return;
    }

  // Use the parent class to compute a covariance matrix for each request.
  this->Superclass::Derive( inMeta );

  // Now that we have the covariance matrices, compute the SVD of each.
  vtkIdType nb = static_cast<vtkIdType>( inMeta->GetNumberOfBlocks() );
  for ( vtkIdType b = 1; b < nb; ++ b )
    {
    vtkTable* reqModel = vtkTable::SafeDownCast( inMeta->GetBlock( b ) );
    if ( ! reqModel )
      {
      continue;
      }
    vtkIdType m = reqModel->GetNumberOfColumns() - 2;
    ap::real_2d_array cov;
    cov.setbounds( 0, m - 1, 0, m - 1 );
    // Fill the cov array with values from the vtkTable
    vtkIdType i, j;
    for ( j = 2; j < 2 + m; ++ j )
      {
      for ( i = 0; i < j - 1; ++ i )
        {
        cov( i, j - 2 ) = reqModel->GetValue( i, j ).ToDouble();
        }
      }
    // Fill in the lower triangular portion of the matrix
    for ( j = 0; j < m - 1; ++ j )
      {
      for ( i = j; i < m; ++ i )
        {
        cov( i, j ) = cov( j, i );
        }
      }
    // If normalization of the covariance array is requested, perform it:
    vtkVariantArray* normData = vtkVariantArray::New();
    switch ( this->NormalizationScheme )
      {
      case TRIANGLE_SPECIFIED:
      case DIAGONAL_SPECIFIED:
        vtkPCAStatisticsNormalizeSpec(
                                      normData, cov, this->GetSpecifiedNormalization(), reqModel,
                                      this->NormalizationScheme == TRIANGLE_SPECIFIED );
        break;
      case DIAGONAL_VARIANCE:
        vtkPCAStatisticsNormalizeVariance( normData, cov );
        break;
      case NONE:
      case NUM_NORMALIZATION_SCHEMES:
      default:
        // do nothing
        break;
      }
    ap::real_2d_array u;
    ap::real_1d_array s;
    ap::real_2d_array vt;
    // Now that we have the covariance matrix, compute the SVD.
    // Note that vt is not computed since the VtNeeded parameter is 0.
    bool status = rmatrixsvd( cov, m, m, 2, 0, 2, s, u, vt );
    if ( ! status )
      {
      vtkWarningMacro( "Could not compute PCA for request " << b );
      continue;
      }
    vtkVariantArray* row = vtkVariantArray::New();
    //row->SetNumberOfComponents( m + 2 );
    //row->SetNumberOfTuples( 1 );
    row->SetNumberOfComponents( 1 );
    row->SetNumberOfTuples( m + 2 );
    for ( i = 0; i < m; ++ i )
      {
      vtksys_ios::ostringstream pcaCompName;
      pcaCompName << VTK_PCA_COMPCOLUMN << " " << i;
      row->SetValue( 0, pcaCompName.str().c_str() );
      row->SetValue( 1, s( i ) );
      for ( j = 0; j < m; ++ j )
        {
        // transpose the matrix so basis is row vectors (and thus
        // eigenvalues are to the left of their eigenvectors):
        row->SetValue( j + 2, u( j, i ) );
        }
      reqModel->InsertNextRow( row );
      }
    // Now insert the subset of the normalization data we used to
    // process this request at the bottom of the results.
    switch ( this->NormalizationScheme )
      {
      case TRIANGLE_SPECIFIED:
        for ( i = 0; i < m; ++ i )
          {
          vtksys_ios::ostringstream normCompName;
          normCompName << VTK_PCA_NORMCOLUMN << " " << i;
          row->SetValue( 0, normCompName.str().c_str() );
          row->SetValue( 1, 0. );
          for ( j = 0; j < i; ++ j )
            {
            row->SetValue( j + 2, 0. );
            }
          for ( ; j < m; ++ j )
            {
            row->SetValue( j + 2, normData->GetValue( j ) );
            }
          reqModel->InsertNextRow( row );
          }
        break;
      case DIAGONAL_SPECIFIED:
      case DIAGONAL_VARIANCE:
        {
        row->SetValue( 0, VTK_PCA_NORMCOLUMN );
        row->SetValue( 1, 0. );
        for ( j = 0; j < m; ++ j )
          {
          row->SetValue( j + 2, normData->GetValue( j ) );
          }
        reqModel->InsertNextRow( row );
        }
        break;
      case NONE:
      case NUM_NORMALIZATION_SCHEMES:
      default:
        // do nothing
        break;
      }
    normData->Delete();
    row->Delete();
    }
}

// ----------------------------------------------------------------------
void vtkPCAStatistics::Test( vtkTable* inData,
                             vtkMultiBlockDataSet* inMeta,
                             vtkTable* outMeta )

{
  if ( ! inMeta )
    {
    return;
    }

  if ( ! outMeta )
    {
    return;
    }

  // Prepare columns for the test:
  // 0: (derived) model block index
  // 1: multivariate Srivastava skewness
  // 2: multivariate Srivastava kurtosis
  // 3: multivariate Jarque-Bera-Srivastava statistic
  // 4: multivariate Jarque-Bera-Srivastava p-value (calculated only if R is available, filled with -1 otherwise)
  // 5: number of degrees of freedom of Chi square distribution
  // NB: These are not added to the output table yet, for they will be filled individually first
  //     in order that R be invoked only once.
  vtkIdTypeArray* blockCol = vtkIdTypeArray::New();
  blockCol->SetName( "Block" );

  vtkDoubleArray* bS1Col = vtkDoubleArray::New();
  bS1Col->SetName( "Srivastava Skewness" );

  vtkDoubleArray* bS2Col = vtkDoubleArray::New();
  bS2Col->SetName( "Srivastava Kurtosis" );

  vtkDoubleArray* statCol = vtkDoubleArray::New();
  statCol->SetName( "Jarque-Bera-Srivastava" );

  vtkIdTypeArray* dimCol = vtkIdTypeArray::New();
  dimCol->SetName( "d" );

  // Retain data cardinality to check that models are applicable
  vtkIdType nRowData = inData->GetNumberOfRows();

  // Now iterate over model blocks
  unsigned int nBlocks = inMeta->GetNumberOfBlocks();
  for ( unsigned int b = 1; b < nBlocks; ++ b )
    {
    vtkTable* derivedTab = vtkTable::SafeDownCast( inMeta->GetBlock( b ) );

    // Silenty ignore empty blocks
    if ( ! derivedTab )
      {
      continue;
      }

    // Figure out dimensionality; it is assumed that the 2 first columns
    // are what they should be: namely, Column and Mean.
    int p = derivedTab->GetNumberOfColumns() - 2;

    // Return informative message when cardinalities do not match.
    if ( derivedTab->GetValueByName( p, "Mean" ).ToInt() != nRowData )
      {
      vtkWarningMacro( "Inconsistent input: input data has "
                       << nRowData
                       << " rows but primary model has cardinality "
                       << derivedTab->GetValueByName( p, "Mean" ).ToInt()
                       << " for block "
                       << b
                       <<". Cannot test." );
      continue;
      }

    // Create and fill entries of name and mean vectors
    vtkStdString *varNameX = new vtkStdString[p];
    double *mX = new double[p];
    for ( int i = 0; i < p; ++ i )
      {
      varNameX[i] = derivedTab->GetValueByName( i, "Column" ).ToString();
      mX[i] = derivedTab->GetValueByName( i, "Mean" ).ToDouble();
      }

    // Create and fill entries of eigenvalue vector and change of basis matrix
    double *wX = new double[p];
    double *P = new double[p * p];
    for ( int i = 0; i < p; ++ i )
      {
      // Skip p + 1 (Means and Cholesky) rows and 1 column (Column)
      wX[i] = derivedTab->GetValue( i + p + 1, 1).ToDouble();

      for ( int j = 0; j < p; ++ j )
        {
        // Skip p + 1 (Means and Cholesky) rows and 2 columns (Column and Mean)
        P[p * i + j] = derivedTab->GetValue( i + p + 1, j + 2).ToDouble();
        }
      }

    // Now iterate over all observations
    double tmp, t;
    double *x = new double[p];
    double *sum3 = new double[p];
    double *sum4 = new double[p];
    for ( int i = 0; i < p; ++ i )
      {
      sum3[i] = 0.;
      sum4[i] = 0.;
      }
    for ( vtkIdType r = 0; r < nRowData; ++ r )
      {
      // Read and center observation
      for ( int i = 0; i < p; ++ i )
        {
        x[i] = inData->GetValueByName( r, varNameX[i] ).ToDouble() - mX[i];
        }

      // Now calculate skewness and kurtosis per component
      for ( int i = 0; i < p; ++ i )
        {
        // Transform coordinate into eigencoordinates
        t = 0.;
        for ( int j = 0; j < p; ++ j )
          {
          // Pij = P[p*i+j]
          t += P[p * i + j] * x[j];
          }

        // Update third and fourth order sums for each eigencoordinate
        tmp = t * t;
        sum3[i] += tmp * t;
        sum4[i] += tmp * tmp;
        }
      }

    // Finally calculate moments by normalizing sums with corresponding eigenvalues and powers
    double bS1 = 0.;
    double bS2 = 0.;
    for ( int i = 0; i < p; ++ i )
      {
      tmp = wX[i] * wX[i];
      bS1 += sum3[i] * sum3[i] / ( tmp * wX[i] );
      bS2 += sum4[i] / tmp;
      }
    bS1 /= ( nRowData * nRowData * p );
    bS2 /= ( nRowData * p );

    // Finally, calculate Jarque-Bera-Srivastava statistic
    tmp = bS2 - 3.;
    double jbs = static_cast<double>( nRowData * p ) * ( bS1 / 6. + ( tmp * tmp ) / 24. );

    // Insert variable name and calculated Jarque-Bera-Srivastava statistic
    blockCol->InsertNextValue( b );
    bS1Col->InsertNextTuple1( bS1 );
    bS2Col->InsertNextTuple1( bS2 );
    statCol->InsertNextTuple1( jbs );
    dimCol->InsertNextTuple1( p + 1 );

    // Clean up
    delete [] sum3;
    delete [] sum4;
    delete [] x;
    delete [] P;
    delete [] wX;
    delete [] mX;
    delete [] varNameX;
    } // b

  // Now, add the already prepared columns to the output table
  outMeta->AddColumn( blockCol );
  outMeta->AddColumn( bS1Col );
  outMeta->AddColumn( bS2Col );
  outMeta->AddColumn( statCol );
  outMeta->AddColumn( dimCol );

  // Last phase: compute the p-values or assign invalid value if they cannot be computed
  vtkDoubleArray* testCol = 0;
  bool calculatedP = false;

  // If available, use R to obtain the p-values for the Chi square distribution with 2 DOFs
#ifdef VTK_USE_GNU_R
  // Prepare VTK - R interface
  vtkRInterface* ri = vtkRInterface::New();

  // Use the calculated Jarque-Bera-Srivastava statistics as input to the Chi square function
  ri->AssignVTKDataArrayToRVariable( statCol, "jbs" );
  ri->AssignVTKDataArrayToRVariable( dimCol, "d" );

  // Calculate the p-values (p+1 degrees of freedom)
  // Now prepare R script and calculate the p-values (in a single R script evaluation for efficiency)
  vtksys_ios::ostringstream rs;
  rs << "p<-c();"
     << "for(i in 1:"
     << dimCol->GetNumberOfTuples()
     << "){"
     << "p<-c(p,1-pchisq(jbs[i],d[i]));"
     << "}";
  ri->EvalRscript( rs.str().c_str() );

  // Retrieve the p-values
  testCol = vtkDoubleArray::SafeDownCast( ri->AssignRVariableToVTKDataArray( "p" ) );
  if ( ! testCol || testCol->GetNumberOfTuples() != statCol->GetNumberOfTuples() )
    {
    vtkWarningMacro( "Something went wrong with the R calculations. Reported p-values will be invalid." );
    }
  else
    {
    // Test values have been calculated by R: the test column can be added to the output table
    outMeta->AddColumn( testCol );
    calculatedP = true;
    }

  // Clean up
  ri->Delete();
#endif // VTK_USE_GNU_R

  // Use the invalid value of -1 for p-values if R is absent or there was an R error
  if ( ! calculatedP )
    {
    // A column must be created first
    testCol = vtkDoubleArray::New();

    // Fill this column
    vtkIdType n = statCol->GetNumberOfTuples();
    testCol->SetNumberOfTuples( n );
    for ( vtkIdType r = 0; r < n; ++ r )
      {
      testCol->SetTuple1( r, -1 );
      }

    // Now add the column of invalid values to the output table
    outMeta->AddColumn( testCol );

    // Clean up
    testCol->Delete();
    }

  // The test column name can only be set after the column has been obtained from R
  testCol->SetName( "P" );

  // Clean up
  blockCol->Delete();
  bS1Col->Delete();
  bS2Col->Delete();
  statCol->Delete();
  dimCol->Delete();
}
// ----------------------------------------------------------------------
void vtkPCAStatistics::Assess( vtkTable* inData,
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

  // For each request, add a column to the output data related to the likelihood of each input datum wrt the model in the request.
  // Column names of the metadata and input data are assumed to match.
  // The output columns will be named "RelDevSq(A,B,C)" where "A", "B", and "C" are the column names specified in the
  // per-request metadata tables.
  vtkIdType nRow = inData->GetNumberOfRows();
  int nb = static_cast<int>( inMeta->GetNumberOfBlocks() );
  AssessFunctor* dfunc = 0;
  for ( int req = 1; req < nb; ++ req )
    {
    vtkTable* reqModel = vtkTable::SafeDownCast( inMeta->GetBlock( req ) );
    if ( ! reqModel )
      { // silently skip invalid entries. Note we leave assessValues column in output data even when it's empty.
      continue;
      }

    this->SelectAssessFunctor( inData,
                               reqModel,
                               0,
                               dfunc );

    vtkPCAAssessFunctor* pcafunc = static_cast<vtkPCAAssessFunctor*>( dfunc );
    if ( ! pcafunc )
      {
      vtkWarningMacro( "Request " << req - 1 << " could not be accommodated. Skipping." );
      if ( dfunc )
        {
        delete dfunc;
        }
      continue;
      }

    // Create an array to hold the assess values for all the input data
    std::vector<double*> assessValues;
    int comp;
    for ( comp = 0; comp < pcafunc->BasisSize; ++ comp )
      {
      vtksys_ios::ostringstream reqNameStr;
      reqNameStr << VTK_PCA_COMPCOLUMN << "{";
      for ( int i = 0; i < pcafunc->GetNumberOfColumns(); ++ i )
        {
        if ( i > 0 )
          {
          reqNameStr << ",";
          }
        reqNameStr << pcafunc->GetColumn(i)->GetName();
        }
      reqNameStr << "}(" << comp << ")";
      vtkDoubleArray* arr = vtkDoubleArray::New();
      arr->SetName( reqNameStr.str().c_str() );
      arr->SetNumberOfTuples( nRow );
      outData->AddColumn( arr );
      arr->Delete();
      assessValues.push_back( arr->GetPointer( 0 ) );
      }

    // Something to hold assessed values for a single input datum
    vtkVariantArray* singleResult = vtkVariantArray::New();
    // Loop over all the input data and assess each datum:
    for ( vtkIdType row = 0; row < nRow; ++ row )
      {
      (*dfunc)( singleResult, row );
      for ( comp = 0; comp < pcafunc->BasisSize; ++ comp )
        {
        assessValues[comp][row] = singleResult->GetValue( comp ).ToDouble();
        }
      }
    delete dfunc;
    singleResult->Delete();
    }
}

// ----------------------------------------------------------------------
void vtkPCAStatistics::SelectAssessFunctor( vtkTable* inData,
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

  vtkPCAAssessFunctor* pcafunc = vtkPCAAssessFunctor::New();
  if ( ! pcafunc->InitializePCA(
                                inData, reqModel, this->NormalizationScheme,
                                this->BasisScheme, this->FixedBasisSize, this->FixedBasisEnergy ) )
    {
    delete pcafunc;
    return;
    }

  dfunc = pcafunc;
}
