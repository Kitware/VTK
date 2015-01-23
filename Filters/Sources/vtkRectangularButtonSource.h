/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectangularButtonSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRectangularButtonSource - create a rectangular button
// .SECTION Description
// vtkRectangularButtonSource creates a rectangular shaped button with
// texture coordinates suitable for application of a texture map. This
// provides a way to make nice looking 3D buttons. The buttons are
// represented as vtkPolyData that includes texture coordinates and
// normals. The button lies in the x-y plane.
//
// To use this class you must define its width, height and length. These
// measurements are all taken with respect to the shoulder of the button.
// The shoulder is defined as follows. Imagine a box sitting on the floor.
// The distance from the floor to the top of the box is the depth; the other
// directions are the length (x-direction) and height (y-direction). In
// this particular widget the box can have a smaller bottom than top. The
// ratio in size between bottom and top is called the box ratio (by
// default=1.0). The ratio of the texture region to the shoulder region
// is the texture ratio. And finally the texture region may be out of plane
// compared to the shoulder. The texture height ratio controls this.

// .SECTION See Also
// vtkButtonSource vtkEllipticalButtonSource

// .SECTION Caveats
// The button is defined in the x-y plane. Use vtkTransformPolyDataFilter
// or vtkGlyph3D to orient the button in a different direction.


#ifndef vtkRectangularButtonSource_h
#define vtkRectangularButtonSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkButtonSource.h"

class vtkCellArray;
class vtkFloatArray;
class vtkPoints;

class VTKFILTERSSOURCES_EXPORT vtkRectangularButtonSource : public vtkButtonSource
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkRectangularButtonSource,vtkButtonSource);

  // Description:
  // Construct a circular button with depth 10% of its height.
  static vtkRectangularButtonSource *New();

  // Description:
  // Set/Get the width of the button.
  vtkSetClampMacro(Width,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(Width,double);

  // Description:
  // Set/Get the height of the button.
  vtkSetClampMacro(Height,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(Height,double);

  // Description:
  // Set/Get the depth of the button (the z-eliipsoid axis length).
  vtkSetClampMacro(Depth,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(Depth,double);

  // Description:
  // Set/Get the ratio of the bottom of the button with the
  // shoulder region. Numbers greater than one produce buttons
  // with a wider bottom than shoulder; ratios less than one
  // produce buttons that have a wider shoulder than bottom.
  vtkSetClampMacro(BoxRatio,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(BoxRatio,double);

  // Description:
  // Set/Get the ratio of the texture region to the
  // shoulder region. This number must be 0<=tr<=1.
  // If the texture style is to fit the image, then satisfying
  // the texture ratio may only be possible in one of the
  // two directions (length or width) depending on the
  // dimensions of the texture.
  vtkSetClampMacro(TextureRatio,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(TextureRatio,double);

  // Description:
  // Set/Get the ratio of the height of the texture region
  // to the shoulder height. Values greater than 1.0 yield
  // convex buttons with the texture region raised above the
  // shoulder. Values less than 1.0 yield concave buttons with
  // the texture region below the shoulder.
  vtkSetClampMacro(TextureHeightRatio,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(TextureHeightRatio,double);

  // Description:
  // Set/get the desired precision for the output points.
  // vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
  // vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);

protected:
  vtkRectangularButtonSource();
  ~vtkRectangularButtonSource() {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double Width;
  double Height;
  double Depth;

  double BoxRatio;
  double TextureRatio;
  double TextureHeightRatio;

  int OutputPointsPrecision;

private:
  vtkRectangularButtonSource(const vtkRectangularButtonSource&);  // Not implemented.
  void operator=(const vtkRectangularButtonSource&);  // Not implemented.
};

#endif


