/*=========================================================================

  Program:   Visualization Library
  Module:    BoolSPts.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlBooleanStructuredPoints - combine two or more structured point sets
// .SECTION Description
// vlBooleanStructuredPoints is a filter that performs boolean combinations on
// two or more input structured point sets. Operations supported include union,
// intersection, and difference. A special method is provided that allows 
// incremental appending of data to the filter output.

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

  void AddInput(vlStructuredPoints *in);
  void AddInput(vlStructuredPoints &in) {this->AddInput(&in);};
  void RemoveInput(vlStructuredPoints *in);
  void RemoveInput(vlStructuredPoints &in) {this->RemoveInput(&in);};
  vlStructuredPointsCollection *GetInput() {return &(this->InputList);};

  // filter interface
  unsigned long int GetMTime();
  void Update();

  // alternative method to boolean data
  void Append(vlStructuredPoints *);
  
  // Description:
  // Specify the type of boolean operation.
  vlSetClampMacro(OperationType,int,UNION_OPERATOR,DIFFERENCE_OPERATOR);
  vlGetMacro(OperationType,int);

  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vlGetVectorMacro(SampleDimensions,int,3);

  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vlGetVectorMacro(ModelBounds,float,6);

protected:
  // Usual data generation method
  void Execute();
  void InitializeBoolean();

  // list of data sets to append together
  vlStructuredPointsCollection InputList;

  // pointer to operation function
  //BTX
  void (vlBooleanStructuredPoints::*Operator)();
  //ETX

  // boolean is performed on this resolution in this space
  int SampleDimensions[3];
  float ModelBounds[6];

  // various operations
  int OperationType;
};

#endif
