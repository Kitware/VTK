/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageProperty - image display properties
// .SECTION Description
// vtkImageProperty is an object that allows control of the display
// of an image slice.
// .SECTION Thanks
// Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
// Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
// .SECTION See also
// vtkImage vtkImageMapper3D vtkImageSliceMapper vtkImageResliceMapper

#ifndef __vtkImageProperty_h
#define __vtkImageProperty_h

#include "vtkObject.h"

class vtkScalarsToColors;

class VTK_RENDERING_EXPORT vtkImageProperty : public vtkObject
{
public:
  vtkTypeMacro(vtkImageProperty,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct a property with no lookup table.
  static vtkImageProperty *New();

  // Description:
  // Assign one property to another.
  void DeepCopy(vtkImageProperty *p);

  // Description:
  // The window value for window/level.
  vtkSetMacro(ColorWindow, double);
  vtkGetMacro(ColorWindow, double);

  // Description:
  // The level value for window/level.
  vtkSetMacro(ColorLevel, double);
  vtkGetMacro(ColorLevel, double);

  // Description:
  // Specify a lookup table for the data.  If the data is
  // to be displayed as greyscale, or if the input data is
  // already RGB, there is no need to set a lookup table.
  virtual void SetLookupTable(vtkScalarsToColors *lut);
  vtkGetObjectMacro(LookupTable, vtkScalarsToColors);

  // Description:
  // Use the range that is set in the lookup table, instead
  // of setting the range from the Window/Level settings.
  // This is off by default.
  vtkSetMacro(UseLookupTableScalarRange, int);
  vtkGetMacro(UseLookupTableScalarRange, int);
  vtkBooleanMacro(UseLookupTableScalarRange, int);

  // Description:
  // The opacity of the image, where 1.0 is opaque and 0.0 is
  // transparent.  If the image has an alpha component, then
  // the alpha component will be multiplied by this value.
  vtkSetClampMacro(Opacity, double, 0.0, 1.0);
  vtkGetMacro(Opacity, double);

  // Description:
  // The ambient lighting coefficient.  The default is 1.0.
  vtkSetClampMacro(Ambient, double, 0.0, 1.0);
  vtkGetMacro(Ambient, double);

  // Description:
  // The diffuse lighting coefficient.  The default is 0.0.
  vtkSetClampMacro(Diffuse, double, 0.0, 1.0);
  vtkGetMacro(Diffuse, double);

  // Description:
  // The interpolation type (default: nearest neighbor).
  vtkSetClampMacro(InterpolationType, int,
                   VTK_NEAREST_INTERPOLATION, VTK_CUBIC_INTERPOLATION);
  vtkGetMacro(InterpolationType, int);
  void SetInterpolationTypeToNearest() {
    this->SetInterpolationType(VTK_NEAREST_INTERPOLATION); };
  void SetInterpolationTypeToLinear() {
    this->SetInterpolationType(VTK_LINEAR_INTERPOLATION); };
  void SetInterpolationTypeToCubic() {
    this->SetInterpolationType(VTK_CUBIC_INTERPOLATION); };
  virtual const char *GetInterpolationTypeAsString();

  // Description:
  // Get the MTime for this property.  If the lookup table is set,
  // the mtime will include the mtime of the lookup table.
  unsigned long GetMTime();

protected:
  vtkImageProperty();
  ~vtkImageProperty();

  vtkScalarsToColors *LookupTable;
  double ColorWindow;
  double ColorLevel;
  int UseLookupTableScalarRange;
  int InterpolationType;
  double Opacity;
  double Ambient;
  double Diffuse;

private:
  vtkImageProperty(const vtkImageProperty&);  // Not implemented.
  void operator=(const vtkImageProperty&);  // Not implemented.
};

#endif
