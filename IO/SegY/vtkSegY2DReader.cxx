/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegY2DReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSegY2DReader.h"

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>

#include <chrono>

vtkStandardNewMacro(vtkSegY2DReader);

//-----------------------------------------------------------------------------
vtkSegY2DReader::vtkSegY2DReader()
{
  this->FileName = nullptr;
  this->SetNumberOfInputPorts(0);
}

//-----------------------------------------------------------------------------
vtkSegY2DReader::~vtkSegY2DReader()
{
  this->SetFileName(nullptr);
}

//-----------------------------------------------------------------------------
int vtkSegY2DReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkStructuredGrid* output =
    vtkStructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!this->FileName)
  {
    vtkErrorMacro(<< "A File Name must be specified.");
    return 0;
  }

  reader.LoadFromFile(FileName);
  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();
  vtkDebugMacro(<< "Exporting to poly data ...");
  reader.ExportData2D(output);
  end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;
  vtkDebugMacro(<< "Elapsed time: " << elapsed_seconds.count());
  output->Squeeze();
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSegY2DReader::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)")
     << "\n";
  Superclass::PrintSelf(os, indent);
}
