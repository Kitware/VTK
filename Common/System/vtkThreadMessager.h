/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadMessager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkThreadMessager - A class for performing inter-thread messaging
// .SECTION Description
// vtkMultithreader is a class that provides support for messaging between
// threads multithreaded using pthreads or Windows messaging.

#ifndef __vtkThreadMessager_h
#define __vtkThreadMessager_h

#include "vtkCommonSystemModule.h" // For export macro
#include "vtkObject.h"

#if defined(VTK_USE_PTHREADS) || defined(VTK_HP_PTHREADS)
#include <pthread.h> // Needed for pthread types
#endif

class VTKCOMMONSYSTEM_EXPORT vtkThreadMessager : public vtkObject
{
public:
  static vtkThreadMessager *New();

  vtkTypeMacro(vtkThreadMessager,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Wait (block, non-busy) until another thread sends a
  // message.
  void WaitForMessage();

  // Description:
  // Send a message to all threads who are waiting via
  // WaitForMessage().
  void SendWakeMessage();

  // Description:
  // pthreads only. If the wait is enabled, the thread who
  // is to call WaitForMessage() will block until a receiver
  // thread is ready to receive.
  void EnableWaitForReceiver();

  // Description:
  // pthreads only. If the wait is enabled, the thread who
  // is to call WaitForMessage() will block until a receiver
  // thread is ready to receive.
  void DisableWaitForReceiver();

  // Description:
  // pthreads only.
  // If wait is enable, this will block until one thread is ready
  // to receive a message.
  void WaitForReceiver();

protected:
  vtkThreadMessager();
  ~vtkThreadMessager();

#ifdef VTK_USE_PTHREADS
  pthread_mutex_t Mutex;
  pthread_cond_t PSignal;
#endif

#ifdef VTK_USE_WIN32_THREADS
  vtkWindowsHANDLE WSignal;
#endif

private:
  vtkThreadMessager(const vtkThreadMessager&);  // Not implemented.
  void operator=(const vtkThreadMessager&);  // Not implemented.
};

#endif
