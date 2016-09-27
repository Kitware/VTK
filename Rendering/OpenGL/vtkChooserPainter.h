/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChooserPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkChooserPainter
 * @brief   Painter that selects painters to render
 * primitives.
 *
 *
 *
 * This painter does not actually do any painting.  Instead, it picks other
 * painters based on the current state of itself and its poly data.  It then
 * delegates the work to these other painters.
 *
*/

#ifndef vtkChooserPainter_h
#define vtkChooserPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPolyDataPainter.h"

class VTKRENDERINGOPENGL_EXPORT vtkChooserPainter : public vtkPolyDataPainter
{
public:
  static vtkChooserPainter *New();
  vtkTypeMacro(vtkChooserPainter, vtkPolyDataPainter);
  void PrintSelf(ostream &os, vtkIndent indent);

  void SetVertPainter(vtkPolyDataPainter*);
  void SetLinePainter(vtkPolyDataPainter*);
  void SetPolyPainter(vtkPolyDataPainter*);
  void SetStripPainter(vtkPolyDataPainter*);

  /*
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release. Merely propagates the call to the painter.
   */
  // void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkChooserPainter();
  ~vtkChooserPainter();

  vtkPolyDataPainter *VertPainter;
  vtkPolyDataPainter *LinePainter;
  vtkPolyDataPainter *PolyPainter;
  vtkPolyDataPainter *StripPainter;



  /**
   * Some subclasses may need to do some preprocessing
   * before the actual rendering can be done eg. build efficient
   * representation for the data etc. This should be done here.
   * This method get called after the ProcessInformation()
   * but before RenderInternal().
   * Overridden to setup the the painters if needed.
   */
  virtual void PrepareForRendering(vtkRenderer*, vtkActor*);

  /**
   * Called to pick which painters to used based on the current state of
   * this painter and the poly data.
   */
  virtual void ChoosePainters(vtkRenderer *renderer, vtkActor*);

  /**
   * Called from ChoosePainters.  Returns a string for the type of
   * each painter.  The painters will be built with CreatePainter.
   */
  virtual void SelectPainters(vtkRenderer *renderer, vtkActor* actor,
                              const char *&vertpaintertype,
                              const char *&linepaintertype,
                              const char *&polypaintertype,
                              const char *&strippaintertype);

  /**
   * Passes on the information and the data to the chosen painters.
   */
  virtual void UpdateChoosenPainters();

  /**
   * Creates a painter of the given type.
   */
  virtual vtkPolyDataPainter *CreatePainter(const char *paintertype);

  /**
   * Performs the actual rendering. Subclasses may override this method.
   * default implementation merely call a Render on the DelegatePainter,
   * if any. When RenderInternal() is called, it is assured that the
   * DelegatePainter is in sync with this painter i.e. UpdatePainter()
   * has been called.
   */
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor,
                              unsigned long typeflags, bool forceCompileOnly);

  /**
   * Take part in garbage collection.
   */
  void ReportReferences(vtkGarbageCollector *collector) VTK_OVERRIDE;

  vtkRenderer *LastRenderer;
  vtkTimeStamp PaintersChoiceTime;

private:
  vtkChooserPainter(const vtkChooserPainter &) VTK_DELETE_FUNCTION;
  void operator=(const vtkChooserPainter &) VTK_DELETE_FUNCTION;
};

#endif //_vtkChooserPainter_h
