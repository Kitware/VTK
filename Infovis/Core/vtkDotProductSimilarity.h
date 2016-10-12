/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDotProductSimilarity.h

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

/**
 * @class   vtkDotProductSimilarity
 * @brief   compute dot-product similarity metrics.
 *
 *
 * Treats matrices as collections of vectors and computes dot-product similarity
 * metrics between vectors.
 *
 * The results are returned as an edge-table that lists the index of each vector
 * and their computed similarity.  The output edge-table is typically used with
 * vtkTableToGraph to create a similarity graph.
 *
 * This filter can be used with one or two input matrices.  If you provide a single
 * matrix as input, every vector in the matrix is compared with every other vector. If
 * you provide two matrices, every vector in the first matrix is compared with every
 * vector in the second matrix.
 *
 * Note that this filter *only* computes the dot-product between each pair of vectors;
 * if you want to compute the cosine of the angles between vectors, you will need to
 * normalize the inputs yourself.
 *
 * Inputs:
 *   Input port 0: (required) A vtkDenseArray<double> with two dimensions (a matrix).
 *   Input port 1: (optional) A vtkDenseArray<double> with two dimensions (a matrix).
 *
 * Outputs:
 *   Output port 0: A vtkTable containing "source", "target", and "similarity" columns.
 *
 * @warning
 * Note that the complexity of this filter is quadratic!  It also requires dense arrays
 * as input, in the future it should be generalized to accept sparse arrays.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkDotProductSimilarity_h
#define vtkDotProductSimilarity_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTableAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkDotProductSimilarity : public vtkTableAlgorithm
{
public:
  static vtkDotProductSimilarity* New();
  vtkTypeMacro(vtkDotProductSimilarity, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Controls whether to compute similarities for row-vectors or column-vectors.
   * 0 = rows, 1 = columns.
   */
  vtkGetMacro(VectorDimension, vtkIdType);
  vtkSetMacro(VectorDimension, vtkIdType);
  //@}

  //@{
  /**
   * When computing similarities for a single input matrix, controls whether the
   * results will include the upper diagonal of the similarity matrix.  Default: true.
   */
  vtkGetMacro(UpperDiagonal, int);
  vtkSetMacro(UpperDiagonal, int);
  //@}

  //@{
  /**
   * When computing similarities for a single input matrix, controls whether the
   * results will include the diagonal of the similarity matrix.  Default: false.
   */
  vtkGetMacro(Diagonal, int);
  vtkSetMacro(Diagonal, int);
  //@}

  //@{
  /**
   * When computing similarities for a single input matrix, controls whether the
   * results will include the lower diagonal of the similarity matrix.  Default: false.
   */
  vtkGetMacro(LowerDiagonal, int);
  vtkSetMacro(LowerDiagonal, int);
  //@}

  //@{
  /**
   * When computing similarities for two input matrices, controls whether the results
   * will include comparisons from the first matrix to the second matrix.
   */
  vtkGetMacro(FirstSecond, int);
  vtkSetMacro(FirstSecond, int);
  //@}

  //@{
  /**
   * When computing similarities for two input matrices, controls whether the results
   * will include comparisons from the second matrix to the first matrix.
   */
  vtkGetMacro(SecondFirst, int);
  vtkSetMacro(SecondFirst, int);
  //@}

  //@{
  /**
   * Specifies a minimum threshold that a similarity must exceed to be included in
   * the output.
   */
  vtkGetMacro(MinimumThreshold, double);
  vtkSetMacro(MinimumThreshold, double);
  //@}

  //@{
  /**
   * Specifies a minimum number of edges to include for each vector.
   */
  vtkGetMacro(MinimumCount, vtkIdType);
  vtkSetMacro(MinimumCount, vtkIdType);
  //@}

  //@{
  /**
   * Specifies a maximum number of edges to include for each vector.
   */
  vtkGetMacro(MaximumCount, vtkIdType);
  vtkSetMacro(MaximumCount, vtkIdType);
  //@}

protected:
  vtkDotProductSimilarity();
  ~vtkDotProductSimilarity();

  int FillInputPortInformation(int, vtkInformation*);

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkDotProductSimilarity(const vtkDotProductSimilarity&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDotProductSimilarity&) VTK_DELETE_FUNCTION;

  vtkIdType VectorDimension;
  double MinimumThreshold;
  vtkIdType MinimumCount;
  vtkIdType MaximumCount;

  int UpperDiagonal;
  int Diagonal;
  int LowerDiagonal;
  int FirstSecond;
  int SecondFirst;
};

#endif

