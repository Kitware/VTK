/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoolSPts.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkBooleanStructuredPoints - combine two or more structured point sets
// .SECTION Description
// vtkBooleanStructuredPoints is a filter that performs boolean combinations on
// two or more input structured point sets. Operations supported include union,
// intersection, and difference. A special method is provided that allows 
// incremental appending of data to the filter output.

#ifndef __vtkBooleanStructuredPoints_h
#define __vtkBooleanStructuredPoints_h

#include "Filter.hh"
#include "StrPts.hh"
#include "StrPtsC.hh"

#define UNION_OPERATOR 0
#define INTERSECTION_OPERATOR 1
#define DIFFERENCE_OPERATOR 2

class vtkBooleanStructuredPoints : public vtkStructuredPoints, public vtkFilter
{
public:
  vtkBooleanStructuredPoints();
  ~vtkBooleanStructuredPoints();
  char *GetClassName() {return "vtkBooleanStructuredPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddInput(vtkStructuredPoints *in);
  void AddInput(vtkStructuredPoints &in) {this->AddInput(&in);};
  void RemoveInput(vtkStructuredPoints *in);
  void RemoveInput(vtkStructuredPoints &in) {this->RemoveInput(&in);};
  vtkStructuredPointsCollection *GetInput() {return &(this->InputList);};

  // filter interface
  unsigned long int GetMTime();
  void Update();

  // alternative method to boolean data
  void Append(vtkStructuredPoints *);
  
  // Description:
  // Specify the type of boolean operation.
  vtkSetClampMacro(OperationType,int,UNION_OPERATOR,DIFFERENCE_OPERATOR);
  vtkGetMacro(OperationType,int);

  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vtkGetVectorMacro(ModelBounds,float,6);

protected:
  // Usual data generation method
  void Execute();

  // Filter interface
  int GetDataReleased();
  void SetDataReleased(int flag);

  void InitializeBoolean();

  // list of data sets to append together
  vtkStructuredPointsCollection InputList;

  // pointer to operation function
  //BTX
  void (vtkBooleanStructuredPoints::*Operator)();
  //ETX

  // boolean is performed on this resolution in this space
  int SampleDimensions[3];
  float ModelBounds[6];

  // various operations
  int OperationType;
};

#endif
