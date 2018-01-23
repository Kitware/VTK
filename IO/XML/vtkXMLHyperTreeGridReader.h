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
 * format.  One rectilinear grid file can be read to produce one
 * output.  Streaming is supported.  The standard extension for this
 * reader's file format is "vto".  This reader is also used to read a
 * single piece of the parallel file format.
 *
*/

#ifndef vtkXMLHyperTreeGridReader_h
#define vtkXMLHyperTreeGridReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLDataReader.h"

class vtkHyperTreeGrid;
class vtkHyperTree;
class vtkHyperTreeCursor;
class vtkCharArray;
class vtkIdTypeArray;

using namespace std;

class VTKIOXML_EXPORT vtkXMLHyperTreeGridReader : public vtkXMLDataReader
{
public:
  vtkTypeMacro(vtkXMLHyperTreeGridReader,vtkXMLDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
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
  ~vtkXMLHyperTreeGridReader() VTK_OVERRIDE;

  const char* GetDataSetName() VTK_OVERRIDE;

  // Setup the output with no data available.  Used in error cases.
  void SetupEmptyOutput() VTK_OVERRIDE;

  // Declare that this reader produces HyperTreeGrids
  int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

  //These defer to the HyperTreeGrid output.
  vtkIdType GetNumberOfPoints() VTK_OVERRIDE;
  vtkIdType GetNumberOfCells() VTK_OVERRIDE;

  // Overridden here to do allocation.
  int ReadArrayForPoints(vtkXMLDataElement* da,
                                 vtkAbstractArray* outArray) VTK_OVERRIDE;
  int ReadArrayForCells(vtkXMLDataElement* da,
                                 vtkAbstractArray* outArray) VTK_OVERRIDE;

  // The most important stuff is here.
  // Read the rest of the file and create the HyperTreeGrid.
  void ReadXMLData() VTK_OVERRIDE;

  // Read the coordinates describing the grid
  void ReadCoordinates(vtkXMLDataElement *elem);

  // Recover the structure of the HyperTreeGrid, used by ReadXMLData.
  void ReadTopology(vtkXMLDataElement *elem);

  // Used by ReadTopology to recusively build the tree
  void SubdivideFromDescriptor(
                          vtkHyperTreeCursor* treeCursor,
                          vtkHyperTree* tree,
                          unsigned int level,
                          int numChildren,
                          vtkCharArray* desc,
                          vtkIdTypeArray* posByLevel,
                          vtkIdType* cellsOnProcessor);

private:
  vtkXMLHyperTreeGridReader(const vtkXMLHyperTreeGridReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLHyperTreeGridReader&) VTK_DELETE_FUNCTION;
};

#endif
