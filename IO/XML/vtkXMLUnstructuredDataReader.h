// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLUnstructuredDataReader
 * @brief   Superclass for unstructured data XML readers.
 *
 * vtkXMLUnstructuredDataReader provides functionality common to all
 * unstructured data format readers.
 *
 * @sa
 * vtkXMLPolyDataReader vtkXMLUnstructuredGridReader
 */

#ifndef vtkXMLUnstructuredDataReader_h
#define vtkXMLUnstructuredDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLDataReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkIdTypeArray;
class vtkPointSet;
class vtkUnsignedCharArray;

class VTKIOXML_EXPORT vtkXMLUnstructuredDataReader : public vtkXMLDataReader
{
public:
  vtkTypeMacro(vtkXMLUnstructuredDataReader, vtkXMLDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the number of points in the output.
   */
  vtkIdType GetNumberOfPoints() override;

  /**
   * Get the number of cells in the output.
   */
  vtkIdType GetNumberOfCells() override;

  /**
   * Get the number of pieces in the file
   */
  virtual vtkIdType GetNumberOfPieces();

  /**
   * Setup the reader as if the given update extent were requested by
   * its output.  This can be used after an UpdateInformation to
   * validate GetNumberOfPoints() and GetNumberOfCells() without
   * actually reading data.
   */
  void SetupUpdateExtent(int piece, int numberOfPieces, int ghostLevel);

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  void CopyOutputInformation(vtkInformation* outInfo, int port) override;

protected:
  vtkXMLUnstructuredDataReader();
  ~vtkXMLUnstructuredDataReader() override;

  vtkPointSet* GetOutputAsPointSet();
  vtkXMLDataElement* FindDataArrayWithName(vtkXMLDataElement* eParent, const char* name);

  // note that these decref the input array and return an array with a
  // new reference:
  vtkIdTypeArray* ConvertToIdTypeArray(vtkDataArray* a);
  vtkUnsignedCharArray* ConvertToUnsignedCharArray(vtkDataArray* a);

  // Pipeline execute data driver.  Called by vtkXMLReader.
  void ReadXMLData() override;

  void SetupEmptyOutput() override;
  virtual void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) = 0;
  virtual void SetupOutputTotals();
  virtual void SetupNextPiece();
  void SetupPieces(int numPieces) override;
  void DestroyPieces() override;

  // Setup the output's information.
  void SetupOutputInformation(vtkInformation* outInfo) override;

  void SetupOutputData() override;
  int ReadPiece(vtkXMLDataElement* ePiece) override;
  int ReadPieceData() override;
  int ReadCellArray(vtkIdType numberOfCells, vtkIdType totalNumberOfCells,
    vtkXMLDataElement* eCells, vtkCellArray* outCells);

  // Read faces and faceoffsets arrays for unstructured grid with polyhedon cells
  int ReadPolyhedronCellArray(vtkIdType numberOfCells, vtkXMLDataElement* eCells,
    vtkCellArray* outFaces, vtkCellArray* outFaceOffsets);
  // Backward compatibility layer to read unstructured grid with polyhedron cells.
  int ReadFaceArray(vtkIdType numberOfCells, vtkXMLDataElement* eCells, vtkIdTypeArray* outFaces,
    vtkIdTypeArray* outFaceOffsets);
  int ReadFaceCellArray(vtkIdType numberOfCells, vtkXMLDataElement* eCells, vtkCellArray* outFaces,
    vtkCellArray* outFaceOffsets);

  // Read a data array whose tuples coorrespond to points.
  int ReadArrayForPoints(vtkXMLDataElement* da, vtkAbstractArray* outArray) override;

  // Get the number of points/cells in the given piece.  Valid after
  // UpdateInformation.
  virtual vtkIdType GetNumberOfPointsInPiece(int piece);
  virtual vtkIdType GetNumberOfCellsInPiece(int piece) = 0;

  // The update request.
  int UpdatePieceId;
  int UpdateNumberOfPieces;
  int UpdateGhostLevel;

  // The range of pieces from the file that will form the UpdatePiece.
  int StartPiece;
  int EndPiece;
  vtkIdType TotalNumberOfPoints;
  vtkIdType TotalNumberOfCells;
  vtkIdType StartPoint;

  // The Points element for each piece.
  vtkXMLDataElement** PointElements;
  vtkIdType* NumberOfPoints;

  int PointsTimeStep;
  unsigned long PointsOffset;
  int PointsNeedToReadTimeStep(vtkXMLDataElement* eNested);
  int CellsNeedToReadTimeStep(
    vtkXMLDataElement* eNested, int& cellstimestep, unsigned long& cellsoffset);

  int CellArrayTimeStepRead;
  bool CanReadCellArray;
  const char* CellArrayCachedInputString;
  const char* CellArrayCachedFileName;

private:
  vtkXMLUnstructuredDataReader(const vtkXMLUnstructuredDataReader&) = delete;
  void operator=(const vtkXMLUnstructuredDataReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
