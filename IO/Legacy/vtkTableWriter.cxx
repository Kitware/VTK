/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTableWriter.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

vtkStandardNewMacro(vtkTableWriter);

void vtkTableWriter::WriteData()
{
  ostream* fp = 0;
  vtkDebugMacro(<<"Writing vtk table data...");

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
  // Write table specific stuff
  //
  *fp << "DATASET TABLE\n";

  this->WriteFieldData(fp, this->GetInput()->GetFieldData());
  this->WriteRowData(fp, this->GetInput());

  this->CloseVTKFile(fp);
}

int vtkTableWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

vtkTable* vtkTableWriter::GetInput()
{
  return vtkTable::SafeDownCast(this->Superclass::GetInput());
}

vtkTable* vtkTableWriter::GetInput(int port)
{
  return vtkTable::SafeDownCast(this->Superclass::GetInput(port));
}

void vtkTableWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
