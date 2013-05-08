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

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPainter.h"
#include "vtkColor.h" // needed for vtkColor3d
#include <stack> //  needed for RenderBlockState.

class vtkCompositeDataDisplayAttributes;
class vtkInformationObjectBaseKey;
class vtkProperty;
class vtkRenderWindow;

class VTKRENDERINGCORE_EXPORT vtkCompositePainter : public vtkPainter
{
public:
  static vtkCompositePainter* New();
  vtkTypeMacro(vtkCompositePainter, vtkPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object from this painter. The default implementation
  // simply forwards the input data object as the output.
  virtual vtkDataObject* GetOutput();

  // Description:
  // Key used to pass a vtkCompositeDataDisplayAttributes instance doing the
  // painter pipeline.
  static vtkInformationObjectBaseKey* DISPLAY_ATTRIBUTES();

  // Description:
  // Set/get the composite data set display attributes. If set, these attributes
  // can be used by the painter to control specific rendering attributes on a
  // per-block basis for a multi-block dataset.
  void SetCompositeDataDisplayAttributes(vtkCompositeDataDisplayAttributes *attributes);
  vtkGetObjectMacro(CompositeDataDisplayAttributes, vtkCompositeDataDisplayAttributes)

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
  virtual void ProcessInformation(vtkInformation* information);

  // Description:
  // Performs the actual rendering. Subclasses may override this method.
  // default implementation merely call a Render on the DelegatePainter,
  // if any. When RenderInternal() is called, it is assured that the
  // DelegatePainter is in sync with this painter i.e. UpdateDelegatePainter()
  // has been called.
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor,
    unsigned long typeflags, bool forceCompileOnly);

  class RenderBlockState
    {
  public:
    std::stack<bool> Visibility;
    std::stack<double> Opacity;
    std::stack<vtkColor3d> AmbientColor;
    std::stack<vtkColor3d> DiffuseColor;
    std::stack<vtkColor3d> SpecularColor;

    double RenderedOpacity;
    vtkColor3d RenderedAmbientColor;
    vtkColor3d RenderedDiffuseColor;
    vtkColor3d RenderedSpecularColor;
    };

  void RenderBlock(vtkRenderer *renderer,
                   vtkActor *actor,
                   unsigned long typeflags,
                   bool forceCompileOnly,
                   vtkDataObject *dobj,
                   unsigned int &flat_index,
                   RenderBlockState &state);

  // Description:
  // Overridden in vtkOpenGLCompositePainter to pass attributes to OpenGL.
  virtual void UpdateRenderingState(
    vtkRenderWindow*, vtkProperty*, RenderBlockState&) {}

  vtkDataObject* OutputData;
  vtkCompositeDataDisplayAttributes *CompositeDataDisplayAttributes;
private:
  vtkCompositePainter(const vtkCompositePainter&); // Not implemented.
  void operator=(const vtkCompositePainter&); // Not implemented.
//ETX
};

#endif


