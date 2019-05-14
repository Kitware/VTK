/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * vtkADIOS2ReaderDriver.h  public facing class, to be used in a private setting
 * (e.g. forward declare on .h, include in .cxx or .cpp)
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTKADIOS2READERDRIVER_H_
#define VTKADIOS2READERDRIVER_H_

#include <memory>
#include <set>
#include <string>

#include "vtkIndent.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockDataSetAlgorithm.h"

// For export macro
#include "vtkIOADIOS2Module.h"

namespace adios2vtk
{
class ADIOS2Schema;
}

class VTKIOADIOS2_EXPORT vtkADIOS2Reader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkADIOS2Reader* New();
  vtkTypeMacro(vtkADIOS2Reader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent index);

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkADIOS2Reader();
  ~vtkADIOS2Reader() = default;

  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector);
  int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector);
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector);

private:
  char* FileName;

  class Impl;
  std::unique_ptr<Impl> m_Impl;
};

#endif
