/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridSource.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridSource.h"

#include "vtkBitArray.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeCursor.h"
#include "vtkHyperTreeGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkQuadric.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <sstream>

#include <cassert>

vtkStandardNewMacro(vtkHyperTreeGridSource);
vtkCxxSetObjectMacro(vtkHyperTreeGridSource, DescriptorBits, vtkBitArray);
vtkCxxSetObjectMacro(vtkHyperTreeGridSource, MaterialMaskBits, vtkBitArray);
vtkCxxSetObjectMacro(vtkHyperTreeGridSource, Quadric, vtkQuadric);


//----------------------------------------------------------------------------
vtkHyperTreeGridSource::vtkHyperTreeGridSource()
{
  // This a source: no input ports
  this->SetNumberOfInputPorts( 0 );

  // Grid parameters
  this->BranchFactor = 2;
  this->MaximumLevel = 1;
  this->BlockSize = 0;

  // Grid topology
  this->Dimension = 3;
  this->GridSize[0] = 1;
  this->GridSize[1] = 1;
  this->GridSize[2] = 1;
  this->TransposedRootIndexing = false;

  // Grid geometry
  this->Origin[0] = 0.;
  this->Origin[1] = 0.;
  this->Origin[2] = 0.;
  this->GridScale[0] = 1.;
  this->GridScale[1] = 1.;
  this->GridScale[2] = 1.;
  this->XCoordinates = vtkDoubleArray::New();
  this->XCoordinates->SetNumberOfTuples( 2 );
  this->XCoordinates->SetComponent( 0, 0, 0. );
  this->XCoordinates->SetComponent( 1, 0, this->GridScale[0] );
  this->YCoordinates = vtkDoubleArray::New();
  this->YCoordinates->SetNumberOfTuples( 2 );
  this->YCoordinates->SetComponent( 0, 0, 0. );
  this->YCoordinates->SetComponent( 1, 0, this->GridScale[1] );
  this->ZCoordinates = vtkDoubleArray::New();
  this->ZCoordinates->SetNumberOfTuples( 2 );
  this->ZCoordinates->SetComponent( 0, 0, 0. );
  this->ZCoordinates->SetComponent( 1, 0, this->GridScale[2] );

  // By default use the descriptor string
  this->UseDescriptor = true;

  // By default do not use the material mask
  this->UseMaterialMask = false;

  // Grid description & material mask as strings
  this->Descriptor = new char[2];
  this->Descriptor[0] = '.';
  this->Descriptor[1] = 0;
  this->MaterialMask = new char[2];
  this->MaterialMask[0] = '0';
  this->MaterialMask[1] = 0;

  // Grid description & material mask as bit arrays
  this->DescriptorBits = 0;
  this->MaterialMaskBits = 0;
  this->LevelZeroMaterialIndex = 0;
  this->LevelZeroMaterialMap.clear();

  // Default quadric is a sphere with radius 1
  this->Quadric = vtkQuadric::New();
  this->Quadric->SetCoefficients( 1., 1., 1.,
                                  0., 0., 0.,
                                  0., 0., 0.,
                                  -1. );

  // Keep reference to hyper tree grid output
  this->Output = 0;
}

//----------------------------------------------------------------------------
vtkHyperTreeGridSource::~vtkHyperTreeGridSource()
{
  if ( this->XCoordinates )
    {
    this->XCoordinates->UnRegister( this );
    this->XCoordinates = 0;
    }

  if ( this->YCoordinates )
    {
    this->YCoordinates->UnRegister( this );
    this->YCoordinates = 0;
    }

  if ( this->ZCoordinates )
    {
    this->ZCoordinates->UnRegister( this );
    this->ZCoordinates = 0;
    }

  if ( this->DescriptorBits )
    {
    this->DescriptorBits->UnRegister( this );
    this->DescriptorBits = 0;
    }

  if ( this->MaterialMaskBits )
    {
    this->MaterialMaskBits->UnRegister( this );
    this->MaterialMaskBits = 0;
    }

  if ( this->LevelZeroMaterialIndex )
    {
    this->LevelZeroMaterialIndex->UnRegister( this );
    this->LevelZeroMaterialIndex = 0;
    }

  this->LevelZeroMaterialMap.clear();

  delete [] this->Descriptor;
  this->Descriptor = 0;

  delete [] this->MaterialMask;
  this->MaterialMask = 0;

  if ( this->Quadric )
    {
    this->Quadric->UnRegister( this );
    this->Quadric = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridSource::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "GridSize: "
     << this->GridSize[0] <<","
     << this->GridSize[1] <<","
     << this->GridSize[2] << endl;

  os << indent << "Origin: "
     << this->Origin[0] <<","
     << this->Origin[1] <<","
     << this->Origin[2] << endl;

  os << indent << "GridScale: "
     << this->GridScale[0] <<","
     << this->GridScale[1] <<","
     << this->GridScale[2] << endl;

  os << indent << "MaximumLevel: " << this->MaximumLevel << endl;
  os << indent << "Dimension: " << this->Dimension << endl;
  os << indent << "BranchFactor: " << this->BranchFactor << endl;
  os << indent << "BlockSize: " << this->BlockSize << endl;
  os << indent << "TransposedRootIndexing: " << this->TransposedRootIndexing << endl;

  if ( this->XCoordinates )
    {
    this->XCoordinates->PrintSelf( os, indent.GetNextIndent() );
    }
  if ( this->YCoordinates )
    {
    this->YCoordinates->PrintSelf( os, indent.GetNextIndent() );
    }
  if ( this->ZCoordinates )
    {
    this->ZCoordinates->PrintSelf( os, indent.GetNextIndent() );
    }

  os << indent << "UseDescriptor: " << this->UseDescriptor << endl;
  os << indent << "UseMaterialMask: " << this->UseMaterialMask << endl;
  os << indent << "Descriptor: " << this->Descriptor << endl;
  os << indent << "MaterialMask: " << this->Descriptor << endl;
  os << indent << "LevelDescriptors: " << this->LevelDescriptors.size() << endl;
  os << indent << "LevelMaterialMasks: " << this->LevelMaterialMasks.size() << endl;
  os << indent << "LevelCounters: " << this->LevelCounters.size() << endl;

  if ( this->Quadric )
    {
    this->Quadric->PrintSelf( os, indent.GetNextIndent() );
    }

  os << indent
     << "Output: ";
  if ( this->Output )
    {
    this->Output->PrintSelf( os, indent );
    }
  else
    {
    os << "(none)" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridSource::SetIndexingModeToKJI()
{
  this->SetTransposedRootIndexing( false );
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridSource::SetIndexingModeToIJK()
{
  this->SetTransposedRootIndexing( true );
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridSource::SetLevelZeroMaterialIndex( vtkIdTypeArray* indexArray )
{
  if ( this->LevelZeroMaterialIndex == indexArray )
    {
    return;
    }

  if ( this->LevelZeroMaterialIndex )
    {
    this->LevelZeroMaterialIndex->UnRegister( this );
    }

  this->LevelZeroMaterialIndex = indexArray;
  this->LevelZeroMaterialIndex->Register( this );

  this->LevelZeroMaterialMap.clear();
  vtkIdType len = indexArray->GetNumberOfTuples();
  // Fill the map index - key is leaf number, value is index in the array that
  // will be used to fetch the descriptor value.
  for ( vtkIdType i = 0; i < len; ++ i )
    {
    this->LevelZeroMaterialMap[ indexArray->GetValue( i ) ] = i;
    }
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
// Return the maximum number of levels of the hypertree.
// \post positive_result: result>=1
unsigned int vtkHyperTreeGridSource::GetMaximumLevel()
{
  assert( "post: positive_result" && this->MaximumLevel >= 1 );
  return this->MaximumLevel;
}

//----------------------------------------------------------------------------
// Description:
// Set the maximum number of levels of the hypertrees. If
// GetMinLevels()>=levels, GetMinLevels() is changed to levels-1.
// \pre positive_levels: levels>=1
// \post is_set: this->GetLevels()==levels
// \post min_is_valid: this->GetMinLevels()<this->GetLevels()
void vtkHyperTreeGridSource::SetMaximumLevel( unsigned int levels )
{
  if ( levels < 1 )
    {
    levels = 1;
    }

  if ( this->MaximumLevel == levels )
    {
    return;
    }

  this->MaximumLevel = levels;
  this->Modified();

  assert( "post: is_set" && this->GetMaximumLevel() == levels );
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridSource::RequestInformation( vtkInformation*,
                                                vtkInformationVector**,
                                                vtkInformationVector* outputVector )
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // We cannot give the exact number of levels of the hypertrees
  // because it is not generated yet and this process depends on the recursion formula.
  // Just send an upper limit instead.
  outInfo->Set( vtkHyperTreeGrid::LEVELS(), this->MaximumLevel );
  outInfo->Set( vtkHyperTreeGrid::DIMENSION(), this->Dimension );

  double origin[3];
  origin[0] = this->XCoordinates->GetTuple1( 0 );
  origin[1] = this->YCoordinates->GetTuple1( 0 );
  origin[2] = this->ZCoordinates->GetTuple1( 0 );
  outInfo->Set( vtkDataObject::ORIGIN(), origin, 3 );

  int extent[6];
  extent[0] = 0;
  extent[1] = this->GridSize[0] - 1;
  extent[2] = 0;
  extent[3] = this->GridSize[1] - 1;
  extent[4] = 0;
  extent[5] = this->GridSize[2] - 1;
  outInfo->Set( vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6 );

  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridSource::RequestData( vtkInformation*,
                                         vtkInformationVector**,
                                         vtkInformationVector* outputVector )
{
  // Retrieve the output
  vtkInformation *outInfo = outputVector->GetInformationObject( 0 );
  this->Output =
    vtkHyperTreeGrid::SafeDownCast( outInfo->Get(vtkDataObject::DATA_OBJECT()) );
  if ( ! this->Output )
    {
    return 0;
    }
  vtkPointData* outData = this->Output->GetPointData();

  // TODO: add support for update extent
  //int updateExtent[6];
  //outInfo->Get( vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent );

  this->LevelBitsIndexCnt.clear();
  this->LevelBitsIndexCnt.push_back(0);

  // When using descriptor-based definition, initialize descriptor parsing
  if ( this->UseDescriptor )
    {
    // Calculate refined block size
    this->BlockSize = this->BranchFactor;
    for ( unsigned int i = 1; i < this->Dimension; ++ i )
      {
      this->BlockSize *= this->BranchFactor;
      }

    if ( ! this->DescriptorBits && ! this->InitializeFromStringDescriptor() )
      {
      return 0;
      }
    else if ( this->DescriptorBits && ! this->InitializeFromBitsDescriptor() )
      {
      return 0;
      }
    }

  // Set grid parameters
  this->Output->SetGridSize( this->GridSize );
  this->Output->SetTransposedRootIndexing( this->TransposedRootIndexing );
  this->Output->SetDimension( this->Dimension );
  this->Output->SetBranchFactor( this->BranchFactor );
  this->Output->SetMaterialMaskIndex( this->LevelZeroMaterialIndex );
  this->Output->GenerateTrees();

  // Create geometry
  for ( unsigned int i = 0; i < 3; ++ i )
    {
    vtkNew<vtkDoubleArray> coords;
    unsigned int n = this->GridSize[i] + 1;
    coords->SetNumberOfValues( n );
    for ( unsigned int j = 0; j < n; ++ j )
      {
      double coord = this->Origin[i] + this->GridScale[i] * static_cast<double>( j );
      coords->SetValue( j, coord );
      }

    switch ( i )
      {
      case 0:
        this->Output->SetXCoordinates( coords.GetPointer() );
        break;
      case 1:
        this->Output->SetYCoordinates( coords.GetPointer() );
        break;
      case 2:
        this->Output->SetZCoordinates( coords.GetPointer() );
        break;
      default:
        break;
      }
    }

  // Prepare array of doubles for depth values
  vtkNew<vtkDoubleArray> depthArray;
  depthArray->SetName( "Depth" );
  depthArray->SetNumberOfComponents( 1 );
  vtkIdType fact = 1;
  for ( unsigned int i = 1; i < this->MaximumLevel; ++ i )
    {
    fact *= this->BranchFactor;
    }
  fact *= fact;
  depthArray->Allocate( fact );
  outData->SetScalars( depthArray.GetPointer() );

  if ( ! this->UseDescriptor )
    {
    // Prepare array of doubles for quadric values
    vtkNew<vtkDoubleArray> quadricArray;
    quadricArray->SetName( "Quadric" );
    quadricArray->SetNumberOfComponents( 1 );

    quadricArray->Allocate( fact );
    outData->AddArray( quadricArray.GetPointer() );
    }

  // Iterate over all hyper trees
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeIterator it;
  this->Output->InitializeTreeIterator( it );
  while ( vtkHyperTree* tree = it.GetNextTree( index ) )
    {
    unsigned int i, j, k;
    this->Output->GetLevelZeroCoordsFromIndex( index, i, j, k );

    // Initialize cursor
    vtkHyperTreeCursor* cursor = this->Output->NewCursor( index );
    if ( !cursor )
      {
      continue;
      }
    cursor->ToRoot();

    // Initialize local cell index
    int idx[3] = { 0, 0, 0 };

    if ( this->UseDescriptor )
      {
      this->InitTreeFromDescriptor( cursor, index, idx );
      }
    else
      {
      // Initialize the tree global start index with the number of
      // points added so far. This avoid the storage of a local
      // to global node id per tree.
      tree->SetGlobalIndexStart( this->LevelBitsIndexCnt[0] );

      // Initialize coordinate system for implicit function
      double origin[3];
      origin[0] = ( i % this->GridSize[0] ) * this->GridScale[0];
      origin[1] = ( j % this->GridSize[1] ) * this->GridScale[1];
      origin[2] = ( k % this->GridSize[2] ) * this->GridScale[2];
      // Subdivide based on quadric implicit function
      this->SubdivideFromQuadric( cursor, 0, index, idx, origin, this->GridScale );
      }

    // Clean up
    cursor->UnRegister( this );
    } // it

  // Squeeze output data arrays
  for ( int a = 0; a < outData->GetNumberOfArrays(); ++ a )
    {
    outData->GetArray( a )->Squeeze();
    }

  assert( "post: dataset_and_data_size_match" && this->Output->CheckAttributes() == 0 );

  this->LevelBitsIndexCnt.clear();
  this->LevelBitsIndex.clear();

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridSource::InitTreeFromDescriptor( vtkHyperTreeCursor* cursor,
                                                      int treeIdx,
                                                      int idx[3])
{
  // Subdivide using descriptor
  if ( ! this->DescriptorBits )
    {
    this->SubdivideFromStringDescriptor( cursor, 0, treeIdx, 0, idx, 0 );
    }
  else
    {
    this->SubdivideFromBitsDescriptor( cursor, 0, treeIdx, 0, idx, 0 );
    }
}
//-----------------------------------------------------------------------------
int vtkHyperTreeGridSource::InitializeFromStringDescriptor()
{
  size_t descLen = strlen( this->Descriptor );

  // Verify that grid and material specifications are consistent
  if ( this->UseMaterialMask
        && strlen( this->MaterialMask ) != descLen )
    {
    vtkErrorMacro(<<"Material mask is used but has length "
                  << strlen( this->MaterialMask )
                  << " != "
                  << descLen
                  << " which is the length of the grid descriptor.");

    return 0;
    }

  // Calculate total level 0 grid size
  unsigned int nTotal = this->GridSize[0] * this->GridSize[1] * this->GridSize[2];

  // Parse string descriptor and material mask if used
  unsigned int nRefined = 0;
  unsigned int nLeaves = 0;
  unsigned int nNextLevel = nTotal;
  bool rootLevel = true;
  std::ostringstream descriptor;
  std::ostringstream mask;

  for ( size_t i = 0; i < descLen; ++ i )
    {
    char c = this->Descriptor[i];
    char m = this->UseMaterialMask ? this->MaterialMask[i] : 0;
    switch ( c )
      {
      case ' ':
        // Space is allowed as separator, verify mask consistenty if needed
        if ( this->UseMaterialMask && m != ' ' )
          {
          vtkErrorMacro(<<"Space separators do not match between "
            "descriptor and material mask.");
          return 0;
          }
        break; // case ' '

      case '|':
        //  A level is complete, verify mask consistenty if needed
        if ( this->UseMaterialMask && m != '|' )
          {
          vtkErrorMacro(<<"Level separators do not match between "
            "descriptor and material mask.");
          return 0;
          }

        // Store descriptor and material mask for current level
        this->LevelDescriptors.push_back( descriptor.str().c_str() );
        this->LevelMaterialMasks.push_back( mask.str().c_str() );

        // Check whether cursor is still at rool level
        if ( rootLevel )
          {
          rootLevel = false;

          // Verify that total number of root cells is consistent with descriptor
          if ( nRefined + nLeaves != nTotal )
            {
            vtkErrorMacro(<<"String "
                          << this->Descriptor
                          << " describes "
                          << nRefined + nLeaves
                          << " root cells != "
                          << nTotal);
            return 0;
            }
          } // if ( rootLevel )
        else
          {
          // Verify that level descriptor cardinality matches expected value
          if ( descriptor.str().size() != nNextLevel )
            {
            vtkErrorMacro(<<"String level descriptor "
                          << descriptor.str().c_str()
                          << " has cardinality "
                          << descriptor.str().size()
                          << " which is not expected value of "
                          << nNextLevel);

            return 0;
            }
          } // else

        // Predict next level descriptor cardinality
        nNextLevel = nRefined * this->BlockSize;

        // Reset per level values
        descriptor.str( "" );
        mask.str( "" );
        nRefined = 0;
        nLeaves = 0;
        break; // case '|'

      case '1':
      case 'R':
        //  Refined cell, verify mask consistenty if needed
        if ( this->UseMaterialMask && m == '0' )
          {
          vtkErrorMacro(<<"A refined branch must contain material.");
          return 0;
          }
        // Refined cell, update branch counter
        ++ nRefined;

        // Append characters to per level descriptor and material mask if used
        descriptor << c;
        if ( this->UseMaterialMask )
          {
          mask << m;
          }
        break; // case 'R'

      case '0':
      case '.':
        // Leaf cell, update leaf counter
        ++ nLeaves;

        // Append characters to per level descriptor and material mask if used
        descriptor << c;
        if ( this->UseMaterialMask )
          {
          mask << m;
          }
        break; // case '.'

      default:
        vtkErrorMacro(<< "Unrecognized character: "
                      << c
                      << " at pos " << i << " in descriptor "
                      << this->Descriptor);

        return 0; // default
      } // switch( c )
    } // c

  // Verify and append last level string
  if ( descriptor.str().size() != nNextLevel )
    {
    vtkErrorMacro(<<"String level descriptor "
                  << descriptor.str().c_str()
                  << " has cardinality "
                  << descriptor.str().size()
                  << " which is not expected value of "
                  << nNextLevel);

    return 0;
    }

  // Push per-level descriptor and material mask if used
  this->LevelDescriptors.push_back( descriptor.str().c_str() );
  if ( this->UseMaterialMask )
    {
    this->LevelMaterialMasks.push_back( mask.str().c_str() );
    }

  // Reset maximum depth if fewer levels are described
  unsigned int nLevels =
    static_cast<unsigned int>( this->LevelDescriptors.size() );
  if ( nLevels < this->MaximumLevel )
    {
    this->MaximumLevel = nLevels;
    }

  // Create vector of counters as long as tree depth
  for ( unsigned int i = 0; i < nLevels; ++ i )
    {
    this->LevelCounters.push_back( 0 );
    }

  this->LevelBitsIndex.clear();
  this->LevelBitsIndex.push_back(0);
  for ( unsigned int i = 1; i < nLevels; ++ i )
    {
    this->LevelBitsIndex.push_back(
      LevelBitsIndex[i-1] + this->LevelDescriptors[i-1].length());
    }
  this->LevelBitsIndexCnt = this->LevelBitsIndex;

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridSource::SubdivideFromStringDescriptor(
  vtkHyperTreeCursor* cursor, unsigned int level, int treeIdx, int childIdx,
  int idx[3], int parentPos )
{
  // Get handle on leaf scalar data
  vtkDataArray* depthArray = this->Output->GetPointData()->GetArray( "Depth" );

  // Calculate pointer into level descriptor string
  int pointer = level ? childIdx + parentPos * this->BlockSize : treeIdx;

  // Calculate the node global index
  vtkIdType id = this->LevelBitsIndexCnt[level];
  this->LevelBitsIndexCnt[level]++;
  // Cell value: depth level
  depthArray->InsertTuple1( id, level );
  cursor->GetTree()->SetGlobalIndexFromLocal( cursor->GetNodeId(), id );

  // Subdivide further or stop recursion with terminal leaf
  if ( level + 1 < this->MaximumLevel
       && this->LevelDescriptors.at( level ).at( pointer ) == 'R' )
    {
    // Subdivide hyper tree grid leaf
    this->Output->SubdivideLeaf( cursor, treeIdx );

    // Now traverse to children
    int xDim = ( this->Dimension >= 1 ) ? this->BranchFactor : 1;
    int yDim = ( this->Dimension >= 2 ) ? this->BranchFactor : 1;
    int zDim = ( this->Dimension >= 3 ) ? this->BranchFactor : 1;

    int newChildIdx = 0;
    int newIdx[3];
    for ( int z = 0; z < zDim; ++ z )
      {
      newIdx[2] = idx[2] * zDim + z;
      for ( int y = 0; y < yDim; ++ y )
        {
        newIdx[1] = idx[1] * yDim + y;
        for ( int x = 0; x < xDim; ++ x )
          {
          newIdx[0] = idx[0] * xDim + x;

          // Set cursor to child
          cursor->ToChild( newChildIdx );

          // Recurse
          this->SubdivideFromStringDescriptor( cursor, level + 1, treeIdx,
            newChildIdx, newIdx, this->LevelCounters.at( level ) );

          // Reset cursor to parent
          cursor->ToParent();

          // Increment child index
          ++ newChildIdx;
          } // x
        } // y
      } // z

    // Increment current level counter
    ++ this->LevelCounters.at( level );
    } // if ( subdivide )
  else
    {
    bool isMasked = ( this->UseMaterialMask
         && this->LevelMaterialMasks.at( level ).at( pointer ) == '0' );
    // Blank leaf if needed
    this->Output->GetMaterialMask()->InsertTuple1( id, isMasked ? 1 : 0 );
    } // else
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridSource::InitializeFromBitsDescriptor()
{
  // Verify that grid and material specifications are consistent
  if ( this->UseMaterialMask && ! this->LevelZeroMaterialIndex
    && this->MaterialMaskBits->GetSize() != this->DescriptorBits->GetSize() )
    {
    vtkErrorMacro(<<"Material mask is used but has length "
                  << this->MaterialMaskBits->GetSize() << " != "
                  << this->DescriptorBits->GetSize()
                  << " which is the length of the grid descriptor.");

    return 0;
    }

  // Calculate total level 0 grid size
  vtkIdType nTotal = this->LevelZeroMaterialIndex ?
    this->LevelZeroMaterialMap.size() :
    this->GridSize[0] * this->GridSize[1] * this->GridSize[2];

  // Parse descriptor and material mask if used
  this->LevelBitsIndex.clear();
  this->LevelBitsIndex.push_back(0);
  vtkIdType nRefined = 0;
  vtkIdType nLeaves = 0;
  vtkIdType nNextLevel = nTotal;
  vtkIdType nCurrentLevelCount = 0;
  vtkIdType descSize = this->DescriptorBits->GetNumberOfTuples();
  unsigned int nCurrentLevel = this->LevelZeroMaterialIndex ? 1 : 0;

  for ( vtkIdType i = 0; i < descSize; ++i )
    {
    if ( nCurrentLevelCount >= nNextLevel )
      {
      nNextLevel = nRefined * this->BlockSize;
      nRefined = 0;
      nLeaves = 0;
      nCurrentLevelCount = 0;
      nCurrentLevel++;
      this->LevelBitsIndex.push_back(i);
      }
    nRefined += this->DescriptorBits->GetValue(i);
    nLeaves += this->DescriptorBits->GetValue(i) == 0 ? 1 : 0;

    nCurrentLevelCount++;
    }

  this->LevelBitsIndexCnt = this->LevelBitsIndex;

  // Verify and append last level string
  if ( nCurrentLevelCount != nNextLevel )
    {
    vtkErrorMacro(<<"Level descriptor " << nCurrentLevel << " has cardinality "
                  << nCurrentLevelCount << " which is not expected value of "
                  << nNextLevel);

    return 0;
    }

  nCurrentLevel++;

  this->LevelBitsIndexCnt = this->LevelBitsIndex;

  // Reset maximum depth if fewer levels are described
  if ( nCurrentLevel < this->MaximumLevel )
    {
    this->MaximumLevel = nCurrentLevel;
    }

  // Create vector of counters as long as tree depth
  for ( unsigned int i = 0; i < nCurrentLevel; ++ i )
    {
    this->LevelCounters.push_back( 0 );
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridSource::SubdivideFromBitsDescriptor(
  vtkHyperTreeCursor* cursor, unsigned int level, int treeIdx, int childIdx,
  int idx[3], int parentPos )
{
  // Get handle on leaf scalar data
  vtkDataArray* depthArray = this->Output->GetPointData()->GetArray( "Depth" );

  vtkIdType startIdx = this->LevelBitsIndex[level];
  int pointer = level ? childIdx + parentPos * this->BlockSize : treeIdx;

  // Calculate the node global index
  vtkIdType id = this->LevelBitsIndexCnt[level];
  this->LevelBitsIndexCnt[level]++;

  // Cell value: depth level
  depthArray->InsertTuple1( id, level );

  // Set the global index of the node
  cursor->GetTree()->SetGlobalIndexFromLocal( cursor->GetNodeId(), id );

  bool refine = false;

  if ( this->LevelZeroMaterialIndex && level == 0 )
    {
    if ( this->LevelZeroMaterialMap.find( treeIdx ) !=
      this->LevelZeroMaterialMap.end() )
      {
       refine = this->DescriptorBits->GetValue(
         this->LevelZeroMaterialMap[ treeIdx ] ) == 1;
      }
    }
  else
    {
    // Calculate pointer into level descriptor string

    refine = this->DescriptorBits->GetValue( startIdx + pointer ) == 1;
    }

  // Subdivide further or stop recursion with terminal leaf
  if ( level + 1 < this->MaximumLevel && refine )
    {
    // Subdivide hyper tree grid leaf
    this->Output->SubdivideLeaf( cursor, treeIdx );

    // Now traverse to children
    int xDim = ( this->Dimension >= 1 ) ? this->BranchFactor : 1;
    int yDim = ( this->Dimension >= 2 ) ? this->BranchFactor : 1;
    int zDim = ( this->Dimension >= 3 ) ? this->BranchFactor : 1;

    int newChildIdx = 0;
    int newIdx[3];
    for ( int z = 0; z < zDim; ++ z )
      {
      newIdx[2] = idx[2] * zDim + z;
      for ( int y = 0; y < yDim; ++ y )
        {
        newIdx[1] = idx[1] * yDim + y;
        for ( int x = 0; x < xDim; ++ x )
          {
          newIdx[0] = idx[0] * xDim + x;

          // Set cursor to child
          cursor->ToChild( newChildIdx );

          // Recurse
          this->SubdivideFromBitsDescriptor(
            cursor, level + 1, treeIdx, newChildIdx, newIdx,
            this->LevelCounters.at( level ) );

          // Reset cursor to parent
          cursor->ToParent();

          // Increment child index
          ++ newChildIdx;
          } // x
        } // y
      } // z

    // Increment current level counter
    ++ this->LevelCounters.at( level );

    this->Output->GetMaterialMask()->InsertTuple1( id, 0 );
    } // if ( subdivide )
  else
    {
    bool isMasked = false;

    if ( this->UseMaterialMask  )
      {
      if ( this->LevelZeroMaterialIndex )
        {
        isMasked = ( level == 0 ) ? false : this->MaterialMaskBits->GetValue(
          startIdx - this->LevelBitsIndex[1] + pointer ) == 0;
        }
      else
        {
        isMasked = this->MaterialMaskBits->GetValue( startIdx + pointer ) == 0;
        }
      }
    // Blank leaf if needed
    this->Output->GetMaterialMask()->InsertTuple1( id, isMasked ? 1 : 0 );
    } // else
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridSource::SubdivideFromQuadric( vtkHyperTreeCursor* cursor,
                                                   unsigned int level,
                                                   int treeIdx,
                                                   const int idx[3],
                                                   double origin[3],
                                                   double size[3] )
{
  // Get handle on leaf scalar data
  vtkPointData* outData = this->Output->GetPointData();
  vtkDataArray* depthArray = outData->GetArray( "Depth" );
  vtkDataArray* quadricArray = outData->GetArray( "Quadric" );

    // Calculate the node global index
  vtkIdType id =
    cursor->GetTree()->GetGlobalIndexFromLocal( cursor->GetNodeId() );
  this->LevelBitsIndexCnt[0]++;

  // Compute cell origin coordinates
  double O[] = { 0., 0., 0. };
  for ( unsigned int d = 0; d < this->Dimension; ++ d )
    {
    O[d] = origin[d] + idx[d] * size[d];
    }

  // Iterate over all vertices
  int nPos = 0;
  int nNeg = 0;
  double sum = 0.;
  double nVert = 1 << this->Dimension;
  for ( int v = 0; v < nVert; ++ v )
    {
    // Transform flat index into triple
    div_t d1 = div( v, 2 );
    div_t d2 = div( d1.quot, 2 );

    // Compute vertex coordinates
    double pt[3];
    pt[0] = O[0] + d1.rem * size[0];
    pt[1] = O[1] + d2.rem * size[1];
    pt[2] = O[2] + d2.quot * size[2];

    // Evaluate quadric at current vertex
    double qv = this->Quadric->EvaluateFunction( pt );
    if ( qv > 0 )
      {
      // Found positive value at this vertex
      ++ nPos;

      // Update integral
      sum += qv;
      }
    else if ( qv < 0 )
      {
      // Found negative value at this vertex
      ++ nNeg;

      // Update integral
      sum += qv;
      }
    } // v

  // Subdivide iff quadric changes sign within cell
  bool subdivide = ( nPos != nVert && nNeg != nVert ) ? true : false;

  // Assign cell value
  if ( subdivide && level + 1 == this->MaximumLevel )
    {
    // Intersecting cells at deepest level are 0-set
    sum = 0.;
    }
  else
    {
    // Cell value is average of all corner quadric values
    sum /= nVert;
    }

  // Cell value: depth level
  depthArray->InsertTuple1( id, level );

  // Subdivide further or stop recursion with terminal leaf
  if ( subdivide && level + 1 < this->MaximumLevel )
    {
    // Cell is subdivided so it cannot be masked
    this->Output->GetMaterialMask()->InsertTuple1( id, 0 );

    // Subdivide hyper tree grid leaf
    this->Output->SubdivideLeaf( cursor, treeIdx );

    // Now traverse to children
    int xDim = this->BranchFactor;
    int yDim = this->Dimension > 1 ? this->BranchFactor : 1;
    int zDim = this->Dimension > 2 ? this->BranchFactor : 1;
    double newSize[] = { 0., 0., 0. };
    switch ( this->Dimension )
      {
      // Warning: Run through is intended! Do NOT add break statements
      case 3:
        newSize[2] = size[2] / this->BranchFactor;
      case 2:
        newSize[1] = size[1] / this->BranchFactor;
      case 1:
        newSize[0] = size[0] / this->BranchFactor;
      }

    int newChildIdx = 0;
    int newIdx[3];
    for ( int z = 0; z < zDim; ++ z )
      {
      newIdx[2] = idx[2] * zDim + z;
      for ( int y = 0; y < yDim; ++ y )
        {
        newIdx[1] = idx[1] * yDim + y;
        for ( int x = 0; x < xDim; ++ x )
          {
          newIdx[0] = idx[0] * xDim + x;

          // Set cursor to child
          cursor->ToChild( newChildIdx );

          // Recurse
          this->SubdivideFromQuadric( cursor, level + 1, treeIdx, newIdx,
                                      origin, newSize );

          // Reset cursor to parent
          cursor->ToParent();

          // Increment child index
          ++ newChildIdx;
          } // x
        } // y
      } // z
    } // if ( subdivide )
  else
    {
    bool isMasked = this->UseMaterialMask && nPos > 0;

    // Blank leaf if needed
    this->Output->GetMaterialMask()->InsertTuple1( id, isMasked ? 1 : 0 );

    // Cell values: depth level and quadric function value
    depthArray->InsertTuple1( id, level );
    quadricArray->InsertTuple1( id, sum );
    } // else
}
//-----------------------------------------------------------------------------
void vtkHyperTreeGridSource::SetQuadricCoefficients( double a[10] )
{
  if ( ! this->Quadric )
    {
    this->Quadric = vtkQuadric::New();
    }
  this->Quadric->SetCoefficients( a );
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridSource::GetQuadricCoefficients( double a[10] )
{
  this->Quadric->GetCoefficients( a );
}

//-----------------------------------------------------------------------------
double* vtkHyperTreeGridSource::GetQuadricCoefficients()
{
  return this->Quadric->GetCoefficients();
}

//----------------------------------------------------------------------------
unsigned long vtkHyperTreeGridSource::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();

  if ( this->Quadric )
    {
    unsigned long time = this->Quadric->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
vtkBitArray* vtkHyperTreeGridSource::ConvertDescriptorStringToBitArray(
  const std::string& str )
{
  vtkBitArray* desc = vtkBitArray::New();
  desc->Allocate( str.length() );
  for ( std::string::const_iterator dit = str.begin();
    dit != str.end();  ++ dit )
    {
    switch ( *dit )
      {
      case '_':
      case '-':
      case ' ':
      case '|':
        break;

      case '1':
      case 'R':
        //  Refined cell
        desc->InsertNextValue(1);
        break;

      case '0':
      case '.':
        // Leaf cell
        desc->InsertNextValue(0);
        break;

      default:
        vtkErrorMacro(<< "Unrecognized character: "
                      << *dit
                      << " in string "
                      << str);
        desc->Delete();
        return 0;
      } // switch( *dit )
    }
  desc->Squeeze();
  return desc;
}

//----------------------------------------------------------------------------
vtkBitArray* vtkHyperTreeGridSource::ConvertMaterialMaskStringToBitArray(
  const std::string& str )
{
  return ConvertDescriptorStringToBitArray( str );
}
