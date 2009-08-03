/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFreeTypeLabelRenderStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFreeTypeLabelRenderStrategy - Renders labels with freetype
//
// .SECTION Description
// Uses the FreeType to render labels and compute label sizes.
// This strategy may be used with vtkLabelPlacementMapper.

#ifndef __vtkFreeTypeLabelRenderStrategy_h
#define __vtkFreeTypeLabelRenderStrategy_h

#include "vtkLabelRenderStrategy.h"

class vtkActor2D;
class vtkFreeTypeUtilities;
class vtkTextMapper;

class VTK_RENDERING_EXPORT vtkFreeTypeLabelRenderStrategy : public vtkLabelRenderStrategy
{
 public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkFreeTypeLabelRenderStrategy, vtkLabelRenderStrategy);
  static vtkFreeTypeLabelRenderStrategy* New();

  // Description:
  // The free type render strategy currently does not support rotation.
  virtual bool SupportsRotation()
    { return false; }

  //BTX
  // Description:
  // Compute the bounds of a label. Must be performed after the renderer is set.
  virtual void ComputeLabelBounds(vtkTextProperty* tprop, vtkUnicodeString label, double bds[4]);

  // Description:
  // Render a label at a location in world coordinates.
  // Must be performed between StartFrame() and EndFrame() calls.
  virtual void RenderLabel(double x[3], vtkTextProperty* tprop, vtkUnicodeString label);
  //ETX

protected:
  vtkFreeTypeLabelRenderStrategy();
  ~vtkFreeTypeLabelRenderStrategy();

  vtkFreeTypeUtilities* FreeTypeUtilities;
  vtkTextMapper* Mapper;
  vtkActor2D* Actor;

private:
  vtkFreeTypeLabelRenderStrategy(const vtkFreeTypeLabelRenderStrategy&);  // Not implemented.
  void operator=(const vtkFreeTypeLabelRenderStrategy&);  // Not implemented.
};

#endif

