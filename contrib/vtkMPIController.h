/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkMPIController.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkMPIController - Generic multiprocessing communication controller
// .SECTION Description
// vtkMPIController supplies a minimal set of communication methods as an
// abstract interface through a variety of multi-processing communication
// techniques.  It accepts Sends and Receives as well as implements
// remove method invocations (RMI)
// The "RegisterAndGetGlobalController" ensures that at most one 
// controller exists per process.  In most cases, the controller will
// be created automatically by a higher level object.

// .SECTION see also
// vtkMultiProcessPortUp vtkMultiProcessPortDown

#ifndef __vtkMPIController_h
#define __vtkMPIController_h

#include "vtkObject.h"
#include "mpi.h"
class vtkPolyData;
class vtkCollection;

class VTK_EXPORT vtkMPIController : public vtkObject
{
public:
  static vtkMPIController *New() {return new vtkMPIController;};
  const char *GetClassName() {return "vtkMPIController";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // This method returns an controller which must be UnRegistered.
  // If a global object already exists, it is registered and returned.
  static vtkMPIController *RegisterAndGetGlobalController(vtkObject *obj);
  
  // Description:
  // This method sends data to another process.  Tag eliminates ambiguity
  // when multiple sends ar receives exist in the same process.
  int Send(vtkObject *data, int remoteProcessId, int tag);
  int Send(int *data, int length, int remoteProcessId, int tag);
  int Send(unsigned long *data, int length, int remoteProcessId, int tag);

  // Description:
  // This method receives data from a corresponding send. It blocks
  // until the receive is finished.  It calls methods in "data"
  // to communicate the sending data.
  int Receive(vtkObject *data, int remoteProcessId, int tag);
  int Receive(int *data, int length, int remoteProcessId, int tag);
  int Receive(unsigned long *data, int length, int remoteProcessId, int tag);
  
  // Description:
  // Register remote method invocation in the receiving process
  // which makes the call.  It must have a unique tag as an RMI id.
  // The argument is a pointer local to the call.  Any arguments
  // from the call ing process must be gotten through sens and receives.
  void AddRMI(void (*f)(void *arg, int remoteProcessId), void *arg, int tag);
  
  // Description:
  // Take an RMI away.
  void RemoveRMI(void (*f)(void *, int), void *arg, int tag)
    {vtkErrorMacro("RemoveRMI Not Implemented Yet");};
  
  // Description:
  // A method to trigger a method invocation in another process.
  void TriggerRMI(int remoteProcessId, int tag);

  // Description:
  // Calling this method gives control to the controller to start
  // processing RMIs.
  void ProcessRMIs();
  
protected:
  vtkMPIController();
  ~vtkMPIController();
  
  int LocalProcessId;
  vtkCollection *RMIs;
  
  char *MarshalString;
  int MarshalStringLength;
  // The data may not take up all of the string.
  int MarshalDataLength;
  
  // convenience method
  void DeleteAndSetMarshalString(char *str, int strLength);
  
  // Write and read from marshal string
  // return 1 success, 0 fail
  int WriteObject(vtkObject *object);
  int ReadObject(vtkObject *object);
  
  int WritePolyData(vtkPolyData *object);
  int ReadPolyData(vtkPolyData *object);
  void CopyPolyData(vtkPolyData *src, vtkPolyData *dest);

  int WriteUnstructuredExtent(vtkUnstructuredExtent *object);
  int ReadUnstructuredExtent(vtkUnstructuredExtent *object);
};


#endif


