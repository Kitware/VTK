/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceCubes.h
  Language:  C++
  Date:      11 Sep 1995
  Version:   1.12


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

    THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 4,710,876
    "System and Method for the Display of Surface Structures Contained
    Within The Interior Region of a Solid body".
    Application of this software for commercial purposes requires 
    a license grant from GE. Contact:
        Jerald Roehling
        GE Licensing
        One Independence Way
        PO Box 2023
        Princeton, N.J. 08540
        phone 609-734-9823
        e-mail:Roehlinj@gerlmo.ge.com
    for more information.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

class VTK_EXPORT vtkSliceCubes : public vtkObject
{
public:
  static vtkSliceCubes *New();
  vtkTypeMacro(vtkSliceCubes,vtkObject);
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
  vtkSliceCubes(const vtkSliceCubes&);
  void operator=(const vtkSliceCubes&);

  void Execute();

  vtkVolumeReader *Reader;
  char *FileName;  
  float Value;
  char *LimitsFileName;
};

#endif

