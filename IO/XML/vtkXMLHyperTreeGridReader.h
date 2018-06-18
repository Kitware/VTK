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
 * format. The standard extension for this
 * reader's file format is "htg".
 *
*/

#ifndef vtkXMLHyperTreeGridReader_h
#define vtkXMLHyperTreeGridReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLDataReader.h"

class vtkBitArray;
class vtkHyperTree;
class vtkHyperTreeCursor;
class vtkHyperTreeGrid;
class vtkIdTypeArray;

class VTKIOXML_EXPORT vtkXMLHyperTreeGridReader : public vtkXMLDataReader
{
public:
  vtkTypeMacro(vtkXMLHyperTreeGridReader,vtkXMLDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLHyperTreeGridReader *New();

  //@{
  /**
   * Get the reader's output.
   */
  vtkHyperTreeGrid *GetOutput();
  vtkHyperTreeGrid *GetOutput(int idx);
  //@}

protected:
  vtkXMLHyperTreeGridReader();
  ~vtkXMLHyperTreeGridReader() override;

  const char* GetDataSetName() override;

  // Setup the output with no data available.  Used in error cases.
  void SetupEmptyOutput() override;

  // Declare that this reader produces HyperTreeGrids
  int FillOutputPortInformation(int, vtkInformation*) override;

  //These defer to the HyperTreeGrid output.
  vtkIdType GetNumberOfPoints() override;
  vtkIdType GetNumberOfCells() override;

  // Overridden here to do allocation.
  int ReadArrayForPoints(vtkXMLDataElement* da,
                         vtkAbstractArray* outArray) override;
  int ReadArrayForCells(vtkXMLDataElement* da,
                        vtkAbstractArray* outArray) override;

  // The most important stuff is here.
  // Read the rest of the file and create the HyperTreeGrid.
  void ReadXMLData() override;

  // Read the coordinates describing the grid
  void ReadCoordinates(vtkXMLDataElement *elem);

  // Recover the structure of the HyperTreeGrid, used by ReadXMLData.
  void ReadTopology(vtkXMLDataElement *elem);

protected:
  // Used by ReadTopology to recursively build the tree
  void SubdivideFromDescriptor(
                          vtkHyperTreeCursor* treeCursor,
                          vtkHyperTree* tree,
                          unsigned int level,
                          int numChildren,
                          vtkBitArray* desc,
                          vtkIdTypeArray* posByLevel,
                          vtkIdType* cellsOnProcessor);


private:
  vtkXMLHyperTreeGridReader(const vtkXMLHyperTreeGridReader&) = delete;
  void operator=(const vtkXMLHyperTreeGridReader&) = delete;
};

#endif
