/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextProperty.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTextProperty - represent text properties.
// .SECTION Description
// vtkTextProperty is an object that represents text properties.
// The primary properties that can be set are color, font size, font family
// horizontal and vertical justification, bold/italic/shadow styles.

// .SECTION See Also
// vtkTextMapper vtkTextActor vtkLegendBoxActor vtkCaptionActor2D

#ifndef __vtkTextProperty_h
#define __vtkTextProperty_h

#include "vtkObject.h"

// To be moved to SetGet.h at some point ?

#define VTK_TEXT_GLOBAL_ANTIALIASING_SOME 0
#define VTK_TEXT_GLOBAL_ANTIALIASING_NONE 1
#define VTK_TEXT_GLOBAL_ANTIALIASING_ALL 2

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
  // Set the color of the text..
  vtkSetVector3Macro(Color,float);
  vtkGetVectorMacro(Color,float,3);

  // Description:
  // Set/Get the font family. Three font types are allowed: Arial (VTK_ARIAL),
  // Courier (VTK_COURIER), and Times (VTK_TIMES).
  vtkSetClampMacro(FontFamily,int,VTK_ARIAL,VTK_TIMES);
  vtkGetMacro(FontFamily, int);
  void SetFontFamilyToArial()   { this->SetFontFamily(VTK_ARIAL);  };
  void SetFontFamilyToCourier() { this->SetFontFamily(VTK_COURIER);};
  void SetFontFamilyToTimes()   { this->SetFontFamily(VTK_TIMES);  };
  char *GetFontFamilyAsString();

  // Description:
  // Set/Get the font size.
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
  // Enable/disable text shadows.
  vtkSetMacro(Shadow, int);
  vtkGetMacro(Shadow, int);
  vtkBooleanMacro(Shadow, int);
  
  // Description:
  // Enable/disable the local aliasing hint.
  vtkSetMacro(AntiAliasing, int);
  vtkGetMacro(AntiAliasing, int);
  vtkBooleanMacro(AntiAliasing, int);

  // Description:
  // Set/Get the global antialiasing hint. Control whether to *globally* force
  // text antialiasing (ALL), disable antialiasing (NONE) or allow antialising
  // depending on the per-object AntiAliasing attribute (SOME).
  static int GetGlobalAntiAliasing();
  static void SetGlobalAntiAliasing(int val);
  static void SetGlobalAntiAliasingToSome() 
    { vtkTextProperty::SetGlobalAntiAliasing(VTK_TEXT_GLOBAL_ANTIALIASING_SOME); };
  static void SetGlobalAntiAliasingToNone() 
    { vtkTextProperty::SetGlobalAntiAliasing(VTK_TEXT_GLOBAL_ANTIALIASING_NONE); };
  static void SetGlobalAntiAliasingToAll()  
    { vtkTextProperty::SetGlobalAntiAliasing(VTK_TEXT_GLOBAL_ANTIALIASING_ALL); };
    
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
  char *GetJustificationAsString();
    
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
  char *GetVerticalJustificationAsString();
    
  // Description:
  // These methods can be used to control the spacing and placement of 
  // text (in the vertical direction). LineOffset is a vertical offset 
  // (measured in lines); LineSpacing is the spacing between lines.
  vtkSetMacro(LineOffset, float);
  vtkGetMacro(LineOffset, float);
  vtkSetMacro(LineSpacing, float);
  vtkGetMacro(LineSpacing, float);
  
  // Description:
  // Shallow copy of a text property.
  void ShallowCopy(vtkTextProperty *tprop);
  
protected:
  vtkTextProperty();
  ~vtkTextProperty();

  float Color[3];
  int   FontFamily;
  int   FontSize;
  int   Bold;
  int   Italic;
  int   Shadow;
  int   AntiAliasing;
  int   Justification;
  int   VerticalJustification;
  float LineOffset;
  float LineSpacing;
  
private:
  vtkTextProperty(const vtkTextProperty&);  // Not implemented.
  void operator=(const vtkTextProperty&);  // Not implemented.
};

inline char *vtkTextProperty::GetFontFamilyAsString(void)
{
  if (this->FontFamily == VTK_ARIAL)
    {
    return (char *)"Arial";
    }
  else if (this->FontFamily == VTK_COURIER) 
    {
    return (char *)"Courier";
    }
  else if (this->FontFamily == VTK_TIMES) 
    {
    return (char *)"Times";
    }
  return (char *)"Unknown";
}

inline char *vtkTextProperty::GetJustificationAsString(void)
{
  if (this->Justification == VTK_TEXT_LEFT)
    {
    return (char *)"Left";
    }
  else if (this->Justification == VTK_TEXT_CENTERED) 
    {
    return (char *)"Centered";
    }
  else if (this->Justification == VTK_TEXT_RIGHT) 
    {
    return (char *)"Right";
    }
  return (char *)"Unknown";
}

inline char *vtkTextProperty::GetVerticalJustificationAsString(void)
{
  if (this->VerticalJustification == VTK_TEXT_BOTTOM)
    {
    return (char *)"Bottom";
    }
  else if (this->VerticalJustification == VTK_TEXT_CENTERED) 
    {
    return (char *)"Centered";
    }
  else if (this->VerticalJustification == VTK_TEXT_TOP) 
    {
    return (char *)"Top";
    }
  return (char *)"Unknown";
}

#endif
