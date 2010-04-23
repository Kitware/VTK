/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeDataSet - abstract superclass for composite 
// (multi-block or AMR) datasets
// .SECTION Description
// vtkCompositeDataSet is an abstract class that represents a collection
// of datasets (including other composite datasets). It
// provides an interface to access the datasets through iterators.
// vtkCompositeDataSet provides methods that are used by subclasses to store the
// datasets.
// vtkCompositeDataSet provides the datastructure for a full tree 
// representation. Subclasses provide the semantics for it and control how
// this tree is built.

// .SECTION See Also
// vtkCompositeDataIterator

#ifndef __vtkCompositeDataSet_h
#define __vtkCompositeDataSet_h

#include "vtkDataObject.h"

class vtkCompositeDataIterator;
class vtkCompositeDataSetInternals;
class vtkInformation;
class vtkInformationStringKey;

class VTK_FILTERING_EXPORT vtkCompositeDataSet : public vtkDataObject
{
public:
  vtkTypeMacro(vtkCompositeDataSet, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a new iterator (the iterator has to be deleted by user).
  virtual vtkCompositeDataIterator* NewIterator();

  // Description:
  // Return class name of data type (see vtkType.h for
  // definitions).
  virtual int GetDataObjectType() {return VTK_COMPOSITE_DATA_SET;}

  // Description:
  // Get the port currently producing this object.
  virtual vtkAlgorithmOutput* GetProducerPort();

  // Description:
  // Copies the tree structure from the input. All pointers to non-composite
  // data objects are intialized to NULL. This also shallow copies the meta data
  // associated with all the nodes.
  virtual void CopyStructure(vtkCompositeDataSet* input);

  // Description:
  // Sets the data set at the location pointed by the iterator.
  // The iterator does not need to be iterating over this dataset itself. It can
  // be any composite datasite with similar structure (achieved by using
  // CopyStructure).
  virtual void SetDataSet(vtkCompositeDataIterator* iter, vtkDataObject* dataObj);

  // Description:
  // Returns the dataset located at the positiong pointed by the iterator.
  // The iterator does not need to be iterating over this dataset itself. It can
  // be an iterator for composite dataset with similar structure (achieved by
  // using CopyStructure).
  virtual vtkDataObject* GetDataSet(vtkCompositeDataIterator* iter);

  // Description:
  // Returns the meta-data associated with the position pointed by the iterator.
  // This will create a new vtkInformation object if none already exists. Use
  // HasMetaData to avoid creating the vtkInformation object unnecessarily.
  // The iterator does not need to be iterating over this dataset itself. It can
  // be an iterator for composite dataset with similar structure (achieved by
  // using CopyStructure).
  virtual vtkInformation* GetMetaData(vtkCompositeDataIterator* iter);

  // Description:
  // Returns if any meta-data associated with the position pointed by the iterator.
  // The iterator does not need to be iterating over this dataset itself. It can
  // be an iterator for composite dataset with similar structure (achieved by
  // using CopyStructure).
  virtual int HasMetaData(vtkCompositeDataIterator* iter);

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkCompositeDataSet* GetData(vtkInformation* info);
  static vtkCompositeDataSet* GetData(vtkInformationVector* v, int i=0);
  //ETX
  
  // Description:
  // Restore data object to initial state,
  virtual void Initialize();
  
  // Description:
  // Shallow and Deep copy.
  virtual void ShallowCopy(vtkDataObject *src);
  virtual void DeepCopy(vtkDataObject *src);

  // Description:
  // Returns the total number of points of all blocks. This will
  // iterate over all blocks and call GetNumberOfPoints() so it
  // might be expansive.
  virtual vtkIdType GetNumberOfPoints();
  
  // Description:
  // Key used to put node name in the meta-data associated with a node.
  static vtkInformationStringKey* NAME();

//BTX
protected:
  vtkCompositeDataSet();
  ~vtkCompositeDataSet();

  // Description:
  // Set the number of children.
  void SetNumberOfChildren(unsigned int num);

  // Description:
  // Get the number of children.
  unsigned int GetNumberOfChildren();

  // Description:
  // Set child dataset at a given index. The number of children is adjusted to
  // to be greater than the index specified. 
  void SetChild(unsigned int index, vtkDataObject*);

  // Description:
  // Remove the child at a given index. 
  void RemoveChild(unsigned int index);

  // Description:
  // Returns a child dataset at a given index.
  vtkDataObject* GetChild(unsigned int num);

  // Description:
  // Returns the meta-data at a given index. If the index is valid, however, no
  // information object is set, then a new one will created and returned.
  // To avoid unnecessary creation, use HasMetaData().
  vtkInformation* GetChildMetaData(unsigned int index);

  // Description:
  // Sets the meta-data at a given index. 
  void SetChildMetaData(unsigned int index, vtkInformation* info);

  // Description:
  // Returns if meta-data information is available for the given child index.
  // Returns 1 is present, 0 otherwise.
  int HasChildMetaData(unsigned int index);

  // The internal datastructure. Subclasses need not access this directly.
  vtkCompositeDataSetInternals* Internals;

  friend class vtkCompositeDataIterator;
private:
  vtkCompositeDataSet(const vtkCompositeDataSet&); // Not implemented.
  void operator=(const vtkCompositeDataSet&); // Not implemented.
//ETX
};

#endif


