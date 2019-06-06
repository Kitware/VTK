/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkADIO2ReaderMultiBlock.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * vtkADIOS2ReaderMultiBlock.h  public facing class used by Paraview plugin
 *                              enables reading adios2 bp files with a
 *                              vtk.xml attribute or file
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef vtkADIOS2ReaderMultiBlock_h
#define vtkADIOS2ReaderMultiBlock_h

#include <memory> //std::unique_ptr

#include "vtkIOADIOS2Module.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

// forward declaring to keep it private
namespace adios2vtk
{
class ADIOS2SchemaManager;
}

class vtkIndent;
class vtkInformation;
class vtkInformationvector;

class VTKIOADIOS2_EXPORT vtkADIOS2ReaderMultiBlock : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkADIOS2ReaderMultiBlock* New();
  vtkTypeMacro(vtkADIOS2ReaderMultiBlock, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent index) override;

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkADIOS2ReaderMultiBlock();
  ~vtkADIOS2ReaderMultiBlock() = default;

  vtkADIOS2ReaderMultiBlock(const vtkADIOS2ReaderMultiBlock&) = delete;
  void operator=(const vtkADIOS2ReaderMultiBlock&) = delete;

  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector);
  int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector);
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector);

private:
  char* FileName;
  std::unique_ptr<adios2vtk::ADIOS2SchemaManager> SchemaManager;
};

#endif
