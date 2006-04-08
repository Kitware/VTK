/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTextProperty - represent text properties.
// .SECTION Description
// vtkTextProperty is an object that represents text properties.
// The primary properties that can be set are color, opacity, font size,
// font family horizontal and vertical justification, bold/italic/shadow
// styles.
// .SECTION See Also
// vtkTextMapper vtkTextActor vtkLegendBoxActor vtkCaptionActor2D

#ifndef __vtkTextProperty_h
#define __vtkTextProperty_h

#include "vtkObject.h"

class VTK_RENDERING_EXPORT vtkTextProperty : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkTextProperty,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates a new text property with font size 12, bold off, italic off,
  // and Arial font.
  static vtkTextProperty *New();

  // Description:
  // Set the color of the text.
  vtkSetVector3Macro(Color,double);
  vtkGetVector3Macro(Color,double);

  // Description:
  // Set/Get the text's opacity. 1.0 is totally opaque and 0.0 is completely
  // transparent.
  vtkSetMacro(Opacity,double);
  vtkGetMacro(Opacity,double);

  // Description:
  // Set/Get the font family. Three font types are allowed: Arial (VTK_ARIAL),
  // Courier (VTK_COURIER), and Times (VTK_TIMES).
  vtkSetClampMacro(FontFamily,int,VTK_ARIAL,VTK_TIMES);
  vtkGetMacro(FontFamily, int);
  void SetFontFamilyToArial()   { this->SetFontFamily(VTK_ARIAL);  };
  void SetFontFamilyToCourier() { this->SetFontFamily(VTK_COURIER);};
  void SetFontFamilyToTimes()   { this->SetFontFamily(VTK_TIMES);  };
  const char *GetFontFamilyAsString();
  static const char *GetFontFamilyAsString( int f );

  // Description:
  // Set/Get the font size (in points).
  vtkSetClampMacro(FontSize,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(FontSize, int);

  // Description:
  // Enable/disable text bolding.
  vtkSetMacro(Bold, int);
  vtkGetMacro(Bold, int);
  vtkBooleanMacro(Bold, int);

  // Description:
  // Enable/disable text italic.
  vtkSetMacro(Italic, int);
  vtkGetMacro(Italic, int);
  vtkBooleanMacro(Italic, int);

  // Description:
  // Enable/disable text shadow.
  vtkSetMacro(Shadow, int);
  vtkGetMacro(Shadow, int);
  vtkBooleanMacro(Shadow, int);

  // Description:
  // Set/Get the shadow offset, i.e. the distance from the text to
  // its shadow, in the same unit as FontSize.
  vtkSetVector2Macro(ShadowOffset,int);
  vtkGetVectorMacro(ShadowOffset,int,2);

  // Description:
  // Get the shadow color. It is computed from the Color ivar
  void GetShadowColor(double color[3]);

  // Description:
  // Set/Get the horizontal justification to left (default), centered,
  // or right.
  vtkSetClampMacro(Justification,int,VTK_TEXT_LEFT,VTK_TEXT_RIGHT);
  vtkGetMacro(Justification,int);
  void SetJustificationToLeft()
    { this->SetJustification(VTK_TEXT_LEFT);};
  void SetJustificationToCentered()
    { this->SetJustification(VTK_TEXT_CENTERED);};
  void SetJustificationToRight()
    { this->SetJustification(VTK_TEXT_RIGHT);};
  const char *GetJustificationAsString();

  // Description:
  // Set/Get the vertical justification to bottom (default), middle,
  // or top.
  vtkSetClampMacro(VerticalJustification,int,VTK_TEXT_BOTTOM,VTK_TEXT_TOP);
  vtkGetMacro(VerticalJustification,int);
  void SetVerticalJustificationToBottom()
    {this->SetVerticalJustification(VTK_TEXT_BOTTOM);};
  void SetVerticalJustificationToCentered()
    {this->SetVerticalJustification(VTK_TEXT_CENTERED);};
  void SetVerticalJustificationToTop()
    {this->SetVerticalJustification(VTK_TEXT_TOP);};
  const char *GetVerticalJustificationAsString();

  // Description:
  // Set/Get the text's orientation (in degrees).
  vtkSetMacro(Orientation,double);
  vtkGetMacro(Orientation,double);

  // Description:
  // Set/Get the (extra) spacing between lines,
  // expressed as a text height multiplication factor.
  vtkSetMacro(LineSpacing, double);
  vtkGetMacro(LineSpacing, double);

  // Description:
  // Set/Get the vertical offset (measured in pixels).
  vtkSetMacro(LineOffset, double);
  vtkGetMacro(LineOffset, double);

  // Description:
  // Shallow copy of a text property.
  void ShallowCopy(vtkTextProperty *tprop);

protected:
  vtkTextProperty();
  ~vtkTextProperty() {};

  double Color[3];
  double Opacity;
  int   FontFamily;
  int   FontSize;
  int   Bold;
  int   Italic;
  int   Shadow;
  int   ShadowOffset[2];
  int   Justification;
  int   VerticalJustification;
  double Orientation;
  double LineOffset;
  double LineSpacing;

private:
  vtkTextProperty(const vtkTextProperty&);  // Not implemented.
  void operator=(const vtkTextProperty&);  // Not implemented.
};

inline const char *vtkTextProperty::GetFontFamilyAsString( int f )
{
  if ( f == VTK_ARIAL )
    {
    return "Arial";
    }
  else if ( f == VTK_COURIER )
    {
    return "Courier";
    }
  else if ( f == VTK_TIMES )
    {
    return "Times";
    }
  return "Unknown";
}

inline const char *vtkTextProperty::GetFontFamilyAsString(void)
{
  return vtkTextProperty::GetFontFamilyAsString( this->GetFontFamily() );
}

inline const char *vtkTextProperty::GetJustificationAsString(void)
{
  if (this->Justification == VTK_TEXT_LEFT)
    {
    return "Left";
    }
  else if (this->Justification == VTK_TEXT_CENTERED)
    {
    return "Centered";
    }
  else if (this->Justification == VTK_TEXT_RIGHT)
    {
    return "Right";
    }
  return "Unknown";
}

inline const char *vtkTextProperty::GetVerticalJustificationAsString(void)
{
  if (this->VerticalJustification == VTK_TEXT_BOTTOM)
    {
    return "Bottom";
    }
  else if (this->VerticalJustification == VTK_TEXT_CENTERED)
    {
    return "Centered";
    }
  else if (this->VerticalJustification == VTK_TEXT_TOP)
    {
    return "Top";
    }
  return "Unknown";
}

#endif
