/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNewickTreeWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNewickTreeWriter - write vtkTree data to Newick format.
// .SECTION Description
// vtkNewickTreeWriter is writes a vtkTree to a Newick formatted file
// or string.

#ifndef vtkNewickTreeWriter_h
#define vtkNewickTreeWriter_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkDataWriter.h"
#include "vtkStdString.h"       // For get/set ivars

class vtkTree;

class VTKIOINFOVIS_EXPORT vtkNewickTreeWriter : public vtkDataWriter
{
public:
  static vtkNewickTreeWriter *New();
  vtkTypeMacro(vtkNewickTreeWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the input to this writer.
  vtkTree* GetInput();
  vtkTree* GetInput(int port);

  // Description:
  // Get/Set the name of the input's tree edge weight array.
  // This array must be part of the input tree's EdgeData.
  // The default name is "weight".  If this array cannot be
  // found, then no edge weights will be included in the
  // output of this writer.
  vtkGetMacro(EdgeWeightArrayName, vtkStdString);
  vtkSetMacro(EdgeWeightArrayName, vtkStdString);

  // Description:
  // Get/Set the name of the input's tree node name array.
  // This array must be part of the input tree's VertexData.
  // The default name is "node name".  If this array cannot
  // be found, then no node names will be included in the
  // output of this writer.
  vtkGetMacro(NodeNameArrayName, vtkStdString);
  vtkSetMacro(NodeNameArrayName, vtkStdString);

protected:
  vtkNewickTreeWriter();
  ~vtkNewickTreeWriter() {}

  void WriteData();

  // Description:
  // Write one vertex.  This function calls itself recursively for
  // any children of the input vertex.
  void WriteVertex(ostream *fp, vtkTree* const input, vtkIdType vertex);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkStdString EdgeWeightArrayName;
  vtkStdString NodeNameArrayName;

  vtkAbstractArray *EdgeWeightArray;
  vtkAbstractArray *NodeNameArray;

private:
  vtkNewickTreeWriter(const vtkNewickTreeWriter&);  // Not implemented.
  void operator=(const vtkNewickTreeWriter&);  // Not implemented.
};

#endif
