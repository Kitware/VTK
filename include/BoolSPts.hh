/*=========================================================================

  Program:   Visualization Library
  Module:    BoolSPts.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#ifndef __vlBooleanStructuredPoints_h
#define __vlBooleanStructuredPoints_h

#include "Filter.hh"
#include "StrPts.hh"
#include "StrPtsC.hh"

#define UNION_OPERATOR 0
#define INTERSECTION_OPERATOR 1
#define DIFFERENCE_OPERATOR 2

class vlBooleanStructuredPoints : public vlStructuredPoints, public vlFilter
{
public:
  vlBooleanStructuredPoints();
  ~vlBooleanStructuredPoints();
  char *GetClassName() {return "vlBooleanStructuredPoints";};
  void PrintSelf(ostream& os, vlIndent indent);

  void AddInput(vlStructuredPoints *);
  void RemoveInput(vlStructuredPoints *);
  vlStructuredPointsCollection *GetInput() {return &(this->Input);};

  // filter interface
  void Update();

  // alternative method to boolean data
  void Append(vlStructuredPoints *);
  
  // Various operations
  vlSetClampMacro(OperationType,int,UNION_OPERATOR,DIFFERENCE_OPERATOR);
  vlGetMacro(OperationType,int);

  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vlGetVectorMacro(SampleDimensions,int);

  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vlGetVectorMacro(ModelBounds,float);

protected:
  // Usual data generation method
  void Execute();
  void InitializeBoolean();

  // list of data sets to append together
  vlStructuredPointsCollection Input;

  // pointer to operation function
  void (vlBooleanStructuredPoints::*Operator)();

  // boolean is performed on this resolution in this space
  int SampleDimensions[3];
  float ModelBounds[6];

  // various operations
  int OperationType;
  void Union();
  void Intersection();
  void Difference();
};

#endif
