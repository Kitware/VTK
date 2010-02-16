/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWedgeMark.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkWedgeMark - A wedge, or pie slice.
//
// .SECTION Description
// Represents a wedge, or pie slice. Specified in terms of start and end angle,
// inner and outer radius, wedges can be used to construct donut charts and
// polar bar charts as well. If the `angle' property is used, the end angle is
// implied by adding this value to start angle. By default, the start angle is
// the previously-generated wedge's end angle. This design allows explicit
// control over the wedge placement if desired, while offering convenient
// defaults for the construction of radial graphs.
//
// The center point of the circle is positioned using the standard box model.
// The wedge can be stroked and filled, similar to {link Bar}. 
//
// ref: http://protovis-js.googlecode.com/svn/trunk/jsdoc/symbols/pv.Wedge.html

#ifndef __vtkWedgeMark_h
#define __vtkWedgeMark_h

#include "vtkMark.h"

class vtkBrush;
class vtkDataObject;
class vtkPen;
class vtkInformationDoubleKey;
class vtkInformationStringKey;

class VTK_CHARTS_EXPORT vtkWedgeMark : public vtkMark
{
public:
  vtkTypeRevisionMacro(vtkWedgeMark, vtkMark);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkWedgeMark* New();

  /** @defgroup fields Fields
   *@{
   */
  
  // Description:
  // The angular span of the wedge, in degrees.
  // This property is used if end angle is not specified.
  // WARNING: protovis API uses radians.
  static vtkInformationDoubleKey *ANGLE();
  
  // Description:
  // The end angle of the wedge, in degrees.
  // If not specified, the end angle is implied as the START_ANGLE plus the
  // ANGLE.
  // WARNING: protovis API uses radians.
  static vtkInformationDoubleKey *END_ANGLE();
  
  // Description:
  // The wedge fill style; if non-null, the interior of the wedge is filled
  // with the specified color. The default value of this property is a
  // categorial color.
  static vtkInformationStringKey *FILL_STYLE();
  
  // Description:
  // The inner radius of the wedge, in pixels. The default value of this
  // property is zero; a positive value will produce a donut slice rather than
  // a pie slice. The inner radius can vary per-wedge.
  static vtkInformationDoubleKey *INNER_RADIUS();
  
  // Description:
  // The width of stroked lines, in pixels; used in conjunction with
  // strokeStyle to stroke the wedge's border.
  static vtkInformationDoubleKey *LINE_WIDTH();
  
  // Description:
  // The outer radius of the wedge, in pixels. This property is required. For
  // pies, only this radius is required; for donuts, the inner radius must be
  // specified as well. The outer radius can vary per-wedge.
  static vtkInformationDoubleKey *OUTER_RADIUS();
  
  // Description:
  // The start angle of the wedge, in degrees. The start angle is measured
  // clockwise from the 3 o'clock position. The default value of this property
  // is the end angle of the previous instance (the GetSibling()),
  // or -90 degrees for the first wedge; for pie and donut charts, typically
  // only the ANGLE property needs to be specified. 
  static vtkInformationDoubleKey *START_ANGLE();
  
  // Description:
  // The style of stroked lines; used in conjunction with lineWidth to stroke
  // the wedge's border. The default value of this property is null, meaning
  // wedges are not stroked by default. 
  static vtkInformationStringKey *STROKE_STYLE();
  
  
  /** @} */ // end of field group
  
  // Description:
  // Set default values.
  void AddWedgeDefault();
  
  // Description:
  // Returns the mid-angle of the wedge, which is defined as half-way between
  // the start and end angles. 
  double GetMidAngle();
  
  // Description:
  // Returns the mid-radius of the wedge, which is defined as half-way between
  // the inner and outer radii.
  double GetMidRadius();
  
  // Description:
  // Returns true if the specified angle is considered "upright", as in, text
  // rendered at that angle would appear upright. If the angle is not upright,
  // text is rotated 180 degrees to be upright, and the text alignment
  // properties are correspondingly changed.
  // \param angle in degrees.
  // WARNING: protovis API uses radians.
  bool upright(double angle);
  
  // Description:
  // Paint event for the item, called whenever the item needs to be drawn,
  virtual bool Paint(vtkContext2D *painter);

//BTX
  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);
//ETX
  
  virtual int GetType();
  
protected:
  vtkWedgeMark();
  ~vtkWedgeMark();
  
private:
  vtkWedgeMark(const vtkWedgeMark &); // Not implemented.
  void operator=(const vtkWedgeMark &);   // Not implemented.
};

#endif //__vtkWedgeMark_h
