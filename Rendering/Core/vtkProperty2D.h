/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProperty2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkProperty2D
 * @brief   represent surface properties of a 2D image
 *
 * vtkProperty2D contains properties used to render two dimensional images
 * and annotations.
 *
 * @sa
 * vtkActor2D
*/

#ifndef vtkProperty2D_h
#define vtkProperty2D_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkViewport;

#define VTK_BACKGROUND_LOCATION 0
#define VTK_FOREGROUND_LOCATION 1

class VTKRENDERINGCORE_EXPORT vtkProperty2D : public vtkObject
{
public:
  vtkTypeMacro(vtkProperty2D,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Creates a vtkProperty2D with the following default values:
   * Opacity 1, Color (1,1,1)
   */
  static vtkProperty2D *New();

  /**
   * Assign one property to another.
   */
  void DeepCopy(vtkProperty2D *p);

  //@{
  /**
   * Set/Get the RGB color of this property.
   */
  vtkSetVector3Macro(Color, double);
  vtkGetVector3Macro(Color, double);
  //@}

  //@{
  /**
   * Set/Get the Opacity of this property.
   */
  vtkGetMacro(Opacity, double);
  vtkSetMacro(Opacity, double);
  //@}

  //@{
  /**
   * Set/Get the diameter of a Point. The size is expressed in screen units.
   * This is only implemented for OpenGL. The default is 1.0.
   */
  vtkSetClampMacro(PointSize,float,0,VTK_FLOAT_MAX);
  vtkGetMacro(PointSize,float);
  //@}

  //@{
  /**
   * Set/Get the width of a Line. The width is expressed in screen units.
   * This is only implemented for OpenGL. The default is 1.0.
   */
  vtkSetClampMacro(LineWidth,float,0,VTK_FLOAT_MAX);
  vtkGetMacro(LineWidth,float);
  //@}

  //@{
  /**
   * Set/Get the stippling pattern of a Line, as a 16-bit binary pattern
   * (1 = pixel on, 0 = pixel off).
   * This is only implemented for OpenGL. The default is 0xFFFF.
   */
  vtkSetMacro(LineStipplePattern,int);
  vtkGetMacro(LineStipplePattern,int);
  //@}

  //@{
  /**
   * Set/Get the stippling repeat factor of a Line, which specifies how
   * many times each bit in the pattern is to be repeated.
   * This is only implemented for OpenGL. The default is 1.
   */
  vtkSetClampMacro(LineStippleRepeatFactor,int,1,VTK_INT_MAX);
  vtkGetMacro(LineStippleRepeatFactor,int);
  //@}

  //@{
  /**
   * The DisplayLocation is either background or foreground.
   * If it is background, then this 2D actor will be drawn
   * behind all 3D props or foreground 2D actors. If it is
   * background, then this 2D actor will be drawn in front of
   * all 3D props and background 2D actors. Within 2D actors
   * of the same DisplayLocation type, order is determined by
   * the order in which the 2D actors were added to the viewport.
   */
  vtkSetClampMacro( DisplayLocation, int,
                    VTK_BACKGROUND_LOCATION, VTK_FOREGROUND_LOCATION );
  vtkGetMacro( DisplayLocation, int );
  void SetDisplayLocationToBackground()
    {this->DisplayLocation = VTK_BACKGROUND_LOCATION;};
  void SetDisplayLocationToForeground()
    {this->DisplayLocation = VTK_FOREGROUND_LOCATION;};
  //@}

  /**
   * Have the device specific subclass render this property.
   */
  virtual void Render (vtkViewport* vtkNotUsed(viewport))  {}

protected:
  vtkProperty2D();
  ~vtkProperty2D() VTK_OVERRIDE;

  double Color[3];
  double Opacity;
  float PointSize;
  float LineWidth;
  int   LineStipplePattern;
  int   LineStippleRepeatFactor;
  int   DisplayLocation;

private:
  vtkProperty2D(const vtkProperty2D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkProperty2D&) VTK_DELETE_FUNCTION;
};

#endif
