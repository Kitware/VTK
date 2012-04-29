/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTextMapper - 2D text annotation
// .SECTION Description
// vtkTextMapper provides 2D text annotation support for VTK.  It is a
// vtkMapper2D that can be associated with a vtkActor2D and placed into a
// vtkRenderer.
//
// To use vtkTextMapper, specify an input text string.

// .SECTION See Also
// vtkMapper2D vtkActor2D vtkLegendBoxActor vtkCaptionActor2D vtkVectorText vtkTextProperty

#ifndef __vtkTextMapper_h
#define __vtkTextMapper_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkMapper2D.h"

class vtkActor2D;
class vtkTextProperty;
class vtkViewport;

class VTKRENDERINGCORE_EXPORT vtkTextMapper : public vtkMapper2D
{
public:
  vtkTypeMacro(vtkTextMapper,vtkMapper2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates a new text mapper.
  static vtkTextMapper *New();

  // Description:
  // Return the size[2]/width/height of the rectangle required to draw this
  // mapper (in pixels).
  virtual void GetSize(vtkViewport*, int size[2]) {size[0]=size[0];}
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
  // Shallow copy of an actor.
  void ShallowCopy(vtkTextMapper *tm);

  // Description:
  // Determine the number of lines in the input string (delimited by "\n").
  int  GetNumberOfLines(const char *input);

  // Description:
  // Get the number of lines in the input string (the method GetNumberOfLines(char*)
  // must have been previously called for the return value to be valid).
  vtkGetMacro(NumberOfLines,int);

  // Description:
  // Set and return the font size required to make this mapper fit in a given
  // target rectangle (width x height, in pixels). A static version of the method
  // is also available for convenience to other classes (e.g., widgets).
  virtual int SetConstrainedFontSize(vtkViewport*, int targetWidth, int targetHeight);
  static int SetConstrainedFontSize(vtkTextMapper*, vtkViewport*, int targetWidth, int targetHeight);

  // Description:
  // Set and return the font size required to make each element of an array
  // of mappers fit in a given rectangle (width x height, in pixels).  This
  // font size is the smallest size that was required to fit the largest
  // mapper in this constraint.
  static int SetMultipleConstrainedFontSize(vtkViewport*,
                                            int targetWidth, int targetHeight,
                                            vtkTextMapper** mappers,
                                            int nbOfMappers,
                                            int* maxResultingSize);

  // Description:
  // Use these methods when setting font size relative to the renderer's size. These
  // methods are static so that external classes (e.g., widgets) can easily use them.
  static int SetRelativeFontSize(vtkTextMapper*, vtkViewport*, int *winSize,
                                 int *stringSize, float sizeFactor=0.0);
  static int SetMultipleRelativeFontSize(vtkViewport *viewport,
                                         vtkTextMapper **textMappers,
                                         int nbOfMappers, int *winSize,
                                         int *stringSize, float sizeFactor);

  // Description:
  // Get the available system font size matching a font size.
  virtual int GetSystemFontSize(int size)
    { return size; }

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

