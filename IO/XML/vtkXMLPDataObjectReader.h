/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataObjectReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPDataObjectReader
 * @brief   Superclass for PVTK XML file readers.
 *
 * vtkXMLPDataObjectReader provides functionality common to all PVTK XML
 * file readers. Concrete subclasses call upon this functionality when needed.
*/

#ifndef vtkXMLPDataObjectReader_h
#define vtkXMLPDataObjectReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLReader.h"

class VTKIOXML_EXPORT vtkXMLPDataObjectReader : public vtkXMLReader
{
public:
  vtkTypeMacro(vtkXMLPDataObjectReader, vtkXMLReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the number of pieces from the summary file being read.
   */
  vtkGetMacro(NumberOfPieces, int);

protected:
  vtkXMLPDataObjectReader();
  ~vtkXMLPDataObjectReader() override;

  /**
   * Delete all piece readers and related information
   */
  virtual void DestroyPieces();

  /**
   * Initialize the output data
   */
  void SetupOutputData() override;

  /**
   * Setup the number of pieces to be read and allocate space accordingly
   */
  virtual void SetupPieces(int numPieces);

  /**
  * Pipeline execute information driver.  Called by vtkXMLReader.
  */
  int ReadXMLInformation() override;

  /**
   * Whether or not the current reader can read the current piece
   */
  virtual int CanReadPiece(int index) = 0;

  /**
   * Setup the piece reader at the given index
   */
  int ReadPiece(vtkXMLDataElement* ePiece, int index);

  /**
   * Setup the current piece reader. It needs to be overridden by subclass.
   */
  virtual int ReadPiece(vtkXMLDataElement* ePiece) = 0;

  //@{
  /**
   * Methods for creating a filename for each piece in the dataset
   */
  char* CreatePieceFileName(const char* fileName);
  void SplitFileName();
  //@}

  //@{
  /**
  * Callback registered with the PieceProgressObserver.
  */
  static void PieceProgressCallbackFunction(vtkObject*, unsigned long, void*, void*);
  virtual void PieceProgressCallback() = 0;
  //@}

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

  //@{
  /**
  * Information per-piece.
  */
  vtkXMLDataElement** PieceElements;
  int* CanReadPieceFlag;
  //@}

  vtkCallbackCommand* PieceProgressObserver;

private:
  vtkXMLPDataObjectReader(const vtkXMLPDataObjectReader&) = delete;
  void operator=(const vtkXMLPDataObjectReader&) = delete;
};

#endif
