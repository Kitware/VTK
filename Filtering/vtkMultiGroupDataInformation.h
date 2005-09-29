/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataInformation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupDataInformation - hierarchical information collection
// .SECTION Description
// vtkMultiGroupDataInformation stores information objects in a structure
// corresponding to that of a multi-group dataset. This is essentially a
// vector of vectors of information object pointers. Each entry in the
// outer vector corresponds to one group, whereas each entry in the inner
// vector corresponds to one dataset.
// .SECTION See Also
// vtkMultiGroupDataSet vtkCompositeDataPipeline

#ifndef __vtkMultiGroupDataInformation_h
#define __vtkMultiGroupDataInformation_h

#include "vtkObject.h"

class vtkInformation;
//BTX
struct vtkMultiGroupDataInformationInternal;
//ETX

class VTK_FILTERING_EXPORT vtkMultiGroupDataInformation : public vtkObject
{
public:
  static vtkMultiGroupDataInformation *New();
  vtkTypeRevisionMacro(vtkMultiGroupDataInformation,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the number of hierarchy groups.
  unsigned int GetNumberOfGroups();

  // Description:
  // Set the number of hierarchy groups.
  void SetNumberOfGroups(unsigned int numGroups);

  // Description:
  // Given a group, returns the number of datasets.
  unsigned int GetNumberOfDataSets(unsigned int group);

  // Description:
  // Given a group, sets the number of datasets.
  void SetNumberOfDataSets(unsigned int group, unsigned int numDataSets);

  // Description:
  // Given a group and a dataset id, returns the corresponding information
  // object. If the information does not exist, one is created. Use
  // HasInformation() to check whether the information already exists.
  vtkInformation* GetInformation(unsigned int group, unsigned int id);

  // Description:
  // Returns 1 if information exists, 0 otherwise.
  int HasInformation(unsigned int group, unsigned int id);
  
  // Description:
  // Creates a duplicate hierarchy and calls Copy() on each information
  // object.
  void DeepCopy(vtkMultiGroupDataInformation* from);

  // Description:
  // Initializes the data structure to empty.
  void Clear();

protected:
  vtkMultiGroupDataInformation();
  ~vtkMultiGroupDataInformation();

private:
  vtkMultiGroupDataInformationInternal* Internal;

  vtkMultiGroupDataInformation(const vtkMultiGroupDataInformation&);  // Not implemented.
  void operator=(const vtkMultiGroupDataInformation&);  // Not implemented.
};

#endif
