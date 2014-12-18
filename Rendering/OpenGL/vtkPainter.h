/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME vtkPainter - Abstract class for drawing poly data.
//
// .SECTION Description
// This defines the interface for a Painter. Painters are helpers used
// by Mapper to perform the rendering. The mapper sets up a chain of painters
// and passes the render request to the painter. Every painter may have a
// delegate painter to which the render request is forwarded. The Painter may
// modify the request or data before passing it to the delegate painter.
// All the information to control the rendering must be passed to the painter
// using the vtkInformation object. A concrete painter may read special keys
// from the vtkInformation object and affect the rendering.
//
// .SECTION See Also
// vtkPainterPolyDataMapper

#ifndef vtkPainter_h
#define vtkPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkObject.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkAbstractArray;
class vtkActor;
class vtkDataObject;
class vtkDataSet;
class vtkInformation;
class vtkInformationIntegerKey;
class vtkPainterObserver;
class vtkRenderer;
class vtkTimerLog;
class vtkWindow;

class VTKRENDERINGOPENGL_EXPORT vtkPainter : public vtkObject
{
public:
  vtkTypeMacro(vtkPainter, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Keys used to specify control the behaviour of the painter.
  // When on, the painter assumes that the poly data changes infrequently.
  // It is thus more likely to take time and memory to build auxiliary data
  // structures for faster frame rates.  Is off by default.
  static vtkInformationIntegerKey* STATIC_DATA();

  // Description:
  // Keys used to specify control the behaviour of the painter.
  // When on, the painter avoids using more memory than it has to.  Thus,
  // auxiliary data structures for faster rendering may not be built.  Is off
  // by default.
  static vtkInformationIntegerKey* CONSERVE_MEMORY();

  // Description:
  // Keys used to specify control the behaviour of the painter.
  // When off, the painter may make approximations that will make the rendering
  // go faster but may degrade image quality.  Is on by default.
  static vtkInformationIntegerKey* HIGH_QUALITY();

  // Description:
  // Get/Set the information object associated with this painter.
  vtkGetObjectMacro(Information, vtkInformation);
  virtual void SetInformation(vtkInformation*);

  // Description:
  // Set/Get the painter to which this painter should propagare its draw calls.
  vtkGetObjectMacro(DelegatePainter, vtkPainter);
  virtual void SetDelegatePainter(vtkPainter*);

  // Description:
  // Take part in garbage collection.
  virtual void Register(vtkObjectBase *o);
  virtual void UnRegister(vtkObjectBase *o);

  //BTX
  enum {
    VERTS = 0x1,
    LINES = 0x2,
    POLYS = 0x4,
    STRIPS = 0x8
  };
  //ETX

  // Description:
  // Generates rendering primitives of appropriate type(s). Multiple types
  // of primitives can be requested by or-ring the primitive flags.
  // Default implementation calls UpdateDelegatePainter() to update the
  // deletagate painter and then calls RenderInternal().
  // forceCompileOnly is passed to the display list painters.
  virtual void Render(vtkRenderer* renderer, vtkActor* actor,
                      unsigned long typeflags, bool forceCompileOnly);

  // Description:
  // Release any graphics resources that are being consumed by this painter.
  // The parameter window could be used to determine which graphic
  // resources to release.
  // The call is propagated to the delegate painter, if any.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Set/Get the execution progress of a process object.
  vtkSetClampMacro(Progress,double,0.0,1.0);
  vtkGetMacro(Progress,double);

  // Description:
  // Get the time required to draw the geometry last time it was rendered.
  // Default implementation adds the current TimeToDraw with that of the
  // delegate painter.
  virtual double GetTimeToDraw();

  // Description:
  // Expand or shrink the estimated bounds of the object based on the
  // geometric transformations performed in the painter. If the painter
  // does not modify the geometry, the bounds are passed through.
  virtual void UpdateBounds(double bounds[6]);

  // Description:
  // Set the data object to paint. Currently we only support one data object per
  // painter chain.
  void SetInput(vtkDataObject*);
  vtkGetObjectMacro(Input, vtkDataObject);

  // Description:
  // Get the output data object from this painter. The default implementation
  // simply forwards the input data object as the output.
  virtual vtkDataObject* GetOutput()
    { return this->Input; }

//BTX
protected:
  vtkPainter();
  ~vtkPainter();

  // Description:
  // Take part in garbage collection.
  virtual void ReportReferences(vtkGarbageCollector *collector);

  // Description:
  // Updates the delegate painter. This method is called just before
  // the Render call is passed on to the DelegatePainter.
  // Hence, it gets called only if the DelegatePainter is set.
  // Internally calls PassInformation with argument as
  // this->DelegatePainter. Subclasses must not override this method,
  // instead override PassInformation().
  void UpdateDelegatePainter();

  // Description:
  // Pass on the information and data (output) from the
  // this to the argument painter. The method passes
  // the information only if it has changed.
  virtual void PassInformation(vtkPainter* toPainter);

  // Description:
  // Some subclasses may need to do some preprocessing
  // before the actual rendering can be done eg. build efficient
  // representation for the data etc. This should be done here.
  // This method get called after the ProcessInformation()
  // but before RenderInternal().
  virtual void PrepareForRendering(vtkRenderer*, vtkActor*) { }

  // Description:
  // Performs the actual rendering. Subclasses may override this method.
  // default implementation merely call a Render on the DelegatePainter,
  // if any. When RenderInternal() is called, it is assured that the
  // DelegatePainter is in sync with this painter i.e. UpdateDelegatePainter()
  // has been called.
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor,
                              unsigned long typeflags, bool forceCompileOnly);

  // Description:
  // Called when the delegate painter reports its progress.
  // Default implementation reports the delegate's progress
  // as its own.
  virtual void UpdateDelegateProgress(vtkPainter* delegate, double amount);

  // Description:
  // Called before RenderInternal() if the Information has been changed
  // since the last time this method was called.
  virtual void ProcessInformation(vtkInformation*) { }

  // Description:
  // Adds a progress event observer to toObserve. This provides a means
  // for the subclasses to observe progress events from painters.
  virtual void ObserverPainterProgress(vtkPainter* toObserve);

  // Description:
  // Update the progress of the process object. Then set the Progress ivar to
  // amount. The parameter amount should range between (0,1).
  // Raises vtkCommand::ProgressEvent.
  void UpdateProgress(double amount);

  // Description:
  // Helper method to get input array to process.
  vtkAbstractArray* GetInputArrayToProcess(int fieldAssociation,
    int fieldAttributeType,
    vtkDataSet* ds,
    bool *use_cell_data=0);
  vtkAbstractArray* GetInputArrayToProcess(int fieldAssociation,
    const char* name, vtkDataSet* dsl,
    bool *use_cell_data=0);

  // Time of most recent call to ProcessInformation().
  vtkTimeStamp InformationProcessTime;
  friend class vtkPainterObserver;
  vtkPainterObserver* Observer;

  vtkInformation* Information;
  vtkPainter* DelegatePainter;

  double Progress;
  double ProgressOffset;
  double ProgressScaleFactor;

  double TimeToDraw;
  vtkTimerLog* Timer;


  vtkWeakPointer<vtkWindow> LastWindow; // Window used for previous render.
                         // This is not reference counted.
private:
  vtkPainter(const vtkPainter &); // Not implemented.
  void operator=(const vtkPainter &);   // Not implemented.

  vtkDataObject* Input;
//ETX
};

#endif //vtkPainter_h
