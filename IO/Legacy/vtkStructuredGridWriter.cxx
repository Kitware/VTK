/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredGridWriter.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

vtkStandardNewMacro(vtkStructuredGridWriter);

void vtkStructuredGridWriter::WriteData()
{
  ostream *fp;
  vtkStructuredGrid *input= vtkStructuredGrid::SafeDownCast(this->GetInput());
  int dim[3];

  vtkDebugMacro(<<"Writing vtk structured grid...");

  if ( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
  {
    if (fp)
    {
      vtkErrorMacro("Ran out of disk space; deleting file: "
                    << this->FileName);
      this->CloseVTKFile(fp);
      unlink(this->FileName);
    }
    return;
  }

  // Write structured grid specific stuff
  //
  *fp << "DATASET STRUCTURED_GRID\n";

  // Write data owned by the dataset
  if (!this->WriteDataSetData(fp, input))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  if (this->WriteExtent)
  {
    int extent[6];
    input->GetExtent(extent);
    *fp << "EXTENT "
        << extent[0] << " " << extent[1] << " " << extent[2] << " "
        << extent[3] << " " << extent[4] << " " << extent[5] << "\n";
  }
  else
  {
    input->GetDimensions(dim);
    *fp << "DIMENSIONS " << dim[0] << " " << dim[1] << " " << dim[2] << "\n";
  }

  if (!this->WritePoints(fp, input->GetPoints()))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  if (!this->WriteCellData(fp, input))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }
  if (!this->WritePointData(fp, input))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  this->CloseVTKFile(fp);
}

int vtkStructuredGridWriter::FillInputPortInformation(int,
                                                      vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
  return 1;
}

vtkStructuredGrid* vtkStructuredGridWriter::GetInput()
{
  return vtkStructuredGrid::SafeDownCast(this->Superclass::GetInput());
}

vtkStructuredGrid* vtkStructuredGridWriter::GetInput(int port)
{
  return vtkStructuredGrid::SafeDownCast(this->Superclass::GetInput(port));
}

void vtkStructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
