// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLUnstructuredDataWriter
 * @brief   Superclass for VTK XML unstructured data writers.
 *
 * vtkXMLUnstructuredDataWriter provides VTK XML writing functionality
 * that is common among all the unstructured data formats.
 */

#ifndef vtkXMLUnstructuredDataWriter_h
#define vtkXMLUnstructuredDataWriter_h

#include "vtkDeprecation.h" // For VTK_DEPRECATED_IN_9_5_0
#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

#include "vtkSmartPointer.h" // for vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkPointSet;
class vtkCellArray;
class vtkCellIterator;
class vtkDataArray;
class vtkIdTypeArray;
class vtkUnstructuredGrid;

class VTKIOXML_EXPORT vtkXMLUnstructuredDataWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLUnstructuredDataWriter, vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the number of pieces used to stream the image through the
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

  ///@{
  /**
   * Get/Set the ghost level used to pad each piece.
   */
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  ///@}

  // See the vtkAlgorithm for a description of what these do
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkXMLUnstructuredDataWriter();
  ~vtkXMLUnstructuredDataWriter() override;

  vtkPointSet* GetPointSetInput();
  VTK_DEPRECATED_IN_9_5_0("Use GetPointSetInput() instead.")
  vtkPointSet* GetInputAsPointSet() { return this->GetPointSetInput(); }
  const char* GetDataSetName() override = 0;
  virtual void SetInputUpdateExtent(int piece, int numPieces, int ghostLevel);

  virtual int WriteHeader();
  virtual int WriteAPiece();
  virtual int WriteFooter();

  virtual void AllocatePositionArrays();
  virtual void DeletePositionArrays();

  virtual int WriteInlineMode(vtkIndent indent);
  virtual void WriteInlinePieceAttributes();
  virtual void WriteInlinePiece(vtkIndent indent);

  virtual void WriteAppendedPieceAttributes(int index);
  virtual void WriteAppendedPiece(int index, vtkIndent indent);
  virtual void WriteAppendedPieceData(int index);

  void WriteCellsInline(const char* name, vtkCellIterator* cellIter, vtkIdType numCells,
    vtkIdType cellSizeEstimate, vtkIndent indent);

  void WriteCellsInline(
    const char* name, vtkCellArray* cells, vtkDataArray* types, vtkIndent indent);

  // New API with face information for polyhedron cell support.
  VTK_DEPRECATED_IN_9_4_0("Use WritePolyCellsInline instead.")
  void WriteCellsInline(const char* name, vtkCellArray* cells, vtkDataArray* types,
    vtkIdTypeArray* faces, vtkIdTypeArray* faceOffsets, vtkIndent indent);

  void WritePolyCellsInline(const char* name, vtkCellArray* cells, vtkDataArray* types,
    vtkCellArray* faces, vtkCellArray* faceOffsets, vtkIndent indent);

  void WriteCellsInlineWorker(const char* name, vtkDataArray* types, vtkIndent indent);

  void WriteCellsAppended(
    const char* name, vtkDataArray* types, vtkIndent indent, OffsetsManagerGroup* cellsManager);

  VTK_DEPRECATED_IN_9_4_0("Use WritePolyCellsAppended instead.")
  void WriteCellsAppended(const char* name, vtkDataArray* types, vtkIdTypeArray* faces,
    vtkIdTypeArray* faceOffsets, vtkIndent indent, OffsetsManagerGroup* cellsManager);

  void WriteCellsAppended(const char* name, vtkCellIterator* cellIter, vtkIdType numCells,
    vtkIndent indent, OffsetsManagerGroup* cellsManager);

  void WritePolyCellsAppended(const char* name, vtkDataArray* types, vtkCellArray* faces,
    vtkCellArray* faceOffsets, vtkIndent indent, OffsetsManagerGroup* cellsManager);

  void WriteCellsAppendedData(
    vtkCellArray* cells, vtkDataArray* types, int timestep, OffsetsManagerGroup* cellsManager);

  void WriteCellsAppendedData(vtkCellIterator* cellIter, vtkIdType numCells,
    vtkIdType cellSizeEstimate, int timestep, OffsetsManagerGroup* cellsManager);

  // New API with face information for polyhedron cell support.
  VTK_DEPRECATED_IN_9_4_0("Use WritePolyCellsAppendedData instead.")
  void WriteCellsAppendedData(vtkCellArray* cells, vtkDataArray* types, vtkIdTypeArray* faces,
    vtkIdTypeArray* faceOffsets, int timestep, OffsetsManagerGroup* cellsManager);

  void WritePolyCellsAppendedData(vtkCellArray* cells, vtkDataArray* types, vtkCellArray* faces,
    vtkCellArray* faceOffsets, int timestep, OffsetsManagerGroup* cellsManager);

  void WriteCellsAppendedDataWorker(
    vtkDataArray* types, int timestep, OffsetsManagerGroup* cellsManager);

  void ConvertCells(vtkCellIterator* cellIter, vtkIdType numCells, vtkIdType cellSizeEstimate);

  void ConvertCells(vtkCellArray* cells);

  // For polyhedron support, conversion results are stored in Faces and FaceOffsets
  VTK_DEPRECATED_IN_9_4_0("Use ConvertPolyFaces instead.")
  void ConvertFaces(vtkIdTypeArray* faces, vtkIdTypeArray* faceOffsets);

  void ConvertPolyFaces(vtkCellArray* faces, vtkCellArray* faceOffsets);

  // Get the number of points/cells.  Valid after Update has been
  // invoked on the input.
  virtual vtkIdType GetNumberOfInputPoints();
  virtual vtkIdType GetNumberOfInputCells() = 0;
  void CalculateDataFractions(float* fractions);
  void CalculateCellFractions(float* fractions, vtkIdType typesSize);

  // Number of pieces used for streaming.
  int NumberOfPieces;

  // Which piece to write, if not all.
  int WritePiece;

  // The ghost level on each piece.
  int GhostLevel;

  // Positions of attributes for each piece.
  vtkTypeInt64* NumberOfPointsPositions;

  // For TimeStep support
  OffsetsManagerGroup* PointsOM;
  OffsetsManagerArray* PointDataOM;
  OffsetsManagerArray* CellDataOM;

  // Hold the new cell representation arrays while writing a piece.
  vtkSmartPointer<vtkDataArray> CellPoints;
  vtkSmartPointer<vtkDataArray> CellOffsets;

  int CurrentPiece;

  /**
   *  Legacy support -- hold the face arrays for legacy polyhedron cells
   *     and deprecated writing methods.
   */
  VTK_DEPRECATED_IN_9_4_0("This member is deprecated.")
  vtkIdTypeArray* LegacyFaces;
  VTK_DEPRECATED_IN_9_4_0("This member is deprecated.")
  vtkIdTypeArray* LegacyFaceOffsets;

  // Hold the face arrays for polyhedron cells.
  vtkSmartPointer<vtkDataArray> FaceConnectivity;
  vtkSmartPointer<vtkDataArray> FaceOffsets;
  vtkSmartPointer<vtkDataArray> PolyhedronToFaces;
  vtkSmartPointer<vtkDataArray> PolyhedronOffsets;

private:
  vtkXMLUnstructuredDataWriter(const vtkXMLUnstructuredDataWriter&) = delete;
  void operator=(const vtkXMLUnstructuredDataWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
