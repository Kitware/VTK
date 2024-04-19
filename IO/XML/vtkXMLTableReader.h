// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLTableReader
 * @brief   Read VTK XML Table files.
 *
 * vtkXMLTableReader provides a functionality for reading .vtt files as
 * vtkTable
 *
 */

#ifndef vtkXMLTableReader_h
#define vtkXMLTableReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLReader.h"

#include <map> // needed for std::map

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkIdTypeArray;
class vtkUnsignedCharArray;
class vtkTable;

class VTKIOXML_EXPORT vtkXMLTableReader : public vtkXMLReader
{
public:
  vtkTypeMacro(vtkXMLTableReader, vtkXMLReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLTableReader* New();

  ///@{
  /**
   * Get the reader's output.
   */
  vtkTable* GetOutput();
  vtkTable* GetOutput(int idx);
  ///@}

  /**
   * Get the number of rows in the output.
   */
  vtkIdType GetNumberOfRows();

  /**
   * Get the number of pieces in the file
   */
  vtkIdType GetNumberOfPieces();

  /**
   * Setup the reader as if the given update extent were requested by
   * its output.  This can be used after an UpdateInformation to
   * validate GetNumberOfPoints() and GetNumberOfCells() without
   * actually reading data.
   */
  void SetupUpdateExtent(int piece, int numberOfPieces);

  /**
   * For the specified port, copy the information this reader sets up in
   * SetupOutputInformation to outInfo
   */
  void CopyOutputInformation(vtkInformation* outInfo, int port) override;

protected:
  vtkXMLTableReader();
  ~vtkXMLTableReader() override;

  /**
   * Check whether the given array element is an enabled array.
   */
  int ColumnIsEnabled(vtkXMLDataElement* eRowData);

  void DestroyPieces();

  /**
   * Get the name of the data set being read.
   */
  const char* GetDataSetName() override;

  /**
   * Get the current piece index and the total number of piece in the dataset
   */
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces);

  /**
   * Specify the last time step read, useful to know if we need to read data
   */
  int RowDataNeedToReadTimeStep(vtkXMLDataElement* eNested);

  /**
   * Initialize current output
   */
  void SetupEmptyOutput() override;

  /**
   * Initialize the total number of rows to be read.
   */
  void SetupOutputTotals();

  /**
   * Initialize the index of the first row to be read in the next piece
   */
  void SetupNextPiece();

  /**
   * Initialize current output data: allocate arrays for RowData
   */
  void SetupOutputData() override;

  /**
   * Setup the output's information.
   */
  void SetupOutputInformation(vtkInformation* outInfo) override;

  /**
   * Setup the number of pieces to be read and allocate space accordingly
   */
  void SetupPieces(int numPieces);

  /**
   * Pipeline execute data driver.  Called by vtkXMLReader.
   */
  void ReadXMLData() override;

  /**
   * Pipeline execute data driver. Called by vtkXMLReader.
   */
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) override;

  /**
   * Setup the piece reader at the given index.
   */
  int ReadPiece(vtkXMLDataElement* ePiece, int piece);

  /**
   * Setup the current piece reader.
   */
  int ReadPiece(vtkXMLDataElement* ePiece);

  /**
   * Actually read the current piece data
   */
  int ReadPieceData(int);

  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * The update request.
   */
  int UpdatedPiece;
  int UpdateNumberOfPieces;
  /**
   * The range of pieces from the file that will form the UpdatedPiece.
   */
  int StartPiece;
  int EndPiece;
  vtkIdType TotalNumberOfRows;
  vtkIdType StartPoint;

  /**
   * The Points element for each piece.
   */
  vtkXMLDataElement** RowElements;
  vtkIdType* NumberOfRows;

  /**
   * The number of Pieces of data found in the file.
   */
  int NumberOfPieces;

  /**
   * The piece currently being read.
   */
  int Piece;

  /**
   * The RowData element representations for each piece.
   */
  vtkXMLDataElement** RowDataElements;

  /**
   * The number of columns arrays in the output. Valid after
   * SetupOutputData has been called.
   */
  int NumberOfColumns;

private:
  std::map<std::string, int> RowDataTimeStep;
  std::map<std::string, vtkTypeInt64> RowDataOffset;

  vtkXMLTableReader(const vtkXMLTableReader&) = delete;
  void operator=(const vtkXMLTableReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
