// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLPUnstructuredDataReader
 * @brief   Superclass for parallel unstructured data XML readers.
 *
 * vtkXMLPUnstructuredDataReader provides functionality common to all
 * parallel unstructured data format readers.
 *
 * @sa
 * vtkXMLPPolyDataReader vtkXMLPUnstructuredGridReader
 */

#ifndef vtkXMLPUnstructuredDataReader_h
#define vtkXMLPUnstructuredDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPDataReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkPointSet;
class vtkCellArray;
class vtkXMLUnstructuredDataReader;

class VTKIOXML_EXPORT vtkXMLPUnstructuredDataReader : public vtkXMLPDataReader
{
public:
  vtkTypeMacro(vtkXMLPUnstructuredDataReader, vtkXMLPDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  void CopyOutputInformation(vtkInformation* outInfo, int port) override;

protected:
  vtkXMLPUnstructuredDataReader();
  ~vtkXMLPUnstructuredDataReader() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  vtkPointSet* GetOutputAsPointSet();
  vtkPointSet* GetPieceInputAsPointSet(int piece);
  virtual void SetupOutputTotals();
  virtual void SetupNextPiece();
  vtkIdType GetNumberOfPoints() override;
  vtkIdType GetNumberOfCells() override;
  void CopyArrayForPoints(vtkAbstractArray* inArray, vtkAbstractArray* outArray) override;

  void SetupEmptyOutput() override;

  // Setup the output's information.
  void SetupOutputInformation(vtkInformation* outInfo) override;

  void SetupOutputData() override;
  virtual void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) = 0;

  // Pipeline execute data driver.  Called by vtkXMLReader.
  void ReadXMLData() override;
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) override;
  void SetupUpdateExtent(int piece, int numberOfPieces, int ghostLevel);

  int ReadPieceData() override;
  void CopyCellArray(vtkIdType totalNumberOfCells, vtkCellArray* inCells, vtkCellArray* outCells);

  // Get the number of points/cells in the given piece.  Valid after
  // UpdateInformation.
  virtual vtkIdType GetNumberOfPointsInPiece(int piece);
  virtual vtkIdType GetNumberOfCellsInPiece(int piece);

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

  // The PPoints element with point information.
  vtkXMLDataElement* PPointsElement;

private:
  vtkXMLPUnstructuredDataReader(const vtkXMLPUnstructuredDataReader&) = delete;
  void operator=(const vtkXMLPUnstructuredDataReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
