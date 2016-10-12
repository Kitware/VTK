/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOverrideInformationCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOverrideInformationCollection
 * @brief   maintain a list of override information objects
 *
 * vtkOverrideInformationCollection is an object that creates and manipulates
 * lists of objects of type vtkOverrideInformation.
 * @sa
 * vtkCollection
*/

#ifndef vtkOverrideInformationCollection_h
#define vtkOverrideInformationCollection_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkCollection.h"

#include "vtkOverrideInformation.h" // Needed for inline methods

class VTKCOMMONCORE_EXPORT vtkOverrideInformationCollection : public vtkCollection
{
public:
  vtkTypeMacro(vtkOverrideInformationCollection,vtkCollection);
  static vtkOverrideInformationCollection *New();

  /**
   * Add a OverrideInformation to the list.
   */
  void AddItem(vtkOverrideInformation *);

  /**
   * Get the next OverrideInformation in the list.
   */
  vtkOverrideInformation *GetNextItem();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkOverrideInformation *GetNextOverrideInformation(
    vtkCollectionSimpleIterator &cookie) {
    return static_cast<vtkOverrideInformation *>(
      this->GetNextItemAsObject(cookie));};

protected:
  vtkOverrideInformationCollection() {}
  ~vtkOverrideInformationCollection() VTK_OVERRIDE {}


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkOverrideInformationCollection(const vtkOverrideInformationCollection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOverrideInformationCollection&) VTK_DELETE_FUNCTION;
};

inline void vtkOverrideInformationCollection::AddItem(vtkOverrideInformation *f)
{
  this->vtkCollection::AddItem(f);
}

inline vtkOverrideInformation *vtkOverrideInformationCollection::GetNextItem()
{
  return static_cast<vtkOverrideInformation *>(this->GetNextItemAsObject());
}

#endif
// VTK-HeaderTest-Exclude: vtkOverrideInformationCollection.h
