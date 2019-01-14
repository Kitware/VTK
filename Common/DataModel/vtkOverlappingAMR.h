/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOverlappingAMR.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOverlappingAMR
 * @brief   hierarchical dataset of vtkUniformGrids
 *
 *
 * vtkOverlappingAMR extends vtkUniformGridAMR by exposing access to the
 * amr meta data, which stores all structural information represented
 * by an vtkAMRInformation object
 *
 * @sa
 * vtkAMRInformation
*/

#ifndef vtkOverlappingAMR_h
#define vtkOverlappingAMR_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkUniformGridAMR.h"

class vtkAMRBox;
class vtkCompositeDataIterator;
class vtkUniformGrid;
class vtkAMRInformation;
class vtkInformationIdTypeKey;

class VTKCOMMONDATAMODEL_EXPORT vtkOverlappingAMR: public vtkUniformGridAMR
{
public:
  static vtkOverlappingAMR *New();

  /**
   * Return class name of data type (see vtkType.h for definitions).
   */
  int GetDataObjectType() override {return VTK_OVERLAPPING_AMR;}

  vtkTypeMacro(vtkOverlappingAMR,vtkUniformGridAMR);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a new iterator (the iterator has to be deleted by the user).
   */
  VTK_NEWINSTANCE vtkCompositeDataIterator* NewIterator() override;

  //@{
  /**
   * Get/Set the global origin of the amr data set
   */
  void SetOrigin(const double*);
  double* GetOrigin();
  //@}

  //@{
  /**
   * Get/Set the grid spacing at a given level
   */
  void SetSpacing(unsigned int level, const double spacing[3]);
  void GetSpacing(unsigned int level, double spacing[3]);
  //@}

  //@{
  /**
   * Set/Get the AMRBox for a given block
   */
  void SetAMRBox(unsigned int level, unsigned int id, const vtkAMRBox& box) ;
  const vtkAMRBox& GetAMRBox(unsigned int level, unsigned int id) ;
  //@}

  /**
   * Returns the bounding information of a data set.
   */
  void GetBounds(unsigned int level, unsigned int id, double* bb);


  /**
   * Returns the origin of an AMR block
   */
  void GetOrigin(unsigned int level, unsigned int id, double origin[3]);

  static vtkInformationIdTypeKey* NUMBER_OF_BLANKED_POINTS();

  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkOverlappingAMR* GetData(vtkInformation* info)
    { return vtkOverlappingAMR::SafeDownCast(Superclass::GetData(info)); }
  static vtkOverlappingAMR* GetData(vtkInformationVector* v, int i=0)
    { return vtkOverlappingAMR::SafeDownCast(Superclass::GetData(v, i)); }

  /**
   * Sets the refinement of a given level. The spacing at level
   * level+1 is defined as spacing(level+1) = spacing(level)/refRatio(level).
   * Note that currently, this is not enforced by this class however
   * some algorithms might not function properly if the spacing in
   * the blocks (vtkUniformGrid) does not match the one described
   * by the refinement ratio.
   */
  void SetRefinementRatio(unsigned int level, int refRatio);

  /**
   * Returns the refinement of a given level.
   */
  int GetRefinementRatio(unsigned int level);

  //@{
  /**
   * Set/Get the source id of a block. The source id is produced by an
   * AMR source, e.g. a file reader might set this to be a file block id
   */
  void SetAMRBlockSourceIndex(unsigned int level, unsigned int id, int sourceId);
  int GetAMRBlockSourceIndex(unsigned int level, unsigned int id);
  //@}

  /**
   * Returns the refinement ratio for the position pointed by the iterator.
   */
  int GetRefinementRatio(vtkCompositeDataIterator* iter);

  /**
   * Return whether parent child information has been generated
   */
  bool HasChildrenInformation();

  /**
   * Generate the parent/child relationships - needed to be called
   * before GetParents or GetChildren can be used!
   */
  void GenerateParentChildInformation();

  /**
   * Return a pointer to Parents of a block.  The first entry is the number
   * of parents the block has followed by its parent ids in level-1.
   * If none exits it returns nullptr.
   */
  unsigned int *GetParents(unsigned int level, unsigned int index,  unsigned int& numParents);

  /**
   * Return a pointer to Children of a block.  The first entry is the number
   * of children the block has followed by its children ids in level+1.
   * If none exits it returns nullptr.
   */
  unsigned int *GetChildren(unsigned int level, unsigned int index, unsigned int& numChildren);

  /**
   * Prints the parents and children of a requested block (Debug Routine)
   */
  void PrintParentChildInfo(unsigned int level, unsigned int index);

  //Unhide superclass method
  void GetBounds(double b[6]) { Superclass::GetBounds(b);}

  /**
   * Given a point q, find the highest level grid that contains it.
   */
  bool FindGrid(double q[3], unsigned int& level, unsigned int& gridId);

  /**
   * Get/Set the internal representation of amr meta meta data
   */
  vtkAMRInformation* GetAMRInfo() override
    { return Superclass::GetAMRInfo();}
  void SetAMRInfo(vtkAMRInformation* info) override
    { return Superclass::SetAMRInfo(info);}

  //@{
  /**
   * Check whether the data set is internally consistent, e.g.
   * whether the meta data and actual data blocks match.
   * Incorrectness will be reported as error messages
   */
  void Audit();
 protected:
  vtkOverlappingAMR();
  ~vtkOverlappingAMR() override;
  //@}

private:
  vtkOverlappingAMR(const vtkOverlappingAMR&) = delete;
  void operator=(const vtkOverlappingAMR&) = delete;
};

#endif
