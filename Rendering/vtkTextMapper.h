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
// To use vtkTextMapper, specify an input text string.

// .SECTION See Also
// vtkMapper2D vtkActor2D vtkLegendBoxActor vtkCaptionActor2D vtkVectorText vtkTextProperty

#ifndef __vtkTextMapper_h
#define __vtkTextMapper_h

#include "vtkMapper2D.h"

class vtkActor2D;
class vtkTextProperty;
class vtkViewport;

class VTK_RENDERING_EXPORT vtkTextMapper : public vtkMapper2D
{
public:
  vtkTypeRevisionMacro(vtkTextMapper,vtkMapper2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates a new text mapper.
  static vtkTextMapper *New();

  // Description:
  // Return the size[2]/width/height of the rectangle required to draw this
  // mapper (in pixels).
  virtual void GetSize(vtkViewport*, int size[2]) {size[0]=size[0];};
  virtual int GetWidth(vtkViewport*v);
  virtual int GetHeight(vtkViewport*v);
  
  // Description:
  // Set the input text string to the mapper.  The mapper recognizes "\n"
  // as a carriage return/linefeed (line separator).
  virtual void SetInput(const char *inputString);
  vtkGetStringMacro(Input);

  // Description:
  // Set/Get the text property.
  virtual void SetTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TextProperty,vtkTextProperty);
  
  // Description:
  // Set/Get the font family. Three font types are allowed: Arial (VTK_ARIAL),
  // Courier (VTK_COURIER), and Times (VTK_TIMES).
  // Warning: these functions remain for backward compatibility. Please access
  // the vtkTextProperty associated to the actor that use the text mapper.
  virtual void SetFontFamily(int val);
  virtual int GetFontFamily();
  void SetFontFamilyToArial()   { this->SetFontFamily(VTK_ARIAL);  };
  void SetFontFamilyToCourier() { this->SetFontFamily(VTK_COURIER);};
  void SetFontFamilyToTimes()   { this->SetFontFamily(VTK_TIMES);  };

  // Description:
  // Set/Get the font size.
  // Warning: these functions remain for backward compatibility. Please access
  // the vtkTextProperty associated to the actor that use the text mapper.
  virtual void SetFontSize(int size);
  virtual int GetFontSize();

  // Description:
  // Enable/disable text bolding.
  // Warning: these functions remain for backward compatibility. Please access
  // the vtkTextProperty associated to the actor that use the text mapper.
  virtual void SetBold(int val);
  virtual int GetBold();
  vtkBooleanMacro(Bold, int);

  // Description:
  // Enable/disable text italic.
  // Warning: these functions remain for backward compatibility. Please access
  // the vtkTextProperty associated to the actor that use the text mapper.
  virtual void SetItalic(int val);
  virtual int GetItalic();
  vtkBooleanMacro(Italic, int);

  // Description:
  // Enable/disable text shadows.
  // Warning: these functions remain for backward compatibility. Please access
  // the vtkTextProperty associated to the actor that use the text mapper.
  virtual void SetShadow(int val);
  virtual int GetShadow();
  vtkBooleanMacro(Shadow, int);
  
  // Description:
  // Set/Get the horizontal justification to left (default), centered,
  // or right.
  // Warning: these functions remain for backward compatibility. Please access
  // the vtkTextProperty associated to the actor that use the text mapper.
  virtual void SetJustification(int val);
  virtual int GetJustification();
  void SetJustificationToLeft()     
    { this->SetJustification(VTK_TEXT_LEFT);};
  void SetJustificationToCentered() 
    { this->SetJustification(VTK_TEXT_CENTERED);};
  void SetJustificationToRight()    
    { this->SetJustification(VTK_TEXT_RIGHT);};
    
  // Description:
  // Set/Get the vertical justification to bottom (default), middle,
  // or top.
  // Warning: these functions remain for backward compatibility. Please access
  // the vtkTextProperty associated to the actor that use the text mapper.
  virtual void SetVerticalJustification(int val);
  virtual int GetVerticalJustification();
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
  // Warning: these functions remain for backward compatibility. Please access
  // the vtkTextProperty associated to the actor that use the text mapper.
  virtual void SetLineOffset(float val);
  virtual float GetLineOffset();
  virtual void SetLineSpacing(float val);
  virtual float GetLineSpacing();

  vtkGetMacro(NumberOfLines,int);
  
  // Description:
  // Shallow copy of an actor.
  void ShallowCopy(vtkTextMapper *tm);
  
  // Description:
  // Determine the number of lines in the Input string (delimited by "\n").
  int  GetNumberOfLines(const char *input);

  // Description:
  // Set and return the font size required to make this mapper fit in a given 
  // target rectangle (width * height, in pixels).
  virtual int SetConstrainedFontSize(vtkViewport*, 
                                     int targetWidth, int targetHeight);

  // Description:
  // Set and return the font size required to make each element of an array of
  // mappers fit in a given rectangle (width * height, in pixels). 
  // This font size is the smallest size that was required to fit the largest 
  // mapper in this constraint. 
  // The resulting maximum area of the mappers is also returned.
  static int SetMultipleConstrainedFontSize(vtkViewport*, 
                                            int targetWidth, int targetHeight,
                                            vtkTextMapper** mappers, 
                                            int nbOfMappers, 
                                            int* maxResultingSize);

  // Description:
  // Get the available system font size matching a font size.
  virtual int GetSystemFontSize(int size) 
    { return size; };

protected:
  vtkTextMapper();
  ~vtkTextMapper();

  char* Input;
  vtkTextProperty *TextProperty;

  int  LineSize;
  int  NumberOfLines;
  int  NumberOfLinesAllocated;

  vtkTextMapper **TextLines;

  // These functions are used to parse, process, and render multiple lines 

  char *NextLine(const char *input, int lineNum);
  void GetMultiLineSize(vtkViewport* viewport, int size[2]);
  void RenderOverlayMultipleLines(vtkViewport *viewport, vtkActor2D *actor);
  
private:
  vtkTextMapper(const vtkTextMapper&);  // Not implemented.
  void operator=(const vtkTextMapper&);  // Not implemented.
};

#endif

