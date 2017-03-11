/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLHyperOctreeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLHyperOctreeReader
 * @brief   Read VTK XML HyperOctree files.
 *
 * vtkXMLHyperOctreeReader reads the VTK XML HyperOctree file
 * format.  One rectilinear grid file can be read to produce one
 * output.  Streaming is supported.  The standard extension for this
 * reader's file format is "vto".  This reader is also used to read a
 * single piece of the parallel file format.
 *
 * @sa
 * vtkXMLPHyperOctreeReader
*/

#ifndef vtkXMLHyperOctreeReader_h
#define vtkXMLHyperOctreeReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLDataReader.h"

class vtkHyperOctree;
class vtkHyperOctreeCursor;
class vtkIntArray;

class VTKIOXML_EXPORT vtkXMLHyperOctreeReader : public vtkXMLDataReader
{
public:
  vtkTypeMacro(vtkXMLHyperOctreeReader,vtkXMLDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkXMLHyperOctreeReader *New();

  //@{
  /**
   * Get the reader's output.
   */
  vtkHyperOctree *GetOutput();
  vtkHyperOctree *GetOutput(int idx);
  //@}

protected:
  vtkXMLHyperOctreeReader();
  ~vtkXMLHyperOctreeReader() VTK_OVERRIDE;

  const char* GetDataSetName() VTK_OVERRIDE;

  // Setup the output with no data available.  Used in error cases.
  void SetupEmptyOutput() VTK_OVERRIDE;

  // Declare that this reader produces HyperOctrees
  int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

  //These defer to the HyperOctree output.
  vtkIdType GetNumberOfPoints() VTK_OVERRIDE;
  vtkIdType GetNumberOfCells() VTK_OVERRIDE;

  // Overridden here to do allocation.
  int ReadArrayForPoints(vtkXMLDataElement* da,
                                 vtkAbstractArray* outArray) VTK_OVERRIDE;
  int ReadArrayForCells(vtkXMLDataElement* da,
                                vtkAbstractArray* outArray) VTK_OVERRIDE;



  // The most important stuff is here.
  // Read the rest of the file and create the HyperOctree.
  void ReadXMLData() VTK_OVERRIDE;

  // Recover the structure of the HyperOctree, used by ReadXMLData.
  void ReadTopology(vtkXMLDataElement *elem);

  // Used by ReadTopology to recusively build the tree, one cell at a time.
  int BuildNextCell(vtkIntArray *, vtkHyperOctreeCursor *, int);

  //Helper for BuildNextCell
  int ArrayIndex;

private:
  vtkXMLHyperOctreeReader(const vtkXMLHyperOctreeReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLHyperOctreeReader&) VTK_DELETE_FUNCTION;
};

#endif
