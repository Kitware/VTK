/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensorGlyph.hh
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

#include "vtkDataSetToPolyFilter.hh"

class vtkTensorGlyph : public vtkDataSetToPolyFilter
{
public:
  vtkTensorGlyph();
  char *GetClassName() {return "vtkTensorGlyph";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void Update();

  // Description:
  // Specify the geometry to copy to each point.
  vtkSetObjectMacro(Source,vtkPolyData);
  vtkGetObjectMacro(Source,vtkPolyData);

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
  void Execute();

  vtkPolyData *Source; // Geometry to copy to each point
  int Scaling; // Determine whether scaling of geometry is performed
  float ScaleFactor; // Scale factor to use to scale geometry
  int ExtractEigenvalues; // Boolean controls eigenfunction extraction
  int ColorGlyphs; // Boolean controls coloring with input scalar data
  int ClampScaling; // Boolean controls whether scaling is clamped.
  float MaxScaleFactor; // Maximum scale factor (ScaleFactor*eigenvalue)
};

#endif
