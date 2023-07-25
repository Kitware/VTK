// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLPHyperTreeGridWriter
 * @brief   Write PVTK XML HyperTreeGrid files.
 *
 * vtkXMLPHyperTreeGridWriter writes the PVTK XML HyperTreeGrid
 * file format.  One hypertree grid input can be written into a
 * parallel file format with any number of pieces spread across files.
 * The standard extension for this writer's file format is "phtg".
 * This writer uses vtkXMLHyperTreeGridWriter to write the
 * individual piece files.
 *
 * @sa
 * vtkXMLHyperTreeGridWriter
 */

#ifndef vtkXMLPHyperTreeGridWriter_h
#define vtkXMLPHyperTreeGridWriter_h

#include "vtkXMLPDataObjectWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCallbackCommand;
class vtkMultiProcessController;
class vtkHyperTreeGrid;
class vtkXMLHyperTreeGridWriter;
class vtkXMLPDataObjectWriter;

class VTKIOPARALLELXML_EXPORT vtkXMLPHyperTreeGridWriter : public vtkXMLPDataObjectWriter
{
public:
  static vtkXMLPHyperTreeGridWriter* New();
  vtkTypeMacro(vtkXMLPHyperTreeGridWriter, vtkXMLPDataObjectWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set the writer's input.
   */
  vtkHyperTreeGrid* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  vtkXMLPHyperTreeGridWriter();
  ~vtkXMLPHyperTreeGridWriter() override;

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
  vtkXMLHyperTreeGridWriter* CreateHyperTreeGridPieceWriter(int index);

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

private:
  vtkXMLPHyperTreeGridWriter(const vtkXMLPHyperTreeGridWriter&) = delete;
  void operator=(const vtkXMLPHyperTreeGridWriter&) = delete;

  /**
   * Initializes PieceFileNameExtension.
   */
  void SetupPieceFileNameExtension() override;
};

VTK_ABI_NAMESPACE_END
#endif
