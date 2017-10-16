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

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSegYReader.h"

#include <chrono>

vtkStandardNewMacro(vtkSegY2DReader);

//-----------------------------------------------------------------------------
vtkSegY2DReader::vtkSegY2DReader()
{
  this->FileName = nullptr;
  this->SetNumberOfInputPorts(0);
  this->Reader = new vtkSegYReader();

  this->XYCoordMode = VTK_SEGY_SOURCE;
  this->XCoordByte = 73;
  this->YCoordByte = 77;

  this->VerticalCRS = VTK_SEGY_VERTICAL_HEIGHTS;
}

//-----------------------------------------------------------------------------
vtkSegY2DReader::~vtkSegY2DReader()
{
  this->SetFileName(nullptr);
  delete this->Reader;
  this->Reader = nullptr;
}

//-----------------------------------------------------------------------------
void vtkSegY2DReader::SetXYCoordModeToSource()
{
  this->SetXYCoordMode(VTK_SEGY_SOURCE);
}

//-----------------------------------------------------------------------------
void vtkSegY2DReader::SetXYCoordModeToCDP()
{
  this->SetXYCoordMode(VTK_SEGY_CDP);
}

//-----------------------------------------------------------------------------
void vtkSegY2DReader::SetXYCoordModeToCustom()
{
  this->SetXYCoordMode(VTK_SEGY_CUSTOM);
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

  switch (this->XYCoordMode)
  {
    case VTK_SEGY_SOURCE:
    {
      this->Reader->SetXYCoordBytePositions(72, 76);
      break;
    }
    case VTK_SEGY_CDP:
    {
      this->Reader->SetXYCoordBytePositions(180, 184);
      break;
    }
    case VTK_SEGY_CUSTOM:
    {
      this->Reader->SetXYCoordBytePositions(this->XCoordByte - 1,
                                            this->YCoordByte - 1);
      break;
    }
    default:
    {
      vtkErrorMacro(<< "Unknown value for XYCoordMode " << this->XYCoordMode);
      return 1;
    }
  }

  this->Reader->SetVerticalCRS(this->VerticalCRS);

  this->Reader->LoadFromFile(FileName);
  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();
  vtkDebugMacro(<< "Exporting to poly data ...");
  this->Reader->ExportData2D(output);
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
     << endl;
  switch (this->XYCoordMode)
  {
    case VTK_SEGY_SOURCE:
      os << indent << "XYCoordMode: VTK_SEGY_SOURCE" << endl;
      break;
    case VTK_SEGY_CDP:
      os << indent << "XYCoordMode: VTK_SEGY_CDP" << endl;
      break;
    case VTK_SEGY_CUSTOM:
      os << indent << "XYCoordMode: VTK_SEGY_CUSTOM" << endl;
      break;
    default:
      os << indent << "XYCoordMode: (unidentified)" << endl;
  }
  os << indent << "XCoordByte " << this->XCoordByte << endl;
  os << indent << "YCoordByte " << this->YCoordByte << endl;
  os << indent
     << "VerticalCRS: " << (this->VerticalCRS ? "VTK_SEGY_VERTICAL_DEPTHS"
                                              : "VTK_SEGY_VERTICAL_HEIGHTS")
     << endl;
  Superclass::PrintSelf(os, indent);
}
