/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStripper.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkStripper - create triangle strips
// .SECTION Description
// vtkStripper is a filter that generates triangle strips from input
// polygons and triangle strips. Input polygons are assumed to be 
// triangles. (Use vtkTriangleFilter to triangulate non-triangular
// polygons.) The filter will also pass through vertices and lines, if
// requested.

#ifndef __vtkStripper_h
#define __vtkStripper_h

#include "vtkPolyToPolyFilter.hh"

class vtkStripper : public vtkPolyToPolyFilter
{
public:
  vtkStripper();
  char *GetClassName() {return "vtkStripper";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the maximum number of triangles in a triangle strip.
  vtkSetClampMacro(MaximumStripLength,int,4,VTK_CELL_SIZE-2);
  vtkGetMacro(MaximumStripLength,int);

  // Description:
  // Turn on/off passing of vertices through to output.
  vtkBooleanMacro(PassVerts,int);
  vtkSetMacro(PassVerts,int);
  vtkGetMacro(PassVerts,int);

  // Description:
  // Turn on/off passing of lines through to output.
  vtkBooleanMacro(PassLines,int);
  vtkSetMacro(PassLines,int);
  vtkGetMacro(PassLines,int);

protected:
  // Usual data generation method
  void Execute();

  int MaximumStripLength;
  // control whether vertices and lines are passed through filter
  int PassVerts;
  int PassLines;
};

#endif


