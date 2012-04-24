#include "vtkMath.h"
#include "vtkStdString.h"
#include "vtkIntArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPolyData.h"
#include "vtkExodusIIReader.h"
#include "vtkExodusIIReaderPrivate.h"
#include "vtkExodusIIReaderVariableCheck.h"
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>
#include <vtksys/String.hxx>
#include <vector>
#include <set>

#include <ctype.h>

bool vtkExodusIIReaderVariableCheck::Start( vtksys_stl::string name, const int* truth, int numTruth )
{
  this->SeqTruth.clear();
  this->SeqTruth.insert( this->SeqTruth.begin(), truth, truth + numTruth );
  this->OriginalNames.clear();
  bool result = this->StartInternal( name, truth, numTruth );
  bool atLeastOne = false;
  for ( int i = 0; i < numTruth; ++ i )
    {
    if ( truth[i] )
      atLeastOne = true;
    }
  return result && atLeastOne;
}

std::vector<vtksys_stl::string>::size_type vtkExodusIIReaderVariableCheck::Length()
{
  return this->OriginalNames.size();
}

int vtkExodusIIReaderVariableCheck::Accept(
  std::vector<vtkExodusIIReaderPrivate::ArrayInfoType>& arr,
  int startIndex, vtkExodusIIReaderPrivate* priv, int objtyp )
{
  vtksys_stl::string::size_type len = this->Length();
  vtkExodusIIReaderPrivate::ArrayInfoType ainfo;
  ainfo.Name = this->Prefix;
  ainfo.Source = vtkExodusIIReaderPrivate::Result;
  ainfo.Components = static_cast<int>( len );
  for ( unsigned int i = 0; i < len; ++ i )
    {
    ainfo.OriginalIndices.push_back( startIndex + i + 1 /* FORTRAN. Blech. */ );
    ainfo.OriginalNames.push_back( this->OriginalNames[i] );
    }
  ainfo.GlomType = this->GlomType;
  ainfo.StorageType = VTK_DOUBLE;
  ainfo.Status = 0;
  ainfo.ObjectTruth = this->SeqTruth;
  this->UniquifyName( ainfo, arr );
  if ( priv )
    {
    priv->GetInitialObjectArrayStatus( objtyp, &ainfo );
    }
  arr.push_back( ainfo );
  return static_cast<int>( this->Length() );
}

vtkExodusIIReaderVariableCheck::vtkExodusIIReaderVariableCheck()
{
  this->GlomType = -1;
}

vtkExodusIIReaderVariableCheck::~vtkExodusIIReaderVariableCheck()
{
}

bool vtkExodusIIReaderVariableCheck::CheckTruth( const int* truth )
{
  if ( ! truth )
    return false;

  for ( std::vector<int>::iterator it = this->SeqTruth.begin(); it != this->SeqTruth.end(); ++ it, ++ truth )
    {
    if ( *truth != *it )
      {
      return false;
      }
    }

  return true;
}

bool vtkExodusIIReaderVariableCheck::UniquifyName(
    vtkExodusIIReaderPrivate::ArrayInfoType& ainfo,
    std::vector<vtkExodusIIReaderPrivate::ArrayInfoType>& arrays )
{
  bool nameChanged = false;
  std::vector<vtkExodusIIReaderPrivate::ArrayInfoType>::iterator it = arrays.begin();
  while ( it != arrays.end() )
    {
    if ( it->Name == ainfo.Name )
      {
      nameChanged = true;
      ainfo.Name.append( "_" );
      it = arrays.begin(); // Have to start over now that we've changed the name.
      }
    else
      {
      ++ it;
      }
    }
  return nameChanged;
}

vtkExodusIIReaderScalarCheck::vtkExodusIIReaderScalarCheck()
{
  this->GlomType = vtkExodusIIReaderPrivate::Scalar;
}

bool vtkExodusIIReaderScalarCheck::StartInternal( vtksys_stl::string name, const int*, int )
{
  this->Prefix = name;
  this->OriginalNames.push_back( name );
  return false;
}

bool vtkExodusIIReaderScalarCheck::Add( vtksys_stl::string, const int* )
{ // Scalars never have more than 1 name
  return false;
}

vtkExodusIIReaderVectorCheck::vtkExodusIIReaderVectorCheck( const char* seq, int n )
{
  this->Endings.clear();
  this->Endings.insert( this->Endings.begin(), seq, seq + n );
  this->Endings = vtksys::SystemTools::LowerCase( this->Endings );
  switch ( n )
    {
  case 2:
    this->GlomType = vtkExodusIIReaderPrivate::Vector2;
    break;
  case 3:
    this->GlomType = vtkExodusIIReaderPrivate::Vector3;
    break;
  default:
    this->GlomType = -1; // Oops. What goes here?
    break;
    }
}

bool vtkExodusIIReaderVectorCheck::StartInternal( vtksys_stl::string name, const int*, int )
{
  vtksys_stl::string::size_type len = name.size();
  if ( len > 1 && tolower( name[len - 1] ) == this->Endings[0] )
    {
    this->Prefix = name.substr( 0, len - 1 );
    this->OriginalNames.push_back( name );
    this->StillAdding = true;
    return true;
    }
  this->StillAdding = false;
  this->Prefix = "";
  return false;
}

bool vtkExodusIIReaderVectorCheck::Add( vtksys_stl::string name, const int* truth )
{
  if (
    ( ! this->StillAdding ) ||
    ( this->OriginalNames.size() >= this->Endings.size() ) ||
    ( ! this->CheckTruth( truth ) ) )
    {
    this->StillAdding = false;
    return false;
    }
  vtksys_stl::string::size_type len = name.size();
  if (
    ( len != this->Prefix.size() + 1 ) ||
    ( name.substr( 0, len - 1 ) != this->Prefix ) ||
    ( tolower( name[len - 1] ) != this->Endings[this->OriginalNames.size()] ) )
    {
    this->StillAdding = false;
    return false;
    }

  this->OriginalNames.push_back( name );
  return true;
}

std::vector<vtksys_stl::string>::size_type vtkExodusIIReaderVectorCheck::Length()
{
  std::vector<vtksys_stl::string>::size_type len = this->OriginalNames.size();
  return ( len == this->Endings.size() ) ? len : 0;
}

/*
rank 1:
dim 1( 1):   x
dim 2( 2):   x   y
dim 3( 3):   x   y   z
dim 4( 4):   x   y   z   w
1
1 1
1 1 1
1 1 1 1
rank 2:
dim 1( 1):  xx
dim 2( 3):  xx  yy  xy
dim 3( 6):  xx  yy  zz  xy  xz  yz
dim 4(10):  xx  yy  zz  ww  xy  xz  xw  yz  yw  zw
1
2 1
3 2 1
4 3 2 1
rank 3:
dim 1( 1): xxx
dim 2( 4): xxx yyy xxy xyy
dim 3(10): xxx yyy zzz xxy xxz xyy xyz xzz yyz yzz
dim 4(20): xxx yyy zzz www xxy xxz xxw xyy xyz xyw xzz xzw xww yyz yyw yzz yzw yww zzw zww
1
3 1
6 3 1
10 6 3 1
5!/3!/2 + 4!/2!/2 + 3!/1!/2 + 2!/0!/2 = 20
4!/2!/2 + 3!/1!/2 + 2!/0!/2 = 10
3!/1!/2 + 2!/0!/2 = 4
2!/0!/2 = 1

number of endings = nchoosek( rank + dim - 1, rank )
*/

vtkExodusIIReaderTensorCheck::vtkExodusIIReaderTensorCheck( const char* seq, int n, int rank, int dim )
{
  this->NumEndings = vtkMath::Binomial( rank + dim - 1, rank );
  if ( n == (int) this->NumEndings && rank > 0 && dim > 0 )
    {
    this->Dimension = dim;
    this->Rank = rank;
    this->Endings.insert( this->Endings.begin(), seq, seq + n * rank );
    this->Endings = vtksys::SystemTools::LowerCase( this->Endings );
    if ( this->Rank == 1 && this->Dimension == 2 )
      {
      this->GlomType = vtkExodusIIReaderPrivate::Vector2;
      }
    else if ( this->Rank == 1 && this->Dimension == 3 )
      {
      this->GlomType = vtkExodusIIReaderPrivate::Vector3;
      }
    else
      {
      this->GlomType = vtkExodusIIReaderPrivate::SymmetricTensor;
      }
    }
  else
    {
    vtkGenericWarningMacro(
      "Invalid number of tensor endings " << n << " for "
      "rank " << rank << " and dimension " << dim << "; expected "
      "vtkMath::Binomial( " << ( rank + dim - 1 ) << ", " << rank << ")"
      " = " << this->NumEndings );
    this->GlomType = -1;
    this->NumEndings = 0;
    }
}

bool vtkExodusIIReaderTensorCheck::StartInternal( vtksys_stl::string name, const int*, int )
{
  vtksys_stl::string::size_type len = name.size();
  if ( ( len > (unsigned) this->Rank ) &&
    vtksys::SystemTools::LowerCase( name.substr(len - this->Rank ) ) == this->Endings.substr( 0, this->Rank ) )
    {
    this->Prefix = name.substr( 0, len - this->Rank );
    this->OriginalNames.push_back( name );
    this->StillAdding = true;
    return true;
    }
  this->Prefix = "";
  this->StillAdding = false;
  return false;
}

bool vtkExodusIIReaderTensorCheck::Add( vtksys_stl::string name, const int* truth )
{
  if (
    ( ! this->StillAdding ) ||
    ( this->OriginalNames.size() >= this->NumEndings ) ||
    ( ! this->CheckTruth( truth ) ) )
    {
    this->StillAdding = false;
    return false;
    }
  vtksys_stl::string::size_type len = name.size();
  if (
    ( len != this->Prefix.size() + this->Rank ) ||
    ( name.substr( 0, len - this->Rank ) != this->Prefix ) )
    {
    this->StillAdding = false;
    return false;
    }
  vtksys_stl::string::size_type endingOffset = this->OriginalNames.size() * this->Rank;
  if ( vtksys::SystemTools::LowerCase( name.substr( len - this->Rank ) ) != this->Endings.substr( endingOffset, this->Rank )  )
    {
    this->StillAdding = false;
    return false;
    }

  this->OriginalNames.push_back( name );
  return true;
}

std::vector<vtksys_stl::string>::size_type vtkExodusIIReaderTensorCheck::Length()
{
  std::vector<vtksys_stl::string>::size_type len = this->OriginalNames.size();
  //std::vector<vtksys_stl::string>::size_type expected = this->Rank * this->Dimension;
  return ( len == this->NumEndings ) ? len : 0;
}

vtkExodusIIReaderIntPointCheck::vtkExodusIIReaderIntPointCheck()
  : RegExp( "(.*)_([^_]*)_GP([0-9,]+)$" )
{
  this->GlomType = vtkExodusIIReaderPrivate::IntegrationPoint;
}

bool vtkExodusIIReaderIntPointCheck::StartInternal( vtksys_stl::string name, const int*, int )
{
  if ( this->RegExp.find( name ) )
    {
    this->VarName = this->RegExp.match( 1 );
    this->CellType = this->RegExp.match( 2 );
    this->Prefix = this->VarName + "_" + this->CellType;
    // Can't have 3-D Gauss points on a quad (unless it's a shell) or 2-D Gauss points for a hex,
    // so verify that the integration domain has a rank appropriate to the cell type.
    // This also verifies that the cell type is valid and initializes IntPtMin, IntPtMax, and IntPtNames.
    if ( this->StartIntegrationPoints( this->CellType, this->RegExp.match( 3 ) ) )
      {
      this->OriginalNames.push_back( name );
      this->StillAdding = true;
      return true;
      }
    }
  this->Prefix = "";
  this->StillAdding = false;
  return false;
}

bool vtkExodusIIReaderIntPointCheck::Add( vtksys_stl::string name, const int* )
{
  if (
    ( ! this->StillAdding ) ||
    ( this->Rank == 0 ) )
    {
    this->StillAdding = false;
    return false;
    }
  vtksys_stl::string::size_type nlen = name.size();
  vtksys_stl::string::size_type plen = this->Prefix.size();
  if (
    ( nlen != plen + this->Rank + 3 /* for "_GP" */ ) ||
    ( name.substr( 0, plen ) != this->Prefix ) ||
    ( ! this->AddIntegrationPoint( name.substr( nlen - this->Rank ) ) ) )
    {
    this->StillAdding = false;
    return false;
    }

  this->OriginalNames.push_back( name );
  return true;
}

std::vector<vtksys_stl::string>::size_type vtkExodusIIReaderIntPointCheck::Length()
{
  if ( this->IntPtMin.size() != this->IntPtMax.size() )
    return 0;

  // Compute the size of the product space of the integration point indices.
  // FIXME: This assumes that integration points will be placed in a full-tensor
  //        product arrangement, which may not be true for triangular, tetrahedral
  //        wedge, or pyramidal elements depending on how they are parameterized.
  vtkTypeUInt64 numExpected = 1;
  for ( unsigned int i = 0; i < this->IntPtMax.size(); ++ i )
    {
    numExpected *= ( this->IntPtMax[i] - this->IntPtMin[i] + 1 );
    }
  if ( numExpected < 1 || numExpected != this->OriginalNames.size() )
    return 0;

  return this->OriginalNames.size();
}

/*
int vtkExodusIIReaderIntPointCheck::Accept(
std::vector<vtkExodusIIReaderPrivate::ArrayInfoType>& arr, int startIndex, vtkExodusIIReaderPrivate* priv, int objtyp )
{
}
*/

bool vtkExodusIIReaderIntPointCheck::StartIntegrationPoints(
  vtksys_stl::string cellType, vtksys_stl::string iptName )
{
  struct
    {
    const char* RE;
    int Rank;
    }
  cellTypes[] =
    {
      { "[Qq][Uu][Aa][Dd]", 2 },
      { "[Hh][Ee][Xx]", 3 },
      { "[Tt][Ee][Tt]", 3 },
      { "[Tt][Rr][Ii]", 2 },
      { "[Ww][Ee][Dd][Gg][Ee]", 3 },
      { "[Pp][Yy][Rr]", 3 }
    };
  vtksys::RegularExpression ctrexp;
  vtksys_stl::string::size_type expectedRank = static_cast<vtksys_stl::string::size_type>( -1 );
  for ( unsigned int i = 0; i < sizeof(cellTypes)/sizeof(cellTypes[0]); ++ i )
    {
    ctrexp.compile( cellTypes[i].RE );
    if ( ctrexp.find( cellType ) )
      {
      expectedRank = cellTypes[i].Rank;
      break;
      }
    }
  vtksys_stl::string::size_type rank = iptName.size();
  if ( expectedRank > 0 && rank != expectedRank )
    {
    this->Rank = 0;
    return false;
    }
  this->Rank = rank;
  this->IntPtMin.clear();
  this->IntPtMax.clear();
  for ( vtksys_stl::string::size_type i = 0; i < rank; ++ i )
    {
    int ival = iptName[i] - '0';
    if ( ival < 0 || ival > 9 )
      {
      this->Rank = 0;
      return false;
      }
    this->IntPtMin.push_back( ival );
    this->IntPtMax.push_back( ival );
    }
  this->IntPtNames.clear(); // clear out any old values
  this->IntPtNames.insert( iptName );
  return true;
}

bool vtkExodusIIReaderIntPointCheck::AddIntegrationPoint( vtksys_stl::string iptName )
{
  vtksys_stl::string::size_type rank = iptName.size();
  if ( rank != this->Rank )
    {
    this->Rank = 0;
    return false;
    }
  std::pair<std::set<vtksys_stl::string>::iterator,bool> result;
  result = this->IntPtNames.insert( iptName );
  if ( ! result.second )
    { // Oops, this integration point is a duplicate.
    this->Rank = 0;
    return false;
    }
  for ( vtksys_stl::string::size_type i = 0; i < rank; ++ i )
    {
    int ival = iptName[i] - '0';
    if ( ival < 0 || ival > 9 )
      {
      this->Rank = 0;
      return false;
      }
    if ( this->IntPtMin[i] > ival )
      this->IntPtMin[i] = ival;
    if ( this->IntPtMax[i] < ival )
      this->IntPtMax[i] = ival;
    }
  return true;
}
