/*=========================================================================

  Program:   Visualization Toolkit
  Module:    %M%
  Language:  C++
  Date:      %D%
  Version:   %V%

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
// .NAME vtkMCubesReader - write binary marching cubes file
// .SECTION Description
// vtkMCubesWriter is a writer object that writes binary marching cubes
// files. (Marching cubes is an isosurfacing technique that generates 
// many triangles.) The binary format is supported by W. Lorensen's
// marching cubes program (and the vtkSliceCubes object). Each triangle
// is represented by three records, with each record consisting of
// six single precision floating point numbers representing the a triangle vertex
// coordiante and vertex normal.
// .SECTION Caveats
// Binary files are written in sun/hp/sgi (i.e., Big Endian) form.
// .SECTION See Also
// vtkMarchingCubes vtkSliceCubes vtkMCubesWriter

#ifndef __vtkMCubesWriter_h
#define __vtkMCubesWriter_h

#include <stdio.h>
#include "vtkPolyWriter.h"
#include "vtkNormals.h"
class VTK_EXPORT vtkMCubesWriter : public vtkPolyWriter
{

public:
  vtkMCubesWriter();
  ~vtkMCubesWriter();
  static vtkMCubesWriter *New() {return new vtkMCubesWriter;};
  char *GetClassName() {return "vtkMCubesWriter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of marching cubes file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Specify file name of marching cubes limits file.
  vtkSetStringMacro(LimitsFileName);
  vtkGetStringMacro(LimitsFileName);

protected:
  void WriteData();

  char *FileName;
  char *LimitsFileName;
};

#endif


