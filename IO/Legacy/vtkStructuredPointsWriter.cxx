/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredPointsWriter.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredPoints.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif


vtkStandardNewMacro(vtkStructuredPointsWriter);

void vtkStructuredPointsWriter::WriteData()
{
  ostream *fp;
  vtkImageData *input= vtkImageData::SafeDownCast(this->GetInput());
  int dim[3];
  int *ext;
  double spacing[3], origin[3];

  vtkDebugMacro(<<"Writing vtk structured points...");

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
  //
  // Write structured points specific stuff
  //
  *fp << "DATASET STRUCTURED_POINTS\n";

  // Write data owned by the dataset
  if (!this->WriteDataSetData(fp, input))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  input->GetDimensions(dim);
  *fp << "DIMENSIONS " << dim[0] << " " << dim[1] << " " << dim[2] << "\n";

  input->GetSpacing(spacing);
  *fp << "SPACING " << spacing[0] << " " << spacing[1] << " " << spacing[2] << "\n";

  input->GetOrigin(origin);
  // Do the electric slide. Move origin to min corner of extent.
  // The alternative is to change the format to include an extent instead of dimensions.
  ext = input->GetExtent();
  origin[0] += ext[0] * spacing[0];
  origin[1] += ext[2] * spacing[1];
  origin[2] += ext[4] * spacing[2];
  *fp << "ORIGIN " << origin[0] << " " << origin[1] << " " << origin[2] << "\n";

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

int vtkStructuredPointsWriter::FillInputPortInformation(int,
                                                        vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

vtkImageData* vtkStructuredPointsWriter::GetInput()
{
  return vtkImageData::SafeDownCast(this->Superclass::GetInput());
}

vtkImageData* vtkStructuredPointsWriter::GetInput(int port)
{
  return vtkImageData::SafeDownCast(this->Superclass::GetInput(port));
}

void vtkStructuredPointsWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
