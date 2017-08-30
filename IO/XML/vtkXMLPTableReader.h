/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPTableReader
 * @brief   Read PVTK XML Table files.
 *
 * vtkXMLPTableReader reads the PVTK XML Table
 * file format.  This reads the parallel format's summary file and
 * then uses vtkXMLTableReader to read data from the
 * individual Table piece files.  Streaming is supported.
 * The standard extension for this reader's file format is "pvtt".
 *
 * @sa
 * vtkXMLTableReader
*/

#ifndef vtkXMLPTableReader_h
#define vtkXMLPTableReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPDataObjectReader.h"

class vtkTable;
class vtkXMLTableReader;

class VTKIOXML_EXPORT vtkXMLPTableReader : public vtkXMLPDataObjectReader
{
public:
  vtkTypeMacro(vtkXMLPTableReader, vtkXMLPDataObjectReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkXMLPTableReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  vtkTable* GetOutput();
  vtkTable* GetOutput(int idx);
  //@}

  /**
  * For the specified port, copy the information this reader sets up in
  * SetupOutputInformation to outInfo
  */
  void CopyOutputInformation(vtkInformation* outInfo, int port) VTK_OVERRIDE;

  /**
   * Get the number of columns arrays available in the input.
   */
  int GetNumberOfColumnArrays();

  /**
   * Get the name of the column with the given index in
   * the input.
   */
  const char* GetColumnArrayName(int index);

  //@{
  /**
   * Get/Set whether the column array with the given name is to
   * be read.
   */
  int GetColumnArrayStatus(const char* name);
  void SetColumnArrayStatus(const char* name, int status);
  //@}

  /**
   * Get the data array selection tables used to configure which data
   * arrays are loaded by the reader.
   */
  vtkGetObjectMacro(ColumnSelection, vtkDataArraySelection);

protected:
  vtkXMLPTableReader();
  ~vtkXMLPTableReader() VTK_OVERRIDE;

  /**
   * Return hte type of the dataset being read
   */
  const char* GetDataSetName() VTK_OVERRIDE;

  /**
   * Get the number of rows of the table
   */
  vtkIdType GetNumberOfRows();

  /**
  * Get the number of rows in the given piece.  Valid after
  * UpdateInformation.
  */
  virtual vtkIdType GetNumberOfRowsInPiece(int piece);

  vtkTable* GetOutputAsTable();

  vtkTable* GetPieceInputAsTable(int piece);

  /**
   * Get the current piece index and the total number of piece in the dataset
   */
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces);

  /**
   * Initialize current output
   */
  void SetupEmptyOutput() VTK_OVERRIDE;

  /**
   * Initialize current output data: allocate arrays for RowData
   */
  void SetupOutputData() VTK_OVERRIDE;

  /**
   * Setup the output's information.
   */
  void SetupOutputInformation(vtkInformation* outInfo) VTK_OVERRIDE;

  /**
   * Initialize the total number of rows to be read.
   */
  void SetupOutputTotals();

  /**
   * Initialize the index of the first row to be read in the next piece
   */
  void SetupNextPiece();

  /**
   * Setup the number of pieces to be read and allocate space accordingly
   */
  void SetupPieces(int numPieces) VTK_OVERRIDE;

  /**
   * Setup the extent for the parallel reader and the piece readers.
   */
  void SetupUpdateExtent(int piece, int numberOfPieces);

  /**
   * Setup the readers and then read the input data
   */
  void ReadXMLData() VTK_OVERRIDE;

  /**
   * Whether or not the current reader can read the current piece
   */
  int CanReadPiece(int index) VTK_OVERRIDE;

  /**
   * Pipeline execute data driver. Called by vtkXMLReader.
   */
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) VTK_OVERRIDE;

  /**
   * Delete all piece readers and related information
   */
  void DestroyPieces() VTK_OVERRIDE;

  using vtkXMLPDataObjectReader::ReadPiece;

  /**
   * Setup the current piece reader.
   */
  int ReadPiece(vtkXMLDataElement* ePiece) VTK_OVERRIDE;

  /**
   * Read piece at the given index RowData
   */
  int ReadPieceData(int index);

  /**
   * Actually read the current piece data
   */
  int ReadPieceData();

  /**
   * Create a reader according to the data to read
   */
  vtkXMLTableReader* CreatePieceReader();

  int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  /**
  * Callback registered with the PieceProgressObserver.
  */
  void PieceProgressCallback() VTK_OVERRIDE;

  /**
  * Check whether the given array element is an enabled array.
  */
  int ColumnIsEnabled(vtkXMLDataElement* elementRowData);

  int GetNumberOfRowArrays();
  const char* GetRowArrayName(int index);

  /**
  * The update request.
  */
  int UpdatePiece;
  int UpdateNumberOfPieces;

  /**
  * The range of pieces from the file that will form the UpdatePiece.
  */
  int StartPiece;
  int EndPiece;
  vtkIdType TotalNumberOfRows;
  vtkIdType StartRow;

  vtkXMLTableReader** PieceReaders;

  /**
  * The PRowData element representations.
  */
  vtkXMLDataElement* PRowElement;

  vtkDataArraySelection* ColumnSelection;

private:
  vtkXMLPTableReader(const vtkXMLPTableReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPTableReader&) VTK_DELETE_FUNCTION;
};

#endif
