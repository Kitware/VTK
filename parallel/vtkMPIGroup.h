/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIGroup.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

class VTK_EXPORT vtkMPIGroup : public vtkObject
{

public:

  vtkTypeMacro( vtkMPIGroup,vtkObject);
  
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

  friend vtkMPICommunicator;

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
  vtkMPIGroup(const vtkMPIGroup&);
  void operator=(const vtkMPIGroup&);

};

#endif




