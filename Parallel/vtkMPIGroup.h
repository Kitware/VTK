/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIGroup.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMPIGroup - Class for creating MPI groups.

// .SECTION Description
// This class is used to create MPI groups. A vtkMPIGroup object
// has to be initialized by passing the controller. Then the
// group can be modified by adding, removing ids and copying the contents
// of another group.

// .SECTION See Also
// vtkMPIController vtkMPICommunicator

#ifndef __vtkMPIGroup_h
#define __vtkMPIGroup_h

#include "vtkObject.h"

class vtkMPIController;
class vtkMPICommunicator;

class VTK_PARALLEL_EXPORT vtkMPIGroup : public vtkObject
{

public:

  vtkTypeRevisionMacro( vtkMPIGroup,vtkObject);
  
  // Description:
  // Construct a vtkMPIGroup with the following initial state:
  // Processes = 0, MaximumNumberOfProcesses = 0.
  static vtkMPIGroup* New();
  
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate memory for N process ids where 
  // N = controller->NumberOfProcesses
  void Initialize(vtkMPIController* controller);

  // Description:
  // Add a process id to the end of the list (if it is not already
  // in the group). Returns non-zero on success.
  // This will not add a process id >= MaximumNumberOfProcessIds.
  int AddProcessId(int processId);

  // Description:
  // Remove the given process id from the list and
  // shift all ids, starting from the position of the removed
  // id, left by one.
  void RemoveProcessId(int processId);

  // Description:
  // Find the location of a process id in the group.
  // Returns -1 if the process id is not on the list.
  int FindProcessId(int processId);

  // Description:
  // Get the process id at position pos.
  // Returns -1 if pos >= max. available pos.
  int GetProcessId(int pos);

  // Description:
  // Copy the process ids from a given group.
  // This will copy N ids, where N is the smallest 
  // MaximumNumberOfProcessIds.
  void CopyProcessIdsFrom(vtkMPIGroup* group);

  // Description:
  // Returns the number of ids currently stored.
  int GetNumberOfProcessIds()
    {
      return this->CurrentPosition;
    }

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




