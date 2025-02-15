// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLPDataReader
 * @brief   Superclass for PVTK XML file readers that read vtkDataSets.
 *
 * vtkXMLPDataReader provides functionality common to all PVTK XML
 * file readers that read vtkDataSets. Concrete subclasses call upon
 * this functionality when needed.
 *
 * @sa
 * vtkXMLDataReader
 */

#ifndef vtkXMLPDataReader_h
#define vtkXMLPDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPDataObjectReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkDataSet;
class vtkXMLDataReader;

class VTKIOXML_EXPORT vtkXMLPDataReader : public vtkXMLPDataObjectReader
{
public:
  vtkTypeMacro(vtkXMLPDataReader, vtkXMLPDataObjectReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * For the specified port, copy the information this reader sets up in
   * SetupOutputInformation to outInfo
   */
  void CopyOutputInformation(vtkInformation* outInfo, int port) override;

protected:
  vtkXMLPDataReader();
  ~vtkXMLPDataReader() override;

  // Reuse any superclass signatures that we don't override.
  using vtkXMLPDataObjectReader::ReadPiece;

  /**
   * Delete all piece readers and related information
   */
  void DestroyPieces() override;

  virtual vtkIdType GetNumberOfPoints() = 0;

  virtual vtkIdType GetNumberOfCells() = 0;

  /**
   * Get a given piece input as a dataset, return nullptr if there is none.
   */
  vtkDataSet* GetPieceInputAsDataSet(int piece);

  /**
   * Initialize the output data
   */
  void SetupOutputData() override;

  /**
   * Pipeline execute information driver.  Called by vtkXMLReader.
   */
  void SetupOutputInformation(vtkInformation* outInfo) override;

  /**
   * Setup the number of pieces to be read and allocate space accordingly
   */
  void SetupPieces(int numPieces) override;

  /**
   * Whether or not the current reader can read the current piece
   */
  int CanReadPiece(int index) override;

  /**
   * Create a reader according to the data to read. It needs to be overridden by subclass.
   */
  virtual vtkXMLDataReader* CreatePieceReader() = 0;

  /**
   * Setup the current piece reader
   */
  int ReadPiece(vtkXMLDataElement* ePiece) override;

  /**
   * Actually read the piece at the given index data
   */
  int ReadPieceData(int index);

  /**
   * Actually read the current piece data
   */
  virtual int ReadPieceData();

  /**
   * Read the information relative to the dataset and allocate the needed structures according to it
   */
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) override;

  virtual void CopyArrayForPoints(vtkAbstractArray* inArray, vtkAbstractArray* outArray) = 0;
  virtual void CopyArrayForCells(vtkAbstractArray* inArray, vtkAbstractArray* outArray) = 0;

  /**
   * Callback registered with the PieceProgressObserver.
   */
  void PieceProgressCallback() override;

  /**
   * The ghost level available on each input piece.
   */
  int GhostLevel;

  /**
   * Information per-piece.
   */
  vtkXMLDataReader** PieceReaders;

  /**
   * The PPointData and PCellData element representations.
   */
  vtkXMLDataElement* PPointDataElement;
  vtkXMLDataElement* PCellDataElement;

private:
  vtkXMLPDataReader(const vtkXMLPDataReader&) = delete;
  void operator=(const vtkXMLPDataReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
