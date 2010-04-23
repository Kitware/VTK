/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIGroup.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMPIGroup - DEPRECATED

// .SECTION Description
// This class has been deprecated in VTK 5.2.  Use vtkProcessGroup instead.

// .SECTION See Also
// vtkProcessGroup

#ifndef __vtkMPIGroup_h
#define __vtkMPIGroup_h

#include "vtkObject.h"

class vtkMPIController;
class vtkMPICommunicator;
class vtkProcessGroup;

#ifndef VTK_REMOVE_LEGACY_CODE
class VTK_PARALLEL_EXPORT vtkMPIGroup : public vtkObject
{

public:

  vtkTypeMacro( vtkMPIGroup,vtkObject);
  
  // Description:
  // Construct a vtkMPIGroup with the following initial state:
  // Processes = 0, MaximumNumberOfProcesses = 0.
  static vtkMPIGroup* New();
  
  VTK_LEGACY(virtual void PrintSelf(ostream& os, vtkIndent indent));

  // Description:
  // Allocate memory for N process ids where 
  // N = controller->NumberOfProcesses
  VTK_LEGACY(void Initialize(vtkMPIController* controller));

  // Description:
  // Add a process id to the end of the list (if it is not already
  // in the group). Returns non-zero on success.
  // This will not add a process id >= MaximumNumberOfProcessIds.
  VTK_LEGACY(int AddProcessId(int processId));

  // Description:
  // Remove the given process id from the list and
  // shift all ids, starting from the position of the removed
  // id, left by one.
  VTK_LEGACY(void RemoveProcessId(int processId));

  // Description:
  // Find the location of a process id in the group.
  // Returns -1 if the process id is not on the list.
  VTK_LEGACY(int FindProcessId(int processId));

  // Description:
  // Get the process id at position pos.
  // Returns -1 if pos >= max. available pos.
  VTK_LEGACY(int GetProcessId(int pos));

  // Description:
  // Copy the process ids from a given group.
  // This will copy N ids, where N is the smallest 
  // MaximumNumberOfProcessIds.
  VTK_LEGACY(void CopyProcessIdsFrom(vtkMPIGroup* group));

//BTX
  // Description:
  // Returns the number of ids currently stored.
  VTK_LEGACY(int GetNumberOfProcessIds());
//ETX

  // Description:
  // This method can be used to copy the MPIGroup into a vtkProcessGroup,
  // which is the successor to this class.
  void CopyInto(vtkProcessGroup *destination, vtkMPICommunicator *mpiComm);

//BTX

  friend class vtkMPICommunicator;

//ETX

protected:

  // Description:
  // Copies all the information from group, erasing
  // previously stored data. Similar to copy constructor
  void CopyFrom(vtkMPIGroup* group);

  // Description:
  // Allocate memory for numProcIds process ids
  void Initialize(int numProcIds);

  int* ProcessIds;
  int MaximumNumberOfProcessIds;
  int Initialized;
  int CurrentPosition;

  vtkMPIGroup();
  ~vtkMPIGroup();

private:
  vtkMPIGroup(const vtkMPIGroup&);  // Not implemented.
  void operator=(const vtkMPIGroup&);  // Not implemented.
};
#endif

#endif




