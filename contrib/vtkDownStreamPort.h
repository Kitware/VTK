/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkDownStreamPort.h
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
// .NAME vtkDownStreamPort - First pass at new ports: DownStreamPort.
// .SECTION Description
// Warning:  I do not think the logic handles output->ReleaseDataFlagOn()
// condition.  It is the UpdateInformation/Update role reversal issue.

// .SECTION see also
// vtkUpStreamPort vtkMultiProcessController

#ifndef __vtkDownStreamPort_h
#define __vtkDownStreamPort_h

#include "vtkSource.h"
#include "vtkMultiProcessController.h"
class vtkPolyData;

// Arbitrary tags used by the ports for communication.
#define VTK_PORT_DOWN_DATA_TIME_TAG         98970
#define VTK_PORT_UPDATE_EXTENT_TAG          98971
#define VTK_PORT_TRANSFER_NEEDED_TAG        98972
#define VTK_PORT_INFORMATION_TRANSFER_TAG   98973
#define VTK_PORT_DATA_TRANSFER_TAG          98974
#define VTK_PORT_NEW_DATA_TIME_TAG          98975


class VTK_EXPORT vtkDownStreamPort : public vtkSource
{
public:
  static vtkDownStreamPort *New() {return new vtkDownStreamPort;};
  const char *GetClassName() {return "vtkDownStreamPort";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Note: You have to ask for the right type, and it has to match
  // the type of the up stream port input, or you will get an error.
  // We have to live with the fact that the error will not occur until
  // an update is called.
  vtkPolyData *GetPolyDataOutput();
  
  // Description:
  // Output is specified by the process the output port is in,
  // and a tag so there can be more than one output port per process.
  // THE TAG MUST BE EVEN BECAUSE TWO RMIs ARE CREATED FROM IT!!!
  vtkSetMacro(UpStreamProcessId, int);
  vtkGetMacro(UpStreamProcessId, int);
  vtkSetMacro(Tag, int);
  vtkGetMacro(Tag, int);
  
  // Description:
  // We need special UpdateInformation and Update methods to 
  // communicate with the up-stream process.
  void InternalUpdate(vtkDataObject *output);
  void UpdateInformation();
  
  // Description:
  // Access to the global controller.
  vtkMultiProcessController *GetController() {return this->Controller;}

protected:
  vtkDownStreamPort();
  ~vtkDownStreamPort();  
  
  vtkMultiProcessController *Controller;
  int UpStreamProcessId;
  int Tag;

  unsigned long DataTime;
  int TransferNeeded;
};

#endif


