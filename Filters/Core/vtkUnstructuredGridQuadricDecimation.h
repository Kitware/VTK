/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridQuadricDecimation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

  Copyright 2007, 2008 by University of Utah.

=========================================================================*/

/**
 * @class   vtkUnstructuredGridQuadricDecimation
 * @brief   reduce the number of
 * tetrahedra in a mesh
 *
 *
 *
 * vtkUnstructuredGridQuadricDecimation is a class that simplifies
 * tetrahedral meshes using randomized multiple choice edge
 * collapses. The input to this filter is a vtkUnstructuredGrid object
 * with a single scalar field (specifying in the ScalarsName
 * attribute). Users can determine the size of the output mesh by
 * either setting the value of TargetReduction or
 * NumberOfTetsOutput. The BoundaryWeight can be set to control how
 * well the mesh boundary should be preserved. The implementation uses
 * Michael Garland's generalized Quadric Error Metrics, the Corner
 * Table representation and the Standard Conjugate Gradient Method to
 * order the edge collapse sequence.
 *
 * Instead of using the traditional priority queue, the algorithm uses
 * a randomized approach to yield faster performance with comparable
 * quality. At each step, a set of 8 random candidate edges are
 * examined to select the best edge to collapse. This number can also
 * be changed by users through NumberOfCandidates.
 *
 * For more information as well as the streaming version of this
 * algorithm see:
 *
 * "Streaming Simplification of Tetrahedral Meshes" by H. T. Vo,
 * S. P. Callahan, P. Lindstrom, V. Pascucci and C. T. Silva, IEEE
 * Transactions on Visualization and Computer Graphics, 13(1):145-155,
 * 2007.
 *
 *
 * @par Acknowledgments:
 * This code was developed by Huy T. Vo under the supervision of
 * Prof. Claudio T. Silva. The code also contains contributions from
 * Peter Lindstrom and Steven P. Callahan.
 *
 * @par Acknowledgments:
 * The work was supported by grants, contracts, and gifts from the
 * National Science Foundation, the Department of Energy and IBM.
*/

#ifndef vtkUnstructuredGridQuadricDecimation_h
#define vtkUnstructuredGridQuadricDecimation_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkUnstructuredGridQuadricDecimation : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkUnstructuredGridQuadricDecimation, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkUnstructuredGridQuadricDecimation *New();

  // The following 3 parameters will control the process of simplification in
  // the priority:
  // NumberOfEdgesToDecimate, NumberOfTetsOutput, TargetReduction.
  // If NumberOfEdgesToDecimate is 0, NumberOfTetsOutput will be considered. If
  // NumbersOfTetsOutput is also 0, then TargetReduction will control the
  // output.

  //@{
  /**
   * Set/Get the desired reduction (expressed as a fraction of the original
   * number of tetrehedra)
   */
  vtkSetMacro(TargetReduction, double);
  vtkGetMacro(TargetReduction, double);
  //@}

  //@{
  /**
   * Set/Get the desired number of tetrahedra to be outputted
   */
  vtkSetMacro(NumberOfTetsOutput, int);
  vtkGetMacro(NumberOfTetsOutput, int);
  //@}

  //@{
  /**
   * Set/Get the desired number of edge to collapse
   */
  vtkSetMacro(NumberOfEdgesToDecimate, int);
  vtkGetMacro(NumberOfEdgesToDecimate, int);
  //@}

  //@{
  /**
   * Set/Get the number of candidates selected for each randomized set before
   * performing an edge collapse. Increasing this number can help producing
   * higher quality output but it will be slower. Default is 8.
   */
  vtkSetMacro(NumberOfCandidates, int);
  vtkGetMacro(NumberOfCandidates, int);
  //@}

  //@{
  /**
   * Enable(1)/Disable(0) the feature of temporarily doubling the number of
   * candidates for each randomized set if the quadric error was significantly
   * increased over the last edge collapse, i.e. if the ratio between the error
   * difference and the last error is over some threshold. Basically, we are
   * trying to make up for cases when random selection returns so many 'bad'
   * edges. By doing this we can achieve a higher quality output with much less
   * time than just double the NumberOfCandidates. Default is Enabled(1)
   */
  vtkSetMacro(AutoAddCandidates, int);
  vtkGetMacro(AutoAddCandidates, int);
  //@}

  //@{
  /**
   * Set/Get the threshold that decides when to double the set size.
   * Default is 0.4.
   */
  vtkSetMacro(AutoAddCandidatesThreshold, double);
  vtkGetMacro(AutoAddCandidatesThreshold, double);
  //@}

  //@{
  /**
   * Set/Get the weight of the boundary on the quadric metrics. The larger
   * the number, the better the boundary is preserved.
   */
  vtkSetMacro(BoundaryWeight, double);
  vtkGetMacro(BoundaryWeight, double);
  //@}

  //@{
  /**
   * Set/Get the scalar field name used for simplification
   */
  vtkSetStringMacro(ScalarsName);
  vtkGetStringMacro(ScalarsName);
  //@}

  enum
  {
    NO_ERROR=0,
    NON_TETRAHEDRA=1,
    NO_SCALARS=2,
    NO_CELLS=3
  };

protected:
  vtkUnstructuredGridQuadricDecimation();
  ~vtkUnstructuredGridQuadricDecimation() override;

  void ReportError(int err);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  int NumberOfTetsOutput;
  int NumberOfEdgesToDecimate;
  int NumberOfCandidates;
  int AutoAddCandidates;

  double TargetReduction;
  double AutoAddCandidatesThreshold;
  double BoundaryWeight;
  char *ScalarsName;

private:
  vtkUnstructuredGridQuadricDecimation(const vtkUnstructuredGridQuadricDecimation&) = delete;
  void operator=(const vtkUnstructuredGridQuadricDecimation&) = delete;

};

#endif
