/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPStructuredDataWriter
 * @brief   Superclass for PVTK XML structured data writers.
 *
 * vtkXMLPStructuredDataWriter provides PVTK XML writing functionality
 * that is common among all the parallel structured data formats.
*/

#ifndef vtkXMLPStructuredDataWriter_h
#define vtkXMLPStructuredDataWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPDataWriter.h"

#include <map> // for keeping track of extents
#include <vector> // for keeping track of extents

class vtkXMLStructuredDataWriter;

class VTKIOPARALLELXML_EXPORT vtkXMLPStructuredDataWriter : public vtkXMLPDataWriter
{
public:
  vtkTypeMacro(vtkXMLPStructuredDataWriter,vtkXMLPDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkXMLPStructuredDataWriter();
  ~vtkXMLPStructuredDataWriter() VTK_OVERRIDE;

  virtual vtkXMLStructuredDataWriter* CreateStructuredPieceWriter()=0;
  void WritePrimaryElementAttributes(ostream &os, vtkIndent indent) VTK_OVERRIDE;
  void WritePPieceAttributes(int index) VTK_OVERRIDE;
  vtkXMLWriter* CreatePieceWriter(int index) VTK_OVERRIDE;

  int WriteInternal() VTK_OVERRIDE;

  void PrepareSummaryFile() VTK_OVERRIDE;
  int WritePiece(int index) VTK_OVERRIDE;

private:
  vtkXMLPStructuredDataWriter(const vtkXMLPStructuredDataWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPStructuredDataWriter&) VTK_DELETE_FUNCTION;

  typedef std::map<int, std::vector<int> > ExtentsType;
  ExtentsType Extents;
};

#endif
