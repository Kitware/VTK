/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformGridAMR.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOverlappingAMR.h"

#include "vtkAMRBox.h"
#include "vtkUniformGridAMRDataIterator.h"
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
#include "vtkUnsignedIntArray.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkType.h"

#include <cmath>
#include <cassert>

vtkStandardNewMacro(vtkOverlappingAMR);

vtkInformationKeyRestrictedMacro(vtkOverlappingAMR,BOX,IntegerVector, 6);
vtkInformationKeyMacro(vtkOverlappingAMR,NUMBER_OF_BLANKED_POINTS,IdType);
vtkInformationKeyMacro(vtkOverlappingAMR,REFINEMENT_RATIO,Integer);
vtkInformationKeyMacro(vtkOverlappingAMR,BOX_DIMENSIONALITY,Integer);
vtkInformationKeyRestrictedMacro(vtkOverlappingAMR,BOX_ORIGIN,DoubleVector,3);
vtkInformationKeyRestrictedMacro(vtkOverlappingAMR,SPACING,DoubleVector,3);
vtkInformationKeyMacro(vtkOverlappingAMR,RANK,Integer);
vtkInformationKeyMacro(vtkOverlappingAMR,BLOCK_ID,Integer);
vtkInformationKeyMacro(vtkOverlappingAMR,GEOMETRIC_DESCRIPTION,Integer);
vtkInformationKeyRestrictedMacro(vtkOverlappingAMR,REAL_EXTENT,IntegerVector,6);

typedef std::vector<vtkAMRBox> vtkAMRBoxList;
//----------------------------------------------------------------------------
vtkOverlappingAMR::vtkOverlappingAMR()
{
  this->ScalarRange[0]    = VTK_DOUBLE_MAX;
  this->ScalarRange[1]    = VTK_DOUBLE_MIN;
  this->PadCellVisibility = false;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Bounds[0] = this->Bounds[1] = this->Bounds[2] =
  this->Bounds[3] = this->Bounds[4] = this->Bounds[5] = 0.0;
  this->ParentInformation = NULL;
  this->ParentInformationMap = NULL;
  this->ChildrenInformation = NULL;
  this->ChildrenInformationMap = NULL;
  this->LevelMap = NULL;
}

//----------------------------------------------------------------------------
vtkOverlappingAMR::~vtkOverlappingAMR()
{
  AssignUnsignedIntArray(&(this->LevelMap), NULL);
  AssignUnsignedIntArray(&(this->ChildrenInformation), NULL);
  AssignUnsignedIntArray(&(this->ChildrenInformationMap), NULL);
  AssignUnsignedIntArray(&(this->ParentInformation), NULL);
  AssignUnsignedIntArray(&(this->ParentInformationMap), NULL);
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR::SetOrigin( const double o[3] )
{
  for( int i=0; i < 3; ++i )
    {
    this->Origin[i] = o[i];
    }
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR::GetOrigin( double o[3] )
{
  for( int i=0; i < 3; ++i )
    {
    o[i] = this->Origin[i];
    }
}


//----------------------------------------------------------------------------
void vtkOverlappingAMR::SetDataSet(
  unsigned int level, unsigned int id,
  int LoCorner[3], int HiCorner[3], vtkUniformGrid* dataSet)
{
  vtkAMRBox box(3, LoCorner, HiCorner);
  this->SetDataSet(level, id, box, dataSet);
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR::SetDataSet(
  unsigned int level, unsigned int id, vtkAMRBox &box, vtkUniformGrid *dataSet)
{
  this->SetDataSet( level, id, dataSet );
  this->SetMetaData( level, id, box );
}

//----------------------------------------------------------------------------
vtkUniformGrid* vtkOverlappingAMR::GetDataSet(
    unsigned int level, unsigned int id, vtkAMRBox& box)
{
  this->GetMetaData(level,id,box);
  return( this->GetDataSet(level,id) );
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::SetMetaData(
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
      {
      levelDS->SetPiece( id, NULL);
      }

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

//----------------------------------------------------------------------------
void vtkOverlappingAMR::SetRefinementRatio(unsigned int level,
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
int vtkOverlappingAMR::GetRefinementRatio(unsigned int level)
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
int vtkOverlappingAMR::GetRefinementRatio(vtkCompositeDataIterator* iter)
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
bool vtkOverlappingAMR::GetRootAMRBox( vtkAMRBox &root )
{
  if( (this->GetNumberOfLevels() == 0) ||
      (this->GetNumberOfDataSets(0) == 0) )
      {
      return false;
      }

  double min[3];
  double max[3];
  min[0] = min[1] = min[2] = VTK_DOUBLE_MAX;
  max[0] = max[1] = max[2] = VTK_DOUBLE_MIN;

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
    for( int i=0; i < 3; ++i )
      {
      if( boxmin[i] < min[i] )
        {
        min[i] = boxmin[i];
        }

      if( boxmax[i] > max[i] )
        {
        max[i] = boxmax[i];
        }
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
void vtkOverlappingAMR::GetGlobalAMRBoxWithSpacing(
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
void vtkOverlappingAMR::SetCompositeIndex(
      const unsigned int level, const unsigned int index, const int idx )
{
  assert( "pre: level is out-of-bounds" &&
          ( level < this->GetNumberOfLevels() ) );
  assert( "pre: index is out-of-bounds" &&
          ( index < this->GetNumberOfDataSets( level ) )  );

  vtkInformation *metadata = this->GetMetaData( level, index );
  assert( "pre: metadata object is NULL" && (metadata != NULL) );

  std::pair< unsigned int, unsigned int > p;
  p.first  = level;
  p.second = index;
  this->CompositeIndex2LevelIdPair[ idx ] = p;
  metadata->Set( vtkCompositeDataSet::COMPOSITE_INDEX(), idx );
}


//----------------------------------------------------------------------------
int vtkOverlappingAMR::GetCompositeIndex(
    const unsigned int level, const unsigned int index )
{

  assert( "pre: level is out-of-bounds" &&
           ( level < this->GetNumberOfLevels() ) );
  assert( "pre: index is out-of-bounds" &&
           ( index < this->GetNumberOfDataSets( level ) )  );

  vtkInformation *metadata = this->GetMetaData( level, index );
  assert( "pre: metadata object is NULL" && (metadata != NULL) );

  int idx = -1;
  if( metadata->Has( vtkCompositeDataSet::COMPOSITE_INDEX() ) )
    {
    idx = metadata->Get( vtkCompositeDataSet::COMPOSITE_INDEX() );
    }

  return( idx );
}

//----------------------------------------------------------------------------
int vtkOverlappingAMR::GetMetaData(
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
int vtkOverlappingAMRIsInBoxes(vtkAMRBoxList& boxes,
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
void vtkOverlappingAMR::GetHigherResolutionCoarsenedBoxes(
    vtkAMRBoxList &boxes, const unsigned int levelIdx )
{
  // if we are at the highest level, return immediately.
  if( levelIdx == this->GetNumberOfLevels()-1 )
    return;

  if( !this->HasLevelMetaData(levelIdx) )
    {
    vtkGenericWarningMacro("No Level MetaData associated with this instance!\n");
    return;
    }
  unsigned int numDataSets = this->GetNumberOfDataSets( levelIdx+1 );
  unsigned int dataSetIdx  = 0;
  int refinementRatio = this->GetRefinementRatio(levelIdx);
  assert( "Invalid refinement ratio!" && (refinementRatio >= 2)  );

  for( ; dataSetIdx<numDataSets; dataSetIdx++)
    {
    if( !this->HasMetaData(levelIdx+1, dataSetIdx) )
      {
      vtkGenericWarningMacro("No MetaData associated with this instance!\n" );
      continue;
      }

    vtkAMRBox coarsebox;
    this->GetMetaData( levelIdx+1, dataSetIdx, coarsebox );

    coarsebox.Coarsen(refinementRatio);
    boxes.push_back(coarsebox);
    } // END for all datasets
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR::GetBoxesFromLevel(
    const unsigned int levelIdx,
    vtkAMRBoxList &boxes)
{
  boxes.clear();
  unsigned int numDataSets = this->GetNumberOfDataSets( levelIdx);
  unsigned int dataSetIdx  = 0;
  vtkAMRBox box;
  for( ; dataSetIdx<numDataSets; dataSetIdx++)
    {
    if( !this->HasMetaData(levelIdx, dataSetIdx) )
      {
      vtkGenericWarningMacro( "No MetaData associated with this instance!\n" );
      continue;
      }
    this->GetMetaData( levelIdx, dataSetIdx, box );
    boxes.push_back(box);
    } // END for all datasets
}

//----------------------------------------------------------------------------

bool vtkOverlappingAMR::
HasChildrenInformation()
{
  bool hasChildrenInfo = this->ChildrenInformation!=NULL;
  return hasChildrenInfo;
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR::
GenerateParentChildLevelInformation(const unsigned int levelIdx,
                                    vtkAMRBoxList &lboxes,
                                    vtkAMRBoxList &nlboxes)
{
  if( lboxes.empty() )
    {
    nlboxes.clear();
    return;
    }

  // Get the boxes for the next level
  this->GetBoxesFromLevel(levelIdx+1, nlboxes);

  vtkAMRBoxList::iterator it, end=lboxes.end(), nit, nend=nlboxes.end();

  // Get the refinement ratio between this and the next level
  int refinementRatio = this->GetRefinementRatio(levelIdx);
  assert( "Invalid refinement ratio!" && (refinementRatio >= 2)  );

  // for the child parent relationships we need an array to hold how many
  // parents each child has - for trees this will always be 1
  std::vector<unsigned int> parentsVec;
  int i, j, n = (int)nlboxes.size();
  int childInfoStartIndex = this->ChildrenInformation->GetNumberOfTuples();
  int numChildren;
  int childrenSizePos;
  parentsVec.resize(n);
  parentsVec.assign(0, n);
  unsigned int lbid, nlbid;
  // For each block determine which of the higher res blocks intersect it
  // (they will be considered its children)
  for (it = lboxes.begin(), lbid = 0; it != end; ++it, ++lbid)
    {
    // Get the box and refine it to its child's level
    it->Refine(refinementRatio);
    // Get the position of where the number of children information is going to be
    // stored for this block
    childrenSizePos = this->ChildrenInformation->GetNumberOfTuples();
    // Store this starting position in the children map and insert an initial number of
    // children size of 0 into the children information
    this->ChildrenInformationMap->InsertNextValue(childrenSizePos);
    this->ChildrenInformation->InsertNextValue(0); // Initially the block has no children
    numChildren = 0;
    // Now see if the block intersects any of the boxes in the more refined level
    for (nit = nlboxes.begin(), nlbid = 0; nit != nend; ++nit, ++nlbid)
      {
      if (!it->DoesIntersect(*nit))
        {
        continue; // No Intersection
        }

      // Insert the "child"
      this->ChildrenInformation->InsertNextValue(nlbid);
      ++parentsVec[nlbid];
      ++numChildren;
      }
    // OK - we've processed this block so lets update its number of children
    if (numChildren)
      {
      this->ChildrenInformation->SetValue(childrenSizePos, numChildren);
      }
    }

  // At this point the Parent and Children Maps should be the same size
  assert("Children and Parent Maps are not the same size!" &&
         (this->ChildrenInformationMap->GetNumberOfTuples() ==
          this->ParentInformationMap->GetNumberOfTuples()));

  // Store where the next level blocks will begin
  this->LevelMap->SetValue(levelIdx+1, this->ChildrenInformationMap->GetNumberOfTuples());

  // Now we need to create the Parent Information of the next level based on
  // the Children information we just created - First based on the parent information
  // lets insert the number of parents and dummy data for the parent block id
  for (nit = nlboxes.begin(), nlbid = 0; nit != nend; ++nit, ++nlbid)
    {
    n = parentsVec[nlbid];
    // n should never be 0 - it must always have a parent!
    assert("Found Orphan Block" && (n > 0));
    this->ParentInformationMap->InsertNextValue(this->ParentInformation->InsertNextValue(n));
    // Now lets store where we are going to start storing the parent block ids for this block
    parentsVec[nlbid] = this->ParentInformation->InsertNextValue(0);
    // Now store dummy values for the rest of the ids as well
    for (i = 1; i < n; i++)
      {
      this->ParentInformation->InsertNextValue(0);
      }
    }
  // Now we can go through the Children Information of the previous level and fill in
  // the proper parent block id values - doing it this way avoids doing mulitple passes through
  // the children information which would be O(n2)
  n = this->ChildrenInformation->GetNumberOfTuples();
  for (i = childInfoStartIndex, lbid = 0; i < n; ++lbid)
    {
    //Get the number of children for this block
    n = this->ChildrenInformation->GetValue(i++);
    for (j = 0; j < n; j++)
      {
      nlbid = this->ChildrenInformation->GetValue(i++);
      // Update the Parent Information for this child block
      this->ParentInformation->SetValue(parentsVec[nlbid], lbid);
      // advance the position of the next available parent id position
      ++parentsVec[nlbid];
      }
    }
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR::BlankGridsAtLevel(
    vtkAMRBoxList &boxes, const unsigned int levelIdx )
{
  if( boxes.empty() )
    {
    // ASSUME that we don't have to clear the visibility arrays for
    // the grids at this level!
    return;
    }
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
      grid->GetCellVisibilityArray()->FillComponent( 0,static_cast<char>(1) );
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
void vtkOverlappingAMR::GenerateVisibilityArrays()
{
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
void vtkOverlappingAMR::GenerateParentChildInformation()
{
  // Note that we do not add the  the
  // children of the max level to our arrays - hence the
  // GetChildren() methods must take this into consideration
  AssignUnsignedIntArray(&(this->LevelMap), NULL);
  AssignUnsignedIntArray(&(this->ChildrenInformation), NULL);
  AssignUnsignedIntArray(&(this->ChildrenInformationMap), NULL);
  AssignUnsignedIntArray(&(this->ParentInformation), NULL);
  AssignUnsignedIntArray(&(this->ParentInformationMap), NULL);
  unsigned int numLevels = this->GetNumberOfLevels();
  // If there are no levels just return
  if (!numLevels)
    {
    return;
    }

  this->LevelMap = vtkUnsignedIntArray::New();
  this->ChildrenInformation = vtkUnsignedIntArray::New();
  this->ChildrenInformationMap = vtkUnsignedIntArray::New();
  this->ParentInformation = vtkUnsignedIntArray::New();
  this->ParentInformationMap = vtkUnsignedIntArray::New();


  this->LevelMap->SetNumberOfTuples(numLevels);
  // Set the first block of level 0 is the first entry in the map arrays
  this->LevelMap->SetValue(0,0);

  vtkAMRBoxList boxes[2];
//  int i, j;
  // Grab the boxes at level 0
  this->GetBoxesFromLevel(0, boxes[0]);
  // We need to insert the level 0 parent information into the Parent Information Arrays since the
  // Generate Parent Child Level Information creates the Children information for level i
  // and the Parents of level i+1
  //  Though all of the blocks in level 0 have no parents we are creating parent information
  // so that the LevelMap can be used for both the parent and children information (else we
  // would need to separate maps) - since the number of blocks in level 0 is very small this
  // does not seem to be a big deal

  vtkAMRBoxList::iterator it, end=boxes[0].end();
  // Insert 0 parents into the Parent Information array
  this->ParentInformation->InsertNextValue(0);
  for (it = boxes[0].begin(); it != end; ++it)
    {
    // Have all the blocks in level 0 point to 0th entry in the Parent Information
    this->ParentInformationMap->InsertNextValue(0);
    }

  for (unsigned int levelIdx=0, i= 0, j = 1; levelIdx<numLevels-1; levelIdx++)
    {
    this->GenerateParentChildLevelInformation(levelIdx, boxes[i], boxes[j]);
    if (i)
      {
      i = 0;
      j = 1;
      }
    else
      {
      i = 1;
      j = 0;
      }
    }
}


//----------------------------------------------------------------------------
vtkAMRBox vtkOverlappingAMR::GetAMRBox(vtkCompositeDataIterator* iter)
{
  vtkUniformGridAMRDataIterator *amrIter =
      vtkUniformGridAMRDataIterator::SafeDownCast( iter );
  assert( "pre: Cannot downcast to an AMR iterator" && (amrIter != NULL) );

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
    box.SetLevel( amrIter->GetCurrentLevel() );
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
void vtkOverlappingAMR::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "PadCellVisibility: ";
  if( this->PadCellVisibility )
    {
    os << "ON(";
    }
  else
    {
    os << "OFF(";
    }
  os << this->PadCellVisibility << ")\n";
}

//----------------------------------------------------------------------------
unsigned int vtkOverlappingAMR::GetFlatIndex(unsigned int level,
  unsigned int index)
{
  if (level > this->GetNumberOfLevels() ||
      index > this->GetNumberOfDataSets(level))
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
void vtkOverlappingAMR::GetLevelAndIndex(
    const unsigned int flatIdx, unsigned int &level, unsigned int &idx )
{

  level = 0;
  idx   = 0;
  if( this->CompositeIndex2LevelIdPair.find( flatIdx ) !=
      this->CompositeIndex2LevelIdPair.end() )
    {
    std::pair< unsigned int, unsigned int > p=
          this->CompositeIndex2LevelIdPair[ flatIdx ];
    level = p.first;
    idx   = p.second;
    }
  else
    {
    vtkErrorMacro( "Could not find block index: " << flatIdx );
    }
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR::Clear()
{
  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
vtkOverlappingAMR* vtkOverlappingAMR::GetData(
  vtkInformation* info)
{
  return
    info?vtkOverlappingAMR::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkOverlappingAMR* vtkOverlappingAMR::GetData(
  vtkInformationVector* v, int i)
{
  return vtkOverlappingAMR::GetData(v->GetInformationObject(i));
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::AssignUnsignedIntArray(vtkUnsignedIntArray **dest,
                                               vtkUnsignedIntArray *src)
{
  if (*dest == src)
    {
    return;
    }
  if (*dest)
    {
    (*dest)->Delete();
    }
  *dest = src;
  if (src)
    {
    (*dest)->Register(0);
    }
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::ShallowCopy( vtkDataObject *src )
{
  if( src == this )
    {
    return;
    }

  this->Superclass::ShallowCopy( src );

  vtkOverlappingAMR *hbds =
      vtkOverlappingAMR::SafeDownCast(src);

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

  AssignUnsignedIntArray(&(this->LevelMap), hbds->LevelMap);
  AssignUnsignedIntArray(&(this->ChildrenInformation), hbds->ChildrenInformation);
  AssignUnsignedIntArray(&(this->ChildrenInformationMap), hbds->ChildrenInformationMap);
  AssignUnsignedIntArray(&(this->ParentInformation), hbds->ParentInformation);
  AssignUnsignedIntArray(&(this->ParentInformationMap), hbds->ParentInformationMap);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::DeepCopy( vtkDataObject *src )
{
  if( src == this )
    {
    return;
    }

  this->Superclass::DeepCopy( src );

  vtkOverlappingAMR *hbds =
      vtkOverlappingAMR::SafeDownCast(src);

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


  AssignUnsignedIntArray(&(this->LevelMap), hbds->LevelMap);
  AssignUnsignedIntArray(&(this->ChildrenInformation), hbds->ChildrenInformation);
  AssignUnsignedIntArray(&(this->ChildrenInformationMap), hbds->ChildrenInformationMap);
  AssignUnsignedIntArray(&(this->ParentInformation), hbds->ParentInformation);
  AssignUnsignedIntArray(&(this->ParentInformationMap), hbds->ParentInformationMap);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::CopyStructure( vtkCompositeDataSet *src )
{
  if( src == this )
    {
    return;
    }

  this->Superclass::CopyStructure( src );

  vtkOverlappingAMR *hbds =
      vtkOverlappingAMR::SafeDownCast(src);

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

  this->CompositeIndex2LevelIdPair = hbds->CompositeIndex2LevelIdPair;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkOverlappingAMR::ComputeBounds()
{
  vtkAMRBox rootAMRBox;
  if( this->GetRootAMRBox( rootAMRBox ) )
    {
    rootAMRBox.GetBounds( this->Bounds );
    }
  else
    {
    this->Superclass::ComputeBounds();
    }

}

//----------------------------------------------------------------------------
unsigned int *vtkOverlappingAMR::
GetParents(unsigned int level, unsigned int index)
{
  // If we are at level 0 there are no parents or if there is no level
  // map there is no parent information
  if ((!level) || (this->LevelMap == NULL))
    {
    return NULL;
    }
  unsigned int startLevel = this->LevelMap->GetValue(level);
  unsigned int blockPos = startLevel+index;
  unsigned int parentInfo = this->ParentInformationMap->GetValue(blockPos);
  return this->ParentInformation->GetPointer(parentInfo);
}

//------------------------------------------------------------------------------
unsigned int *vtkOverlappingAMR::
GetChildren(unsigned int level, unsigned int index)
{
  // If there is no level map or if the level is the max level
  // there is no children information
  if ((this->LevelMap == NULL) ||
      (this->LevelMap->GetNumberOfTuples() == (level+1)))
    {
    return NULL;
    }
  unsigned int startLevel = this->LevelMap->GetValue(level);
  unsigned int blockPos = startLevel+index;
  unsigned int childInfo = this->ChildrenInformationMap->GetValue(blockPos);
  return this->ChildrenInformation->GetPointer(childInfo);
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::
PrintParentChildInfo(unsigned int level, unsigned int index)
{
  unsigned int *ptr, i, n;
  std::cerr << "Parent Child Info for block " << index
            << " of Level: " << level << endl;
  ptr = this->GetParents(level, index);
  if ((!ptr) || (ptr[0] == 0))
    {
    std::cerr << "\tParents: None" << endl;
    }
  else
    {
    std::cerr << "\tParents: ";
    n = ptr[0];
    for (i = 1; i <= n; i++)
      {
      std::cerr << ptr[i] << " ";
      }
    std::cerr << endl;
    }
  ptr = this->GetChildren(level, index);
  if ((!ptr) || (ptr[0] == 0))
    {
    std::cerr << "\tChildren: None" << endl;
    }
  else
    {
    std::cerr << "\tChildren: ";
    n = ptr[0];
    for (i = 1; i <= n; i++)
      {
      std::cerr << ptr[i] << " ";
      }
    std::cerr << endl;
    }
}
