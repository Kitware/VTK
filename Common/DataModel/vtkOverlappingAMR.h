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
// .NAME vtkOverlappingAMR - hierarchical dataset of vtkUniformGrids
//
// .SECTION Description
// vtkOverlappingAMR extends vtkUniformGridAMR by exposing access to the
// meta data
// The class maybe used as a pure meta data object when no data set is
// inserted.
//
// .SECTION See Also
// vtkAMRInformation

#ifndef __vtkOverlappingAmr_h
#define __vtkOverlappingAmr_h

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

  // Description:
  // Return class name of data type (see vtkType.h for definitions).
  virtual int GetDataObjectType() {return VTK_OVERLAPPING_AMR;}

  vtkTypeMacro(vtkOverlappingAMR,vtkUniformGridAMR);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a new iterator (the iterator has to be deleted by the user).
  virtual vtkCompositeDataIterator* NewIterator();

  //Description:
  //Initialize the overlapping AMR, the origin is global to the data set
  //Every data set must be of type gridDescription
  void Initialize(int numLevels, int * blocksPerLevel,  double origin[3],  int gridDescription);

  // Description:
  // Get/Set the meta AMR meta data
  vtkAMRInformation* GetAMRInfo(){ return Superclass::GetAMRInfo();}
  virtual void SetAMRInfo(vtkAMRInformation* info){ return Superclass::SetAMRInfo(info);}

  // Description:
  // Sets the AMRBox for a given block
  void SetAMRBox(unsigned int level, unsigned int id, double* min, int* dimensions, double* h);
  void SetAMRBox(unsigned int level, unsigned int id, const int* extents);

  // Description
  // Get the AMRBox for a given block
  const vtkAMRBox& GetAMRBox(unsigned int level, unsigned int id) ;

  // Description
  // Returns the bounding box of a given box
  void GetBounds(unsigned int level, unsigned int id, double* bb);

  // Description
  // Get the global origin of the amr data set
  double* GetOrigin();

  // Description:
  // In-line Set & Get
  vtkSetMacro( PadCellVisibility, bool );
  vtkGetMacro( PadCellVisibility, bool );

  // Description
  // Returns the grid spacing at a given level
  void GetGridSpacing(unsigned int level, double spacing[3]);

  static vtkInformationIdTypeKey* NUMBER_OF_BLANKED_POINTS();

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkOverlappingAMR* GetData(vtkInformation* info)
    { return vtkOverlappingAMR::SafeDownCast(Superclass::GetData(info)); }
  static vtkOverlappingAMR* GetData(vtkInformationVector* v, int i=0)
    { return vtkOverlappingAMR::SafeDownCast(Superclass::GetData(v, i)); }
  //ETX

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
  //Return whether parent child information has been generated
  bool HasChildrenInformation();

  //Description:
  // Generate the parent/child relationships - needed to be called
  // before GetParents or GetChildren can be used!
  void GenerateParentChildInformation();

  // Description:
  // Return a pointer to Parents of a block.  The first entry is the number
  // of parents the block has followed by its parent ids in level-1.
  // If none exits it returns NULL.
  unsigned int *GetParents(unsigned int level, unsigned int index,  unsigned int& numParents);

  // Description:
  // Return a pointer to Children of a block.  The first entry is the number
  // of children the block has followed by its childern ids in level+1.
  // If none exits it returns NULL.
  unsigned int *GetChildren(unsigned int level, unsigned int index, unsigned int& numChildren);

  // Description:
  // Prints the parents and children of a requested block (Debug Routine)
  void PrintParentChildInfo(unsigned int level, unsigned int index);

    //Description:
  //Unhide superclass method
  void Initialize(){ Superclass::Initialize();}
  virtual void Initialize(int numLevels, int * blocksPerLevel) { Superclass::Initialize(numLevels,blocksPerLevel);} ;
  void GetBounds(double b[6]) { Superclass::GetBounds(b);}

protected:
  vtkOverlappingAMR();
  virtual ~vtkOverlappingAMR();

private:
  vtkOverlappingAMR(const vtkOverlappingAMR&);  // Not implemented.
  void operator=(const vtkOverlappingAMR&);  // Not implemented.

  bool PadCellVisibility;
};

#endif
