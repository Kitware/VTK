// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLTableWriter
 * @brief   Write VTK XML Table files.
 *
 * vtkXMLTableWriter provides a functionality for writing vtTable as
 * XML .vtt files.
 */

#ifndef vtkXMLTableWriter_h
#define vtkXMLTableWriter_h

#include "vtkDeprecation.h" // For VTK_DEPRECATED_IN_9_5_0
#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkTable;

class VTKIOXML_EXPORT vtkXMLTableWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLTableWriter, vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLTableWriter* New();

  ///@{
  /**
   * Get/Set the number of pieces used to stream the table through the
   * pipeline while writing to the file.
   */
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  ///@}

  ///@{
  /**
   * Get/Set the piece to write to the file.  If this is
   * negative or equal to the NumberOfPieces, all pieces will be written.
   */
  vtkSetMacro(WritePiece, int);
  vtkGetMacro(WritePiece, int);
  ///@}

  /**
   * See the vtkAlgorithm for a description of what these do
   */
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkXMLTableWriter();
  ~vtkXMLTableWriter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkTable* GetTableInput();
  VTK_DEPRECATED_IN_9_5_0("Use GetTableInput() instead.")
  vtkTable* GetInputAsTable() { return this->GetTableInput(); }
  const char* GetDataSetName() override; // vtkTable isn't a DataSet but it's used by vtkXMLWriter

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

  void SetInputUpdateExtent(int piece, int numPieces);

  int WriteHeader();
  int WriteAPiece();
  int WriteFooter();

  void AllocatePositionArrays();
  void DeletePositionArrays();

  int WriteInlineMode(vtkIndent indent);
  void WriteInlinePieceAttributes();
  void WriteInlinePiece(vtkIndent indent);

  void WriteAppendedPieceAttributes(int index);
  void WriteAppendedPiece(int index, vtkIndent indent);
  void WriteAppendedPieceData(int index);

  void WriteRowDataAppended(
    vtkDataSetAttributes* ds, vtkIndent indent, OffsetsManagerGroup* dsManager);

  void WriteRowDataAppendedData(
    vtkDataSetAttributes* ds, int timestep, OffsetsManagerGroup* pdManager);

  void WriteRowDataInline(vtkDataSetAttributes* ds, vtkIndent indent);

  /**
   * Number of pieces used for streaming.
   */
  int NumberOfPieces;

  /**
   * Which piece to write, if not all.
   */
  int WritePiece;

  /**
   * Positions of attributes for each piece.
   */
  vtkTypeInt64* NumberOfColsPositions;
  vtkTypeInt64* NumberOfRowsPositions;

  /**
   * For TimeStep support
   */
  OffsetsManagerArray* RowsOM;

  int CurrentPiece;

private:
  vtkXMLTableWriter(const vtkXMLTableWriter&) = delete;
  void operator=(const vtkXMLTableWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
