/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkROIStencilSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkROIStencilSource
 * @brief   create simple mask shapes
 *
 * vtkROIStencilSource will create an image stencil with a
 * simple shape like a box, a sphere, or a cylinder.  Its output can
 * be used with vtkImageStecil or other vtk classes that apply
 * a stencil to an image.
 * @sa
 * vtkImplicitFunctionToImageStencil vtkLassoStencilSource
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to VTK.
*/

#ifndef vtkROIStencilSource_h
#define vtkROIStencilSource_h


#include "vtkImagingStencilModule.h" // For export macro
#include "vtkImageStencilSource.h"

class VTKIMAGINGSTENCIL_EXPORT vtkROIStencilSource : public vtkImageStencilSource
{
public:
  static vtkROIStencilSource *New();
  vtkTypeMacro(vtkROIStencilSource, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  enum {
    BOX = 0,
    ELLIPSOID = 1,
    CYLINDERX = 2,
    CYLINDERY = 3,
    CYLINDERZ = 4
  };

  //@{
  /**
   * The shape of the region of interest.  Cylinders can be oriented
   * along the X, Y, or Z axes.  The default shape is "Box".
   */
  vtkGetMacro(Shape, int);
  vtkSetClampMacro(Shape, int, BOX, CYLINDERZ);
  void SetShapeToBox() { this->SetShape(BOX); };
  void SetShapeToEllipsoid() { this->SetShape(ELLIPSOID); };
  void SetShapeToCylinderX() { this->SetShape(CYLINDERX); };
  void SetShapeToCylinderY() { this->SetShape(CYLINDERY); };
  void SetShapeToCylinderZ() { this->SetShape(CYLINDERZ); };
  virtual const char *GetShapeAsString();
  //@}

  //@{
  /**
   * Set the bounds of the region of interest.  The bounds take
   * the spacing and origin into account.
   */
  vtkGetVector6Macro(Bounds, double);
  vtkSetVector6Macro(Bounds, double);
  //@}

protected:
  vtkROIStencilSource();
  ~vtkROIStencilSource();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  int Shape;
  double Bounds[6];

private:
  vtkROIStencilSource(const vtkROIStencilSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkROIStencilSource&) VTK_DELETE_FUNCTION;
};

#endif
