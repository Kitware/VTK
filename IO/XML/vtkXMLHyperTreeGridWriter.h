/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLHyperTreeGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLHyperTreeGridWriter
 * @brief   Write VTK XML HyperTreeGrid files.
 *
 * vtkXMLHyperTreeGridWriter writes the VTK XML HyperTreeGrid file
 * format. The standard extension for this writer's file format is "htg".
 */

#ifndef vtkXMLHyperTreeGridWriter_h
#define vtkXMLHyperTreeGridWriter_h

#include "vtkBitArray.h"    // For ivar
#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

#include <vector> // std::vector

class OffsetsManagerGroup;
class OffsetsManagerArray;
class vtkHyperTree;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;

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

protected:
  vtkXMLHyperTreeGridWriter();
  ~vtkXMLHyperTreeGridWriter() override;

  const char* GetDataSetName() override;

  // specify that we require HyperTreeGrid input
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // The most important method, make the XML file for my input.
  int WriteData() override;

  // <HyperTreeGrid ...
  int StartPrimaryElement(vtkIndent);

  // ... dim, size, origin>
  void WritePrimaryElementAttributes(ostream&, vtkIndent) override;

  // Grid coordinates and mask
  int WriteGrid(vtkIndent);

  // Tree Descriptor and  PointData
  int WriteTrees(vtkIndent);

  // </HyperTreeGrid>
  int FinishPrimaryElement(vtkIndent);

  // Descriptors for individual hypertrees
  std::vector<vtkBitArray*> Descriptors;

  // Masks for individual hypertrees
  std::vector<vtkBitArray*> Masks;

  // Helper to simplify writing appended array data
  void WriteAppendedArrayDataHelper(vtkAbstractArray* array, OffsetsManager& offsets);
  void WritePointDataAppendedArrayDataHelper(
    vtkAbstractArray* array, vtkIdType treeCount, OffsetsManager& offsets, vtkHyperTree* tree);

  OffsetsManagerGroup* CoordsOMG;
  OffsetsManagerGroup* DescriptorOMG;
  OffsetsManagerGroup* MaskOMG;
  OffsetsManagerGroup* PointDataOMG;

  int NumberOfTrees;

private:
  vtkXMLHyperTreeGridWriter(const vtkXMLHyperTreeGridWriter&) = delete;
  void operator=(const vtkXMLHyperTreeGridWriter&) = delete;
};

#endif
