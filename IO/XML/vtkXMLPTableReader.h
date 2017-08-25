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
#include "vtkXMLReader.h"

//class vtkDataArray;
class vtkTable;
class vtkXMLTableReader;

class VTKIOXML_EXPORT vtkXMLPTableReader : public vtkXMLReader
{
public:
  vtkTypeMacro(vtkXMLPTableReader,vtkXMLReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkXMLPTableReader *New();

  //@{
  /**
   * Get the reader's output.
   */
  vtkTable *GetOutput();
  vtkTable *GetOutput(int idx);
  //@}

  /**
  * For the specified port, copy the information this reader sets up in
  * SetupOutputInformation to outInfo
  */
  void CopyOutputInformation(vtkInformation *outInfo, int port) VTK_OVERRIDE;

  //@{
  /**
   * Get the number of pieces from the summary file being read.
   */
  vtkGetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * Get the number of columns arrays available in the input.
   */
  int GetNumberOfColumnArrays();
  //@}

  //@{
  /**
   * Get the name of the column with the given index in
   * the input.
   */
  const char* GetColumnArrayName(int index);
  //@}

  //@{
  /**
   * Get/Set whether the column array with the given name is to
   * be read.
   */
  int GetColumnArrayStatus(const char* name);
  void SetColumnArrayStatus(const char* name, int status);
  //@}

  //@{
  /**
   * Get the data array selection tables used to configure which data
   * arrays are loaded by the reader.
   */
  vtkGetObjectMacro(ColumnSelection, vtkDataArraySelection);
  //@}

protected:
  vtkXMLPTableReader();
  ~vtkXMLPTableReader() VTK_OVERRIDE;

  const char* GetDataSetName() VTK_OVERRIDE;
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces);
  void SetupOutputTotals();

  void SetupOutputData() VTK_OVERRIDE;
  void SetupNextPiece();
  int ReadPieceData();

  vtkXMLTableReader *CreatePieceReader();
  int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

  int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector) VTK_OVERRIDE;


  vtkTable* GetOutputAsTable();
  vtkTable* GetPieceInputAsTable(int piece);
  vtkIdType GetNumberOfRows();

  void SetupEmptyOutput() VTK_OVERRIDE;

  /**
  * Setup the output's information.
  */
  void SetupOutputInformation(vtkInformation *outInfo) VTK_OVERRIDE;

  /**
  * Pipeline execute data driver.  Called by vtkXMLReader.
  */
  void ReadXMLData() VTK_OVERRIDE;
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) VTK_OVERRIDE;
  void SetupUpdateExtent(int piece, int numberOfPieces);

  /**
  * Get the number of rows in the given piece.  Valid after
  * UpdateInformation.
  */
  virtual vtkIdType GetNumberOfRowsInPiece(int piece);

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

  /**
  * Pipeline execute information driver.  Called by vtkXMLReader.
  */
  int ReadXMLInformation() VTK_OVERRIDE;

  virtual void SetupPieces(int numPieces);
  virtual void DestroyPieces();
  int ReadPiece(vtkXMLDataElement* ePiece, int index);
  virtual int ReadPiece(vtkXMLDataElement* ePiece);
  int ReadPieceData(int index);
  int CanReadPiece(int index);

  char* CreatePieceFileName(const char* fileName);
  void SplitFileName();

  /**
  * Callback registered with the PieceProgressObserver.
  */
  static void PieceProgressCallbackFunction(vtkObject*, unsigned long, void*,
                                           void*);
  virtual void PieceProgressCallback();

  /**
  * Pieces from the input summary file.
  */
  int NumberOfPieces;

  /**
  * The piece currently being read.
  */
  int Piece;

  /**
  * The path to the input file without the file name.
  */
  char* PathName;

  /**
  * Information per-piece.
  */
  vtkXMLDataElement** PieceElements;
  vtkXMLTableReader** PieceReaders;
  int* CanReadPieceFlag;

  /**
  * The PRowData element representations.
  */
  vtkXMLDataElement* PRowElement;

  /**
  * The observer to report progress from reading serial data in each
  * piece.
  */
  vtkCallbackCommand* PieceProgressObserver;

  /**
  * Check whether the given array element is an enabled array.
  */
  int ColumnIsEnabled(vtkXMLDataElement* elementRowData);

  int GetNumberOfRowArrays();
  const char* GetRowArrayName(int index);


  vtkDataArraySelection* ColumnSelection;

private:
  vtkXMLPTableReader(const vtkXMLPTableReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPTableReader&) VTK_DELETE_FUNCTION;
};

#endif
