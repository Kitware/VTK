/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellType.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkCellType - define types of cells
// .SECTION Description
// vtkCellType defines the allowable cell types in the visualization 
// library (vtk). In vtk, datasets consist of collections of cells. 
// Different datasets consist of different cell types. The cells may be 
// explicitly represented (as in vtkPolyData), or may be implicit to the
// data type (vtkStructuredPoints).

#ifndef __vtkCellType_h
#define __vtkCellType_h

#define VTK_NULL_ELEMENT 0
#define VTK_VERTEX 1
#define VTK_POLY_VERTEX 2
#define VTK_LINE 3
#define VTK_POLY_LINE 4
#define VTK_TRIANGLE 5
#define VTK_TRIANGLE_STRIP 6
#define VTK_POLYGON 7
#define VTK_PIXEL 8
#define VTK_QUAD 9
#define VTK_TETRA 10
#define VTK_VOXEL 11
#define VTK_HEXAHEDRON 12

#endif


