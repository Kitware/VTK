/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPSurfaceLICMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPSurfaceLICMapper - parallel parts of the vtkSurfaceLICMapper
//
// .SECTION Description
// Parallel parts of the vtkSurfaceLICMapper, see that class for
// documentation.

#ifndef vtkPSurfaceLICMapper_h
#define vtkPSurfaceLICMapper_h

#include "vtkSurfaceLICMapper.h"
#include "vtkRenderingParallelLICModule.h" // For export macro
#include <string> // for string

class vtkPainterCommunicator;

class VTKRENDERINGPARALLELLIC_EXPORT vtkPSurfaceLICMapper : public vtkSurfaceLICMapper
{
public:
  static vtkPSurfaceLICMapper* New();
  vtkTypeMacro(vtkPSurfaceLICMapper, vtkSurfaceLICMapper);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Methods used for parallel benchmarks. Use cmake to define
  // vtkSurfaceLICMapperTIME to enable benchmarks. During each
  // update timing information is stored, it can be written to
  // disk by calling WriteLog.
  virtual void WriteTimerLog(const char *fileName);

protected:
  vtkPSurfaceLICMapper();
  ~vtkPSurfaceLICMapper();

  //BTX
  // Description:
  // Get the min/max across all ranks. min/max are in/out.
  // In serial operation this is a no-op, in parallel it
  // is a global collective reduction.
  virtual void GetGlobalMinMax(
        vtkPainterCommunicator *comm,
        float &min,
        float &max);

  // Description:
  // Creates a new communicator with/without the calling processes
  // as indicated by the passed in flag, if not 0 the calling process
  // is included in the new communicator. In parallel this call is mpi
  // collective on the world communicator. In serial this is a no-op.
  virtual vtkPainterCommunicator *CreateCommunicator(int include);
  //ETX

  // Description:
  // Ensure that if any rank udpates the communicator they all
  // do. This is a global collective operation.
  virtual bool NeedToUpdateCommunicator();

  // Description:
  // Methods used for parallel benchmarks. Use cmake to define
  // vtkSurfaceLICMapperTIME to enable benchmarks. During each
  // update timing information is stored, it can be written to
  // disk by calling WriteLog.
  virtual void StartTimerEvent(const char *name);
  virtual void EndTimerEvent(const char *name);

private:
  std::string LogFileName;

private:
  vtkPSurfaceLICMapper(const vtkPSurfaceLICMapper&); // Not implemented.
  void operator=(const vtkPSurfaceLICMapper&); // Not implemented.
};

#endif
