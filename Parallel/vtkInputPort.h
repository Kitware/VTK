/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkInputPort.h
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
// .NAME vtkInputPort - Receives data from another process. 
// .SECTION Description
// InputPort connects the pipeline in this process to one in another 
// processes.  It communicates all the pipeline protocol so that
// the fact you are running in multiple processes is transparent.
// An input port is used as a source (input to a process). 
// One is placed at the start of a pipeline, and has a single 
// corresponding output port in another process 
// (specified by RemoteProcessId).

// .SECTION see also
// vtkOutputPort vtkMultiProcessController

#ifndef __vtkInputPort_h
#define __vtkInputPort_h

#include "vtkSource.h"
#include "vtkMultiProcessController.h"
class vtkPolyData;
class vtkUnstructuredGrid;
class vtkStructuredGrid;
class vtkRectilinearGrid;
class vtkStructuredPoints;
class vtkImageData;



class VTK_EXPORT vtkInputPort : public vtkSource
{
public:
  static vtkInputPort *New();
  vtkTypeMacro(vtkInputPort,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Note: You have to ask for the right type, and it has to match
  // the type of the up stream port input, or you will get an error.
  // We have to live with the fact that the error will not occur until
  // an update is called.
  vtkPolyData *GetPolyDataOutput();
  vtkUnstructuredGrid *GetUnstructuredGridOutput();
  vtkStructuredGrid *GetStructuredGridOutput();
  vtkRectilinearGrid *GetRectilinearGridOutput();
  vtkStructuredPoints *GetStructuredPointsOutput();
  vtkImageData *GetImageDataOutput();
  
  // Description:
  // The matching OutputPort is specified by the output port's process
  // and a tag.  There can be more than one output port per process.
  // THE TAG MUST BE EVEN BECAUSE TWO RMIs ARE CREATED FROM IT!!!
  vtkSetMacro(RemoteProcessId, int);
  vtkGetMacro(RemoteProcessId, int);
  vtkSetMacro(Tag, int);
  vtkGetMacro(Tag, int);
  
  // Description:
  // Need to override to propagate across port.
  void UpdateInformation();

  // Description:
  // Need to override to propagate across port.
  void PropagateUpdateExtent(vtkDataObject *vtkNotUsed(output)) {};

  // Description:
  // Need to override to propagate across port
  void UpdateData( vtkDataObject *out );

  // Description:
  // Need to override to trigger the update across the port
  void TriggerAsynchronousUpdate();  
  
  // Description:
  // Access to the controller used for communication.  By default, the
  // global controller is used.
  vtkMultiProcessController *GetController() {return this->Controller;}
  vtkSetObjectMacro(Controller, vtkMultiProcessController);
  
  // Description:
  // If DoUpdateInformation if false (it is true by default),
  // UpdateInformation is not performed during Update. This can
  // be used to avoid  unnecessary communication once the data
  // has been transferred. However, if the pipeline changes
  // upstream, DoUpdateInformation has to be set to true again.
  // Otherwise, Updata will not occur.
  vtkSetMacro(DoUpdateInformation, int);
  vtkGetMacro(DoUpdateInformation, int);
  
//BTX

// Arbitrary tags used by the ports for communication.
  enum Tags {
    DOWN_DATA_TIME_TAG = 98970,
    UPDATE_EXTENT_TAG = 98971,
    TRANSFER_NEEDED_TAG = 98972,
    INFORMATION_TRANSFER_TAG = 98973,
    DATA_TRANSFER_TAG = 98974,
    NEW_DATA_TIME_TAG = 98975
  };

//ETX

protected:
  vtkInputPort();
  ~vtkInputPort();  
  vtkInputPort(const vtkInputPort&);
  void operator=(const vtkInputPort&);
  
  vtkMultiProcessController *Controller;
  int RemoteProcessId;
  int Tag;

  unsigned long DataTime;
  unsigned long UpStreamMTime;
  int TransferNeeded;
  int DoUpdateInformation;
};

#endif


