/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPrimitivePainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPrimitivePainter - superclass for class that handle single
// privmitives.
// .SECTION Description
// This is the abstract superclass for classes that handle single type
// of primitive i.e. verts, lines, polys or tstrips. 
// Concrete subclasses will pass a Render() call to the delegate painter,
// if any, only if it could not render.
// .SECTION Thanks
// Support for generic vertex attributes in VTK was contributed in
// collaboration with Stephane Ploix at EDF.

#ifndef __vtkPrimitivePainter_h
#define __vtkPrimitivePainter_h

#include "vtkPolyDataPainter.h"

class vtkDataArray;
class vtkPoints;
class vtkUnsignedCharArray;

class VTK_RENDERING_EXPORT vtkPrimitivePainter : public vtkPolyDataPainter
{
public:
  vtkTypeMacro(vtkPrimitivePainter, vtkPolyDataPainter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the type of primitive supported by this painter.
  // This must be set by concrete subclasses.
  vtkGetMacro(SupportedPrimitive, int);

protected:
  vtkPrimitivePainter();
  ~vtkPrimitivePainter();

  //BTX
  enum {
    VTK_PDM_NORMALS = 0x001,
    VTK_PDM_COLORS = 0x002,
    VTK_PDM_TCOORDS = 0x004,
    VTK_PDM_CELL_COLORS = 0x008,
    VTK_PDM_CELL_NORMALS = 0x010,
    VTK_PDM_OPAQUE_COLORS = 0x020,
    VTK_PDM_FIELD_COLORS = 0x040,
    VTK_PDM_EDGEFLAGS = 0x080,
    VTK_PDM_GENERIC_VERTEX_ATTRIBUTES = 0x100
  };
  //ETX

  // Description:
  // Some subclasses may need to do some preprocessing
  // before the actual rendering can be done eg. build effecient
  // representation for the data etc. This should be done here.
  // This method get called after the ProcessInformation()
  // but before RenderInternal(). 
  // This method is overridden to update the output data
  // as per the input.
  virtual void PrepareForRendering(vtkRenderer*, vtkActor*);

  // Description:
  // Called before RenderInternal() if the Information has been changed
  // since the last time this method was called.
  virtual void ProcessInformation(vtkInformation*);

  // Description:
  // Subclasses need to override this to return the output of the pipeline.
  virtual vtkDataObject* GetOutput();

  // Description:
  // The actual rendering happens here. This method is called only when
  // SupportedPrimitive is present in typeflags when Render() is invoked.
  // This method returns 1 when the rendering was successful. 
  // Concrete Primitive painters may support rendering a primitive only
  // when the input data satifies certain criteria. The return value is used
  // to decide if the subclasses succeeded in rendereing. If not the
  // render request is forwarded to the delegate. On success, the request 
  // forwareded to the delegate does not include a request to render the 
  // supported primitive type.
  virtual int RenderPrimitive(unsigned long flags, vtkDataArray* n,
    vtkUnsignedCharArray* c, vtkDataArray* t, vtkRenderer* ren) =0;

  // Description:
  // Based on the input polydata, setups certains flags and call
  // RenderPrimitive() which is overridden by subclasses. If RenderPrimitive()
  // is successful, the request forwarded to the delegate painter 
  // is with typeflags = (typeflags & ~this->SupportedPrimitive) i.e.
  // the request is to render everything other than what the subclass rendered.
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
                              unsigned long typeflags,
                              bool forceCompileOnly);

  // Description:
  // Take part in garbage collection.
  virtual void ReportReferences(vtkGarbageCollector *collector);

  int SupportedPrimitive; // must be set by subclasses.
  vtkSetMacro(SupportedPrimitive, int);

  int DisableScalarColor;

  vtkPolyData* OutputData;
  vtkTimeStamp OutputUpdateTime;
  bool GenericVertexAttributes;
  bool MultiTextureAttributes;

private:
  vtkPrimitivePainter(const vtkPrimitivePainter&); // Not implemented.
  void operator=(const vtkPrimitivePainter&); // Not implemented.
};

#endif
