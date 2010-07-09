/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkDIMACSGraphReader.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkDIMACSGraphReader - reads vtkGraph data from a DIMACS
// formatted file

// .SECTION Description
// vtkDIMACSGraphReader is a source object that reads
// vtkGraph data files from a DIMACS format.
// DIMACS graphs are undirected.
// See webpage for format details.
//  http://dimacs.rutgers.edu/Challenges/
//  http://www.dis.uniroma1.it/~challenge9/format.shtml

// .SECTION See Also
// vtkDIMACSGraphWriter
//

#ifndef _vtkDIMACSGraphReader_h
#define _vtkDIMACSGraphReader_h

#include "vtkGraphAlgorithm.h"
#include "vtkStdString.h" // For string API

class VTK_INFOVIS_EXPORT vtkDIMACSGraphReader : public vtkGraphAlgorithm
{

public:

  static vtkDIMACSGraphReader *New();
  vtkTypeMacro(vtkDIMACSGraphReader, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The DIMACS file name.
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);

  // Description:
  // Vertex attribute array name
  vtkGetStringMacro(VertexAttributeArrayName);
  vtkSetStringMacro(VertexAttributeArrayName);

  // Description:
  // Edge attribute array name
  vtkGetStringMacro(EdgeAttributeArrayName);
  vtkSetStringMacro(EdgeAttributeArrayName);

protected:

  vtkDIMACSGraphReader();
  ~vtkDIMACSGraphReader();

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

  int buildGenericGraph(vtkGraph     * output,
                        vtkStdString & defaultVertexAttrArrayName,
                        vtkStdString & defaultEdgeAttrArrayName);

  int buildColoringGraph(vtkGraph * output);
  int buildMaxflowGraph(vtkGraph * output);

  // Description:
  // Creates directed or undirected output based on Directed flag.
  virtual int RequestDataObject(vtkInformation*,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  int ReadGraphMetaData();

private:

  bool   fileOk;
  bool   Directed;
  char * FileName;
  char * VertexAttributeArrayName;
  char * EdgeAttributeArrayName;

  int numVerts;
  int numEdges;
  vtkStdString dimacsProblemStr;

  vtkDIMACSGraphReader(const vtkDIMACSGraphReader&);  // Not implemented.
  void operator=(const vtkDIMACSGraphReader&);  // Not implemented.
};

#endif // _vtkDIMACSGraphReader_h
