/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlyph2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGlyph2D - copy oriented and scaled glyph geometry to every input point (2D specialization)
// .SECTION Description
// This subclass of vtkGlyph3D is a specialization to 2D. Transformations
// (i.e., translation, scaling, and rotation) are constrained to the plane.
// For example, rotations due to a vector are computed from the x-y
// coordinates of the vector only, and are assumed to occur around the
// z-axis. (See vtkGlyph3D for documentation on the interface to this
// class.)
//
// .SECTION See Also
// vtkTensorGlyph vtkGlyph3D vtkProgrammableGlyphFilter

#ifndef vtkGlyph2D_h
#define vtkGlyph2D_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkGlyph3D.h"

class VTKFILTERSCORE_EXPORT vtkGlyph2D : public vtkGlyph3D
{
public:
  vtkTypeMacro(vtkGlyph2D,vtkGlyph3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct object with scaling on, scaling mode is by scalar value,
  // scale factor = 1.0, the range is (0,1), orient geometry is on, and
  // orientation is by vector. Clamping and indexing are turned off. No
  // initial sources are defined.
  static vtkGlyph2D *New();

protected:
  vtkGlyph2D() {}
  ~vtkGlyph2D() {}

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkGlyph2D(const vtkGlyph2D&);  // Not implemented.
  void operator=(const vtkGlyph2D&);  // Not implemented.
};

#endif
