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
 * This algorithm is fast because is is threaded, and may build (in a
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
 * If the data is image data(i.e., the VTK_SMP_IMPLEMENTATION_TYPE is
 * Sequential) then a vtkPolyData is produced as output. Otherwise a
 * vtkMultiBlockDataSet is created.
 *
 * @warning
 * This filter delegates to vtkFlyingEdgesPlaneCutter to process image
 * data. Thus when processing a vtkImageData a single vtkPolyData is output.
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

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkPlane;
class vtkImageData;
class vtkStructuredGrid;
class vtkUnstructuredGrid;
class vtkSphereTree;
class vtkPoints;
class vtkCellArray;
class vtkPointData;
class vtkCellData;

class VTKFILTERSCORE_EXPORT vtkPlaneCutter : public vtkDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard construction and print methods.
   */
  static vtkPlaneCutter *New();
  vtkTypeMacro(vtkPlaneCutter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * The modified time depends on the delegated cut plane.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  //@{
  /**
   * Specify the plane (an implicit function) to perform the cutting. The
   * definition of the plane (its origin and normal) is controlled via this
   * instance of vtkPlane.
   */
  virtual void SetPlane(vtkPlane*);
  vtkGetObjectMacro(Plane,vtkPlane);
  //@}

  //@{
  /**
   * Set/Get the computation of normals. The normal generated is simply the
   * cut plane normal. The normal, if generated, is defined by cell data
   * associated with the output polygons. By default computing of normals is
   * disabled.
   */
  vtkSetMacro(ComputeNormals,int);
  vtkGetMacro(ComputeNormals,int);
  vtkBooleanMacro(ComputeNormals,int);
  //@}

  //@{
  /**
   * Indicate whether to interpolate attribute data. By default this is
   * enabled. Note that both cell data and point data is interpolated and
   * output.
   */
  vtkSetMacro(InterpolateAttributes,int);
  vtkGetMacro(InterpolateAttributes,int);
  vtkBooleanMacro(InterpolateAttributes,int);
  //@}

  /**
   * See vtkAlgorithm for details.
   */
  int ProcessRequest(vtkInformation*, vtkInformationVector**,
                     vtkInformationVector*) VTK_OVERRIDE;

  /**
   * Retrieve the sphere tree used to accelerate cutting. This API may
   * be changed in the future (i.e., use a general locator as compared
   * to a sphere tree).
   */
  vtkGetObjectMacro(SphereTree,vtkSphereTree);

protected:
  vtkPlaneCutter();
  ~vtkPlaneCutter() VTK_OVERRIDE;

  vtkPlane *Plane;
  int ComputeNormals;
  int InterpolateAttributes;

  // Helpers
  vtkSphereTree *SphereTree;

  // Pipeline-related methods
  int RequestDataObject(vtkInformation *, vtkInformationVector **,
                        vtkInformationVector *) VTK_OVERRIDE;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;
  int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

private:
  vtkPlaneCutter(const vtkPlaneCutter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPlaneCutter&) VTK_DELETE_FUNCTION;
};

#endif
