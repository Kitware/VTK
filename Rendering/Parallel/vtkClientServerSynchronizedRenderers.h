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
// .NAME vtkClientServerSynchronizedRenderers
// .SECTION Description
// vtkClientServerSynchronizedRenderers is a vtkSynchronizedRenderers subclass
// designed to be used in 2 processes, client-server mode.

#ifndef vtkClientServerSynchronizedRenderers_h
#define vtkClientServerSynchronizedRenderers_h

#include "vtkRenderingParallelModule.h" // For export macro
#include "vtkSynchronizedRenderers.h"

class VTKRENDERINGPARALLEL_EXPORT vtkClientServerSynchronizedRenderers :
  public vtkSynchronizedRenderers
{
public:
  static vtkClientServerSynchronizedRenderers* New();
  vtkTypeMacro(vtkClientServerSynchronizedRenderers, vtkSynchronizedRenderers);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkClientServerSynchronizedRenderers();
  ~vtkClientServerSynchronizedRenderers();

  virtual void MasterEndRender();
  virtual void SlaveEndRender();

private:
  vtkClientServerSynchronizedRenderers(const vtkClientServerSynchronizedRenderers&); // Not implemented.
  void operator=(const vtkClientServerSynchronizedRenderers&); // Not implemented.
//ETX
};

#endif
