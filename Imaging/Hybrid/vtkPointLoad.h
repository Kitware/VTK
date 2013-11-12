/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointLoad.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

#include "vtkImagingHybridModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKIMAGINGHYBRID_EXPORT vtkPointLoad :  public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkPointLoad,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with ModelBounds=(-1,1,-1,1,-1,1), SampleDimensions=(50,50,50),
  // and LoadValue = 1.
  static vtkPointLoad *New();

  // Description:
  // Set/Get value of applied load.
  vtkSetMacro(LoadValue,double);
  vtkGetMacro(LoadValue,double);

  // Description:
  // Specify the dimensions of the volume. A stress tensor will be computed for
  // each point in the volume.
  void SetSampleDimensions(int i, int j, int k);

  // Description:
  // Specify the dimensions of the volume. A stress tensor will be computed for
  // each point in the volume.
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // Specify the region in space over which the tensors are computed. The point
  // load is assumed to be applied at top center of the volume.
  vtkSetVector6Macro(ModelBounds,double);
  vtkGetVectorMacro(ModelBounds,double,6);

  // Description:
  // Set/Get Poisson's ratio.
  vtkSetMacro(PoissonsRatio,double);
  vtkGetMacro(PoissonsRatio,double);

  // Description:
  // Turn on/off computation of effective stress scalar. These methods do
  // nothing. The effective stress is always computed.
  void SetComputeEffectiveStress(int) {}
  int GetComputeEffectiveStress() {return 1;};
  void ComputeEffectiveStressOn() {}
  void ComputeEffectiveStressOff() {}

protected:
  vtkPointLoad();
  ~vtkPointLoad() {}

  virtual int RequestInformation (vtkInformation *,
                                   vtkInformationVector **,
                                   vtkInformationVector *);
  virtual void ExecuteDataWithInformation(vtkDataObject *, vtkInformation *);

  double LoadValue;
  double PoissonsRatio;
  int SampleDimensions[3];
  double ModelBounds[6];

private:
  vtkPointLoad(const vtkPointLoad&);  // Not implemented.
  void operator=(const vtkPointLoad&);  // Not implemented.
};

#endif


