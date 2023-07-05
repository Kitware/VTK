// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFacetWriter
 * @brief   reads a dataset in Facet format
 *
 * vtkFacetWriter creates an unstructured grid dataset. It reads ASCII files
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

#ifndef vtkFacetWriter_h
#define vtkFacetWriter_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkInformation;

class VTKIOGEOMETRY_EXPORT vtkFacetWriter : public vtkPolyDataAlgorithm
{
public:
  static vtkFacetWriter* New();
  vtkTypeMacro(vtkFacetWriter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify file name of Facet datafile to read
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  /**
   * Write data
   */
  void Write();

  void WriteToStream(ostream* ost);

protected:
  vtkFacetWriter();
  ~vtkFacetWriter() override;

  // This is called by the superclass.
  // This is the method you should override.
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillInputPortInformation(int, vtkInformation*) override;

  int WriteDataToStream(ostream* ost, vtkPolyData* data);

  char* FileName;
  ostream* OutputStream;

private:
  vtkFacetWriter(const vtkFacetWriter&) = delete;
  void operator=(const vtkFacetWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
