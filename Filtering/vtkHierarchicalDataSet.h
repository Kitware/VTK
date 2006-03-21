/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalDataSet - abstract superclass for hierarchical datasets
// .SECTION Description
// vtkHierarchicalDataSet is a vtkCompositeDataSet that stores
// a hierarchy of datasets. The dataset collection consists of
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
class vtkHierarchicalDataInformation;

class VTK_FILTERING_EXPORT vtkHierarchicalDataSet : public vtkCompositeDataSet
{
public:
  static vtkHierarchicalDataSet *New();

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
  // Set the dataset pointer for a given node. This method does
  // not remove the existing parent/child links. It only replaces
  // the dataset pointer.
  void SetDataSet(unsigned int level, unsigned int id, vtkDataObject* dataSet);

  // Description:
  // Uses keys LEVEL() and INDEX() to call SetDataSet(LEVEL, INDEX, dobj)
  virtual void AddDataSet(vtkInformation* index, vtkDataObject* dobj);

  // Description:
  // Get a dataset give a level and an id.
  vtkDataObject* GetDataSet(unsigned int level, unsigned int id);

  // Description:
  // Uses keys LEVEL() and INDEX() to call GetDataSet(LEVEL, INDEX)
  virtual vtkDataObject* GetDataSet(vtkInformation* index);

  // Description:
  // Shallow and Deep copy.
  virtual void ShallowCopy(vtkDataObject *src);  
  virtual void DeepCopy(vtkDataObject *src);

  // Description:
  // Returns the data structure containing information about
  // the datasets.
  vtkGetObjectMacro(HierarchicalDataInformation,vtkHierarchicalDataInformation);

  // Description:
  // Set the information about the datasets.
  void SetHierarchicalDataInformation(vtkHierarchicalDataInformation* info);

  // Description:
  // Returns the total number of points of all blocks. This will
  // iterate over all blocks and call GetNumberOfPoints() so it
  // might be expansive.
  virtual vtkIdType GetNumberOfPoints();

//BTX
  friend class vtkHierarchicalDataIterator;
//ETX

  static vtkInformationIntegerKey* LEVEL();

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkHierarchicalDataSet* GetData(vtkInformation* info);
  static vtkHierarchicalDataSet* GetData(vtkInformationVector* v, int i=0);
  //ETX

protected:
  vtkHierarchicalDataSet();
  ~vtkHierarchicalDataSet();

  vtkHierarchicalDataSetInternal* Internal;

  void InitializeDataSets();

  virtual vtkHDSNode* NewNode();

  vtkHierarchicalDataInformation* HierarchicalDataInformation;

private:
  vtkHierarchicalDataSet(const vtkHierarchicalDataSet&);  // Not implemented.
  void operator=(const vtkHierarchicalDataSet&);  // Not implemented.
};

#endif

