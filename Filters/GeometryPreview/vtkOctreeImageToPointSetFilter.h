// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOctreeImageToPointSetFilter
 * @brief   Convert an octree image to point set
 *
 * vtkOctreeImageToPointSetFilter is a filter that converts an image with an octree unsigned char
 * cell array to a pointset. Each bit of the unsigned char indicates if the cell had a point close
 * to one of its 8 corners.
 *
 * It can optionally also output a point data array based on an input cell data scalar array by
 * setting SetInputArrayToProcess. This array will have one of the components of the input array.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa vtkPointSetToOctreeImageFilter
 */
#ifndef vtkOctreeImageToPointSetFilter_h
#define vtkOctreeImageToPointSetFilter_h

#include "vtkFiltersGeometryPreviewModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGEOMETRYPREVIEW_EXPORT vtkOctreeImageToPointSetFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkOctreeImageToPointSetFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkOctreeImageToPointSetFilter* New();

  ///@{
  /**
   * Set/Get if a cell array of vertices will be created.
   *
   * The default is on.
   */
  vtkSetMacro(CreateVerticesCellArray, bool);
  vtkGetMacro(CreateVerticesCellArray, bool);
  vtkBooleanMacro(CreateVerticesCellArray, bool);
  ///@}

  ///@{
  /**
   * Set/Get if array defined using SetInputArrayToProcess, which MUST be a cell data array, will
   * be processed.
   *
   * The default is off.
   */
  vtkSetMacro(ProcessInputCellArray, bool);
  vtkGetMacro(ProcessInputCellArray, bool);
  vtkBooleanMacro(ProcessInputCellArray, bool);
  ///@}

  ///@{
  /**
   * Set/Get the component of the input cell data array to process.
   *
   * The default is 0.
   */
  vtkSetClampMacro(CellArrayComponent, int, 0, VTK_INT_MAX);
  vtkGetMacro(CellArrayComponent, int);
  ///@}

protected:
  vtkOctreeImageToPointSetFilter();
  ~vtkOctreeImageToPointSetFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkOctreeImageToPointSetFilter(const vtkOctreeImageToPointSetFilter&) = delete;
  void operator=(const vtkOctreeImageToPointSetFilter&) = delete;

  bool CreateVerticesCellArray = true;
  bool ProcessInputCellArray = false;
  int CellArrayComponent = 0;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOctreeImageToPointSetFilter_h
