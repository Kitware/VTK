/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosineSimilarity.h
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkCosineSimilarity_h
#define __vtkCosineSimilarity_h

#include "vtkTableAlgorithm.h"

// .NAME vtkCosineSimilarity - compute vector similarity metrics for a matrix.

// .SECTION Description
// Treats a matrix as a collection of vectors and computes the dot-product 
// between each pair of vectors, returning the results as an edge-table that
// includes the index of each vector with their computed similarity.  The
// output edge-table is typically used with vtkTableToGraph to create a similarity
// graph.
//
// Note: this filter *only* computes the dot-product between each pair of vectors -
// you must normalize the inputs to this filter if you want the results to be equal
// to the cosine of the angle between the vectors.
//
// .SECTION Caveats
// Note that the complexity of this filter is quadratic!
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_INFOVIS_EXPORT vtkCosineSimilarity : public vtkTableAlgorithm
{
public:
  static vtkCosineSimilarity* New();
  vtkTypeRevisionMacro(vtkCosineSimilarity, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls whether to compute similarities for row-vectors or column-vectors.
  // 0 = rows, 1 = columns.
  vtkGetMacro(VectorDimension, int);
  vtkSetMacro(VectorDimension, int);

  // Description:
  // Specifies a minimum threshold that a similarity must exceed to be included in
  // the output.
  vtkGetMacro(MinimumThreshold, double);
  vtkSetMacro(MinimumThreshold, double);

  // Description:
  // Specifies a minimum number of edges to include for each vector.
  vtkGetMacro(MinimumCount, int);
  vtkSetMacro(MinimumCount, int);

  // Description:
  // Specifies a maximum number of edges to include for each vector.
  vtkGetMacro(MaximumCount, int);
  vtkSetMacro(MaximumCount, int);

protected:
  vtkCosineSimilarity();
  ~vtkCosineSimilarity();

  int FillInputPortInformation(int, vtkInformation*);

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkCosineSimilarity(const vtkCosineSimilarity&); // Not implemented
  void operator=(const vtkCosineSimilarity&);   // Not implemented

  int VectorDimension;
  double MinimumThreshold;
  int MinimumCount;
  int MaximumCount;
};

#endif

