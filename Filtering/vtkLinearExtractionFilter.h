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
// .NAME vtkLinearExtractionFilter - select cells intersecting a line
//
// .SECTION Description
// This filter takes a vtkCompositeDataSet as it's input and a line segment as it's parameters. It outputs a vtkSelection identifying all the cells intersecting the given line segment.
//
// .SECTION Thanks
// This file has been initially developed in the frame of CEA's Love visualization software development <br>
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
// BP12, F-91297 Arpajon, France. <br>
// Initial implementation by Thierry Carrard and Charles Pignerol, CEA.
// Modifed and ported by Philippe Pébay, Kitware, 2011

#ifndef VTK_LINEAR_EXTRACTION_FILTER_H
#define VTK_LINEAR_EXTRACTION_FILTER_H

#include <vtkSelectionAlgorithm.h>
#include <vtkSystemIncludes.h>

class vtkDataSet;
class vtkIdTypeArray;
class vtkDoubleArray;
class vtkAlgorithmOutput;

class VTK_EXPORT vtkLinearExtractionFilter: public vtkSelectionAlgorithm
{
 public:

  static vtkLinearExtractionFilter *New();
  vtkTypeRevisionMacro(vtkLinearExtractionFilter,vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Tolerance
  vtkSetMacro(Tolerance,double);
  vtkGetMacro(Tolerance,double);

  // Description:
  // Starting point
  vtkSetVector3Macro(StartPoint,double);
  vtkGetVectorMacro(StartPoint,double,3);

  // Description:
  // End point
  vtkSetVector3Macro(EndPoint,double);
  vtkGetVectorMacro(EndPoint,double,3);

 protected:

  vtkLinearExtractionFilter();
  virtual ~vtkLinearExtractionFilter();

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  void RequestDataInternal(vtkDataSet* input, vtkIdTypeArray* outIndices);

 private:

  vtkLinearExtractionFilter(const vtkLinearExtractionFilter&);  // Not implemented
  vtkLinearExtractionFilter& operator =(const vtkLinearExtractionFilter&); // Not implemented

  double StartPoint[3];
  double EndPoint[3];
  double Tolerance;
}; // class vtkLinearExtractionFilter


#endif	// VTK_LINEAR_EXTRACTION_FILTER_H
