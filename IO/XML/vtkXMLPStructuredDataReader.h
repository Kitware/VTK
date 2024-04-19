// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLPStructuredDataReader
 * @brief   Superclass for parallel structured data XML readers.
 *
 * vtkXMLPStructuredDataReader provides functionality common to all
 * parallel structured data format readers.
 *
 * @sa
 * vtkXMLPImageDataReader vtkXMLPStructuredGridReader
 * vtkXMLPRectilinearGridReader
 */

#ifndef vtkXMLPStructuredDataReader_h
#define vtkXMLPStructuredDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPDataReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkExtentSplitter;
class vtkXMLStructuredDataReader;

class VTKIOXML_EXPORT vtkXMLPStructuredDataReader : public vtkXMLPDataReader
{
public:
  vtkTypeMacro(vtkXMLPStructuredDataReader, vtkXMLPDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  void CopyOutputInformation(vtkInformation* outInfo, int port) override;

protected:
  vtkXMLPStructuredDataReader();
  ~vtkXMLPStructuredDataReader() override;

  vtkIdType GetNumberOfPoints() override;
  vtkIdType GetNumberOfCells() override;
  void CopyArrayForPoints(vtkAbstractArray* inArray, vtkAbstractArray* outArray) override;
  void CopyArrayForCells(vtkAbstractArray* inArray, vtkAbstractArray* outArray) override;

  virtual void SetOutputExtent(int* extent) = 0;
  virtual void GetPieceInputExtent(int index, int* extent) = 0;

  // Pipeline execute data driver.  Called by vtkXMLReader.
  void ReadXMLData() override;
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) override;

  void SetupOutputData() override;

  void SetupPieces(int numPieces) override;
  void DestroyPieces() override;
  int ReadPiece(vtkXMLDataElement* ePiece) override;
  int ReadPieceData() override;
  void CopySubExtent(int* inExtent, int* inDimensions, vtkIdType* inIncrements, int* outExtent,
    int* outDimensions, vtkIdType* outIncrements, int* subExtent, int* subDimensions,
    vtkAbstractArray* inArray, vtkAbstractArray* outArray);
  int ComputePieceSubExtents();

  vtkExtentSplitter* ExtentSplitter;

  // The extent to be updated in the output.
  int UpdateExtent[6];
  int PointDimensions[3];
  vtkIdType PointIncrements[3];
  int CellDimensions[3];
  vtkIdType CellIncrements[3];

  // The extent currently being read from a piece.
  int SubExtent[6];
  int SubPointDimensions[3];
  int SubCellDimensions[3];
  int SubPieceExtent[6];
  int SubPiecePointDimensions[3];
  vtkIdType SubPiecePointIncrements[3];
  int SubPieceCellDimensions[3];
  vtkIdType SubPieceCellIncrements[3];

  // Information per-piece.
  int* PieceExtents;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkXMLPStructuredDataReader(const vtkXMLPStructuredDataReader&) = delete;
  void operator=(const vtkXMLPStructuredDataReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
