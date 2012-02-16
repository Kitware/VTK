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
// NOTE vtkAMRBox is used to compute cell visibility, therefore it
// should be dimensioned according to the visible region.


#ifndef __vtkOverlappingAmr_h
#define __vtkOverlappingAmr_h

#include "vtkUniformGridAMR.h"
#include <vector>    // For STL vector
#include <map>       // For STL map
#include <utility>   // For STL pair

class vtkAMRBox;
class vtkDataObject;
class vtkCompositeDataIterator;
class vtkInformationIdTypeKey;
class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;
class vtkUniformGrid;
class vtkUnsignedIntArray;

typedef std::vector<vtkAMRBox> vtkAMRBoxList;

class VTK_FILTERING_EXPORT vtkOverlappingAMR: public vtkUniformGridAMR
{
public:
  static vtkOverlappingAMR *New();
  vtkTypeMacro(vtkOverlappingAMR,vtkUniformGridAMR);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Descrition:
  // Set & Get the AMR dataset origin
  // The origin is essentially the minimum of all the grids.
  void SetOrigin( const double origin[3] );
  void GetOrigin( double origin[3] );

  // Description:
  // Return class name of data type (see vtkType.h for definitions).
  virtual int GetDataObjectType() {return VTK_OVERLAPPING_AMR;}

  // Description:
  // This method returns the root AMR box for the entire root level.
  // The root AMR box covers the entire domain.
  bool GetRootAMRBox( vtkAMRBox &root );

  // Description:
  // This method returns the global AMR box, covering the entire
  // domain, with the prescribed spacing.
  void GetGlobalAMRBoxWithSpacing( vtkAMRBox &box, double h[3] );

  // SetDataSet methods

  // Description:
  // Unhiding superclass method
  virtual void SetDataSet(
      vtkCompositeDataIterator* iter, vtkDataObject* dataObj)
     { this->Superclass::SetDataSet(iter, dataObj); };

  // Description:
  // Unhiding superclass method
  virtual void SetDataSet(
          unsigned int level,
          unsigned int idx,
          vtkUniformGrid *grid)
    {this->Superclass::SetDataSet(level,idx,grid); }

  // Description:
  // Set the dataset pointer for a given node. This will resize the number of
  // levels and the number of datasets in the level to fit level, id requested. 
  virtual void SetDataSet(
                  unsigned int level,
                  unsigned int id,
                  int LoCorner[3],
                  int HiCorner[3],
                  vtkUniformGrid* dataSet);

  // Description:
  // Set the dataset pointer for a given node. This will resize the number of
  // levels and the number of datasets in the level to fit level, id requested.
  // The information carried by the vtkAMRBox is redundant with the extent
  // of the vtkUniformGrid. However, in case of parallel computation, the
  // vtkAMRBox is defined on each processor whereas the vtkUniformGrid is
  // defined only on the processor that owns it.
  virtual void SetDataSet(
                   unsigned int level,
                   unsigned int id,
                   vtkAMRBox& box,
                   vtkUniformGrid* dataSet);

  // Description:
  // Sets the meta-data object at a given node. This will resize the number
  // of levels and number of datasets acoordingly.
  virtual void SetMetaData( unsigned int level,
                            unsigned int id,
                            const vtkAMRBox &box );


  // GetDataSet methods

  // Description:
  // Unhiding superclass method.
  virtual vtkUniformGrid* GetDataSet(unsigned int level, unsigned int idx)
    { return(this->Superclass::GetDataSet(level,idx)); }

  // Description:
  // Get a dataset given a level and an id. In case of parallel computation,
  // the dataset can be a null pointer whereas the vtkAMRBox is always defined.
  virtual vtkUniformGrid* GetDataSet(
      unsigned int level, unsigned int id, vtkAMRBox& box);

  // Description:
  // Returns the AMR box for the location pointer by the iterator.
  vtkAMRBox GetAMRBox(vtkCompositeDataIterator* iter);

  // Description:
  // Sets the composite index of the data at the given (level,index) pair.
  void SetCompositeIndex(
      const unsigned int level, const unsigned int index, const int idx );

  // Description:
  // Retrieves the composite index  associated with the data at the given
  // (level,index) pair.
  int GetCompositeIndex( const unsigned int level, const unsigned int index );

  // Description:
  // Get the AMR box meta-data associated with a given dataset.
  // Returns 1 iff GetMetaData() was successful, else 0.
  virtual int GetMetaData(
      unsigned int level, unsigned int index, vtkAMRBox &box);

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

  //Description:
  // Generate the parent/child relationships - needed to be called
  // before GetParents or GetChildren can be used!
  void GenerateParentChildInformation();

  // Description:
  // Override ShallowCopy/DeepCopy and CopyStructure
  virtual void ShallowCopy(vtkDataObject *src);
  virtual void DeepCopy(vtkDataObject *src);
  virtual void CopyStructure(vtkCompositeDataSet *src);

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
  static vtkOverlappingAMR* GetData(vtkInformation* info);
  static vtkOverlappingAMR* GetData(vtkInformationVector* v, int i=0);
  //ETX

  // Description:
  // Unhiding superclass method.
  virtual vtkDataObject* GetDataSet(vtkCompositeDataIterator* iter)
    { return this->Superclass::GetDataSet(iter); }

  // Description:
  // Unhiding superclass method.
  virtual vtkInformation* GetMetaData(vtkCompositeDataIterator* iter)
    {return this->Superclass::GetMetaData(iter); }


  // Description:
  // Unhiding superclass method.
  virtual int HasMetaData(vtkCompositeDataIterator* iter)
    {return this->Superclass::HasMetaData(iter); }

  // Description:
  // Unhiding superclass method.
  virtual int HasMetaData(unsigned int level, unsigned int index)
    {return this->Superclass::HasMetaData(level,index); }
 
  // Desription:
  // Unhiding superclass method.
  virtual vtkInformation* GetMetaData(unsigned int level, unsigned int index)
   {return this->Superclass::GetMetaData(level,index); }

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
  // Removes all AMR data stored in this instance of the vtkOverlappingAMR
  void Clear();

  // Description:
  // In-line Set & Get
  vtkSetMacro( PadCellVisibility, bool );
  vtkGetMacro( PadCellVisibility, bool );

  // Description:
  // Return a pointer to Parents of a block.  The first entry is the number
  // of parents the block has followed by its parent ids in level-1.
  // If none exits it returns NULL.
  unsigned int *GetParents(unsigned int level, unsigned int index);

  // Description:
  // Return a pointer to Children of a block.  The first entry is the number
  // of children the block has followed by its childern ids in level+1.
  // If none exits it returns NULL.
  unsigned int *GetChildren(unsigned int level, unsigned int index);

  // Description:
  // Prints the parents and children of a requested block (Debug Routine)
  void PrintParentChildInfo(unsigned int level, unsigned int index);
protected:
  vtkOverlappingAMR();
  virtual ~vtkOverlappingAMR();

  // Description:
  // Gets the list of higher res boxes from this level at the level, l+1
  void GetHigherResolutionCoarsenedBoxes(
      vtkAMRBoxList &blist, const unsigned int l );

  // Description:
  // Gets the list of boxes for this level
  void GetBoxesFromLevel(const unsigned int l, vtkAMRBoxList &blist);

  // Description:
  // Blanks the grids at level, l, Given the list of high-res boxes at level
  // l+1 coarsened to level l.
  void BlankGridsAtLevel( vtkAMRBoxList &blist, const unsigned int l );

  // Description:
  // Generate the Children Information for level l and the Parent Information
  // for level l+1 - Note that lboxes will be converted to the more refined
  // level and nlboxes will contain the boxes of level l+1
  void GenerateParentChildLevelInformation(const unsigned int levelIdx,
                                           vtkAMRBoxList &lboxes,
                                           vtkAMRBoxList &nlboxes);

  // Description:
  // Assign an array from the src
  static void AssignUnsignedIntArray(
      vtkUnsignedIntArray **dest, vtkUnsignedIntArray *src);


  // Description:
  // See vtkUniformGridAMR::ComputeBounds
  virtual void ComputeBounds();

  bool PadCellVisibility;

  // Global Origin
  double Origin[3];

  // Mapping of composite indices to the (level,id) pair.
  std::map< int, std::pair<unsigned int,unsigned int> >
    CompositeIndex2LevelIdPair;

  // Arrays needed to get the Parents of a block - the first holds
  // the number of parents for each block and the parent block ids w/r
  // to the courser level.  The second array indicates where the parent
  // information of each block begins in the Parentinformation array
  // NOTE: That all the blocks in level 0 point to the first entry in 
  // the parent information array (whose value is 0)
  vtkUnsignedIntArray *ParentInformation;
  vtkUnsignedIntArray *ParentInformationMap;

  // Arrays needed to get the Children of a block - the first holds
  // the number of children for each block and the child block ids w/r
  // to the refined level.  The second array indicates where the children
  // information of each block begins in the Childreninformation array
  // NOTE: That all the blocks in most refined level don't have entries since
  // this would be a lot of zeros!
  vtkUnsignedIntArray *ChildrenInformation;
  vtkUnsignedIntArray *ChildrenInformationMap;

  // Array needed to indicate where each level begins in the information map arrays
  vtkUnsignedIntArray *LevelMap;
private:
  vtkOverlappingAMR(const vtkOverlappingAMR&);  // Not implemented.
  void operator=(const vtkOverlappingAMR&);  // Not implemented.
};

#endif
