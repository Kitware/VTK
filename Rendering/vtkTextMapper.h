/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextMapper.h
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
// .NAME vtkTextMapper - 2D text annotation
// .SECTION Description
// vtkTextMapper provides 2D text annotation support for vtk.  It is a
// vtkMapper2D that can be associated with a vtkActor2D and placed into a
// vtkRenderer or vtkImager.
//
// To use vtkTextMapper, specify an input text string, a font size,
// a font name, and whether to turn on bold or shadows (shadows make the
// font more visible when on top of other objects). You'll also need to 
// create a vtkActor2D and add it to the renderer or imager. You can create
// multiple lines by embedding "\n" in the Input string.
//
// The position of the text can be controlled by setting the justification
// to right, centered, or left. If you have specified multiple lines, all
// lines will be justified the same way.

// .SECTION See Also
// vtkMapper2D vtkActor2D vtkLegendBoxActor vtkCaptionActor2D vtkVectorText

#ifndef __vtkTextMapper_h
#define __vtkTextMapper_h

#include "vtkMapper2D.h"
#include "vtkWindow.h"
#include "vtkViewport.h"
#include "vtkActor2D.h"
#include "vtkProperty2D.h"

class VTK_RENDERING_EXPORT vtkTextMapper : public vtkMapper2D
{
public:
  vtkTypeRevisionMacro(vtkTextMapper,vtkMapper2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates a new text mapper with Font size 12, bold off, italic off,
  // and Arial font.
  static vtkTextMapper *New();

  // Description:
  // Return the size[2]/width/height of the rectangle required to draw this
  // mapper (in pixels).
  virtual void GetSize(vtkViewport*, int size[2]) {size[0]=size[0];};
  int GetWidth(vtkViewport*);
  int GetHeight(vtkViewport*);
  
  // Description:
  // Set the input text string to the mapper.  The mapper recognizes "\n"
  // as a carriage return/linefeed (line separator).
  void SetInput(const char *inputString);
  vtkGetStringMacro(Input);

  // Description:
  // Set the font size used by the mapper.  The subclasses can override
  // this function since all font sizes may not be available (especially
  // in X).
  virtual void SetFontSize(int size);

  // Description:
  // Return the font size actually in use by the mapper.  This value may
  // not match the value specified in the last SetFontSize if the last size
  // was unavailable.
  vtkGetMacro(FontSize, int);

  // Description:
  // Enable/disable text bolding.
  void SetBold(int val);
  vtkGetMacro(Bold, int);
  vtkBooleanMacro(Bold, int);

  // Description:
  // Enable/disable text italic.
  void SetItalic(int val);
  vtkGetMacro(Italic, int);
  vtkBooleanMacro(Italic, int);

  // Description:
  // Enable/disable text shadows.
  void SetShadow(int val);
  vtkGetMacro(Shadow, int);
  vtkBooleanMacro(Shadow, int);
  
  // Description:
  // Set/Get the font family.  Three font types are allowed: Arial (VTK_ARIAL),
  // Courier (VTK_COURIER), and Times (VTK_TIMES).
  void SetFontFamily(int val);
  vtkGetMacro(FontFamily, int);
  void SetFontFamilyToArial() {this->SetFontFamily(VTK_ARIAL);};
  void SetFontFamilyToCourier() {this->SetFontFamily(VTK_COURIER);};
  void SetFontFamilyToTimes() {this->SetFontFamily(VTK_TIMES);};

  // Description:
  // Set/Get the horizontal justification to left (default), centered,
  // or right.
  vtkSetClampMacro(Justification,int,VTK_TEXT_LEFT,VTK_TEXT_RIGHT);
  vtkGetMacro(Justification,int);
  void SetJustificationToLeft() {this->SetJustification(VTK_TEXT_LEFT);};
  void SetJustificationToCentered() {this->SetJustification(VTK_TEXT_CENTERED);};
  void SetJustificationToRight() {this->SetJustification(VTK_TEXT_RIGHT);};
    
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
    
  // Description:
  // These methods can be used to control the spacing and placement of 
  // text (in the vertical direction). LineOffset is a vertical offset 
  // (measured in lines); LineSpacing is the spacing between lines.
  vtkSetMacro(LineOffset, float);
  vtkGetMacro(LineOffset, float);
  vtkSetMacro(LineSpacing, float);
  vtkGetMacro(LineSpacing, float);
  vtkGetMacro(NumberOfLines,int);
  
  // Description:
  // Shallow copy of an actor.
  void ShallowCopy(vtkTextMapper *tm);
  
  // Description:
  // Determine the number of lines in the Input string (delimited by "\n").
  int  GetNumberOfLines(const char *input);

protected:
  vtkTextMapper();
  ~vtkTextMapper();

  int   Italic;
  int   Bold;
  int   Shadow;
  int   FontSize;
  int   FontFamily;
  char* Input;
  int   Justification;
  int   VerticalJustification;
  vtkTimeStamp FontMTime;
  // these functions are used to parse, process, and render multiple lines 
  int LineSize;
  float LineOffset;
  float LineSpacing;
  int  NumberOfLines;
  int  NumberOfLinesAllocated;
  vtkTextMapper **TextLines;
  char *NextLine(const char *input, int lineNum);
  void GetMultiLineSize(vtkViewport* viewport, int size[2]);
  void RenderOverlayMultipleLines(vtkViewport *viewport, vtkActor2D *actor);
  
private:
  vtkTextMapper(const vtkTextMapper&);  // Not implemented.
  void operator=(const vtkTextMapper&);  // Not implemented.
};

#endif

