// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLStatisticalModelReader
 * @brief   Read VTK XML StatisticalModel (.vtstat) files.
 *
 * vtkXMLStatisticalModelReader provides a functionality for reading .vtstat files as
 * vtkStatisticalModel instances.
 *
 */

#ifndef vtkXMLStatisticalModelReader_h
#define vtkXMLStatisticalModelReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLReader.h"

#include <map> // needed for std::map

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkIdTypeArray;
class vtkUnsignedCharArray;
class vtkStatisticalModel;

class VTKIOXML_EXPORT vtkXMLStatisticalModelReader : public vtkXMLReader
{
public:
  vtkTypeMacro(vtkXMLStatisticalModelReader, vtkXMLReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLStatisticalModelReader* New();

  ///@{
  /**
   * Get the reader's output.
   */
  vtkStatisticalModel* GetOutput();
  vtkStatisticalModel* GetOutput(int idx);
  ///@}

  /**
   * For the specified port, copy the information this reader sets up in
   * SetupOutputInformation to outInfo
   */
  void CopyOutputInformation(vtkInformation* outInfo, int port) override;

protected:
  vtkXMLStatisticalModelReader();
  ~vtkXMLStatisticalModelReader() override;

  /**
   * Get the name of the data set being read.
   */
  const char* GetDataSetName() override;

  /**
   * Initialize current output
   */
  void SetupEmptyOutput() override;

  /**
   * Setup the output's information.
   */
  void SetupOutputInformation(vtkInformation* outInfo) override;

  /**
   * Pipeline execute data driver.  Called by vtkXMLReader.
   */
  void ReadXMLData() override;

  /**
   * Pipeline execute data driver. Called by vtkXMLReader.
   */
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) override;

  int FillOutputPortInformation(int, vtkInformation*) override;

private:
  /**
   * Setup the current piece reader.
   */
  int ReadPiece(vtkXMLDataElement* ePiece);

  /**
   * Actually read the current piece data
   */
  int ReadPieceData(int);

  // XML elements for the current (lone) piece:
  vtkXMLDataElement* ParamElement;
  std::vector<vtkXMLDataElement*> TableGroupElements;

  vtkXMLStatisticalModelReader(const vtkXMLStatisticalModelReader&) = delete;
  void operator=(const vtkXMLStatisticalModelReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
