/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaneCutter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPlaneCutter
 * @brief   cut any dataset with a plane and generate a
 * polygonal cut surface
 *
 * vtkPlaneCutter is a specialization of the vtkCutter algorithm to cut a
 * dataset grid with a single plane. It is designed for performance and an
 * exploratory, fast workflow. It produces output polygons that result from
 * cutting the icnput dataset with the specified plane.
 *
 * This algorithm is fast because it is threaded, and may build (in a
 * preprocessing step) a spatial search structure that accelerates the plane
 * cuts. The search structure, which is typically a sphere tree, is used to
 * quickly cull candidate cells. Also unlike vtkCutter, the vtkPlane implicit
 * function (representing the plane) does not need to be evaluated with each
 * cut. (Note that other methods of acceleration are delegated to for image
 * data, see vtkFlyingEdgesPlaneCutter documentation.)
 *
 * Because this filter builds an initial data structure during a
 * preprocessing step, the first execution of the filter may take longer than
 * subsequent operations. Typically the first execution is still faster than
 * vtkCutter (especially with threading enabled), but for certain types of
 * data this may not be true. However if you are using the filter to cut a
 * dataset multiple times (as in an exploratory or interactive workflow) this
 * filter works well.
 *
 * @warning
 * This filter outputs a vtkMultiPieceDataSet.
 *
 * @warning
 * This filter delegates to vtkFlyingEdgesPlaneCutter to process image
 * data, but output and input have been standardized when possible.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkCutter vtkFlyingEdgesPlaneCutter vtkPlane
*/

#ifndef vtkPlaneCutter_h
#define vtkPlaneCutter_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkSmartPointer.h" // For SmartPointer
#include <vector> // For vector

class vtkCellArray;
class vtkCellData;
class vtkImageData;
class vtkMultiPieceDataSet;
class vtkPlane;
class vtkPointData;
class vtkPoints;
class vtkSphereTree;
class vtkStructuredGrid;
class vtkUnstructuredGrid;

class VTKFILTERSCORE_EXPORT vtkPlaneCutter : public vtkDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard construction and print methods.
   */
  static vtkPlaneCutter* New();
  vtkTypeMacro(vtkPlaneCutter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * The modified time depends on the delegated cut plane.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Specify the plane (an implicit function) to perform the cutting. The
   * definition of the plane (its origin and normal) is controlled via this
   * instance of vtkPlane.
   */
  virtual void SetPlane(vtkPlane*);
  vtkGetObjectMacro(Plane, vtkPlane);
  //@}

  //@{
  /**
   * Set/Get the computation of normals. The normal generated is simply the
   * cut plane normal. The normal, if generated, is defined by cell data
   * associated with the output polygons. By default computing of normals is
   * disabled.
   */
  vtkSetMacro(ComputeNormals, bool);
  vtkGetMacro(ComputeNormals, bool);
  vtkBooleanMacro(ComputeNormals, bool);
  //@}

  //@{
  /**
   * Indicate whether to interpolate attribute data. By default this is
   * enabled. Note that both cell data and point data is interpolated and
   * outputted, except for image data input where only point data are outputted.
   */
  vtkSetMacro(InterpolateAttributes, bool);
  vtkGetMacro(InterpolateAttributes, bool);
  vtkBooleanMacro(InterpolateAttributes, bool);
  //@}

  //@{
  /**
   * Indicate whether to generate polygons instead of triangles when cutting
   * structured and rectilinear grid.
   * No effect with other kinds of inputs, enabled by default.
   */
  vtkSetMacro(GeneratePolygons, bool);
  vtkGetMacro(GeneratePolygons, bool);
  vtkBooleanMacro(GeneratePolygons, bool);
  //@}

  //@{
  /**
   * Indicate whether to build the sphere tree. Computing the sphere
   * will take some time on the first computation
   * but if the input does not change, the computation of all further
   * slice will be much faster. Default is on.
   */
  vtkSetMacro(BuildTree, bool);
  vtkGetMacro(BuildTree, bool);
  vtkBooleanMacro(BuildTree, bool);
  //@}

  //@{
  /**
   * Indicate whether to build tree hierarchy. Computing the tree
   * hierarchy can take some time on the first computation but if
   * the input does not change, the computation of all further
   * slice will be faster. Default is on.
   */
  vtkSetMacro(BuildHierarchy, bool);
  vtkGetMacro(BuildHierarchy, bool);
  vtkBooleanMacro(BuildHierarchy, bool);
  //@}

  /**
   * See vtkAlgorithm for details.
   */
  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkPlaneCutter();
  ~vtkPlaneCutter() override;

  vtkPlane* Plane;
  bool ComputeNormals;
  bool InterpolateAttributes;
  bool GeneratePolygons;
  bool BuildTree;
  bool BuildHierarchy;

  // Helpers
  std::vector<vtkSmartPointer<vtkSphereTree>> SphereTrees;

  // Pipeline-related methods
  int RequestDataObject(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  virtual int ExecuteDataSet(vtkDataSet* input, vtkSphereTree* tree, vtkMultiPieceDataSet* output);

  static void AddNormalArray(double* planeNormal, vtkDataSet* ds);
  static void InitializeOutput(vtkMultiPieceDataSet* output);

private:
  vtkPlaneCutter(const vtkPlaneCutter&) = delete;
  void operator=(const vtkPlaneCutter&) = delete;
};

#endif
