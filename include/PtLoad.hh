/*=========================================================================

  Program:   Visualization Library
  Module:    PtLoad.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPointLoad - compute stress tensors given point load on semi-inifinite domain
// .SECTION Description
// vtkPointLoad is a source object that computes stress tensors on a volume. 
// The tensors are computed from the application of a point load on a 
// semi-infinite domain. This object serves as a specialized data generator
// for some of the examples in the text.

#ifndef __vtkPointLoad_h
#define __vtkPointLoad_h

#include "SPtsSrc.hh"

class vtkPointLoad : public vtkStructuredPointsSource
{
public:
  vtkPointLoad();
  ~vtkPointLoad() {};
  char *GetClassName() {return "vtkPointLoad";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get value of applied load.
  vtkSetMacro(LoadValue,float);
  vtkGetMacro(LoadValue,float);

  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  void SetModelBounds(float *bounds);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // Set/Get Poisson's ratio.
  vtkSetMacro(PoissonsRatio,float);
  vtkGetMacro(PoissonsRatio,float);

protected:
  void Execute();

  float LoadValue;
  float PoissonsRatio;
  int SampleDimensions[3];
  float ModelBounds[6];

};

#endif


