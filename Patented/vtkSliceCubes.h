/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceCubes.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

     THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 4,710,876
     "System and Method for the Display of Surface Structures Contained
     Within the Interior Region of a Solid Body".
     Application of this software for commercial purposes requires 
     a license grant from GE. Contact:

         Carl B. Horton
         Sr. Counsel, Intellectual Property
         3000 N. Grandview Blvd., W-710
         Waukesha, WI  53188
         Phone:  (262) 513-4022
         E-Mail: Carl.Horton@med.ge.com

     for more information.

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

class vtkVolumeReader;

class VTK_PATENTED_EXPORT vtkSliceCubes : public vtkObject
{
public:
  static vtkSliceCubes *New();
  vtkTypeRevisionMacro(vtkSliceCubes,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // methods to make it look like a filter
  void Write() {this->Update();};
  void Update();

  // Description:
  // Set/get object to read slices.
  virtual void SetReader(vtkVolumeReader*);
  vtkGetObjectMacro(Reader,vtkVolumeReader);

  // Description:
  // Specify file name of marching cubes output file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Set/get isosurface contour value.
  vtkSetMacro(Value,float);
  vtkGetMacro(Value,float);

  // Description:
  // Specify file name of marching cubes limits file. The limits file
  // speeds up subsequent reading of output triangle file.
  vtkSetStringMacro(LimitsFileName);
  vtkGetStringMacro(LimitsFileName);

protected:
  vtkSliceCubes();
  ~vtkSliceCubes();

  void Execute();

  vtkVolumeReader *Reader;
  char *FileName;  
  float Value;
  char *LimitsFileName;

private:
  vtkSliceCubes(const vtkSliceCubes&);  // Not implemented.
  void operator=(const vtkSliceCubes&);  // Not implemented.
};

#endif

