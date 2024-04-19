// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPOutlineFilterInternals
 * @brief   create wireframe outline (or corners) for arbitrary data set
 *
 * vtkPOutlineFilterInternals has common code for vtkOutlineFilter and
 * vtkOutlineCornerFilter. It assumes the filter is operated in a data parallel
 * pipeline.
 *
 * This class does not inherit from vtkObject and is not intended to be used
 * outside of VTK.
 */

#ifndef vtkPOutlineFilterInternals_h
#define vtkPOutlineFilterInternals_h

#include "vtkBoundingBox.h"           //  needed for vtkBoundingBox.
#include "vtkFiltersParallelModule.h" // For export macro
#include <vector>                     // needed for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkBoundingBox;
class vtkDataObject;
class vtkDataObjectTree;
class vtkDataSet;
class vtkGraph;
class vtkInformation;
class vtkInformationVector;
class vtkMultiProcessController;
class vtkOverlappingAMR;
class vtkPolyData;
class vtkUniformGridAMR;

class VTKFILTERSPARALLEL_EXPORT vtkPOutlineFilterInternals
{
public:
  vtkPOutlineFilterInternals() = default;
  virtual ~vtkPOutlineFilterInternals() = default;

  /**
   * Behave like a vtkAlgorithm::RequestData and compute the outline geometry
   * based on the parameters and provided inputs.
   * Intended to be called in vtkOutlineCornerFilter::RequestData and in
   * vtkOutlineFilter::RequestData.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  /**
   * Set the controller to be used.
   * Default is nullptr.
   */
  void SetController(vtkMultiProcessController*);

  /**
   * Set whether or not to generate a corner outline.
   * Default is false.
   */
  void SetIsCornerSource(bool value);

  /**
   * Set the corner factor to use when creating corner outline.
   * Default is 0.2.
   */
  void SetCornerFactor(double cornerFactor);

private:
  vtkPOutlineFilterInternals(const vtkPOutlineFilterInternals&) = delete;
  vtkPOutlineFilterInternals& operator=(const vtkPOutlineFilterInternals&) = delete;

  int RequestData(vtkOverlappingAMR* amr, vtkPolyData* output);
  int RequestData(vtkUniformGridAMR* amr, vtkPolyData* output);
  int RequestData(vtkDataObjectTree* cd, vtkPolyData* output);
  int RequestData(vtkDataSet* ds, vtkPolyData* output);
  int RequestData(vtkGraph* graph, vtkPolyData* output);

  void CollectCompositeBounds(vtkDataObject* input);
  vtkSmartPointer<vtkPolyData> GenerateOutlineGeometry(double bounds[6]);

  std::vector<vtkBoundingBox> BoundsList;
  vtkMultiProcessController* Controller = nullptr;

  bool IsCornerSource = false;
  double CornerFactor = 0.2;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkPOutlineFilterInternals.h
