/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorEventRecorder.h
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
// .NAME vtkInteractorEventRecorder - record VTK events to a file; play them back to an object

// .SECTION Description
// vtkInteractorEventRecorder records all VTK events invoked from a
// vtkRenderWindowInteractor. The events are recorded to a
// file. vtkInteractorEventRecorder can also be used to play those events
// back and invoke them on an vtkRenderWindowInteractor.

// .SECTION See Also
// vtkInteractorObserver vtkCallback

#ifndef __vtkInteractorEventRecorder_h
#define __vtkInteractorEventRecorder_h

#include "vtkInteractorObserver.h"

// The superclass that all commands should be subclasses of
class VTK_RENDERING_EXPORT vtkInteractorEventRecorder : public vtkInteractorObserver
{
public:
  static vtkInteractorEventRecorder *New();
  vtkTypeRevisionMacro(vtkInteractorEventRecorder,vtkInteractorObserver);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Satisfy the superclass API. Enable/disable listening for events.
  virtual void SetEnabled(int);
  virtual void SetInteractor(vtkRenderWindowInteractor* iren);

  // Description:
  // Set/Get the name of a file events should be written to/from.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Invoke this method to begin recording events. The events will be
  // recorded to the filename indicated.
  void Record();

  // Description:
  // Invoke this method to begin playing events from the current position.
  // The events will be played back from the filename indicated.
  void Play();

  // Description:
  // Invoke this method to stop recording/playing events.
  // played back from the filename indicated.
  void Stop();

  // Description:
  // Rewind to the beginning of the file.
  void Rewind();

protected:
  vtkInteractorEventRecorder();
  ~vtkInteractorEventRecorder();

  // file to read/write from
  char *FileName;

  // for reading and writing
  istream *InputStream;
  ostream *OutputStream;

  //methods for processing events
  static void ProcessCharEvent(vtkObject* object, unsigned long event,
                               void* clientdata, void* calldata);
  static void ProcessEvents(vtkObject* object, unsigned long event,
                            void* clientdata, void* calldata);

  virtual void WriteEvent(const char* event, int pos[2], int ctrlKey,
                          int shiftKey, int keyCode, int repeatCount,
                          char* keySym);
  
  virtual void ReadEvent();

//BTX - manage the state of the recorder
  int State;
  enum WidgetState
  {
    Start=0,
    Playing,
    Recording
  };
//ETX

  static float StreamVersion;

private:
  vtkInteractorEventRecorder(const vtkInteractorEventRecorder&);  // Not implemented.
  void operator=(const vtkInteractorEventRecorder&);  // Not implemented.
  
};

#endif /* __vtkInteractorEventRecorder_h */
 
