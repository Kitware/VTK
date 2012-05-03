/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAlgorithmOutput.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAlgorithmOutput - Proxy object to connect input/output ports.
// .SECTION Description
// vtkAlgorithmOutput is a proxy object returned by the GetOutputPort
// method of vtkAlgorithm.  It may be passed to the
// SetInputConnection, AddInputConnection, or RemoveInputConnection
// methods of another vtkAlgorithm to establish a connection between
// an output and input port.  The connection is not stored in the
// proxy object: it is simply a convenience for creating or removing
// connections.

#ifndef __vtkAlgorithmOutput_h
#define __vtkAlgorithmOutput_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkObject.h"

class vtkAlgorithm;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkAlgorithmOutput : public vtkObject
{
public:
  static vtkAlgorithmOutput *New();
  vtkTypeMacro(vtkAlgorithmOutput,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetIndex(int index);
  int GetIndex();

  vtkAlgorithm* GetProducer();
  void SetProducer(vtkAlgorithm* producer);

protected:
  vtkAlgorithmOutput();
  ~vtkAlgorithmOutput();

  int Index;
  vtkAlgorithm* Producer;

private:
  vtkAlgorithmOutput(const vtkAlgorithmOutput&);  // Not implemented.
  void operator=(const vtkAlgorithmOutput&);  // Not implemented.
};

#endif
