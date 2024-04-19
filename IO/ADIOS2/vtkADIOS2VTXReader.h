// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/*
 * vtkADIOS2VTXReader.h  public facing class
 *                     enables reading adios2 bp files using the
 *                     VTK ADIOS2 Readers (VTX) developed
 *                     at Oak Ridge National Laboratory
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef vtkADIOS2VTXReader_h
#define vtkADIOS2VTXReader_h

#include <memory> // std::unique_ptr

#include "vtkIOADIOS2Module.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

// forward declaring to keep it private
namespace vtx
{
VTK_ABI_NAMESPACE_BEGIN
class VTXSchemaManager;
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN

class vtkIndent;
class vtkInformation;
class vtkInformationvector;

class VTKIOADIOS2_EXPORT vtkADIOS2VTXReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkADIOS2VTXReader* New();
  vtkTypeMacro(vtkADIOS2VTXReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);

protected:
  vtkADIOS2VTXReader();
  ~vtkADIOS2VTXReader() override;

  vtkADIOS2VTXReader(const vtkADIOS2VTXReader&) = delete;
  void operator=(const vtkADIOS2VTXReader&) = delete;

  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override;
  int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override;
  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override;

private:
  char* FileName;
  std::unique_ptr<vtx::VTXSchemaManager> SchemaManager;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkADIOS2VTXReader_h */
