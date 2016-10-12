/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExodusIIReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkExodusIIReader.h"
#include "vtkExodusIICache.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSortDataArray.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLParser.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVariantArray.h"
#include "vtkSmartPointer.h"
#include "vtkExodusIIReaderParser.h"

#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <string>
#include "vtksys/SystemTools.hxx"

#include "vtksys/RegularExpression.hxx"

#include "vtk_exodusII.h"
#include <cstdio>
#include <cstdlib> /* for free() */
#include <cstring> /* for memset() */
#include <cctype> /* for toupper(), isgraph() */
#include <cmath> /* for cos() */

#ifdef EXODUSII_HAVE_MALLOC_H
#  include <malloc.h>
#endif /* EXODUSII_HAVE_MALLOC_H */

/// Define this to get printouts summarizing array glomming process
#undef VTK_DBG_GLOM

#define VTK_EXO_FUNC(funcall,errmsg)\
  if ( (funcall) < 0 ) \
  { \
      vtkErrorMacro( errmsg ); \
      return 1; \
  }

// ------------------------------------------------------------------- CONSTANTS
static int obj_types[] = {
  EX_EDGE_BLOCK,
  EX_FACE_BLOCK,
  EX_ELEM_BLOCK,
  EX_NODE_SET,
  EX_EDGE_SET,
  EX_FACE_SET,
  EX_SIDE_SET,
  EX_ELEM_SET,
  EX_NODE_MAP,
  EX_EDGE_MAP,
  EX_FACE_MAP,
  EX_ELEM_MAP,
  EX_NODAL
};

static int num_obj_types = (int)(sizeof(obj_types)/sizeof(obj_types[0]));

static int obj_sizes[] = {
  EX_INQ_EDGE_BLK,
  EX_INQ_FACE_BLK,
  EX_INQ_ELEM_BLK,
  EX_INQ_NODE_SETS,
  EX_INQ_EDGE_SETS,
  EX_INQ_FACE_SETS,
  EX_INQ_SIDE_SETS,
  EX_INQ_ELEM_SETS,
  EX_INQ_NODE_MAP,
  EX_INQ_EDGE_MAP,
  EX_INQ_FACE_MAP,
  EX_INQ_ELEM_MAP,
  EX_INQ_NODES
};

static const char* objtype_names[] = {
  "Edge block",
  "Face block",
  "Element block",
  "Node set",
  "Edge set",
  "Face set",
  "Side set",
  "Element set",
  "Node map",
  "Edge map",
  "Face map",
  "Element map",
  "Nodal"
};

static const char* obj_typestr[] = {
  "L",
  "F",
  "E",
  "M",
  "D",
  "A",
  "S",
  "T",
  0, /* maps have no result variables */
  0,
  0,
  0,
  "N"
};

#define OBJTYPE_IS_BLOCK(i) (((i)>=0)&&((i)<3))
#define OBJTYPE_IS_SET(i) (((i)>2)&&((i)<8))
#define OBJTYPE_IS_MAP(i) (((i)>7)&&((i)<12))
#define OBJTYPE_IS_NODAL(i) ((i)==12)

// Unlike obj* items above:
// - conn* arrays only reference objects that generate connectivity information
// - conn* arrays are ordered the way users expect the output (*not* the same as above)
static int conn_types[] = {
  vtkExodusIIReader::ELEM_BLOCK_ELEM_CONN,
  vtkExodusIIReader::FACE_BLOCK_CONN,
  vtkExodusIIReader::EDGE_BLOCK_CONN,
  vtkExodusIIReader::ELEM_SET_CONN,
  vtkExodusIIReader::SIDE_SET_CONN,
  vtkExodusIIReader::FACE_SET_CONN,
  vtkExodusIIReader::EDGE_SET_CONN,
  vtkExodusIIReader::NODE_SET_CONN
};

static const char* conn_types_names[] = {
  "Element Blocks",
  "Face Blocks",
  "Edge Blocks",
  "Element Sets",
  "Side Sets",
  "Face Sets",
  "Edge Sets",
  "Node Sets"
};

static int num_conn_types = (int)(sizeof(conn_types)/sizeof(conn_types[0]));

// Given a conn_type index, what is its matching obj_type index?
static int conn_obj_idx_cvt[] = {
  2, 1, 0, 7, 6, 5, 4, 3
};

#define CONNTYPE_IS_BLOCK(i) (((i)>=0)&&((i)<3))
#define CONNTYPE_IS_SET(i) (((i)>2)&&((i)<8))

static const char* glomTypeNames[] = {
  "Scalar",
  "Vector2",
  "Vector3",
  "Symmetric Tensor",
  "Integration Point Values"
};

// used to store pointer to ex_get_node_num_map or ex_get_elem_num_map:
extern "C" { typedef int (*vtkExodusIIGetMapFunc)( int, int* ); }

// --------------------------------------------------- PRIVATE CLASS DECLARATION
#include "vtkExodusIIReaderPrivate.h"
#include "vtkExodusIIReaderVariableCheck.h"

// --------------------------------------------------- PRIVATE CLASS Implementations
vtkExodusIIReaderPrivate::BlockSetInfoType::BlockSetInfoType(
  const vtkExodusIIReaderPrivate::BlockSetInfoType &block):
  vtkExodusIIReaderPrivate::ObjectInfoType(block),
  FileOffset(block.FileOffset),
  PointMap(block.PointMap),
  ReversePointMap(block.ReversePointMap),
  CachedConnectivity(0)
{
  //this is needed to properly manage memory.
  //when vectors are resized or reserved the container
  //might be copied to a memory spot, so we need a proper copy constructor
  //so that the cache remains valid
  this->CachedConnectivity = block.CachedConnectivity;
}

vtkExodusIIReaderPrivate::BlockSetInfoType::~BlockSetInfoType()
{
  if (this->CachedConnectivity)
  {
    this->CachedConnectivity->Delete();
  }
}

vtkExodusIIReaderPrivate::BlockSetInfoType& vtkExodusIIReaderPrivate::BlockSetInfoType::operator=(const vtkExodusIIReaderPrivate::BlockSetInfoType& block)
{
  // protect against invalid self-assignment
  if (this != &block)
  {
    // superclass
    this->ObjectInfoType::operator=(static_cast<ObjectInfoType const&>(block));

    // delete existing
    if (this->CachedConnectivity)
    {
      this->CachedConnectivity->Delete();
      this->CachedConnectivity = NULL;
    }

    this->FileOffset = block.FileOffset;
    this->PointMap = block.PointMap;
    this->ReversePointMap = block.ReversePointMap;
    this->NextSqueezePoint = block.NextSqueezePoint;
    if (block.CachedConnectivity)
    {
      this->CachedConnectivity = vtkUnstructuredGrid::New();
      this->CachedConnectivity->ShallowCopy(block.CachedConnectivity);
    }
  }

  return *this;
}

// ----------------------------------------------------------- UTILITY ROUTINES

// This function exists because FORTRAN ordering sucks.
static void extractTruthForVar( int num_obj, int num_vars, const int* truth_tab, int var, std::vector<int>& truth )
{
  truth.clear();

  int obj;
  int ttObj; // truth table entry for variable var on object obj.
  for ( obj = 0; obj < num_obj; ++obj )
  {
    ttObj = truth_tab[ var + obj * num_vars ];
    truth.push_back( ttObj );
  }
}

static void printBlock( ostream& os, vtkIndent indent, int btyp,
                        vtkExodusIIReaderPrivate::BlockInfoType& binfo )
{
  int b = 0;
  while ( obj_types[b] >= 0 && obj_types[b] != btyp )
    ++b;
  const char* btypnam = objtype_names[b];
  os << indent << btypnam << " " << binfo.Id << " \"" << binfo.Name.c_str()
     << "\" (" << binfo.Size << ")\n";
  os << indent << "    FileOffset: " << binfo.FileOffset << "\n";
  os << indent << "    CachedConn: " << binfo.CachedConnectivity << " ("
     << binfo.Status << ")\n";
  os << indent << "    PointMap: " << binfo.PointMap.size() << " entries, "
     << "ReversePointMap: " << binfo.ReversePointMap.size() << " entries\n";
  os << indent << "    Type: " << binfo.TypeName.c_str() << "\n";
  os << indent << "    Bounds per entry, Node: " << binfo.BdsPerEntry[0]
     << " Edge: " << binfo.BdsPerEntry[1] << " Face: " << binfo.BdsPerEntry[2]
     << "\n";
  os << indent << "    Attributes (" << binfo.AttributesPerEntry << "):";
  int a;
  for ( a = 0; a < binfo.AttributesPerEntry; ++a )
  {
    os << " \"" << binfo.AttributeNames[a].c_str() << "\"("
       << binfo.AttributeStatus[a] << ")";
  }
  os << "\n";
}

static void printSet( ostream& os, vtkIndent indent, int styp,
                      vtkExodusIIReaderPrivate::SetInfoType& sinfo )
{
  int s = 0;
  while ( obj_types[s] >= 0 && obj_types[s] != styp )
    ++s;
  const char* stypnam = objtype_names[s];
  os << indent << stypnam << " " << sinfo.Id << " \"" << sinfo.Name.c_str()
     << "\" (" << sinfo.Size << ")\n";
  os << indent << "    FileOffset: " << sinfo.FileOffset << "\n";
  os << indent << "    CachedConn: " << sinfo.CachedConnectivity << " ("
     << sinfo.Status << ")\n";
  os << indent << "    PointMap: " << sinfo.PointMap.size() << " entries, "
     << "ReversePointMap: " << sinfo.ReversePointMap.size() << " entries\n";
  os << indent << "    DistFact: " << sinfo.DistFact << "\n";
}

static void printMap( ostream& os,
                      vtkIndent indent,
                      int mtyp,
                      vtkExodusIIReaderPrivate::MapInfoType& minfo )
{
  int m = 0;
  while ( obj_types[m] >= 0 && obj_types[m] != mtyp )
    ++m;
  const char* mtypnam = objtype_names[m];
  os << indent << mtypnam << " " << minfo.Id << " \"" << minfo.Name.c_str()
     << "\" (" << minfo.Size << ")\n";
  os << indent << "    Status: " << minfo.Status << "\n";
}

static void printArray( ostream& os, vtkIndent indent, int atyp,
                        vtkExodusIIReaderPrivate::ArrayInfoType& ainfo )
{
  (void)atyp;
  os << indent << "    " << ainfo.Name.c_str() << " [" << ainfo.Status << "] ( "
     << ainfo.Components << " = { ";
  os << ainfo.OriginalIndices[0] << " \"" << ainfo.OriginalNames[0] << "\"";
  int i;
  for ( i = 1; i < (int) ainfo.OriginalIndices.size(); ++i )
  {
    os << ", " << ainfo.OriginalIndices[i] << " \"" << ainfo.OriginalNames[i]
       << "\"";
  }
  os << " } )\n";
  os << indent << "    " << glomTypeNames[ ainfo.GlomType ] << " Truth:";
  for ( i = 0; i < (int)ainfo.ObjectTruth.size(); ++i )
  {
    os << " " << ainfo.ObjectTruth[i];
  }
  os << "\n";
}

// --------------------------------------------------- PRIVATE SUBCLASS MEMBERS
void vtkExodusIIReaderPrivate::ArrayInfoType::Reset()
{
  if ( ! this->Name.empty() )
  {
    this->Name.erase( this->Name.begin(), this->Name.end() );
  }
  this->Components = 0;
  this->GlomType = -1;
  this->Status = 0;
  this->Source = -1;
  this->OriginalNames.clear();
  this->OriginalIndices.clear();
  this->ObjectTruth.clear();
}

// ------------------------------------------------------- PRIVATE CLASS MEMBERS
vtkStandardNewMacro(vtkExodusIIReaderPrivate);

//-----------------------------------------------------------------------------
vtkExodusIIReaderPrivate::vtkExodusIIReaderPrivate()
{
  this->Exoid = -1;
  this->ExodusVersion = -1.;

  this->AppWordSize = 8;
  this->DiskWordSize = 8;

  this->Cache = vtkExodusIICache::New();
  this->CacheSize = 0;

  this->HasModeShapes = 0;
  this->ModeShapeTime = -1.;
  this->AnimateModeShapes = 1;

  this->GenerateObjectIdArray = 1;
  this->GenerateGlobalElementIdArray = 0;
  this->GenerateGlobalNodeIdArray = 0;
  this->GenerateImplicitElementIdArray = 0;
  this->GenerateImplicitNodeIdArray = 0;
  this->GenerateGlobalIdArray = 0;
  this->GenerateFileIdArray = 0;
  this->FileId = 0;
  this->ApplyDisplacements = 1;
  this->DisplacementMagnitude = 1.;

  this->SqueezePoints = 1;

  this->Parser = 0;

  this->SIL = vtkMutableDirectedGraph::New();
  this->SkipUpdateTimeInformation = false;

  memset( (void*)&this->ModelParameters, 0, sizeof(this->ModelParameters) );
}

//-----------------------------------------------------------------------------
vtkExodusIIReaderPrivate::~vtkExodusIIReaderPrivate()
{
  this->CloseFile();
  this->Cache->Delete();
  this->CacheSize = 0;
  this->ClearConnectivityCaches();
  if(this->Parser)
  {
    this->Parser->Delete();
    this->Parser = 0;
  }
  this->SIL->Delete();
  this->SIL = 0;
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::GlomArrayNames( int objtyp,
                                               int num_obj,
                                               int num_vars,
                                               char** var_names,
                                               int* truth_tab )
{
  // Clear out existing array names since we are re-reading them in.
  this->ArrayInfo[objtyp].clear();

  // Create some objects that try to glom names together in different ways.
  const char endRZ[] = "RZ";
  const char endV2[] = "xy";
  const char endV3[] = "xYz";
  const char endST23[] = "XXYYZZXYXZYZ";
  const char endST34[] = "XXXYYYZZZWWWXXYXXZXXWXYYXYZXYWXZZXZWXWWYYZYYWYZZYZWYWWZZWZWW";

  vtkExodusIIReaderScalarCheck* scalar = new vtkExodusIIReaderScalarCheck;
  //vtkExodusIIReaderVectorCheck* vecx2 = new vtkExodusIIReaderVectorCheck( endV2, 2 );
  //vtkExodusIIReaderVectorCheck* vecx3 = new vtkExodusIIReaderVectorCheck( endV3, 3 );
  //vtkExodusIIReaderVectorCheck* vecrz = new vtkExodusIIReaderVectorCheck( endRZ, 2 );
  vtkExodusIIReaderTensorCheck* vecx2 = new vtkExodusIIReaderTensorCheck( endV2, 2, 1, 2 );
  vtkExodusIIReaderTensorCheck* vecx3 = new vtkExodusIIReaderTensorCheck( endV3, 3, 1, 3 );
  vtkExodusIIReaderTensorCheck* vecrz = new vtkExodusIIReaderTensorCheck( endRZ, 2, 1, 2 );
  vtkExodusIIReaderTensorCheck* ten23 = new vtkExodusIIReaderTensorCheck( endST23, 6, 2, 3 );
  vtkExodusIIReaderTensorCheck* ten34 = new vtkExodusIIReaderTensorCheck( endST34, 20, 3, 4 );
  vtkExodusIIReaderIntPointCheck* intpt = new vtkExodusIIReaderIntPointCheck;
  typedef std::vector<vtkExodusIIReaderVariableCheck*> glomVec;
  glomVec glommers;
  glommers.push_back( scalar );
  glommers.push_back( vecx2 );
  glommers.push_back( vecx3 );
  glommers.push_back( vecrz );
  glommers.push_back( ten23 );
  glommers.push_back( ten34 );
  glommers.push_back( intpt );
  glomVec::iterator glommer;
  std::vector<int> tmpTruth;
  // Advance through the variable names.
  for ( int i = 0; i < num_vars; ++ i )
  {
    // Prepare all the glommers with the next unused variable name
    extractTruthForVar( num_obj, num_vars, truth_tab, i, tmpTruth );
    bool stop = true;
    for ( glommer = glommers.begin(); glommer != glommers.end(); ++ glommer )
    {
      if ( (*glommer)->Start( var_names[i], &tmpTruth[0], num_obj ) )
      {
        stop = false;
      }
    }
    int j = i + 1;
    // If any glommers can continue accepting names, give them more names until no more can accept names
    while ( j < num_vars && ! stop )
    {
      stop = true;
      for ( glommer = glommers.begin(); glommer != glommers.end(); ++ glommer )
      {
        if ( (*glommer)->Add( var_names[j], &tmpTruth[0] ) )
        {
          stop = false;
        }
      }
      ++ j;
    }
    // Find longest glom that worked. (The scalar glommer always works with Length() 1.)
    unsigned int longestGlom = 0;
    glomVec::iterator longestGlommer = glommers.end();
    for ( glommer = glommers.begin(); glommer != glommers.end(); ++ glommer )
    {
      if ( (*glommer)->Length() > longestGlom )
      {
        longestGlom = static_cast<unsigned int>( (*glommer)->Length() );
        longestGlommer = glommer;
      }
    }
    if ( longestGlommer != glommers.end() )
    {
      i += (*longestGlommer)->Accept( this->ArrayInfo[objtyp], i, this, objtyp ) - 1; // the ++i takes care of length 1
    }
  }

  // Now see what the gloms were.
  /*
  typedef std::vector<vtkExodusIIReaderPrivate::ArrayInfoType> varVec;
  for ( varVec::iterator it = this->ArrayInfo[objtyp].begin(); it != this->ArrayInfo[objtyp].end(); ++ it )
    {
    cout << "Name: \"" << it->Name.c_str() << "\" (" << it->Components << ")\n";
    }
    */

  delete scalar;
  delete vecx2;
  delete vecx3;
  delete vecrz;
  delete ten23;
  delete ten34;
  delete intpt;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::AssembleOutputConnectivity(
  vtkIdType timeStep, int otyp, int oidx, int conntypidx,
  BlockSetInfoType* bsinfop, vtkUnstructuredGrid* output )
{
  // FIXME: Don't think I need this, since we ShallowCopy over it... right?
  output->Reset();
  if ( bsinfop->CachedConnectivity )
  {
    output->ShallowCopy( bsinfop->CachedConnectivity );
    return 1;
  }

  // OK, we needed to remake the cache...
  bsinfop->CachedConnectivity = vtkUnstructuredGrid::New();
  bsinfop->CachedConnectivity->Allocate( bsinfop->Size );
  if ( this->SqueezePoints )
  {
    bsinfop->NextSqueezePoint = 0;
    bsinfop->PointMap.clear();
    bsinfop->ReversePointMap.clear();
  }

  // Need to assemble connectivity array from smaller ones.
  // Call GetCacheOrRead() for each smaller array

  // Might want to experiment with the effectiveness of caching connectivity...
  //   set up the ExodusIICache class with the ability to never cache some
  //   key types.
  // Might also want to experiment with policies other than LRU, especially
  //   applied to arrays that are not time-varying. During animations, they
  //   will most likely get dropped even though that might not be wise.

  if ( CONNTYPE_IS_BLOCK(conntypidx) )
  {
    this->InsertBlockCells( otyp, oidx, conn_types[conntypidx],
      timeStep, static_cast<BlockInfoType*>( bsinfop ) );
  }
  else if ( CONNTYPE_IS_SET(conntypidx) )
  {
    this->InsertSetCells( otyp, oidx, conn_types[conntypidx],
      timeStep, static_cast<SetInfoType*>( bsinfop ) );
  }
  else
  {
    vtkErrorMacro( "Bad connectivity object type. Harass the responsible programmer." );
  }

  // OK, now copy our cache to the output...
  output->ShallowCopy( bsinfop->CachedConnectivity );
  //this->CachedConnectivity->ShallowCopy( output );
  if ( this->SqueezePoints )
  {
    vtkDebugMacro( << "Squeezed down to " << bsinfop->NextSqueezePoint << " points\n" );
  }
  return 0;
}

int vtkExodusIIReaderPrivate::AssembleOutputPoints(
  vtkIdType timeStep, BlockSetInfoType* bsinfop, vtkUnstructuredGrid* output )
{
  (void)timeStep;
  vtkPoints* pts = output->GetPoints();
  if ( ! pts )
  {
    pts = vtkPoints::New();
    output->SetPoints( pts );
    pts->FastDelete();
  }
  else
  {
    pts->Reset();
  }

  int ts = -1; // If we don't have displacements, only cache the array under one key.
  if ( this->ApplyDisplacements && this->FindDisplacementVectors( timeStep ) )
  { // Otherwise, each time step's array will be different.
    ts = timeStep;
  }

  vtkDataArray* arr = this->GetCacheOrRead( vtkExodusIICacheKey( ts, vtkExodusIIReader::NODAL_COORDS, 0, 0 ) );
  if ( ! arr )
  {
    vtkErrorMacro( "Unable to read points from file." );
    return 0;
  }

  if ( this->SqueezePoints )
  {
    pts->SetNumberOfPoints( bsinfop->NextSqueezePoint );
    std::map<vtkIdType,vtkIdType>::iterator it;
    for ( it = bsinfop->PointMap.begin(); it != bsinfop->PointMap.end(); ++ it )
    {
      pts->SetPoint( it->second, arr->GetTuple( it->first ) );
    }
  }
  else
  {
    pts->SetData( arr );
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::AssembleOutputPointArrays(
  vtkIdType timeStep, BlockSetInfoType* bsinfop, vtkUnstructuredGrid* output )
{
  int status = 1;
  std::vector<ArrayInfoType>::iterator ai;
  int aidx = 0;

  for (
    ai = this->ArrayInfo[ vtkExodusIIReader::NODAL ].begin();
    ai != this->ArrayInfo[ vtkExodusIIReader::NODAL ].end();
    ++ai, ++aidx )
  {
    if ( ! ai->Status )
      continue; // Skip arrays we don't want.

    vtkExodusIICacheKey key( timeStep, vtkExodusIIReader::NODAL, 0, aidx );
    vtkDataArray* src = this->GetCacheOrRead( key );
    if ( !src )
    {
      vtkDebugMacro( "Unable to read point array " << ai->Name.c_str() << " at time step " << timeStep );
      status = 0;
      continue;
    }

    this->AddPointArray( src, bsinfop, output );
  }
  return status;
}

//-----------------------------------------------------------------------------
#if 0
// Copy tuples from one array to another, possibly with a different number of components per tuple.
static void vtkEmbedTuplesInLargerArray(
  vtkDataSetAttributes* attr, vtkDataArray* dst, vtkDataArray* src, vtkIdType numTuples, vtkIdType offset )
{
  vtkIdType i;
  int srcNumComp = src->GetNumberOfComponents();
  int dstNumComp = dst->GetNumberOfComponents();
  if ( dstNumComp != srcNumComp )
  { // We've promoted the array from 2-D to 3-D... can't use CopyTuple
    if ( dst->GetDataType() != src->GetDataType() )
    {
      return;
    }
    vtkIdType sid = 0;
    vtkIdType did = offset * dstNumComp;
    int minNumComp = dstNumComp < srcNumComp ? dstNumComp : srcNumComp;
    switch( dst->GetDataType() )
    {
      vtkTemplateMacro(
      {
        VTK_TT* srcTuple = (VTK_TT*) src->GetVoidPointer( sid );
        VTK_TT* dstTuple = (VTK_TT*) dst->GetVoidPointer( did );
        for ( i = 0; i < numTuples; ++i, srcTuple += srcNumComp, dstTuple += dstNumComp )
        {
          for ( int j = 0; j < minNumComp; ++j )
          {
            dstTuple[j] = srcTuple[j];
          }
        }
      }
      );
    }
  }
  else
  {
    for ( i = 0; i < numTuples; ++i )
    {
      attr->CopyTuple( src, dst, i, i + offset );
    }
  }
}
#endif // 0

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::AssembleOutputCellArrays(
  vtkIdType timeStep, int otyp, int obj, BlockSetInfoType* bsinfop,
  vtkUnstructuredGrid* output )
{
  // Don't create arrays for deselected objects
  if ( !output || ! bsinfop->Status )
  {
    return 1;
  }

  vtkCellData* cd = output->GetCellData();
  // Load (time-constant) attributes first because their status is in the block info.
  if (
    otyp == vtkExodusIIReader::ELEM_BLOCK ||
    otyp == vtkExodusIIReader::EDGE_BLOCK ||
    otyp == vtkExodusIIReader::FACE_BLOCK)
  {
    BlockInfoType* binfop = (BlockInfoType*)bsinfop;
    std::vector<int>::iterator atit;
    vtkIdType a = 0;
    for (atit = binfop->AttributeStatus.begin(); atit != binfop->AttributeStatus.end(); ++atit, ++a)
    {
      if (*atit)
      {
        vtkDataArray* arr = this->GetCacheOrRead(
          vtkExodusIICacheKey( timeStep, vtkExodusIIReader::ELEM_BLOCK_ATTRIB, obj, a ) );
        if ( arr )
        {
          cd->AddArray( arr );
        }
      }
    }
  }

  // Panic if we're given a bad otyp.
  std::map<int,std::vector<ArrayInfoType> >::iterator ami = this->ArrayInfo.find( otyp );
  if ( ami == this->ArrayInfo.end() )
  {
#if 0
    vtkErrorMacro( "Unknown block or set type \"" << otyp << "\" encountered." );
    for ( ami = this->ArrayInfo.begin(); ami != this->ArrayInfo.end(); ++ ami )
    {
      cerr << "   Have type: \"" << ami->first << "\"\n";
    }
    return 0;
#else
    return 1;
#endif // 0
  }

  // For each array defined on objects of the same type as our output,
  // look for ones that are turned on (Status != 0) and have a truth
  // table indicating values are present for object obj in the file.
  std::vector<ArrayInfoType>::iterator ai;
  int aidx = 0;
  for ( ai = ami->second.begin(); ai != ami->second.end(); ++ai, ++aidx )
  {
    if ( ! ai->Status )
      continue;

    if ( ! ai->ObjectTruth[obj] )
      continue;

    vtkDataArray* arr = this->GetCacheOrRead( vtkExodusIICacheKey( timeStep, ami->first, obj, aidx ) );
    if ( arr )
    {
      cd->AddArray( arr );
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::AssembleOutputProceduralArrays(
  vtkIdType timeStep, int otyp, int obj,
  vtkUnstructuredGrid* output )
{
  (void)timeStep;
  int status = 7;
  vtkCellData* cd = output->GetCellData();
  if ( this->GenerateObjectIdArray )
  {
    vtkExodusIICacheKey key( -1, vtkExodusIIReader::OBJECT_ID, otyp, obj );
    vtkDataArray* arr = this->GetCacheOrRead( key );
    if ( arr )
    {
      cd->AddArray( arr );
      status -= 1;
    }
  }

  if ( this->GenerateGlobalElementIdArray &&
       ( otyp == vtkExodusIIReader::SIDE_SET_CONN ||
         otyp == vtkExodusIIReader::SIDE_SET ) )
  {
    vtkExodusIICacheKey ckey( -1, vtkExodusIIReader::ELEMENT_ID, 0, 0 );
    vtkIdTypeArray* src = 0;

    if ( vtkDataArray* elems = this->GetCacheOrRead( ckey ) )
    {
      src = vtkIdTypeArray::New ();
      src->DeepCopy  (elems);
    }

    vtkExodusIICacheKey key( -1, vtkExodusIIReader::SIDE_SET_CONN, obj, 1 );
    if ( vtkDataArray* arr = this->GetCacheOrRead( key ) )
    {

      vtkIdTypeArray* idarray = vtkArrayDownCast<vtkIdTypeArray>(arr);
      vtkIdTypeArray* elementid = vtkIdTypeArray::New();
      elementid->SetNumberOfTuples(idarray->GetNumberOfTuples());
      elementid->SetName(vtkExodusIIReader::GetSideSetSourceElementIdArrayName());
      vtkIntArray* elementside = vtkIntArray::New();
      elementside->SetNumberOfTuples(idarray->GetNumberOfTuples());
      elementside->SetName(vtkExodusIIReader::GetSideSetSourceElementSideArrayName());
      vtkIdType values[2];

      for(vtkIdType i=0;i<idarray->GetNumberOfTuples();i++)
      {
        idarray->GetTypedTuple(i, values);
        if (src == 0 || src->GetValue (values[0] - 1) <= 0)
        {
          elementid->SetValue(i, values[0] - 1);
        }
        else
        {
          elementid->SetValue(i, src->GetValue (values[0] - 1) - 1); // find the global element id
        }
        // now we have to worry about mapping from exodus canonical side
        // ordering to vtk canonical side ordering for wedges and hexes.
        // Even if the element block isn't loaded that we still know what
        // types of cells it would have contained since all elements
        // in a block are of the same type.
        BlockInfoType* type =
          this->GetBlockFromFileGlobalId(vtkExodusIIReader::ELEM_BLOCK, values[0]);
        switch(type->CellType)
        {
          case VTK_WEDGE:
          {
            int wedgeMapping[5] = {2, 3, 4, 0, 1};
            elementside->SetValue(i, wedgeMapping[ values[1]-1 ] );
            break;
          }
          case VTK_HEXAHEDRON:
          {
            int hexMapping[6] = {2, 1, 3, 0, 4, 5};
            elementside->SetValue(i,  hexMapping[ values[1]-1 ] );
            break;
          }
          default:
          { // switch to 0-based indexing
            elementside->SetValue(i, values[1]-1 );
          }
        }
      }
      cd->AddArray( elementid );
      cd->AddArray( elementside );
      elementid->FastDelete();
      elementside->FastDelete();
      status -= 2;
    }

    if (src != 0)
    {
      src->Delete ();
    }
  }

  if ( this->GenerateGlobalElementIdArray && ! OBJTYPE_IS_SET( otyp ) )
  {
    // This retrieves the first new-style map, or if that is not present,
    // the solitary old-style map (which always exists but may be
    // procedurally generated if it is not stored with the file).
    vtkExodusIICacheKey key( -1, vtkExodusIIReader::GLOBAL_ELEMENT_ID, otyp, obj );
    vtkDataArray* arr = this->GetCacheOrRead( key );
    if ( arr )
    {
      vtkDataArray* ped = vtkIdTypeArray::New();
      ped->DeepCopy( arr );
      ped->SetName( vtkExodusIIReader::GetPedigreeElementIdArrayName() );

      cd->SetGlobalIds( arr );
      cd->SetPedigreeIds( ped );
      ped->FastDelete();

      status -= 2;
    }
  }

  if ( this->GenerateGlobalNodeIdArray )
  {
    // This retrieves the first new-style map, or if that is not present,
    // the solitary old-style map (which always exists but may be
    // procedurally generated if it is not stored with the file).
    vtkExodusIICacheKey key( -1, vtkExodusIIReader::GLOBAL_NODE_ID, otyp, obj );
    vtkDataArray* arr = this->GetCacheOrRead( key );
    vtkPointData* pd = output->GetPointData();
    if ( arr )
    {
      vtkDataArray* ped = vtkIdTypeArray::New();
      ped->DeepCopy( arr );
      ped->SetName( vtkExodusIIReader::GetPedigreeNodeIdArrayName() );

      pd->SetGlobalIds( arr );
      pd->SetPedigreeIds( ped );
      ped->FastDelete();

      status -= 4;
    }
  }

  if ( this->GenerateImplicitElementIdArray )
  {
    // This retrieves the old style map if it is a parallel data set.  The old
    // style map stores the global implicit id if parallel.  Otherwise it
    // generates the implicit id.
    vtkExodusIICacheKey key( -1, vtkExodusIIReader::IMPLICIT_ELEMENT_ID, otyp, obj );
    vtkDataArray* arr = this->GetCacheOrRead( key );
    if ( arr )
    {
      cd->AddArray( arr );
    }

  }

  if ( this->GenerateImplicitNodeIdArray )
  {
    // This retrieves the old style map if it is a parallel data set.  The old
    // style map stores the global implicit id if parallel.  Otherwise it
    // generates the implicit id.
    vtkExodusIICacheKey key( -1, vtkExodusIIReader::IMPLICIT_NODE_ID, otyp, obj );
    vtkDataArray* arr = this->GetCacheOrRead( key );
    vtkPointData* pd = output->GetPointData();
    if ( arr )
    {
      pd->AddArray( arr );
    }
  }

  if ( this->GenerateFileIdArray )
  { // Don't cache this... it's not worth it.
    vtkIdType numCells = output->GetNumberOfCells();
    vtkIntArray* iarr = vtkIntArray::New();
    iarr->SetNumberOfComponents( 1 );
    iarr->SetNumberOfTuples( numCells );
    iarr->SetName( this->GetFileIdArrayName() );
    cd->AddArray( iarr );
    iarr->FastDelete();
    for ( vtkIdType i = 0; i < numCells; ++ i )
    {
      iarr->SetValue( i, this->FileId );
    }
  }

  return status;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::AssembleOutputGlobalArrays(
  vtkIdType timeStep, int otyp, int obj, BlockSetInfoType* bsinfop,
  vtkUnstructuredGrid* output )
{
  (void)obj;
  vtkFieldData *ofieldData = output->GetFieldData();

  int status = 1;
  std::vector<ArrayInfoType>::iterator ai;
  int aidx = 0;

  for (
    ai = this->ArrayInfo[ vtkExodusIIReader::GLOBAL ].begin();
    ai != this->ArrayInfo[ vtkExodusIIReader::GLOBAL ].end();
    ++ai, ++aidx )
  {
    if ( ! ai->Status )
    {
      continue;
    }

    // Add time-varying global data
    vtkExodusIICacheKey tdKey( -1, vtkExodusIIReader::GLOBAL_TEMPORAL, -1, aidx );
    vtkDataArray* temporalData = this->GetCacheOrRead( tdKey );
    if ( ! temporalData )
    {
      vtkDebugMacro( "Unable to read array " << ai->Name.c_str() );
      status = 0;
      continue;
    }

    ofieldData->AddArray(temporalData);
  }

  // Add block id information for the exodus writer (if we're an element block)
  if ( otyp == vtkExodusIIReader::ELEM_BLOCK )
  {
    vtkIntArray *elemBlockIdArray = vtkIntArray::New();
    elemBlockIdArray->SetNumberOfComponents( 1 );
    elemBlockIdArray->SetNumberOfValues( 1 ); // one elem block per unstructured grid
    elemBlockIdArray->SetName( "ElementBlockIds" );
    elemBlockIdArray->SetValue( 0, bsinfop->Id );
    ofieldData->AddArray( elemBlockIdArray );
    elemBlockIdArray->Delete();
  }

  // Add QA record, title, and INFO record metadata from the Exodus II file
  vtkExodusIICacheKey qakey( -1, vtkExodusIIReader::QA_RECORDS, 0, 0 );
  vtkDataArray* arr = this->GetCacheOrRead( qakey );
  if ( arr )
  {
    ofieldData->AddArray(arr);
  }

  // Add the title
  {
    vtkStringArray* sarr = vtkStringArray::New();
    sarr->SetName( "Title" );
    sarr->SetNumberOfComponents( 1 );
    sarr->SetNumberOfTuples(1);
    sarr->SetValue(0, this->ModelParameters.title);
    ofieldData->AddArray(sarr);
    sarr->Delete();
  }

  // Add mode_shape/time_step
  if (this->HasModeShapes)
  {
    vtkNew<vtkIntArray> dataIndexArray;
    dataIndexArray->SetName("mode_shape");
    dataIndexArray->SetNumberOfComponents(1);
    dataIndexArray->SetNumberOfTuples(1);
    // mode-shape == (timestep + 1). See vtkExodusIIReader::SetModeShape().
    dataIndexArray->SetValue(0, (timeStep + 1));
    ofieldData->AddArray(dataIndexArray.GetPointer());

    vtkNew<vtkIntArray> modeShapeRange;
    modeShapeRange->SetName("mode_shape_range");
    modeShapeRange->SetNumberOfComponents(2);
    modeShapeRange->SetNumberOfTuples(1);
    modeShapeRange->SetValue(0, this->Parent->GetModeShapesRange()[0]);
    modeShapeRange->SetValue(1, this->Parent->GetModeShapesRange()[1]);
    ofieldData->AddArray(modeShapeRange.GetPointer());
  }

  vtkExodusIICacheKey infokey( -1, vtkExodusIIReader::INFO_RECORDS, 0, 0 );
  arr = this->GetCacheOrRead( infokey );
  if ( arr )
  {
    ofieldData->AddArray(arr);
  }

  return status;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::AssembleOutputPointMaps( vtkIdType timeStep,
  BlockSetInfoType* bsinfop, vtkUnstructuredGrid* output )
{
  (void)timeStep;
  int status = 1;
  std::vector<MapInfoType>::iterator mi;
  int midx = 0;

  for (
    mi = this->MapInfo[ vtkExodusIIReader::NODE_MAP ].begin();
    mi != this->MapInfo[ vtkExodusIIReader::NODE_MAP ].end();
    ++mi, ++midx )
  {
    if ( ! mi->Status )
      continue; // Skip arrays we don't want.

    vtkIdTypeArray* src = vtkArrayDownCast<vtkIdTypeArray>( this->GetCacheOrRead(
        vtkExodusIICacheKey( -1, vtkExodusIIReader::NODE_MAP, 0, midx ) ) );
    if ( !src )
    {
      vtkDebugMacro(
        "Unable to read point map array \""
        << mi->Name.c_str() << "\" (" << midx << ")" );
      status = 0;
      continue;
    }

    this->AddPointArray( src, bsinfop, output );
  }
  return status;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::AssembleOutputCellMaps(
  vtkIdType vtkNotUsed(timeStep), int otyp, int obj, BlockSetInfoType* bsinfop,
  vtkUnstructuredGrid* output )
{
  (void)obj;
  // Don't create arrays for deselected objects
  if ( ! output || ! bsinfop->Status )
  {
    return 1;
  }

  // Ignore invalid otyp values (sets cannot have maps, only blocks).
  int mtyp = this->GetMapTypeFromObjectType( otyp );
  std::map<int,std::vector<MapInfoType> >::iterator mmi =
    this->MapInfo.find( mtyp );
  if ( mmi == this->MapInfo.end() )
  {
    return 1;
  }

  vtkCellData* cd = output->GetCellData();
  // For each map defined on objects of the same type as our output,
  // look for ones that are turned on (Status != 0).
  std::vector<MapInfoType>::iterator mi;
  int midx = 0;
  for ( mi = mmi->second.begin(); mi != mmi->second.end(); ++mi, ++midx )
  {
    if ( ! mi->Status )
      continue;

    vtkDataArray* src = this->GetCacheOrRead(
      vtkExodusIICacheKey( -1, mmi->first, 0, midx ) );
    if ( ! src )
      continue;

    if ( otyp == vtkExodusIIReader::ELEM_BLOCK )
    {
      if (
        bsinfop->Size == src->GetNumberOfTuples() &&
        bsinfop->FileOffset == 1 &&
        this->BlockInfo[otyp].size() == 1 )
      {
        cd->AddArray( src );
      }
      else
      {
        // Create the array and copy the applicable subset from the map
        vtkIdTypeArray* arr = vtkIdTypeArray::New();
        arr->SetName( mi->Name.c_str() );
        arr->SetNumberOfComponents( 1 );
        arr->SetNumberOfTuples( bsinfop->Size );
        memcpy( arr->GetVoidPointer(0), src->GetVoidPointer( bsinfop->FileOffset - 1 ),
          bsinfop->Size * sizeof(vtkIdType) );
        cd->AddArray( arr );
        arr->FastDelete();
      }
    }
    else
    {
      // FIXME: We have a set (no maps are defined on sets but we could determine
      //        map values given the set generators) or an edge/face block (unclear
      //        whether maps are useful/possible on these block types).
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::InsertBlockPolyhedra(
  BlockInfoType* binfo,
  vtkIntArray* facesPerCell,
  vtkIntArray* pointsPerFace,
  vtkIntArray* exoCellConn,
  vtkIntArray* exoFaceConn)
{
  vtkIdType numCells = facesPerCell->GetMaxId() + 1;
  vtkIdType numFaces = pointsPerFace->GetMaxId() + 1;

  // The Exodus file format is more compact than VTK's; it
  // allows multiple elements(cells) to refer to the same face so
  // that no face->point connectivity needs to be repeated.
  // VTK's polyhedral cells unpacks each element's faces
  // into a contiguous list for fast access to each element's
  // face->point connectivity.
  // So, we cannot use the arrays we are given as-is.
  // Also, VTK requires a list, without duplicates, of all the
  // points per cell (across all its faces), which Exodus does
  // not provide.

  // I. Break out face connectivity,
  //    squeezing points along the way if needed.
  std::vector<std::vector<vtkIdType> > facePointLists;
  facePointLists.resize(numFaces);
  vtkIdType curFacePoint = 0;
  for (vtkIdType i = 0; i < numFaces; ++i)
  {
    std::vector<vtkIdType>& curPtList(facePointLists[i]);
    for (vtkIdType j = 0; j < pointsPerFace->GetValue(i); ++j)
    {
      vtkIdType ptId = this->SqueezePoints ?
        this->GetSqueezePointId(binfo, exoFaceConn->GetValue(curFacePoint++)) :
        exoFaceConn->GetValue(curFacePoint++);
      curPtList.push_back(ptId);
    }
  }

  // II. Insert cells using face-point connectivity.
  vtkIdType curCell = 0;
  vtkIdType curCellCurFace = 0;
  for (vtkIdType i = 0; i < numCells; ++i)
  {
    std::vector<vtkIdType> vtkCellPts;
    int numFacesThisCell = facesPerCell->GetValue(curCell++);
    for (vtkIdType j = 0; j < numFacesThisCell; ++j)
    {
      vtkIdType curFace = exoCellConn->GetValue(curCellCurFace++);
      std::vector<vtkIdType>& curFacePts(facePointLists[curFace]);
      vtkCellPts.push_back(static_cast<vtkIdType>(curFacePts.size()));
      vtkCellPts.insert(vtkCellPts.end(), curFacePts.begin(), curFacePts.end());
    }
    binfo->CachedConnectivity->InsertNextCell(
      VTK_POLYHEDRON, numFacesThisCell, &vtkCellPts[0]);
  }
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::InsertBlockCells(
  int otyp, int obj, int conn_type, int timeStep, BlockInfoType* binfo )
{
  (void)timeStep;
  (void)otyp;
  if ( binfo->Size == 0 )
  {
    // No entries in this block.
    // This happens in parallel filesets when all
    // elements are distributed to other files.
    // Silently ignore.
    return;
  }

  vtkIntArray* ent = 0;
  if ( binfo->PointsPerCell == 0 )
  {
    int arrId = (conn_type == vtkExodusIIReader::ELEM_BLOCK_ELEM_CONN ? 0 : 1);
    ent = vtkArrayDownCast<vtkIntArray>(
      this->GetCacheOrRead(
        vtkExodusIICacheKey( -1, vtkExodusIIReader::ENTITY_COUNTS, obj, arrId
          )));
    if ( ! ent )
    {
      vtkErrorMacro(
        "Entity used 0 points per cell, "
        "but didn't return polyhedra correctly" );
      binfo->Status = 0;
      return;
    }
    ent->Register(this);
  }

  // Handle 3-D polyhedra (not 2-D polygons) separately
  // from other cell types for simplicity.
  // In addition to the element block connectivity (which
  // lists faces bounding the polyhedra, we must load face
  // block connectivity (which lists corner nodes for each
  // face).
  if (binfo->CellType == VTK_POLYHEDRON)
  {
    vtkIntArray* efconn;
    efconn = vtkArrayDownCast<vtkIntArray>(
      this->GetCacheOrRead(
        vtkExodusIICacheKey( -1, vtkExodusIIReader::ELEM_BLOCK_FACE_CONN, obj, 0 )));
    if (efconn)
      efconn->Register(this);
    vtkIntArray* fconn;
    fconn = vtkArrayDownCast<vtkIntArray>(
      this->GetCacheOrRead(
        vtkExodusIICacheKey( -1, vtkExodusIIReader::FACE_BLOCK_CONN, obj, 0 )));
    if (fconn)
      fconn->Register(this);
    vtkIntArray* ptsPerFace = NULL;
    ptsPerFace = vtkArrayDownCast<vtkIntArray>(
      this->GetCacheOrRead(
        vtkExodusIICacheKey( -1, vtkExodusIIReader::ENTITY_COUNTS, obj, 1
          )));
    if (!efconn || !fconn || !ent || !ptsPerFace)
    {
      vtkWarningMacro(
        << "Element (" << efconn << ") and face (" << fconn << ") block, "
        << "plus number of faces per poly (" << ent << ") and "
        << "number of points per face (" << ptsPerFace << ") are all required. "
        << "Skipping block id " << binfo->Id << "; expect trouble." );
      binfo->Status = 0;
      if (ent) ent->UnRegister(this);
      if (fconn) fconn->UnRegister(this);
      if (efconn) efconn->UnRegister(this);
      return;
    }
    ptsPerFace->Register(this); // For sanity, own this for a moment
    this->InsertBlockPolyhedra(binfo, ent, ptsPerFace, efconn, fconn);
    ptsPerFace->UnRegister(this); // OK, we're done.
    efconn->UnRegister(this);
    fconn->UnRegister(this);
    ent->UnRegister(this);
    return;
  }

  vtkIntArray* arr;
  arr = vtkArrayDownCast<vtkIntArray>(
    this->GetCacheOrRead(
      vtkExodusIICacheKey( -1, conn_type, obj, 0 )));
  if ( ! arr )
  {
    vtkWarningMacro(
      "Block wasn't present in file? Working around it. Expect trouble." );
    binfo->Status = 0;
    if (ent)
    {
      ent->UnRegister(this);
    }
    return;
  }

  if (this->SqueezePoints)
  {
    std::vector<vtkIdType> cellIds;
    cellIds.resize( binfo->PointsPerCell );
    int* srcIds = arr->GetPointer( 0 );

    for ( int i = 0; i < binfo->Size; ++i )
    {
      int entitiesPerCell;
      if (ent)
      {
        entitiesPerCell = ent->GetValue (i);
        cellIds.resize( entitiesPerCell );
      }
      else
      {
        entitiesPerCell = binfo->PointsPerCell;
      }

      for ( int p = 0; p < entitiesPerCell; ++p )
      {
        cellIds[p] = this->GetSqueezePointId( binfo, srcIds[p] );
        //cout << " " << srcIds[p] << "(" << cellIds[p] << ")";
      }
      //cout << "\n";
      //cout << " " <<
      binfo->CachedConnectivity->InsertNextCell( binfo->CellType, entitiesPerCell, &cellIds[0] );
      srcIds += entitiesPerCell;
    }
      //cout << "\n";
  }
  else
  {
#ifdef VTK_USE_64BIT_IDS
    std::vector<vtkIdType> cellIds;
    cellIds.resize( binfo->PointsPerCell );
    int* srcIds = arr->GetPointer( 0 );

    for ( int i = 0; i < binfo->Size; ++i )
    {
      int entitiesPerCell = binfo->PointsPerCell;
      if ( ent != 0)
      {
        entitiesPerCell = ent->GetValue (i);
        cellIds.resize( entitiesPerCell );
      }
      for ( int p = 0; p < entitiesPerCell; ++p )
      {
        cellIds[p] = srcIds[p];
        //cout << " " << srcIds[p];
      }
      //cout << "\n";
      binfo->CachedConnectivity->InsertNextCell( binfo->CellType, entitiesPerCell, &cellIds[0] );
      srcIds += entitiesPerCell;
    }
#else // VTK_USE_64BIT_IDS
    vtkIdType* srcIds = (vtkIdType*) arr->GetPointer( 0 );

    for ( int i = 0; i < binfo->Size; ++i )
    {
      int entitiesPerCell = binfo->PointsPerCell;
      if ( ent != 0)
      {
        entitiesPerCell = ent->GetValue (i);
      }
      binfo->CachedConnectivity->InsertNextCell( binfo->CellType, entitiesPerCell, srcIds );
      srcIds += entitiesPerCell;
      //for ( int k = 0; k < binfo->PointsPerCell; ++k )
        //cout << " " << srcIds[k];
      //cout << "\n";
    }
#endif // VTK_USE_64BIT_IDS
  }
  if (ent)
  {
    ent->UnRegister (this);
  }
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::InsertSetCells(
  int otyp, int obj, int conn_type, int timeStep,
  SetInfoType* sinfo )
{
  (void)timeStep;
  if ( sinfo->Size == 0 )
  {
    // No entries in this set.
    // This happens in parallel filesets when all elements are distributed to other files.
    // Silently ignore.
    return;
  }

  vtkIntArray* arr = vtkArrayDownCast<vtkIntArray>(
    this->GetCacheOrRead( vtkExodusIICacheKey( -1, conn_type, obj, 0 ) ) );
  if ( ! arr )
  {
    vtkWarningMacro( "Set wasn't present in file? Working around it. Expect trouble." );
    sinfo->Status = 0;
    return;
  }

  switch ( otyp )
  {
  case vtkExodusIIReader::NODE_SET:
    // Easy
    this->InsertSetNodeCopies( arr, otyp, obj, sinfo );
    break;
  case vtkExodusIIReader::EDGE_SET:
    // Not so fun. We must copy cells from possibly many edge blocks.
    this->InsertSetCellCopies( arr, vtkExodusIIReader::EDGE_BLOCK, obj, sinfo );
    break;
  case vtkExodusIIReader::FACE_SET:
    // Not so fun. We must copy cells from possibly many face blocks.
    this->InsertSetCellCopies( arr, vtkExodusIIReader::FACE_BLOCK, obj, sinfo );
    break;
  case vtkExodusIIReader::SIDE_SET:
    // Way hard even when we let Exodus do a lot for us.
    this->InsertSetSides( arr, otyp, obj, sinfo );
    break;
  case vtkExodusIIReader::ELEM_SET:
    // Not so fun. We must copy cells from possibly many element blocks.
    this->InsertSetCellCopies( arr, vtkExodusIIReader::ELEM_BLOCK, obj, sinfo );
    break;
  }
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::AddPointArray(
  vtkDataArray* src, BlockSetInfoType* bsinfop, vtkUnstructuredGrid* output )
{
  vtkPointData* pd = output->GetPointData();
  if ( this->SqueezePoints )
  {
    // subset the array using PointMap
    vtkDataArray* dest = vtkDataArray::CreateDataArray( src->GetDataType() );
    dest->SetName( src->GetName() );
    dest->SetNumberOfComponents( src->GetNumberOfComponents() );
    dest->SetNumberOfTuples( bsinfop->NextSqueezePoint );
    std::map<vtkIdType,vtkIdType>::iterator it, itEnd;
    //
    // I moved the end condition of the loop out of the for(;;) loop.
    //   Assuming it doesn't change within the loop itself!
    //   The reason is that the code was making the call every loop.
    //
    itEnd = bsinfop->PointMap.end();
    for ( it = bsinfop->PointMap.begin(); it != itEnd; ++ it )
    {
      pd->CopyTuple( src, dest, it->first, it->second );
    }
    pd->AddArray( dest );
    dest->FastDelete();
  }
  else
  {
    pd->AddArray( src );
  }
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::InsertSetNodeCopies( vtkIntArray* refs, int otyp, int obj, SetInfoType* sinfo )
{
  (void)otyp;
  (void)obj;
  // Insert a "VERTEX" cell for each node in the set.
  vtkIdType ref;
  vtkIdType tmp;
  int* iptr = refs->GetPointer( 0 );

  if ( this->SqueezePoints )
  { // this loop is separated out to handle case (stride > 1 && pref[1] < 0 && this->SqueezePoints)
    for ( ref = 0; ref < refs->GetNumberOfTuples(); ++ref, ++iptr )
    {
      tmp = *iptr;
      vtkIdType x = this->GetSqueezePointId( sinfo, tmp );
      sinfo->CachedConnectivity->InsertNextCell( VTK_VERTEX, 1, &x );
    }
  }
  else
  {
    for ( ref = 0; ref < refs->GetNumberOfTuples(); ++ref, ++iptr )
    {
      tmp = *iptr;
      sinfo->CachedConnectivity->InsertNextCell( VTK_VERTEX, 1, &tmp );
    }
  }
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::InsertSetCellCopies(
  vtkIntArray* refs, int otyp, int obj, SetInfoType* sinfo )
{
  (void)obj;
  // First, sort the set by entry number (element, face, or edge ID)
  // so that we can refer to each block just once as we process cells.
  vtkSortDataArray::SortArrayByComponent( refs, 0 );
  refs->Register( this ); // Don't let the cache delete this array when we fetch others...

  vtkIdType nrefs = refs->GetNumberOfTuples();
  vtkIdType ref = 0;
  vtkIdType bnum = -1;
  vtkIdType lastBlockEntry = -1;
  int* pref = refs->GetPointer( 0 );
  int stride = refs->GetNumberOfComponents();
  BlockInfoType* binfop = 0; //&this->BlockInfo[otyp][bnum];
  int* nodeconn = 0;
  vtkIdType* cellConn;
  int nnpe = 0;
  vtkIntArray* nconn;
  std::vector<vtkIdType> tmpTuple;
  while ( ref < nrefs )
  {
    int loadNewBlk = 0;
    while ( pref[0] >= lastBlockEntry )
    { // advance to the next block (always true first time through parent loop)
      ++bnum;
      if ( bnum >= (int) this->BlockInfo[otyp].size() )
        return;
      binfop = &this->BlockInfo[otyp][bnum];
      lastBlockEntry = binfop->FileOffset + binfop->Size - 1;
      loadNewBlk = 1;
    }
    if ( loadNewBlk )
    {
      nconn = vtkArrayDownCast<vtkIntArray>(
        this->GetCacheOrRead( vtkExodusIICacheKey( -1, this->GetBlockConnTypeFromBlockType( otyp ), bnum, 0 ) )
        );
      if ( ! nconn )
      {
        vtkErrorMacro( "Unable to read block \"" << binfop->Name.c_str() << "\" (" << binfop->Id << ")" );
        break;
      }
      nodeconn = nconn->GetPointer( 0 );
      nnpe = nconn->GetNumberOfComponents();
      if ( stride > 1 || this->SqueezePoints )
      {
        tmpTuple.resize( nnpe );
      }
    }

    if ( stride > 1 && pref[1] < 0 )
    { // negative orientation => reverse cell connectivity
      vtkIdType off = (pref[0] + 2 - binfop->FileOffset) * nnpe - 1;
      for ( int k = 0; k < nnpe; ++k )
        tmpTuple[k] = nodeconn[off-k];
      cellConn = &tmpTuple[0];
    }
    else
#ifndef VTK_USE_64BIT_IDS
      if ( this->SqueezePoints )
#endif // VTK_USE_64BIT_IDS
      {
      vtkIdType off = (pref[0] + 1 - binfop->FileOffset) * nnpe;
      for ( int k = 0; k < nnpe; ++k )
        tmpTuple[k] = nodeconn[off+k];
      cellConn = &tmpTuple[0];
      }
#ifndef VTK_USE_64BIT_IDS
    else
    {
      cellConn = (int*)nodeconn + (pref[0] + 1 - binfop->FileOffset) * nnpe;
    }
#endif // VTK_USE_64BIT_IDS

    if ( this->SqueezePoints )
    { // this loop is separated out to handle case (stride > 1 && pref[1] < 0 && this->SqueezePoints)
      for ( int k = 0; k < nnpe; ++k )
      { // FIXME: Double-check that cellConn[k] should be in-place re-assigned.
        cellConn[k] = this->GetSqueezePointId( sinfo, cellConn[k] );
      }
    }

    sinfo->CachedConnectivity->InsertNextCell( binfop->CellType, nnpe, cellConn );

    pref += stride;
    ++ref;
  }

  refs->UnRegister( this );
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::InsertSetSides(
  vtkIntArray* refs, int otyp, int obj, SetInfoType* sinfo )
{
  static const int sideCellTypes[] =
    {
    VTK_EMPTY_CELL, // don't support any cells with 0 nodes per side
    VTK_VERTEX,
    VTK_LINE,
    VTK_TRIANGLE,
    VTK_QUAD,
    VTK_EMPTY_CELL, // don't support any cells with 5 nodes per side
    VTK_QUADRATIC_TRIANGLE,
    VTK_EMPTY_CELL, // don't support any cells with 7 nodes per side
    VTK_QUADRATIC_QUAD,
    VTK_BIQUADRATIC_QUAD
    };

  int numSides = this->SetInfo[otyp][obj].Size;
  int* nodesPerSide = refs->GetPointer( 0 );
  int* sideNodes = nodesPerSide + numSides;
  std::vector<vtkIdType> cellConn;
  cellConn.resize( 9 );

  if ( this->SqueezePoints )
  {
    int nnpe;
    for ( int side = 0; side < numSides; ++side )
    {
      nnpe = nodesPerSide[side];
      for ( int k = 0; k < nnpe; ++k )
      {
        cellConn[k] = this->GetSqueezePointId( sinfo, sideNodes[k] );
      }
      sinfo->CachedConnectivity->InsertNextCell( sideCellTypes[nnpe], nnpe, &cellConn[0] );
      sideNodes += nnpe;
    }
  }
  else
  {
    int nnpe;
    for ( int side = 0; side < numSides; ++side )
    {
      nnpe = nodesPerSide[side];
#ifdef VTK_USE_64BIT_IDS
      for ( int k = 0; k < nnpe; ++k )
      {
        cellConn[k] = sideNodes[k];
      }
      sinfo->CachedConnectivity->InsertNextCell( sideCellTypes[nnpe], nnpe, &cellConn[0] );
#else // VTK_USE_64BIT_IDS
      sinfo->CachedConnectivity->InsertNextCell( sideCellTypes[nnpe], nnpe, sideNodes );
#endif // VTK_USE_64BIT_IDS
      sideNodes += nnpe;
    }
  }
}

//-----------------------------------------------------------------------------
vtkDataArray* vtkExodusIIReaderPrivate::GetCacheOrRead( vtkExodusIICacheKey key )
{
  vtkDataArray* arr;
  // Never cache points deflected for a mode shape animation... doubles don't make good keys.
  if ( this->HasModeShapes && key.ObjectType == vtkExodusIIReader::NODAL_COORDS )
  {
    arr = 0;
  }
  else
  {
    arr = this->Cache->Find( key );
  }

  if ( arr )
  {
    //
    return arr;
  }

  int exoid = this->Exoid;

  // If array is NULL, try reading it from file.
  if ( key.ObjectType == vtkExodusIIReader::GLOBAL )
  {
    // need to assemble result array from smaller ones.
    // call GetCacheOrRead() for each smaller array
    // pay attention to SqueezePoints

    //ArrayInfoType* ainfop = &this->ArrayInfo[vtkExodusIIReader::GLOBAL][key.ArrayId];
    arr = vtkDataArray::CreateDataArray( VTK_DOUBLE );
    arr->SetName( this->GetGlobalVariableValuesArrayName() );
    arr->SetNumberOfComponents( 1 );
    arr->SetNumberOfTuples( this->ArrayInfo[ vtkExodusIIReader::GLOBAL ].size() );

    if ( ex_get_glob_vars( exoid, key.Time + 1, arr->GetNumberOfTuples(),
        arr->GetVoidPointer( 0 ) ) < 0 )
    {
      vtkErrorMacro( "Could not read global variable " << this->GetGlobalVariableValuesArrayName() << "." );
      arr->Delete();
      arr = 0;
    }
  }
  else if ( key.ObjectType == vtkExodusIIReader::NODAL )
  {
    // read nodal array
    ArrayInfoType* ainfop = &this->ArrayInfo[vtkExodusIIReader::NODAL][key.ArrayId];
    int ncomps = ( this->ModelParameters.num_dim == 2 && ainfop->Components == 2 ) ? 3 : ainfop->Components;
    arr = vtkDataArray::CreateDataArray( ainfop->StorageType );
    arr->SetName( ainfop->Name.c_str() );
    arr->SetNumberOfComponents( ncomps );
    arr->SetNumberOfTuples( this->ModelParameters.num_nodes );
    if ( ncomps != ainfop->Components )
    {
      arr->FillComponent( 2, 0. );
    }
    if ( ncomps == 1 )
    {
      if ( ex_get_var( exoid, key.Time + 1, static_cast<ex_entity_type>( key.ObjectType ),
          ainfop->OriginalIndices[0], 0, arr->GetNumberOfTuples(),
          arr->GetVoidPointer( 0 ) ) < 0 )
      {
        vtkErrorMacro( "Could not read nodal result variable " << ainfop->Name.c_str() << "." );
        arr->Delete();
        arr = 0;
      }
    }
    else
    {
      // Exodus doesn't support reading with a stride, so we have to manually interleave the arrays. Bleh.
      std::vector<std::vector<double> > tmpVal;
      tmpVal.resize( ainfop->Components );
      int c;
      for ( c = 0; c < ainfop->Components; ++c )
      {
        vtkIdType N = this->ModelParameters.num_nodes;
        tmpVal[c].resize( N );
        if ( ex_get_var( exoid, key.Time + 1, static_cast<ex_entity_type>( key.ObjectType ),
            ainfop->OriginalIndices[c], 0, arr->GetNumberOfTuples(),
            &tmpVal[c][0] ) < 0)
        {
          vtkErrorMacro( "Could not read nodal result variable " << ainfop->OriginalNames[c].c_str() << "." );
          arr->Delete();
          arr = 0;
          return 0;
        }
      }
      int t;
      std::vector<double> tmpTuple;
      tmpTuple.resize( ncomps );
      tmpTuple[ncomps - 1] = 0.; // In case we're embedding a 2-D vector in 3-D

      //
      // Lets unroll the most common case - components == 3.
      //
      if ( ainfop->Components == 3)
      {
        int maxTuples =  arr->GetNumberOfTuples();
        for ( t = 0; t < maxTuples; ++t )
        {
          tmpTuple[0] = tmpVal[0][t];
          tmpTuple[1] = tmpVal[1][t];
          tmpTuple[2] = tmpVal[2][t];
          arr->SetTuple( t, &tmpTuple[0] );
        }
      }
      else
      {
        for ( t = 0; t < arr->GetNumberOfTuples(); ++t )
        {
          for ( c = 0; c < ainfop->Components; ++c )
          {
            tmpTuple[c] = tmpVal[c][t];
          }
          arr->SetTuple( t, &tmpTuple[0] );
        }
      }
    }
  }
  else if ( key.ObjectType == vtkExodusIIReader::GLOBAL_TEMPORAL )
  {
    // read temporal nodal array
    ArrayInfoType* ainfop = &this->ArrayInfo[vtkExodusIIReader::GLOBAL][key.ArrayId];
    arr = vtkDataArray::CreateDataArray( ainfop->StorageType );
    //vtkStdString newArrayName = ainfop->Name + "OverTime";
    arr->SetName( ainfop->Name );
    arr->SetNumberOfComponents( ainfop->Components );
    arr->SetNumberOfTuples( this->GetNumberOfTimeSteps() );
    if ( ainfop->Components != 1 )
    {
      // Exodus doesn't support reading with a stride, so we have to manually interleave the arrays. Bleh.
      std::vector<std::vector<double> > tmpVal;
      tmpVal.resize( ainfop->Components );
      int c;
      for ( c = 0; c < ainfop->Components; ++c )
      {
        vtkIdType N = this->GetNumberOfTimeSteps();
        tmpVal[c].resize( N );
        if ( ex_get_var_time( exoid, EX_GLOBAL,
            ainfop->OriginalIndices[c], key.ObjectId,
            1, this->GetNumberOfTimeSteps(), &tmpVal[c][0] ) < 0 )
        {
          vtkErrorMacro(
            "Could not read temporal global result variable "
            << ainfop->OriginalNames[c].c_str() << "." );
          arr->Delete();
          arr = 0;
          return 0;
        }
      }
      int t;
      std::vector<double> tmpTuple;
      tmpTuple.resize( ainfop->Components );
      for ( t = 0; t < arr->GetNumberOfTuples(); ++t )
      {
        for ( c = 0; c < ainfop->Components; ++c )
        {
          tmpTuple[c] = tmpVal[c][t];
        }
        arr->SetTuple( t, &tmpTuple[0] );
      }
    }
    else if ( ex_get_var_time( exoid, EX_GLOBAL,
        ainfop->OriginalIndices[0], key.ObjectId,
        1, this->GetNumberOfTimeSteps(), arr->GetVoidPointer( 0 ) ) < 0 )
    {
      vtkErrorMacro(
        "Could not read global result variable "
        << ainfop->Name.c_str() << "." );
      arr->Delete();
      arr = 0;
    }
  }
  else if ( key.ObjectType == vtkExodusIIReader::NODAL_TEMPORAL )
  {
    // read temporal nodal array
    ArrayInfoType* ainfop = &this->ArrayInfo[vtkExodusIIReader::NODAL][key.ArrayId];
    arr = vtkDataArray::CreateDataArray( ainfop->StorageType );
    vtkStdString newArrayName = ainfop->Name + "OverTime";
    arr->SetName( newArrayName.c_str() );
    arr->SetNumberOfComponents( ainfop->Components );
    arr->SetNumberOfTuples( this->GetNumberOfTimeSteps() );
    if ( ainfop->Components == 1 )
    {
      if ( ex_get_var_time( exoid, EX_NODAL,
          ainfop->OriginalIndices[0], key.ObjectId,
          1, this->GetNumberOfTimeSteps(), arr->GetVoidPointer( 0 ) ) < 0 )
      {
        vtkErrorMacro( "Could not read nodal result variable " << ainfop->Name.c_str() << "." );
        arr->Delete();
        arr = 0;
      }
    }
    else
    {
      // Exodus doesn't support reading with a stride, so we have to manually interleave the arrays. Bleh.
      std::vector<std::vector<double> > tmpVal;
      tmpVal.resize( ainfop->Components );
      int c;
      for ( c = 0; c < ainfop->Components; ++c )
      {
        vtkIdType N = this->GetNumberOfTimeSteps();
        tmpVal[c].resize( N );
        if ( ex_get_var_time( exoid, EX_NODAL,
            ainfop->OriginalIndices[c], key.ObjectId,
            1, this->GetNumberOfTimeSteps(), &tmpVal[c][0] ) < 0 )
        {
          vtkErrorMacro( "Could not read temporal nodal result variable " << ainfop->OriginalNames[c].c_str() << "." );
          arr->Delete();
          arr = 0;
          return 0;
        }
      }
      int t;
      std::vector<double> tmpTuple;
      tmpTuple.resize( ainfop->Components );
      for ( t = 0; t < arr->GetNumberOfTuples(); ++t )
      {
        for ( c = 0; c < ainfop->Components; ++c )
        {
          tmpTuple[c] = tmpVal[c][t];
        }
        arr->SetTuple( t, &tmpTuple[0] );
      }
    }
  }
  else if ( key.ObjectType == vtkExodusIIReader::ELEM_BLOCK_TEMPORAL )
  {
    // read temporal element array
    ArrayInfoType* ainfop = &this->ArrayInfo[vtkExodusIIReader::ELEM_BLOCK][key.ArrayId];
    arr = vtkDataArray::CreateDataArray( ainfop->StorageType );
    vtkStdString newArrayName = ainfop->Name + "OverTime";
    arr->SetName( newArrayName.c_str() );
    arr->SetNumberOfComponents( ainfop->Components );
    arr->SetNumberOfTuples( this->GetNumberOfTimeSteps() );
    if ( ainfop->Components == 1 )
    {
      if ( ex_get_var_time( exoid, EX_ELEM_BLOCK,
          ainfop->OriginalIndices[0], key.ObjectId,
          1, this->GetNumberOfTimeSteps(), arr->GetVoidPointer( 0 ) ) < 0 )
      {
        vtkErrorMacro( "Could not read element result variable " << ainfop->Name.c_str() << "." );
        arr->Delete();
        arr = 0;
      }
    }
    else
    {
      // Exodus doesn't support reading with a stride, so we have to manually interleave the arrays. Bleh.
      std::vector<std::vector<double> > tmpVal;
      tmpVal.resize( ainfop->Components );
      int c;
      for ( c = 0; c < ainfop->Components; ++c )
      {
        vtkIdType N = this->GetNumberOfTimeSteps();
        tmpVal[c].resize( N );
        if ( ex_get_var_time( exoid, EX_ELEM_BLOCK,
            ainfop->OriginalIndices[c], key.ObjectId,
            1, this->GetNumberOfTimeSteps(), &tmpVal[c][0] ) < 0 )
        {
          vtkErrorMacro( "Could not read temporal element result variable " << ainfop->OriginalNames[c].c_str() << "." );
          arr->Delete();
          arr = 0;
          return 0;
        }
      }
      int t;
      std::vector<double> tmpTuple;
      tmpTuple.resize( ainfop->Components );
      for ( t = 0; t < arr->GetNumberOfTuples(); ++t )
      {
        for ( c = 0; c < ainfop->Components; ++c )
        {
          tmpTuple[c] = tmpVal[c][t];
        }
        arr->SetTuple( t, &tmpTuple[0] );
      }
    }
  }
  else if (
    key.ObjectType == vtkExodusIIReader::EDGE_BLOCK ||
    key.ObjectType == vtkExodusIIReader::FACE_BLOCK ||
    key.ObjectType == vtkExodusIIReader::ELEM_BLOCK ||
    key.ObjectType == vtkExodusIIReader::NODE_SET ||
    key.ObjectType == vtkExodusIIReader::EDGE_SET ||
    key.ObjectType == vtkExodusIIReader::FACE_SET ||
    key.ObjectType == vtkExodusIIReader::SIDE_SET ||
    key.ObjectType == vtkExodusIIReader::ELEM_SET
    )
  {
    int otypidx = this->GetObjectTypeIndexFromObjectType( key.ObjectType );
    ArrayInfoType* ainfop = &this->ArrayInfo[key.ObjectType][key.ArrayId];
    ObjectInfoType* oinfop = this->GetObjectInfo( otypidx, key.ObjectId );

    arr = vtkDataArray::CreateDataArray( ainfop->StorageType );
    arr->SetName( ainfop->Name.c_str() );
    if ( ainfop->Components == 2 && this->ModelParameters.num_dim == 2 )
    {
      // Promote 2-component arrays to 3-component arrays when we have 2-D coordinates
      arr->SetNumberOfComponents( 3 );
    }
    else
    {
      arr->SetNumberOfComponents( ainfop->Components );
    }
    arr->SetNumberOfTuples( oinfop->Size );
    if ( ainfop->Components == 1 )
    {
      if ( ex_get_var( exoid, key.Time + 1, static_cast<ex_entity_type>( key.ObjectType ),
          ainfop->OriginalIndices[0], oinfop->Id, arr->GetNumberOfTuples(),
          arr->GetVoidPointer( 0 ) ) < 0)
      {
        vtkErrorMacro( "Could not read result variable " << ainfop->Name.c_str() <<
          " for " << objtype_names[otypidx] << " " << oinfop->Id << "." );
        arr->Delete();
        arr = 0;
      }
    }
    else
    {
      // Exodus doesn't support reading with a stride, so we have to manually interleave the arrays. Bleh.
      std::vector<std::vector<double> > tmpVal;
      tmpVal.resize( ainfop->Components );
      int c;
      for ( c = 0; c < ainfop->Components; ++c )
      {
        vtkIdType N = arr->GetNumberOfTuples();
        tmpVal[c].resize( N+1 ); // + 1 to avoid errors when N == 0.
                                 // BUG #8746.
        if ( ex_get_var( exoid, key.Time + 1, static_cast<ex_entity_type>( key.ObjectType ),
            ainfop->OriginalIndices[c], oinfop->Id, arr->GetNumberOfTuples(),
            &tmpVal[c][0] ) < 0)
        {
          vtkErrorMacro( "Could not read result variable " << ainfop->OriginalNames[c].c_str() <<
            " for " << objtype_names[otypidx] << " " << oinfop->Id << "." );
          arr->Delete();
          arr = 0;
        }
      }
      // Carefully use arr->GetNumberOfComponents() when sizing
      // output as we may have promoted 2-D arrays to 3-D.
      int t = arr->GetNumberOfComponents();
      std::vector<double> tmpTuple;
      tmpTuple.resize( t );
      tmpTuple[t - 1] = 0.;
      for ( t = 0; t < arr->GetNumberOfTuples(); ++t )
      {
        for ( c = 0; c < ainfop->Components; ++c )
        {
          tmpTuple[c] = tmpVal[c][t];
        }
        arr->SetTuple( t, &tmpTuple[0] );
      }
    }
  }
  else if (
    key.ObjectType == vtkExodusIIReader::NODE_MAP ||
    key.ObjectType == vtkExodusIIReader::EDGE_MAP ||
    key.ObjectType == vtkExodusIIReader::FACE_MAP ||
    key.ObjectType == vtkExodusIIReader::ELEM_MAP
    )
  {
    MapInfoType* minfop = &this->MapInfo[key.ObjectType][key.ArrayId];
    vtkIdTypeArray* iarr = vtkIdTypeArray::New();
    arr = iarr;
    arr->SetName( minfop->Name.c_str() );
    arr->SetNumberOfComponents( 1 );
    switch ( key.ObjectType )
    {
    case vtkExodusIIReader::NODE_MAP:
      arr->SetNumberOfTuples( this->ModelParameters.num_nodes );
      break;
    case vtkExodusIIReader::EDGE_MAP:
      arr->SetNumberOfTuples( this->ModelParameters.num_edge );
      break;
    case vtkExodusIIReader::FACE_MAP:
      arr->SetNumberOfTuples( this->ModelParameters.num_face );
      break;
    case vtkExodusIIReader::ELEM_MAP:
      arr->SetNumberOfTuples( this->ModelParameters.num_elem );
      break;
    }
#ifdef VTK_USE_64BIT_IDS
    {
      std::vector<int> tmpMap( arr->GetNumberOfTuples() );
      if ( ex_get_num_map( exoid, static_cast<ex_entity_type>( key.ObjectType ), minfop->Id, &tmpMap[0] ) < 0 )
      {
        vtkErrorMacro( "Could not read map \"" << minfop->Name.c_str() << "\" (" << minfop->Id << ") from disk." );
        arr->Delete();
        arr = 0;
        return 0;
      }
      //
      // Move the check of maxTuples out of the for(;;) loop.
      //
      vtkIdType maxTuples =  arr->GetNumberOfTuples();
      for ( vtkIdType i = 0; i < maxTuples; ++i )
      {
        iarr->SetValue( i, tmpMap[i] );
      }
    }
#else
    if ( ex_get_num_map( exoid, static_cast<ex_entity_type>( key.ObjectType ), minfop->Id, (int*)arr->GetVoidPointer( 0 ) ) < 0 )
    {
      vtkErrorMacro( "Could not read nodal map variable " << minfop->Name.c_str() << "." );
      arr->Delete();
      arr = 0;
    }
#endif // VTK_USE_64BIT_IDS
  }
  else if ( key.ObjectType == vtkExodusIIReader::GLOBAL_ELEMENT_ID )
  {
    // Yes, the next 2 statements are an intentional misuse of key
    // fields reserved for the ObjectId and ArrayId (since ObjectType
    // is used to signal that we want IDs instead of a field value).
    int otypidx = this->GetObjectTypeIndexFromObjectType( key.ObjectId );
    int obj = key.ArrayId;
    BlockSetInfoType* bsinfop = (BlockSetInfoType*) this->GetObjectInfo( otypidx, obj );

    vtkExodusIICacheKey ckey( -1, -1, 0, 0 );
    switch ( key.ObjectId )
    {
    case vtkExodusIIReader::EDGE_BLOCK:
      ckey.ObjectType = vtkExodusIIReader::EDGE_ID;
      break;
    case vtkExodusIIReader::FACE_BLOCK:
      ckey.ObjectType = vtkExodusIIReader::FACE_ID;
      break;
    case vtkExodusIIReader::ELEM_BLOCK:
    default:
      ckey.ObjectType = vtkExodusIIReader::ELEMENT_ID;
      break;
    }
    vtkIdTypeArray* src = vtkArrayDownCast<vtkIdTypeArray>( this->GetCacheOrRead( ckey ) );
    if ( ! src )
    {
      arr = 0;
      return 0;
    }
    vtkIdTypeArray* iarr = vtkIdTypeArray::New();
    iarr->SetName( vtkExodusIIReader::GetGlobalElementIdArrayName() );
    iarr->SetNumberOfComponents( 1 );
    iarr->SetNumberOfTuples( bsinfop->Size );
    vtkIdType* gloIds = iarr->GetPointer( 0 );
    vtkIdType* srcIds = src->GetPointer( bsinfop->FileOffset - 1 );
    memcpy( gloIds, srcIds, sizeof(vtkIdType) * bsinfop->Size );
    arr = iarr;
  }
  else if ( key.ObjectType == vtkExodusIIReader::IMPLICIT_ELEMENT_ID )
  {
    // Yes, the next 2 statements are an intentional misuse of key
    // fields reserved for the ObjectId and ArrayId (since ObjectType
    // is used to signal that we want IDs instead of a field value).
    int otypidx = this->GetObjectTypeIndexFromObjectType( key.ObjectId );
    int obj = key.ArrayId;
    BlockSetInfoType* bsinfop = (BlockSetInfoType*) this->GetObjectInfo( otypidx, obj );

    vtkExodusIICacheKey ckey( -1, -1, 0, 0 );
    vtkIdType mapSize;
    int nMaps;
    switch ( key.ObjectId )
    {
    case vtkExodusIIReader::EDGE_BLOCK:
      ckey.ObjectType = vtkExodusIIReader::EDGE_ID;
      mapSize = this->ModelParameters.num_edge;
      nMaps = this->ModelParameters.num_edge_maps;
      break;
    case vtkExodusIIReader::FACE_BLOCK:
      ckey.ObjectType = vtkExodusIIReader::FACE_ID;
      mapSize = this->ModelParameters.num_face;
      nMaps = this->ModelParameters.num_face_maps;
      break;
    case vtkExodusIIReader::ELEM_BLOCK:
    default:
      ckey.ObjectType = vtkExodusIIReader::ELEMENT_ID;
      mapSize = this->ModelParameters.num_elem;
      nMaps = this->ModelParameters.num_elem_maps;
      break;
    }
    vtkIdTypeArray* src = vtkIdTypeArray::New ();
    src->SetNumberOfComponents( 1 );
    src->SetNumberOfTuples( mapSize );
    if (nMaps > 0) // FIXME correctly detect parallel
    {
#ifdef VTK_USE_64BIT_IDS
        std::vector<int> tmpMap( src->GetNumberOfTuples() );
        if ( ex_get_id_map( exoid, static_cast<ex_entity_type>( ckey.ObjectType ), &tmpMap[0] ) < 0 )
        {
          vtkErrorMacro( "Could not read elem num map for global implicit id" );
          src->Delete();
          return 0;
        }
        else
        {
          for ( vtkIdType i = 0; i < src->GetNumberOfTuples(); ++i )
          {
            src->SetValue( i, tmpMap[i] );
          }
        }
#else
        if ( ex_get_id_map( exoid, static_cast<ex_entity_type>( ckey.ObjectType ), (int*)src->GetPointer( 0 ) ) < 0 )
        {
          vtkErrorMacro( "Could not read elem num map for global implicit id" );
          src->Delete();
          return 0;
        }
#endif // VTK_USE_64BIT_IDS
    }
    else // single file, just make the implicit index explicit
    {
      for (vtkIdType i = 0; i < src->GetNumberOfTuples (); i ++)
      {
        src->SetValue (i, i + 1);
      }
    }
    vtkIdTypeArray* iarr = vtkIdTypeArray::New();
    iarr->SetName( vtkExodusIIReader::GetImplicitElementIdArrayName() );
    iarr->SetNumberOfComponents( 1 );
    iarr->SetNumberOfTuples( bsinfop->Size );
    vtkIdType* gloIds = iarr->GetPointer( 0 );
    vtkIdType* srcIds = src->GetPointer( bsinfop->FileOffset - 1 );
    memcpy( gloIds, srcIds, sizeof(vtkIdType) * bsinfop->Size );
    arr = iarr;
    src->Delete ();
  }
  else if ( key.ObjectType == vtkExodusIIReader::GLOBAL_NODE_ID )
  { // subset the NODE_ID array choosing only entries for nodes in output grid (using PointMap)
    // Yes, the next 2 statements are an intentional misuse of key
    // fields reserved for the ObjectId and ArrayId (since ObjectType
    // is used to signal that we want IDs instead of a field value).
    int otypidx = this->GetObjectTypeIndexFromObjectType( key.ObjectId );
    int obj = key.ArrayId;
    BlockSetInfoType* bsinfop = (BlockSetInfoType*) this->GetObjectInfo( otypidx, obj );
    vtkIdTypeArray* src = vtkArrayDownCast<vtkIdTypeArray>(
      this->GetCacheOrRead( vtkExodusIICacheKey( -1, vtkExodusIIReader::NODE_ID, 0, 0 ) ) );
    if ( this->SqueezePoints && src )
    {
      vtkIdTypeArray* iarr = vtkIdTypeArray::New();
      iarr->SetName( vtkExodusIIReader::GetGlobalNodeIdArrayName() );
      iarr->SetNumberOfComponents( 1 );
      iarr->SetNumberOfTuples( bsinfop->NextSqueezePoint );
      vtkIdType* gloIds = iarr->GetPointer( 0 );
      vtkIdType* srcIds = src->GetPointer( 0 );
      std::map<vtkIdType,vtkIdType>::iterator it;
      for ( it = bsinfop->PointMap.begin(); it != bsinfop->PointMap.end(); ++ it )
      {
        gloIds[it->second] = srcIds[it->first];
      }
      arr = iarr;
    }
    else
    {
      arr = src;
    }
  }
  else if ( key.ObjectType == vtkExodusIIReader::IMPLICIT_NODE_ID )
  { // subset the NODE_ID array choosing only entries for nodes in output grid (using PointMap)
    // Yes, the next 2 statements are an intentional misuse of key
    // fields reserved for the ObjectId and ArrayId (since ObjectType
    // is used to signal that we want IDs instead of a field value).
    int otypidx = this->GetObjectTypeIndexFromObjectType( key.ObjectId );
    int obj = key.ArrayId;
    BlockSetInfoType* bsinfop = (BlockSetInfoType*) this->GetObjectInfo( otypidx, obj );
    vtkIdTypeArray* src = vtkIdTypeArray::New ();
    src->SetNumberOfComponents( 1 );
    src->SetNumberOfTuples( this->ModelParameters.num_nodes );
    if (this->ModelParameters.num_node_maps > 0) // FIXME correctly detect parallel
    {
#ifdef VTK_USE_64BIT_IDS
        std::vector<int> tmpMap( src->GetNumberOfTuples() );
        if ( ex_get_id_map( exoid, (ex_entity_type)( vtkExodusIIReader::NODE_MAP ), &tmpMap[0] ) < 0 )
        {
          vtkErrorMacro( "Could not read node num map for global implicit id" );
          src->Delete();
          return 0;
        }
        else
        {
          for ( vtkIdType i = 0; i < src->GetNumberOfTuples(); ++i )
          {
            src->SetValue( i, tmpMap[i] );
          }
        }
#else
        if ( ex_get_id_map( exoid, (ex_entity_type)( vtkExodusIIReader::NODE_MAP ), (int*)src->GetPointer( 0 ) ) < 0 )
        {
          vtkErrorMacro( "Could not node node num map for global implicit id" );
          src->Delete();
          return 0;
        }
#endif // VTK_USE_64BIT_IDS
    }
    else // single file, just make the implicit index explicit
    {
      for (vtkIdType i = 0; i < src->GetNumberOfTuples (); i ++)
      {
        src->SetValue (i, i + 1);
      }
    }
    if ( this->SqueezePoints && src )
    {
      vtkIdTypeArray* iarr = vtkIdTypeArray::New();
      iarr->SetName( vtkExodusIIReader::GetImplicitNodeIdArrayName() );
      iarr->SetNumberOfComponents( 1 );
      iarr->SetNumberOfTuples( bsinfop->NextSqueezePoint );
      vtkIdType* gloIds = iarr->GetPointer( 0 );
      vtkIdType* srcIds = src->GetPointer( 0 );
      std::map<vtkIdType,vtkIdType>::iterator it;
      for ( it = bsinfop->PointMap.begin(); it != bsinfop->PointMap.end(); ++ it )
      {
        gloIds[it->second] = srcIds[it->first];
      }
      arr = iarr;
    }
    else
    {
      arr = src;
    }
    src->Delete ();
  }
  else if (
    key.ObjectType == vtkExodusIIReader::ELEMENT_ID ||
    key.ObjectType == vtkExodusIIReader::EDGE_ID ||
    key.ObjectType == vtkExodusIIReader::FACE_ID ||
    key.ObjectType == vtkExodusIIReader::NODE_ID
    )
  {
    vtkIdTypeArray* iarr;
    int nMaps;
    vtkIdType mapSize;
    vtkExodusIICacheKey ktmp;
    if ( key.ObjectType == vtkExodusIIReader::ELEMENT_ID )
    {
      nMaps = this->ModelParameters.num_elem_maps;
      mapSize = this->ModelParameters.num_elem;
      ktmp = vtkExodusIICacheKey( -1, vtkExodusIIReader::ELEM_MAP, 0, 0 );
    }
    else if ( key.ObjectType == vtkExodusIIReader::FACE_ID )
    {
      nMaps = this->ModelParameters.num_face_maps;
      mapSize = this->ModelParameters.num_face;
      ktmp = vtkExodusIICacheKey( -1, vtkExodusIIReader::FACE_MAP, 0, 0 );
    }
    else if ( key.ObjectType == vtkExodusIIReader::EDGE_ID )
    {
      nMaps = this->ModelParameters.num_edge_maps;
      mapSize = this->ModelParameters.num_edge;
      ktmp = vtkExodusIICacheKey( -1, vtkExodusIIReader::EDGE_MAP, 0, 0 );
    }
    else // ( key.ObjectType == vtkExodusIIReader::NODE_ID )
    {
      nMaps = this->ModelParameters.num_node_maps;
      mapSize = this->ModelParameters.num_nodes;
      ktmp = vtkExodusIICacheKey( -1, vtkExodusIIReader::NODE_MAP, 0, 0 );
    }
    // If there are no new-style maps, get the old-style map (which creates a default if nothing is stored on disk).
    if ( nMaps < 1 || ! (iarr = vtkArrayDownCast<vtkIdTypeArray>(this->GetCacheOrRead( ktmp ))) )
    {
      iarr = vtkIdTypeArray::New();
      iarr->SetNumberOfComponents( 1 );
      iarr->SetNumberOfTuples( mapSize );
      if ( mapSize )
      {
#ifdef VTK_USE_64BIT_IDS
        std::vector<int> tmpMap( iarr->GetNumberOfTuples() );
        if ( ex_get_id_map( exoid, static_cast<ex_entity_type>( ktmp.ObjectType ), &tmpMap[0] ) < 0 )
        {
          vtkErrorMacro( "Could not read old-style node or element map." );
          iarr->Delete();
          iarr = 0;
        }
        else
        {
          //
          // Move getting the maxTuples from the for(;;) loop.
          //
          vtkIdType maxTuples = iarr->GetNumberOfTuples();
          for ( vtkIdType i = 0; i < maxTuples ; ++i )
          {
            iarr->SetValue( i, tmpMap[i] );
          }
        }
#else
        if ( ex_get_id_map( exoid, static_cast<ex_entity_type>( ktmp.ObjectType ), (int*)iarr->GetPointer( 0 ) ) < 0 )
        {
          vtkErrorMacro( "Could not read old-style node or element map." );
          iarr->Delete();
          iarr = 0;
        }
#endif // VTK_USE_64BIT_IDS
      }
    }
    else
    {
      // FastDelete will be called below (because we are assumed to have created the array with New()).
      // So we must reference the array one extra time here to account for the extra delete...
      iarr->Register( this );
    }
    arr = iarr;
  }
  else if ( key.ObjectType == vtkExodusIIReader::GLOBAL_CONN )
  {
    vtkErrorMacro(
      "Global connectivity is created in AssembleOutputConnectivity since it can't be cached\n"
      "with a single vtkDataArray. Who told you to call this routine to get it?"
      );
  }
  else if ( key.ObjectType == vtkExodusIIReader::ENTITY_COUNTS )
  {
    int ctypidx = (key.ArrayId == 0 ? 0 : 1);
    int otypidx = conn_obj_idx_cvt[ctypidx];
    int otyp = obj_types[ otypidx ];
    BlockInfoType* binfop = (BlockInfoType*) this->GetObjectInfo( otypidx, key.ObjectId );
    vtkIntArray* iarr = vtkIntArray::New();
    iarr->SetNumberOfComponents (1);
    iarr->SetNumberOfTuples (binfop->Size);
    if ( ex_get_entity_count_per_polyhedra ( exoid, static_cast<ex_entity_type>( otyp ), binfop->Id,
                                             iarr->GetPointer(0)) < 0 )
    {
      vtkErrorMacro( "Unable to read " << binfop->Id << " (index " << key.ObjectId <<
        ") entity count per polyhedra" );
      iarr->Delete();
      iarr = 0;
    }
    arr = iarr;
  }
  else if (
    key.ObjectType == vtkExodusIIReader::ELEM_BLOCK_ELEM_CONN ||
    key.ObjectType == vtkExodusIIReader::FACE_BLOCK_CONN ||
    key.ObjectType == vtkExodusIIReader::EDGE_BLOCK_CONN
  )
  {
    int ctypidx = this->GetConnTypeIndexFromConnType( key.ObjectType );
    int otypidx = conn_obj_idx_cvt[ctypidx];
    int otyp = obj_types[ otypidx ];
    BlockInfoType* binfop = (BlockInfoType*) this->GetObjectInfo( otypidx, key.ObjectId );

    vtkIntArray* iarr = vtkIntArray::New();
    if (binfop->CellType == VTK_POLYGON)
    {
      iarr->SetNumberOfValues (binfop->BdsPerEntry[0]);
    }
    else if (binfop->CellType == VTK_POLYHEDRON)
    {
      iarr->SetNumberOfValues (binfop->BdsPerEntry[2]);
    }
    else
    {
      iarr->SetNumberOfComponents( binfop->BdsPerEntry[0] );
      iarr->SetNumberOfTuples( binfop->Size );
    }

    if ( ex_get_conn( exoid, static_cast<ex_entity_type>( otyp ), binfop->Id, iarr->GetPointer(0), 0, 0 ) < 0 )
    {
      vtkErrorMacro( "Unable to read " << objtype_names[otypidx] << " " << binfop->Id << " (index " << key.ObjectId <<
        ") nodal connectivity." );
      iarr->Delete();
      iarr = 0;
    }

    vtkIdType c;
    int* ptr = iarr->GetPointer( 0 );
    if (
      binfop->CellType == VTK_QUADRATIC_HEXAHEDRON ||
      binfop->CellType == VTK_TRIQUADRATIC_HEXAHEDRON
      )
    {
      // Edge order for VTK is different than Exodus edge order.
      for ( c = 0; c < iarr->GetNumberOfTuples(); ++c )
      {
        int k;
        int itmp[4];

        for ( k = 0; k < 12; ++k, ++ptr)
          *ptr = *ptr - 1;

        for ( k = 0; k < 4; ++k, ++ptr)
        {
          itmp[k] = *ptr;
          *ptr = ptr[4] - 1;
        }

        for ( k = 0; k < 4; ++k, ++ptr )
          *ptr = itmp[k] - 1;

        if ( binfop->CellType == VTK_TRIQUADRATIC_HEXAHEDRON )
        { // Face/body order for VTK is different than Exodus (Patran) order.
          for ( k = 0; k < 4; ++k, ++ptr )
          {
            itmp[k] = *ptr;
            *ptr = ptr[3] - 1;
          }
          *(ptr++) = itmp[1] - 1;
          *(ptr++) = itmp[2] - 1;
          *(ptr++) = itmp[0] - 1;
        }
      }
      ptr += binfop->BdsPerEntry[0] - binfop->PointsPerCell;
    }
    else if (binfop->CellType == VTK_QUADRATIC_WEDGE)
    {
      int k;
      int itmp[3];
      for ( c = 0; c < iarr->GetNumberOfTuples(); ++c )
      {
        for (k = 0; k < 9; ++k, ++ptr)
          *ptr = *ptr - 1;

        for (k = 0; k < 3; ++k, ++ptr)
        {
          itmp[k] = *ptr;
          *ptr = ptr[3] - 1;
        }

        for (k = 0; k < 3; ++k, ++ptr)
        {
          *ptr = itmp[k] - 1;
        }
      }
      ptr += binfop->BdsPerEntry[0] - binfop->PointsPerCell;
    }
    else
    {
      for ( c = 0; c <= iarr->GetMaxId(); ++c, ++ptr )
      {
        *ptr = *ptr - 1;
      }
    }

    arr = iarr;
  }
  else if (
    key.ObjectType == vtkExodusIIReader::ELEM_BLOCK_FACE_CONN ||
    key.ObjectType == vtkExodusIIReader::ELEM_BLOCK_EDGE_CONN )
  {
    arr = 0;

    // bdsEntry will determine whether we call ex_get_conn to read edge or face connectivity:
    int bdsEntry = key.ObjectType == vtkExodusIIReader::ELEM_BLOCK_EDGE_CONN ? 1 : 2;

    // Fetch the block information from the key
    //int ctypidx = this->GetConnTypeIndexFromConnType( key.ObjectType );
    int otypidx = 2; // These always refer to the element block
    int otyp = obj_types[ otypidx ];
    BlockInfoType* binfop = (BlockInfoType*) this->GetObjectInfo( otypidx, key.ObjectId );

    // Only create the array if there's anything to put in it.
    if (binfop->BdsPerEntry[bdsEntry] > 0)
    {
      vtkIntArray* iarr = vtkIntArray::New();
      iarr->SetNumberOfValues (binfop->BdsPerEntry[2]);

      if (
        ex_get_conn(
          exoid,
          static_cast<ex_entity_type>( otyp ),
          binfop->Id,
          NULL,
          bdsEntry == 1 ? iarr->GetPointer(0) : NULL,
          bdsEntry == 2 ? iarr->GetPointer(0) : NULL ) < 0)
      {
        vtkErrorMacro(
          "Unable to read " << objtype_names[otypidx] << " "
          << binfop->Id << " (index " << key.ObjectId <<
          ") " << (bdsEntry == 1 ? "edge" : "face") << " connectivity." );
        iarr->Delete();
        iarr = 0;
      }
      else
      {
        vtkIdType c;
        int* ptr = iarr->GetPointer( 0 );
        for ( c = 0; c <= iarr->GetMaxId(); ++c, ++ptr )
        {
          *ptr = *ptr - 1;
        }
      }
      arr = iarr;
    }
  }
  else if (
    key.ObjectType == vtkExodusIIReader::NODE_SET_CONN ||
    key.ObjectType == vtkExodusIIReader::ELEM_SET_CONN
  )
  {
    int otyp = this->GetSetTypeFromSetConnType( key.ObjectType );
    int otypidx = this->GetObjectTypeIndexFromObjectType( otyp );
    SetInfoType* sinfop = &this->SetInfo[otyp][key.ObjectId];
    vtkIntArray* iarr = vtkIntArray::New();
    iarr->SetNumberOfComponents( 1 );
    iarr->SetNumberOfTuples( sinfop->Size );
    int* iptr = iarr->GetPointer( 0 );

    if ( ex_get_set( exoid, static_cast<ex_entity_type>( otyp ), sinfop->Id, iptr, 0 ) < 0 )
    {
      vtkErrorMacro( "Unable to read " << objtype_names[otypidx] << " " << sinfop->Id << " (index " << key.ObjectId <<
        ") nodal connectivity." );
      iarr->Delete();
      iarr = 0;
    }

    vtkIdType id;
    for ( id = 0; id < sinfop->Size; ++id, ++iptr )
    { // VTK uses 0-based indexing, unlike Exodus:
      --(*iptr);
    }

    arr = iarr;
  }
  else if ( key.ObjectType == vtkExodusIIReader::EDGE_SET_CONN || key.ObjectType == vtkExodusIIReader::FACE_SET_CONN )
  {
    int otyp = this->GetSetTypeFromSetConnType( key.ObjectType );
    int otypidx = this->GetObjectTypeIndexFromObjectType( otyp );
    SetInfoType* sinfop = &this->SetInfo[otyp][key.ObjectId];
    vtkIntArray* iarr = vtkIntArray::New();
    iarr->SetNumberOfComponents( 2 );
    iarr->SetNumberOfTuples( sinfop->Size );
    std::vector<int> tmpOrient; // hold the edge/face orientation information until we can interleave it.
    tmpOrient.resize( sinfop->Size );

    if ( ex_get_set( exoid, static_cast<ex_entity_type>( otyp ), sinfop->Id, iarr->GetPointer(0), &tmpOrient[0] ) < 0 )
    {
      vtkErrorMacro( "Unable to read " << objtype_names[otypidx] << " " << sinfop->Id << " (index " << key.ObjectId <<
        ") nodal connectivity." );
      iarr->Delete();
      iarr = 0;
      return 0;
    }

    int* iap = iarr->GetPointer( 0 );
    vtkIdType c;
    for ( c = sinfop->Size - 1; c >= 0; --c )
    {
      iap[2*c] = iap[c] - 1; // VTK uses 0-based indexing
      iap[2*c + 1] = tmpOrient[c];
    }

    arr = iarr;
  }
  else if ( key.ObjectType == vtkExodusIIReader::SIDE_SET_CONN )
  {
    if(key.ArrayId <= 0)
    {
      // Stick all of side_set_node_list and side_set_node_count and side_set_nodes_per_side in one array
      // let InsertSetSides() figure it all out. Except for 0-based indexing
      SetInfoType* sinfop = &this->SetInfo[vtkExodusIIReader::SIDE_SET][key.ObjectId];
      int ssnllen; // side set node list length
      if ( ex_get_side_set_node_list_len( exoid, sinfop->Id, &ssnllen ) < 0 )
      {
        vtkErrorMacro( "Unable to fetch side set \"" << sinfop->Name.c_str() << "\" (" << sinfop->Id << ") node list length" );
        arr = 0;
        return 0;
      }
      vtkIntArray* iarr = vtkIntArray::New();
      vtkIdType ilen = ssnllen + sinfop->Size;
      iarr->SetNumberOfComponents( 1 );
      iarr->SetNumberOfTuples( ilen );
      int* dat = iarr->GetPointer( 0 );
      if ( ex_get_side_set_node_list( exoid, sinfop->Id, dat, dat + sinfop->Size ) < 0 )
      {
        vtkErrorMacro( "Unable to fetch side set \"" << sinfop->Name.c_str() << "\" (" << sinfop->Id << ") node list" );
        iarr->Delete();
        arr = 0;
        return 0;
      }
      while ( ilen > sinfop->Size )
      { // move to 0-based indexing on nodes, don't touch nodes/side counts at head of array
        --dat[--ilen];
      }
      arr = iarr;
    } // if(key.ArrayId <= 0)
    else
    {
      // return information about where the side set cells come from on the elements
      // the first tuple value is the element id and the second is the canonical side
      // sinfop->Size is the number of sides in this side set
      SetInfoType* sinfop = &this->SetInfo[vtkExodusIIReader::SIDE_SET][key.ObjectId];
      std::vector<int> side_set_elem_list(sinfop->Size);
      std::vector<int> side_set_side_list(sinfop->Size);
      if ( ex_get_side_set( exoid, sinfop->Id, &side_set_elem_list[0], &side_set_side_list[0]) < 0 )
      {
        vtkErrorMacro( "Unable to fetch side set \"" << sinfop->Name.c_str() << "\" (" << sinfop->Id << ") node list" );
        arr = 0;
        return 0;
      }
      vtkIdTypeArray* iarr = vtkIdTypeArray::New();
      iarr->SetNumberOfComponents( 2 );
      iarr->SetNumberOfTuples( sinfop->Size );
      for(int i=0;i<sinfop->Size;i++)
      { // we'll have to fix up the side indexing later
        // because Exodus and VTK have different canonical orderings for wedges and hexes.
        vtkIdType info[2] = {side_set_elem_list[i], side_set_side_list[i]};
        iarr->SetTypedTuple(i, info);
      }
      arr = iarr;
    }
  }
  else if ( key.ObjectType == vtkExodusIIReader::NODAL_COORDS )
  {
    // read node coords
    vtkDataArray* displ = 0;
    if ( this->ApplyDisplacements && key.Time >= 0 )
    {
      displ = this->FindDisplacementVectors( key.Time );
    }

    std::vector<double> coordTmp;
    vtkDoubleArray* darr = vtkDoubleArray::New();
    arr = darr;
    arr->SetNumberOfComponents( 3 );
    arr->SetNumberOfTuples( this->ModelParameters.num_nodes );
    int dim = this->ModelParameters.num_dim;
    int c;
    vtkIdType t;
    double* xc = 0;
    double* yc = 0;
    double* zc = 0;
    coordTmp.resize( this->ModelParameters.num_nodes );
    for ( c = 0; c < dim; ++c )
    {
      switch ( c )
      {
      case 0:
        xc = &coordTmp[0];
        break;
      case 1:
        yc = xc;
        xc = 0;
        break;
      case 2:
        zc = yc;
        yc = 0;
        break;
      default:
        vtkErrorMacro( "Bad coordinate index " << c << " when reading point coordinates." );
        xc = yc = zc = 0;
      }
      if ( ex_get_coord( exoid, xc, yc, zc ) < 0 )
      {
        vtkErrorMacro( "Unable to read node coordinates for index " << c << "." );
        arr->Delete();
        arr = 0;
        break;
      }
      double* cptr = darr->GetPointer( c );

      //
      // num_nodes can get big.  Lets unroll the loop
      //
      for ( t = 0; t+8 < this->ModelParameters.num_nodes; t+=8 )
      {
        *(cptr+3*0) = coordTmp[t+0];
        *(cptr+3*1) = coordTmp[t+1];
        *(cptr+3*2) = coordTmp[t+2];
        *(cptr+3*3) = coordTmp[t+3];
        *(cptr+3*4) = coordTmp[t+4];
        *(cptr+3*5) = coordTmp[t+5];
        *(cptr+3*6) = coordTmp[t+6];
        *(cptr+3*7) = coordTmp[t+7];
        cptr += 3*8;
      }

      for ( ; t < this->ModelParameters.num_nodes; ++t )
      {
        *cptr = coordTmp[t];
        cptr += 3;
      }
    }
    if ( dim < 3 )
    {
      double* cptr = darr->GetPointer( 2 );
      for ( t = 0; t < this->ModelParameters.num_nodes; ++t, cptr += 3 )
      {
        *cptr = 0.;
      }
    }
    //
    // Unrolling some of the inner loops for the most common case - dim 3.
    // Also moving the maxTuples from inside of the for(;;) loops
    // Also moving cos() calculations out of the bottom of loops.
    //
    if ( displ )
    {
      double* coords = darr->GetPointer( 0 );
      if ( this->HasModeShapes && this->AnimateModeShapes )
      {
        double tempDouble = this->DisplacementMagnitude * cos(2.0 * vtkMath::Pi() * this->ModeShapeTime);
        if ( dim == 3)
        {
          vtkIdType  maxTuples =  arr->GetNumberOfTuples();
          for ( vtkIdType idx = 0; idx < maxTuples; ++idx )
          {
            double* dispVal = displ->GetTuple( idx );
            coords[0] += dispVal[0] * tempDouble;
            coords[1] += dispVal[1] * tempDouble;
            coords[2] += dispVal[2] * tempDouble;
            coords += 3;
          }
        }
        else
        {
          for ( vtkIdType idx = 0; idx < displ->GetNumberOfTuples(); ++idx )
          {
            double* dispVal = displ->GetTuple( idx );
            for ( c = 0; c < dim; ++c )
              coords[c] += dispVal[c] * tempDouble;

            coords += 3;
          }
        }
      }
      else
      {
        if ( dim == 3)
        {
          vtkIdType  maxTuples =  arr->GetNumberOfTuples();
          for ( vtkIdType idx = 0; idx < maxTuples; ++idx )
          {
            double* dispVal = displ->GetTuple( idx );
            coords[0] += dispVal[0] * this->DisplacementMagnitude;
            coords[1] += dispVal[1] * this->DisplacementMagnitude;
            coords[2] += dispVal[2] * this->DisplacementMagnitude;

            coords += 3;
          }
        }
        else
        {
          for ( vtkIdType idx = 0; idx < displ->GetNumberOfTuples(); ++idx )
          {
            double* dispVal = displ->GetTuple( idx );
            for ( c = 0; c < dim; ++c )
              coords[c] += dispVal[c] * this->DisplacementMagnitude;

            coords += 3;
          }
        }
      }
    }
  }
  else if ( key.ObjectType == vtkExodusIIReader::OBJECT_ID )
  {
    BlockSetInfoType* bsinfop;
    // Yes, the next 2 statements are an intentional misuse of key
    // fields reserved for the ObjectId and ArrayId (since ObjectType
    // is used to signal that we want IDs instead of a field value).
    int otypidx = this->GetObjectTypeIndexFromObjectType( key.ObjectId );
    int obj = key.ArrayId;
    bsinfop = (BlockSetInfoType*) this->GetObjectInfo( otypidx, obj );

    arr = vtkIntArray::New();
    arr->SetName( this->GetObjectIdArrayName() );
    arr->SetNumberOfComponents( 1 );
    arr->SetNumberOfTuples( bsinfop->Size );
    arr->FillComponent( 0, bsinfop->Id );

  }
  else if (
    key.ObjectType == vtkExodusIIReader::ELEM_BLOCK_ATTRIB ||
    key.ObjectType == vtkExodusIIReader::FACE_BLOCK_ATTRIB ||
    key.ObjectType == vtkExodusIIReader::EDGE_BLOCK_ATTRIB
    )
  {
    int blkType =
      (key.ObjectType == vtkExodusIIReader::ELEM_BLOCK_ATTRIB ? vtkExodusIIReader::ELEM_BLOCK :
       (key.ObjectType == vtkExodusIIReader::FACE_BLOCK_ATTRIB ? vtkExodusIIReader::FACE_BLOCK :
        vtkExodusIIReader::EDGE_BLOCK));
    BlockInfoType* binfop = &this->BlockInfo[blkType][key.ObjectId];
    vtkDoubleArray* darr = vtkDoubleArray::New();
    arr = darr;
    darr->SetName( binfop->AttributeNames[key.ArrayId].c_str() );
    darr->SetNumberOfComponents( 1 );
    darr->SetNumberOfTuples( binfop->Size );
    if ( ex_get_one_attr(
        exoid, static_cast<ex_entity_type>(blkType), binfop->Id, key.ArrayId + 1, darr->GetVoidPointer( 0 ) ) < 0 )
    { // NB: The error message references the file-order object id, not the numerically sorted index presented to users.
      vtkErrorMacro( "Unable to read attribute " << key.ArrayId
        << " for object " << key.ObjectId << " of type " << key.ObjectType  << " block type " << blkType << "." );
      arr->Delete();
      arr = 0;
    }
  }
  else if ( key.ObjectType == vtkExodusIIReader::INFO_RECORDS )
  {
    // Get Exodus II INFO records.  Each INFO record is a single character string.
    int num_info = 0;
    float fdum;
    char* cdum = 0;
    int i;

    vtkCharArray* carr = vtkCharArray::New();
    carr->SetName("Info_Records");
    carr->SetNumberOfComponents( MAX_LINE_LENGTH+1 );

    if( ex_inquire( exoid, EX_INQ_INFO, &num_info, &fdum, cdum ) < 0 )
    {
      vtkErrorMacro( "Unable to get number of INFO records from ex_inquire" );
      carr->Delete();
      arr = 0;
    }
    else
    {
      if ( num_info > 0 )
      {
        carr->SetNumberOfTuples(num_info);
        char** info = (char**) calloc( num_info, sizeof(char*) );

        for ( i = 0; i < num_info; ++i )
          info[i] = (char *) calloc ( ( MAX_LINE_LENGTH + 1 ), sizeof(char) );

        if ( ex_get_info( exoid, info ) < 0 )
        {
          vtkErrorMacro( "Unable to read INFO records from ex_get_info");
          carr->Delete();
          arr = 0;
        }
        else
        {
          for ( i = 0; i < num_info; ++i )
          {
            carr->InsertTypedTuple( i, info[i] );
          }
          arr = carr;
        }

        for ( i = 0; i < num_info; ++i )
        {
          free(info[i]);
        }
        free(info);
      }
      else
      {
        carr->Delete();
        arr = 0;
      }
    }
  }
  else if ( key.ObjectType == vtkExodusIIReader::QA_RECORDS )
  {
    // Get Exodus II QA records.  Each QA record is made up of 4 character strings.
    int num_qa_rec = 0;
    float fdum;
    char* cdum = 0;
    int i,j;

    vtkCharArray* carr = vtkCharArray::New();
    carr->SetName( "QA_Records" );
    carr->SetNumberOfComponents( MAX_STR_LENGTH + 1 );

    if ( ex_inquire( exoid, EX_INQ_QA, &num_qa_rec, &fdum, cdum ) < 0 )
    {
      vtkErrorMacro( "Unable to get number of QA records from ex_inquire" );
      carr->Delete();
      arr = 0;
    }
    else
    {
      if( num_qa_rec > 0 )
      {
        carr->SetNumberOfTuples(num_qa_rec*4);
        char* (*qa_record)[4] = (char* (*)[4]) calloc(num_qa_rec, sizeof(*qa_record));

        for ( i = 0; i < num_qa_rec; ++i )
        {
          for ( j = 0; j < 4 ; ++j )
          {
            qa_record[i][j] = (char *) calloc( ( MAX_STR_LENGTH + 1 ), sizeof(char) );
          }
        }

        if ( ex_get_qa( exoid, qa_record ) < 0 )
        {
          vtkErrorMacro( "Unable to read QA records from ex_get_qa");
          carr->Delete();
          arr = 0;
        }
        else
        {
          for ( i = 0; i < num_qa_rec; ++i )
          {
            for ( j = 0; j < 4 ; ++j )
            {
              carr->InsertTypedTuple( ( i * 4 ) + j, qa_record[i][j] );
            }
          }
          arr = carr;
        }

        for ( i = 0; i < num_qa_rec; ++i )
        {
          for ( j = 0; j < 4 ; ++j )
          {
            free( qa_record[i][j] );
          }
        }
        free( qa_record );
      }
      else
      {
        carr->Delete();
        arr = 0;
      }
    }
  }
  else
  {
    vtkWarningMacro( "You requested an array for objects of type " << key.ObjectType <<
      " which I know nothing about" );
    arr = 0;
  }

  // Even if the array is larger than the allowable cache size, it will keep the most recent insertion.
  // So, we delete our reference knowing that the Cache will keep the object "alive" until whatever
  // called GetCacheOrRead() references the array. But, once you get an array from GetCacheOrRead(),
  // you better start running!
  if ( arr )
  {
    this->Cache->Insert( key, arr );
    arr->FastDelete();
  }
  return arr;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetConnTypeIndexFromConnType( int ctyp )
{
  int i;
  for ( i = 0; i < num_conn_types; ++i )
  {
    if ( conn_types[i] == ctyp )
    {
      return i;
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetObjectTypeIndexFromObjectType( int otyp )
{
  int i;
  for ( i = 0; i < num_obj_types; ++i )
  {
    if ( obj_types[i] == otyp )
    {
      return i;
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetNumberOfObjectsAtTypeIndex( int typeIndex )
{
  if ( typeIndex < 0 )
  {
    return 0;
  }
  else if ( typeIndex < 3 )
  {
    return (int) this->BlockInfo[obj_types[typeIndex]].size();
  }
  else if ( typeIndex < 8 )
  {
    return (int) this->SetInfo[obj_types[typeIndex]].size();
  }
  else if ( typeIndex < 12 )
  {
    return (int) this->MapInfo[obj_types[typeIndex]].size();
  }
  return 0;
}

//-----------------------------------------------------------------------------
vtkExodusIIReaderPrivate::ObjectInfoType* vtkExodusIIReaderPrivate::GetObjectInfo( int typeIndex, int objectIndex )
{
  if ( typeIndex < 0 )
  {
    return 0;
  }
  else if ( typeIndex < 3 )
  {
    return &this->BlockInfo[obj_types[typeIndex]][objectIndex];
  }
  else if ( typeIndex < 8 )
  {
    return &this->SetInfo[obj_types[typeIndex]][objectIndex];
  }
  else if ( typeIndex < 12 )
  {
    return &this->MapInfo[obj_types[typeIndex]][objectIndex];
  }
  return 0;
}

//-----------------------------------------------------------------------------
vtkExodusIIReaderPrivate::ObjectInfoType* vtkExodusIIReaderPrivate::GetSortedObjectInfo( int otyp, int k )
{
  int i = this->GetObjectTypeIndexFromObjectType( otyp );
  if ( i < 0 )
  {
    vtkDebugMacro( "Could not find collection of objects with type " << otyp << "." );
    return 0;
  }
  int N = this->GetNumberOfObjectsAtTypeIndex( i );
  if ( k < 0 || k >= N )
  {
    const char* otname = i >= 0 ? objtype_names[i] : "object";
    static_cast<void>(otname); // not referenced warning
    vtkDebugMacro( "You requested " << otname << " " << k << " in a collection of only " << N << " objects." );
    return 0;
  }
  return this->GetObjectInfo( i, this->SortedObjectIndices[otyp][k] );
}

//-----------------------------------------------------------------------------
vtkExodusIIReaderPrivate::ObjectInfoType* vtkExodusIIReaderPrivate::GetUnsortedObjectInfo( int otyp, int k )
{
  int i = this->GetObjectTypeIndexFromObjectType( otyp );
  if ( i < 0 )
  {
    vtkDebugMacro( "Could not find collection of objects with type " << otyp << "." );
    return 0;
  }
  int N = this->GetNumberOfObjectsAtTypeIndex( i );
  if ( k < 0 || k >= N )
  {
    const char* otname = i >= 0 ? objtype_names[i] : "object";
    static_cast<void>(otname); // not referenced warning
    vtkDebugMacro( "You requested " << otname << " " << k << " in a collection of only " << N << " objects." );
    return 0;
  }
  return this->GetObjectInfo( i, k );
}


//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetBlockIndexFromFileGlobalId( int otyp, int refId )
{
  std::vector<BlockInfoType>::iterator bi;
  int i = 0;
  for ( bi = this->BlockInfo[otyp].begin(); bi != this->BlockInfo[otyp].end(); ++bi, ++i )
  {
    if ( refId >= bi->FileOffset && refId <= bi->FileOffset + bi->Size )
      return i;
  }
  return -1;
}

//-----------------------------------------------------------------------------
vtkExodusIIReaderPrivate::BlockInfoType* vtkExodusIIReaderPrivate::GetBlockFromFileGlobalId( int otyp, int refId )
{
  int blk = this->GetBlockIndexFromFileGlobalId( otyp, refId );
  if ( blk >= 0 )
  {
    return &this->BlockInfo[otyp][blk];
  }
  return 0;
}

//-----------------------------------------------------------------------------
vtkIdType vtkExodusIIReaderPrivate::GetSqueezePointId( BlockSetInfoType* bsinfop, int i )
{
  if (i<0)
  {
    vtkGenericWarningMacro("Invalid point id: " << i
      << ". Data file may be incorrect.");
    i = 0;
  }

  vtkIdType x;
  std::map<vtkIdType,vtkIdType>::iterator it = bsinfop->PointMap.find( i );
  if ( it == bsinfop->PointMap.end() )
  { // Nothing found; add a new entry to the map.
    x = bsinfop->NextSqueezePoint ++ ;
    bsinfop->PointMap[i] = x;
    bsinfop->ReversePointMap[x] = i;
  }
  else
  {
    x = it->second;
  }
  return x;
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::DetermineVtkCellType( BlockInfoType& binfo )
{
  vtkStdString elemType( vtksys::SystemTools::UpperCase( binfo.TypeName ) );

  // Check for quadratic elements
  if ((elemType.substr(0,3) == "TRI") &&           (binfo.BdsPerEntry[0] == 6))
    { binfo.CellType=VTK_QUADRATIC_TRIANGLE;       binfo.PointsPerCell = 6; }
  else if ((elemType.substr(0,3) == "SHE") &&      (binfo.BdsPerEntry[0] == 8))
    { binfo.CellType=VTK_QUADRATIC_QUAD;           binfo.PointsPerCell = 8; }
  else if ((elemType.substr(0,3) == "SHE") &&      (binfo.BdsPerEntry[0] == 9))
    { binfo.CellType=VTK_QUADRATIC_QUAD;           binfo.PointsPerCell = 8; }
  else if ((elemType.substr(0,3) == "TET") &&      (binfo.BdsPerEntry[0] == 10))
    { binfo.CellType=VTK_QUADRATIC_TETRA;          binfo.PointsPerCell = 10; }
  else if ((elemType.substr(0,3) == "TET") &&      (binfo.BdsPerEntry[0] == 11))
    { binfo.CellType=VTK_QUADRATIC_TETRA;          binfo.PointsPerCell = 10; }
  else if ((elemType.substr(0,3) == "WED") &&      (binfo.BdsPerEntry[0] == 15))
    { binfo.CellType=VTK_QUADRATIC_WEDGE;          binfo.PointsPerCell = 15; }
  else if ((elemType.substr(0,3) == "HEX") &&      (binfo.BdsPerEntry[0] == 20))
    { binfo.CellType=VTK_QUADRATIC_HEXAHEDRON;     binfo.PointsPerCell = 20; }
  else if ((elemType.substr(0,3) == "HEX") &&      (binfo.BdsPerEntry[0] == 21))
    { binfo.CellType=VTK_QUADRATIC_HEXAHEDRON;     binfo.PointsPerCell = 20; }
  else if ((elemType.substr(0,3) == "HEX") &&      (binfo.BdsPerEntry[0] == 27))
    { binfo.CellType=VTK_TRIQUADRATIC_HEXAHEDRON;  binfo.PointsPerCell = 27; }
  else if ((elemType.substr(0,3) == "QUA") &&      (binfo.BdsPerEntry[0] == 8))
    { binfo.CellType=VTK_QUADRATIC_QUAD;           binfo.PointsPerCell = 8; }
  else if ((elemType.substr(0,3) == "QUA") &&      (binfo.BdsPerEntry[0] == 9))
    { binfo.CellType=VTK_BIQUADRATIC_QUAD;         binfo.PointsPerCell = 9; }
  else if ((elemType.substr(0,3) == "TRU") &&      (binfo.BdsPerEntry[0] == 3))
    { binfo.CellType=VTK_QUADRATIC_EDGE;           binfo.PointsPerCell = 3; }
  else if ((elemType.substr(0,3) == "BEA") &&      (binfo.BdsPerEntry[0] == 3))
    { binfo.CellType=VTK_QUADRATIC_EDGE;           binfo.PointsPerCell = 3; }
  else if ((elemType.substr(0,3) == "BAR") &&      (binfo.BdsPerEntry[0] == 3))
    { binfo.CellType=VTK_QUADRATIC_EDGE;           binfo.PointsPerCell = 3; }
  else if ((elemType.substr(0,3) == "EDG") &&      (binfo.BdsPerEntry[0] == 3))
    { binfo.CellType=VTK_QUADRATIC_EDGE;           binfo.PointsPerCell = 3; }
  else if ((elemType.substr(0,3) == "PYR") &&      (binfo.BdsPerEntry[0] == 13))
    { binfo.CellType=VTK_QUADRATIC_PYRAMID;        binfo.PointsPerCell = 13; }

  // Check for regular elements
  else if (elemType.substr(0,3) == "CIR") { binfo.CellType = VTK_VERTEX;     binfo.PointsPerCell = 1; }
  else if (elemType.substr(0,3) == "SPH") { binfo.CellType = VTK_VERTEX;     binfo.PointsPerCell = 1; }
  else if (elemType.substr(0,3) == "BAR") { binfo.CellType = VTK_LINE;       binfo.PointsPerCell = 2; }
  else if (elemType.substr(0,3) == "TRU") { binfo.CellType = VTK_LINE;       binfo.PointsPerCell = 2; }
  else if (elemType.substr(0,3) == "BEA") { binfo.CellType = VTK_LINE;       binfo.PointsPerCell = 2; }
  else if (elemType.substr(0,3) == "EDG") { binfo.CellType = VTK_LINE;       binfo.PointsPerCell = 2; }
  else if (elemType.substr(0,3) == "TRI") { binfo.CellType = VTK_TRIANGLE;   binfo.PointsPerCell = 3; }
  else if (elemType.substr(0,3) == "QUA") { binfo.CellType = VTK_QUAD;       binfo.PointsPerCell = 4; }
  else if (elemType.substr(0,3) == "TET") { binfo.CellType = VTK_TETRA;      binfo.PointsPerCell = 4; }
  else if (elemType.substr(0,3) == "PYR") { binfo.CellType = VTK_PYRAMID;    binfo.PointsPerCell = 5; }
  else if (elemType.substr(0,3) == "WED") { binfo.CellType = VTK_WEDGE;      binfo.PointsPerCell = 6; }
  else if (elemType.substr(0,3) == "HEX") { binfo.CellType = VTK_HEXAHEDRON; binfo.PointsPerCell = 8; }
  else if (elemType.substr(0,3) == "NSI") { binfo.CellType = VTK_POLYGON;    binfo.PointsPerCell = 0; }
  else if (elemType.substr(0,3) == "NFA") { binfo.CellType = VTK_POLYHEDRON; binfo.PointsPerCell = 0; }
  else if ((elemType.substr(0,3) == "SHE") && (binfo.BdsPerEntry[0] == 3))
    { binfo.CellType = VTK_TRIANGLE;           binfo.PointsPerCell = 3; }
  else if ((elemType.substr(0,3) == "SHE") && (binfo.BdsPerEntry[0] == 4))
    { binfo.CellType = VTK_QUAD;               binfo.PointsPerCell = 4; }
  else if ((elemType.substr(0,8) == "STRAIGHT") && (binfo.BdsPerEntry[0] == 2 ))
    { binfo.CellType = VTK_LINE;                    binfo.PointsPerCell = 2; }
  else if (elemType.substr(0,3) == "SUP")
  {
    binfo.CellType=VTK_POLY_VERTEX;
    binfo.PointsPerCell = binfo.BdsPerEntry[0];
  }
  else if ((elemType.substr(0,4) == "NULL") && (binfo.Size == 0))
  {
    (void)binfo; // silently ignore empty element blocks
  }
  else
  {
    vtkErrorMacro("Unsupported element type: " << elemType.c_str());
  }

  //cell types not currently handled
  //quadratic wedge - 15,16 nodes
  //quadratic pyramid - 13 nodes
}

//-----------------------------------------------------------------------------
vtkExodusIIReaderPrivate::ArrayInfoType* vtkExodusIIReaderPrivate::FindArrayInfoByName( int otyp, const char* name )
{
  std::vector<ArrayInfoType>::iterator ai;
  for ( ai = this->ArrayInfo[otyp].begin(); ai != this->ArrayInfo[otyp].end(); ++ai )
  {
    if ( ai->Name == name )
      return &(*ai);
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::IsObjectTypeBlock( int otyp )
{
  return (otyp == vtkExodusIIReader::ELEM_BLOCK || otyp == vtkExodusIIReader::EDGE_BLOCK || otyp == vtkExodusIIReader::FACE_BLOCK);
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::IsObjectTypeSet( int otyp )
{
  return (otyp == vtkExodusIIReader::ELEM_SET || otyp == vtkExodusIIReader::EDGE_SET || otyp == vtkExodusIIReader::FACE_SET || otyp == vtkExodusIIReader::NODE_SET || otyp == vtkExodusIIReader::SIDE_SET);
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::IsObjectTypeMap( int otyp )
{
  return (otyp == vtkExodusIIReader::ELEM_MAP || otyp == vtkExodusIIReader::EDGE_MAP || otyp == vtkExodusIIReader::FACE_MAP || otyp == vtkExodusIIReader::NODE_MAP);
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetObjectTypeFromMapType( int mtyp )
{
  switch (mtyp)
  {
  case vtkExodusIIReader::ELEM_MAP:
    return vtkExodusIIReader::ELEM_BLOCK;
  case vtkExodusIIReader::FACE_MAP:
    return vtkExodusIIReader::FACE_BLOCK;
  case vtkExodusIIReader::EDGE_MAP:
    return vtkExodusIIReader::EDGE_BLOCK;
  case vtkExodusIIReader::NODE_MAP:
    return vtkExodusIIReader::NODAL;
  }
  return -1;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetMapTypeFromObjectType( int otyp )
{
  switch (otyp)
  {
  case vtkExodusIIReader::ELEM_BLOCK:
    return vtkExodusIIReader::ELEM_MAP;
  case vtkExodusIIReader::FACE_BLOCK:
    return vtkExodusIIReader::FACE_MAP;
  case vtkExodusIIReader::EDGE_BLOCK:
    return vtkExodusIIReader::EDGE_MAP;
  case vtkExodusIIReader::NODAL:
    return vtkExodusIIReader::NODE_MAP;
  }
  return -1;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetTemporalTypeFromObjectType( int otyp )
{
  switch (otyp)
  {
  case vtkExodusIIReader::ELEM_BLOCK:
    return vtkExodusIIReader::ELEM_BLOCK_TEMPORAL;
  //case vtkExodusIIReader::FACE_BLOCK:
  //  return vtkExodusIIReader::FACE_MAP;
  //case vtkExodusIIReader::EDGE_BLOCK:
  //  return vtkExodusIIReader::EDGE_MAP;
  case vtkExodusIIReader::NODAL:
    return vtkExodusIIReader::NODAL_TEMPORAL;
  case vtkExodusIIReader::GLOBAL:
    return vtkExodusIIReader::GLOBAL_TEMPORAL;
  }
  return -1;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetSetTypeFromSetConnType( int sctyp )
{
  switch ( sctyp )
  {
  case vtkExodusIIReader::NODE_SET_CONN:
    return vtkExodusIIReader::NODE_SET;
  case vtkExodusIIReader::EDGE_SET_CONN:
    return vtkExodusIIReader::EDGE_SET;
  case vtkExodusIIReader::FACE_SET_CONN:
    return vtkExodusIIReader::FACE_SET;
  case vtkExodusIIReader::SIDE_SET_CONN:
    return vtkExodusIIReader::SIDE_SET;
  case vtkExodusIIReader::ELEM_SET_CONN:
    return vtkExodusIIReader::ELEM_SET;
  }
  return -1;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetBlockConnTypeFromBlockType( int btyp )
{
  switch ( btyp )
  {
  case vtkExodusIIReader::EDGE_BLOCK:
    return vtkExodusIIReader::EDGE_BLOCK_CONN;
  case vtkExodusIIReader::FACE_BLOCK:
    return vtkExodusIIReader::FACE_BLOCK_CONN;
  case vtkExodusIIReader::ELEM_BLOCK:
    return vtkExodusIIReader::ELEM_BLOCK_ELEM_CONN;
  }
  return -1;
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::RemoveBeginningAndTrailingSpaces( int len, char **names )
{
  int i, j;

  for (i=0; i<len; i++)
  {
    char *c = names[i];
    int nmlen = (int)strlen(c);

    char *cbegin = c;
    char *cend = c + nmlen - 1;

    // remove spaces or non-printing character from start and end

    for (j=0; j<nmlen; j++)
    {
      if (!isgraph(*cbegin)) cbegin++;
      else break;
    }

    for (j=0; j<nmlen; j++)
    {
      if (!isgraph(*cend)) cend--;
      else break;
    }

    if (cend < cbegin)
    {
      sprintf(names[i], "null_%d", i);
      continue;
    }

    int newlen = cend - cbegin + 1;

    if (newlen < nmlen)
    {
      for (j=0; j<newlen; j++)
      {
        *c++ = *cbegin++;
      }
      *c = '\0';
    }
  }
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::ClearConnectivityCaches()
{
  std::map<int,std::vector<BlockInfoType> >::iterator blksit;
  for ( blksit = this->BlockInfo.begin(); blksit != this->BlockInfo.end(); ++ blksit )
  {
    std::vector<BlockInfoType>::iterator blkit;
    for ( blkit = blksit->second.begin(); blkit != blksit->second.end(); ++ blkit )
    {
      if ( blkit->CachedConnectivity )
      {
        blkit->CachedConnectivity->Delete();
        blkit->CachedConnectivity = 0;
      }
    }
  }
  std::map<int,std::vector<SetInfoType> >::iterator setsit;
  for ( setsit = this->SetInfo.begin(); setsit != this->SetInfo.end(); ++ setsit )
  {
    std::vector<SetInfoType>::iterator setit;
    for ( setit = setsit->second.begin(); setit != setsit->second.end(); ++ setit )
    {
      if ( setit->CachedConnectivity )
      {
        setit->CachedConnectivity->Delete();
        setit->CachedConnectivity = 0;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::SetParser(vtkExodusIIReaderParser *parser)
{
  // Properly sets the parser object but does not call Modified.  The parser
  // represents the state of the data in files, not the state of this object.
  if (this->Parser != parser)
  {
    vtkExodusIIReaderParser *oldParser = this->Parser;
    this->Parser = parser;
    if (this->Parser) this->Parser->Register(this);
    if (oldParser) oldParser->UnRegister(this);
  }
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetNumberOfParts()
{
  return static_cast<int>( this->PartInfo.size() );
}

//-----------------------------------------------------------------------------
const char* vtkExodusIIReaderPrivate::GetPartName(int idx)
{
  return this->PartInfo[idx].Name.c_str();
}

//-----------------------------------------------------------------------------
const char* vtkExodusIIReaderPrivate::GetPartBlockInfo(int idx)
{
  char buffer[80];
  vtkStdString blocks;
  std::vector<int> blkIndices = this->PartInfo[idx].BlockIndices;
  for (unsigned int i=0;i<blkIndices.size();i++)
  {
    sprintf(buffer,"%d, ",blkIndices[i]);
    blocks += buffer;
  }

  blocks.erase(blocks.size()-2,blocks.size()-1);

  return blocks.c_str();
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetPartStatus(int idx)
{
  //a part is only active if all its blocks are active
  std::vector<int> blkIndices = this->PartInfo[idx].BlockIndices;
  for (unsigned int i=0;i<blkIndices.size();i++)
  {
    if (!this->GetUnsortedObjectStatus(vtkExodusIIReader::ELEM_BLOCK,blkIndices[i]))
    {
      return 0;
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetPartStatus(vtkStdString name)
{
  for (unsigned int i=0;i<this->PartInfo.size();i++)
  {
    if (this->PartInfo[i].Name==name)
    {
      return this->GetPartStatus(i);
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::SetPartStatus(int idx, int on)
{
  //update the block status for all the blocks in this part
  std::vector<int> blkIndices = this->PartInfo[idx].BlockIndices;
  for (unsigned int i=0;i<blkIndices.size();i++)
  {
    this->SetUnsortedObjectStatus(vtkExodusIIReader::ELEM_BLOCK, blkIndices[i], on);
  }
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::SetPartStatus(vtkStdString name, int flag)
{
  for(unsigned int idx=0; idx<this->PartInfo.size(); ++idx)
  {
    if ( name == this->PartInfo[idx].Name )
    {
      this->SetPartStatus(idx,flag);
      return;
    }
  }
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetNumberOfMaterials()
{
  return static_cast<int>( this->MaterialInfo.size() );
}

//-----------------------------------------------------------------------------
const char* vtkExodusIIReaderPrivate::GetMaterialName(int idx)
{
  return this->MaterialInfo[idx].Name.c_str();
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetMaterialStatus(int idx)
{
  std::vector<int> blkIndices = this->MaterialInfo[idx].BlockIndices;

  for (unsigned int i=0;i<blkIndices.size();i++)
  {
    if (!this->GetUnsortedObjectStatus(vtkExodusIIReader::ELEM_BLOCK,blkIndices[i]))
    {
      return 0;
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetMaterialStatus(vtkStdString name)
{
  for (unsigned int i=0;i<this->MaterialInfo.size();i++)
  {
    if (this->MaterialInfo[i].Name==name)
    {
      return this->GetMaterialStatus(i);
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::SetMaterialStatus(int idx, int on)
{
  //update the block status for all the blocks in this material
  std::vector<int> blkIndices = this->MaterialInfo[idx].BlockIndices;

  for (unsigned int i=0;i<blkIndices.size();i++)
  {
    this->SetUnsortedObjectStatus(vtkExodusIIReader::ELEM_BLOCK,blkIndices[i],on);
  }
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::SetMaterialStatus(vtkStdString name, int flag)
{
  for(unsigned int idx=0; idx<this->MaterialInfo.size(); ++idx)
  {
    if ( name == this->MaterialInfo[idx].Name )
    {
        this->SetMaterialStatus(idx,flag);
        return;
    }
  }
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetNumberOfAssemblies()
{
  return static_cast<int>( this->AssemblyInfo.size() );
}

//-----------------------------------------------------------------------------
const char* vtkExodusIIReaderPrivate::GetAssemblyName(int idx)
{
  return this->AssemblyInfo[idx].Name.c_str();
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetAssemblyStatus(int idx)
{
  std::vector<int> blkIndices = this->AssemblyInfo[idx].BlockIndices;

  for (unsigned int i=0;i<blkIndices.size();i++)
  {
    if (!this->GetUnsortedObjectStatus(vtkExodusIIReader::ELEM_BLOCK,blkIndices[i]))
    {
      return 0;
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::GetAssemblyStatus(vtkStdString name)
{
  for (unsigned int i=0;i<this->AssemblyInfo.size();i++)
  {
    if (this->AssemblyInfo[i].Name==name)
    {
      return this->GetAssemblyStatus(i);
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::SetAssemblyStatus(int idx, int on)
{
  std::vector<int> blkIndices = this->AssemblyInfo[idx].BlockIndices;

  //update the block status for all the blocks in this material
  for (unsigned int i=0;i<blkIndices.size();i++)
  {
    this->SetUnsortedObjectStatus(vtkExodusIIReader::ELEM_BLOCK,blkIndices[i],on);
  }
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::SetAssemblyStatus(vtkStdString name, int flag)
{
  for(unsigned int idx=0; idx<this->AssemblyInfo.size(); ++idx)
  {
    if ( name == this->AssemblyInfo[idx].Name )
    {
      this->SetAssemblyStatus(idx,flag);
      return;
    }
  }
}

//-----------------------------------------------------------------------------
// Normally, this would be below with all the other vtkExodusIIReader member definitions,
// but the Tcl PrintSelf test script is really lame.
void vtkExodusIIReader::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "FileName: " << ( this->FileName ? this->FileName : "(null)" ) << "\n";
  os << indent << "XMLFileName: " << ( this->XMLFileName ? this->XMLFileName : "(null)" ) << "\n";
  os << indent << "DisplayType: " << this->DisplayType << "\n";
  os << indent << "TimeStep: " << this->TimeStep << "\n";
  os << indent << "TimeStepRange: [" << this->TimeStepRange[0] << ", " << this->TimeStepRange[1] << "]\n";
  os << indent << "ModeShapesRange:  [ "
    << this->GetModeShapesRange()[0] << ", " << this->GetModeShapesRange()[1] << "]\n";
  os << indent << "SILUpdateStamp: " << this->SILUpdateStamp << "\n";
  if ( this->Metadata )
  {
    os << indent << "Metadata:\n";
    this->Metadata->PrintData( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "Metadata: (null)\n";
  }
}


void vtkExodusIIReaderPrivate::PrintData( ostream& os, vtkIndent indent )
{
  //this->Superclass::Print Self( os, indent );
  os << indent << "Exoid: " << this->Exoid << "\n";
  os << indent << "AppWordSize: " << this->AppWordSize << "\n";
  os << indent << "DiskWordSize: " << this->DiskWordSize << "\n";
  os << indent << "ExodusVersion: " << this->ExodusVersion << "\n";
  os << indent << "ModelParameters:\n";

  vtkIndent inden2 = indent.GetNextIndent();
  os << inden2 << "Title: " << this->ModelParameters.title << "\n";
  os << inden2 << "Dimension: " << this->ModelParameters.num_dim << "\n";
  os << inden2 << "Nodes: " << this->ModelParameters.num_nodes << "\n";
  os << inden2 << "Edges: " << this->ModelParameters.num_edge << "\n";
  os << inden2 << "Faces: " << this->ModelParameters.num_face << "\n";
  os << inden2 << "Elements: " << this->ModelParameters.num_elem << "\n";
  os << inden2 << "Edge Blocks: " << this->ModelParameters.num_edge_blk << "\n";
  os << inden2 << "Face Blocks: " << this->ModelParameters.num_face_blk << "\n";
  os << inden2 << "Element Blocks: " << this->ModelParameters.num_elem_blk << "\n";
  os << inden2 << "Node Sets: " << this->ModelParameters.num_node_sets << "\n";
  os << inden2 << "Edge Sets: " << this->ModelParameters.num_edge_sets << "\n";
  os << inden2 << "Face Sets: " << this->ModelParameters.num_face_sets << "\n";
  os << inden2 << "Side Sets: " << this->ModelParameters.num_side_sets << "\n";
  os << inden2 << "Element Sets: " << this->ModelParameters.num_elem_sets << "\n";
  os << inden2 << "Node Maps: " << this->ModelParameters.num_node_maps << "\n";
  os << inden2 << "Edge Maps: " << this->ModelParameters.num_edge_maps << "\n";
  os << inden2 << "Face Maps: " << this->ModelParameters.num_face_maps << "\n";
  os << inden2 << "Element Maps: " << this->ModelParameters.num_elem_maps << "\n";

  os << indent << "Time steps (" << this->Times.size() << "):";
  int i;
  for ( i = 0; i < (int)this->Times.size(); ++i )
  {
    os << " " << this->Times[i];
  }
  os << "\n";
  os << indent << "HasModeShapes: " << this->HasModeShapes << "\n";
  os << indent << "ModeShapeTime: " << this->ModeShapeTime << "\n";
  os << indent << "AnimateModeShapes: " << this->AnimateModeShapes << "\n";

  // Print nodal variables
  if ( this->ArrayInfo[ vtkExodusIIReader::NODAL ].size() > 0 )
  {
    os << indent << "Nodal Arrays:\n";
    std::vector<ArrayInfoType>::iterator ai;
    for ( ai = this->ArrayInfo[ vtkExodusIIReader::NODAL ].begin(); ai != this->ArrayInfo[ vtkExodusIIReader::NODAL ].end(); ++ai )
    {
      printArray( os, indent, vtkExodusIIReader::NODAL, *ai );
    }
  }

  // Print blocks
  os << indent << "Blocks:\n";
  std::map<int,std::vector<BlockInfoType> >::iterator bti;
  for ( bti = this->BlockInfo.begin(); bti != this->BlockInfo.end(); ++bti )
  {
    std::vector<BlockInfoType>::iterator bi;
    for ( bi = bti->second.begin(); bi != bti->second.end(); ++bi )
    {
      printBlock( os, indent.GetNextIndent(), bti->first, *bi );
    }
    if ( this->ArrayInfo[ bti->first ].size() > 0 )
    {
      os << indent << "    Results variables:\n";
      std::vector<ArrayInfoType>::iterator ai;
      for ( ai = this->ArrayInfo[ bti->first ].begin(); ai != this->ArrayInfo[ bti->first ].end(); ++ai )
      {
        printArray( os, indent.GetNextIndent(), bti->first, *ai );
      }
    }
  }

  // Print sets
  os << indent << "Sets:\n";
  std::map<int,std::vector<SetInfoType> >::iterator sti;
  for ( sti = this->SetInfo.begin(); sti != this->SetInfo.end(); ++sti )
  {
    std::vector<SetInfoType>::iterator si;
    for ( si = sti->second.begin(); si != sti->second.end(); ++si )
    {
      printSet( os, indent.GetNextIndent(), sti->first, *si );
    }
    if ( this->ArrayInfo[ sti->first ].size() > 0 )
    {
      os << indent << "    Results variables:\n";
      std::vector<ArrayInfoType>::iterator ai;
      for ( ai = this->ArrayInfo[ sti->first ].begin(); ai != this->ArrayInfo[ sti->first ].end(); ++ai )
      {
        printArray( os, indent.GetNextIndent(), sti->first, *ai );
      }
    }
  }

  // Print maps
  os << indent << "Maps:\n";
  std::map<int,std::vector<MapInfoType> >::iterator mti;
  for ( mti = this->MapInfo.begin(); mti != this->MapInfo.end(); ++mti )
  {
    std::vector<MapInfoType>::iterator mi;
    for ( mi = mti->second.begin(); mi != mti->second.end(); ++mi )
    {
      printMap( os, indent.GetNextIndent(), mti->first, *mi );
    }
  }

  os << indent << "Array Cache:\n";
  this->Cache->PrintSelf( os, inden2 );

  os << indent << "SqueezePoints: " << this->SqueezePoints << "\n";
  os << indent << "ApplyDisplacements: " << this->ApplyDisplacements << "\n";
  os << indent << "DisplacementMagnitude: " << this->DisplacementMagnitude << "\n";
  os << indent << "GenerateObjectIdArray: " << this->GenerateObjectIdArray << "\n";
  os << indent << "GenerateFileIdArray: " << this->GenerateFileIdArray << "\n";
  os << indent << "FileId: " << this->FileId << "\n";
}

int vtkExodusIIReaderPrivate::OpenFile( const char* filename )
{
  if ( ! filename || ! strlen( filename ) )
  {
    vtkErrorMacro( "Exodus filename pointer was NULL or pointed to an empty string." );
    return 0;
  }

  if ( this->Exoid >= 0 )
  {
    this->CloseFile();
  }

  this->Exoid = ex_open( filename, EX_READ,
    &this->AppWordSize, &this->DiskWordSize, &this->ExodusVersion );

  if ( this->Exoid <= 0 )
  {
    vtkErrorMacro( "Unable to open \"" << filename << "\" for reading" );
    return 0;
  }

  int numNodesInFile;
  char dummyChar;
  float dummyFloat;
  ex_inquire(this->Exoid, EX_INQ_NODES, &numNodesInFile, &dummyFloat, &dummyChar);

  return 1;
}

int vtkExodusIIReaderPrivate::CloseFile()
{
  if ( this->Exoid >= 0 )
  {
    VTK_EXO_FUNC( ex_close( this->Exoid ), "Could not close an open file (" << this->Exoid << ")" );
    this->Exoid = -1;
  }
  return 0;
}



int vtkExodusIIReaderPrivate::UpdateTimeInformation()
{
  // BUG #15632: For files with spatial partitions, vtkPExodusIIReader uses vtkExodusIIReader
  // to read each of the files. Since time information between those files doesn't change and
  // it can be quite time consuming to collect the time information, vtkPExodusIIReader forcibly passes
  // time information from the first reader to all others. SkipUpdateTimeInformation helps us get that
  // going without significant changes to the reader.
  if (this->SkipUpdateTimeInformation)
  {
    return 0;
  }

  int exoid = this->Exoid;
  int itmp[5];
  int num_timesteps;
  int i;

  VTK_EXO_FUNC( ex_inquire( exoid, EX_INQ_TIME, itmp, 0, 0 ), "Inquire for EX_INQ_TIME failed" );
  num_timesteps = itmp[0];

  this->Times.clear();
  if ( num_timesteps > 0 )
  {
    this->Times.resize( num_timesteps );

    int exo_err = ex_get_all_times( this->Exoid, &this->Times[0] );
    if ( exo_err < 0)
    {
      for ( i = 0; i < num_timesteps; ++i )
      {
          this->Times[i] = i;
      }
      vtkWarningMacro("Could not retrieve time values, assuming times equal to timesteps");
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkExodusIIReaderPrivate::BuildSIL()
{
  // Initialize the SIL, dump all previous information.
  this->SIL->Initialize();
  if (this->Parser)
  {
    // The parser has built the SIL for us,
    // use that.
    this->SIL->ShallowCopy(this->Parser->GetSIL());
    return;
  }

  // Else build a minimal SIL with only the blocks.
  vtkSmartPointer<vtkVariantArray> childEdge =
    vtkSmartPointer<vtkVariantArray>::New();
  childEdge->InsertNextValue(0);

  vtkSmartPointer<vtkVariantArray> crossEdge =
    vtkSmartPointer<vtkVariantArray>::New();
  crossEdge->InsertNextValue(0);

  // CrossEdge is an edge linking hierarchies.
  vtkUnsignedCharArray* crossEdgesArray = vtkUnsignedCharArray::New();
  crossEdgesArray->SetName("CrossEdges");
  this->SIL->GetEdgeData()->AddArray(crossEdgesArray);
  crossEdgesArray->Delete();

  std::deque<std::string> names;
  int cc;

  // Now build the hierarchy.
  vtkIdType rootId = this->SIL->AddVertex();
  names.push_back("SIL");

  // Add the ELEM_BLOCK subtree.
  vtkIdType blocksRoot = this->SIL->AddChild(rootId, childEdge);
  names.push_back("Blocks");

  // Add the assembly subtree
  this->SIL->AddChild(rootId, childEdge);
  names.push_back("Assemblies");

  // Add the materials subtree
  this->SIL->AddChild(rootId, childEdge);
  names.push_back("Materials");

  // This is the map of block names to node ids.
  std::map<std::string, vtkIdType> blockids;
  int numBlocks = this->GetNumberOfObjectsOfType(vtkExodusIIReader::ELEM_BLOCK);
  for (cc=0; cc < numBlocks; cc++)
  {
    vtkIdType child = this->SIL->AddChild(blocksRoot, childEdge);
    std::string block_name =
      this->GetObjectName(vtkExodusIIReader::ELEM_BLOCK, cc);
    names.push_back(block_name);
    blockids[block_name] = child;
  }

  // This array is used to assign names to nodes.
  vtkStringArray* namesArray = vtkStringArray::New();
  namesArray->SetName("Names");
  namesArray->SetNumberOfTuples(this->SIL->GetNumberOfVertices());
  this->SIL->GetVertexData()->AddArray(namesArray);
  namesArray->Delete();

  std::deque<std::string>::iterator iter;
  for (cc=0, iter = names.begin(); iter != names.end(); ++iter, ++cc)
  {
    namesArray->SetValue(cc, (*iter).c_str());
  }
}

//-----------------------------------------------------------------------------
int vtkExodusIIReaderPrivate::RequestInformation()
{
  int exoid = this->Exoid;
  //int itmp[5];
  int* ids;
  int nids;
  int obj;
  int i, j;
  int num_timesteps;
  char** obj_names;
  char** obj_typenames = 0;
  char** var_names = 0;
  int have_var_names;
  int num_vars = 0; /* number of variables per object */
  char tmpName[256];
  tmpName[255] = '\0';

  this->InformationTimeStamp.Modified(); // Update MTime so that it will be newer than parent's FileNameMTime

  VTK_EXO_FUNC( ex_get_init_ext( exoid, &this->ModelParameters ),
    "Unable to read database parameters." );

  VTK_EXO_FUNC( this->UpdateTimeInformation(), "" );

  //VTK_EXO_FUNC( ex_inquire( exoid, EX_INQ_TIME,       itmp, 0, 0 ), "Inquire for EX_INQ_TIME failed" );
  //num_timesteps = itmp[0];

  num_timesteps = static_cast<int>( this->Times.size() );
/*
  this->Times.clear();
  if ( num_timesteps > 0 )
    {
    this->Times.resize( num_timesteps );
    VTK_EXO_FUNC( ex_get_all_times( this->Exoid, &this->Times[0] ), "Could not retrieve time values." );
    }
*/
  for ( i = 0; i < num_obj_types; ++i )
  {
    if(OBJTYPE_IS_NODAL(i))
    {
      continue;
    }

    vtkIdType blockEntryFileOffset = 1;
    vtkIdType setEntryFileOffset = 1;

    std::map<int,int> sortedObjects;

    int* truth_tab = 0;
    have_var_names = 0;

    VTK_EXO_FUNC( ex_inquire( exoid, obj_sizes[i], &nids, 0, 0 ), "Object ID list size could not be determined." );

    if ( nids )
    {
      ids = (int*) malloc( nids * sizeof(int) );
      obj_names = (char**) malloc( nids * sizeof(char*) );
      for ( obj = 0; obj < nids; ++obj )
        obj_names[obj] = (char*) malloc( (MAX_STR_LENGTH + 1) * sizeof(char) );
      if ( OBJTYPE_IS_BLOCK(i) )
      {
        obj_typenames = (char**) malloc( nids * sizeof(char*) );
        for ( obj = 0; obj < nids; ++obj )
        {
          obj_typenames[obj] = (char*) malloc( (MAX_STR_LENGTH + 1) * sizeof(char) );
          obj_typenames[obj][0] = '\0';
        }
      }
    }
    else
    {
      ids = 0;
      obj_names = 0;
      obj_typenames = 0;
    }

    if ( nids == 0 && ! OBJTYPE_IS_MAP(i) )
      continue;

    if ( nids )
    {
      VTK_EXO_FUNC( ex_get_ids( exoid, static_cast<ex_entity_type>( obj_types[i] ), ids ),
        "Could not read object ids for i=" << i << " and otyp=" << obj_types[i] << "." );
      VTK_EXO_FUNC( ex_get_names( exoid, static_cast<ex_entity_type>( obj_types[i] ), obj_names ),
        "Could not read object names." );
    }

    BlockInfoType binfo;
    SetInfoType sinfo;
    MapInfoType minfo;

    if ( OBJTYPE_IS_BLOCK(i) )
    {
      this->BlockInfo[obj_types[i]].clear();
      this->BlockInfo[obj_types[i]].reserve( nids );
    }
    else if ( OBJTYPE_IS_SET(i) )
    {
      this->SetInfo[obj_types[i]].clear();
      this->SetInfo[obj_types[i]].reserve( nids );
    }
    else
    {
      this->MapInfo[obj_types[i]].clear();
      this->MapInfo[obj_types[i]].reserve( nids );
    }

    if ( (OBJTYPE_IS_BLOCK(i)) || (OBJTYPE_IS_SET(i)))
    {
      VTK_EXO_FUNC( ex_get_var_param( exoid, obj_typestr[i], &num_vars ), "Could not read number of variables." );

      if (num_vars && num_timesteps > 0)
      {
        truth_tab = (int*) malloc(num_vars * nids * sizeof(int));
        VTK_EXO_FUNC( ex_get_var_tab( exoid, obj_typestr[i], nids, num_vars, truth_tab ), "Could not read truth table." );

        var_names = (char**) malloc(num_vars * sizeof(char*));
        for (j = 0; j < num_vars; ++j)
          var_names[j] = (char*) malloc( (MAX_STR_LENGTH + 1) * sizeof(char));

        VTK_EXO_FUNC( ex_get_var_names( exoid, obj_typestr[i], num_vars, var_names ), "Could not read variable names." );
        this->RemoveBeginningAndTrailingSpaces(num_vars, var_names);
        have_var_names = 1;
      }
    }

    if ( !have_var_names)
      var_names = 0;

    for (obj = 0; obj < nids; ++obj)
    {

      if ( OBJTYPE_IS_BLOCK(i))
      {

        binfo.Name = obj_names[obj];
        binfo.Id = ids[obj];
        binfo.CachedConnectivity = 0;
        binfo.NextSqueezePoint = 0;
        if (obj_types[i] == vtkExodusIIReader::ELEM_BLOCK)
        {
          VTK_EXO_FUNC( ex_get_block( exoid, static_cast<ex_entity_type>( obj_types[i] ), ids[obj], obj_typenames[obj],
                  &binfo.Size, &binfo.BdsPerEntry[0], &binfo.BdsPerEntry[1], &binfo.BdsPerEntry[2], &binfo.AttributesPerEntry ),
              "Could not read block params." );
          binfo.Status = 1; // load element blocks by default
          binfo.TypeName = obj_typenames[obj];
        }
        else
        {
          VTK_EXO_FUNC( ex_get_block( exoid, static_cast<ex_entity_type>( obj_types[i] ), ids[obj], obj_typenames[obj],
                  &binfo.Size, &binfo.BdsPerEntry[0], &binfo.BdsPerEntry[1], &binfo.BdsPerEntry[2], &binfo.AttributesPerEntry ),
              "Could not read block params." );
          binfo.Status = 0; // don't load edge/face blocks by default
          binfo.TypeName = obj_typenames[obj];
          binfo.BdsPerEntry[1] = binfo.BdsPerEntry[2] = 0;
        }
        this->GetInitialObjectStatus(obj_types[i], &binfo);
        //num_entries = binfo.Size;
        binfo.FileOffset = blockEntryFileOffset;
        blockEntryFileOffset += binfo.Size;
        if (binfo.Name.length() == 0)
        {
          snprintf( tmpName, 255, "Unnamed block ID: %d Type: %s",
              ids[obj], binfo.TypeName.length() ? binfo.TypeName.c_str() : "NULL");
          binfo.Name = tmpName;
        }
        binfo.OriginalName = binfo.Name;
        this->DetermineVtkCellType(binfo);

        if (binfo.AttributesPerEntry)
        {
          char** attr_names;
          attr_names
              = (char**) malloc(binfo.AttributesPerEntry * sizeof(char*));
          for (j = 0; j < binfo.AttributesPerEntry; ++j)
            attr_names[j]
                = (char*) malloc( (MAX_STR_LENGTH + 1) * sizeof(char));

          VTK_EXO_FUNC( ex_get_attr_names( exoid, static_cast<ex_entity_type>( obj_types[i] ), ids[obj], attr_names ),
            "Could not read attributes names." );

          for (j = 0; j < binfo.AttributesPerEntry; ++j)
          {
            binfo.AttributeNames.push_back(attr_names[j]);
            binfo.AttributeStatus.push_back( 0); // don't load attributes by default
          }

          for (j = 0; j < binfo.AttributesPerEntry; ++j)
            free(attr_names[j]);
          free(attr_names);
        }

        // Check to see if there is metadata that defines what part, material,
        //  and assembly(ies) this block belongs to.

        if (this->Parser && this->Parser->HasInformationAboutBlock(binfo.Id))
        {
          // Update the block name using the XML.
          binfo.Name = this->Parser->GetBlockName(binfo.Id);

          // int blockIdx = static_cast<int>( this->BlockInfo[obj_types[i]].size() );
          //// Add this block to our parts, materials, and assemblies collections
          //unsigned int k;
          //int found = 0;

          //// Look to see if this part has already been created
          //for (k=0;k<this->PartInfo.size();k++)
          //  {
          //  if (this->PartInfo[k].Name==partName)
          //    {
          //    //binfo.PartId = k;
          //    this->PartInfo[k].BlockIndices.push_back(blockIdx);
          //    found=1;
          //    }
          //  }

          //if (!found)
          //  {
          //  PartInfoType pinfo;
          //  pinfo.Name = partName;
          //  pinfo.Id = static_cast<int>( this->PartInfo.size() );
          //  //binfo.PartId = k;
          //  pinfo.BlockIndices.push_back(blockIdx);
          //  this->PartInfo.push_back(pinfo);
          //  }
          //
          //found=0;
          //for (k=0;k<this->MaterialInfo.size();k++)
          //  {
          //  if (this->MaterialInfo[k].Name==materialName)
          //    {
          //    //binfo.MaterialId = k;
          //    this->MaterialInfo[k].BlockIndices.push_back(blockIdx);
          //    found=1;
          //    }
          //  }
          //if (!found)
          //  {
          //  MaterialInfoType matinfo;
          //  matinfo.Name = materialName;
          //  matinfo.Id = static_cast<int>( this->MaterialInfo.size() );
          //  //binfo.MaterialId = k;
          //  matinfo.BlockIndices.push_back(blockIdx);
          //  this->MaterialInfo.push_back(matinfo);
          //  }
          //
          //for (k=0;k<localAssemblyNames.size();k++)
          //  {
          //  vtkStdString assemblyName=localAssemblyNames[k];
          //  found=0;
          //  for (unsigned int n=0;n<this->AssemblyInfo.size();n++)
          //    {
          //    if (this->AssemblyInfo[n].Name==assemblyName){
          //      //binfo.AssemblyIds.push_back(j);
          //      this->AssemblyInfo[n].BlockIndices.push_back(blockIdx);
          //      found=1;
          //      }
          //    }
          //  if (!found)
          //    {
          //    AssemblyInfoType ainfo;
          //    ainfo.Name = assemblyName;
          //    ainfo.Id = static_cast<int>( this->AssemblyInfo.size() );
          //    //binfo.AssemblyIds.push_back(k);
          //    ainfo.BlockIndices.push_back(blockIdx);
          //    this->AssemblyInfo.push_back(ainfo);
          //    }
          //  }
        }

        sortedObjects[binfo.Id]
            = static_cast<int>(this->BlockInfo[obj_types[i]].size());
        this->BlockInfo[obj_types[i]].push_back(binfo);

      }
      else if ( OBJTYPE_IS_SET(i))
      {

        sinfo.Name = obj_names[obj];
        sinfo.Status = 0;
        sinfo.Id = ids[obj];
        sinfo.CachedConnectivity = 0;
        sinfo.NextSqueezePoint = 0;

        VTK_EXO_FUNC( ex_get_set_param( exoid, static_cast<ex_entity_type>( obj_types[i] ), ids[obj], &sinfo.Size, &sinfo.DistFact),
            "Could not read set parameters." );
        //num_entries = sinfo.Size;
        sinfo.FileOffset = setEntryFileOffset;
        setEntryFileOffset += sinfo.Size;
        this->GetInitialObjectStatus(obj_types[i], &sinfo);
        if (sinfo.Name.length() == 0)
        {
          snprintf( tmpName, 255, "Unnamed set ID: %d", ids[obj]);
          sinfo.Name = tmpName;
        }
        sortedObjects[sinfo.Id] = (int) this->SetInfo[obj_types[i]].size();
        this->SetInfo[obj_types[i]].push_back(sinfo);

      }
      else
      { /* object is map */

        minfo.Id = ids[obj];
        minfo.Status = obj == 0 ? 1 : 0; // only load the first map by default
        switch (obj_types[i])
        {
          case vtkExodusIIReader::NODE_MAP:
            minfo.Size = this->ModelParameters.num_nodes;
            break;
          case vtkExodusIIReader::EDGE_MAP:
            minfo.Size = this->ModelParameters.num_edge;
            break;
          case vtkExodusIIReader::FACE_MAP:
            minfo.Size = this->ModelParameters.num_face;
            break;
          case vtkExodusIIReader::ELEM_MAP:
            minfo.Size = this->ModelParameters.num_elem;
            break;
          default:
            minfo.Size = 0;
        }
        minfo.Name = obj_names[obj];
        if (minfo.Name.length() == 0)
        { // make up a name. FIXME: Possible buffer overflow w/ sprintf
          snprintf( tmpName, 255, "Unnamed map ID: %d", ids[obj] );
          minfo.Name = tmpName;
        }
        sortedObjects[minfo.Id] = (int) this->MapInfo[obj_types[i]].size();
        this->MapInfo[obj_types[i]].push_back(minfo);

      }

    } // end of loop over all object ids

    // Now that we have all objects of that type in the sortedObjects, we can
    // iterate over it to fill in the SortedObjectIndices (the map is a *sorted*
    // associative container)
    std::map<int,int>::iterator soit;
    for ( soit = sortedObjects.begin(); soit != sortedObjects.end(); ++soit )
    {
      this->SortedObjectIndices[obj_types[i]].push_back( soit->second );
    }

    if ( ((OBJTYPE_IS_BLOCK(i)) || (OBJTYPE_IS_SET(i))) && num_vars && num_timesteps > 0 )
    {
      this->ArrayInfo[obj_types[i]].clear();
      // Fill in ArrayInfo entries, combining array names into vectors/tensors where appropriate:
      this->GlomArrayNames( obj_types[i], nids, num_vars, var_names, truth_tab );
    }

    if ( var_names )
    {
      for ( j = 0; j < num_vars; ++j )
        free( var_names[j] );
      free( var_names );
    }
    if ( truth_tab )
      free( truth_tab );

    if ( nids )
    {
      free( ids );

      for ( obj = 0; obj < nids; ++obj )
        free( obj_names[obj] );
      free( obj_names );

      if ( OBJTYPE_IS_BLOCK(i) )
      {
        for ( obj = 0; obj < nids; ++obj )
          free( obj_typenames[obj] );
        free( obj_typenames );
      }
    }

  } // end of loop over all object types
  //this->ComputeGridOffsets();

  // Now read information for nodal arrays
  VTK_EXO_FUNC( ex_get_var_param( exoid, "n", &num_vars ), "Unable to read number of nodal variables." );
  if ( num_vars > 0 )
  {
    var_names = (char**) malloc( num_vars * sizeof(char*) );
    for ( j = 0; j < num_vars; ++j )
      var_names[j] = (char*) malloc( (MAX_STR_LENGTH + 1) * sizeof(char) );

    VTK_EXO_FUNC( ex_get_var_names( exoid, "n", num_vars, var_names ), "Could not read nodal variable names." );
    this->RemoveBeginningAndTrailingSpaces( num_vars, var_names );

    nids = 1;
    std::vector<int> dummy_truth;
    dummy_truth.reserve( num_vars );
    for ( j = 0; j < num_vars; ++j )
    {
      dummy_truth.push_back( 1 );
    }

    this->GlomArrayNames( vtkExodusIIReader::NODAL, nids, num_vars, var_names, &dummy_truth[0] );

    for ( j = 0; j < num_vars; ++j )
    {
      free( var_names[j] );
    }
    free( var_names );
    var_names = 0;
  }

  // Now read information for global variables
  VTK_EXO_FUNC( ex_get_var_param( exoid, "g", &num_vars ), "Unable to read number of global variables." );
  if ( num_vars > 0 )
  {
    var_names = (char**) malloc( num_vars * sizeof(char*) );
    for ( j = 0; j < num_vars; ++j )
      var_names[j] = (char*) malloc( (MAX_STR_LENGTH + 1) * sizeof(char) );

    VTK_EXO_FUNC( ex_get_var_names( exoid, "g", num_vars, var_names ), "Could not read global variable names." );
    this->RemoveBeginningAndTrailingSpaces( num_vars, var_names );

    nids = 1;
    std::vector<int> dummy_truth;
    dummy_truth.reserve( num_vars );
    for ( j = 0; j < num_vars; ++j )
    {
      dummy_truth.push_back( 1 );
    }

    this->GlomArrayNames( vtkExodusIIReader::GLOBAL, nids, num_vars, var_names, &dummy_truth[0] );

    for ( j = 0; j < num_vars; ++j )
    {
      free( var_names[j] );
    }
    free( var_names );
    var_names = 0;
  }

  return 0;
}

int vtkExodusIIReaderPrivate::RequestData( vtkIdType timeStep, vtkMultiBlockDataSet* output )
{
  // The work done here depends on several conditions:
  // - Has connectivity changed (i.e., has block/set status changed)?
  //   - If so, AND if point "squeeze" turned on, must reload points and re-squeeze.
  //   - If so, must re-assemble all arrays
  //   - Must recreate block/set id array.
  // - Has requested time changed?
  //   - If so, AND if "deflect mesh" turned on, must load new deflections and compute new points.
  //   - If so, must assemble all time-varying arrays for new time.
  // - Has array status changed?
  //   - If so, must delete old and/or load new arrays.
  // Obviously, many of these tasks overlap. For instance, it would be
  // foolish to re-assemble all the arrays when the connectivity has
  // changed and then toss them out in order to load arrays for a
  // different time step.

  // Caching strategy: use GLOBAL "object type" for assembled arrays.
  // If connectivity hasn't changed, then these arrays can be used;
  // otherwise, "raw" arrays must be used.
  // Pro:
  //   - single cache == easier bookkeeping (two caches would require us to decide how to equitably split avail mem between them)
  //   - many different operations are accelerated:
  //     - just changing which variables are loaded
  //     - changing which blocks are in output (doesn't require disk access if cache hit)
  //     - possible extension to single-node/cell over time
  // Con:
  //   - higher memory consumption for caching the same set of arrays (or, holding cache size fixed: fewer arrays fit)

  if ( ! output )
  {
    vtkErrorMacro( "You must specify an output mesh" );
  }

  // Iterate over all block and set types, creating a
  // multiblock dataset to hold objects of each type.
  int conntypidx;
  int nbl = 0;
  output->SetNumberOfBlocks( num_conn_types );
  for ( conntypidx = 0; conntypidx < num_conn_types; ++conntypidx )
  {
    int otypidx = conn_obj_idx_cvt[conntypidx];
    int otyp = obj_types[otypidx];
    // Loop over all blocks/sets of this type
    int numObj = this->GetNumberOfObjectsOfType( otyp );
    vtkMultiBlockDataSet* mbds;
    mbds = vtkMultiBlockDataSet::New();
    mbds->SetNumberOfBlocks( numObj );
    output->SetBlock( conntypidx, mbds );
    output->GetMetaData(conntypidx)->Set(vtkCompositeDataSet::NAME(),
      conn_types_names[conntypidx]);
    mbds->FastDelete();
    //cout << "++ Block: " << mbds << " ObjectType: " << otyp << "\n";
    int obj;
    int sortIdx;
    for ( sortIdx = 0; sortIdx < numObj; ++sortIdx )
    {
      const char* object_name = this->GetObjectName(otyp, sortIdx);

      // Preserve the "sorted" order when concatenating
      obj = this->SortedObjectIndices[otyp][sortIdx];
      BlockSetInfoType* bsinfop = static_cast<BlockSetInfoType*>( this->GetObjectInfo( otypidx, obj ) );
      //cout << ( bsinfop->Status ? "++" : "--" ) << "   ObjectId: " << bsinfop->Id;
      if ( ! bsinfop->Status )
      {
        mbds->SetBlock( sortIdx, 0 );
        if (object_name)
        {
          mbds->GetMetaData(sortIdx)->Set(vtkCompositeDataSet::NAME(), object_name);
        }
        continue;
      }
      vtkUnstructuredGrid* ug = vtkUnstructuredGrid::New();
      mbds->SetBlock( sortIdx, ug );
      if (object_name)
      {
        mbds->GetMetaData(sortIdx)->Set(vtkCompositeDataSet::NAME(), object_name);
      }
      ug->FastDelete();
      //cout << " Grid: " << ug << "\n";

      // Connectivity first. Either from the cache in bsinfop or read from disk.
      // Connectivity isn't allowed to change with time.
      this->AssembleOutputConnectivity( timeStep, otyp, obj, conntypidx, bsinfop, ug );

      // Now prepare points.
      // These shouldn't change unless the connectivity has changed.
      this->AssembleOutputPoints( timeStep, bsinfop, ug );

      // Then, add the desired arrays from cache (or disk)
      // Point and cell arrays are handled differently because they
      // have different problems to solve.
      // Point arrays must use the PointMap index to subset values.
      // Cell arrays may be used as-is.
      this->AssembleOutputPointArrays( timeStep, bsinfop, ug );
      this->AssembleOutputCellArrays( timeStep, otyp, obj, bsinfop, ug );

      // Some arrays may be procedurally generated (e.g., the ObjectId
      // array, global element and node number arrays). This constructs
      // them as required.
      this->AssembleOutputProceduralArrays( timeStep, otyp, obj, ug );

      // QA and informational records in the ExodusII file are appended
      // to each and every output unstructured grid.
      this->AssembleOutputGlobalArrays( timeStep, otyp, obj, bsinfop, ug );

      // Maps (as distinct from the global element and node arrays above)
      // are per-cell or per-node integers. As with point arrays, the
      // PointMap is used to subset node maps. Cell arrays are stored in
      // ExodusII files for all elements (across all blocks of a given type)
      // and thus must be subset for the unstructured grid of interest.
      this->AssembleOutputPointMaps( timeStep, bsinfop, ug );
      this->AssembleOutputCellMaps( timeStep, otyp, obj, bsinfop, ug );
      ++nbl;
    }
  }

  this->CloseFile();

  return 0;
}

int vtkExodusIIReaderPrivate::SetUpEmptyGrid( vtkMultiBlockDataSet* output )
{
  if ( ! output )
  {
    vtkErrorMacro( "You must specify an output mesh" );
  }

  // Iterate over all block and set types, creating a
  // multiblock dataset to hold objects of each type.
  int conntypidx;
  int nbl = 0;
  output->SetNumberOfBlocks( num_conn_types );
  for ( conntypidx = 0; conntypidx < num_conn_types; ++conntypidx )
  {
    int otypidx = conn_obj_idx_cvt[conntypidx];
    int otyp = obj_types[otypidx];
    // Loop over all blocks/sets of this type
    int numObj = this->GetNumberOfObjectsOfType( otyp );
    vtkMultiBlockDataSet* mbds;
    mbds = vtkMultiBlockDataSet::New();
    mbds->SetNumberOfBlocks( numObj );
    output->SetBlock( conntypidx, mbds );
    output->GetMetaData(conntypidx)->Set(vtkCompositeDataSet::NAME(),
      conn_types_names[conntypidx]);
    mbds->FastDelete();
    int obj;
    int sortIdx;
    for ( sortIdx = 0; sortIdx < numObj; ++sortIdx )
    {
      // Preserve the "sorted" order when concatenating
      obj = this->SortedObjectIndices[otyp][sortIdx];
      BlockSetInfoType* bsinfop = static_cast<BlockSetInfoType*>( this->GetObjectInfo( otypidx, obj ) );
      //cout << ( bsinfop->Status ? "++" : "--" ) << "   ObjectId: " << bsinfop->Id;
      if ( ! bsinfop->Status )
      {
        //cout << "\n";
        mbds->SetBlock( sortIdx, 0 );
        continue;
      }
      vtkUnstructuredGrid* ug = vtkUnstructuredGrid::New();
      mbds->SetBlock( sortIdx, ug );
      ug->FastDelete();
      ++nbl;
    }
  }
#if 0
  int idx;
  vtkUnstructuredGrid* leaf;

  // Set up an empty unstructured grid
  leaf->Allocate(0);

  // Create new points
  vtkPoints* newPoints = vtkPoints::New();
  newPoints->SetNumberOfPoints( 0 );
  leaf->SetPoints( newPoints );
  newPoints->Delete();
  newPoints = NULL;

  // Create point and cell arrays
  int typ;
  for ( typ = 0; typ < numObjResultTypes; ++typ )
  {
    int otyp = objResultTypes[typ];
    int nObjArr = this->GetNumberOfObjectArrays( otyp );
    for ( idx = 0; idx < nObjArr; ++idx )
    {
      vtkDoubleArray* da = vtkDoubleArray::New();
      da->SetName( this->GetObjectArrayName( otyp, idx ) );
      da->SetNumberOfComponents( this->GetNumberOfObjectArrayComponents( otyp, idx ) );
      if ( otyp == vtkExodusIIReader::NODAL )
      {
        leaf->GetPointData()->AddArray( da );
      }
      else
      {
        leaf->GetCellData()->AddArray( da );
      }
      da->FastDelete();
    }
  }

  for ( typ = 0; typ < numObjAttribTypes; ++typ )
  {
    int otyp = objAttribTypes[typ];
    int nObj = this->GetNumberOfObjects( otyp );
    for ( idx = 0; idx < nObj; ++ idx )
    {
      // Attributes are defined per block, not per block type.
      int nObjAtt = this->GetNumberOfObjectAttributes( otyp, idx );
      for ( int aidx = 0; aidx < nObjAtt; ++ aidx )
      {
        vtkDoubleArray* da = vtkDoubleArray::New();
        da->SetName( this->GetObjectAttributeName( otyp, idx, aidx ) );
        da->SetNumberOfComponents( 1 );
        // All attributes are cell data
        leaf->GetCellData()->AddArray( da );
        da->FastDelete();
      }
    }
  }

  if ( this->GetGenerateObjectIdCellArray() )
  {
    vtkIntArray* ia = vtkIntArray::New();
    ia->SetName( this->GetObjectIdArrayName() );
    ia->SetNumberOfComponents( 1 );
    leaf->GetCellData()->AddArray( ia );
    ia->FastDelete();
  }

  if ( this->GetGenerateGlobalNodeIdArray() )
  {
    vtkIntArray* ia = vtkIntArray::New();
    ia->SetName( this->GetGlobalNodeIdArrayName() );
    ia->SetNumberOfComponents( 1 );
    leaf->GetPointData()->AddArray( ia );
    ia->FastDelete();
  }

  if ( this->GetGenerateGlobalElementIdArray() )
  {
    vtkIntArray* ia = vtkIntArray::New();
    ia->SetName( this->GetGlobalElementIdArrayName() );
    ia->SetNumberOfComponents( 1 );
    leaf->GetCellData()->AddArray( ia );
    ia->FastDelete();
  }
#endif // 0
  return 1;
}

void vtkExodusIIReaderPrivate::Reset()
{
  this->CloseFile();
  this->ResetCache(); // must come before BlockInfo and SetInfo are cleared.
  this->BlockInfo.clear();
  this->SetInfo.clear();
  this->MapInfo.clear();
  this->PartInfo.clear();
  this->MaterialInfo.clear();
  this->AssemblyInfo.clear();
  this->SortedObjectIndices.clear();
  this->ArrayInfo.clear();
  this->ExodusVersion = -1.;
  this->Times.clear();
  memset( (void*)&this->ModelParameters, 0, sizeof(this->ModelParameters) );

  // Don't clear file id since it's not part of meta-data that's read from the
  // file, it's set externally (by vtkPExodusIIReader).
  // Refer to BUG #7633.
  //this->FileId = 0;

  this->Modified();
}

void vtkExodusIIReaderPrivate::ResetSettings()
{
  this->GenerateGlobalElementIdArray = 0;
  this->GenerateGlobalNodeIdArray = 0;
  this->GenerateImplicitElementIdArray = 0;
  this->GenerateImplicitNodeIdArray = 0;
  this->GenerateGlobalIdArray = 0;
  this->GenerateObjectIdArray = 1;
  this->GenerateFileIdArray = 0;

  this->ApplyDisplacements = 1;
  this->DisplacementMagnitude = 1.;

  this->HasModeShapes = 0;
  this->ModeShapeTime = -1.;
  this->AnimateModeShapes = 1;

  this->SqueezePoints = 1;

  this->InitialArrayInfo.clear();
  this->InitialObjectInfo.clear();
}

void vtkExodusIIReaderPrivate::ResetCache()
{
  this->Cache->Clear();
  this->Cache->SetCacheCapacity(this->CacheSize); // FIXME: Perhaps Cache should have a Reset and a Clear method?
  this->ClearConnectivityCaches();
}

void vtkExodusIIReaderPrivate::SetCacheSize( double size )
{
  if (this->CacheSize != size)
  {
    this->CacheSize = size;
    this->Cache->SetCacheCapacity(this->CacheSize);
    this->Modified();
  }
}

bool vtkExodusIIReaderPrivate::IsXMLMetadataValid()
{
  // Make sure that each block id referred to in the metadata arrays exist
  // in the data

  std::set<int> blockIdsFromXml;
  this->Parser->GetBlockIds(blockIdsFromXml);
  std::vector<BlockInfoType> blocksFromData = this->BlockInfo[vtkExodusIIReader::ELEM_BLOCK];
  std::vector<BlockInfoType>::iterator iter2;
  std::set<int>::iterator iter;
  bool isBlockValid = false;
  for(iter = blockIdsFromXml.begin(); iter!=blockIdsFromXml.end(); ++iter)
  {
    isBlockValid = false;
    for(iter2 = blocksFromData.begin(); iter2!=blocksFromData.end(); ++iter2)
    {
      if(*iter == (*iter2).Id)
      {
        isBlockValid = true;
        break;
      }
    }
    if(!isBlockValid)
    {
      break;
    }
  }

  return isBlockValid;
}

void vtkExodusIIReaderPrivate::SetSqueezePoints( int sp )
{
  if ( this->SqueezePoints == sp )
    return;

  this->SqueezePoints = sp;
  this->Modified();

  // Invalidate global "topology" cache
  // The point maps should be invalidated
  // FIXME: bsinfop->NextSqueezePoint = 0 for all bsinfop
  // FIXME: bsinfop->CachedConnectivity = 0 for all bsinfop
  // FIXME: bsinfop->PointMap.clear() for all bsinfop
  // FIXME: bsinfop->ReversePointMap.clear() for all bsinfop
}

int vtkExodusIIReaderPrivate::GetNumberOfNodes()
{
  return this->ModelParameters.num_nodes;
}

int vtkExodusIIReaderPrivate::GetNumberOfObjectsOfType( int otyp )
{
  int i = this->GetObjectTypeIndexFromObjectType( otyp );
  if ( i < 0 )
  {
    // Could signal warning here, but might not want it if file simply doesn't have objects of some obscure type (e.g., edge sets)
    return 0;
  }
  return this->GetNumberOfObjectsAtTypeIndex( i );
}

int vtkExodusIIReaderPrivate::GetNumberOfObjectArraysOfType( int otyp )
{
  std::map<int,std::vector<ArrayInfoType> >::iterator it = this->ArrayInfo.find( otyp );
  if ( it != this->ArrayInfo.end() )
  {
    return (int) it->second.size();
  }
  // Could signal warning here, but might not want it if file simply doesn't have objects of some obscure type (e.g., edge sets)
  return 0;
}

const char* vtkExodusIIReaderPrivate::GetObjectName( int otyp, int k )
{
  ObjectInfoType* oinfop = this->GetSortedObjectInfo( otyp, k );
  return oinfop ? oinfop->Name.c_str() : 0;
}

int vtkExodusIIReaderPrivate::GetObjectId( int otyp, int k )
{
  ObjectInfoType* oinfop = this->GetSortedObjectInfo( otyp, k );
  return oinfop ? oinfop->Id : -1;
}

int vtkExodusIIReaderPrivate::GetObjectSize( int otyp, int k )
{
  ObjectInfoType* oinfop = this->GetSortedObjectInfo( otyp, k );
  return oinfop ? oinfop->Size : 0;
}

int vtkExodusIIReaderPrivate::GetObjectStatus( int otyp, int k )
{
  ObjectInfoType* oinfop = this->GetSortedObjectInfo( otyp, k );
  return oinfop ? oinfop->Status : 0;
}

int vtkExodusIIReaderPrivate::GetUnsortedObjectStatus( int otyp, int k )
{
  ObjectInfoType* oinfop = this->GetUnsortedObjectInfo( otyp, k );
  return oinfop ? oinfop->Status : 0;
}

void vtkExodusIIReaderPrivate::GetInitialObjectStatus( int otyp, ObjectInfoType *objType )
{
  for(unsigned int oidx=0; oidx<this->InitialObjectInfo[otyp].size(); oidx++)
  {
    if( (this->InitialObjectInfo[otyp][oidx].Name != "" &&
         objType->Name == this->InitialObjectInfo[otyp][oidx].Name) ||
        (this->InitialObjectInfo[otyp][oidx].Id != -1 &&
        objType->Id == this->InitialObjectInfo[otyp][oidx].Id) )
    {
      objType->Status = this->InitialObjectInfo[otyp][oidx].Status;
      break;
    }
  }
}

void vtkExodusIIReaderPrivate::SetObjectStatus( int otyp, int k, int stat )
{
  stat = (stat != 0); // Force stat to be either 0 or 1
  // OK, found the object
  ObjectInfoType* oinfop = this->GetSortedObjectInfo( otyp, k );
  if ( ! oinfop )
  { // error message will have been generated by GetSortedObjectInfo()
    return;
  }

  if ( oinfop->Status == stat )
  { // no change => do nothing
    return;
  }
  oinfop->Status = stat;

  this->Modified();
}

void vtkExodusIIReaderPrivate::SetUnsortedObjectStatus( int otyp, int k, int stat )
{
  stat = (stat != 0); // Force stat to be either 0 or 1
  // OK, found the object
  ObjectInfoType* oinfop = this->GetUnsortedObjectInfo( otyp, k );
  if ( ! oinfop )
  { // error message will have been generated by GetSortedObjectInfo()
    return;
  }

  if ( oinfop->Status == stat )
  { // no change => do nothing
    return;
  }
  oinfop->Status = stat;

  this->Modified();
}

void vtkExodusIIReaderPrivate::SetInitialObjectStatus( int objectType, const char* objName, int status )
{
  ObjectInfoType info;
  vtkStdString nm = objName;
  int idx = 0;
  int idlen = 0;
  int id = -1;

  // When no name is found for an object, it is given one of a certain format.
  // Parse the id out of that string and use it to identify the object later.
  if ( ( idx = static_cast<int>( nm.find( "ID: " ) ) ) != (int) vtkStdString::npos )
  {
    idx += 4;
    idlen = 0;
    while(idx+idlen < static_cast<int>(nm.length()) &&  nm.at(idx+idlen) != ' ' )
    {
      idlen++;
    }
    id = atoi(nm.substr(idx,idlen).c_str());
  }
  else
  {
    info.Name = objName;
  }
  info.Id = id;
  info.Status = status;
  this->InitialObjectInfo[objectType].push_back(info);
}

const char* vtkExodusIIReaderPrivate::GetObjectArrayName( int otyp, int i )
{
  std::map<int,std::vector<ArrayInfoType> >::iterator it = this->ArrayInfo.find( otyp );
  if ( it != this->ArrayInfo.end() )
  {
    int N = (int) it->second.size();
    if ( i < 0 || i >= N )
    {
      vtkDebugMacro( "You requested array " << i << " in a collection of only " << N << " arrays." );
      return 0;
    }
    return it->second[i].Name.c_str();
  }
  vtkDebugMacro( "Could not find collection of arrays for objects of type " << otyp <<
    " (" << objtype_names[this->GetObjectTypeIndexFromObjectType(otyp)] << ")." );
  return 0;
}

int vtkExodusIIReaderPrivate::GetNumberOfObjectArrayComponents( int otyp, int i )
{
  std::map<int,std::vector<ArrayInfoType> >::iterator it = this->ArrayInfo.find( otyp );
  if ( it != this->ArrayInfo.end() )
  {
    int N = (int) it->second.size();
    if ( i < 0 || i >= N )
    {
      vtkDebugMacro( "You requested array " << i << " in a collection of only " << N << " arrays." );
      return 0;
    }
    return it->second[i].Components;
  }
  vtkDebugMacro( "Could not find collection of arrays for objects of type " << otyp <<
    " (" << objtype_names[this->GetObjectTypeIndexFromObjectType(otyp)] << ")." );
  return 0;
}

int vtkExodusIIReaderPrivate::GetObjectArrayStatus( int otyp, int i )
{
  std::map<int,std::vector<ArrayInfoType> >::iterator it = this->ArrayInfo.find( otyp );
  if ( it != this->ArrayInfo.end() )
  {
    int N = (int) it->second.size();
    if ( i < 0 || i >= N )
    {
      vtkDebugMacro( "You requested array " << i << " in a collection of only " << N << " arrays." );
      return 0;
    }
    return it->second[i].Status;
  }
  vtkDebugMacro( "Could not find collection of arrays for objects of type " << otyp <<
    " (" << objtype_names[this->GetObjectTypeIndexFromObjectType(otyp)] << ")." );
  return 0;
}


void vtkExodusIIReaderPrivate::GetInitialObjectArrayStatus( int otyp, ArrayInfoType *objType )
{
  for(unsigned int oidx=0; oidx<this->InitialArrayInfo[otyp].size(); oidx++)
  {
    if(objType->Name == this->InitialArrayInfo[otyp][oidx].Name)
    {
      objType->Status = this->InitialArrayInfo[otyp][oidx].Status;
      break;
    }
  }
}

void vtkExodusIIReaderPrivate::SetObjectArrayStatus( int otyp, int i, int stat )
{
  stat = ( stat != 0 ); // Force stat to be either 0 or 1
  std::map<int,std::vector<ArrayInfoType> >::iterator it = this->ArrayInfo.find( otyp );
  if ( it != this->ArrayInfo.end() )
  {
    int N = (int) it->second.size();
    if ( i < 0 || i >= N )
    {
      vtkDebugMacro( "You requested array " << i << " in a collection of only " << N << " arrays." );
      return;
    }
    if ( it->second[i].Status == stat )
    {
      // no change => do nothing
      return;
    }
    it->second[i].Status = stat;
    this->Modified();
    // FIXME: Mark something so we know what's changed since the last RequestData?!
    // For the "global" (assembled) array, this is tricky because we really only want
    // to invalidate a range of the total array... For now, we'll just force the "global"
    // array to be reassembled even if it does mean a lot more copying -- it's not like
    // it was any faster before.
    //vtkExodusIICacheKey key( 0, GLOBAL, 0, i );
    //vtkExodusIICacheKey pattern( 0, 1, 0, 1 );
    this->Cache->Invalidate(
      vtkExodusIICacheKey( 0, vtkExodusIIReader::GLOBAL, otyp, i ),
      vtkExodusIICacheKey( 0, 1, 1, 1 ) );
  }
  else
  {
    vtkDebugMacro( "Could not find collection of arrays for objects of type " << otyp <<
      " (" << objtype_names[this->GetObjectTypeIndexFromObjectType(otyp)] << ")." );
  }
}

void vtkExodusIIReaderPrivate::SetInitialObjectArrayStatus( int objectType, const char* arrayName, int status )
{
  ArrayInfoType ainfo;
  ainfo.Name = arrayName;
  ainfo.Status = status;
  this->InitialArrayInfo[objectType].push_back(ainfo);
}

int vtkExodusIIReaderPrivate::GetNumberOfObjectAttributes( int otyp, int oi )
{
  std::map<int,std::vector<BlockInfoType> >::iterator it = this->BlockInfo.find( otyp );
  if ( it != this->BlockInfo.end() )
  {
    int N = (int) it->second.size();
    if ( oi < 0 || oi >= N )
    {
      int otypIdx = this->GetObjectTypeIndexFromObjectType(otyp);
      const char* btname = otypIdx >= 0 ? objtype_names[otypIdx] : "block";
      static_cast<void>(btname); // not referenced warning
      vtkDebugMacro( "You requested " << btname << " " << oi << " in a collection of only " << N << " blocks." );
      return 0;
    }
    oi = this->SortedObjectIndices[otyp][oi]; // index into sorted list of objects (block order, not file order)
    return (int) it->second[oi].AttributeNames.size();
  }
  return 0;
}

const char* vtkExodusIIReaderPrivate::GetObjectAttributeName( int otyp, int oi, int ai )
{
  std::map<int,std::vector<BlockInfoType> >::iterator it = this->BlockInfo.find( otyp );
  if ( it != this->BlockInfo.end() )
  {
    int N = (int) it->second.size();
    if ( oi < 0 || oi >= N )
    {
      vtkDebugMacro( "You requested block " << oi << " in a collection of only " << N << " blocks." );
      return 0;
    }
    oi = this->SortedObjectIndices[otyp][oi]; // index into sorted list of objects (block order, not file order)
    N = (int) it->second[oi].AttributeNames.size();
    if ( ai < 0 || ai >= N )
    {
      vtkDebugMacro( "You requested attribute " << ai << " in a collection of only " << N << " attributes." );
      return 0;
    }
    else
    {
      return it->second[oi].AttributeNames[ai].c_str();
    }
  }
  vtkDebugMacro( "Could not find collection of blocks of type " << otyp <<
    " (" << objtype_names[this->GetObjectTypeIndexFromObjectType(otyp)] << ")." );
  return 0;
}

int vtkExodusIIReaderPrivate::GetObjectAttributeIndex( int otyp, int oi, const char* attribName )
{
  std::map<int,std::vector<BlockInfoType> >::iterator it = this->BlockInfo.find( otyp );
  if ( it != this->BlockInfo.end() )
  {
    int N = (int) it->second.size();
    if ( oi < 0 || oi >= N )
    {
      vtkDebugMacro( "You requested block " << oi << " in a collection of only " << N << " blocks." );
      return -1;
    }
    oi = this->SortedObjectIndices[otyp][oi]; // index into sorted list of objects (block order, not file order)
    N = (int) it->second[oi].AttributeNames.size();
    int ai;
    for ( ai = 0; ai < N; ++ai )
    {
      if ( it->second[oi].AttributeNames[ai] == attribName )
      {
        return ai;
      }
    }
    return -1;
  }
  vtkDebugMacro( "Could not find collection of blocks of type " << otyp <<
    " (" << objtype_names[this->GetObjectTypeIndexFromObjectType(otyp)] << ")." );
  return -1;
}

int vtkExodusIIReaderPrivate::GetObjectAttributeStatus( int otyp, int oi, int ai )
{
  std::map<int,std::vector<BlockInfoType> >::iterator it = this->BlockInfo.find( otyp );
  if ( it != this->BlockInfo.end() )
  {
    int N = (int) it->second.size();
    if ( oi < 0 || oi >= N )
    {
      vtkDebugMacro( "You requested block " << oi << " in a collection of only " << N << " blocks." );
      return 0;
    }
    oi = this->SortedObjectIndices[otyp][oi]; // index into sorted list of objects (block order, not file order)
    N = (int) it->second[oi].AttributeStatus.size();
    if ( ai < 0 || ai >= N )
    {
      vtkDebugMacro( "You requested attribute " << ai << " in a collection of only " << N << " attributes." );
      return 0;
    }
    else
    {
      return it->second[oi].AttributeStatus[ai];
    }
  }
  vtkDebugMacro( "Could not find collection of blocks of type " << otyp <<
    " (" << objtype_names[this->GetObjectTypeIndexFromObjectType(otyp)] << ")." );
  return 0;
}

void vtkExodusIIReaderPrivate::SetObjectAttributeStatus( int otyp, int oi, int ai, int status )
{
  status = status ? 1 : 0;
  std::map<int,std::vector<BlockInfoType> >::iterator it = this->BlockInfo.find( otyp );
  if ( it != this->BlockInfo.end() )
  {
    int N = (int) it->second.size();
    if ( oi < 0 || oi >= N )
    {
      vtkDebugMacro( "You requested block " << oi << " in a collection of only " << N << " blocks." );
      return;
    }
    oi = this->SortedObjectIndices[otyp][oi]; // index into sorted list of objects (block order, not file order)
    N = (int) it->second[oi].AttributeStatus.size();
    if ( ai < 0 || ai >= N )
    {
      vtkDebugMacro( "You requested attribute " << ai << " in a collection of only " << N << " attribute." );
      return;
    }
    else
    {
      if ( it->second[oi].AttributeStatus[ai] == status )
      {
        return;
      }
      it->second[oi].AttributeStatus[ai] = status;
      this->Modified();
    }
  }
  vtkDebugMacro( "Could not find collection of blocks of type " << otyp <<
    " (" << objtype_names[this->GetObjectTypeIndexFromObjectType(otyp)] << ")." );
}

void vtkExodusIIReaderPrivate::SetApplyDisplacements( int d )
{
  if ( this->ApplyDisplacements == d )
    return;

  this->ApplyDisplacements = d;
  this->Modified();

  // Require the coordinates to be recomputed:
  this->Cache->Invalidate(
    vtkExodusIICacheKey( 0, vtkExodusIIReader::NODAL_COORDS, 0, 0 ),
    vtkExodusIICacheKey( 0, 1, 0, 0 ) );
}

void vtkExodusIIReaderPrivate::SetDisplacementMagnitude( double s )
{
  if ( this->DisplacementMagnitude == s )
    return;

  this->DisplacementMagnitude = s;
  this->Modified();

  // Require the coordinates to be recomputed:
  this->Cache->Invalidate(
    vtkExodusIICacheKey( 0, vtkExodusIIReader::NODAL_COORDS, 0, 0 ),
    vtkExodusIICacheKey( 0, 1, 0, 0 ) );
}

vtkDataArray* vtkExodusIIReaderPrivate::FindDisplacementVectors( int timeStep )
{
  std::map<int,std::vector<ArrayInfoType> >::iterator it = this->ArrayInfo.find( vtkExodusIIReader::NODAL );
  if ( it != this->ArrayInfo.end() )
  {
    int N = (int) it->second.size();
    for ( int i = 0; i < N; ++i )
    {
      std::string upperName = vtksys::SystemTools::UpperCase( it->second[i].Name.substr( 0, 3 ) );
      if ( upperName == "DIS" && it->second[i].Components == this->ModelParameters.num_dim )
      {
        return this->GetCacheOrRead( vtkExodusIICacheKey( timeStep, vtkExodusIIReader::NODAL, 0, i ) );
      }
    }
  }
  return 0;
}


// -------------------------------------------------------- PUBLIC CLASS MEMBERS

vtkStandardNewMacro(vtkExodusIIReader);
vtkCxxSetObjectMacro(vtkExodusIIReader,Metadata,vtkExodusIIReaderPrivate);

vtkExodusIIReader::vtkExodusIIReader()
{
  this->FileName = 0;
  this->XMLFileName = 0;
  this->Metadata = vtkExodusIIReaderPrivate::New();
  this->Metadata->Parent = this;
  this->Metadata->SetCacheSize(0.0);
  this->TimeStep = 0;
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  this->ModeShapesRange[0] = 0;
  this->ModeShapesRange[1] = 0;
  this->DisplayType = 0;
  this->DisplayType = 0;
  this->SILUpdateStamp = -1;

  this->SetNumberOfInputPorts( 0 );
}

vtkExodusIIReader::~vtkExodusIIReader()
{
  this->SetXMLFileName( 0 );
  this->SetFileName( 0 );

  this->SetMetadata( 0 );
  //this->SetExodusModel( 0 );
}

// Normally, vtkExodusIIReader::PrintSelf would be here.
// But it's above to prevent PrintSelf-Hybrid from failing because it assumes
// the first PrintSelf method is the one for the class declared in the header file.

int vtkExodusIIReader::CanReadFile( const char* fname )
{
  int exoid;
  int appWordSize = 8;
  int diskWordSize = 8;
  float version;

  if ( (exoid = ex_open( fname, EX_READ, &appWordSize, &diskWordSize, &version )) < 0 )
  {
    return 0;
  }
  if ( ex_close( exoid ) != 0 )
  {
    vtkWarningMacro( "Unable to close \"" << fname << "\" opened for testing." );
    return 0;
  }
  return 1;
}

#if 0
void vtkExodusIIReaderPrivate::Modified()
{
  cout << "E2RP modified\n"; this->Superclass::Modified();
}

void vtkExodusIIReader::Modified()
{
  cout << "E2R modified\n"; this->Superclass::Modified();
}
#endif // 0

vtkMTimeType vtkExodusIIReader::GetMTime()
{
  //return this->MTime.GetMTime();
  /*
  vtkMTimeType mtime1, mtime2;
  vtkMTimeType readerMTime = this->MTime.GetMTime();
  vtkMTimeType privateMTime = this->Metadata->GetMTime();
  vtkMTimeType fileNameMTime = this->FileNameMTime.GetMTime();
  vtkMTimeType xmlFileNameMTime = this->XMLFileNameMTime.GetMTime();
  mtime1 = privateMTime > readerMTime ? privateMTime : readerMTime;
  mtime2 = fileNameMTime > xmlFileNameMTime ? fileNameMTime : xmlFileNameMTime;
  return mtime1 > mtime2 ? mtime1 : mtime2;
  */
  vtkMTimeType readerMTime = this->MTime.GetMTime();
  vtkMTimeType privateMTime = this->Metadata->GetMTime();
  return privateMTime > readerMTime ? privateMTime : readerMTime;
}

vtkMTimeType vtkExodusIIReader::GetMetadataMTime()
{
  return this->Metadata->InformationTimeStamp < this->Metadata->GetMTime() ?
    this->Metadata->InformationTimeStamp : this->Metadata->GetMTime();
}

#define vtkSetStringMacroBody(propName,fname) \
  int modified = 0; \
  if ( fname == this->propName ) \
    return; \
  if ( fname && this->propName && !strcmp( fname, this->propName ) ) \
    return; \
  modified = 1; \
  delete [] this->propName; \
  if ( fname ) \
  { \
    size_t fnl = strlen( fname ) + 1; \
    char* dst = new char[fnl]; \
    const char* src = fname; \
    this->propName = dst; \
    do { *dst++ = *src++; } while ( --fnl ); \
  } \
  else \
  { \
    this->propName = 0; \
  }

void vtkExodusIIReader::SetFileName( const char* fname )
{
  vtkSetStringMacroBody(FileName,fname);
  if ( modified )
  {
    this->Metadata->Reset();
    this->FileNameMTime.Modified();
  }
}

void vtkExodusIIReader::SetXMLFileName( const char* fname )
{
  vtkSetStringMacroBody(XMLFileName,fname);
  if ( modified )
  {
    this->XMLFileNameMTime.Modified();
  }
}



//----------------------------------------------------------------------------
int vtkExodusIIReader::ProcessRequest(vtkInformation* request,
                                        vtkInformationVector** inputVector,
                                        vtkInformationVector* outputVector)
{
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}


//----------------------------------------------------------------------------
int vtkExodusIIReader::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector )
{
  int newMetadata = 0;
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // If the metadata is older than the filename
  if ( this->GetMetadataMTime() < this->FileNameMTime )
  {
    if ( this->Metadata->OpenFile( this->FileName ) )
    {
      // We need to initialize the XML parser before calling RequestInformation
      //    on the metadata
      if ( this->FindXMLFile() )
      {
        vtkExodusIIReaderParser *parser = vtkExodusIIReaderParser::New();
        this->Metadata->SetParser(parser);
        // Now overwrite any names in the exodus file with names from XML file.
        parser->Go(this->XMLFileName);
        parser->Delete();
      }

      this->Metadata->RequestInformation();

      // Now check to see if the DART metadata is valid
      if(this->Metadata->Parser && !this->Metadata->IsXMLMetadataValid())
      {
        this->Metadata->Parser->Delete();
        this->Metadata->Parser = 0;

        // Reset block names.
        int numBlocks = this->Metadata->GetNumberOfObjectsOfType(vtkExodusIIReader::ELEM_BLOCK);
        for (int cc=0; cc < numBlocks; cc++)
        {
          vtkExodusIIReaderPrivate::BlockInfoType* binfop =
            static_cast<vtkExodusIIReaderPrivate::BlockInfoType*>(
              this->Metadata->GetSortedObjectInfo(vtkExodusIIReader::ELEM_BLOCK, cc));
          binfop->Name = binfop->OriginalName;
        }
      }

      // Once meta-data has been refreshed we update the SIL.
      this->Metadata->BuildSIL();
      this->SILUpdateStamp++; // update the timestamp.

      this->Metadata->CloseFile();
      newMetadata = 1;
    }
    else
    {
      vtkErrorMacro( "Unable to open file \"" << (this->FileName ? this->FileName : "(null)") << "\" to read metadata" );
      return 0;
    }
  }

  this->AdvertiseTimeSteps( outInfo );

  // Advertize the SIL.
  outInfo->Set(vtkDataObject::SIL(), this->Metadata->GetSIL());

  if ( newMetadata )
  {
    // update ExodusModelMetadata
  }

  return 1;
}

int vtkExodusIIReader::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector )
{
  if ( ! this->FileName || ! this->Metadata->OpenFile( this->FileName ) )
  {
    vtkErrorMacro( "Unable to open file \"" << (this->FileName ? this->FileName : "(null)") << "\" to read data" );
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast( outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  // Check if a particular time was requested.
  if ( outInfo->Has( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP() ) )
  { // Get the requested time step. We only support requests of a single time step in this reader right now
    double requestedTimeStep = outInfo->Get( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    // Save the time value in the output data information.
    int length = outInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    double* steps = outInfo->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );

    if ( ! this->GetHasModeShapes() )
    {
      // find the highest time step with a time value that is smaller than the requested time.
      //timeStep = 0;
      //while (timeStep < length - 1 && steps[timeStep] < requestedTimeStep)
      //  {
      //  timeStep++;
      //  }
      //this->TimeStep = timeStep;

      //find the timestep with the closest value
      int cnt=0;
      int closestStep=0;
      double minDist=-1;
      for (cnt=0;cnt<length;cnt++)
      {
        double tdist=(steps[cnt]-requestedTimeStep>requestedTimeStep-steps[cnt])?
          steps[cnt]-requestedTimeStep:
          requestedTimeStep-steps[cnt];
        if (minDist<0 || tdist<minDist)
        {
          minDist=tdist;
          closestStep=cnt;
        }
      }
      this->TimeStep=closestStep;
      //cout << "Requested value: " << requestedTimeStep << " Step: " << this->TimeStep << endl;
      output->GetInformation()->Set( vtkDataObject::DATA_TIME_STEP(), steps[this->TimeStep] );
    }
    else if (this->GetAnimateModeShapes())
    {
      // Let the metadata know the time value so that the Metadata->RequestData call below will generate
      // the animated mode shape properly.
      this->Metadata->ModeShapeTime = requestedTimeStep;
      output->GetInformation()->Set( vtkDataObject::DATA_TIME_STEP(), this->Metadata->ModeShapeTime );
      //output->GetInformation()->Remove( vtkDataObject::DATA_TIME_STEP() );
    }
  }

  this->Metadata->RequestData( this->TimeStep, output );

  return 1;
}

void vtkExodusIIReader::SetGenerateObjectIdCellArray( int x ) { this->Metadata->SetGenerateObjectIdArray( x ); }
int vtkExodusIIReader::GetGenerateObjectIdCellArray() { return this->Metadata->GetGenerateObjectIdArray(); }

void vtkExodusIIReader::SetGenerateGlobalElementIdArray( int x ) { this->Metadata->SetGenerateGlobalElementIdArray( x ); }
int vtkExodusIIReader::GetGenerateGlobalElementIdArray() { return this->Metadata->GetGenerateGlobalElementIdArray(); }

void vtkExodusIIReader::SetGenerateGlobalNodeIdArray( int x ) { this->Metadata->SetGenerateGlobalNodeIdArray( x ); }
int vtkExodusIIReader::GetGenerateGlobalNodeIdArray() { return this->Metadata->GetGenerateGlobalNodeIdArray(); }

void vtkExodusIIReader::SetGenerateImplicitElementIdArray( int x ) { this->Metadata->SetGenerateImplicitElementIdArray( x ); }
int vtkExodusIIReader::GetGenerateImplicitElementIdArray() { return this->Metadata->GetGenerateImplicitElementIdArray(); }

void vtkExodusIIReader::SetGenerateImplicitNodeIdArray( int x ) { this->Metadata->SetGenerateImplicitNodeIdArray( x ); }
int vtkExodusIIReader::GetGenerateImplicitNodeIdArray() { return this->Metadata->GetGenerateImplicitNodeIdArray(); }

void vtkExodusIIReader::SetGenerateFileIdArray( int x ) { this->Metadata->SetGenerateFileIdArray( x ); }
int vtkExodusIIReader::GetGenerateFileIdArray() { return this->Metadata->GetGenerateFileIdArray(); }

void vtkExodusIIReader::SetFileId( int x ) { this->Metadata->SetFileId( x ); }
int vtkExodusIIReader::GetFileId() { return this->Metadata->GetFileId(); }

// FIXME: Implement the four functions that return ID_NOT_FOUND below.
int vtkExodusIIReader::GetGlobalElementID( vtkDataSet* data, int localID )
{ return GetGlobalElementID( data, localID, SEARCH_TYPE_ELEMENT_THEN_NODE ); }
int vtkExodusIIReader::GetGlobalElementID ( vtkDataSet* data, int localID, int searchType )
{ (void)data; (void)localID; (void)searchType; return ID_NOT_FOUND; }

int vtkExodusIIReader::GetGlobalFaceID( vtkDataSet* data, int localID )
{ return GetGlobalFaceID( data, localID, SEARCH_TYPE_ELEMENT_THEN_NODE ); }
int vtkExodusIIReader::GetGlobalFaceID ( vtkDataSet* data, int localID, int searchType )
{ (void)data; (void)localID; (void)searchType; return ID_NOT_FOUND; }

int vtkExodusIIReader::GetGlobalEdgeID( vtkDataSet* data, int localID )
{ return GetGlobalEdgeID( data, localID, SEARCH_TYPE_ELEMENT_THEN_NODE ); }
int vtkExodusIIReader::GetGlobalEdgeID ( vtkDataSet* data, int localID, int searchType )
{ (void)data; (void)localID; (void)searchType; return ID_NOT_FOUND; }

int vtkExodusIIReader::GetGlobalNodeID( vtkDataSet* data, int localID )
{ return GetGlobalNodeID( data, localID, SEARCH_TYPE_NODE_THEN_ELEMENT ); }
int vtkExodusIIReader::GetGlobalNodeID( vtkDataSet* data, int localID, int searchType )
{ (void)data; (void)localID; (void)searchType; return ID_NOT_FOUND; }

void vtkExodusIIReader::SetApplyDisplacements( int d )
{
  this->Metadata->SetApplyDisplacements( d );
}
int vtkExodusIIReader::GetApplyDisplacements()
{
  return this->Metadata->GetApplyDisplacements();
}

void vtkExodusIIReader::SetDisplacementMagnitude( float s )
{
  this->Metadata->SetDisplacementMagnitude( s );
}
float vtkExodusIIReader::GetDisplacementMagnitude()
{
  return this->Metadata->GetDisplacementMagnitude();
}

void vtkExodusIIReader::SetHasModeShapes( int ms )
{
  this->Metadata->SetHasModeShapes(ms);
}

int vtkExodusIIReader::GetHasModeShapes()
{
  return this->Metadata->GetHasModeShapes();
}

void vtkExodusIIReader::SetModeShapeTime( double phase )
{
  // Phase should repeat outside the bounds [0,1].  For example, 0.25 is
  // equivalent to 1.25, 2.25, -0.75, and -1.75.
  double x = phase - floor(phase);
  this->Metadata->SetModeShapeTime( x );
}

double vtkExodusIIReader::GetModeShapeTime()
{
  return this->Metadata->GetModeShapeTime();
}

void vtkExodusIIReader::SetAnimateModeShapes(int flag)
{
  this->Metadata->SetAnimateModeShapes(flag);
}

int vtkExodusIIReader::GetAnimateModeShapes()
{
  return this->Metadata->GetAnimateModeShapes();
}

const char* vtkExodusIIReader::GetTitle() { return this->Metadata->ModelParameters.title; }
int vtkExodusIIReader::GetDimensionality() { return this->Metadata->ModelParameters.num_dim; }
int vtkExodusIIReader::GetNumberOfTimeSteps() { return (int) this->Metadata->Times.size(); }

int vtkExodusIIReader::GetNumberOfNodesInFile() { return this->Metadata->ModelParameters.num_nodes; }
int vtkExodusIIReader::GetNumberOfEdgesInFile() { return this->Metadata->ModelParameters.num_edge; }
int vtkExodusIIReader::GetNumberOfFacesInFile() { return this->Metadata->ModelParameters.num_face; }
int vtkExodusIIReader::GetNumberOfElementsInFile() { return this->Metadata->ModelParameters.num_elem; }

int vtkExodusIIReader::GetNumberOfObjects( int objectType )
{
  return this->Metadata->GetNumberOfObjectsOfType( objectType );
}

int vtkExodusIIReader::GetObjectTypeFromName( const char* name )
{
  vtkStdString tname( name );
  if ( tname == "edge" ) return EDGE_BLOCK;
  else if ( tname == "face" ) return FACE_BLOCK;
  else if ( tname == "element" ) return ELEM_BLOCK;
  else if ( tname == "node set" ) return NODE_SET;
  else if ( tname == "edge set" ) return EDGE_SET;
  else if ( tname == "face set" ) return FACE_SET;
  else if ( tname == "side set" ) return SIDE_SET;
  else if ( tname == "element set" ) return ELEM_SET;
  else if ( tname == "node map" ) return NODE_MAP;
  else if ( tname == "edge map" ) return EDGE_MAP;
  else if ( tname == "face map" ) return FACE_MAP;
  else if ( tname == "element map" ) return ELEM_MAP;
  else if ( tname == "grid" ) return GLOBAL;
  else if ( tname == "node" ) return NODAL;
  else if ( tname == "assembly" ) return ASSEMBLY;
  else if ( tname == "part" ) return PART;
  else if ( tname == "material" ) return MATERIAL;
  else if ( tname == "hierarchy" ) return HIERARCHY;
  else if ( tname == "cell" ) return GLOBAL_CONN;
  else if ( tname == "element block cell" ) return ELEM_BLOCK_ELEM_CONN;
  else if ( tname == "element block face" ) return ELEM_BLOCK_FACE_CONN;
  else if ( tname == "element block edge" ) return ELEM_BLOCK_EDGE_CONN;
  else if ( tname == "face block cell" ) return FACE_BLOCK_CONN;
  else if ( tname == "edge block cell" ) return EDGE_BLOCK_CONN;
  else if ( tname == "element set cell" ) return ELEM_SET_CONN;
  else if ( tname == "side set cell" ) return SIDE_SET_CONN;
  else if ( tname == "face set cell" ) return FACE_SET_CONN;
  else if ( tname == "edge set cell" ) return EDGE_SET_CONN;
  else if ( tname == "node set cell" ) return NODE_SET_CONN;
  else if ( tname == "nodal coordinates" ) return NODAL_COORDS;
  else if ( tname == "object id" ) return OBJECT_ID;
  else if ( tname == "implicit element id" ) return IMPLICIT_ELEMENT_ID;
  else if ( tname == "implicit node id" ) return IMPLICIT_NODE_ID;
  else if ( tname == "global element id" ) return GLOBAL_ELEMENT_ID;
  else if ( tname == "global node id" ) return GLOBAL_NODE_ID;
  else if ( tname == "element id" ) return ELEMENT_ID;
  else if ( tname == "node id" ) return NODE_ID;
  else if ( tname == "pointmap" ) return NODAL_SQUEEZEMAP;
  return -1;
}

const char* vtkExodusIIReader::GetObjectTypeName( int otyp )
{
  switch ( otyp )
  {
  case EDGE_BLOCK: return "edge";
  case FACE_BLOCK: return "face";
  case ELEM_BLOCK: return "element";
  case NODE_SET: return "node set";
  case EDGE_SET: return "edge set";
  case FACE_SET: return "face set";
  case SIDE_SET: return "side set";
  case ELEM_SET: return "element set";
  case NODE_MAP: return "node map";
  case EDGE_MAP: return "edge map";
  case FACE_MAP: return "face map";
  case ELEM_MAP: return "element map";
  case GLOBAL: return "grid";
  case NODAL: return "node";
  case ASSEMBLY: return "assembly";
  case PART: return "part";
  case MATERIAL: return "material";
  case HIERARCHY: return "hierarchy";
  case GLOBAL_CONN: return "cell";
  case ELEM_BLOCK_ELEM_CONN: return "element block cell";
  case ELEM_BLOCK_FACE_CONN: return "element block face";
  case ELEM_BLOCK_EDGE_CONN: return "element block edge";
  case FACE_BLOCK_CONN: return "face block cell";
  case EDGE_BLOCK_CONN: return "edge block cell";
  case ELEM_SET_CONN: return "element set cell";
  case SIDE_SET_CONN: return "side set cell";
  case FACE_SET_CONN: return "face set cell";
  case EDGE_SET_CONN: return "edge set cell";
  case NODE_SET_CONN: return "node set cell";
  case NODAL_COORDS: return "nodal coordinates";
  case OBJECT_ID: return "object id";
  case IMPLICIT_ELEMENT_ID: return "implicit element id";
  case IMPLICIT_NODE_ID: return "implicit node id";
  case GLOBAL_ELEMENT_ID: return "global element id";
  case GLOBAL_NODE_ID: return "global node id";
  case ELEMENT_ID: return "element id";
  case NODE_ID: return "node id";
  case NODAL_SQUEEZEMAP: return "pointmap";
  }
  return 0;
}

int vtkExodusIIReader::GetNumberOfNodes() { return this->Metadata->GetNumberOfNodes(); }

int vtkExodusIIReader::GetNumberOfEntriesInObject( int objectType, int objectIndex )
{
  return this->Metadata->GetObjectSize( objectType, objectIndex );
}

int vtkExodusIIReader::GetObjectId( int objectType, int objectIndex )
{
  return this->Metadata->GetObjectId( objectType, objectIndex );
}

int vtkExodusIIReader::GetObjectStatus( int objectType, int objectIndex )
{
  return this->Metadata->GetObjectStatus( objectType, objectIndex );
}

void vtkExodusIIReader::SetObjectStatus( int objectType, int objectIndex, int status )
{
  this->Metadata->SetObjectStatus( objectType, objectIndex, status );
}

void vtkExodusIIReader::SetObjectStatus( int objectType, const char* objectName, int status )
{
  if(objectName && strlen(objectName)>0)
  {
    if(this->GetNumberOfObjects(objectType) == 0)
    {
      // The object status is being set before the meta data has been finalized
      // so cache this value for later and use as the initial value
      // If the number of objects really is zero then this doesn't do any harm.
      this->Metadata->SetInitialObjectStatus(objectType, objectName, status);
      return;
    }
    this->SetObjectStatus( objectType, this->GetObjectIndex( objectType, objectName ), status );
  }
}

const char* vtkExodusIIReader::GetObjectName( int objectType, int objectIndex )
{
  return this->Metadata->GetObjectName( objectType, objectIndex );
}

int vtkExodusIIReader::GetObjectIndex( int objectType, const char* objectName )
{
  if ( ! objectName )
  {
    vtkErrorMacro( "You must specify a non-NULL name" );
    return -1;
  }
  int nObj = this->GetNumberOfObjects( objectType );
  if ( nObj == 0 )
  {
    vtkDebugMacro( "No objects of that type (" << objectType << ") to find index for given name " << objectName << "." );
    return -1;
  }
  vtkStdString objectRealName(objectName);
  size_t i = objectRealName.find(" Size: ");
  if(i!= vtkStdString::npos)
  {
    objectRealName.erase(i);
  }
  for ( int obj = 0; obj < nObj; ++obj )
  {
    const char* storedObjName = this->GetObjectName( objectType, obj );
    if(objectRealName == vtkStdString(storedObjName))
    {
      return obj;
    }
  }
  vtkDebugMacro( "No objects named \"" << objectName << "\" of the specified type (" << objectType <<  ")." );
  return -1;
}

int vtkExodusIIReader::GetObjectIndex( int objectType, int id )
{
  int nObj = this->GetNumberOfObjects( objectType );
  if ( nObj == 0 )
  {
    vtkDebugMacro( "No objects of that type (" << objectType << ") to find index for given id " << id << "." );
    return -1;
  }
  for ( int obj = 0; obj < nObj; ++obj )
  {
    if ( this->GetObjectId( objectType, obj ) == id)
    {
      return obj;
    }
  }
  vtkDebugMacro( "No objects with id \"" << id << "\" of the specified type (" << objectType <<  ")." );
  return -1;
}

int vtkExodusIIReader::GetNumberOfObjectArrays( int objectType )
{
  return this->Metadata->GetNumberOfObjectArraysOfType( objectType );
}

const char* vtkExodusIIReader::GetObjectArrayName( int objectType, int arrayIndex )
{
  return this->Metadata->GetObjectArrayName( objectType, arrayIndex );
}

int vtkExodusIIReader::GetNumberOfObjectArrayComponents( int objectType, int arrayIndex )
{
  return this->Metadata->GetNumberOfObjectArrayComponents( objectType, arrayIndex );
}

int vtkExodusIIReader::GetObjectArrayStatus( int objectType, int arrayIndex )
{
  return this->Metadata->GetObjectArrayStatus( objectType, arrayIndex );
}

void vtkExodusIIReader::SetObjectArrayStatus( int objectType, int arrayIndex, int status )
{
  this->Metadata->SetObjectArrayStatus( objectType, arrayIndex, status );
}

void vtkExodusIIReader::SetObjectArrayStatus( int objectType, const char* arrayName, int status )
{
  if(arrayName && strlen(arrayName)>0)
  {
    if(this->GetNumberOfObjectArrays( objectType ) == 0)
    {
      // The array status is being set before the meta data has been finalized
      // so cache this value for later and use as the initial value
      // If the number of arrays really is zero then this doesn't do any harm.
      this->Metadata->SetInitialObjectArrayStatus(objectType, arrayName, status);
      return;
    }
    this->SetObjectArrayStatus( objectType, this->GetObjectArrayIndex( objectType, arrayName ), status );
  }
}

int vtkExodusIIReader::GetNumberOfObjectAttributes( int objectType, int objectIndex )
{
  return this->Metadata->GetNumberOfObjectAttributes( objectType, objectIndex );
}

const char* vtkExodusIIReader::GetObjectAttributeName( int objectType, int objectIndex, int attribIndex )
{
  return this->Metadata->GetObjectAttributeName( objectType, objectIndex, attribIndex );
}

int vtkExodusIIReader::GetObjectAttributeIndex( int objectType, int objectIndex, const char* attribName )
{
  return this->Metadata->GetObjectAttributeIndex( objectType, objectIndex, attribName );
}

int vtkExodusIIReader::GetObjectAttributeStatus( int objectType, int objectIndex, int attribIndex )
{
  return this->Metadata->GetObjectAttributeStatus( objectType, objectIndex, attribIndex );
}

void vtkExodusIIReader::SetObjectAttributeStatus( int objectType, int objectIndex, int attribIndex, int status )
{
  this->Metadata->SetObjectAttributeStatus( objectType, objectIndex, attribIndex, status );
}

int vtkExodusIIReader::GetObjectArrayIndex( int objectType, const char* arrayName )
{
  if ( ! arrayName )
  {
    vtkErrorMacro( "You must specify a non-NULL name" );
    return -1;
  }
  int nObj = this->GetNumberOfObjectArrays( objectType );
  if ( nObj == 0 )
  {
    vtkDebugMacro( "No objects of that type (" << objectType << ") to find index for given array " << arrayName << "." );
    return -1;
  }
  for ( int obj = 0; obj < nObj; ++obj )
  {
    if ( !strcmp( arrayName, this->GetObjectArrayName( objectType, obj ) ) )
    {
      return obj;
    }
  }
  vtkDebugMacro( "No arrays named \"" << arrayName << "\" of the specified type (" << objectType <<  ")." );
  return -1;
}

vtkIdType vtkExodusIIReader::GetTotalNumberOfNodes() { return this->Metadata->GetModelParams()->num_nodes; }
vtkIdType vtkExodusIIReader::GetTotalNumberOfEdges() { return this->Metadata->GetModelParams()->num_edge; }
vtkIdType vtkExodusIIReader::GetTotalNumberOfFaces() { return this->Metadata->GetModelParams()->num_face; }
vtkIdType vtkExodusIIReader::GetTotalNumberOfElements() { return this->Metadata->GetModelParams()->num_elem; }

// %---------------------------------------------------------------------------
int vtkExodusIIReader::GetNumberOfPartArrays()
{
  return this->Metadata->GetNumberOfParts();
}

const char* vtkExodusIIReader::GetPartArrayName( int arrayIdx )
{
  return this->Metadata->GetPartName(arrayIdx);
}

int vtkExodusIIReader::GetPartArrayID( const char *name )
{
  int numArrays = this->GetNumberOfPartArrays();
  for ( int i=0;i<numArrays;i++ )
  {
    if ( strcmp( name, this->GetPartArrayName( i ) ) == 0 )
    {
      return i;
    }
  }
  return -1;
}

const char* vtkExodusIIReader::GetPartBlockInfo( int arrayIdx )
{
  return this->Metadata->GetPartBlockInfo(arrayIdx);
}

void vtkExodusIIReader::SetPartArrayStatus( int index, int flag )
{
  // Only modify if we are 'out of sync'
  if (this->Metadata->GetPartStatus(index) != flag)
  {
    this->Metadata->SetPartStatus(index, flag);

    // Because which parts are on/off affects the
    // geometry we need to remake the mesh cache
    //this->RemakeDataCacheFlag = 1;
    this->Modified();
  }
}

void vtkExodusIIReader::SetPartArrayStatus( const char* name, int flag )
{
  // Only modify if we are 'out of sync'
  if (this->Metadata->GetPartStatus(name) != flag)
  {
    this->Metadata->SetPartStatus(name, flag);

    // Because which parts are on/off affects the
    // geometry we need to remake the mesh cache
    //this->RemakeDataCacheFlag = 1;
    this->Modified();
  }
}

int vtkExodusIIReader::GetPartArrayStatus( int index )
{
  return this->Metadata->GetPartStatus(index);
}

int vtkExodusIIReader::GetPartArrayStatus( const char* part )
{
  return this->Metadata->GetPartStatus(part);
}

int vtkExodusIIReader::GetNumberOfMaterialArrays()
{
  return this->Metadata->GetNumberOfMaterials();
}

const char* vtkExodusIIReader::GetMaterialArrayName( int arrayIdx )
{
  return this->Metadata->GetMaterialName(arrayIdx);
}

int vtkExodusIIReader::GetMaterialArrayID( const char* matl )
{
  (void)matl;
  return 0;
}

void vtkExodusIIReader::SetMaterialArrayStatus( int index, int flag )
{
  // Only modify if we are 'out of sync'
  if (this->Metadata->GetMaterialStatus(index) != flag)
  {
    this->Metadata->SetMaterialStatus(index, flag);

    // Because which materials are on/off affects the
    // geometry we need to remake the mesh cache
    //this->RemakeDataCacheFlag = 1;
    this->Modified();
  }
}

void vtkExodusIIReader::SetMaterialArrayStatus( const char* matl, int flag )
{
  // Only modify if we are 'out of sync'
  if (this->Metadata->GetMaterialStatus(matl) != flag)
  {
    this->Metadata->SetMaterialStatus(matl, flag);

    // Because which materials are on/off affects the
    // geometry we need to remake the mesh cache
    //this->RemakeDataCacheFlag = 1;
    this->Modified();
  }
}

int vtkExodusIIReader::GetMaterialArrayStatus( int index )
{
  return this->Metadata->GetMaterialStatus(index);
}

int vtkExodusIIReader::GetMaterialArrayStatus( const char* matl )
{
  return this->Metadata->GetMaterialStatus(matl);
}

int vtkExodusIIReader::GetNumberOfAssemblyArrays()
{
  return this->Metadata->GetNumberOfAssemblies();
}

const char* vtkExodusIIReader::GetAssemblyArrayName( int arrayIdx )
{
  return this->Metadata->GetAssemblyName(arrayIdx);
}

int vtkExodusIIReader::GetAssemblyArrayID( const char* name )
{
  int numArrays = this->GetNumberOfAssemblyArrays();
  for ( int i=0;i<numArrays;i++ )
  {
    if ( strcmp( name, this->GetAssemblyArrayName( i ) ) == 0 )
    {
      return i;
    }
  }
  return -1;
}


void vtkExodusIIReader::SetAssemblyArrayStatus(int index, int flag)
{
  // Only modify if we are 'out of sync'
  if (this->Metadata->GetAssemblyStatus(index) != flag)
  {
    this->Metadata->SetAssemblyStatus(index, flag);

    // Because which materials are on/off affects the
    // geometry we need to remake the mesh cache
    //this->RemakeDataCacheFlag = 1;
    this->Modified();
  }
}

void vtkExodusIIReader::SetAssemblyArrayStatus(const char* name, int flag)
{
  // Only modify if we are 'out of sync'
  if (this->Metadata->GetAssemblyStatus(name) != flag)
  {
    this->Metadata->SetAssemblyStatus(name, flag);

    // Because which materials are on/off affects the
    // geometry we need to remake the mesh cache
    //this->RemakeDataCacheFlag = 1;
    this->Modified();
  }
}


int vtkExodusIIReader::GetAssemblyArrayStatus(int index)
{
  return this->Metadata->GetAssemblyStatus(index);
}

int vtkExodusIIReader::GetAssemblyArrayStatus(const char* name)
{
  return this->Metadata->GetAssemblyStatus(name);
}

int vtkExodusIIReader::GetNumberOfHierarchyArrays()
{
  //if (this->Metadata->Parser)
  //  {
  //  return this->Metadata->Parser->GetNumberOfHierarchyEntries();
  //  }
  return 0;
}


const char* vtkExodusIIReader::GetHierarchyArrayName(int vtkNotUsed(arrayIdx))
{
  //if (this->Metadata->Parser)
  //  {
  //  this->Metadata->Parser->SetCurrentHierarchyEntry(arrayIdx);
  //  return this->Metadata->Parser->GetCurrentHierarchyEntry();
  //  }
  return "Should not see this";
}

void vtkExodusIIReader::SetHierarchyArrayStatus(int vtkNotUsed(index), int vtkNotUsed(flag))
{
  //// Only modify if we are 'out of sync'
  ////if (this->GetHierarchyArrayStatus(index) != flag)
  //// {
  //if (this->Metadata->Parser)
  //  {
  //  std::vector<int> blocksIds = this->Metadata->Parser->GetBlocksForEntry(index);
  //  for (std::vector<int>::size_type i=0;i<blocksIds.size();i++)
  //    {
  //    this->Metadata->SetObjectStatus(vtkExodusIIReader::ELEM_BLOCK,
  //      this->GetObjectIndex(ELEM_BLOCK,blocksIds[i]),flag);
  //    }
  //
  //  // Because which materials are on/off affects the
  //  // geometry we need to remake the mesh cache
  //  //this->RemakeDataCacheFlag = 1;
  //  this->Modified();
  //  }
}

void vtkExodusIIReader::SetHierarchyArrayStatus(const char* vtkNotUsed(name), int vtkNotUsed(flag))
{
  //// Only modify if we are 'out of sync'
  ////if (this->GetHierarchyArrayStatus(name) != flag)
  ////{
  //if (this->Metadata->Parser)
  //  {
  //  std::vector<int> blocksIds=this->Metadata->Parser->GetBlocksForEntry
  //    (vtkStdString(name));
  //  for (std::vector<int>::size_type i=0;i<blocksIds.size();i++)
  //    {
  //    //cout << "turning block " << blocks[i] << " " << flag << endl;
  //    this->Metadata->SetObjectStatus(vtkExodusIIReader::ELEM_BLOCK,
  //      this->GetObjectIndex(ELEM_BLOCK,blocksIds[i]),flag);
  //    }
  //
  //  // Because which materials are on/off affects the
  //  // geometry we need to remake the mesh cache
  //  //this->RemakeDataCacheFlag = 1;
  //  this->Modified();
  //  }
}

//-----------------------------------------------------------------------------
int vtkExodusIIReader::GetHierarchyArrayStatus(int vtkNotUsed(index))
{
  //if (this->Metadata->Parser)
  //  {
  //  std::vector<int> blocksIds=this->Metadata->Parser->GetBlocksForEntry(index);
  //  for (std::vector<int>::size_type i=0;i<blocksIds.size();i++)
  //    {
  //    if (this->Metadata->GetObjectStatus(vtkExodusIIReader::ELEM_BLOCK,
  //        this->GetObjectIndex(ELEM_BLOCK,blocksIds[i]))==0)
  //      return 0;
  //    }
  //  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkExodusIIReader::GetHierarchyArrayStatus(const char* vtkNotUsed(name))
{
  //if (this->Metadata->Parser)
  //  {
  //  std::vector<int> blocksIds=this->Metadata->Parser->GetBlocksForEntry(name);
  //  for (std::vector<int>::size_type i=0;i<blocksIds.size();i++)
  //    {
  //    if (this->Metadata->GetObjectStatus(vtkExodusIIReader::ELEM_BLOCK,
  //        this->GetObjectIndex(ELEM_BLOCK,blocksIds[i]))==0)
  //      return 0;
  //    }
  //  }
  return 1;
}

//-----------------------------------------------------------------------------
vtkGraph* vtkExodusIIReader::GetSIL()
{
  return this->Metadata->GetSIL();
}

//-----------------------------------------------------------------------------
void vtkExodusIIReader::SetDisplayType( int typ )
{
  if ( typ == this->DisplayType || typ < 0 || typ > 2 )
    return;

  this->DisplayType = typ;
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkExodusIIReader::IsValidVariable( const char *type, const char *name )
{
  return (this->GetVariableID( type, name ) >= 0);
}

//-----------------------------------------------------------------------------
int vtkExodusIIReader::GetVariableID( const char *type, const char *name )
{
  int otyp = this->GetObjectTypeFromName( type );
  if ( otyp < 0 )
  {
    return 0;
  }
  switch ( otyp )
  {
  case NODAL:
  case EDGE_BLOCK:
  case FACE_BLOCK:
  case ELEM_BLOCK:
  case NODE_SET:
  case EDGE_SET:
  case FACE_SET:
  case SIDE_SET:
  case ELEM_SET:
    return this->GetObjectArrayIndex( otyp, name );
  case ASSEMBLY:
    return this->GetAssemblyArrayID( name );
  case HIERARCHY:
    return -1; // FIXME: There is no this->GetHierarchyArrayID( name ) and it's not clear there should be.
  case MATERIAL:
    return this->GetMaterialArrayID( name );
  case PART:
    return this->GetPartArrayID( name );
  default:
    return -1;
  }
}

int vtkExodusIIReader::GetTimeSeriesData( int ID, const char* vName, const char* vType, vtkFloatArray* result )
{
  (void)ID;
  (void)vName;
  (void)vType;
  (void)result;
  return -1;
}

void vtkExodusIIReader::SetAllArrayStatus( int otyp, int status )
{
  int numObj;
  int i;
  switch ( otyp )
  {
  case EDGE_BLOCK_CONN:
  case FACE_BLOCK_CONN:
  case ELEM_BLOCK_ELEM_CONN:
  case NODE_SET_CONN:
  case EDGE_SET_CONN:
  case FACE_SET_CONN:
  case SIDE_SET_CONN:
  case ELEM_SET_CONN:
  { // Convert the "connectivity" type into an "object" type:
      int ctypidx = this->Metadata->GetConnTypeIndexFromConnType(otyp);
      int otypidx = conn_obj_idx_cvt[ctypidx];
      otyp = obj_types[otypidx];
      // Now set the status
      numObj = this->GetNumberOfObjects( otyp );
      for ( i = 0; i < numObj; ++i )
      {
        this->SetObjectStatus( otyp, i, status );
      }
  }
    break;
  case NODAL:
  case GLOBAL:
  case EDGE_BLOCK:
  case FACE_BLOCK:
  case ELEM_BLOCK:
  case NODE_SET:
  case EDGE_SET:
  case FACE_SET:
  case SIDE_SET:
  case ELEM_SET:
    numObj = this->GetNumberOfObjectArrays( otyp );
    for ( i = 0; i < numObj; ++i )
    {
      this->SetObjectArrayStatus( otyp, i, status );
    }
    break;
  // ---------------------
  case ASSEMBLY:
    numObj = this->GetNumberOfAssemblyArrays();
    for ( i = 0; i < numObj; ++i )
    {
      this->SetAssemblyArrayStatus( i, status );
    }
    VTK_FALLTHROUGH;
  case PART:
    numObj = this->GetNumberOfPartArrays();
    for ( i = 0; i < numObj; ++i )
    {
      this->SetPartArrayStatus( i, status );
    }
    VTK_FALLTHROUGH;
  case MATERIAL:
    numObj = this->GetNumberOfMaterialArrays();
    for ( i = 0; i < numObj; ++i )
    {
      this->SetMaterialArrayStatus( i, status );
    }
    VTK_FALLTHROUGH;
  case HIERARCHY:
    numObj = this->GetNumberOfHierarchyArrays();
    for ( i = 0; i < numObj; ++i )
    {
      this->SetHierarchyArrayStatus( i, status );
    }
    break;
  default:
    break;
  }
}

void vtkExodusIIReader::Dump()
{
  vtkIndent indent;
  this->PrintSelf( cout, indent );
}


bool vtkExodusIIReader::FindXMLFile()
{
  // If the XML filename exists and is newer than any existing parser (or there is no parser), reread XML file.
  if (
    ( this->Metadata->Parser && this->Metadata->Parser->GetMTime() < this->XMLFileNameMTime && this->XMLFileName ) ||
    ( ! Metadata->Parser ) )
  {
    if ( Metadata->Parser )
    {
      Metadata->Parser->Delete();
      Metadata->Parser = 0;
    }

    if ( ! this->XMLFileName || ! vtksys::SystemTools::FileExists( this->XMLFileName ) )
    {
      if ( this->FileName )
      {
        vtkStdString baseName( vtksys::SystemTools::GetFilenameWithoutExtension( this->FileName ) );
        vtkStdString xmlExt( baseName + ".xml" );
        if ( vtksys::SystemTools::FileExists( xmlExt ) )
        {
          this->SetXMLFileName( xmlExt.c_str() );
          return true;
        }

        vtkStdString dartExt( baseName + ".dart" );
        if ( vtksys::SystemTools::FileExists( dartExt ) )
        {
          this->SetXMLFileName( dartExt.c_str() );
          return true;
        }

        vtkStdString baseDir( vtksys::SystemTools::GetFilenamePath( this->FileName ) );
        vtkStdString artifact( baseDir + "/artifact.dta" );
        if ( vtksys::SystemTools::FileExists( artifact ) )
        {
          this->SetXMLFileName( artifact.c_str() );
          return true;
        }

        // Catch the case where filename was non-NULL but didn't exist.
        this->SetXMLFileName( 0 );
      }
    }
    else
    {
      return true;
    }
  }

  return false;
}

void vtkExodusIIReader::AdvertiseTimeSteps( vtkInformation* outInfo )
{
  // This method is called in vtkExodusIIReader::RequestInformation() to update
  // information about timesteps. Since this gets called after this->Metadata
  // has processed the file meta-data it's a good place to update iVars that
  // reflect the meta-data read from the file about timesteps/mode shapes.

  int nTimes = static_cast<int>(this->Metadata->Times.size());
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = (nTimes > 0)? (nTimes-1) : 0;

  // Since modeshape range is 1 indexed.
  this->ModeShapesRange[0] = this->TimeStepRange[0] + 1;
  this->ModeShapesRange[1] = this->TimeStepRange[1] + 1;

  if ( ! this->GetHasModeShapes() )
  {
    double timeRange[2];
    if ( nTimes )
    {
      timeRange[0] = this->Metadata->Times[0];
      timeRange[1] = this->Metadata->Times[nTimes - 1];
      outInfo->Set( vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->Metadata->Times[0], nTimes );
      outInfo->Set( vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2 );
    }
  }
  else if (this->GetAnimateModeShapes())
  {
    outInfo->Remove( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    static double timeRange[] = { 0, 1 };
    outInfo->Set( vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2 );
  }
  else
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }
}

void vtkExodusIIReader::Reset()
{
  this->Metadata->Reset();
  this->Metadata->ResetSettings();
}

void vtkExodusIIReader::ResetSettings()
{
  this->Metadata->ResetSettings();
}

void vtkExodusIIReader::SetCacheSize(double CacheSize)
{
  this->Metadata->SetCacheSize(CacheSize);
}

double vtkExodusIIReader::GetCacheSize()
{
  return this->Metadata->GetCacheSize();
}

void vtkExodusIIReader::SetSqueezePoints(bool sp)
{
  this->Metadata->SetSqueezePoints(sp ? 1 : 0);
}

bool vtkExodusIIReader::GetSqueezePoints()
{
  return this->Metadata->GetSqueezePoints() != 0;
}

void vtkExodusIIReader::ResetCache()
{
  this->Metadata->ResetCache();
}
