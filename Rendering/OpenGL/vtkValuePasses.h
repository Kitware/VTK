/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValuePasses.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkValuePasses
 * @brief   Top level pass to render scene for ValuePainter
 *
 * vtkValuePasses sets up and renders the scene with a
 * black background (vtkClearRGBPass), antialising and other effects turned off and an inner
 * ValuePass renderer for opaque objects so that the drawn values can be
 * read back from the screen and used later.
 *
 * @sa
 * vtkClearRGBPass, vtkValuePass, vtkValuePainter
*/

#ifndef vtkValuePasses_h
#define vtkValuePasses_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkRenderPass.h"

class VTKRENDERINGOPENGL_EXPORT vtkValuePasses : public vtkRenderPass
{
public:
  static vtkValuePasses *New();
  vtkTypeMacro(vtkValuePasses,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * orchestrates inner helpers to perform overall rendering stateT
   */
  virtual void Render(const vtkRenderState *s);

  //@{
  /**
   * Set the array to be drawn. By default point scalars are
   */
  void SetInputArrayToProcess(int fieldAssociation, const char *name);
  void SetInputArrayToProcess(int fieldAssociation, int fieldAttributeType);
  void SetInputComponentToProcess(int comp);
  void SetScalarRange(double min, double max);
  //@}

 protected:
  /**
   * Default constructor.
   */
  vtkValuePasses();

  /**
   * Destructor.
   */
  virtual ~vtkValuePasses();

 private:
  vtkValuePasses(const vtkValuePasses&) VTK_DELETE_FUNCTION;
  void operator=(const vtkValuePasses&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals *Internals;
};

#endif
