// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLUnstructuredGridWriter
 * @brief   Write VTK XML UnstructuredGrid files.
 *
 * vtkXMLUnstructuredGridWriter writes the VTK XML UnstructuredGrid
 * file format.  One unstructured grid input can be written into one
 * file in any number of streamed pieces (if supported by the rest of
 * the pipeline).  The standard extension for this writer's file
 * format is "vtu".  This writer is also used to write a single piece
 * of the parallel file format.
 *
 * @sa
 * vtkXMLPUnstructuredGridWriter
 */

#ifndef vtkXMLUnstructuredGridWriter_h
#define vtkXMLUnstructuredGridWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLUnstructuredDataWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkUnstructuredGridBase;

class VTKIOXML_EXPORT vtkXMLUnstructuredGridWriter : public vtkXMLUnstructuredDataWriter
{
public:
  vtkTypeMacro(vtkXMLUnstructuredGridWriter, vtkXMLUnstructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLUnstructuredGridWriter* New();

  /**
   * Get/Set the writer's input.
   */
  vtkUnstructuredGridBase* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  vtkXMLUnstructuredGridWriter();
  ~vtkXMLUnstructuredGridWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) override;

  void AllocatePositionArrays() override;
  void DeletePositionArrays() override;

  const char* GetDataSetName() override;

  void WriteInlinePieceAttributes() override;
  void WriteInlinePiece(vtkIndent indent) override;

  void WriteAppendedPieceAttributes(int index) override;
  void WriteAppendedPiece(int index, vtkIndent indent) override;
  void WriteAppendedPieceData(int index) override;

  vtkIdType GetNumberOfInputCells() override;
  void CalculateSuperclassFraction(float* fractions);

  // Positions of attributes for each piece.
  vtkTypeInt64* NumberOfCellsPositions;
  OffsetsManagerArray* CellsOM; // one per piece

private:
  vtkXMLUnstructuredGridWriter(const vtkXMLUnstructuredGridWriter&) = delete;
  void operator=(const vtkXMLUnstructuredGridWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
