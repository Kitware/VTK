/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SampleF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkSampleFunction - sample an implicit function over a structured point set
// .SECTION Description
// vtkSampleFunction is a source object that evaluates an implicit function
// and normals at each point in a vtkStructuredPointSet. The user can 
// specify the sample dimensions and location in space to perform the
// sampling. To create closed surfaces (in conjunction with the 
// vtkContourFilter), capping can be turned on to set a particular 
// value on the boundaries of the sample space.

#ifndef __vtkSampleFunction_h
#define __vtkSampleFunction_h

#include "SPtsSrc.hh"
#include "ImpFunc.hh"

class vtkSampleFunction : public vtkStructuredPointsSource
{
public:
  vtkSampleFunction();
  ~vtkSampleFunction() {};
  char *GetClassName() {return "vtkSampleFunction";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the implicit function to use to generate data.
  vtkSetObjectMacro(ImplicitFunction,vtkImplicitFunction);
  vtkGetObjectMacro(ImplicitFunction,vtkImplicitFunction);

  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  void SetModelBounds(float *bounds);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // Turn on/off capping. If capping is on, then the outer boundaries of the
  // structured point set are set to cap value. This can be used to insure
  // surfaces are closed.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);
  
  // Description:
  // Set the cap value.
  vtkSetMacro(CapValue,float);
  vtkGetMacro(CapValue,float);

  // Description:
  // Turn on/off the computation of normals.
  vtkSetMacro(ComputeNormals,int);
  vtkGetMacro(ComputeNormals,int);
  vtkBooleanMacro(ComputeNormals,int);

  unsigned long int GetMTime();

protected:
  void Execute();
  void Cap(vtkFloatScalars *s);

  int SampleDimensions[3];
  float ModelBounds[6];
  int Capping;
  float CapValue;
  vtkImplicitFunction *ImplicitFunction;
  int ComputeNormals;
};

#endif


