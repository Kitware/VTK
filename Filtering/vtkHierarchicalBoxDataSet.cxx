/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalBoxDataSet.h"

#include "vtkAMRBox.h"
#include "vtkHierarchicalBoxDataIterator.h"
#include "vtkInformation.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIdList.h"
#include "vtkAMRGridIndexEncoder.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkMath.h"
#include "vtkType.h"

#include <cmath>
#include <limits>
#include <vtkstd/vector>
#include <sstream>
#include <cassert>

vtkStandardNewMacro(vtkHierarchicalBoxDataSet);
vtkInformationKeyRestrictedMacro(vtkHierarchicalBoxDataSet,BOX,IntegerVector, 6);
vtkInformationKeyMacro(vtkHierarchicalBoxDataSet,NUMBER_OF_BLANKED_POINTS,IdType);
vtkInformationKeyMacro(vtkHierarchicalBoxDataSet,REFINEMENT_RATIO,Integer);
vtkInformationKeyMacro(vtkHierarchicalBoxDataSet,BOX_DIMENSIONALITY,Integer);
vtkInformationKeyRestrictedMacro(
    vtkHierarchicalBoxDataSet,BOX_ORIGIN,DoubleVector, 3);
vtkInformationKeyRestrictedMacro(
    vtkHierarchicalBoxDataSet,SPACING,DoubleVector, 3);
vtkInformationKeyMacro(vtkHierarchicalBoxDataSet,RANK,Integer);
vtkInformationKeyMacro(vtkHierarchicalBoxDataSet,BLOCK_ID,Integer);
vtkInformationKeyMacro(vtkHierarchicalBoxDataSet,GEOMETRIC_DESCRIPTION,Integer);
vtkInformationKeyRestrictedMacro(
    vtkHierarchicalBoxDataSet,REAL_EXTENT,IntegerVector,6);

typedef vtkstd::vector<vtkAMRBox> vtkAMRBoxList;
//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet::vtkHierarchicalBoxDataSet()
{
  this->ScalarRange[0]    = VTK_DOUBLE_MAX;
  this->ScalarRange[1]    = VTK_DOUBLE_MIN;
  this->PadCellVisibility = false;
  this->origin[0] = this->origin[1] = this->origin[2] = 0.0;
  this->Bounds[0] = this->Bounds[1] = this->Bounds[2] = 
    this->Bounds[3] = this->Bounds[4] = this->Bounds[5] = 0.0;

}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet::~vtkHierarchicalBoxDataSet()
{
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetOrigin( const double o[3] )
{
  for( int i=0; i < 3; ++i )
    this->origin[i] = o[i];
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::GetOrigin( double o[3] )
{
  for( int i=0; i < 3; ++i )
    o[i] = this->origin[i];
}

//----------------------------------------------------------------------------
vtkCompositeDataIterator* vtkHierarchicalBoxDataSet::NewIterator()
{
  vtkHierarchicalBoxDataIterator* iter = vtkHierarchicalBoxDataIterator::New();
  iter->SetDataSet(this);
  return iter;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetNumberOfLevels(unsigned int numLevels)
{
  this->Superclass::SetNumberOfChildren(numLevels);

  // Initialize each level with a vtkMultiPieceDataSet. 
  // vtkMultiPieceDataSet is an overkill here, since the datasets with in a
  // level cannot be composite datasets themselves. 
  // This will make is possible for the user to set information with each level
  // (in future).
  for (unsigned int cc=0; cc < numLevels; cc++)
    {
    if (!this->Superclass::GetChild(cc))
      {
      vtkMultiPieceDataSet* mds = vtkMultiPieceDataSet::New();
      this->Superclass::SetChild(cc, mds);
      mds->Delete();
      }
    }
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalBoxDataSet::GetNumberOfLevels()
{
  return this->Superclass::GetNumberOfChildren();
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetNumberOfDataSets(unsigned int level, 
  unsigned int numDS)
{
  if (level >= this->GetNumberOfLevels())
    {
    this->SetNumberOfLevels(level+1);
    }
  vtkMultiPieceDataSet* levelDS = vtkMultiPieceDataSet::SafeDownCast(
    this->Superclass::GetChild(level));
  if (levelDS)
    {
    levelDS->SetNumberOfPieces(numDS);
    }
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalBoxDataSet::GetNumberOfDataSets(unsigned int level)
{
  vtkMultiPieceDataSet* levelDS = vtkMultiPieceDataSet::SafeDownCast(
    this->Superclass::GetChild(level));
  if (levelDS)
    {
    return levelDS->GetNumberOfPieces();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetDataSet(
  unsigned int level, unsigned int id,
  int LoCorner[3], int HiCorner[3], vtkUniformGrid* dataSet)
{
  vtkAMRBox box(3, LoCorner, HiCorner);
  this->SetDataSet(level, id, box, dataSet);
}

//----------------------------------------------------------------------------

void vtkHierarchicalBoxDataSet::SetDataSet(
  unsigned int level, unsigned int id, vtkAMRBox &box, vtkUniformGrid *dataSet)
{
  this->SetDataSet( level, id, dataSet );
  this->SetMetaData( level, id, box );
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetDataSet(
    unsigned int level, unsigned int idx, vtkUniformGrid *grid )
{

// In some cases the grid could be NULL, i.e., in the case that the data
// is distributed.
//  assert( "Input grid is NULL!" && (grid!=NULL) );

  // STEP 0: Resize the number of levels accordingly
  if( level >= this->GetNumberOfLevels() )
    {
      this->SetNumberOfLevels( level+1 );
    }

  // STEP 1: Insert data at the given location
  vtkMultiPieceDataSet* levelDS =
      vtkMultiPieceDataSet::SafeDownCast( this->Superclass::GetChild(level));
  if( levelDS != NULL )
    {
      levelDS->SetPiece(idx, grid);
    }
  else
    {
      vtkErrorMacro( "Multi-piece data-structure is NULL!!!!" );
    }
}

//------------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::AppendDataSet(
    unsigned int level, vtkUniformGrid* grid )
{
  // STEP 0: Resize the number of levels accordingly
  if( level >= this->GetNumberOfLevels() )
    {
      this->SetNumberOfLevels( level+1 );
    }

  // STEP 1: Insert data at the end
  vtkMultiPieceDataSet* levelDS =
      vtkMultiPieceDataSet::SafeDownCast( this->Superclass::GetChild(level));
  if( levelDS != NULL )
    {
      unsigned int idx = levelDS->GetNumberOfPieces();
      levelDS->SetPiece(idx, grid);
    }
  else
    {
      vtkErrorMacro( "Multi-piece data-structure is NULL!!!!" );
    }
}

//------------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetMetaData(
    unsigned int level, unsigned int id, const vtkAMRBox &box )
{

  // STEP 0: Resize the number of levels accordingly
  if( level >= this->GetNumberOfLevels() )
    {
      this->SetNumberOfLevels( level+1 );
    }

  // STEP 1: Insert the meta data at the given location
  vtkMultiPieceDataSet* levelDS =
      vtkMultiPieceDataSet::SafeDownCast( this->Superclass::GetChild(level));
  if( levelDS != NULL )
    {
      if( id >= levelDS->GetNumberOfPieces() )
        levelDS->SetPiece( id, NULL);

      vtkInformation* info = levelDS->GetMetaData(id);
      if (info)
        {
          const int *loCorner=box.GetLoCorner();
          const int *hiCorner=box.GetHiCorner();
          info->Set(BOX_DIMENSIONALITY(), box.GetDimensionality());
          info->Set(BOX(),loCorner[0], loCorner[1], loCorner[2],
                          hiCorner[0], hiCorner[1], hiCorner[2]);
          double x0[3];
          box.GetDataSetOrigin( x0 );
          info->Set(BOX_ORIGIN(), x0[0], x0[1], x0[2] );
          info->Set(RANK(), box.GetProcessId() );
          info->Set(BLOCK_ID(), box.GetBlockId() );
          info->Set(GEOMETRIC_DESCRIPTION(),box.GetGridDescription()  );

          double spacing[3];
          box.GetGridSpacing( spacing );
          info->Set(SPACING(),spacing[0],spacing[1],spacing[2]);

          int realExtent[6];
          box.GetRealExtent( realExtent );
          info->Set(REAL_EXTENT(),
              realExtent[0], realExtent[1], realExtent[2],
              realExtent[3], realExtent[4], realExtent[5] );
        }
      else
        {
          vtkErrorMacro( "Metadata object is NULL!!!!" );
        }
    }
  else
    {
      vtkErrorMacro( "Multi-piece data-structure is NULL!!!!" );
    }

}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkHierarchicalBoxDataSet::GetDataSet(
    unsigned int level, unsigned int id )
{
  if( this->GetNumberOfLevels() <= level ||
      this->GetNumberOfDataSets(level) <= id )
    {
      return NULL;
    }

  vtkMultiPieceDataSet* levelDS =
      vtkMultiPieceDataSet::SafeDownCast(this->Superclass::GetChild(level));
  if( levelDS )
    {
      return( vtkUniformGrid::SafeDownCast( levelDS->GetPiece( id ) ) );
    }
  vtkErrorMacro( "Unexcepected NULL pointer encountered!\n" );
  return NULL;
}

//----------------------------------------------------------------------------
vtkUniformGrid* vtkHierarchicalBoxDataSet::GetDataSet(
                          unsigned int level, unsigned int id, vtkAMRBox& box )
{
  if( this->GetMetaData( level, id, box ) != 1 )
    {
      vtkErrorMacro( "Could not retrieve meta-data for the given data set!" );
    }
  return( this->GetDataSet( level, id ) );
}


//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetRefinementRatio(unsigned int level,
                                                   int ratio)
{
  assert("pre: valid_ratio" && ratio>=2);
  if (level >= this->GetNumberOfLevels())
    {
    this->SetNumberOfLevels(level+1);
    }

  vtkInformation* info = this->Superclass::GetChildMetaData(level);
  info->Set(REFINEMENT_RATIO(), ratio);
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSet::GetRefinementRatio(unsigned int level)
{
  if (!this->Superclass::HasChildMetaData(level))
    {
    return 0;
    }

  vtkInformation* info = this->Superclass::GetChildMetaData(level);
  if (!info)
    {
    return 0;
    }
  return info->Has(REFINEMENT_RATIO())? info->Get(REFINEMENT_RATIO()): 0;
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSet::GetRefinementRatio(vtkCompositeDataIterator* iter)
{
  if (!this->HasMetaData(iter))
    {
    return 0;
    }
  vtkInformation* info = this->GetMetaData(iter);
  if (!info)
    {
    return 0;
    }
  return info->Has(REFINEMENT_RATIO())? info->Get(REFINEMENT_RATIO()): 0;
}

//----------------------------------------------------------------------------
bool vtkHierarchicalBoxDataSet::GetRootAMRBox( vtkAMRBox &root )
{
  if( (this->GetNumberOfLevels() == 0) ||
      (this->GetNumberOfDataSets(0) == 0) )
      return false;

  double min[3];
  double max[3];
  min[0] = min[1] = min[2] = std::numeric_limits<double>::max();
  max[0] = max[1] = max[2] = std::numeric_limits<double>::min();

  int lo[3];
  lo[0]=lo[1]=lo[2]=0;
  int hi[3];
  hi[0]=hi[1]=hi[2]=0;

  int dimension = 0;
  double spacing[3];

  unsigned int dataIdx = 0;
  for( ; dataIdx < this->GetNumberOfDataSets(0); ++dataIdx )
    {
      if( !this->HasMetaData( 0, dataIdx ) )
        {
        return false;
        }

      vtkAMRBox myBox;
      this->GetMetaData( 0, dataIdx, myBox );

      double boxmin[3];
      double boxmax[3];
      int    boxdims[3];

      // Note: the AMR boxes are cell dimensioned, hence,
      // in order to calculate the global dimensions, we
      // just need to sum the number of cells in the box.
      myBox.GetNumberOfCells( boxdims );

      myBox.GetMinBounds( boxmin );
      myBox.GetMaxBounds( boxmax );
//      for( int i=0; i < myBox.GetDimensionality(); ++i )
      for( int i=0; i < 3; ++i )
        {

          if( boxmin[i] < min[i] )
            min[i] = boxmin[i];

          if( boxmax[i] > max[i] )
            max[i] = boxmax[i];
        }

      dimension = myBox.GetDimensionality();
      myBox.GetGridSpacing( spacing );

    } // END for all data

  // Dimension based on CELLS and start number from 0.
  for( int i=0; i < 3; ++i )
    {
    hi[ i ] = vtkMath::Round( (max[i]-min[i])/spacing[i] )-1;
    }

  root.SetDimensionality( dimension );
  root.SetDataSetOrigin( min );
  root.SetGridSpacing( spacing );
  root.SetDimensions( lo, hi );
  root.SetLevel( 0 );
  root.SetBlockId( 0 );
  root.SetProcessId( -1 ); /* not owned, can be computed by any process */

  return true;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::GetGlobalAMRBoxWithSpacing(
    vtkAMRBox &box, double h[3] )
{
  vtkAMRBox root;
  this->GetRootAMRBox( root );

  double min[3]; double max[3];
  root.GetMinBounds( min );
  root.GetMaxBounds( max );

  int ndim[3];
  ndim[0]=ndim[1]=ndim[2]=0;

  for(int i=0; i < 3; ++i )
    {
      // Note -1 is subtracted here because the data
      // is cell-centered and we downshift to number
      // from 0.
      ndim[ i ] = vtkMath::Round( (max[i]-min[i])/h[i] )-1;
    }

  int lo[3];
  lo[0]=lo[1]=lo[2]=0;

  box.SetDimensionality( root.GetDimensionality() );
  box.SetDataSetOrigin( min );
  box.SetGridSpacing( h );
  box.SetDimensions( lo, ndim );
  box.SetLevel( 0 );
  box.SetBlockId( 0 );
  box.SetProcessId( -1 ); /* any process can compute this block */
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetCompositeIndex(
      const unsigned int level, const unsigned int index, const int idx )
{
  assert( "pre: level is out-of-bounds" &&
          ( level < this->GetNumberOfLevels() ) );
  assert( "pre: index is out-of-bounds" &&
          ( index < this->GetNumberOfDataSets( level ) )  );

  vtkInformation *metadata = this->GetMetaData( level, index );
  assert( "pre: metadata object is NULL" && (metadata != NULL) );

  vtkstd::pair< unsigned int, unsigned int > p;
  p.first  = level;
  p.second = index;
  this->CompositeIndex2LevelIdPair[ idx ] = p;
  metadata->Set( vtkCompositeDataPipeline::COMPOSITE_INDEX(), idx );
}


//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSet::GetCompositeIndex(
    const unsigned int level, const unsigned int index )
{

  assert( "pre: level is out-of-bounds" &&
           ( level < this->GetNumberOfLevels() ) );
  assert( "pre: index is out-of-bounds" &&
           ( index < this->GetNumberOfDataSets( level ) )  );

  vtkInformation *metadata = this->GetMetaData( level, index );
  assert( "pre: metadata object is NULL" && (metadata != NULL) );

  int idx = -1;
  if( metadata->Has( vtkCompositeDataPipeline::COMPOSITE_INDEX() ) )
    idx = metadata->Get( vtkCompositeDataPipeline::COMPOSITE_INDEX() );

  return( idx );
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSet::GetMetaData(
    unsigned int level, unsigned int index, vtkAMRBox &box)
{
  vtkMultiPieceDataSet* levelMDS =
   vtkMultiPieceDataSet::SafeDownCast( this->GetChild(level) );
  if( levelMDS != NULL )
    {
      vtkInformation* info = levelMDS->GetMetaData( index );
      if( info != NULL )
        {
          // Sanity Checks
          assert( "Expected Meta-data" && info->Has( BOX_DIMENSIONALITY() ) );
          assert( "Expected Meta-data" && info->Has( BOX() ) );
          assert( "Expected Meta-data" && info->Has( RANK() ) );
          assert( "Expected Meta-data" && info->Has( BOX_ORIGIN() ) );
          assert( "Expected Meta-data" && info->Has( BLOCK_ID() )  );
          assert( "Expected Meta-data" && info->Has( REAL_EXTENT() )  );
          assert( "Expected Meta-data" && info->Has( GEOMETRIC_DESCRIPTION()));

          box.SetDimensionality( info->Get( BOX_DIMENSIONALITY() ) );
          int *dims = info->Get( BOX() );
          box.SetDimensions(dims,dims+3);
          box.SetDataSetOrigin( info->Get( BOX_ORIGIN() ) );
          box.SetProcessId( info->Get( RANK() ) );
          box.SetGridDescription( info->Get( GEOMETRIC_DESCRIPTION() ) );
          box.SetBlockId( info->Get( BLOCK_ID() ) );
          box.SetRealExtent( info->Get( REAL_EXTENT() ) );
          box.SetLevel( level );
          double *spacing = info->Get( SPACING() );
          box.SetGridSpacing( spacing );

          return 1;
        }
      else
        {
//          vtkErrorMacro( "No meta-data found for requested object!\n" );
          return 0;
        }
    }
  else
    {
      vtkErrorMacro( "No data found at requested level!\n" );
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkInformation* vtkHierarchicalBoxDataSet::GetMetaData(unsigned int level,
  unsigned int index)
{
  vtkMultiPieceDataSet* levelMDS = vtkMultiPieceDataSet::SafeDownCast(
    this->GetChild(level));
  if (levelMDS)
    {
    return levelMDS->GetMetaData(index);
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSet::HasMetaData(unsigned int level,
  unsigned int index)
{
  vtkMultiPieceDataSet* levelMDS = vtkMultiPieceDataSet::SafeDownCast(
    this->GetChild(level));
  if (levelMDS)
    {
    return levelMDS->HasMetaData(index);
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSetIsInBoxes(vtkAMRBoxList& boxes,
                                       int i, int j, int k)
{
  vtkAMRBoxList::iterator it=boxes.begin();
  vtkAMRBoxList::iterator end=boxes.end();
  for(; it!=end; ++it)
    {
    if (it->Contains(i, j, k))
      {
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::GetHigherResolutionCoarsenedBoxes(
    vtkAMRBoxList &boxes, const unsigned int levelIdx )
{
  // if we are at the highest level, return immediately.
  if( levelIdx == this->GetNumberOfLevels()-1 )
    return;

  unsigned int numDataSets = this->GetNumberOfDataSets( levelIdx+1 );
  unsigned int dataSetIdx  = 0;
  for( ; dataSetIdx<numDataSets; dataSetIdx++)
    {
      if( !this->HasMetaData(levelIdx+1, dataSetIdx) ||
          !this->HasLevelMetaData(levelIdx))
        {
          if( !this->HasMetaData(levelIdx+1, dataSetIdx) )
            {
              vtkGenericWarningMacro(
               "No MetaData associated with this instance!\n" );
            }
          if( !this->HasLevelMetaData(levelIdx) )
            {
              vtkGenericWarningMacro(
               "No Level MetaData associated with this instance!\n" );
            }
          continue;
        }

      vtkAMRBox coarsebox;
      this->GetMetaData( levelIdx+1, dataSetIdx, coarsebox );
      int refinementRatio = this->GetRefinementRatio(levelIdx);
      assert( "Invalid refinement ratio!" && (refinementRatio >= 2)  );

      coarsebox.Coarsen(refinementRatio);
      boxes.push_back(coarsebox);

    } // END for all datasets

}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::BlankGridsAtLevel(
    vtkAMRBoxList &boxes, const unsigned int levelIdx )
{
  if( boxes.empty() )
    return;

  unsigned int numDataSets = this->GetNumberOfDataSets(levelIdx);
  vtkAMRBoxList::iterator it, end=boxes.end();
  vtkAMRBox box, ibox;
  int cellDims[3], N;
  vtkIdType numBlankedPts, id;
  for( unsigned int dataSetIdx=0; dataSetIdx<numDataSets; dataSetIdx++)
    {
    vtkUniformGrid* grid = this->GetDataSet(levelIdx, dataSetIdx, box);
    if (grid == NULL )
      {
      continue;
      }
    assert( "Empty AMR box!" && !box.Empty()  );
    box.GetNumberOfCells(cellDims);
    N = box.GetNumberOfCells();
    
    vtkUnsignedCharArray* vis = vtkUnsignedCharArray::New();
    vis->SetNumberOfTuples( N );
    vis->FillComponent(0,static_cast<char>(1));
    numBlankedPts = 0;
    
    // For each higher res box fill in the cells that
    // it covers
    for (it = boxes.begin(); it != end; ++it)
      {
      // First we need to copy the "higher res" box
      // since the intersection operation is done "in place"
      ibox = *it;
      if (!ibox.Intersect(box))
        {
        // We know that this higher res box does not intersect this one
        continue;
        }
      
      const int *loCorner=ibox.GetLoCorner();
      const int *hiCorner=ibox.GetHiCorner();
      for( int iz=loCorner[2]; iz<=hiCorner[2]; iz++ )
        {
        for( int iy=loCorner[1]; iy<=hiCorner[1]; iy++ )
          {
          for( int ix=loCorner[0]; ix<=hiCorner[0]; ix++ )
            {
            id = box.GetCellLinearIndex( ix, iy, iz );
            assert( "cell index out-of-bounds!" &&
                    ( (id >=0) && (id < vis->GetNumberOfTuples() ) ) );
            vis->SetValue(id, 0);
            numBlankedPts++;
            } // END for x
          } // END for y
        } // END for z
      } // Processing all higher boxes for a specific coarse grid
     
    grid->SetCellVisibilityArray(vis);
    vis->Delete();
    if( this->PadCellVisibility == true )
      {
      grid->AttachCellVisibilityToCellData();
      // Turn off visibility since it is attached as cell data
      grid->GetCellVisibilityArray()->FillComponent(
                                                    0,static_cast<char>(1));
      }

    if (this->HasMetaData(levelIdx, dataSetIdx))
      {
      vtkInformation* infotmp =
        this->GetMetaData(levelIdx,dataSetIdx);
      infotmp->Set(NUMBER_OF_BLANKED_POINTS(), numBlankedPts);
      } 
    } // END for all datasets
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::GenerateVisibilityArrays()
{

//  this->PadCellVisibility = true;
  unsigned int numLevels = this->GetNumberOfLevels();

  for (unsigned int levelIdx=0; levelIdx<numLevels; levelIdx++)
    {
      // Copy boxes of higher level and coarsen to this level
      vtkAMRBoxList boxes;
      this->GetHigherResolutionCoarsenedBoxes( boxes, levelIdx );

      this->BlankGridsAtLevel( boxes, levelIdx );
    } // END for all low-res levels
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSet::GetTotalNumberOfBlocks( )
{
  int totalNumBlocks     = 0;
  unsigned int numLevels = this->GetNumberOfLevels();
  for( unsigned int levelIdx=0; levelIdx < numLevels; ++levelIdx )
    totalNumBlocks += this->GetNumberOfDataSets( levelIdx );

  return( totalNumBlocks );
}

//----------------------------------------------------------------------------
vtkAMRBox vtkHierarchicalBoxDataSet::GetAMRBox(vtkCompositeDataIterator* iter)
{
  vtkAMRBox box;
  if (this->HasMetaData(iter))
    {
      vtkInformation* info = this->GetMetaData(iter);
      // Sanity Checks
      assert( "Expected Meta-data" && info->Has( BOX_DIMENSIONALITY() ) );
      assert( "Expected Meta-data" && info->Has( BOX() ) );
      assert( "Expected Meta-data" && info->Has( RANK() ) );
      assert( "Expected Meta-data" && info->Has( BOX_ORIGIN() ) );
      assert( "Expected Meta-data" && info->Has( BLOCK_ID() )  );
      assert( "Expected Meta-data" && info->Has( REAL_EXTENT() )  );

      box.SetDimensionality( info->Get( BOX_DIMENSIONALITY() ) );
      int *dims = info->Get( BOX() );
      box.SetDimensions(dims,dims+3);
      box.SetDataSetOrigin( info->Get( BOX_ORIGIN() ) );
      box.SetProcessId( info->Get( RANK() ) );
      box.SetBlockId( info->Get( BLOCK_ID() ) );
      box.SetRealExtent( info->Get( REAL_EXTENT() ) );
// TODO: How can we determine the level from an iterator? One possibility is
// to store that information in the vtkInformation object, but, that requires
// more storage and hashing.
      double *spacing = info->Get( SPACING() );
      box.SetGridSpacing( spacing );
    }
  else
    {
      vtkErrorMacro( "No Metadata found for item found!" );
    }
  return box;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  /*
  unsigned int numLevels = this->GetNumberOfLevels();
  os << indent << "Number of levels: " <<  numLevels << endl;
  for (unsigned int i=0; i<numLevels; i++)
    {
    unsigned int numDataSets = this->GetNumberOfDataSets(i);
    os << indent << "Level " << i << " number of datasets: " << numDataSets
       << endl;
    for (unsigned j=0; j<numDataSets; j++)
      {
      os << indent << "DataSet(" << i << "," << j << "):";
      vtkDataObject* dobj = this->GetDataSet(i, j);
      if (dobj)
        {
        os << endl;
        dobj->PrintSelf(os, indent.GetNextIndent());
        }
      else
        {
        os << "(none)" << endl;
        }
      }
    }
    */
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalBoxDataSet::GetFlatIndex(unsigned int level, 
  unsigned int index)
{
  if (level > this->GetNumberOfLevels() || index > this->GetNumberOfDataSets(level))
    {
    // invalid level, index.
    vtkErrorMacro("Invalid level (" << level << ") or index (" << index << ")");
    return 0;
    }


  unsigned int findex=0;
  for (unsigned int l=0; l < level; l++)
    {
    findex += 1;
    findex += this->GetNumberOfDataSets(l);
    }
  findex += 1;
  findex += (index + 1);
  return findex;


}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::GetLevelAndIndex(
    const unsigned int flatIdx, unsigned int &level, unsigned int &idx )
{

  level = 0;
  idx   = 0;
  if( this->CompositeIndex2LevelIdPair.find( flatIdx ) !=
      this->CompositeIndex2LevelIdPair.end() )
    {
    std::cout << "Found: " << flatIdx << "!";
    std::cout.flush();

    vtkstd::pair< unsigned int, unsigned int > p=
          this->CompositeIndex2LevelIdPair[ flatIdx ];
    level = p.first;
    idx   = p.second;
    }
  else
    {
    std::cout << "Could not find " << flatIdx << std::endl;
    std::cout.flush();

    vtkErrorMacro( "Could not find block index: " << flatIdx );
    }
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::Clear()
{
  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* vtkHierarchicalBoxDataSet::GetData(
  vtkInformation* info)
{
  return
    info?vtkHierarchicalBoxDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* vtkHierarchicalBoxDataSet::GetData(
  vtkInformationVector* v, int i)
{
  return vtkHierarchicalBoxDataSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
// Description:
// Copy the cached scalar range into range.
void vtkHierarchicalBoxDataSet::GetScalarRange(double range[2])
{
  this->ComputeScalarRange();
  range[0]=this->ScalarRange[0];
  range[1]=this->ScalarRange[1];
}
  
//----------------------------------------------------------------------------
// Description:
// Return the cached range.
double *vtkHierarchicalBoxDataSet::GetScalarRange()
{
  this->ComputeScalarRange();
  return this->ScalarRange;
}

//----------------------------------------------------------------------------
// Description:
// Compute the range of the scalars and cache it into ScalarRange
// only if the cache became invalid (ScalarRangeComputeTime).
void vtkHierarchicalBoxDataSet::ComputeScalarRange()
{
  if ( this->GetMTime() > this->ScalarRangeComputeTime )
    {
    double dataSetRange[2];
    this->ScalarRange[0]=VTK_DOUBLE_MAX;
    this->ScalarRange[1]=VTK_DOUBLE_MIN;
    unsigned int level=0;
    unsigned int levels=this->GetNumberOfLevels();
    vtkAMRBox temp;
    while(level<levels)
      {
      unsigned int dataset=0;
      unsigned int datasets=this->GetNumberOfDataSets(level);
      while(dataset<datasets)
        {
        vtkUniformGrid *ug = 
          static_cast<vtkUniformGrid*>(this->GetDataSet(level, dataset, temp));
        ug->GetScalarRange(dataSetRange);
        if(dataSetRange[0]<this->ScalarRange[0])
          {
          this->ScalarRange[0]=dataSetRange[0];
          }
        if(dataSetRange[1]>this->ScalarRange[1])
          {
          this->ScalarRange[1]=dataSetRange[1];
          }
        ++dataset;
        }
      ++level;
      }
    this->ScalarRangeComputeTime.Modified();
    }
}
//----------------------------------------------------------------------------
// Description:
// Get the Bounds of the Data
double *vtkHierarchicalBoxDataSet::GetBounds()
{
  vtkAMRBox amrBox;
  if( this->GetRootAMRBox( amrBox ) )
    {
    amrBox.GetBounds(this->Bounds);
    }
  else
    {
    this->Bounds[0] = VTK_DOUBLE_MAX;
    this->Bounds[1] = VTK_DOUBLE_MIN;
    this->Bounds[2] = VTK_DOUBLE_MAX;
    this->Bounds[3] = VTK_DOUBLE_MIN;
    this->Bounds[4] = VTK_DOUBLE_MAX;
    this->Bounds[5] = VTK_DOUBLE_MIN;

    double tmpbounds[6];
    unsigned int levelIdx=0;
    for( ; levelIdx < this->GetNumberOfLevels(); ++levelIdx )
      {
      unsigned int dataIdx = 0;
      for( ; dataIdx < this->GetNumberOfDataSets( levelIdx ); ++dataIdx )
        {
        vtkUniformGrid *grd = this->GetDataSet( levelIdx, dataIdx );
        if( grd != NULL )
          {
          grd->GetBounds( tmpbounds );
          for( int i=0; i < 3; ++i )
            {
            if( tmpbounds[i*2] < this->Bounds[i*2] )
              {
              this->Bounds[i*2] = tmpbounds[i*2];
              }
            if( tmpbounds[i*2+1] > this->Bounds[i*2+1] )
              {
              this->Bounds[i*2+1] = tmpbounds[i*2+1];
              }
            } // END for each dimension
          } // END if grid is not NULL
        } // END for all data at level
      } // END for all levels
    }

  return this->Bounds;
}
//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::GetBounds(double bounds[6])
{
  this->GetBounds();
  bounds[0] = this->Bounds[0];
  bounds[1] = this->Bounds[1];
  bounds[2] = this->Bounds[2];
  bounds[3] = this->Bounds[3];
  bounds[4] = this->Bounds[4];
  bounds[5] = this->Bounds[5];
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::ShallowCopy( vtkDataObject *src )
{
  if( src == this )
    {
    return;
    }

  this->Superclass::ShallowCopy( src );

  vtkHierarchicalBoxDataSet *hbds =
      vtkHierarchicalBoxDataSet::SafeDownCast(src);

  if( hbds != NULL )
    {
    // Copy all the meta-data
    vtkAMRBox box;
    unsigned int levelIdx = 0;
    for( ;levelIdx < hbds->GetNumberOfLevels(); ++levelIdx )
      {
      unsigned int dataIdx = 0;
      for( ;dataIdx < hbds->GetNumberOfDataSets( levelIdx ); ++dataIdx )
        {
        if( hbds->HasMetaData( levelIdx, dataIdx ) )
          {
          hbds->GetMetaData( levelIdx, dataIdx, box );
          this->SetMetaData( levelIdx, dataIdx, box );
          }
        } // END for all data
      } // END for all levels
    } // END if hbds


  this->Modified();
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::DeepCopy( vtkDataObject *src )
{
  if( src == this )
    {
    return;
    }

  this->Superclass::DeepCopy( src );

  vtkHierarchicalBoxDataSet *hbds =
      vtkHierarchicalBoxDataSet::SafeDownCast(src);

  if( hbds != NULL )
    {
    // Copy all the meta-data
    vtkAMRBox box;
    unsigned int levelIdx = 0;
    for( ;levelIdx < hbds->GetNumberOfLevels(); ++levelIdx )
      {
      unsigned int dataIdx = 0;
      for( ;dataIdx < hbds->GetNumberOfDataSets( levelIdx ); ++dataIdx )
        {
        if( hbds->HasMetaData( levelIdx, dataIdx ) )
          {
          hbds->GetMetaData( levelIdx, dataIdx, box );
          this->SetMetaData( levelIdx, dataIdx, box );
          }
        } // END for all data
      } // END for all levels
    } // END if hbds


  this->Modified();
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::CopyStructure( vtkCompositeDataSet *src )
{

  if( src == this )
    {
    return;
    }

  this->Superclass::CopyStructure( src );

  vtkHierarchicalBoxDataSet *hbds =
      vtkHierarchicalBoxDataSet::SafeDownCast(src);

  if( hbds != NULL )
    {
    // Copy all the meta-data
    vtkAMRBox box;
    unsigned int levelIdx = 0;
    for( ;levelIdx < hbds->GetNumberOfLevels(); ++levelIdx )
      {
      unsigned int dataIdx = 0;
      for( ;dataIdx < hbds->GetNumberOfDataSets( levelIdx ); ++dataIdx )
        {
        if( hbds->HasMetaData( levelIdx, dataIdx ) )
          {
          hbds->GetMetaData( levelIdx, dataIdx, box );
          this->SetMetaData( levelIdx, dataIdx, box );
          }
        } // END for all data
      } // END for all levels
    } // END if hbds


  this->Modified();
}
