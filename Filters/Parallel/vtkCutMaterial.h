// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCutMaterial
 * @brief   Automatically computes the cut plane for a material array pair.
 *
 * vtkCutMaterial computes a cut plane based on an up vector, center of the bounding box
 * and the location of the maximum variable value.
 *  These computed values are available so that they can be used to set the camera
 * for the best view of the plane.
 */

#ifndef vtkCutMaterial_h
#define vtkCutMaterial_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPlane;

class VTKFILTERSPARALLEL_EXPORT vtkCutMaterial : public vtkPolyDataAlgorithm
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkCutMaterial, vtkPolyDataAlgorithm);
  static vtkCutMaterial* New();

  ///@{
  /**
   * Cell array that contains the material values.
   */
  vtkSetStringMacro(MaterialArrayName);
  vtkGetStringMacro(MaterialArrayName);
  ///@}

  ///@{
  /**
   * Material to probe.
   */
  vtkSetMacro(Material, int);
  vtkGetMacro(Material, int);
  ///@}

  ///@{
  /**
   * For now, we just use the cell values.
   * The array name to cut.
   */
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);
  ///@}

  ///@{
  /**
   * The last piece of information that specifies the plane.
   */
  vtkSetVector3Macro(UpVector, double);
  vtkGetVector3Macro(UpVector, double);
  ///@}

  ///@{
  /**
   * Accesses to the values computed during the execute method.  They
   * could be used to get a good camera view for the resulting plane.
   */
  vtkGetVector3Macro(MaximumPoint, double);
  vtkGetVector3Macro(CenterPoint, double);
  vtkGetVector3Macro(Normal, double);
  ///@}

protected:
  vtkCutMaterial();
  ~vtkCutMaterial() override;

  int RequestData(vtkInformation*, vtkInformationVector**,
    vtkInformationVector*) override; // generate output data
  int FillInputPortInformation(int port, vtkInformation* info) override;
  void ComputeMaximumPoint(vtkDataSet* input);
  void ComputeNormal();

  char* MaterialArrayName;
  int Material;
  char* ArrayName;
  double UpVector[3];
  double MaximumPoint[3];
  double CenterPoint[3];
  double Normal[3];

  vtkPlane* PlaneFunction;

private:
  vtkCutMaterial(const vtkCutMaterial&) = delete;
  void operator=(const vtkCutMaterial&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
