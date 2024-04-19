// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLDataReader
 * @brief   Superclass for VTK XML file readers.
 *
 * vtkXMLDataReader provides functionality common to all file readers for
 * <a href="http://www.vtk.org/Wiki/VTK_XML_Formats">VTK XML formats</a>.
 * Concrete subclasses call upon this functionality when needed.
 *
 * @sa
 * vtkXMLPDataReader
 */

#ifndef vtkXMLDataReader_h
#define vtkXMLDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLReader.h"

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class VTKIOXML_EXPORT vtkXMLDataReader : public vtkXMLReader
{
public:
  vtkTypeMacro(vtkXMLDataReader, vtkXMLReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the number of points in the output.
   */
  virtual vtkIdType GetNumberOfPoints() = 0;

  /**
   * Get the number of cells in the output.
   */
  virtual vtkIdType GetNumberOfCells() = 0;

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  void CopyOutputInformation(vtkInformation* outInfo, int port) override;

protected:
  vtkXMLDataReader();
  ~vtkXMLDataReader() override;

  // Add functionality to methods from superclass.
  void CreateXMLParser() override;
  void DestroyXMLParser() override;
  void SetupOutputInformation(vtkInformation* outInfo) override;

  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) override;
  void SetupOutputData() override;

  // Setup the reader for a given number of pieces.
  virtual void SetupPieces(int numPieces);
  virtual void DestroyPieces();

  // Read information from the file for the given piece.
  int ReadPiece(vtkXMLDataElement* ePiece, int piece);
  virtual int ReadPiece(vtkXMLDataElement* ePiece);

  // Read data from the file for the given piece.
  int ReadPieceData(int piece);
  virtual int ReadPieceData();

  void ReadXMLData() override;

  // Read a data array whose tuples coorrespond to points or cells.
  virtual int ReadArrayForPoints(vtkXMLDataElement* da, vtkAbstractArray* outArray);
  virtual int ReadArrayForCells(vtkXMLDataElement* da, vtkAbstractArray* outArray);

  // Callback registered with the DataProgressObserver.
  static void DataProgressCallbackFunction(vtkObject*, unsigned long, void*, void*);
  // Progress callback from XMLParser.
  virtual void DataProgressCallback();

  // The number of Pieces of data found in the file.
  int NumberOfPieces;

  // The PointData and CellData element representations for each piece.
  vtkXMLDataElement** PointDataElements;
  vtkXMLDataElement** CellDataElements;
  vtkXMLDataElement** TimeDataElements;

  // The piece currently being read.
  int Piece;

  // The number of point/cell data arrays in the output.  Valid after
  // SetupOutputData has been called.
  int NumberOfPointArrays;
  int NumberOfCellArrays;

  // The observer to report progress from reading data from XMLParser.
  vtkCallbackCommand* DataProgressObserver;

private:
  class MapStringToInt;
  class MapStringToInt64;

  // Specify the last time step read, useful to know if we need to rearead data
  // //PointData
  std::unique_ptr<MapStringToInt> PointDataTimeStep;
  std::unique_ptr<MapStringToInt64> PointDataOffset;
  int PointDataNeedToReadTimeStep(vtkXMLDataElement* eNested);

  // CellData
  std::unique_ptr<MapStringToInt> CellDataTimeStep;
  std::unique_ptr<MapStringToInt64> CellDataOffset;
  int CellDataNeedToReadTimeStep(vtkXMLDataElement* eNested);

  vtkXMLDataReader(const vtkXMLDataReader&) = delete;
  void operator=(const vtkXMLDataReader&) = delete;

  void ConvertGhostLevelsToGhostType(
    FieldType type, vtkAbstractArray* data, vtkIdType startIndex, vtkIdType numValues) override;
};

VTK_ABI_NAMESPACE_END
#endif
