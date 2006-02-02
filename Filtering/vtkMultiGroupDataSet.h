/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupDataSet -  Composite dataset that organize datasets in groups
// .SECTION Description
// vtkMultiGroupDataSet is a vtkCompositeDataSet that stores
// a hierarchy of datasets. The dataset collection consists of
// multiple groups. NULL pointers are valid placeholders for datasets.
// Each group can contain zero or more datasets.
// When a multi-group dataset is distributed across processors, no
// two processor should have the dataset. For example, if a data
// has
// @verbatim
// Group 0:
//   * ds 0
//   * ds 1
// @endverbatim
// it cannot be distributed in the following way:
// @verbatim
// proc 0:
// Group 0:
//   * ds 0
//   * ds 1
//
// proc 1:
// Group 0:
//   * ds 0
//   * ds 1
// @endverbatim
// but can be distributed in the following way:
// @verbatim
// proc 0:
// Group 0:
//   * ds 0
//   * (null)
//
// proc 1:
// Group 0:
//   * (null)
//   * ds 1
// @endverbatim


#ifndef __vtkMultiGroupDataSet_h
#define __vtkMultiGroupDataSet_h

#include "vtkCompositeDataSet.h"

//BTX
struct vtkMultiGroupDataSetInternal;
//ETX
class vtkDataObject;
class vtkMGDSNode;
class vtkMultiGroupDataInformation;

class VTK_FILTERING_EXPORT vtkMultiGroupDataSet : public vtkCompositeDataSet
{
public:
  static vtkMultiGroupDataSet *New();

  vtkTypeRevisionMacro(vtkMultiGroupDataSet,vtkCompositeDataSet);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a new (forward) iterator 
  // (the iterator has to be deleted by user)
  virtual vtkCompositeDataIterator* NewIterator();

  // Description:
  // Return class name of data type (see vtkType.h for
  // definitions).
  virtual int GetDataObjectType() {return VTK_MULTIGROUP_DATA_SET;}

  // Description:
  // Restore data object to initial state,
  virtual void Initialize();

  // Description:
  // Set the number of groups. This call might cause
  // allocation if the new number of groups is larger than the
  // current one.
  void SetNumberOfGroups(unsigned int numGroups);

  // Description:
  // Returns the number of groups.
  unsigned int GetNumberOfGroups();

  // Description:
  // Set the number of datasets in a given group. This call might
  // cause allocation if the new number of datasets is larger
  // than the current one.
  void SetNumberOfDataSets(unsigned int group, unsigned int numDataSets);

  // Description:
  // Returns the number of datasets in a given group.
  unsigned int GetNumberOfDataSets(unsigned int group);

  // Description:
  // Initialize the entry for a dataset node. This removes all
  // parent/child links between the given node and others.
  void InitializeNode(unsigned int group, unsigned int id);

  // Description:
  // Set the dataset pointer for a given group id and position. 
  // NULL pointer is an accepted assignment and will replace
  // the dataset. Use NULL pointer to mark a dataset as existant, possibly
  // on another processor. Metadata can still be associated with
  // a NULL dataset.
  void SetDataSet(unsigned int group, unsigned int id, vtkDataObject* dataSet);

  // Description:
  // Uses keys GROUP() and INDEX() to call SetDataSet(GROUP, INDEX, dobj)
  virtual void AddDataSet(vtkInformation* index, vtkDataObject* dobj);

  // Description:
  // Get a dataset give a group and an id.
  vtkDataObject* GetDataSet(unsigned int group, unsigned int id);

  // Description:
  // Uses keys GROUP() and INDEX() to call GetDataSet(GROUP, INDEX)
  virtual vtkDataObject* GetDataSet(vtkInformation* index);

  // Description:
  // Shallow and Deep copy.
  virtual void ShallowCopy(vtkDataObject *src);  
  virtual void DeepCopy(vtkDataObject *src);

  // Description:
  // Returns the data structure containing information about
  // the datasets. This is an information object containing
  // the meta-data associated with this dataset. This may include
  // things like datatypes, extents...
  vtkGetObjectMacro(MultiGroupDataInformation,vtkMultiGroupDataInformation);

  // Description:
  // Set the information about the datasets. 
  // This is an information object containing the meta-data associated with
  // this dataset. This may include things like datatypes, extents...
  void SetMultiGroupDataInformation(vtkMultiGroupDataInformation* info);

  // Description:
  // Returns the total number of points of all blocks. This will
  // iterate over all blocks and call GetNumberOfPoints() so it
  // might be expansive.
  virtual vtkIdType GetNumberOfPoints();

//BTX
  friend class vtkMultiGroupDataIterator;
//ETX

  static vtkInformationIntegerKey* GROUP();

protected:
  vtkMultiGroupDataSet();
  ~vtkMultiGroupDataSet();

  vtkMultiGroupDataSetInternal* Internal;

  void InitializeDataSets();

  virtual vtkMGDSNode* NewNode();

  vtkMultiGroupDataInformation* MultiGroupDataInformation;

private:
  vtkMultiGroupDataSet(const vtkMultiGroupDataSet&);  // Not implemented.
  void operator=(const vtkMultiGroupDataSet&);  // Not implemented.
};

#endif

