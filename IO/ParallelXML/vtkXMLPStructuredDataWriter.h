// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include <map>    // for keeping track of extents
#include <vector> // for keeping track of extents

VTK_ABI_NAMESPACE_BEGIN
class vtkXMLStructuredDataWriter;

class VTKIOPARALLELXML_EXPORT vtkXMLPStructuredDataWriter : public vtkXMLPDataWriter
{
public:
  vtkTypeMacro(vtkXMLPStructuredDataWriter, vtkXMLPDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkXMLPStructuredDataWriter();
  ~vtkXMLPStructuredDataWriter() override;

  virtual vtkXMLStructuredDataWriter* CreateStructuredPieceWriter() = 0;
  void WritePrimaryElementAttributes(ostream& os, vtkIndent indent) override;
  void WritePPieceAttributes(int index) override;
  vtkXMLWriter* CreatePieceWriter(int index) override;

  int WriteInternal() override;

  void PrepareSummaryFile() override;
  int WritePiece(int index) override;

private:
  vtkXMLPStructuredDataWriter(const vtkXMLPStructuredDataWriter&) = delete;
  void operator=(const vtkXMLPStructuredDataWriter&) = delete;

  typedef std::map<int, std::vector<int>> ExtentsType;
  ExtentsType Extents;
};

VTK_ABI_NAMESPACE_END
#endif
