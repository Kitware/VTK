// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLPStructuredGridReader
 * @brief   Read PVTK XML StructuredGrid files.
 *
 * vtkXMLPStructuredGridReader reads the PVTK XML StructuredGrid file
 * format.  This reads the parallel format's summary file and then
 * uses vtkXMLStructuredGridReader to read data from the individual
 * StructuredGrid piece files.  Streaming is supported.  The standard
 * extension for this reader's file format is "pvts".
 *
 * @sa
 * vtkXMLStructuredGridReader
 */

#ifndef vtkXMLPStructuredGridReader_h
#define vtkXMLPStructuredGridReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPStructuredDataReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkStructuredGrid;

class VTKIOXML_EXPORT vtkXMLPStructuredGridReader : public vtkXMLPStructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLPStructuredGridReader, vtkXMLPStructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLPStructuredGridReader* New();

  /**
   * Get the reader's output.
   */
  vtkStructuredGrid* GetOutput();

  /**
   * Needed for ParaView
   */
  vtkStructuredGrid* GetOutput(int idx);

protected:
  vtkXMLPStructuredGridReader();
  ~vtkXMLPStructuredGridReader() override;

  vtkStructuredGrid* GetPieceInput(int index);

  void SetupEmptyOutput() override;
  const char* GetDataSetName() override;
  void SetOutputExtent(int* extent) override;
  void GetPieceInputExtent(int index, int* extent) override;
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) override;
  void SetupOutputData() override;
  int ReadPieceData() override;
  vtkXMLDataReader* CreatePieceReader() override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  // The PPoints element with point information.
  vtkXMLDataElement* PPointsElement;

private:
  vtkXMLPStructuredGridReader(const vtkXMLPStructuredGridReader&) = delete;
  void operator=(const vtkXMLPStructuredGridReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
