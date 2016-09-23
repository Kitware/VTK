/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRInformation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAMRInformation
 * @brief   Meta data that describes the structure of an AMR data set
 *
 *
 * vtkAMRInformation encaspulates the following meta information for an AMR data set
 * - a list of vtkAMRBox objects
 * - Refinement ratio between AMR levels
 * - Grid spacing for each level
 * - The file block index for each block
 * - parent child information, if requested
 *
 * @sa
 * vtkOverlappingAMR, vtkAMRBox
*/

#ifndef vtkAMRInformation_h
#define vtkAMRInformation_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkAMRBox.h" //for storing AMR Boxes
#include "vtkSmartPointer.h" //for ivars
#include <vector> //for storing AMR Boxes


typedef std::vector<vtkAMRBox> vtkAMRBoxList;

class vtkUnsignedIntArray;
class vtkIntArray;
class vtkDoubleArray;
class vtkAMRIndexIterator;

class VTKCOMMONDATAMODEL_EXPORT vtkAMRInformation : public vtkObject
{
public:
  static vtkAMRInformation* New();
  vtkTypeMacro(vtkAMRInformation, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  bool operator==(const vtkAMRInformation& other);

  /**
   * Initialize the meta information
   * numLevels is the number of levels
   * blocksPerLevel[i] is the number of blocks at level i
   */
  void Initialize(int numLevels, const int* blocksPerLevel);

  //@{
  /**
   * returns the value of vtkUniformGrid::GridDescription() of any block
   */
  vtkGetMacro( GridDescription, int );
  void SetGridDescription(int description);
  //@}

  //@{
  /**
   * Get the AMR dataset origin
   * The origin is essentially the minimum of all the grids.
   */
  void GetOrigin( double origin[3] );
  double* GetOrigin();
  void SetOrigin(const double* origin);
  //@}

  /**
   * Return the number of levels
   */
  unsigned int GetNumberOfLevels() const
  { return static_cast<unsigned int>(this->NumBlocks.size()-1);}

  /**
   * Returns the number of datasets at the given levelx
   */
  unsigned int GetNumberOfDataSets(unsigned int level) const;

  /**
   * Returns total number of datasets
   */
  unsigned int GetTotalNumberOfBlocks()
  { return this->NumBlocks.back();}

  /**
   * Returns the single index from a pair of indices
   */
  int GetIndex(unsigned int level, unsigned int id) const
  { return this->NumBlocks[level] + id;}

  /**
   * Returns the an index pair given a single index
   */
  void ComputeIndexPair(unsigned int index, unsigned int& level, unsigned int& id);

  /**
   * Returns the bounds of the entire domain
   */
  const double* GetBounds();

  /**
   * Returns the bounding box of a given box
   */
  void GetBounds(unsigned int level, unsigned int id, double* bb);

  /**
   * Returns the origin of the grid at (level,id)
   */
  bool GetOrigin(unsigned int level, unsigned int id, double* origin);

  /**
   * Return the spacing at the given fiven
   */
  void GetSpacing(unsigned int level, double spacing[3]);

  bool HasSpacing(unsigned int level);

  //@{
  /**
   * Methods to set and get the AMR box at a given position
   */
  void SetAMRBox(unsigned int level, unsigned int id, const vtkAMRBox& box);
  const vtkAMRBox& GetAMRBox(unsigned int level, unsigned int id) const;
  //@}

  /**
   * return the amr box coarsened to the previous level
   */
  bool GetCoarsenedAMRBox(unsigned int level, unsigned int id, vtkAMRBox& box) const;

  //@{
  /**
   * Get/Set the SourceIndex of a block. Typically, this is a file-type specific index
   * that can be used by a reader to load a particular file block
   */
  int GetAMRBlockSourceIndex(int index);
  void SetAMRBlockSourceIndex(int index, int sourceId);
  //@}

  /**
   * This method computes the refinement ratio at each level.
   * At each level, l, the refinement ratio r_l is computed by
   * r_l = D_{l} / D_{l+1}, where D_{l+1} and D_{l} are the grid
   * spacings at the next and current level respectively.

   * .SECTION Assumptions
   * 1) Within each level, the refinement ratios are the same for all blocks.
   * 2) The refinement ratio is uniform along each dimension of the block.
   */
  void GenerateRefinementRatio();

  /**
   * Returns Wether refinement ratio has been set (either by calling
   * GenerateRefinementRatio() or by calling SetRefinementRatio()
   */
  bool HasRefinementRatio();

  /**
   * Set the refinement ratio at a level. This method should be
   * called for all levels, if called at all.
   */
  void SetRefinementRatio(unsigned int level, int ratio);

  /**
   * Returns the refinement of a given level.
   */
  int GetRefinementRatio(unsigned int level) const;

  /**
   * Set the spacing at a given level
   */
  void SetSpacing(unsigned int level,const double* h);

  /**
   * Return whether parent child information has been generated
   */
  bool HasChildrenInformation();

  /**
   * Return a pointer to Parents of a block.  The first entry is the number
   * of parents the block has followed by its parent ids in level-1.
   * If none exits it returns NULL.
   */
  unsigned int *GetParents(unsigned int level, unsigned int index, unsigned int& numParents);

  /**
   * Return a pointer to Children of a block.  The first entry is the number
   * of children the block has followed by its childern ids in level+1.
   * If none exits it returns NULL.
   */
  unsigned int *GetChildren(unsigned int level, unsigned int index, unsigned int& numChildren);

  /**
   * Prints the parents and children of a requested block (Debug Routine)
   */
  void PrintParentChildInfo(unsigned int level, unsigned int index);

  /**
   * Generate the parent/child relationships - needed to be called
   * before GetParents or GetChildren can be used!
   */
  void GenerateParentChildInformation();

  /**
   * Checks whether the meta data is internally consistent.
   */
  bool Audit();

  /**
   * Given a point q, find whether q is bounded by the data set at
   * (level,index).  If it is, set cellIdx to the cell index and return
   * true; otherwise return false
   */
  bool FindCell(double q[3],unsigned int level, unsigned int index,int &cellIdx);

  /**
   * find the grid that contains the point q at the specified level
   */
  bool FindGrid(double q[3], int level, unsigned int& gridId);

  /**
   * Given a point q, find the highest level grid that contains it.
   */
  bool FindGrid(double q[3], unsigned int& level, unsigned int& gridId);

  /**
   * Returns internal arrays.
   */
  const std::vector<int>& GetNumBlocks() const
  { return this->NumBlocks;}

  std::vector<std::vector<unsigned int> >& GetChildrenAtLevel(unsigned int i)
  { return this->AllChildren[i];}

  void DeepCopy(vtkAMRInformation *other);

 private:
  vtkAMRInformation();
  ~vtkAMRInformation() VTK_OVERRIDE;
  vtkAMRInformation(const vtkAMRInformation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAMRInformation&) VTK_DELETE_FUNCTION;

  bool HasValidOrigin();
  bool HasValidBounds();
  void UpdateBounds(const int level, const int id);
  void AllocateBoxes(unsigned int n);
  void GenerateBlockLevel();
  void CalculateParentChildRelationShip( unsigned int level,
                                        std::vector<std::vector<unsigned int> >& children,
                                        std::vector<std::vector<unsigned int> >& parents );

  //-------------------------------------------------------------------------
  // Essential information that determines an AMR structure. Must be copied
  //-------------------------------------------------------------------------
  int GridDescription; //example: VTK_XYZ_GRID
  double Origin[3]; //the origin of the whole data set
  vtkAMRBoxList Boxes; // vtkAMRBoxes, one per data set
  std::vector<int> NumBlocks; //NumBlocks[i] stores the total number of blocks from level 0 to level i-1

  vtkSmartPointer<vtkIntArray> SourceIndex; //Typically, this maps to a file block index used by the reader
  vtkSmartPointer<vtkDoubleArray> Spacing; //The grid spacing for all levels
  double Bounds[6]; //the bounds of the entire domain

  //-------------------------------------------------------------------------
  // Auxillary information that be computed
  //-------------------------------------------------------------------------
  vtkSmartPointer<vtkIntArray> Refinement; //refinement ratio between two adjacent levels
  vtkSmartPointer<vtkUnsignedIntArray> BlockLevel; //only necessary if need to call ComputeIndexPair

  //parent child information
  std::vector<std::vector<std::vector<unsigned int> > > AllChildren;
  std::vector<std::vector<std::vector<unsigned int> > > AllParents;
};

#endif
