/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectTree.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataObjectTree
 * @brief   provides implementation for most abstract
 * methods in the superclass vtkCompositeDataSet
 *
 * vtkDataObjectTree is represents a collection
 * of datasets (including other composite datasets). It
 * provides an interface to access the datasets through iterators.
 * vtkDataObjectTree provides methods that are used by subclasses to store the
 * datasets.
 * vtkDataObjectTree provides the datastructure for a full tree
 * representation. Subclasses provide the semantics for it and control how
 * this tree is built.
 *
 * @sa
 * vtkDataObjectTreeIterator
*/

#ifndef vtkDataObjectTree_h
#define vtkDataObjectTree_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCompositeDataSet.h"

class vtkCompositeDataIterator;
class vtkDataObjectTreeIterator;
class vtkDataObjectTreeInternals;
class vtkInformation;
class vtkInformationStringKey;
class vtkDataObject;

class VTKCOMMONDATAMODEL_EXPORT vtkDataObjectTree : public vtkCompositeDataSet
{
public:
  vtkTypeMacro(vtkDataObjectTree, vtkCompositeDataSet);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Return a new iterator (the iterator has to be deleted by user).
   */
  virtual vtkDataObjectTreeIterator* NewTreeIterator();

  /**
   * Return a new iterator (the iterator has to be deleted by user).

   * Use NewTreeIterator when you have a pointer to a vtkDataObjectTree
   * and NewIterator when you have a pointer to a vtkCompositeDataSet;
   * NewIterator is inherited and calls NewTreeIterator internally.
   */
  VTK_NEWINSTANCE vtkCompositeDataIterator* NewIterator() VTK_OVERRIDE;

  /**
   * Copies the tree structure from the input. All pointers to non-composite
   * data objects are intialized to NULL. This also shallow copies the meta data
   * associated with all the nodes.
   */
  void CopyStructure(vtkCompositeDataSet* input) VTK_OVERRIDE;

  /**
   * Sets the data set at the location pointed by the iterator.
   * The iterator does not need to be iterating over this dataset itself. It can
   * be any composite datasite with similar structure (achieved by using
   * CopyStructure).
   */
  void SetDataSet(vtkCompositeDataIterator* iter,
                  vtkDataObject* dataObj) VTK_OVERRIDE;

  /**
   * Sets the data at the location provided by a vtkDataObjectTreeIterator
   */
  void SetDataSetFrom(vtkDataObjectTreeIterator* iter, vtkDataObject* dataObj);

  /**
   * Returns the dataset located at the positiong pointed by the iterator.
   * The iterator does not need to be iterating over this dataset itself. It can
   * be an iterator for composite dataset with similar structure (achieved by
   * using CopyStructure).
   */
  vtkDataObject* GetDataSet(vtkCompositeDataIterator* iter) VTK_OVERRIDE;

  /**
   * Returns the meta-data associated with the position pointed by the iterator.
   * This will create a new vtkInformation object if none already exists. Use
   * HasMetaData to avoid creating the vtkInformation object unnecessarily.
   * The iterator does not need to be iterating over this dataset itself. It can
   * be an iterator for composite dataset with similar structure (achieved by
   * using CopyStructure).
   */
  virtual vtkInformation* GetMetaData(vtkCompositeDataIterator* iter);

  /**
   * Returns if any meta-data associated with the position pointed by the iterator.
   * The iterator does not need to be iterating over this dataset itself. It can
   * be an iterator for composite dataset with similar structure (achieved by
   * using CopyStructure).
   */
  virtual int HasMetaData(vtkCompositeDataIterator* iter);

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated.
   */
  unsigned long GetActualMemorySize() VTK_OVERRIDE;

  /**
   * Restore data object to initial state,
   */
  void Initialize() VTK_OVERRIDE;

  //@{
  /**
   * Shallow and Deep copy.
   */
  void ShallowCopy(vtkDataObject *src) VTK_OVERRIDE;
  void DeepCopy(vtkDataObject *src) VTK_OVERRIDE;
  //@}

  /**
   * Returns the total number of points of all blocks. This will
   * iterate over all blocks and call GetNumberOfPoints() so it
   * might be expansive.
   */
  vtkIdType GetNumberOfPoints() VTK_OVERRIDE;

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkDataObjectTree* GetData(vtkInformation* info);
  static vtkDataObjectTree* GetData(vtkInformationVector* v, int i=0);
  //@}

protected:
  vtkDataObjectTree();
  ~vtkDataObjectTree() VTK_OVERRIDE;

  /**
   * Set the number of children.
   */
  void SetNumberOfChildren(unsigned int num);

  /**
   * Get the number of children.
   */
  unsigned int GetNumberOfChildren();

  /**
   * Set child dataset at a given index. The number of children is adjusted to
   * to be greater than the index specified.
   */
  void SetChild(unsigned int index, vtkDataObject*);

  /**
   * Remove the child at a given index.
   */
  void RemoveChild(unsigned int index);

  /**
   * Returns a child dataset at a given index.
   */
  vtkDataObject* GetChild(unsigned int num);

  /**
   * Returns the meta-data at a given index. If the index is valid, however, no
   * information object is set, then a new one will created and returned.
   * To avoid unnecessary creation, use HasMetaData().
   */
  vtkInformation* GetChildMetaData(unsigned int index);

  /**
   * Sets the meta-data at a given index.
   */
  void SetChildMetaData(unsigned int index, vtkInformation* info);

  /**
   * Returns if meta-data information is available for the given child index.
   * Returns 1 is present, 0 otherwise.
   */
  int HasChildMetaData(unsigned int index);

  // The internal datastructure. Subclasses need not access this directly.
  vtkDataObjectTreeInternals* Internals;

  friend class vtkDataObjectTreeIterator;

private:
  vtkDataObjectTree(const vtkDataObjectTree&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataObjectTree&) VTK_DELETE_FUNCTION;

};

#endif
