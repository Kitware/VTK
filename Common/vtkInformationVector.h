/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationVector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationVector - Store zero or more vtkInformation instances.
// .SECTION Description

// vtkInformationVector stores a vector of zero or more vtkInformation
// objects corresponding to the input or output information for a
// vtkAlgorithm.  An instance of this class is passed to
// vtkAlgorithm::ProcessUpstreamRequest and
// vtkAlgorithm::ProcessDownstreamRequest calls.

#ifndef __vtkInformationVector_h
#define __vtkInformationVector_h

#include "vtkObject.h"

class vtkInformation;
class vtkInformationVectorInternals;

class VTK_COMMON_EXPORT vtkInformationVector : public vtkObject
{
public:
  static vtkInformationVector *New();
  vtkTypeRevisionMacro(vtkInformationVector,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the number of information objects in the vector.  Setting
  // the number to larger than the current number will create empty
  // vtkInformation instances.  Setting the number to smaller than the
  // current number will remove entries from higher indices.
  int GetNumberOfInformationObjects();
  void SetNumberOfInformationObjects(int n);

  // Description:
  // Get/Set the vtkInformation instance stored at the given index in
  // the vector.  The vector will automatically expand to include the
  // index given if necessary.  Missing entries in-between will be
  // filled with empty vtkInformation instances.
  void SetInformationObject(int index, vtkInformation* info);
  vtkInformation* GetInformationObject(int index);

  // Description:
  // Copy the set of vtkInformation objects from the given vector.
  // All data in the information objects will be duplicated and stored
  // in new instances.  Any existing vector contents are removed.
  void DeepCopy(vtkInformationVector* from);

  // Description:
  // Copy the set of vtkInformation objects from the given vector.
  // The same instances of vtkInformation will be referenced.  Any
  // existing vector contents are removed.
  void ShallowCopy(vtkInformationVector* from);

protected:
  vtkInformationVector();
  ~vtkInformationVector();

  // Internal implementation details.
  vtkInformationVectorInternals* Internal;

private:
  vtkInformationVector(const vtkInformationVector&);  // Not implemented.
  void operator=(const vtkInformationVector&);  // Not implemented.
};

#endif
