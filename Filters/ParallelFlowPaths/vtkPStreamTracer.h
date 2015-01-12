/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPStreamTracer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPStreamTracer -  parallel streamline generators
// .SECTION Description
// This class implements parallel streamline generators.
// Note that all processes must have
// access to the WHOLE seed source, i.e. the source must be identical
// on all processes.
// .SECTION See Also
// vtkStreamTracer

#ifndef vtkPStreamTracer_h
#define vtkPStreamTracer_h

#include "vtkStreamTracer.h"
#include "vtkSmartPointer.h" // This is a leaf node. No need to use PIMPL to avoid compile time penalty.

class vtkAbstractInterpolatedVelocityField;
class vtkMultiProcessController;

class PStreamTracerPoint;
class vtkOverlappingAMR;
class AbstractPStreamTracerUtils;

#include "vtkFiltersParallelFlowPathsModule.h" // For export macro

class  VTKFILTERSPARALLELFLOWPATHS_EXPORT vtkPStreamTracer : public vtkStreamTracer
{
public:
  vtkTypeMacro(vtkPStreamTracer,vtkStreamTracer);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the controller use in compositing (set to
  // the global controller by default)
  // If not using the default, this must be called before any
  // other methods.
  virtual void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  static vtkPStreamTracer * New();

protected:

  vtkPStreamTracer();
  ~vtkPStreamTracer();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkMultiProcessController* Controller;

  vtkAbstractInterpolatedVelocityField* Interpolator;
  void SetInterpolator(vtkAbstractInterpolatedVelocityField*);

  int EmptyData;
private:
  vtkPStreamTracer(const vtkPStreamTracer&);  // Not implemented.
  void operator=(const vtkPStreamTracer&);  // Not implemented.

  void Trace( vtkDataSet *input,
              int vecType,
              const char* vecName,
              PStreamTracerPoint* pt,
              vtkSmartPointer<vtkPolyData>& output,
              vtkAbstractInterpolatedVelocityField* func,
              int maxCellSize);

  bool TraceOneStep(vtkPolyData* traceOut,  vtkAbstractInterpolatedVelocityField*, PStreamTracerPoint* pt);

  void Prepend(vtkPolyData* path, vtkPolyData* headh);
  int Rank;
  int NumProcs;

  friend class AbstractPStreamTracerUtils;
  vtkSmartPointer<AbstractPStreamTracerUtils> Utils;
};
#endif
