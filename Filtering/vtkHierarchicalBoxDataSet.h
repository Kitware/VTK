/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalBoxDataSet - hierarchical dataset of vtkUniformGrids
//
// .SECTION Description
// vtkHierarchicalBoxDataSet is a concrete implementation of
// vtkCompositeDataSet. The dataset type is restricted to
// vtkUniformGrid. Each dataset has an associated vtkAMRBox that represents
// it's region (similar to extent) in space.
//
// .SECTION Warning
// To compute the cellId of a cell within a vtkUniformGrid with AMRBox=box, 
// you should not use vtkUniformGrid::ComputeCellId( {x,y,z} ) but instead
// use the following pseudo code:
// for (int i=0; i<3; i++)
//   {
//   cellDims[i] = box.HiCorner[i] - box.LoCorner[i] + 1;
//   }
// vtkIdType cellId =
//   (z-box.LoCorner[2])*cellDims[0]*cellDims[1] +
//   (y-box.LoCorner[1])*cellDims[0] +
//   (x-box.LoCorner[0]);
//
// NOTE vtkAMRBox is used to compute cell visibility, therefor it 
// should be dimensioned according to the visible region.


#ifndef __vtkHierarchicalBoxDataSet_h
#define __vtkHierarchicalBoxDataSet_h

#include "vtkCompositeDataSet.h"
#include <vtkstd/vector>    // For STL vector
#include <vtkstd/map>       // For STL map
#include <vtkstd/utility>   // For STL pair

class vtkAMRBox;
class vtkInformationIdTypeKey;
class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;
class vtkUniformGrid;

typedef vtkstd::vector<vtkAMRBox> vtkAMRBoxList;

class VTK_FILTERING_EXPORT vtkHierarchicalBoxDataSet: public vtkCompositeDataSet
{
public:
  static vtkHierarchicalBoxDataSet *New();
  vtkTypeMacro(vtkHierarchicalBoxDataSet,vtkCompositeDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Descrition:
  // Set & Get the AMR dataset origin
  // The origin is essentially the minimum of all the grids.
  void SetOrigin( const double origin[3] );
  void GetOrigin( double origin[3] );

  // Description:
  // Return a new iterator (the iterator has to be deleted by user).
  virtual vtkCompositeDataIterator* NewIterator();

  // Description:
  // Return class name of data type (see vtkType.h for definitions).
  virtual int GetDataObjectType() {return VTK_HIERARCHICAL_BOX_DATA_SET;}

  // Description:
  // Set the number of refinement levels. This call might cause
  // allocation if the new number of levels is larger than the
  // current one.
  void SetNumberOfLevels(unsigned int numLevels);

  // Description:
  // Returns the number of levels.
  unsigned int GetNumberOfLevels();

  // Description:
  // Set the number of data set at a given level.
  void SetNumberOfDataSets(unsigned int level, unsigned int numdatasets);

  // Description:
  // Returns the number of data sets available at any level.
  unsigned int GetNumberOfDataSets(unsigned int level);

  // Description:
  // Sets the data set at the location pointed by the iterator.
  // The iterator does not need to be iterating over this dataset itself. It can
  // be any composite datasite with similar structure (achieve by using
  // CopyStructure).
  // Un-hiding superclass overload.
  virtual void SetDataSet(vtkCompositeDataIterator* iter, vtkDataObject* dataObj)
    { this->Superclass::SetDataSet(iter, dataObj); }

  // Description:
  // This method returns the root AMR box for the entire root level.
  // The root AMR box covers the entire domain.
  void GetRootAMRBox( vtkAMRBox &root );

  // Description:
  // This method returns the global AMR box, covering the entire
  // domain, with the prescribed spacing.
  void GetGlobalAMRBoxWithSpacing( vtkAMRBox &box, double h[3] );

  // Description:
  // Set the dataset pointer for a given node. This will resize the number of
  // levels and the number of datasets in the level to fit level, id requested. 
  void SetDataSet(unsigned int level, unsigned int id, 
                  int LoCorner[3], int HiCorner[3], vtkUniformGrid* dataSet);

  // Description:
  // Set the dataset pointer for a given node without any metadata. This will
  // resize the number of levels and the number of datasets accordingly.
  void SetDataSet(unsigned int level, unsigned int id, vtkUniformGrid* dataSet);

  // Description:
  // Appends the dataset to the given level. This will resize the
  // number of levels and the number of datasets accordingly.
  void AppendDataSet(unsigned int level, vtkUniformGrid* dataSet );

  // Description:
  // Sets the meta-data object at a given node. This will resize the number
  // of levels and number of datasets acoordingly.
  void SetMetaData(unsigned int level, unsigned int id, const vtkAMRBox &box );

//BTX
  // Description:
  // Set the dataset pointer for a given node. This will resize the number of
  // levels and the number of datasets in the level to fit level, id requested. 
  // The information carried by the vtkAMRBox is redundant with the extent
  // of the vtkUniformGrid. However, in case of parallel computation, the
  // vtkAMRBox is defined on each processor whereas the vtkUniformGrid is
  // defined only on the processor that owns it.
  void SetDataSet(unsigned int level, unsigned int id, 
                  vtkAMRBox& box, vtkUniformGrid* dataSet);

  // Description:
  // Get a dataset given a level and an id. In case of parallel computation,
  // the dataset can be a null pointer whereas the vtkAMRBox is always defined.
  vtkUniformGrid* GetDataSet(unsigned int level,
                             unsigned int id,
                             vtkAMRBox& box);
  vtkUniformGrid* GetDataSet(unsigned int level,unsigned int id );

  // Description:
  // Returns the AMR box for the location pointer by the iterator.
  vtkAMRBox GetAMRBox(vtkCompositeDataIterator* iter);

//ETX

// Description:
// Get meta-data associated with a level. This may allocate a new
// vtkInformation object if none is already present. Use HasLevelMetaData to
// avoid unnecessary allocations.
  vtkInformation* GetLevelMetaData(unsigned int level)
    { return this->GetChildMetaData(level); }

  // Description:
  // Returns if meta-data exists for a given level.
  int HasLevelMetaData(unsigned int level)
    { return this->HasChildMetaData(level); }

  // Description:
  // Sets the composite index of the data at the given (level,index) pair.
  void SetCompositeIndex(
      const unsigned int level, const unsigned int index, const int idx );

  // Description:
  // Retrieves the composite index  associated with the data at the given
  // (level,index) pair.
  int GetCompositeIndex( const unsigned int level, const unsigned int index );

  // Description:
  // Get meta-data associated with a dataset.  This may allocate a new
  // vtkInformation object if none is already present. Use HasMetaData to
  // avoid unnecessary allocations.
  vtkInformation* GetMetaData(unsigned int level, unsigned int index);

  // Description:
  // Get the AMR box meta-data associated with a given dataset.
  // Returns 1 iff GetMetaData() was successful, else 0.
  int GetMetaData(unsigned int level, unsigned int index, vtkAMRBox &box);

  // Description:
  // Returns if meta-data exists for a given dataset under a given level.
  int HasMetaData(unsigned int level, unsigned int index);

  // Description:
  // Sets the refinement of a given level. The spacing at level
  // level+1 is defined as spacing(level+1) = spacing(level)/refRatio(level).
  // Note that currently, this is not enforced by this class however
  // some algorithms might not function properly if the spacing in
  // the blocks (vtkUniformGrid) does not match the one described
  // by the refinement ratio.
  void SetRefinementRatio(unsigned int level, int refRatio);

  // Description:
  // Returns the refinement of a given level.
  int GetRefinementRatio(unsigned int level);

  // Description:
  // Returns the refinement ratio for the position pointed by the iterator.
  int GetRefinementRatio(vtkCompositeDataIterator* iter);

  // Description:
  // Blank lower level cells if they are overlapped by higher
  // level ones.
  void GenerateVisibilityArrays();

  static vtkInformationIntegerVectorKey* BOX();
  static vtkInformationIntegerKey* BOX_DIMENSIONALITY();
  static vtkInformationIntegerKey* REFINEMENT_RATIO();
  static vtkInformationIdTypeKey* NUMBER_OF_BLANKED_POINTS();
  static vtkInformationDoubleVectorKey* BOX_ORIGIN();
  static vtkInformationDoubleVectorKey* SPACING();
  static vtkInformationIntegerKey* RANK();
  static vtkInformationIntegerKey* BLOCK_ID();
  static vtkInformationIntegerVectorKey* REAL_EXTENT();
  static vtkInformationIntegerKey* GEOMETRIC_DESCRIPTION();

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkHierarchicalBoxDataSet* GetData(vtkInformation* info);
  static vtkHierarchicalBoxDataSet* GetData(vtkInformationVector* v, int i=0);
  //ETX

  // Description:
  // Copy the cached scalar range into range.
  virtual void GetScalarRange(double range[]);
  
  // Description:
  // Return the cached range.
  virtual double *GetScalarRange();

  // Description:
  // Unhiding superclass method.
  virtual vtkDataObject* GetDataSet(vtkCompositeDataIterator* iter)
    { return this->Superclass::GetDataSet(iter); }

  // Description:
  // Unhiding superclass method.
  virtual vtkInformation* GetMetaData(vtkCompositeDataIterator* iter)
    { return this->Superclass::GetMetaData(iter); }


  // Description:
  // Unhiding superclass method.
  virtual int HasMetaData(vtkCompositeDataIterator* iter)
    { return this->Superclass::HasMetaData(iter); }
 
  // Description:
  // Given the level and dataset index, returns the flat index in pre-order
  // traversal.
  unsigned int GetFlatIndex(unsigned int level, unsigned int index);

  // Description:
  // Given the composite Idx (as set by SetCompositeIdx) this method returns the
  // corresponding level and dataset index within the level.
  void GetLevelAndIndex(
      const unsigned int compositeIdx, unsigned int &level, unsigned int &idx );

  // Description:
  // Removes all AMR data stored in this instance of the vtkHierarchicalBoxDataSet
  void Clear();

  // Description:
  // Returns the total number of blocks
  int GetTotalNumberOfBlocks();

  // Description:
  // In-line Set & Get
  vtkSetMacro( PadCellVisibility, bool );
  vtkGetMacro( PadCellVisibility, bool );

protected:
  vtkHierarchicalBoxDataSet();
  ~vtkHierarchicalBoxDataSet();

  // Description:
  // Gets the list of higher res boxes from this level at the level, l+1
  void GetHigherResolutionCoarsenedBoxes(
      vtkAMRBoxList &blist, const unsigned int l );

  // Description:
  // Blanks the grids at level, l, Given the list of high-res boxes at level
  // l+1 coarsened to level l.
  void BlankGridsAtLevel( vtkAMRBoxList &blist, const unsigned int l );

  // Description:
  // Compute the range of the scalars and cache it into ScalarRange
  // only if the cache became invalid (ScalarRangeComputeTime).
  virtual void ComputeScalarRange();
  
  // Cached scalar range
  double ScalarRange[2];
  // Time at which scalar range is computed
  vtkTimeStamp ScalarRangeComputeTime;

  bool PadCellVisibility;

  // Global Origin
  double origin[3];

  // Mapping of composite indices to the (level,id) pair.
  vtkstd::map< int, vtkstd::pair<unsigned int,unsigned int> >
    CompositeIndex2LevelIdPair;

private:



  vtkHierarchicalBoxDataSet(const vtkHierarchicalBoxDataSet&);  // Not implemented.
  void operator=(const vtkHierarchicalBoxDataSet&);  // Not implemented.
};

#endif

