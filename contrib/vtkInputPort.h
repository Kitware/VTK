/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkInputPort.h
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
// vtkInputPort vtkMultiProcessController

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

// Arbitrary tags used by the ports for communication.
#define VTK_PORT_DOWN_DATA_TIME_TAG         98970
#define VTK_PORT_UPDATE_EXTENT_TAG          98971
#define VTK_PORT_TRANSFER_NEEDED_TAG        98972
#define VTK_PORT_INFORMATION_TRANSFER_TAG   98973
#define VTK_PORT_DATA_TRANSFER_TAG          98974
#define VTK_PORT_NEW_DATA_TIME_TAG          98975


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
  // Output is specified by the process the output port is in,
  // and a tag so there can be more than one output port per process.
  // THE TAG MUST BE EVEN BECAUSE TWO RMIs ARE CREATED FROM IT!!!
  vtkSetMacro(RemoteProcessId, int);
  vtkGetMacro(RemoteProcessId, int);
  vtkSetMacro(Tag, int);
  vtkGetMacro(Tag, int);
  
  // Description:
  // We need special UpdateInformation and Update methods to 
  // communicate with the up-stream process.
  void UpdateInformation();
  void PreUpdate(vtkDataObject *output);
  void InternalUpdate(vtkDataObject *output);
  
  // Description:
  // Access to the global controller.
  vtkMultiProcessController *GetController() {return this->Controller;}

protected:
  vtkInputPort();
  ~vtkInputPort();  
  vtkInputPort(const vtkInputPort&) {};
  void operator=(const vtkInputPort&) {};
  
  vtkMultiProcessController *Controller;
  int RemoteProcessId;
  int Tag;

  unsigned long DataTime;
  unsigned long UpStreamMTime;
  int TransferNeeded;
};

#endif


