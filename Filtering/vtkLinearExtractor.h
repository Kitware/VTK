/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile: vtkCellDistanceFilter,v $

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLinearExtractor - select cells intersecting a line
//
// .SECTION Description
// This filter takes a vtkCompositeDataSet as it's input and a line segment as it's parameters. It outputs a vtkSelection identifying all the cells intersecting the given line segment.
//
// .SECTION Thanks
// This file has been initially developed in the frame of CEA's Love visualization software development <br>
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
// BP12, F-91297 Arpajon, France. <br>
// This class was implemented by Thierry Carrard, Charles Pignerol, and Philippe Pébay, Kitware, 2011.

#ifndef VTK_LINEAR_EXTRACTOR_H
#define VTK_LINEAR_EXTRACTOR_H

#include <vtkSelectionAlgorithm.h>
#include <vtkSystemIncludes.h>

class vtkDataSet;
class vtkIdTypeArray;
class vtkDoubleArray;
class vtkAlgorithmOutput;

class VTK_EXPORT vtkLinearExtractor: public vtkSelectionAlgorithm
{
 public:
  static vtkLinearExtractor *New();
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Tolerance to be used by intersection algorithm
  vtkSetMacro(Tolerance,double);
  vtkGetMacro(Tolerance,double);

  // Description:
  // Starting point of segment
  vtkSetVector3Macro(StartPoint,double);
  vtkGetVectorMacro(StartPoint,double,3);

  // Description:
  // End point of segment
  vtkSetVector3Macro(EndPoint,double);
  vtkGetVectorMacro(EndPoint,double,3);

 protected:
  vtkLinearExtractor();
  virtual ~vtkLinearExtractor();

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  void RequestDataInternal(vtkDataSet* input, vtkIdTypeArray* outIndices);

 private:
  vtkLinearExtractor(const vtkLinearExtractor&);  // Not implemented
  vtkLinearExtractor& operator =(const vtkLinearExtractor&); // Not implemented

  double StartPoint[3];
  double EndPoint[3];
  double Tolerance;
};


#endif	// VTK_LINEAR_EXTRACTOR_H
