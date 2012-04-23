/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTStripsPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTStripsPainter - painter for triangle strips.

#ifndef __vtkTStripsPainter_h
#define __vtkTStripsPainter_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPrimitivePainter.h"

class VTKRENDERINGCORE_EXPORT vtkTStripsPainter : public vtkPrimitivePainter
{
public:
  static vtkTStripsPainter* New();
  vtkTypeMacro(vtkTStripsPainter, vtkPrimitivePainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkTStripsPainter();
  ~vtkTStripsPainter();

  // Description:
  // The actual rendering happens here. This method is called only when
  // SupportedPrimitive is present in typeflags when Render() is invoked.
  virtual int RenderPrimitive(unsigned long flags, vtkDataArray* n,
    vtkUnsignedCharArray* c, vtkDataArray* t, vtkRenderer* ren);
private:
  vtkTStripsPainter(const vtkTStripsPainter&); // Not implemented.
  void operator=(const vtkTStripsPainter&); // Not implemented.
};



#endif

