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
// .SECTION Description
// vtkHierarchicalBoxDataSet is a concrete implementation of  
// vtkHierarchicalDataSet. The dataset type is restricted to
// vtkUniformGrid. Each dataset has an associated vtkAMRBox
// that represents it's region (similar to extent) in space.

#ifndef __vtkHierarchicalBoxDataSet_h
#define __vtkHierarchicalBoxDataSet_h

#include "vtkHierarchicalDataSet.h"

//BTX
struct vtkHierarchicalBoxDataSetInternal;
//ETX
class vtkDataObject;
class vtkUniformGrid;
class vtkAMRBox;

class VTK_COMMON_EXPORT vtkHierarchicalBoxDataSet : public vtkHierarchicalDataSet
{
public:
  static vtkHierarchicalBoxDataSet *New();

  vtkTypeRevisionMacro(vtkHierarchicalBoxDataSet,vtkHierarchicalDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This returns a vtkHierarchicalBoxVisitor.
  virtual vtkCompositeDataVisitor* NewVisitor();

  // Description:
  // Return class name of data type (see vtkSystemIncludes.h for
  // definitions).
  virtual int GetDataObjectType() {return VTK_HIERARCHICAL_BOX_DATA_SET;}

//BTX
  // Description:
  // Set the dataset pointer for a given node. This method does
  // not remove the existing parent/child links. It only replaces
  // the dataset pointer.
  void SetDataSet(unsigned int level, 
                  unsigned int id, 
                  vtkAMRBox& box,
                  vtkUniformGrid* dataSet);

  // Description:
  // Get a dataset give a level and an id.
  vtkUniformGrid* GetDataSet(unsigned int level, 
                             unsigned int id, 
                             vtkAMRBox& box);
//ETX
  vtkDataObject* GetDataSet(unsigned int level, unsigned int id)
    { return this->Superclass::GetDataSet(level, id); }

  // Description:
  // Sets the refinement of a given level.
  void SetRefinementRatio(unsigned int level, int refRatio);

  // Description:
  // Blank lower level cells if they are overlapped by higher
  // level ones.
  void GenerateVisibilityArrays();

  // Description:
  // Shallow and Deep copy.
  virtual void ShallowCopy(vtkDataObject *src);  
  virtual void DeepCopy(vtkDataObject *src);

protected:
  vtkHierarchicalBoxDataSet();
  ~vtkHierarchicalBoxDataSet();

  virtual vtkHDSNode* NewNode();

  vtkHierarchicalBoxDataSetInternal* BoxInternal;

private:
  vtkHierarchicalBoxDataSet(const vtkHierarchicalBoxDataSet&);  // Not implemented.
  void operator=(const vtkHierarchicalBoxDataSet&);  // Not implemented.
};

#endif

