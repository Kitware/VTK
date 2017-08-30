/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class vtkDataArray;
class vtkDataSet;
class vtkXMLDataReader;

class VTKIOXML_EXPORT vtkXMLPDataReader : public vtkXMLPDataObjectReader
{
public:
  vtkTypeMacro(vtkXMLPDataReader, vtkXMLPDataObjectReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
  * For the specified port, copy the information this reader sets up in
  * SetupOutputInformation to outInfo
  */
  void CopyOutputInformation(vtkInformation* outInfo, int port) VTK_OVERRIDE;

protected:
  vtkXMLPDataReader();
  ~vtkXMLPDataReader() VTK_OVERRIDE;

  // Re-use any superclass signatures that we don't override.
  using vtkXMLPDataObjectReader::ReadPiece;

  /**
   * Delete all piece readers and related information
   */
  void DestroyPieces() VTK_OVERRIDE;

  virtual vtkIdType GetNumberOfPoints() = 0;

  virtual vtkIdType GetNumberOfCells() = 0;

  /**
   * Get a given piece input as a dataset, return nullptr if there is none.
   */
  vtkDataSet* GetPieceInputAsDataSet(int piece);

  /**
   * Initialize the output data
   */
  void SetupOutputData() VTK_OVERRIDE;

  /**
  * Pipeline execute information driver.  Called by vtkXMLReader.
  */
  void SetupOutputInformation(vtkInformation* outInfo) VTK_OVERRIDE;

  /**
   * Setup the number of pieces to be read and allocate space accordingly
   */
  void SetupPieces(int numPieces) VTK_OVERRIDE;

  /**
   * Whether or not the current reader can read the current piece
   */
  int CanReadPiece(int index) VTK_OVERRIDE;

  /**
   * Create a reader according to the data to read. It needs to be overridden by subclass.
   */
  virtual vtkXMLDataReader* CreatePieceReader() = 0;

  /**
   * Setup the current piece reader
   */
  int ReadPiece(vtkXMLDataElement* ePiece) VTK_OVERRIDE;

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
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) VTK_OVERRIDE;

  virtual void CopyArrayForPoints(vtkDataArray* inArray, vtkDataArray* outArray) = 0;
  virtual void CopyArrayForCells(vtkDataArray* inArray, vtkDataArray* outArray) = 0;

  /**
  * Callback registered with the PieceProgressObserver.
  */
  void PieceProgressCallback() VTK_OVERRIDE;

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
  vtkXMLPDataReader(const vtkXMLPDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPDataReader&) VTK_DELETE_FUNCTION;
};

#endif
