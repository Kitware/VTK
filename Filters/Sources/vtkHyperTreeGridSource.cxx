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
  this->Orientation = 0;
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
  this->XCoordinates->SetComponent( 0, 0., 0. );
  this->XCoordinates->SetComponent( 1, 0., this->GridScale[0] );
  this->YCoordinates = vtkDoubleArray::New();
  this->YCoordinates->SetNumberOfTuples( 2 );
  this->YCoordinates->SetComponent( 0, 0., 0. );
  this->YCoordinates->SetComponent( 1, 0., this->GridScale[1] );
  this->ZCoordinates = vtkDoubleArray::New();
  this->ZCoordinates->SetNumberOfTuples( 2 );
  this->ZCoordinates->SetComponent( 0, 0., 0. );
  this->ZCoordinates->SetComponent( 1, 0., this->GridScale[2] );

  // By default use the descriptor string
  this->UseDescriptor = true;

  // By default do not use the material mask
  this->UseMaterialMask = false;

  // By default do not generate interface vector fields
  this->GenerateInterfaceFields = false;

  // Grid description & material mask as strings
  this->Descriptor = new char[2];
  this->Descriptor[0] = '.';
  this->Descriptor[1] = 0;
  this->MaterialMask = new char[2];
  this->MaterialMask[0] = '0';
  this->MaterialMask[1] = 0;

  // Grid description & material mask as bit arrays
  this->DescriptorBits = nullptr;
  this->MaterialMaskBits = nullptr;
  this->LevelZeroMaterialIndex = nullptr;
  this->LevelZeroMaterialMap.clear();

  // Default quadric is a sphere with radius 1 centered at origin
  this->Quadric = vtkQuadric::New();
  this->Quadric->SetCoefficients( 1., 1., 1.,
                                  0., 0., 0.,
                                  0., 0., 0.,
                                  -1. );
}

//----------------------------------------------------------------------------
vtkHyperTreeGridSource::~vtkHyperTreeGridSource()
{
  if ( this->XCoordinates )
  {
    this->XCoordinates->UnRegister( this );
    this->XCoordinates = nullptr;
  }

  if ( this->YCoordinates )
  {
    this->YCoordinates->UnRegister( this );
    this->YCoordinates = nullptr;
  }

  if ( this->ZCoordinates )
  {
    this->ZCoordinates->UnRegister( this );
    this->ZCoordinates = nullptr;
  }

  if ( this->DescriptorBits )
  {
    this->DescriptorBits->UnRegister( this );
    this->DescriptorBits = nullptr;
  }

  if ( this->MaterialMaskBits )
  {
    this->MaterialMaskBits->UnRegister( this );
    this->MaterialMaskBits = nullptr;
  }

  if ( this->LevelZeroMaterialIndex )
  {
    this->LevelZeroMaterialIndex->UnRegister( this );
    this->LevelZeroMaterialIndex = nullptr;
  }

  this->LevelZeroMaterialMap.clear();

  delete [] this->Descriptor;
  this->Descriptor = nullptr;

  delete [] this->MaterialMask;
  this->MaterialMask = nullptr;

  if ( this->Quadric )
  {
    this->Quadric->UnRegister( this );
    this->Quadric = nullptr;
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
  os << indent << "Orientation: " << this->Orientation << endl;
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
  os << indent << "GenerateInterfaceFields:" << this->GenerateInterfaceFields << endl;

  os << indent << "LevelZeroMaterialIndex: " << this->LevelZeroMaterialIndex << endl;
  os << indent << "Descriptor: " << this->Descriptor << endl;
  os << indent << "MaterialMask: " << this->MaterialMask << endl;
  os << indent << "LevelDescriptors: " << this->LevelDescriptors.size() << endl;
  os << indent << "LevelMaterialMasks: " << this->LevelMaterialMasks.size() << endl;
  os << indent << "LevelCounters: " << this->LevelCounters.size() << endl;

  if ( this->Quadric )
  {
    this->Quadric->PrintSelf( os, indent.GetNextIndent() );
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
unsigned int vtkHyperTreeGridSource::GetMaximumLevel()
{
  assert( "post: positive_result" && this->MaximumLevel >= 1 );
  return this->MaximumLevel;
}

//----------------------------------------------------------------------------
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
int vtkHyperTreeGridSource::FillOutputPortInformation( int, vtkInformation* info )
{
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid" );
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridSource::RequestInformation( vtkInformation*,
                                                vtkInformationVector**,
                                                vtkInformationVector* outputVector )
{
  // Get the information objects
  vtkInformation* outInfo = outputVector->GetInformationObject( 0 );

  // We cannot give the exact number of levels of the hypertrees
  // because it is not generated yet and this process depends on the recursion formula.
  // Just send an upper limit instead.
  outInfo->Set( vtkHyperTreeGrid::LEVELS(), this->MaximumLevel );
  outInfo->Set( vtkHyperTreeGrid::DIMENSION(), this->Dimension );
  outInfo->Set( vtkHyperTreeGrid::ORIENTATION(), this->Orientation );

  double origin[3];
  origin[0] = this->XCoordinates->GetTuple1( 0 );
  origin[1] = this->YCoordinates->GetTuple1( 0 );
  origin[2] = this->ZCoordinates->GetTuple1( 0 );
  outInfo->Set( vtkDataObject::ORIGIN(), origin, 3 );

  int wholeExtent[6];
  wholeExtent[0] = 0;
  wholeExtent[1] = this->GridSize[0];
  wholeExtent[2] = 0;
  wholeExtent[3] = this->GridSize[1];
  wholeExtent[4] = 0;
  wholeExtent[5] = this->GridSize[2];
  outInfo->Set( vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent, 6 );

  outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridSource::RequestData( vtkInformation*,
                                         vtkInformationVector**,
                                         vtkInformationVector* outputVector )
{
  // Retrieve the output
  vtkDataObject* outputDO = vtkDataObject::GetData( outputVector, 0 );
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast( outputDO );
  if ( ! output )
  {
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject( 0 );
  int* extent = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  output->Initialize();
  output->SetGridExtent(extent);

  vtkPointData* outData = output->GetPointData();

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

    if ( ! this->DescriptorBits && ! this->InitializeFromStringDescriptor(extent) )
    {
      return 0;
    }
    else if ( this->DescriptorBits && ! this->InitializeFromBitsDescriptor() )
    {
      return 0;
    }
  } // if this->UseDescriptor

  // Set straightforward grid parameters
  output->SetTransposedRootIndexing( this->TransposedRootIndexing );
  output->SetDimension( this->Dimension );
  output->SetOrientation( this->Orientation );
  output->SetBranchFactor( this->BranchFactor );
  output->SetMaterialMaskIndex( this->LevelZeroMaterialIndex );

  //  Set parameters that depend on dimension
  switch ( this->Dimension )
  {
    case 1:
    {
      // Set 1D grid size depending on orientation
      unsigned int axis = this->Orientation;
      unsigned n = this->GridSize[axis];

      // Create null coordinate array for non-existent dimensions
      ++ n;
      vtkNew<vtkDoubleArray> zeros;
      zeros->SetNumberOfValues( 2 );
      zeros->SetValue( 0, 0. );
      zeros->SetValue( 1, 0. );

      // Create coordinate array for existent dimension
      vtkNew<vtkDoubleArray> coords;
      coords->SetNumberOfValues( n );
      for ( unsigned int i = 0; i < n; ++ i )
      {
        double coord = this->Origin[axis] + this->GridScale[axis] * static_cast<double>( i );
        coords->SetValue( i, coord );
      } // i

      // Assign coordinates
      switch ( axis )
      {
        case 0:
          output->SetXCoordinates( coords );
          output->SetYCoordinates( zeros );
          output->SetZCoordinates( zeros );
          break;
        case 1:
          output->SetXCoordinates( zeros );
          output->SetYCoordinates( coords );
          output->SetZCoordinates( zeros );
          break;
        case 2:
          output->SetXCoordinates( zeros );
          output->SetYCoordinates( zeros );
          output->SetZCoordinates( coords );
          break;
      } // switch ( axis )
      zeros->SetValue( 1, 0. );
    } // case 1
      break;
    case 2:
    {
      // Create null coordinate array for non-existent dimension
      vtkNew<vtkDoubleArray> zeros;
      zeros->SetNumberOfValues( 2 );
      zeros->SetValue( 0, 0. );
      zeros->SetValue( 1, 0. );

      // Create null coordinate arrays for existent dimensions
      unsigned int axis1 = ( this->Orientation + 1 ) % 3;
      vtkNew<vtkDoubleArray> coords1;
      unsigned int n1 = this->GridSize[axis1] + 1;
      coords1->SetNumberOfValues( n1 );
      for ( unsigned int i = 0; i < n1; ++ i )
      {
        double coord = this->Origin[axis1] + this->GridScale[axis1] * static_cast<double>( i );
        coords1->SetValue( i, coord );
      } // i
      unsigned int axis2 = ( this->Orientation + 2 ) % 3;
      vtkNew<vtkDoubleArray> coords2;
      unsigned int n2 = this->GridSize[axis2] + 1;
      coords2->SetNumberOfValues( n2 );
      for ( unsigned int i = 0; i < n2; ++ i )
      {
        double coord = this->Origin[axis2] + this->GridScale[axis2] * static_cast<double>( i );
        coords2->SetValue( i, coord );
      } // i

      // Assign coordinates
      switch ( this->Orientation )
      {
        case 0:
          output->SetXCoordinates( zeros );
          output->SetYCoordinates( coords1 );
          output->SetZCoordinates( coords2 );
          break;
        case 1:
          output->SetXCoordinates( coords2 );
          output->SetYCoordinates( zeros );
          output->SetZCoordinates( coords1 );
          break;
        case 2:
          output->SetXCoordinates( coords1 );
          output->SetYCoordinates( coords2 );
          output->SetZCoordinates( zeros );
          break;
      } // switch ( this->Orientation )
    } // case 2
      break;
    case 3:
    {
      // Create x-coordinates array
      vtkNew<vtkDoubleArray> coordsx;
      unsigned int nx = extent[1] - extent[0] + 1;
      coordsx->SetNumberOfValues( nx );
      for ( unsigned int i = 0; i < nx; ++ i )
      {
        double coord = this->Origin[0] + this->GridScale[0] * static_cast<double>( i+extent[0] );
        coordsx->SetValue( i, coord );
      } // i

      // Create y-coordinates array
      vtkNew<vtkDoubleArray> coordsy;
      unsigned int ny = extent[3] - extent[2] + 1;
      coordsy->SetNumberOfValues( ny );
      for ( unsigned int i = 0; i < ny; ++ i )
      {
        double coord = this->Origin[1] + this->GridScale[1] * static_cast<double>( i+extent[2] );
        coordsy->SetValue( i, coord );
      } // i

      // Create z-coordinates array
      vtkNew<vtkDoubleArray> coordsz;
      unsigned int nz = extent[5] - extent[4] + 1;
      coordsz->SetNumberOfValues( nz );
      for ( unsigned int i = 0; i < nz; ++ i )
      {
        double coord = this->Origin[2] + this->GridScale[2] * static_cast<double>( i+extent[4] );
        coordsz->SetValue( i, coord );
      } // i

      // Assign coordinates
      output->SetXCoordinates( coordsx );
      output->SetYCoordinates( coordsy );
      output->SetZCoordinates( coordsz );
      break;
    } // case 3
    default:
      vtkErrorMacro(<<"Unsupported dimension: "
                    << this->Dimension
                    << ".");
      return 0;
  } // switch ( this->Dimension )

  // Prepare array of doubles for depth values
  vtkNew<vtkDoubleArray> depthArray;
  depthArray->SetName( "Depth" );
  depthArray->SetNumberOfComponents( 1 );
  outData->SetScalars( depthArray );

  if ( this->GenerateInterfaceFields )
  {
    // Prepare arrays of triples for interface surrogates
    vtkNew<vtkDoubleArray> normalsArray;
    normalsArray->SetName( "Normals" );
    normalsArray->SetNumberOfComponents( 3 );
    outData->SetVectors( normalsArray );

    vtkNew<vtkDoubleArray> interceptsArray;
    interceptsArray->SetName( "Intercepts" );
    interceptsArray->SetNumberOfComponents( 3 );
    outData->AddArray( interceptsArray );
  }

  if ( ! this->UseDescriptor )
  {
    // Prepare array of doubles for quadric values
    vtkNew<vtkDoubleArray> quadricArray;
    quadricArray->SetName( "Quadric" );
    quadricArray->SetNumberOfComponents( 1 );
    outData->AddArray( quadricArray );
  }

  // Iterate over constituting hypertrees
  if ( ! this->ProcessTrees( nullptr, outputDO ) )
  {
    return 0;
  }

  // Squeeze output data arrays
  for ( int a = 0; a < outData->GetNumberOfArrays(); ++ a )
  {
    outData->GetArray( a )->Squeeze();
  }

  this->LevelBitsIndexCnt.clear();
  this->LevelBitsIndex.clear();

  if (output->CheckAttributes() != 0)
  {
    vtkErrorMacro("HyperTreeGrid generation failed.");
    output->Initialize();
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridSource::ProcessTrees( vtkHyperTreeGrid*,
                                          vtkDataObject* outputDO )
{
  // Downcast output data object to hyper tree grid
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast( outputDO );
  if ( ! output )
  {
    vtkErrorMacro( "Incorrect type of output: "
                   << outputDO->GetClassName() );
    return 0;
  }

  // Generate grid of empty trees
  output->GenerateTrees();

  // Iterate over all hyper trees
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  output->InitializeTreeIterator( it );
  while ( vtkHyperTree* tree = it.GetNextTree( index ) )
  {
    unsigned int i, j, k;
    output->GetLevelZeroCoordinatesFromIndex( index, i, j, k );

    // Initialize cursor
    vtkHyperTreeCursor* cursor = output->NewCursor( index );
    if ( ! cursor )
    {
      continue;
    }
    cursor->ToRoot();

    // Initialize local cell index
    int idx[3] = { 0, 0, 0 };

    if ( this->UseDescriptor )
    {
      this->InitTreeFromDescriptor( output, cursor, index, idx );
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
      this->SubdivideFromQuadric( output,
                                  cursor,
                                  0,
                                  index,
                                  idx,
                                  origin,
                                  this->GridScale );
    } // else

    // Clean up
    cursor->UnRegister( this );
  } // it

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridSource::InitTreeFromDescriptor( vtkHyperTreeGrid* output,
                                                     vtkHyperTreeCursor* cursor,
                                                     int treeIdx,
                                                     int idx[3])
{
  // Subdivide using descriptor
  if ( ! this->DescriptorBits )
  {
    this->SubdivideFromStringDescriptor( output, cursor, 0, treeIdx, 0, idx, 0 );
  }
  else
  {
    this->SubdivideFromBitsDescriptor( output, cursor, 0, treeIdx, 0, idx, 0 );
  }
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridSource::InitializeFromStringDescriptor(int* extent)
{
  size_t descLen = strlen( this->Descriptor );

  // Verify that grid and material specifications are consistent
  if ( this->UseMaterialMask && strlen( this->MaterialMask ) != descLen )
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
  unsigned int nRefinedLocal = 0;
  unsigned int nLeavesLocal = 0;
  unsigned int nNextLevelLocal = 0;
  bool rootLevel = true;
  std::ostringstream descriptor;
  std::ostringstream mask;

  // Reset parsed level containers
  this->LevelDescriptors.clear();
  this->LevelMaterialMasks.clear();

  // keep track of which global tree we're in
  std::vector<bool> localTree(nTotal, false);
  for (int k=0;k<static_cast<int>(this->GridSize[2]);k++)
  {
    if (k >= extent[4] && k < extent[5])
    {
      for (int j=0;j<static_cast<int>(this->GridSize[1]);j++)
      {
        if (j >= extent[2] && j < extent[3])
        {
          for (int i=0;i<static_cast<int>(this->GridSize[0]);i++)
          {
            if (i >= extent[0] && i < extent[1])
            {
              localTree[i+j*this->GridSize[0]+k*this->GridSize[0]*GridSize[1]] = true;
              nNextLevelLocal++;
            }
          }
        }
      }
    }
  }
  std::vector<int> currentLevelTreeIndices(nTotal);
  for (unsigned int i=0; i < nTotal; i++)
  {
    currentLevelTreeIndices[i] = i;
  }
  std::vector<int> nextLevelTreeIndices;
  int counter = 0;
  bool ownsTree;

  for ( size_t i = 0; i < descLen; ++ i )
  {
    char c = this->Descriptor[i];
    char m = this->UseMaterialMask ? this->MaterialMask[i] : 0;
    switch ( c )
    {
      case ' ':
        // Space is allowed as separator, verify mask consistency if needed
        if ( this->UseMaterialMask && m != ' ' )
        {
          vtkErrorMacro(<<"Space separators do not match between "
                        "descriptor and material mask.");
          return 0;
        }
        break; // case ' '

      case '|':
        //  A level is complete, verify mask consistency if needed
        if ( this->UseMaterialMask && m != '|' )
        {
          vtkErrorMacro(<<"Level separators do not match between "
                        "descriptor and material mask.");
          return 0;
        }
        counter = 0;
        currentLevelTreeIndices = nextLevelTreeIndices;
        nextLevelTreeIndices.clear();

        // Store descriptor and material mask for current level
        this->LevelDescriptors.push_back( descriptor.str() );
        this->LevelMaterialMasks.push_back( mask.str() );

        // Check whether cursor is still at rool level
        if ( rootLevel )
        {
          rootLevel = false;

          // Verify that total number of root cells is consistent with descriptor
          if ( nRefinedLocal + nLeavesLocal != nNextLevelLocal )
          {
            vtkErrorMacro(<<"String "
                          << this->Descriptor
                          << " describes "
                          << nRefinedLocal + nLeavesLocal
                          << " local root cells != "
                          << nNextLevelLocal);
            return 0;
          }
        } // if ( rootLevel )
        else if ( descriptor.str().size() != nNextLevelLocal )
        {
          // Verify that level descriptor cardinality matches expected value
          vtkErrorMacro(<<"String level descriptor "
                        << descriptor.str().c_str()
                        << " has cardinality "
                        << descriptor.str().size()
                        << " which is not expected value of "
                        << nNextLevelLocal);

          return 0;
        } // else if

        // Predict next level descriptor cardinality
        nNextLevelLocal = nRefinedLocal * this->BlockSize;

        // Reset per level values
        descriptor.str( "" );
        mask.str( "" );
        nRefinedLocal = 0;
        nLeavesLocal = 0;
        break; // case '|'

      case '1':
      case 'R':
        nextLevelTreeIndices.insert(nextLevelTreeIndices.end(), this->BlockSize, currentLevelTreeIndices[counter]);
        //  Refined cell, verify mask consistency if needed
        if ( this->UseMaterialMask && m == '0' )
        {
          vtkErrorMacro(<<"A refined branch must contain material.");
          return 0;
        }

        // Append characters to per level descriptor and material mask if used
        ownsTree = localTree[currentLevelTreeIndices[counter]];
        counter++;
        if (ownsTree)
        {
          // Refined cell, update branch counter
          ++ nRefinedLocal;
          descriptor << c;
          if ( this->UseMaterialMask )
          {
            mask << m;
          }
        }
        break; // case 'R'

      case '0':
      case '.':

        // Append characters to per level descriptor and material mask if used
        ownsTree = localTree[currentLevelTreeIndices[counter]];
        counter++;
        if (ownsTree)
        {
          // Leaf cell, update leaf counter
          ++ nLeavesLocal;
          descriptor << c;
          if ( this->UseMaterialMask )
          {
            mask << m;
          }
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
  if ( descriptor.str().size() != nNextLevelLocal )
  {
    vtkErrorMacro(<<"String level descriptor "
                  << descriptor.str().c_str()
                  << " has cardinality "
                  << descriptor.str().size()
                  << " which is not expected value of "
                  << nNextLevelLocal);

    return 0;
  }

  // Push per-level descriptor and material mask if used
  this->LevelDescriptors.push_back( descriptor.str() );
  if ( this->UseMaterialMask )
  {
    this->LevelMaterialMasks.push_back( mask.str() );
  }
  unsigned int nLevels =
    static_cast<unsigned int>( this->LevelDescriptors.size() );

  // Create vector of counters as long as tree depth
  this->LevelCounters.clear();
  this->LevelCounters.resize(nLevels, 0);

  this->LevelBitsIndex.clear();
  this->LevelBitsIndex.push_back( 0 );
  for ( unsigned int i = 1; i < nLevels; ++ i )
  {
    this->LevelBitsIndex.push_back
      (
       this->LevelBitsIndex[i-1] +
       static_cast<vtkIdType>(this->LevelDescriptors[i-1].length())
       );
  }
  this->LevelBitsIndexCnt = this->LevelBitsIndex;

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridSource::SubdivideFromStringDescriptor( vtkHyperTreeGrid* output,
                                                            vtkHyperTreeCursor* cursor,
                                                            unsigned int level,
                                                            int treeIdx,
                                                            int childIdx,
                                                            int idx[3],
                                                            int parentPos )
{
  // Get handle on point data
  vtkPointData* outData = output->GetPointData();

  // Calculate pointer into level descriptor string
  size_t pointer = static_cast<size_t>(level ? childIdx + parentPos * this->BlockSize
                                             : treeIdx);

  if (level >= this->LevelDescriptors.size() ||
      pointer >= this->LevelDescriptors[level].size())
  {
    vtkErrorMacro("Error building HyperTreeGrid. Bad descriptor?");
    return;
  }

  // Calculate the node global index
  vtkIdType id = this->LevelBitsIndexCnt[level];
  ++ this->LevelBitsIndexCnt[level];

  // Set depth array value
  outData->GetArray( "Depth" )->InsertTuple1( id, level );

  if ( this->GenerateInterfaceFields )
  {
    // Set interface arrays values
    double v = 1. / ( 1 << level );
    outData->GetArray( "Normals" )->InsertTuple3( id, v, v, v );
    outData->GetArray( "Intercepts" )->InsertTuple3( id, v, 0., 3. );
  }

  // Initialize global index of tree
  cursor->GetTree()->SetGlobalIndexFromLocal( cursor->GetVertexId(), id );

  // Subdivide further or stop recursion with terminal leaf
  if ( level + 1 < this->MaximumLevel
       && this->LevelDescriptors.at( level ).at( pointer ) == 'R' )
  {
    // Subdivide hyper tree grid leaf
    output->SubdivideLeaf( cursor, treeIdx );

    // Figure out index bounds depending on dimension and orientation
    int xDim = this->BranchFactor;
    int yDim = this->BranchFactor;
    int zDim = this->BranchFactor;
    if ( this->Dimension == 1 )
    {
      switch ( this->Orientation )
      {
        case 0:
          yDim = 1;
          zDim = 1;
          break;
        case 1:
          xDim = 1;
          zDim = 1;
          break;
        case 2:
          xDim = 1;
          yDim = 1;
          break;
        default:
          vtkErrorMacro(<< "Incorrect orientation in 1D: "
                        << this->Orientation);
          return;
      } // switch ( this->Orientation )
    } // if ( this->Dimension == 1 )
    else if ( this->Dimension == 2 )
    {
      switch ( this->Orientation )
      {
        case 0:
          xDim = 1;
          break;
        case 1:
          yDim = 1;
          break;
        case 2:
          zDim = 1;
          break;
        default:
          vtkErrorMacro(<< "Incorrect orientation in 2D: "
                        << this->Orientation);
          return;
      } // switch ( this->Orientation )
    } // else if ( this->Dimension == 2 )

    // Now traverse to children
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
          this->SubdivideFromStringDescriptor( output,
                                               cursor,
                                               level + 1,
                                               treeIdx,
                                               newChildIdx,
                                               newIdx,
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

    if( this->UseMaterialMask )
    {
      output->GetMaterialMask()->InsertTuple1( id, 0 );
    }
  } // if ( subdivide )

  else if ( this->UseMaterialMask )
  {
    // Blank leaf if needed
    bool masked = this->LevelMaterialMasks.at( level ).at( pointer ) == '0' ? 1 : 0;
    output->GetMaterialMask()->InsertTuple1( id, masked );
  } // else if
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridSource::InitializeFromBitsDescriptor()
{
  // Verify that grid and material specifications are consistent
  if ( this->UseMaterialMask && ! this->LevelZeroMaterialIndex
       && this->MaterialMaskBits->GetSize() != this->DescriptorBits->GetSize() )
  {
    vtkErrorMacro(<<"Material mask is used but has length "
                  << this->MaterialMaskBits->GetSize()
                  << " != "
                  << this->DescriptorBits->GetSize()
                  << " which is the length of the grid descriptor.");

    return 0;
  }

  // Calculate total level 0 grid size
  vtkIdType nTotal = this->LevelZeroMaterialIndex ?
    static_cast<vtkIdType>(this->LevelZeroMaterialMap.size()) :
    this->GridSize[0] * this->GridSize[1] * this->GridSize[2];

  // Parse descriptor and material mask if used
  this->LevelBitsIndex.clear();
  this->LevelBitsIndex.push_back( 0 );
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
      ++ nCurrentLevel;
      this->LevelBitsIndex.push_back(i);
    }
    nRefined += this->DescriptorBits->GetValue( i );
    nLeaves += this->DescriptorBits->GetValue(i) == 0 ? 1 : 0;

    ++ nCurrentLevelCount;
  }

  this->LevelBitsIndexCnt = this->LevelBitsIndex;

  // Verify and append last level string
  if ( nCurrentLevelCount != nNextLevel )
  {
    vtkErrorMacro(<<"Level descriptor "
                  << nCurrentLevel
                  << " has cardinality "
                  << nCurrentLevelCount
                  << " which is not expected value of "
                  << nNextLevel);

    return 0;
  }

  ++ nCurrentLevel;

  this->LevelBitsIndexCnt = this->LevelBitsIndex;

  // Create vector of counters as long as tree depth
  for ( unsigned int i = 0; i < nCurrentLevel; ++ i )
  {
    this->LevelCounters.push_back( 0 );
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridSource::SubdivideFromBitsDescriptor( vtkHyperTreeGrid* output,
                                                          vtkHyperTreeCursor* cursor,
                                                          unsigned int level,
                                                          int treeIdx,
                                                          int childIdx,
                                                          int idx[3],
                                                          int parentPos )
{
  // Get handle on point data
  vtkPointData* outData = output->GetPointData();

  vtkIdType startIdx = this->LevelBitsIndex[level];
  int pointer = level ? childIdx + parentPos * this->BlockSize : treeIdx;

  // Calculate the node global index
  vtkIdType id = this->LevelBitsIndexCnt[level];
  ++ this->LevelBitsIndexCnt[level];

  // Set depth array value
  outData->GetArray( "Depth" )->InsertTuple1( id, level );

  if ( this->GenerateInterfaceFields )
  {
    // Set interface arrays values
    double v = 1. / ( 1 << level );
    outData->GetArray( "Normals" )->InsertTuple3( id, v, v, v );
    outData->GetArray( "Intercepts" )->InsertTuple3( id, v, 0., 3. );
  }

  // Initialize global index of tree
  cursor->GetTree()->SetGlobalIndexFromLocal( cursor->GetVertexId(), id );
  bool refine = false;

  if ( this->LevelZeroMaterialIndex && level == 0 )
  {
    if ( this->LevelZeroMaterialMap.find( treeIdx ) !=
         this->LevelZeroMaterialMap.end() )
    {
      refine
        = this->DescriptorBits->GetValue( this->LevelZeroMaterialMap[treeIdx] ) == 1;
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
    output->SubdivideLeaf( cursor, treeIdx );

    // Figure out index bounds depending on dimension and orientation
    int xDim = this->BranchFactor;
    int yDim = this->BranchFactor;
    int zDim = this->BranchFactor;
    if ( this->Dimension == 1 )
    {
      switch ( this->Orientation )
      {
        case 0:
          yDim = 1;
          zDim = 1;
          break;
        case 1:
          xDim = 1;
          zDim = 1;
          break;
        case 2:
          xDim = 1;
          yDim = 1;
          break;
        default:
          vtkErrorMacro(<< "Incorrect orientation in 1D: "
                        << this->Orientation);
          return;
      } // switch ( this->Orientation )
    } // if ( this->Dimension == 1 )
    else if ( this->Dimension == 2 )
    {
      switch ( this->Orientation )
      {
        case 0:
          xDim = 1;
          break;
        case 1:
          yDim = 1;
          break;
        case 2:
          zDim = 1;
          break;
        default:
          vtkErrorMacro(<< "Incorrect orientation in 2D: "
                        << this->Orientation);
          return;
      } // switch ( this->Orientation )
    } // else if ( this->Dimension == 2 )

    // Now traverse to children
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
          this->SubdivideFromBitsDescriptor( output,
                                             cursor,
                                             level + 1,
                                             treeIdx,
                                             newChildIdx,
                                             newIdx,
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

    if( this->UseMaterialMask )
    {
      output->GetMaterialMask()->InsertTuple1( id, 0 );
    }
  } // if ( subdivide )
  else
  {
    bool isMasked = false;

    if ( this->UseMaterialMask  )
    {
      if ( this->LevelZeroMaterialIndex )
      {
        isMasked = ( level == 0 ) ? false :
          this->MaterialMaskBits->GetValue( startIdx - this->LevelBitsIndex[1] + pointer ) == 0;
      }
      else
      {
        isMasked = this->MaterialMaskBits->GetValue( startIdx + pointer ) == 0;
      }
    }

    // Blank leaf if needed
    output->GetMaterialMask()->InsertTuple1( id, isMasked ? 1 : 0 );
  } // else
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridSource::SubdivideFromQuadric( vtkHyperTreeGrid* output,
                                                   vtkHyperTreeCursor* cursor,
                                                   unsigned int level,
                                                   int treeIdx,
                                                   const int idx[3],
                                                   double origin[3],
                                                   double size[3] )
{
  // Get handle on point data
  vtkPointData* outData = output->GetPointData();

  // Calculate the node global index
  vtkIdType id =
    cursor->GetTree()->GetGlobalIndexFromLocal( cursor->GetVertexId() );
  ++ this->LevelBitsIndexCnt[0];

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

  // Set depth array value
  outData->GetArray( "Depth" )->InsertTuple1( id, level );

  if ( this->GenerateInterfaceFields )
  {
    // Set interface arrays values
    double v = 1. / ( 1 << level );
    outData->GetArray( "Normals" )->InsertTuple3( id, v, v, v );
    outData->GetArray( "Intercepts" )->InsertTuple3( id, v, 0., 3. );
  }

  // Subdivide further or stop recursion with terminal leaf
  if ( subdivide && level + 1 < this->MaximumLevel )
  {
    // Cell is subdivided so it cannot be masked
    if( this->UseMaterialMask )
    {
      output->GetMaterialMask()->InsertTuple1( id, 0 );
    }

    // Subdivide hyper tree grid leaf
    output->SubdivideLeaf( cursor, treeIdx );

    // Compute new sizes
    double newSize[] = { 0., 0., 0. };
    switch ( this->Dimension )
    {
      case 3:
        newSize[2] = size[2] / this->BranchFactor;
        VTK_FALLTHROUGH;
      case 2:
        newSize[1] = size[1] / this->BranchFactor;
        VTK_FALLTHROUGH;
      case 1:
        newSize[0] = size[0] / this->BranchFactor;
        break;
    }

    // Figure out index bounds depending on dimension and orientation
    int xDim = this->BranchFactor;
    int yDim = this->BranchFactor;
    int zDim = this->BranchFactor;
    if ( this->Dimension == 1 )
    {
      switch ( this->Orientation )
      {
        case 0:
          yDim = 1;
          zDim = 1;
          break;
        case 1:
          xDim = 1;
          zDim = 1;
          break;
        case 2:
          xDim = 1;
          yDim = 1;
          break;
        default:
          vtkErrorMacro(<< "Incorrect orientation in 1D: "
                        << this->Orientation);
          return;
      } // switch ( this->Orientation )
    } // if ( this->Dimension == 1 )
    else if ( this->Dimension == 2 )
    {
      switch ( this->Orientation )
      {
        case 0:
          xDim = 1;
          break;
        case 1:
          yDim = 1;
          break;
        case 2:
          zDim = 1;
          break;
        default:
          vtkErrorMacro(<< "Incorrect orientation in 2D: "
                        << this->Orientation);
          return;
      } // switch ( this->Orientation )
    } // else if ( this->Dimension == 2 )

    // Now traverse to children
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
          this->SubdivideFromQuadric( output,
                                      cursor,
                                      level + 1,
                                      treeIdx,
                                      newIdx,
                                      origin,
                                      newSize );

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
    if( this->UseMaterialMask )
    {
      output->GetMaterialMask()->InsertTuple1( id, ( nPos > 0 ) ? 1 : 0 );
    }

    // Cell values
    outData->GetArray( "Depth" )->InsertTuple1( id, level );
    if ( this->GenerateInterfaceFields )
    {
      // Set interface arrays values
      double v = 1. / ( 1 << level );
      outData->GetArray( "Normals" )->InsertTuple3( id, v, v, v );
      outData->GetArray( "Intercepts" )->InsertTuple3( id, v, 0., 3. );
    }
    outData->GetArray( "Quadric" )->InsertTuple1( id, sum );
  } // else
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridSource::SetQuadricCoefficients( double q[10] )
{
  if ( ! this->Quadric )
  {
    this->Quadric = vtkQuadric::New();
  }
  this->Quadric->SetCoefficients( q );
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridSource::GetQuadricCoefficients( double q[10] )
{
  this->Quadric->GetCoefficients( q );
}

//-----------------------------------------------------------------------------
double* vtkHyperTreeGridSource::GetQuadricCoefficients()
{
  return this->Quadric->GetCoefficients();
}

//----------------------------------------------------------------------------
vtkMTimeType vtkHyperTreeGridSource::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();

  if ( this->Quadric )
  {
    vtkMTimeType time = this->Quadric->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  return mTime;
}

//----------------------------------------------------------------------------
vtkBitArray* vtkHyperTreeGridSource::ConvertDescriptorStringToBitArray( const std::string& str )
{
  vtkBitArray* desc = vtkBitArray::New();
  desc->Allocate( static_cast<vtkIdType>(str.length()) );

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
        desc->InsertNextValue( 1 );
        break;

      case '0':
      case '.':
        // Leaf cell
        desc->InsertNextValue( 0 );
        break;

      default:
        vtkErrorMacro(<< "Unrecognized character: "
                      << *dit
                      << " in string "
                      << str);
        desc->Delete();
        return nullptr;
    } // switch( *dit )
  }

  desc->Squeeze();
  return desc;
}

//----------------------------------------------------------------------------
vtkBitArray* vtkHyperTreeGridSource::ConvertMaterialMaskStringToBitArray( const std::string& str )
{
  return ConvertDescriptorStringToBitArray( str );
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridSource::SetOrientation( unsigned int i )
{
  if (this->Orientation != (i>2?2:i))
  {
    this->Orientation = (i>2?2:i);
    this->Modified();
  }
}
