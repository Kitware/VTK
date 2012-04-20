/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLYReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPLYReader - read Stanford University PLY polygonal file format
// .SECTION Description
// vtkPLYReader is a source object that reads polygonal data in
// Stanford University PLY file format (see
// http://graphics.stanford.edu/data/3Dscanrep). It requires that
// the elements "vertex" and "face" are defined. The "vertex" element
// must have the properties "x", "y", and "z". The "face" element must
// have the property "vertex_indices" defined. Optionally, if the "face"
// element has the properties "intensity" and/or the triplet "red",
// "green", and "blue"; these are read and added as scalars to the
// output data.

// .SECTION See Also
// vtkPLYWriter

#ifndef __vtkPLYReader_h
#define __vtkPLYReader_h

#include "vtkIOPLYModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKIOPLY_EXPORT vtkPLYReader : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkPLYReader,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with merging set to true.
  static vtkPLYReader *New();

  // Description:
  // Specify file name of stereo lithography file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // A simple, non-exhaustive check to see if a file is a valid ply file.
  static int CanReadFile(const char *filename);

protected:
  vtkPLYReader();
  ~vtkPLYReader();

  char *FileName;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkPLYReader(const vtkPLYReader&);  // Not implemented.
  void operator=(const vtkPLYReader&);  // Not implemented.
};

#endif
