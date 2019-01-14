/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOverlayPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOverlayPass
 * @brief   Render the overlay geometry with property key
 * filtering.
 *
 * vtkOverlayPass renders the overlay geometry of all the props that have the
 * keys contained in vtkRenderState.
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 *
 * @sa
 * vtkRenderPass vtkDefaultPass
*/

#ifndef vtkOverlayPass_h
#define vtkOverlayPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkDefaultPass.h"

class VTKRENDERINGOPENGL2_EXPORT vtkOverlayPass : public vtkDefaultPass
{
public:
  static vtkOverlayPass *New();
  vtkTypeMacro(vtkOverlayPass,vtkDefaultPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState *s) override;

 protected:
  /**
   * Default constructor.
   */
  vtkOverlayPass();

  /**
   * Destructor.
   */
  ~vtkOverlayPass() override;

 private:
  vtkOverlayPass(const vtkOverlayPass&) = delete;
  void operator=(const vtkOverlayPass&) = delete;
};

#endif
