/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataInformation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalDataInformation - hierarchical information collection
// .SECTION Description
// vtkHierarchicalDataInformation stores information objects in a structure
// corresponding to that of a hierarchical dataset. This is essentially a
// vector of vectors of information object pointers. Each entry in the
// outer vector corresponds to one levels, whereas each entry in the inner
// vector corresponds to one dataset.
// .SECTION See Also
// vtkHierarchicalDataSet vtkCompositeDataPipeline

#ifndef __vtkHierarchicalDataInformation_h
#define __vtkHierarchicalDataInformation_h

#include "vtkObject.h"

class vtkInformation;
//BTX
struct vtkHierarchicalDataInformationInternal;
//ETX

class VTK_FILTERING_EXPORT vtkHierarchicalDataInformation : public vtkObject
{
public:
  static vtkHierarchicalDataInformation *New();
  vtkTypeRevisionMacro(vtkHierarchicalDataInformation,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the number of hierarchy levels.
  unsigned int GetNumberOfLevels();

  // Description:
  // Set the number of hierarchy levels.
  void SetNumberOfLevels(unsigned int numLevels);

  // Description:
  // Given a level, returns the number of datasets.
  unsigned int GetNumberOfDataSets(unsigned int level);

  // Description:
  // Given a level, sets the number of datasets.
  void SetNumberOfDataSets(unsigned int level, unsigned int numDataSets);

  // Description:
  // Given a level and a dataset id, returns the corresponding information
  // object. If the information does not exist, one is created. Use
  // HasInformation() to check whether the information already exists.
  vtkInformation* GetInformation(unsigned int level, unsigned int id);

  // Description:
  // Returns 1 if information exists, 0 otherwise.
  int HasInformation(unsigned int level, unsigned int id);
  
  // Description:
  // Creates a duplicate hierarchy and calls Copy() on each information
  // object.
  void DeepCopy(vtkHierarchicalDataInformation* from);

  // Description:
  // Initializes the data structure to empty.
  void Clear();

protected:
  vtkHierarchicalDataInformation();
  ~vtkHierarchicalDataInformation();

private:
  vtkHierarchicalDataInformationInternal* Internal;

  vtkHierarchicalDataInformation(const vtkHierarchicalDataInformation&);  // Not implemented.
  void operator=(const vtkHierarchicalDataInformation&);  // Not implemented.
};

#endif
