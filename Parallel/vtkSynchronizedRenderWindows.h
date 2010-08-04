/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSynchronizedRenderWindows - synchronizes render windows across
// processess.
// .SECTION Description
// vtkSynchronizedRenderWindows is used to synchronize render windows across
// processes for parallel rendering.

#ifndef __vtkSynchronizedRenderWindows_h
#define __vtkSynchronizedRenderWindows_h

#include "vtkObject.h"

class vtkRenderWindow;
class vtkMultiProcessController;
class vtkCommand;
class vtkMultiProcessStream;

class VTK_PARALLEL_EXPORT vtkSynchronizedRenderWindows : public vtkObject
{
public:
  static vtkSynchronizedRenderWindows* New();
  vtkTypeMacro(vtkSynchronizedRenderWindows, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the render window to be synchronized by this
  // vtkSynchronizedRenderWindows instance. A vtkSynchronizedRenderWindows can
  // be used to synchronize exactly 1 vtkRenderWindow on each process.
  void SetRenderWindow(vtkRenderWindow*);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);

  // Description:
  // Set the parallel message communicator. This is used to communicate among
  // processes.
  void SetParallelController(vtkMultiProcessController*);
  vtkGetObjectMacro(ParallelController, vtkMultiProcessController);

  // Description:
  // It's acceptable to have multiple instances on vtkSynchronizedRenderWindows
  // on each processes to synchronize different render windows. In that case
  // there's no way to each of the vtkSynchronizedRenderWindows instance to know
  // how they correspond across processes. To enable that identification, a
  // vtkSynchronizedRenderWindows can be assigned a unique id. All
  // vtkSynchronizedRenderWindows across different processes that have the same
  // id are "linked" together for synchronization. It's critical that the id is
  // set before any rendering happens.
  void SetIdentifier(unsigned int id);
  vtkGetMacro(Identifier, unsigned int);

  // Description:
  // Enable/Disable parallel rendering. Unless ParallelRendering is ON, no
  // synchronization of vtkRenderWindow::Render() calls between processes
  // happens. ON by default.
  vtkSetMacro(ParallelRendering, bool);
  vtkGetMacro(ParallelRendering, bool);
  vtkBooleanMacro(ParallelRendering, bool);

  // Turns on/off render event propagation.  When on (the default) and
  // ParallelRendering is on, process 0 will send an RMI call to all remote
  // processes to perform a synchronized render.  When off, render must be
  // manually called on each process.
  vtkSetMacro(RenderEventPropagation, bool);
  vtkGetMacro(RenderEventPropagation, bool);
  vtkBooleanMacro(RenderEventPropagation, bool);

  // Description:
  // This method call be called while a render is in progress to abort the
  // rendering. It should be called on the root node (or client).
  virtual void AbortRender();

  // Description:
  // Get/Set the root-process id. This is required when the ParallelController
  // is a vtkSocketController. Set to 0 by default (which will not work when
  // using a vtkSocketController but will work for vtkMPIController).
  vtkSetMacro(RootProcessId, int);
  vtkGetMacro(RootProcessId, int);

//BTX
  enum
    {
    SYNC_RENDER_TAG = 15001,
    };
protected:
  vtkSynchronizedRenderWindows();
  ~vtkSynchronizedRenderWindows();

  struct RenderWindowInfo
  {
    int WindowSize[2];
    int TileScale[2];
    double TileViewport[4];
    double DesiredUpdateRate;

    // Save/restore the struct to/from a stream.
    void Save(vtkMultiProcessStream& stream);
    bool Restore(vtkMultiProcessStream& stream);
    void CopyFrom(vtkRenderWindow*);
    void CopyTo(vtkRenderWindow*);
  };

  // These methods are called on all processes as a consequence of corresponding
  // events being called on the render window.
  virtual void HandleStartRender();
  virtual void HandleEndRender() {}
  virtual void HandleAbortRender() {}

  virtual void MasterStartRender();
  virtual void SlaveStartRender();

  unsigned int Identifier;
  bool ParallelRendering;
  bool RenderEventPropagation;
  int RootProcessId;

  vtkRenderWindow* RenderWindow;
  vtkMultiProcessController* ParallelController;

private:
  vtkSynchronizedRenderWindows(const vtkSynchronizedRenderWindows&); // Not implemented.
  void operator=(const vtkSynchronizedRenderWindows&); // Not implemented.

  class vtkObserver;
  vtkObserver* Observer;
  friend class vtkObserver;
//ETX
};

#endif


