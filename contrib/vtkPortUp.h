/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkPortUp.h
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
// .NAME vtkPortUp - First pass at new ports
// .SECTION Description
// UpStreamPort  any number of ports in any number of processes can connect
// to this port to get data.  They just use the tag (and ProcessId) to 
// specify which port they want.

// .SECTION see also
// vtkPortDown vtkMPIController

#ifndef __vtkPortUp_h
#define __vtkPortUp_h

#include "vtkProcessObject.h"
#include "vtkMPIController.h"
class vtkPolyData;


class VTK_EXPORT vtkPortUp : public vtkProcessObject
{
 public:
  vtkPortUp();
  ~vtkPortUp();  
  static vtkPortUp *New() {return new vtkPortUp;};
  const char *GetClassName() {return "vtkPortUp";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Should accept vtkDataObjects in the future.
  void SetInput(vtkPolyData *input);
  vtkDataObject *GetInput();
  
  // Description:
  // Output is specified by the process the output port is in,
  // and a tag so there can be more than one output port per process.
  // Tag must be set before this port can be used.
  void SetTag(int tag);
  vtkGetMacro(Tag, int);
  
  // Description:
  // This just forwards the wait onto the controller, which will wait
  // for a message for any of its ports (or any RMI).
  // For now, this method does not return.  I need to find an elegant
  // way to break this loop. (maybe a message between controllers)
  void WaitForUpdate() {this->Controller->ProcessRMIs();}
  
  // Description:
  // Access to the global controller.
  vtkMPIController *GetController() {return this->Controller;}

  // Description:
  // RMI function needs to call this.  Should make it a friend.
  // No one else should call it.
  void Trigger(int remoteProcessId);
  
protected:
  
  int Tag;
  
  vtkMPIController *Controller;
  vtkTimeStamp UpdateTime;

};

#endif


