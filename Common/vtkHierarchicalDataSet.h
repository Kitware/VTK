/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataSet.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalDataSet - abstact superclass for hierarchical datasets
// .SECTION Description
// vtkHierarchicalDataSet is a vtkCompositeDataSet that stores
// a hieararchy of datasets. The dataset collection consists of
// multiple levels. Each dataset can have an arbitrary number of
// parents and children at levels above and below. Currently,
// the interface for connecting parents-children is incomplete.

#ifndef __vtkHierarchicalDataSet_h
#define __vtkHierarchicalDataSet_h

#include "vtkCompositeDataSet.h"

//BTX
struct vtkHierarchicalDataSetInternal;
//ETX
class vtkDataObject;
class vtkHDSNode;

class VTK_COMMON_EXPORT vtkHierarchicalDataSet : public vtkCompositeDataSet
{
public:
  vtkTypeRevisionMacro(vtkHierarchicalDataSet,vtkCompositeDataSet);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a new (forward) iterator 
  // (the iterator has to be deleted by user)
  virtual vtkCompositeDataIterator* NewIterator();

  // Description:
  // Return class name of data type (see vtkSystemIncludes.h for
  // definitions).
  virtual int GetDataObjectType() {return VTK_HIERARCHICAL_DATA_SET;}

  // Description:
  // Restore data object to initial state,
  virtual void Initialize();

  // Description:
  // Set the number of refinement levels. This call might cause
  // allocation if the new number of levels is larger than the
  // current one.
  void SetNumberOfLevels(unsigned int numLevels);

  // Description:
  // Returns the number of levels.
  unsigned int GetNumberOfLevels();

  // Description:
  // Set the number of datasets in a given level. This call might
  // cause allocation if the new number of datasets is larger
  // than the current one.
  void SetNumberOfDataSets(unsigned int level, unsigned int numDataSets);

  // Description:
  // Returns the number of datasets in a given level.
  unsigned int GetNumberOfDataSets(unsigned int level);

  // Description:
  // Initialize the entry for a dataset node. This removes all
  // parent/child links between the given node and others.
  void InitializeNode(unsigned int level, unsigned int id);

  // Description:
  // Returns 1 if the node [level, id] is initialized. Since
  // GetDataSet() returns NULL when either an existing node
  // has NULL dataset pointer or the node does not exit, this
  // is the only way to check if a node really exists.
  int IsNodePresent(unsigned int level, unsigned int id);

  // Description:
  // Set the dataset pointer for a given node. This method does
  // not remove the existing parent/child links. It only replaces
  // the dataset pointer.
  void SetDataSet(unsigned int level, unsigned int id, vtkDataObject* dataSet);

  // Description:
  // Get a dataset give a level and an id.
  vtkDataObject* GetDataSet(unsigned int level, unsigned int id);

  // Description:
  // Shallow and Deep copy.
  virtual void ShallowCopy(vtkDataObject *src);  
  virtual void DeepCopy(vtkDataObject *src);

//BTX
  friend class vtkHierarchicalDataIterator;
//ETX

protected:
  vtkHierarchicalDataSet();
  ~vtkHierarchicalDataSet();

  vtkHierarchicalDataSetInternal* Internal;

  void InitializeDataSets();

  virtual vtkHDSNode* NewNode();

private:
  vtkHierarchicalDataSet(const vtkHierarchicalDataSet&);  // Not implemented.
  void operator=(const vtkHierarchicalDataSet&);  // Not implemented.
};

#endif

