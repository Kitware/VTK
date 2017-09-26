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
  void PrintSelf(ostream& os, vtkIndent indent) override;
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
  ~vtkXMLHyperOctreeReader() override;

  const char* GetDataSetName() override;

  // Setup the output with no data available.  Used in error cases.
  void SetupEmptyOutput() override;

  // Declare that this reader produces HyperOctrees
  int FillOutputPortInformation(int, vtkInformation*) override;

  //These defer to the HyperOctree output.
  vtkIdType GetNumberOfPoints() override;
  vtkIdType GetNumberOfCells() override;

  // Overridden here to do allocation.
  int ReadArrayForPoints(vtkXMLDataElement* da,
                                 vtkAbstractArray* outArray) override;
  int ReadArrayForCells(vtkXMLDataElement* da,
                                vtkAbstractArray* outArray) override;



  // The most important stuff is here.
  // Read the rest of the file and create the HyperOctree.
  void ReadXMLData() override;

  // Recover the structure of the HyperOctree, used by ReadXMLData.
  void ReadTopology(vtkXMLDataElement *elem);

  // Used by ReadTopology to recusively build the tree, one cell at a time.
  int BuildNextCell(vtkIntArray *, vtkHyperOctreeCursor *, int);

  //Helper for BuildNextCell
  int ArrayIndex;

private:
  vtkXMLHyperOctreeReader(const vtkXMLHyperOctreeReader&) = delete;
  void operator=(const vtkXMLHyperOctreeReader&) = delete;
};

#endif
