/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTextSource - create polygonal text
// .SECTION Description
// vtkTextSource converts a text string into polygons.  This way you can 
// insert text into your renderings. It uses the 9x15 font from X Windows.
// You can specify if you want the background to be drawn or not. The
// characters are formed by scan converting the raster font into
// quadrilaterals. Colors are assigned to the letters using scalar data.
// To set the color of the characters with the source's actor property, set
// BackingOff on the text source and ScalarVisibilityOff on the associated
// vtkPolyDataMapper. Then, the color can be set using the associated actor's
// property.
//
// vtkVectorText generates higher quality polygonal representations of
// characters.

// .SECTION See Also
// vtkVectorText

#ifndef __vtkTextSource_h
#define __vtkTextSource_h

#include "vtkPolyDataAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkTextSource : public vtkPolyDataAlgorithm 
{
public:
  vtkTypeMacro(vtkTextSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with no string set and backing enabled.
  static vtkTextSource *New();

  // Description:
  // Set/Get the text to be drawn.
  vtkSetStringMacro(Text);
  vtkGetStringMacro(Text);

  // Description:
  // Controls whether or not a background is drawn with the text.
  vtkSetMacro(Backing,int);
  vtkGetMacro(Backing,int);
  vtkBooleanMacro(Backing,int);

  // Description:
  // Set/Get the foreground color. Default is white (1,1,1). ALpha is always 1.
  vtkSetVector3Macro(ForegroundColor,double);
  vtkGetVectorMacro(ForegroundColor,double,3);

  // Description:
  // Set/Get the background color. Default is black (0,0,0). Alpha is always 1.
  vtkSetVector3Macro(BackgroundColor,double);
  vtkGetVectorMacro(BackgroundColor,double,3);

protected:
  vtkTextSource();
  ~vtkTextSource();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  char *Text;
  int  Backing;
  double ForegroundColor[4];
  double BackgroundColor[4];
private:
  vtkTextSource(const vtkTextSource&);  // Not implemented.
  void operator=(const vtkTextSource&);  // Not implemented.
};

#endif
