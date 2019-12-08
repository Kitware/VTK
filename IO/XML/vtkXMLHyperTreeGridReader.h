/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLHyperTreeGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLHyperTreeGridReader
 * @brief   Read VTK XML HyperTreeGrid files.
 *
 * vtkXMLHyperTreeGridReader reads the VTK XML HyperTreeGrid file
 * format. The standard extension for this reader's file format is "htg".
 *
 * NOTE: HyperTree exists as separate units with all data within htg
 *       But each htg file is considered one piece for the parallel reader
 *       Later may want to treat individual HyperTrees as separate pieces.
 */

#ifndef vtkXMLHyperTreeGridReader_h
#define vtkXMLHyperTreeGridReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLReader.h"

class vtkBitArray;
class vtkHyperTree;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;
class vtkIdTypeArray;

class VTKIOXML_EXPORT vtkXMLHyperTreeGridReader : public vtkXMLReader
{
public:
  vtkTypeMacro(vtkXMLHyperTreeGridReader, vtkXMLReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLHyperTreeGridReader* New();

  //@{
  /**
   * Get the reader's output.
   */
  vtkHyperTreeGrid* GetOutput();
  vtkHyperTreeGrid* GetOutput(int idx);
  //@}

  // These defer to the HyperTreeGrid output.
  vtkIdType GetNumberOfPoints();

  vtkIdType GetNumberOfPieces();

  void SetupUpdateExtent(int piece, int numberOfPieces);

  void CopyOutputInformation(vtkInformation* outInfo, int port) override;

  // The most important stuff is here.
  // Read the rest of the file and create the HyperTreeGrid.
  void ReadXMLData() override;

protected:
  vtkXMLHyperTreeGridReader();
  ~vtkXMLHyperTreeGridReader() override;

  const char* GetDataSetName() override;

  void DestroyPieces();

  void GetOutputUpdateExtent(int& piece, int& numberOfPieces);

  // Setup the output with no data available.  Used in error cases.
  void SetupEmptyOutput() override;

  // Initialize the total number of vertices
  void SetupOutputTotals();

  // Initialize global start of next piece
  void SetupNextPiece();

  // Initialize current output data
  void SetupOutputData() override;

  // Setup the output's information
  void SetupOutputInformation(vtkInformation* outInfo) override;

  // Setup the number of pieces
  void SetupPieces(int numPieces);

  // Pipeline execute data driver called by vtkXMLReader
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) override;

  // Declare that this reader produces HyperTreeGrids
  int FillOutputPortInformation(int, vtkInformation*) override;

  // Read the coordinates describing the grid
  void ReadGrid(vtkXMLDataElement* elem);

  // Recover the structure of the HyperTreeGrid, used by ReadXMLData.
  void ReadTrees(vtkXMLDataElement* elem);

  // Used by ReadTopology to recursively build the tree
  void SubdivideFromDescriptor(vtkHyperTreeGridNonOrientedCursor* treeCursor, unsigned int level,
    int numChildren, vtkBitArray* desc, vtkIdTypeArray* posByLevel);

  // Number of vertices in HyperTreeGrid being read
  vtkIdType NumberOfPoints;
  vtkIdType NumberOfPieces;

  int UpdatedPiece;
  int UpdateNumberOfPieces;

  int StartPiece;
  int EndPiece;
  int Piece;

private:
  vtkXMLHyperTreeGridReader(const vtkXMLHyperTreeGridReader&) = delete;
  void operator=(const vtkXMLHyperTreeGridReader&) = delete;
};

#endif
