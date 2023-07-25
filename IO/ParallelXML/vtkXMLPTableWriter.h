// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLPTableWriter
 * @brief   Write PVTK XML UnstructuredGrid files.
 *
 * vtkXMLPTableWriter writes the PVTK XML Table
 * file format.  One table input can be written into a
 * parallel file format with any number of pieces spread across files.
 * The standard extension for this writer's file format is "pvtt".
 * This writer uses vtkXMLTableWriter to write the
 * individual piece files.
 *
 * @sa
 * vtkXMLTableWriter
 */

#ifndef vtkXMLPTableWriter_h
#define vtkXMLPTableWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPDataObjectWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCallbackCommand;
class vtkMultiProcessController;
class vtkTable;
class vtkXMLTableWriter;
class vtkXMLPDataObjectWriter;

class VTKIOPARALLELXML_EXPORT vtkXMLPTableWriter : public vtkXMLPDataObjectWriter
{
public:
  static vtkXMLPTableWriter* New();
  vtkTypeMacro(vtkXMLPTableWriter, vtkXMLPDataObjectWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set the writer's input.
   */
  vtkTable* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  vtkXMLPTableWriter();
  ~vtkXMLPTableWriter() override;

  /**
   * see algorithm for more info
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Return the type of data being actually written
   */
  const char* GetDataSetName() override;

  /**
   * Create a writer for the piece at a given index
   */
  vtkXMLWriter* CreatePieceWriter(int index);

  /**
   * Create a table writer for the actual piece. Used by
   * CreatePieceWriter(int index)
   */
  vtkXMLTableWriter* CreateTablePieceWriter();

  /**
   * Write a piece of the dataset on disk. Called by WritePieceInternal()
   */
  int WritePiece(int index) override;

  /**
   * Method called by the superclass::WriteInternal(). Write a piece using
   * WritePiece(int index).
   */
  int WritePieceInternal() override;

  /**
   * Write Data associated with the input dataset
   */
  void WritePData(vtkIndent indent) override;

  /**
   * Write RowData. Called by WritePData(vtkIndent indent)
   */
  void WritePRowData(vtkDataSetAttributes* ds, vtkIndent indent);

private:
  vtkXMLPTableWriter(const vtkXMLPTableWriter&) = delete;
  void operator=(const vtkXMLPTableWriter&) = delete;

  /**
   * Initializes PieceFileNameExtension.
   */
  void SetupPieceFileNameExtension() override;
};

VTK_ABI_NAMESPACE_END
#endif
