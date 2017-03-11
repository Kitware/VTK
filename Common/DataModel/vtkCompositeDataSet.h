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
/**
 * @class   vtkCompositeDataSet
 * @brief   abstract superclass for composite
 * (multi-block or AMR) datasets
 *
 * vtkCompositeDataSet is an abstract class that represents a collection
 * of datasets (including other composite datasets). It
 * provides an interface to access the datasets through iterators.
 * vtkCompositeDataSet provides methods that are used by subclasses to store the
 * datasets.
 * vtkCompositeDataSet provides the datastructure for a full tree
 * representation. Subclasses provide the semantics for it and control how
 * this tree is built.
 *
 * @sa
 * vtkCompositeDataIterator
*/

#ifndef vtkCompositeDataSet_h
#define vtkCompositeDataSet_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"

class vtkCompositeDataIterator;
class vtkCompositeDataSetInternals;
class vtkInformation;
class vtkInformationStringKey;
class vtkInformationIntegerKey;

class VTKCOMMONDATAMODEL_EXPORT vtkCompositeDataSet : public vtkDataObject
{
public:
  vtkTypeMacro(vtkCompositeDataSet, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Return a new iterator (the iterator has to be deleted by user).
   */
  virtual VTK_NEWINSTANCE vtkCompositeDataIterator* NewIterator() =0;

  /**
   * Return class name of data type (see vtkType.h for
   * definitions).
   */
  int GetDataObjectType() VTK_OVERRIDE {return VTK_COMPOSITE_DATA_SET;}

  /**
   * Copies the tree structure from the input. All pointers to non-composite
   * data objects are initialized to NULL. This also shallow copies the meta data
   * associated with all the nodes.
   */
  virtual void CopyStructure(vtkCompositeDataSet* input)=0;

  /**
   * Sets the data set at the location pointed by the iterator.
   * The iterator does not need to be iterating over this dataset itself. It can
   * be any composite datasite with similar structure (achieved by using
   * CopyStructure).
   */
  virtual void SetDataSet(vtkCompositeDataIterator* iter, vtkDataObject* dataObj)=0;

  /**
   * Returns the dataset located at the positiong pointed by the iterator.
   * The iterator does not need to be iterating over this dataset itself. It can
   * be an iterator for composite dataset with similar structure (achieved by
   * using CopyStructure).
   */
  virtual vtkDataObject* GetDataSet(vtkCompositeDataIterator* iter)=0;


  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated.
   */
  unsigned long GetActualMemorySize() VTK_OVERRIDE;

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkCompositeDataSet* GetData(vtkInformation* info);
  static vtkCompositeDataSet* GetData(vtkInformationVector* v, int i=0);
  //@}

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
  virtual vtkIdType GetNumberOfPoints();

  /**
   * Key used to put node name in the meta-data associated with a node.
   */
  static vtkInformationStringKey* NAME();

  /**
   * Key used to indicate that the current process can load the data
   * in the node.  Used for parallel readers where the nodes are assigned
   * to the processes by the reader to indicate further down the pipeline
   * which nodes will be on which processes.
   * ***THIS IS AN EXPERIMENTAL KEY SUBJECT TO CHANGE WITHOUT NOTICE***
   */
  static vtkInformationIntegerKey* CURRENT_PROCESS_CAN_LOAD_BLOCK();

 protected:
  vtkCompositeDataSet();
  ~vtkCompositeDataSet() VTK_OVERRIDE;
 private:

  vtkCompositeDataSet(const vtkCompositeDataSet&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeDataSet&) VTK_DELETE_FUNCTION;

};

#endif


