// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFacetReader
 * @brief   reads a dataset in Facet format
 *
 * vtkFacetReader creates a poly data dataset. It reads ASCII files
 * stored in Facet format
 *
 * The facet format looks like this:
 * FACET FILE ...
 * nparts
 * Part 1 name
 * 0
 * npoints 0 0
 * p1x p1y p1z
 * p2x p2y p2z
 * ...
 * 1
 * Part 1 name
 * ncells npointspercell
 * p1c1 p2c1 p3c1 ... pnc1 materialnum partnum
 * p1c2 p2c2 p3c2 ... pnc2 materialnum partnum
 * ...
 */

#ifndef vtkFacetReader_h
#define vtkFacetReader_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSHYBRID_EXPORT vtkFacetReader : public vtkPolyDataAlgorithm
{
public:
  static vtkFacetReader* New();
  vtkTypeMacro(vtkFacetReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify file name of Facet datafile to read
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  static int CanReadFile(VTK_FILEPATH const char* filename);

protected:
  vtkFacetReader();
  ~vtkFacetReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  char* FileName;

private:
  vtkFacetReader(const vtkFacetReader&) = delete;
  void operator=(const vtkFacetReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
