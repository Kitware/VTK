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
// .NAME vtkCompositeDataSet - abstact superclass for composite (multi-block or AMR) datasets
// .SECTION Description
// vtkCompositeDataSet is an abstract class that represents a collection
// of datasets (including other composite datasets). This superclass
// does not implement an actual method for storing datasets. It
// only provides an interface to access the datasets through iterators.

// .SECTION See Also
// vtkCompositeDataIterator 

#ifndef __vtkCompositeDataSet_h
#define __vtkCompositeDataSet_h

#include "vtkDataObject.h"

class vtkCompositeDataIterator;
class vtkInformation;
class vtkInformationDataObjectKey;

class VTK_FILTERING_EXPORT vtkCompositeDataSet : public vtkDataObject
{
public:
  vtkTypeRevisionMacro(vtkCompositeDataSet,vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a new (forward) iterator 
  // (the iterator has to be deleted by user)
  virtual vtkCompositeDataIterator* NewIterator() = 0;

  // Description:
  // Return class name of data type (see vtkType.h for
  // definitions).
  virtual int GetDataObjectType() {return VTK_COMPOSITE_DATA_SET;}

  // Description:
  // Restore data object to initial state,
  virtual void Initialize();

  // Description:
  // Adds dobj to the composite dataset. Where the dataset goes is determined
  // by appropriate keys in the index information object. Which keys are used
  // depends on the actual subclass.
  virtual void AddDataSet(vtkInformation* index, vtkDataObject* dobj) = 0;

  // Description: 
  // Returns a dataset pointed by appropriate keys in the index information
  // object.  Which keys are used depends on the actual subclass.
  virtual vtkDataObject* GetDataSet(vtkInformation* index) = 0;

  // Description:
  // Set the pipeline information object that owns this data
  // object.
  virtual void SetPipelineInformation(vtkInformation*);

  // Description:
  // Get the port currently producing this object.
  virtual vtkAlgorithmOutput* GetProducerPort();

  static vtkInformationIntegerKey* INDEX();
  static vtkInformationDataObjectKey* COMPOSITE_DATA_SET();

protected:
  vtkCompositeDataSet();
  ~vtkCompositeDataSet();

private:
  vtkCompositeDataSet(const vtkCompositeDataSet&);  // Not implemented.
  void operator=(const vtkCompositeDataSet&);  // Not implemented.
};

#endif

