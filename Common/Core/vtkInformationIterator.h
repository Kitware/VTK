/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationIterator - Iterates over keys of an information object
// .SECTION Description
// vtkInformationIterator can be used to iterate over the keys of an
// information object. The corresponding values can then be directly
// obtained from the information object using the keys.
//
// .SECTION See Also
// vtkInformation vtkInformationKey

#ifndef vtkInformationIterator_h
#define vtkInformationIterator_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkInformation;
class vtkInformationKey;
class vtkInformationIteratorInternals;

class VTKCOMMONCORE_EXPORT vtkInformationIterator : public vtkObject
{
public:
  static vtkInformationIterator *New();
  vtkTypeMacro(vtkInformationIterator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the information to iterator over.
  void SetInformation(vtkInformation*);
  vtkGetObjectMacro(Information, vtkInformation);

  // Description:
  // Set the function to iterate over. The iterator
  // will not hold a reference to the information object.
  // Can be used to optimize certain places by avoiding
  // garbage collection.
  void SetInformationWeak(vtkInformation*);

  // Description:
  // Move the iterator to the beginning of the collection.
  void InitTraversal() { this->GoToFirstItem(); }

  // Description:
  // Move the iterator to the beginning of the collection.
  virtual void GoToFirstItem();

  // Description:
  // Move the iterator to the next item in the collection.
  virtual void GoToNextItem();

  // Description:
  // Test whether the iterator is currently pointing to a valid
  // item. Returns 1 for yes, 0 for no.
  virtual int IsDoneWithTraversal();

  // Description:
  // Get the current item. Valid only when IsDoneWithTraversal()
  // returns 1.
  virtual vtkInformationKey* GetCurrentKey();

protected:
  vtkInformationIterator();
  ~vtkInformationIterator();

  vtkInformation* Information;
  vtkInformationIteratorInternals* Internal;

  bool ReferenceIsWeak;

private:
  vtkInformationIterator(const vtkInformationIterator&);  // Not implemented.
  void operator=(const vtkInformationIterator&);  // Not implemented.
};

#endif
