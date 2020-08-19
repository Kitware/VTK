/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorFieldTopology.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVectorFieldTopology
 * @brief   Extract the topological skeleton as output datasets
 *
 * vtkVectorFieldTopology is a filter that extracts the critical points and the 1D separatrices
 * (lines) If the data is 3D and the user enables ComputeSurfaces, also the 2D separatrices are
 * computed (surfaces)
 *
 * @par Thanks:
 * Developed by Roxana Bujack and Karen Tsai at Los Alamos National Laboratory under LDRD 20190143ER
 */

#ifndef vtkVectorFieldTopology_h
#define vtkVectorFieldTopology_h

#include <vtkFiltersFlowPathsModule.h> // For export macro
#include <vtkPolyDataAlgorithm.h>
#include <vtkStreamTracer.h>

class vtkGradientFilter;
class vtkImageData;
class vtkPolyData;
class vtkStreamSurface;
class vtkUnstructuredGrid;

class VTKFILTERSFLOWPATHS_EXPORT vtkVectorFieldTopology : public vtkPolyDataAlgorithm
{
public:
  static vtkVectorFieldTopology* New();
  vtkTypeMacro(vtkVectorFieldTopology, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify a uniform integration step unit for MinimumIntegrationStep,
   * InitialIntegrationStep, and MaximumIntegrationStep.
   * 1 = LENGTH_UNIT, i.e. all sizes are expresed in coordinate scale or cell scale
   * 2 = CELL_LENGTH_UNIT, i.e. all sizes are expresed in cell scale
   */
  vtkSetMacro(IntegrationStepUnit, int);
  vtkGetMacro(IntegrationStepUnit, int);
  //@}

  //@{
  /**
   * Specify/see the maximal number of iterations in this class and in vtkStreamTracer
   */
  vtkSetMacro(MaxNumSteps, int);
  vtkGetMacro(MaxNumSteps, int);
  //@}

  //@{
  /**
   * Specify the Initial, minimum, and maximum step size used for line integration,
   * expressed in IntegrationStepUnit
   */
  vtkSetMacro(IntegrationStepSize, double);
  vtkGetMacro(IntegrationStepSize, double);
  //@}

  //@{
  /**
   * Specify/see the distance by which the seedpoints of the separatrices are placed away from the
   * saddle expressed in IntegrationStepUnit
   */
  vtkSetMacro(SeparatrixDistance, double);
  vtkGetMacro(SeparatrixDistance, double);
  //@}

  //@{
  /**
   * Specify/see if the simple (fast) or iterative (correct) version is called
   */
  vtkSetMacro(UseIterativeSeeding, bool);
  vtkGetMacro(UseIterativeSeeding, bool);
  //@}

  //@{
  /**
   * Specify/see if the separatring surfaces (separatrices in 3D) are computed or not
   */
  vtkSetMacro(ComputeSurfaces, bool);
  vtkGetMacro(ComputeSurfaces, bool);
  //@}

  //@{
  /**
   * Specify/see if the boundary cells are treated or not
   */
  vtkSetMacro(ExcludeBoundary, bool);
  vtkGetMacro(ExcludeBoundary, bool);
  //@}

protected:
  vtkVectorFieldTopology();
  ~vtkVectorFieldTopology() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkVectorFieldTopology(const vtkVectorFieldTopology&) = delete;
  void operator=(const vtkVectorFieldTopology&) = delete;

  /**
   * main function if input is vtkImageData
   * triangulate, compute critical points, separatrices, and surfaces
   * @param dataset: the input dataset
   * @param tridataset: the triangulated version of the input dataset
   * @return 1 if successfully terminated
   */
  int ImageDataPrepare(vtkDataSet* dataSetInput, vtkUnstructuredGrid* tridataset);

  /**
   * main function if input is vtkUnstructuredGrid
   * triangulate if necessary, compute critical points, separatrices, and surfaces
   * @param dataset: the input dataset
   * @param tridataset: the triangulated version of the input dataset
   * @return 1 if successfully terminated
   */
  int UnstructuredGridPrepare(vtkDataSet* dataSetInput, vtkUnstructuredGrid* tridataset);

  /**
   * delete the cells that touch the boundary
   * @param tridataset: input vector field after triangulation
   * @return 1 if successful, 0 if not
   */
  int RemoveBoundary(vtkSmartPointer<vtkUnstructuredGrid> tridataset);

  /**
   * for each triangle, we solve the linear vector field analytically for its zeros
   *  if this location is inside the triangle, we have found a critical point
   * @param criticalPoints: list of the locations where the vf is zero
   * @param tridataset: input vector field after triangulation
   * @return 1 if successful, 0 if not
   */
  int ComputeCriticalPoints2D(
    vtkSmartPointer<vtkPolyData> criticalPoints, vtkSmartPointer<vtkUnstructuredGrid> tridataset);

  /**
   * for each tetrahedron, we solve the linear vector field analytically for its zeros
   *  if this location is inside the tetrahedron, we have found a critical point
   * @param criticalPoints: list of the locations where the vf is zero
   * @param tridataset: input vector field after tetrahedrization
   * @return 1 if successfully terminated
   */
  int ComputeCriticalPoints3D(
    vtkSmartPointer<vtkPolyData> criticalPoints, vtkSmartPointer<vtkUnstructuredGrid> tridataset);

  /**
   * we classify the critical points based on the eigenvalues of the jacobian
   * for the saddles, we seed in an offset of dist and integrate
   * @param criticalPoints: list of the locations where the vf is zero
   * @param separatrices: inegration lines starting at saddles
   * @param surfaces: inegration surfaces starting at saddles
   * @param dataset: input vector field
   * @param integrationStepUnit: whether the sizes are expresed in coordinate scale or cell scale
   * @param dist: size of the offset of the seeding
   * @param stepSize: stepsize of the integrator
   * @param maxNumSteps: maximal number of integration steps
   * @param computeSurfaces: depending on this boolen the separatring surfaces are computed or not
   * @param useIterativeSeeding: depending on this boolen the separatring surfaces  are computed
   * either good or fast
   * @return 1 if successfully terminated
   */
  int ComputeSeparatrices(vtkSmartPointer<vtkPolyData> criticalPoints,
    vtkSmartPointer<vtkPolyData> separatrices, vtkSmartPointer<vtkPolyData> surfaces,
    vtkSmartPointer<vtkDataSet> dataset, int integrationStepUnit, double dist, double stepSize,
    int maxNumSteps, bool computeSurfaces, bool useIterativeSeeding);

  /**
   * this method computes streamsurfaces
   * in the plane of the two eigenvectors of the same sign around saddles
   * @param isBackward: is 1 if the integration direction is backward and 0 for forward
   * @param normal: direction along the one eigenvector with opposite sign
   * @param zeroPos: location of the saddle
   * @param streamSurfaces: surfaces that have so far been computed
   * @param dataset: the vector field in which we advect
   * @param integrationStepUnit: whether the sizes are expresed in coordinate scale or cell scale
   * @param dist: size of the offset of the seeding
   * @param stepSize: stepsize of the integrator
   * @param maxNumSteps: maximal number of integration steps
   * @param useIterativeSeeding: depending on this boolen the separatring surfaces  are computed
   * either good or fast
   * @return 1 if successful, 0 if empty
   */
  int ComputeSurface(int numberOfSeparatingSurfaces, bool isBackward, double normal[3],
    double zeroPos[3], vtkSmartPointer<vtkPolyData> streamSurfaces,
    vtkSmartPointer<vtkDataSet> dataset, int integrationStepUnit, double dist, double stepSize,
    int maxNumSteps, bool useIterativeSeeding);

  /**
   * simple type that corresponds to the number of positive eigenvalues
   * in analogy to ttk, where the type corresponds to the down directions
   */
  enum CriticalType3D
  {
    DEGENERATE3D = -1,
    SINK3D = 0,
    SADDLE13D = 1,
    SADDLE23D = 2,
    SOURCE3D = 3,
    CENTER3D = 4
  };

  /**
   * simple type that corresponds to the number of positive eigenvalues
   * in analogy to ttk, where the type corresponds to the down directions
   */
  enum CriticalType2D
  {
    DEGENERATE2D = -1,
    SINK2D = 0,
    SADDLE2D = 1,
    SOURCE2D = 2,
    CENTER2D = 3
  };

  /**
   * determine which type of critical point we have based on the eigenvalues of the Jacobian in 3D
   * @param countReal: number of real valued eigenvalues
   * @param countReal: number of complex valued eigenvalues
   * @param countPos: number of positive eigenvalues
   * @param countNeg: number of negative eigenvalues
   * @return type of critical point: SOURCE3D 3, SADDLE23D 2, SADDLE13D 1, SINK3D 0, (CENTER3D 4)
   */
  static int Classify3D(int countReal, int countComplex, int countPos, int countNeg);

  /**
   * determine which type of critical point we have based on the eigenvalues of the Jacobian in 2D
   * @param countReal: number of real valued eigenvalues
   * @param countReal: number of complex valued eigenvalues
   * @param countPos: number of positive eigenvalues
   * @param countNeg: number of negative eigenvalues
   * @return type of critical point: SOURCE2D 2, SADDLE2D 1, SINK2D 0, (CENTER2D 3)
   */
  static int Classify2D(int countReal, int countComplex, int countPos, int countNeg);

  /**
   * number of iterations in this class and in vtkStreamTracer
   */
  int MaxNumSteps = 100;

  /**
   * this value is used as stepsize for the integration
   */
  double IntegrationStepSize = 1;

  /**
   * the separatrices are seeded with this offset from the critical points
   */
  double SeparatrixDistance = 1;

  /**
   * depending on this boolen the simple (fast) or iterative (correct) version is called
   */
  bool UseIterativeSeeding = false;

  /**
   * depending on this boolen the separatring surfaces (separatrices in 3D) are computed or not
   */
  bool ComputeSurfaces = false;

  /**
   * depending on this boolen the cells touching the boundary of the input dataset are treated or
   * not this prevents detection of the whole boundary in no slip boundary settings
   */
  bool ExcludeBoundary = true;

  /**
   * dimension of the input data: 2 or 3
   */
  int Dimension = 2;

  /**
   * Analogous to integration step unit in vtkStreamTracer
   * Specify a uniform integration step unit for MinimumIntegrationStep,
   * InitialIntegrationStep, and MaximumIntegrationStep. NOTE: The valid
   * unit is now limited to only LENGTH_UNIT (1) and CELL_LENGTH_UNIT (2),
   * EXCLUDING the previously-supported TIME_UNIT.
   */
  int IntegrationStepUnit = vtkStreamTracer::CELL_LENGTH_UNIT;

  vtkNew<vtkStreamSurface> StreamSurface;
  vtkNew<vtkGradientFilter> GradientFilter;
};
#endif
