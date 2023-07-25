// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExplicitStructuredGridSurfaceFilter
 * @brief   Filter which creates a surface (polydata) from an explicit structured grid.
 */

#ifndef vtkExplicitStructuredGridSurfaceFilter_h
#define vtkExplicitStructuredGridSurfaceFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkExplicitStructuredGrid;
class vtkIdTypeArray;
class vtkMultiProcessController;

class VTKFILTERSGEOMETRY_EXPORT vtkExplicitStructuredGridSurfaceFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkExplicitStructuredGridSurfaceFilter* New();
  vtkTypeMacro(vtkExplicitStructuredGridSurfaceFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * If on, the output polygonal dataset will have a celldata array that
   * holds the cell index of the original 3D cell that produced each output
   * cell. This is useful for cell picking. The default is off to conserve
   * memory.
   */
  vtkSetMacro(PassThroughCellIds, int);
  vtkGetMacro(PassThroughCellIds, int);
  vtkBooleanMacro(PassThroughCellIds, int);
  vtkSetMacro(PassThroughPointIds, int);
  vtkGetMacro(PassThroughPointIds, int);
  vtkBooleanMacro(PassThroughPointIds, int);
  ///@}

  ///@{
  /**
   * If PassThroughCellIds or PassThroughPointIds is on, then these ivars
   * control the name given to the field in which the ids are written into.  If
   * set to NULL, then vtkOriginalCellIds or vtkOriginalPointIds (the default)
   * is used, respectively.
   */
  vtkSetStringMacro(OriginalCellIdsName);
  virtual const char* GetOriginalCellIdsName()
  {
    return (this->OriginalCellIdsName ? this->OriginalCellIdsName : "vtkOriginalCellIds");
  }
  vtkSetStringMacro(OriginalPointIdsName);
  virtual const char* GetOriginalPointIdsName()
  {
    return (this->OriginalPointIdsName ? this->OriginalPointIdsName : "vtkOriginalPointIds");
  }
  ///@}

protected:
  vtkExplicitStructuredGridSurfaceFilter();
  ~vtkExplicitStructuredGridSurfaceFilter() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  int ExtractSurface(vtkExplicitStructuredGrid*, vtkPolyData*);

  // Helper methods.

  int PieceInvariant;

  int PassThroughCellIds;
  char* OriginalCellIdsName;

  int PassThroughPointIds;
  char* OriginalPointIdsName;

  int WholeExtent[6];

private:
  vtkExplicitStructuredGridSurfaceFilter(const vtkExplicitStructuredGridSurfaceFilter&) = delete;
  void operator=(const vtkExplicitStructuredGridSurfaceFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
