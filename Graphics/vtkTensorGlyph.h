/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensorGlyph.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTensorGlyph - scale and orient glyph according to tensor eigenvalues and eigenvectors
// .SECTION Description
// vtkTensorGlyph is a filter that copies a geometric representation (specified
// as polygonal data) to every input point. The geometric representation, or 
// glyph, can be scaled and/or rotated according to the tensor at the input 
// point. Scaling and rotation is controlled by the eigenvalues/eigenvectors
// of the tensor as follows. For each tensor, the eigenvalues (and associated
// eigenvectors) are sorted to determine the major, medium, and minor 
// eigenvalues/eigenvectors. The major eigenvalue scales the glyph in the 
// x-direction, the medium in the y-direction, and the minor in the 
// z-direction. Then, the glyph is rotated so that the glyph's local x-axis 
// lies along the major eigenvector, y-axis along the medium eigenvector, and
// z-axis along the minor.
//
// A scale factor is provided to control the amount of scaling. Also, you 
// can turn off scaling completely if desired. The boolean variable 
// ClampScaling controls the maximum scaling (in conjunction with
// MaxScaleFactor.) This is useful in certain applications where 
// singularities or large order of magnitude differences exist in 
// the eigenvalues.
//
// Another instance variable, ExtractEigenvalues, has been provided to 
// control extraction of eigenvalues/eigenvectors. If this boolean is false,
// then eigenvalues/eigenvectors are not extracted, and the columns of the
// tensor are taken as the eigenvectors (norm of column is eigenvalue). 
// This allows additional capability over the vtkGlyph3D object. That is, the
// glyph can be oriented in three directions instead of one.

// .SECTION See Also
// vtkGlyph3D vtkPointLoad vtkHyperStreamline

#ifndef __vtkTensorGlyph_h
#define __vtkTensorGlyph_h

#include "vtkDataSetToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkTensorGlyph : public vtkDataSetToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkTensorGlyph,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct object with scaling on and scale factor 1.0. Eigenvalues are 
  // extracted, glyphs are colored with input scalar data, and logarithmic
  // scaling is turned off.
  static vtkTensorGlyph *New();

  // Description:
  // Specify the geometry to copy to each point.
  void SetSource(vtkPolyData *source);
  vtkPolyData *GetSource();

  // Description:
  // Turn on/off scaling of glyph with eigenvalues.
  vtkSetMacro(Scaling,int);
  vtkGetMacro(Scaling,int);
  vtkBooleanMacro(Scaling,int);

  // Description:
  // Specify scale factor to scale object by. (Scale factor always affects
  // output even if scaling is off.)
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Turn on/off extraction of eigenvalues from tensor.
  vtkSetMacro(ExtractEigenvalues,int);
  vtkBooleanMacro(ExtractEigenvalues,int);
  vtkGetMacro(ExtractEigenvalues,int);

  // Description:
  // Turn on/off coloring of glyph with input scalar data. If false, or 
  // input scalar data not present, then the scalars from the source
  // object are passed through the filter.
  vtkSetMacro(ColorGlyphs,int);
  vtkGetMacro(ColorGlyphs,int);
  vtkBooleanMacro(ColorGlyphs,int);

  // Description:
  // Turn on/off scalar clamping. If scalar clamping is on, the ivar
  // MaxScaleFactor is used to control the maximum scale factor. (This is
  // useful to prevent uncontrolled scaling near singularities.)
  vtkSetMacro(ClampScaling,int);
  vtkGetMacro(ClampScaling,int);
  vtkBooleanMacro(ClampScaling,int);

  // Description:
  // Set/Get the maximum allowable scale factor. This value is compared to the
  // combination of the scale factor times the eigenvalue. If less, the scale
  // factor is reset to the MaxScaleFactor. The boolean ClampScaling has to 
  // be "on" for this to work.
  vtkSetMacro(MaxScaleFactor,float);
  vtkGetMacro(MaxScaleFactor,float);

protected:
  vtkTensorGlyph();
  ~vtkTensorGlyph();

  void Execute();

  int Scaling; // Determine whether scaling of geometry is performed
  float ScaleFactor; // Scale factor to use to scale geometry
  int ExtractEigenvalues; // Boolean controls eigenfunction extraction
  int ColorGlyphs; // Boolean controls coloring with input scalar data
  int ClampScaling; // Boolean controls whether scaling is clamped.
  float MaxScaleFactor; // Maximum scale factor (ScaleFactor*eigenvalue)
private:
  vtkTensorGlyph(const vtkTensorGlyph&);  // Not implemented.
  void operator=(const vtkTensorGlyph&);  // Not implemented.
};

#endif
