/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TextSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkTextSource - create a Text centered at the origin
// .SECTION Description
// vtkTextSource converts a text string into polygons.  This way you can 
// insert text into your renderings.

#ifndef __vtkTextSource_h
#define __vtkTextSource_h

#include "PolySrc.hh"

class vtkTextSource : public vtkPolySource 
{
public:
  vtkTextSource();
  char *GetClassName() {return "vtkTextSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the text to be drawn.
  vtkSetStringMacro(Text);
  vtkGetStringMacro(Text);

  // Description:
  // Controlls whether or not a background is drawn with the text.
  vtkSetMacro(Backing,int);
  vtkGetMacro(Backing,int);
  vtkBooleanMacro(Backing,int);

protected:
  void Execute();
  char *Text;
  int   Backing;
};

#endif


