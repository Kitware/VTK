/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositePainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositePainter - painter that can be inserted before any
// vtkDataSet painting chain to handle composite datasets.
// .SECTION Description
// vtkCompositePainter iterates over the leaves in a composite datasets.
// This painter can also handle the case when the dataset is not a composite
// dataset.

#ifndef __vtkCompositePainter_h
#define __vtkCompositePainter_h

#include "vtkPainter.h"

class vtkInformationIntegerKey;

class VTK_RENDERING_EXPORT vtkCompositePainter : public vtkPainter
{
public:
  static vtkCompositePainter* New();
  vtkTypeMacro(vtkCompositePainter, vtkPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // When this key is present and set to 1 in the information passed to the
  // painter, the painter colors each block using a unique color. 
  static vtkInformationIntegerKey* COLOR_LEAVES();

  // Description:
  // Get the output data object from this painter. The default implementation
  // simply forwards the input data object as the output.
  virtual vtkDataObject* GetOutput();

//BTX
protected:
  vtkCompositePainter();
  ~vtkCompositePainter();

  // Description:
  // Take part in garbage collection.
  virtual void ReportReferences(vtkGarbageCollector *collector);

  // Description:
  // Called before RenderInternal() if the Information has been changed
  // since the last time this method was called.
  // Overridden to update the state of COLOR_LEAVES key.
  virtual void ProcessInformation(vtkInformation*);

  // Description:
  // Performs the actual rendering. Subclasses may override this method.
  // default implementation merely call a Render on the DelegatePainter,
  // if any. When RenderInternal() is called, it is assured that the 
  // DelegatePainter is in sync with this painter i.e. UpdateDelegatePainter()
  // has been called.
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
                              unsigned long typeflags, bool forceCompileOnly);

  vtkSetMacro(ColorLeaves, int);
  vtkGetMacro(ColorLeaves, int);
  int ColorLeaves;

  vtkDataObject* OutputData;
private:
  vtkCompositePainter(const vtkCompositePainter&); // Not implemented.
  void operator=(const vtkCompositePainter&); // Not implemented.
//ETX
};

#endif


