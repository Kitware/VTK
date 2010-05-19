#include "vtkPCAStatistics.h"

#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiCorrelativeStatisticsAssessFunctor.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtkstd/map>
#include <vtkstd/vector>
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

  vtkstd::vector<double> EigenValues;
  vtkstd::vector<vtkstd::vector<double> > EigenVectors;
  vtkIdType BasisSize;
};

vtkPCAAssessFunctor* vtkPCAAssessFunctor::New()
{
  return new vtkPCAAssessFunctor;
}

bool vtkPCAAssessFunctor::InitializePCA(
                                        vtkTable* inData, vtkTable* reqModel,
                                        int normScheme, int basisScheme, int fixedBasisSize, double fixedBasisEnergy )
{
  if ( ! this->vtkMultiCorrelativeAssessFunctor::Initialize(
                                                            inData, reqModel, false /* no Cholesky decomp */ ) )
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
    vtkstd::vector<double> evec;
    for ( j = 0; j < m; ++ j )
      {
      evec.push_back( reqModel->GetValue( m + 1 + i, j + 2 ).ToDouble() );
      }
    this->EigenVectors.push_back( evec );
    }
  return true;
}

void vtkPCAAssessFunctor::operator () ( vtkVariantArray* result, vtkIdType row )
{
  vtkIdType i;
  result->SetNumberOfValues( this->BasisSize );
  vtkstd::vector<vtkstd::vector<double> >::iterator it;
  vtkIdType m = this->GetNumberOfColumns();
  for ( i = 0; i < m; ++ i )
    {
    this->Tuple[i] = this->Columns[i]->GetTuple( row )[0] - this->Center[i];
    }
  i = 0;
  for ( it = this->EigenVectors.begin(); it != this->EigenVectors.end(); ++ it, ++ i )
    {
    double cv = 0.;
    vtkstd::vector<double>::iterator tvit;
    vtkstd::vector<double>::iterator evit = this->Tuple.begin();
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
  vtkstd::map<vtkStdString,vtkIdType> colNames;
  // Get a list of columns of interest for this request
  for ( i = 0; i < m; ++ i )
    {
    colNames[ reqModel->GetColumn( i + 2 )->GetName() ] = i;
    }
  // Turn normSpec into a useful array.
  vtkstd::map<vtkstd::pair<vtkIdType,vtkIdType>,double> factor;
  vtkIdType n = normSpec->GetNumberOfRows();
  for ( vtkIdType r = 0; r < n; ++ r )
    {
    vtkstd::map<vtkStdString,vtkIdType>::iterator it;
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
    factor[vtkstd::pair<vtkIdType,vtkIdType>( i, j )] = normSpec->GetValue( r, 2 ).ToDouble();
    }
  // Now normalize cov, recording any missing factors along the way.
  vtksys_ios::ostringstream missing;
  bool gotMissing = false;
  vtkstd::map<vtkstd::pair<vtkIdType,vtkIdType>,double>::iterator fit;
  if ( triangle )
    { // Normalization factors are provided for the upper triangular portion of the covariance matrix.
    for ( i = 0; i < m; ++ i )
      {
      for ( j = i; j < m; ++ j )
        {
        double v;
        fit = factor.find( vtkstd::pair<vtkIdType,vtkIdType>( i, j ) );
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
      fit = factor.find( vtkstd::pair<vtkIdType,vtkIdType>( i, i ) );
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
void vtkPCAStatistics::Derive( vtkDataObject* inMetaDO )
{
  vtkMultiBlockDataSet* inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
  if ( ! inMeta )
    {
    vtkWarningMacro(
                    "Expected a vtkMultiBlockDataSet but was given "
                    << ( inMetaDO ? inMetaDO->GetClassName() : "null pointer" )
                    << "instead" );
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
      row->SetValue( 1, s(i) );
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
void vtkPCAStatistics::Assess( vtkTable* inData, 
                               vtkDataObject* inMetaDO, 
                               vtkTable* outData )
{
  vtkMultiBlockDataSet* inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
  if ( ! inMeta || ! outData )
    {
    return;
    }

  if ( inData->GetNumberOfColumns() <= 0 )
    {
    return;
    }

  vtkIdType nsamples = inData->GetNumberOfRows();
  if ( nsamples <= 0 )
    {
    return;
    }

  // For each request, add a column to the output data related to the likelihood of each input datum wrt the model in the request.
  // Column names of the metadata and input data are assumed to match (no mapping using AssessNames or AssessParameters is done).
  // The output columns will be named "RelDevSq(A,B,C)" where "A", "B", and "C" are the column names specified in the
  // per-request metadata tables.
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
    vtkstd::vector<double*> assessValues;
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
      arr->SetNumberOfTuples( nsamples );
      outData->AddColumn( arr );
      arr->Delete();
      assessValues.push_back( arr->GetPointer( 0 ) );
      }

    // Something to hold assessed values for a single input datum
    vtkVariantArray* singleResult = vtkVariantArray::New();
    // Loop over all the input data and assess each datum:
    for ( vtkIdType row = 0; row < nsamples; ++ row )
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

