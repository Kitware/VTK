/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGL2PSUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGL2PSUtilities - Helper functions for using GL2PS within VTK
// .SECTION Description
// vtkGL2PSUtilities implements some static helper function that simplify
// calling GL2PS routines on VTK objects.

#ifndef __vtkGL2PSUtilities_h
#define __vtkGL2PSUtilities_h

#include "vtkObject.h"
#include "vtkRenderingGL2PSModule.h" // For export macro

class vtkImageData;
class vtkPath;
class vtkTextProperty;

class VTKRENDERINGGL2PS_EXPORT vtkGL2PSUtilities : public vtkObject
{
public:
  static vtkGL2PSUtilities *New();
  vtkTypeMacro(vtkGL2PSUtilities, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent)
  {
    this->Superclass::PrintSelf(os, indent);
  }

  // Description:
  // Format the text in str according to tprop and instruct GL2PS to draw it at
  // world coordinate pos.
  static void DrawString(const char *str, vtkTextProperty *tprop, double pos[3]);

  // Description:
  // Translate the tprop's fontname into a Postscript font name.
  static const char * TextPropertyToPSFontName(vtkTextProperty *tprop);

  // Description:
  // Convert the alignment hint in tprop to a GL2PS text alignment constant.
  static int TextPropertyToGL2PSAlignment(vtkTextProperty *tprop);

  // Description:
  // Generate PS, EPS, or SVG markup from a vtkPath object, and then inject it
  // into the output using the gl2psSpecial command. The path is translated
  // uniformly by translation. It is scaled by scale and rotated
  // counter-clockwise by rotateAngle. The windowSize is used to ensure correct
  // text placement in SVG output and ignored for PS/EPS. The rasterPos
  // is in world coordinates and determines clipping and depth.
  static void DrawPath(vtkPath *path, double rasterPos[3],
                       double windowSize[2], double translation[2],
                       double scale[2], double rotateAngle,
                       unsigned char color[3]);

protected:
  static void DrawPathPS(vtkPath *path, double rasterPos[3],
                         double windowSize[2], double translation[2],
                         double scale[2], double rotateAngle,
                         unsigned char color[3]);
  static void DrawPathPDF(vtkPath *path, double rasterPos[3],
                          double windowSize[2], double translation[2],
                          double scale[2], double rotateAngle,
                          unsigned char color[3]);
  static void DrawPathSVG(vtkPath *path, double rasterPos[3],
                          double windowSize[2], double translation[2],
                          double scale[2], double rotateAngle,
                          unsigned char color[3]);

  vtkGL2PSUtilities() {}
  ~vtkGL2PSUtilities() {}

private:
  vtkGL2PSUtilities(const vtkGL2PSUtilities &); // Not implemented
  void operator=(const vtkGL2PSUtilities&); // Not implemented
};

#endif
