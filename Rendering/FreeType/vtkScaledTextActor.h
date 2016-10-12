/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScaledTextActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkScaledTextActor
 * @brief   create text that will scale as needed
 *
 * vtkScaledTextActor is deprecated. New code should use vtkTextActor with
 * the Scaled = true option.
 *
 * @sa
 * vtkTextActor vtkActor2D vtkTextMapper
*/

#ifndef vtkScaledTextActor_h
#define vtkScaledTextActor_h

#include "vtkRenderingFreeTypeModule.h" // For export macro
#include "vtkTextActor.h"

class VTKRENDERINGFREETYPE_EXPORT vtkScaledTextActor : public vtkTextActor
{
public:
  vtkTypeMacro(vtkScaledTextActor,vtkTextActor);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Instantiate object with a rectangle in normaled view coordinates
   * of (0.2,0.85, 0.8, 0.95).
   */
  static vtkScaledTextActor *New();

protected:
   vtkScaledTextActor();
private:
  vtkScaledTextActor(const vtkScaledTextActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkScaledTextActor&) VTK_DELETE_FUNCTION;
};


#endif

