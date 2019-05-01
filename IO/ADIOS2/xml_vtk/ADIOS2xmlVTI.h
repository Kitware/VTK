/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS2VTI.h
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_ADIOS2xmlVTI_H_
#define VTK_IO_ADIOS2_ADIOS2xmlVTI_H_

#include "ADIOS2xmlVTK.h"

#include <map>
#include <string>
#include <vector>

#include "vtkImageData.h"
#include "vtkNew.h"

namespace adios2vtk
{
namespace xml
{

class ADIOS2xmlVTI : public ADIOS2xmlVTK
{
public:
  ADIOS2xmlVTI(const std::string& schema, adios2::IO* io, adios2::Engine* engine);
  ~ADIOS2xmlVTI();

private:
  vtkNew<vtkImageData> m_ImageData;

  void DoFill(vtkMultiBlockDataSet* multiBlock, const size_t step) final;
  void ReadPiece(const size_t step, const size_t pieceID) final;

  void Init();
};

} // end namespace vtkxml
} // end namespace adios2vtk

#endif
