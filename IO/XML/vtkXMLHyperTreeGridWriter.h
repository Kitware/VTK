// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLHyperTreeGridWriter
 * @brief   Write VTK XML HyperTreeGrid files.
 *
 * vtkXMLHyperTreeGridWriter writes the VTK XML HyperTreeGrid file
 * format. The standard extension for this writer's file format is "htg".
 *
 * Note for developers:
 * The VTK XML HyperTreeGrid file format is versioned.
 * Any evolution of the format must lead to:
 * - a move to a higher major version number, X+1.0, if the evolution is
 *   incompatible with the previous versions either at the level of the
 *   description of the information or the semantic understanding made by
 *   the reader;
 * - a move to a higher minor version number, X.y+1, if its consists of
 *   adding information without calling into question the general
 *   interpretation.
 *
 * Version 0.0 (P. Fasel and D. DeMarle)
 * ...
 * Version 1.0 (J-B Lekien CEA, DAM, DIF, F-91297 Arpajon, France)
 * - writing by HyperTree description and fields;
 * - saving the minimal tree (a hidden refined node becomes a hidden leaf node);
 * - saving the tree by level of refinement (course in width);
 * - the last null values in the binary description of the tree or mask (if
 *   defined) may not / are not explicitly described. The size of the table
 *   given elsewhere is authentic;
 * - all fields are copied to be saved in the implicit order, so even
 *   if an explicit global index map exists, it disappears;
 * - writing in this version requires more memory and CPU;
 * - reading of a part is accelerated (non iterative construction of the tree)
 *   and consumes potentially less memory (suppression of the global index
 *   map explicit);
 * - expanded possibility at the reader level, today these options allow to
 *   accelerate the obtaining of a result which will be less precise and to
 *   allow the loading of a part of a mesh which would not hold in memory:
 *      - loading by limiting the maximum level to load;
 *      - loading by selecting (different description possibilities are
 *        offered) the HTs to take into account.
 *
 * The default version of the VTK XML HyperTreeGrid file format is the latest
 * version, now version 1.0.
 *
 * For developers:
 * To ensure the durability of this storage format over time, at least, the drive
 * must continue to support playback of previous format.
 */

#ifndef vtkXMLHyperTreeGridWriter_h
#define vtkXMLHyperTreeGridWriter_h

#include "vtkIOXMLModule.h"  // For export macro
#include "vtkSmartPointer.h" // For internal attributes
#include "vtkXMLWriter.h"

#include <vector> // std::vector

VTK_ABI_NAMESPACE_BEGIN
class OffsetsManagerGroup;
class OffsetsManagerArray;
class vtkBitArray;
class vtkIdList;
class vtkHyperTree;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;
class vtkTypeInt64Array;
class vtkTypeUInt32Array;

class VTKIOXML_EXPORT vtkXMLHyperTreeGridWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLHyperTreeGridWriter, vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLHyperTreeGridWriter* New();

  /**
   * Get/Set the writer's input.
   */
  vtkHyperTreeGrid* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

  /**
   * Methods to define the file's major and minor version numbers.
   * Major version incremented since v0.1 hypertreegrid data readers
   * cannot read the files written by this new reader.
   * A version is defined by defect, so there is no need to call
   * this function.
   * The default choice is usually the best choice.
   */
  vtkSetMacro(DataSetMajorVersion, int);
  vtkSetMacro(DataSetMinorVersion, int);

protected:
  vtkXMLHyperTreeGridWriter();
  ~vtkXMLHyperTreeGridWriter() override;

  const char* GetDataSetName() override;

  /**
   * Methods to define the file's major and minor version numbers.
   * Major version incremented since v0.1 hypertreegrid data readers
   * cannot read the files written by this new reader.
   * Actually, the default version is 1.0
   */
  int GetDataSetMajorVersion() override { return DataSetMajorVersion; }
  int GetDataSetMinorVersion() override { return DataSetMinorVersion; }

  // Specify that we require HyperTreeGrid input
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // The most important method, make the XML file for my input.
  int WriteData() override;

  // <HyperTreeGrid ...
  int StartPrimaryElement(vtkIndent);

  // ... dim, size, origin>
  void WritePrimaryElementAttributes(ostream&, vtkIndent) override;

  // Grid coordinates and mask
  int WriteGrid(vtkIndent);

  // Tree Descriptor and  CellData
  int WriteTrees_0(vtkIndent);
  int WriteTrees_1(vtkIndent);
  int WriteTrees_2(vtkIndent);

  // </HyperTreeGrid>
  int FinishPrimaryElement(vtkIndent);

  // Descriptors for individual hypertrees
  std::vector<vtkSmartPointer<vtkBitArray>> Descriptors;

  // Descriptors for individual hypertrees
  std::vector<vtkSmartPointer<vtkTypeInt64Array>> NbVerticesByLevels;

  // Masks for individual hypertrees
  std::vector<vtkSmartPointer<vtkBitArray>> Masks;

  // Ids (index selection) for individual hypertrees
  std::vector<vtkSmartPointer<vtkIdList>> Ids;

  // Helper to simplify writing appended array data
  void WriteAppendedArrayDataHelper(vtkAbstractArray* array, OffsetsManager& offsets);

  void WriteCellDataAppendedArrayDataHelper(vtkAbstractArray* array, vtkIdType numberOfVertices,
    OffsetsManager& offsets, vtkHyperTree* tree);

  struct HyperTreeGridMetaDataForVersion2
  {
    void Initialize();

    vtkSmartPointer<vtkBitArray> Descriptors;
    vtkSmartPointer<vtkTypeInt64Array> TreeIds;
    vtkSmartPointer<vtkTypeInt64Array> NumberOfVerticesPerDepth;
    vtkSmartPointer<vtkIdList> BreadthFirstIdMap;
    vtkSmartPointer<vtkTypeUInt32Array> DepthPerTree;
  } MetaDataForVersion2;

  OffsetsManagerGroup* CoordsOMG;
  OffsetsManagerGroup* DescriptorOMG;
  OffsetsManagerGroup* NbVerticesByLevelOMG;
  OffsetsManagerGroup* MaskOMG;
  OffsetsManagerGroup* CellDataOMG;
  OffsetsManagerGroup* TreeIdsOMG;
  OffsetsManagerGroup* DepthPerTreeOMG;

  int NumberOfTrees;

  // Default choice
  int DataSetMajorVersion;
  int DataSetMinorVersion;

private:
  vtkXMLHyperTreeGridWriter(const vtkXMLHyperTreeGridWriter&) = delete;
  void operator=(const vtkXMLHyperTreeGridWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
