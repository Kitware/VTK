// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLStatisticalModelWriter
 * @brief   Write VTK XML Table files.
 *
 * vtkXMLStatisticalModelWriter provides a functionality for writing vtkStatisticalModel as
 * XML .vtstat files.
 */

#ifndef vtkXMLStatisticalModelWriter_h
#define vtkXMLStatisticalModelWriter_h

#include "vtkDeprecation.h" // For VTK_DEPRECATED_IN_9_5_0
#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkStatisticalModel;

class VTKIOXML_EXPORT vtkXMLStatisticalModelWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLStatisticalModelWriter, vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLStatisticalModelWriter* New();

  /**
   * See the vtkAlgorithm for a description of what these do
   */
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkXMLStatisticalModelWriter();
  ~vtkXMLStatisticalModelWriter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkStatisticalModel* GetModelInput();
  const char* GetDataSetName() override; // vtkTable isn't a DataSet but it's used by vtkXMLWriter

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

private:
  void SetInputUpdateExtent(int piece, int numPieces);

  int WriteHeader();
  int WriteAPiece();
  int WriteFooter();

  int WriteInlineModel(vtkIndent indent);
  void WriteInlinePieceAttributes();
  void WriteInlinePiece(vtkIndent indent);

  void WriteAppendedPieceData(int index);

  void WriteModelDataInline(vtkStatisticalModel* ds, vtkIndent indent);

  void AllocatePositionArrays();
  void DeletePositionArrays();

  vtkXMLStatisticalModelWriter(const vtkXMLStatisticalModelWriter&) = delete;
  void operator=(const vtkXMLStatisticalModelWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
