// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcessGroup.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkProcessGroup - A subgroup of processes from a communicator.
//
// .SECTION Description
//
// This class is used for creating groups of processes.  A vtkProcessGroup is
// initialized by passing the controller or communicator on which the group is
// based off of.  You can then use the group to subset and reorder the the
// processes.  Eventually, you can pass the group object to the
// CreateSubController method of vtkMultiProcessController to create a
// controller for the defined group of processes.  You must use the same
// controller (or attached communicator) from which this group was initialized
// with.
//
// .SECTION See Also
// vtkMultiProcessController, vtkCommunicator
//
// .SECTION Thanks
// This class was originally written by Kenneth Moreland (kmorel@sandia.gov)
// from Sandia National Laboratories.
//

#ifndef vtkProcessGroup_h
#define vtkProcessGroup_h

#include "vtkParallelCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkMultiProcessController;
class vtkCommunicator;

class VTKPARALLELCORE_EXPORT vtkProcessGroup : public vtkObject
{
public:
  vtkTypeMacro(vtkProcessGroup, vtkObject);
  static vtkProcessGroup *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Initialize the group to the given controller or communicator.  The group
  // will be set to contain all of the processes in the controller/communicator
  // in the same order.
  void Initialize(vtkMultiProcessController *controller);
  void Initialize(vtkCommunicator *communicator);

  // Description:
  // Get the communicator on which this group is based on.
  vtkGetObjectMacro(Communicator, vtkCommunicator);

  // Description:
  // Set the communicator.  This has the same effect as Initialize except that
  // the contents of the group will not be modified (although they may be
  // truncated if the new communicator is smaller than the current group).
  // Note that this can lead to an invalid group if there are values in the
  // group that are not valid in the new communicator.
  void SetCommunicator(vtkCommunicator *communicator);

  // Description:
  // Returns the size of this group (the number of processes defined in it).
  vtkGetMacro(NumberOfProcessIds, int);

  // Description:
  // Given a position in the group, returns the id of the process in the
  // communicator this group is based on.  For example, if this group contains
  // {6, 2, 8, 1}, then GetProcessId(2) will return 8 and GetProcessId(3) will
  // return 1.
  int GetProcessId(int pos) { return this->ProcessIds[pos]; }

  // Description:
  // Get the process id for the local process (as defined by the group's
  // communicator).  Returns -1 if the local process is not in the group.
  int GetLocalProcessId();

  // Description:
  // Given a process id in the communicator, this method returns its location in
  // the group or -1 if it is not in the group.  For example, if this group
  // contains {6, 2, 8, 1}, then FindProcessId(2) will return 1 and
  // FindProcessId(3) will return -1.
  int FindProcessId(int processId);

  // Description:
  // Add a process id to the end of the group (if it is not already in the
  // group).  Returns the location where the id was stored.
  int AddProcessId(int processId);

  // Description:
  // Remove the given process id from the group (assuming it is in the group).
  // All ids to the "right" of the removed id are shifted over.  Returns 1
  // if the process id was removed, 0 if the process id was not in the group
  // in the first place.
  int RemoveProcessId(int processId);

  // Description:
  // Removes all the processes ids from the group, leaving the group empty.
  void RemoveAllProcessIds();

  // Description:
  // Copies the given group's communicator and process ids.
  void Copy(vtkProcessGroup *group);

protected:
  vtkProcessGroup();
  virtual ~vtkProcessGroup();

  int *ProcessIds;
  int NumberOfProcessIds;

  vtkCommunicator *Communicator;

private:
  vtkProcessGroup(const vtkProcessGroup &);     // Not implemented
  void operator=(const vtkProcessGroup &);      // Not implemented
};

#endif //vtkProcessGroup_h
