// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStructuredDataPlaneCutter
 * @brief   fast plane cutting of vtkImageData/vtkRectilinearGrid/vtkStructuredGrid
 *
 * vtkStructuredDataPlaneCutter is a specialized filter that cuts an input
 * vtkImageData/vtkRectilinearGrid/vtkStructuredGrid. The filter is designed for
 * high speed.
 *
 * To use this filter you must specify an input vtkImageData/vtkRectilinearGrid/
 * vtkStructuredGrid and a plane to cut with.
 *
 * This algorithm is linear with regard to cells. Tt may build (in a preprocessing step)
 * a spatial search structure that accelerates the plane cuts. The search structure, which
 * is typically a sphere tree, is used to quickly cull candidate cells.
 *
 * @warning
 * This filter delegates to vtkFlyingEdgesPlaneCutter for vtkImageData if you DON'T
 * want to generate polygons.
 *
 * @warning
 * This class is templated. It may run slower than serial execution if the code
 * is not optimized during compilation. Build in Release or ReleaseWithDebugInfo.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkCutter vtkPlaneCutter vtkFlyingEdgesPlaneCutter vtk3DLinearGridPlaneCutter
 * vtkPolyDataPlaneCutter
 */

#ifndef vtkStructuredDataPlaneCutter_h
#define vtkStructuredDataPlaneCutter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkSphereTree;
class vtkPlane;

class VTKFILTERSCORE_EXPORT vtkStructuredDataPlaneCutter : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard construction and print methods.
   */
  static vtkStructuredDataPlaneCutter* New();
  vtkTypeMacro(vtkStructuredDataPlaneCutter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * The modified time depends on the delegated cut plane.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Specify the plane (an implicit function) to perform the cutting. The
   * definition of the plane (its origin and normal) is controlled via this
   * instance of vtkPlane.
   */
  virtual void SetPlane(vtkPlane*);
  vtkGetObjectMacro(Plane, vtkPlane);
  ///@}

  ///@{
  /**
   * Indicate whether to generate polygons instead of triangles
   *
   * Default is on.
   */
  vtkSetMacro(GeneratePolygons, bool);
  vtkGetMacro(GeneratePolygons, bool);
  vtkBooleanMacro(GeneratePolygons, bool);
  ///@}

  ///@{
  /**
   * Indicate whether to build the sphere tree. Computing the sphere
   * will take some time on the first computation
   * but if the input does not change, the computation of all further
   * slice will be much faster.
   *
   * Default is on.
   */
  vtkSetMacro(BuildTree, bool);
  vtkGetMacro(BuildTree, bool);
  vtkBooleanMacro(BuildTree, bool);
  ///@}

  ///@{
  /**
   * Indicate whether to build tree hierarchy. Computing the tree
   * hierarchy can take some time on the first computation but if
   * the input does not change, the computation of all further
   * slice will be faster.
   *
   * Default is on.
   */
  vtkSetMacro(BuildHierarchy, bool);
  vtkGetMacro(BuildHierarchy, bool);
  vtkBooleanMacro(BuildHierarchy, bool);
  ///@}

  ///@{
  /**
   * Specify the sphere tree object. If defined, the sphere tree will not be built
   * unless the dataset has changed.
   */
  virtual void SetSphereTree(vtkSphereTree*);
  vtkGetObjectMacro(SphereTree, vtkSphereTree);
  ///@}

  ///@{
  /**
   * Set/Get the computation of normals. The normal generated is simply the
   * cut plane normal.
   *
   * Default is off.
   */
  vtkSetMacro(ComputeNormals, bool);
  vtkGetMacro(ComputeNormals, bool);
  vtkBooleanMacro(ComputeNormals, bool);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  ///@{
  /**
   * Indicate whether to interpolate attribute data. Note that both cell data
   * and point data is interpolated and output
   *
   * Default is on.
   */
  vtkSetMacro(InterpolateAttributes, bool);
  vtkGetMacro(InterpolateAttributes, bool);
  vtkBooleanMacro(InterpolateAttributes, bool);
  ///@}

  ///@{
  /**
   * Specify the number of input cells in a batch, where a batch defines
   * a subset of the input cells operated on during threaded
   * execution. Generally this is only used for debugging or performance
   * studies (since batch size affects the thread workload).
   *
   * Default is 1000.
   */
  vtkSetClampMacro(BatchSize, unsigned int, 1, VTK_INT_MAX);
  vtkGetMacro(BatchSize, unsigned int);
  ///@}
protected:
  vtkStructuredDataPlaneCutter();
  ~vtkStructuredDataPlaneCutter() override;

  vtkPlane* Plane = nullptr;
  vtkSphereTree* SphereTree = nullptr;
  bool ComputeNormals = false;
  bool InterpolateAttributes = true;
  bool GeneratePolygons = true;
  bool BuildTree = true;
  bool BuildHierarchy = true;
  int OutputPointsPrecision = DEFAULT_PRECISION;
  unsigned int BatchSize = 1000;

  struct vtkInputInfo
  {
    vtkDataSet* Input;
    vtkMTimeType LastMTime;

    vtkInputInfo()
      : Input(nullptr)
      , LastMTime(0)
    {
    }
    vtkInputInfo(vtkDataSet* input, vtkMTimeType mtime)
      : Input(input)
      , LastMTime(mtime)
    {
    }
  };
  vtkInputInfo InputInfo;

  // Pipeline-related methods
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Garbage collection method
  void ReportReferences(vtkGarbageCollector*) override;

private:
  vtkStructuredDataPlaneCutter(const vtkStructuredDataPlaneCutter&) = delete;
  void operator=(const vtkStructuredDataPlaneCutter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
