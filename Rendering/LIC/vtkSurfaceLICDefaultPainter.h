/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceLICDefaultPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSurfaceLICDefaultPainter
 * @brief   vtkDefaultPainter replacement that
 *  inserts the vtkSurfaceLICPainter at the correct position in the painter
 *  chain.
 *
 *
 *  vtkSurfaceLICDefaultPainter is a vtkDefaultPainter replacement
 *  that inserts the vtkSurfaceLICPainter at the correct position in the painter
 *  chain.
 *
 * @sa
 *  vtkDefaultPainter vtkSurfaceLICPainter
*/

#ifndef vtkSurfaceLICDefaultPainter_h
#define vtkSurfaceLICDefaultPainter_h

#include "vtkRenderingLICModule.h" // For export macro
#include "vtkDefaultPainter.h"

class vtkSurfaceLICPainter;

class VTKRENDERINGLIC_EXPORT vtkSurfaceLICDefaultPainter
  : public vtkDefaultPainter
{
public:
  static vtkSurfaceLICDefaultPainter* New();
  vtkTypeMacro(vtkSurfaceLICDefaultPainter, vtkDefaultPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get/Set the Surface LIC painter.
   */
  void SetSurfaceLICPainter(vtkSurfaceLICPainter*);
  vtkGetObjectMacro(SurfaceLICPainter, vtkSurfaceLICPainter);
  //@}

protected:
  vtkSurfaceLICDefaultPainter();
  ~vtkSurfaceLICDefaultPainter();

  /**
   * Setup the the painter chain.
   */
  virtual void BuildPainterChain();

  /**
   * Take part in garbage collection.
   */
  void ReportReferences(vtkGarbageCollector *collector) VTK_OVERRIDE;

  /**
   * Override.
   */
  virtual void UpdateBounds(double bounds[6]);

protected:
  vtkSurfaceLICPainter* SurfaceLICPainter;

private:
  vtkSurfaceLICDefaultPainter(const vtkSurfaceLICDefaultPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSurfaceLICDefaultPainter&) VTK_DELETE_FUNCTION;

};

#endif
