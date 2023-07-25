// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
 *
 * For developers:
 * To ensure the durability of this storage format over time, at least,
 * the drive must continue to support playback of previous format.
 *
 * Understand:
 * - version 0.0 (P. Fasel and D. DeMarle Kitware US)
 * - version 1.0 (J-B Lekien CEA, DAM, DIF, F-91297 Arpajon, France)
 *   This version of the format offers extensive loading options.
 *   With these options, regardless of the size of the backed-up mesh,
 *   it is possible to view a "reduced" version either by setting the
 *   maximum level (by SetFixedLevel) or/and setting the HyperTrees
 *   to load (by SetCoordinatesBoundingBox, SetIndicesBoundingBox,
 *   ClearAndAddSelectedHT and AddSelectedHT.
 */

#ifndef vtkXMLHyperTreeGridReader_h
#define vtkXMLHyperTreeGridReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLReader.h"

#include <limits.h> // Use internal
#include <map>      // Use internal

VTK_ABI_NAMESPACE_BEGIN
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

  ///@{
  /**
   * Get the reader's output.
   */
  vtkHyperTreeGrid* GetOutput();
  vtkHyperTreeGrid* GetOutput(int idx);
  ///@}

  ///@{
  /**
   * Set/Get the fixed level to read.
   * Option available in 1.0
   */
  vtkSetMacro(FixedLevel, unsigned int);
  vtkGetMacro(FixedLevel, unsigned int);
  ///@}

  ///@{
  /**
   * Set/Get the selected HyperTrees (HTs) to read :
   * by default, all Hts, or
   * by set coordinates bounding box, exclusive or
   * by set indices coordinates bounding box, exclusive or
   * by set indices HTs (ClearAndAdd and more Add).
   * Only available for files whose major version > 1
   * Option available in 1.0
   */
  void SetCoordinatesBoundingBox(
    double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);

  void SetIndicesBoundingBox(unsigned int imin, unsigned int imax, unsigned int jmin,
    unsigned int jmax, unsigned int kmin, unsigned int kmax);

  void ClearAndAddSelectedHT(unsigned int idg, unsigned int fixedLevel = UINT_MAX);
  void AddSelectedHT(unsigned int idg, unsigned int fixedLevel = UINT_MAX);
  ///@}

  // These defer to the HyperTreeGrid output.
  vtkIdType GetNumberOfPoints() const;

  vtkIdType GetNumberOfPieces() const;

  void SetupUpdateExtent(int piece, int numberOfPieces);

  void CopyOutputInformation(vtkInformation* outInfo, int port) override;

  // The most important stuff is here.
  // Read the rest of the file and create the HyperTreeGrid.
  void ReadXMLData() override;

protected:
  vtkXMLHyperTreeGridReader();
  ~vtkXMLHyperTreeGridReader() override;

  // Finalize the selected HyperTrees by, for example, transform
  // coordinates bounding box in indices coordinates bounding box
  // after initialize HyperTreeGrid.
  void CalculateHTs(const vtkHyperTreeGrid* grid);

  // Return true if HyperTree identified by treeIndx is selected for
  // the load.
  bool IsSelectedHT(const vtkHyperTreeGrid* grid, vtkIdType treeIndx) const;

  // Return the fixedLevel choice for this HyperTree
  unsigned int GetFixedLevelOfThisHT(unsigned int numberOfLevels, vtkIdType treeIndx) const;

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

  //----------- Used for the major version < 1

  // Recover the structure of the HyperTreeGrid, used by ReadXMLData. File
  // format version 0.
  void ReadTrees_0(vtkXMLDataElement* elem);

  // Used by ReadTopology to recursively build the tree
  void SubdivideFromDescriptor_0(vtkHyperTreeGridNonOrientedCursor* treeCursor, unsigned int level,
    unsigned int numChildren, vtkBitArray* desc, vtkIdTypeArray* posByLevel);

  //---------- Used for other the major version

  // Recover the structure of the HyperTreeGrid, used by ReadXMLData. File
  // format version 1.
  void ReadTrees_1(vtkXMLDataElement* elem);

  // Recover the structure of the HyperTreeGrid, used by ReadXMLData. File
  // format version 2.
  void ReadTrees_2(vtkXMLDataElement* elem);

  // Number of vertices in HyperTreeGrid being read
  vtkIdType NumberOfPoints;
  vtkIdType NumberOfPieces;

  // Fixed the load maximum level
  unsigned int FixedLevel;

  bool Verbose = false;

  bool FixedHTs = false;
  enum SelectedType
  {
    ALL,
    COORDINATES_BOUNDING_BOX,
    INDICES_BOUNDING_BOX,
    IDS_SELECTED
  };
  SelectedType SelectedHTs = ALL;

  // Selected HTs by coordinates of bounding box
  double CoordinatesBoundingBox[6];
  // Selected HTs by indice coordinate of bounding box
  unsigned int IndicesBoundingBox[6];
  // Selected HTs by indice of HTs in the map.
  // The value is the fixedLevel, but if this value is
  // UINT_MAX, this is FixedLevel that is used.
  std::map<unsigned int, unsigned int> IdsSelected;

  vtkIdType UpdatedPiece;
  vtkIdType UpdateNumberOfPieces;

  vtkIdType StartPiece;
  vtkIdType EndPiece;
  vtkIdType Piece;

private:
  vtkXMLHyperTreeGridReader(const vtkXMLHyperTreeGridReader&) = delete;
  void operator=(const vtkXMLHyperTreeGridReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
