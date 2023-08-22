// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLPHyperTreeGridReader
 * @brief   Read PVTK XML HyperTreeGrid files.
 *
 * vtkXMLPHyperTreeGridReader reads the PVTK XML HyperTreeGrid file format.
 * This reader uses vtkXMLHyperTreeGridReader to read data from the
 * individual HyperTreeGrid piece files.  Streaming is supported.
 * The standard extension for this reader's file format is "phtg".
 *
 * @sa
 * vtkXMLHyperTreeGridReader
 */

#ifndef vtkXMLPHyperTreeGridReader_h
#define vtkXMLPHyperTreeGridReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPDataObjectReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;
class vtkXMLHyperTreeGridReader;

class VTKIOXML_EXPORT vtkXMLPHyperTreeGridReader : public vtkXMLPDataObjectReader
{
public:
  vtkTypeMacro(vtkXMLPHyperTreeGridReader, vtkXMLPDataObjectReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLPHyperTreeGridReader* New();

  ///@{
  /**
   * Get the reader's output.
   */
  vtkHyperTreeGrid* GetOutput();
  vtkHyperTreeGrid* GetOutput(int idx);
  ///@}

  /**
   * For the specified port, copy the information this reader sets up in
   * SetupOutputInformation to outInfo
   */
  void CopyOutputInformation(vtkInformation* outInfo, int port) override;

protected:
  vtkXMLPHyperTreeGridReader();
  ~vtkXMLPHyperTreeGridReader() override;

  /**
   * Return the type of the dataset being read
   */
  const char* GetDataSetName() override;

  /**
   * Get the number of vertices available in the input.
   */
  vtkIdType GetNumberOfPoints();
  vtkIdType GetNumberOfPointsInPiece(int piece);

  vtkHyperTreeGrid* GetOutputAsHyperTreeGrid();
  vtkHyperTreeGrid* GetPieceInputAsHyperTreeGrid(int piece);

  /**
   * Get the current piece index and the total number of pieces in the dataset
   * Here let's consider a piece to be one hypertreegrid file
   */
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces);

  /**
   * Initialize current output
   */
  void SetupEmptyOutput() override;

  /**
   * Initialize current output data
   */
  void SetupOutputData() override;

  /**
   * Setup the output's information.
   */
  void SetupOutputInformation(vtkInformation* outInfo) override;

  /**
   * Initialize the number of vertices from all the pieces
   */
  void SetupOutputTotals();

  /**
   * no-op
   */
  void SetupNextPiece();

  /**
   * Setup the number of pieces to be read
   */
  void SetupPieces(int numPieces) override;

  /**
   * Setup the extent for the parallel reader and the piece readers.
   */
  void SetupUpdateExtent(int piece, int numberOfPieces);

  /**
   * Setup the readers and then read the input data
   */
  void ReadXMLData() override;

  /**
   * Whether or not the current reader can read the current piece
   */
  int CanReadPiece(int index) override;

  /**
   * Pipeline execute data driver. Called by vtkXMLReader.
   */
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) override;

  /**
   * Delete all piece readers and related information
   */
  void DestroyPieces() override;

  using vtkXMLPDataObjectReader::ReadPiece;

  /**
   * Setup the current piece reader.
   */
  int ReadPiece(vtkXMLDataElement* ePiece) override;

  /**
   * Actually read the current piece data
   */
  int ReadPieceData(int index);
  int ReadPieceData();
  void RecursivelyProcessTree(
    vtkHyperTreeGridNonOrientedCursor* inCursor, vtkHyperTreeGridNonOrientedCursor* outCursor);

  /**
   * Create a reader according to the data to read
   */
  vtkXMLHyperTreeGridReader* CreatePieceReader();

  int FillOutputPortInformation(int, vtkInformation*) override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Callback registered with the PieceProgressObserver.
   */
  void PieceProgressCallback() override;

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

  vtkIdType TotalNumberOfPoints;
  vtkIdType PieceStartIndex;

  vtkXMLHyperTreeGridReader** PieceReaders;

private:
  vtkXMLPHyperTreeGridReader(const vtkXMLPHyperTreeGridReader&) = delete;
  void operator=(const vtkXMLPHyperTreeGridReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
