/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceCubes.h
  Language:  C++
  Date:      11 Sep 1995
  Version:   1.12


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
// .NAME vtkSliceCubes - generate isosurface(s) from volume four slices at a time
// .SECTION Description
// vtkSliceCubes is a special version of the marching cubes filter. Instead
// of ingesting an entire volume at once it processes only four slices at
// a time. This way, it can generate isosurfaces from huge volumes. Also, the 
// output of this object is written to a marching cubes triangle file. That
// way, output triangles do not need to be held in memory.
// 
// To use vtkSliceCubes you must specify an instance of vtkVolumeReader to
// read the data. Set this object up with the proper file prefix, image range,
// data origin, data dimensions, header size, data mask, and swap bytes flag. 
// The vtkSliceCubes object will then take over and read slices as necessary. 
// You also will need to specify the name of an output marching cubes triangle 
// file.
//
// .SECTION Caveats
// This process object is both a source and mapper (i.e., it reads and writes 
// data to a file). This is different than the other marching cubes objects 
// (and most process objects in the system). It's specialized to handle very 
// large data.
//
// This object only extracts a single isosurface. This compares with the other
// contouring objects in vtk that generate multiple surfaces.
//
// To read the output file use vtkMCubesReader.

// .SECTION See Also
// vtkMarchingCubes vtkContourFilter vtkMCubesReader vtkDividingCubes vtkVolumeReader

#ifndef __vtkSliceCubes_h
#define __vtkSliceCubes_h

#include "vtkObject.h"
#include "vtkVolumeReader.h"
#include "vtkMCubesReader.h"

class vtkSliceCubes : public vtkObject
{
public:
  vtkSliceCubes();
  char *GetClassName() {return "vtkSliceCubes";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // methods to make it look like a filter
  void Write() {this->Update();};
  void Update();

  // Description:
  // Set/get object to read slices.
  vtkSetObjectMacro(Reader,vtkVolumeReader);
  vtkGetObjectMacro(Reader,vtkVolumeReader);

  // Description:
  // Specify file name of marching cubes output file.
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

  // Description:
  // Set/get isosurface contour value.
  vtkSetMacro(Value,float);
  vtkGetMacro(Value,float);

  // Description:
  // Specify file name of marching cubes limits file. The limits file
  // speeds up subsequent reading of output triangle file.
  vtkSetStringMacro(LimitsFilename);
  vtkGetStringMacro(LimitsFilename);

protected:
  void Execute();

  vtkVolumeReader *Reader;
  char *Filename;  
  float Value;
  char *LimitsFilename;
};

#endif

