/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAreaContourSignatureFilter - compute an approximation of the area
// contour signature (evolution of the area of the input surface along an arc of
// the Reeb graph).
// .SECTION Description
// The filter takes a vtkPolyData as an input (port 0), along with a
// vtkReebGraph (port 1).
// The Reeb graph arc to consider can be specified with SetArcId() (default: 0).
// The number of (evenly distributed) samples of the signature can be defined
// with SetNumberOfSamples() (default value: 100).
// The filter will first try to pull as a scalar field the vtkDataArray with Id
// 'FieldId' of the vtkPolyData, see SetFieldId (default: 0). The filter will
// abort if this field does not exist.
//
// The filter outputs a vtkTable with the area contour signature
// approximation, each sample being evenly distributed in the function span of
// the arc.
//
// This filter is a typical example for designing your own contour signature
// filter (with customized metrics). It also shows typical vtkReebGraph
// traversals.

#ifndef __vtkAreaContourSignatureFilter_h
#define __vtkAreaContourSignatureFilter_h

#include  "vtkDataObjectAlgorithm.h"
#include  "vtkDoubleArray.h"
#include  "vtkTable.h"
#include  "vtkTriangle.h"
#include  "vtkReebGraph.h"

class VTK_GRAPHICS_EXPORT vtkAreaContourSignatureFilter :
  public vtkDataObjectAlgorithm
{
public:
  static vtkAreaContourSignatureFilter* New();
  vtkTypeRevisionMacro(vtkAreaContourSignatureFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the arc Id for which the contour signature has to be computed.
  // Default value: 0
  vtkSetMacro(ArcId, vtkIdType);
  vtkGetMacro(ArcId, vtkIdType);

  // Description:
  // Set the number of samples in the output signature
  // Default value: 100
  vtkSetMacro(NumberOfSamples, int);
  vtkGetMacro(NumberOfSamples, int);

  // Description:
  // Set the scalar field Id
  // Default value: 0
  vtkSetMacro(FieldId, vtkIdType);
  vtkGetMacro(FieldId, vtkIdType);

  vtkTable* GetOutput();

protected:
  vtkAreaContourSignatureFilter();
  ~vtkAreaContourSignatureFilter();

  vtkIdType ArcId, FieldId;
  int NumberOfSamples;

  int FillInputPortInformation(int portNumber, vtkInformation *);
  int FillOutputPortInformation(int portNumber, vtkInformation *info);

  int RequestData(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector);

private:
  vtkAreaContourSignatureFilter(
    const vtkAreaContourSignatureFilter&);
  // Not implemented.
  void operator=(const vtkAreaContourSignatureFilter&);
  // Not implemented.
};

#endif
