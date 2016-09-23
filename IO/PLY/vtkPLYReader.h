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
/**
 * @class   vtkPLYReader
 * @brief   read Stanford University PLY polygonal file format
 *
 * vtkPLYReader is a source object that reads polygonal data in
 * Stanford University PLY file format (see
 * http://graphics.stanford.edu/data/3Dscanrep). It requires that
 * the elements "vertex" and "face" are defined. The "vertex" element
 * must have the properties "x", "y", and "z". The "face" element must
 * have the property "vertex_indices" defined. Optionally, if the "face"
 * element has the properties "intensity" and/or the triplet "red",
 * "green", and "blue"; these are read and added as scalars to the
 * output data.
 *
 * @sa
 * vtkPLYWriter
*/

#ifndef vtkPLYReader_h
#define vtkPLYReader_h

#include "vtkIOPLYModule.h" // For export macro
#include "vtkAbstractPolyDataReader.h"

class VTKIOPLY_EXPORT vtkPLYReader : public vtkAbstractPolyDataReader
{
public:
  vtkTypeMacro(vtkPLYReader,vtkAbstractPolyDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Construct object with merging set to true.
   */
  static vtkPLYReader *New();

  /**
   * A simple, non-exhaustive check to see if a file is a valid ply file.
   */
  static int CanReadFile(const char *filename);

protected:
  vtkPLYReader();
  ~vtkPLYReader();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkPLYReader(const vtkPLYReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPLYReader&) VTK_DELETE_FUNCTION;
};

#endif
