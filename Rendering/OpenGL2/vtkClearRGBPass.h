/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClearRGBPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkClearRGBPass
 * @brief   Paint in the color buffer.
 *
 * Clear the color buffer to the specified color.
 *
 * @sa
 * vtkValuePasses
*/

#ifndef vtkClearRGBPass_h
#define vtkClearRGBPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkRenderPass.h"

class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkClearRGBPass : public vtkRenderPass
{
public:
  static vtkClearRGBPass *New();
  vtkTypeMacro(vtkClearRGBPass,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state s.
   */
  void Render(const vtkRenderState *s) override;

  //@{
  /**
   * Set/Get the background color of the rendering screen using an rgb color
   * specification.
   */
  vtkSetVector3Macro(Background,double);
  vtkGetVector3Macro(Background,double);
  //@}

 protected:
  /**
   * Default constructor.
   */
  vtkClearRGBPass();

  /**
   * Destructor.
   */
  ~vtkClearRGBPass() override;

  double Background[3];

 private:
  vtkClearRGBPass(const vtkClearRGBPass&) = delete;
  void operator=(const vtkClearRGBPass&) = delete;
};

#endif
