/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointLoad.h
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
// .NAME vtkPointLoad - compute stress tensors given point load on semi-infinite domain
// .SECTION Description
// vtkPointLoad is a source object that computes stress tensors on a volume. 
// The tensors are computed from the application of a point load on a 
// semi-infinite domain. (The analytical results are adapted from Saada - see 
// text.) It also is possible to compute effective stress scalars if desired.
// This object serves as a specialized data generator for some of the examples
// in the text.
// .SECTION See Also
// vtkTensorGlyph, vtkHyperStreamline

#ifndef __vtkPointLoad_h
#define __vtkPointLoad_h

#include "vtkStructuredPointsSource.h"

class VTK_EXPORT vtkPointLoad :  public vtkStructuredPointsSource
{
public:
  vtkPointLoad();
  static vtkPointLoad *New() {return new vtkPointLoad;};
  char *GetClassName() {return "vtkPointLoad";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get value of applied load.
  vtkSetMacro(LoadValue,float);
  vtkGetMacro(LoadValue,float);

  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // Specify the region in space over which the tensors are computed. The point
  // load is assumed to be applied at top center of the volume.
  vtkSetVectorMacro(ModelBounds,float,6);
  vtkGetVectorMacro(ModelBounds,float,6);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, 
                      float zmin, float zmax);

  // Description:
  // Set/Get Poisson's ratio.
  vtkSetMacro(PoissonsRatio,float);
  vtkGetMacro(PoissonsRatio,float);

  // Description:
  // Turn on/off computation of effective stress scalar.
  vtkSetMacro(ComputeEffectiveStress,int);
  vtkGetMacro(ComputeEffectiveStress,int);
  vtkBooleanMacro(ComputeEffectiveStress,int);

protected:
  void Execute();

  float LoadValue;
  float PoissonsRatio;
  int SampleDimensions[3];
  float ModelBounds[6];
  int ComputeEffectiveStress;

};

#endif


