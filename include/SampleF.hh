/*=========================================================================

  Program:   Visualization Library
  Module:    SampleF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlSampleFunction - sample an implicit function over a structured point set
// .SECTION Description
// vlSampleFunction is a source object that evaluates an implicit function
// and normals at each point in a vlStructuredPointSet. The user can 
// specify the sample dimensions and location in space to perform the
// sampling. To create closed surfaces (in conjunction with the 
// vlContourFilter), capping can be turned on to set a particular 
// value on the boundaries of the sample space.

#ifndef __vlSampleFunction_h
#define __vlSampleFunction_h

#include "SPtsSrc.hh"
#include "ImpFunc.hh"

class vlSampleFunction : public vlStructuredPointsSource
{
public:
  vlSampleFunction();
  ~vlSampleFunction() {};
  char *GetClassName() {return "vlSampleFunction";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify the implicit function to use to generate data.
  vlSetObjectMacro(ImplicitFunction,vlImplicitFunction);
  vlGetObjectMacro(ImplicitFunction,vlImplicitFunction);

  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vlGetVectorMacro(SampleDimensions,int);

  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  void SetModelBounds(float *bounds);
  vlGetVectorMacro(ModelBounds,float);

  // Description:
  // Turn on/off capping. If capping is on, then the outer boundaries of the
  // structured point set are set to cap value. This can be used to insure
  // surfaces are closed.
  vlSetMacro(Capping,int);
  vlGetMacro(Capping,int);
  vlBooleanMacro(Capping,int);
  
  // Description:
  // Set the cap value.
  vlSetMacro(CapValue,float);
  vlGetMacro(CapValue,float);

  // Description:
  // Turn on/off the computation of normals.
  vlSetMacro(ComputeNormals,int);
  vlGetMacro(ComputeNormals,int);
  vlBooleanMacro(ComputeNormals,int);

  unsigned long int GetMTime();

protected:
  void Execute();
  void Cap(vlFloatScalars *s);

  int SampleDimensions[3];
  float ModelBounds[6];
  int Capping;
  float CapValue;
  vlImplicitFunction *ImplicitFunction;
  int ComputeNormals;
};

#endif


