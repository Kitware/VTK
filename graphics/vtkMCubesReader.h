/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMCubesReader.h
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
// .NAME vtkMCubesReader - read binary marching cubes file
// .SECTION Description
// vtkMCubesReader is a source object that reads binary marching cubes
// files. (Marching cubes is an isosurfacing technique that generates 
// many triangles.) The binary format is supported by W. Lorensen's
// marching cubes program (and the vtkSliceCubes object). The format 
// repeats point coordinates, so this object will merge the points 
// with a vtkLocator object. You can choose to supply the vtkLocator 
// or use the default.
// .SECTION Caveats
// Binary files assumed written in sun/hp/sgi (i.e., Big Endian) form.
//
// Because points are merged when read, degenerate triangles may be removed.
// Thus the number of triangles read may be fewer than the number of triangles
// actually created.
//
// The point merging does not take into account that the same point may have
// different normals. For example, running vtkPolyNormals after vtkContourFilter
// may split triangles because of the FeatureAngle ivar. Subsequent reading with
// vtkMCubesReader will merge the points and use the first point's normal. For the
// most part, this is undesirable.
//
// Normals are generated from the gradient of the data scalar values. Hence the 
// normals may on occasion point in a direction inconsistent with the ordering of 
// the triangle vertices. If this happens, the resulting surface may be "black".
// Reverse the sense of the FlipNormals boolean flag to correct this.

// .SECTION See Also
// vtkContourFilter vtkMarchingCubes vtkSliceCubes vtkLocator

#ifndef __vtkMCubesReader_h
#define __vtkMCubesReader_h

#include <stdio.h>
#include "vtkPolySource.h"
#include "vtkFloatPoints.h"
#include "vtkCellArray.h"

class VTK_EXPORT vtkMCubesReader : public vtkPolySource 
{
public:
  vtkMCubesReader();
  ~vtkMCubesReader();
  vtkMCubesReader *New() {return new vtkMCubesReader;};
  char *GetClassName() {return "vtkMCubesReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of marching cubes file.
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);
  void SetFileName(char *str){this->SetFilename(str);}
  char *GetFileName(){return this->GetFilename();}

  // Description:
  // Specify file name of marching cubes limits file.
  vtkSetStringMacro(LimitsFilename);
  vtkGetStringMacro(LimitsFilename);

  // Description:
  // Specify whether to flip normals in opposite direction. Flipping ONLY changes
  // the direction of the normal vector. Contrast this with flipping in
  // vtkPolyNormals which flips both the normal and the cell point order.
  vtkSetMacro(FlipNormals,int);
  vtkGetMacro(FlipNormals,int);
  vtkBooleanMacro(FlipNormals,int);

  // Description:
  // Specify whether to read normals.
  vtkSetMacro(Normals,int);
  vtkGetMacro(Normals,int);
  vtkBooleanMacro(Normals,int);

  void SetLocator(vtkPointLocator *locator);
  void SetLocator(vtkPointLocator& locator) {this->SetLocator(&locator);};
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();

protected:
  void Execute();

  char *Filename;
  char *LimitsFilename;
  vtkPointLocator *Locator;
  int SelfCreatedLocator;
  int FlipNormals;
  int Normals;

};

#endif


