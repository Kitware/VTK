/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeFieldAggregator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkTreeFieldAggregator - aggregate field values from the leaves up the tree
//
// .SECTION Description
// vtkTreeFieldAggregator may be used to assign sizes to all the vertices in the
// tree, based on the sizes of the leaves.  The size of a vertex will equal
// the sum of the sizes of the child vertices.  If you have a data array with
// values for all leaves, you may specify that array, and the values will
// be filled in for interior tree vertices.  If you do not yet have an array,
// you may tell the filter to create a new array, assuming that the size
// of each leaf vertex is 1.  You may optionally set a flag to first take the
// log of all leaf values before aggregating.

#ifndef vtkTreeFieldAggregator_h
#define vtkTreeFieldAggregator_h

class vtkPoints;
class vtkTree;

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkTreeFieldAggregator : public vtkTreeAlgorithm
{
public:
  static vtkTreeFieldAggregator *New();

  vtkTypeMacro(vtkTreeFieldAggregator,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The field to aggregate.  If this is a string array, the entries are converted to double.
  // TODO: Remove this field and use the ArrayToProcess in vtkAlgorithm.
  vtkGetStringMacro(Field);
  vtkSetStringMacro(Field);

  // Description:
  // If the value of the vertex is less than MinValue then consider it's value to be minVal.
  vtkGetMacro(MinValue, double);
  vtkSetMacro(MinValue, double);

  // Description:
  // If set, the algorithm will assume a size of 1 for each leaf vertex.
  vtkSetMacro(LeafVertexUnitSize, bool);
  vtkGetMacro(LeafVertexUnitSize, bool);
  vtkBooleanMacro(LeafVertexUnitSize, bool);

  // Description:
  // If set, the leaf values in the tree will be logarithmically scaled (base 10).
  vtkSetMacro(LogScale, bool);
  vtkGetMacro(LogScale, bool);
  vtkBooleanMacro(LogScale, bool);
protected:
  vtkTreeFieldAggregator();
  ~vtkTreeFieldAggregator();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  char* Field;
  bool LeafVertexUnitSize;
  bool LogScale;
  double MinValue;
  vtkTreeFieldAggregator(const vtkTreeFieldAggregator&);  // Not implemented.
  void operator=(const vtkTreeFieldAggregator&);  // Not implemented.
  double GetDoubleValue(vtkAbstractArray* arr, vtkIdType id);
  static void SetDoubleValue(vtkAbstractArray* arr, vtkIdType id, double value);
};

#endif
