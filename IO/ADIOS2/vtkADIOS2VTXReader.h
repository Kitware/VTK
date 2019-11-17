/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkADIOS2VTXReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

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
class VTXSchemaManager;
}

class vtkIndent;
class vtkInformation;
class vtkInformationvector;

class VTKIOADIOS2_EXPORT vtkADIOS2VTXReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkADIOS2VTXReader* New();
  vtkTypeMacro(vtkADIOS2VTXReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent index) override;

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkADIOS2VTXReader();
  ~vtkADIOS2VTXReader() = default;

  vtkADIOS2VTXReader(const vtkADIOS2VTXReader&) = delete;
  void operator=(const vtkADIOS2VTXReader&) = delete;

  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector);
  int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector);
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector);

private:
  char* FileName;
  std::unique_ptr<vtx::VTXSchemaManager> SchemaManager;
};

#endif /* vtkADIOS2VTXReader_h */
