// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSimplePointsWriter.h"

#include "vtkErrorCode.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtksys/FStream.hxx"

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

#include <iomanip>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSimplePointsWriter);

vtkSimplePointsWriter::vtkSimplePointsWriter()
{
  vtksys::ofstream fout; // only used to extract the default precision
  this->DecimalPrecision = fout.precision();
}

void vtkSimplePointsWriter::WriteData()
{
  vtkPointSet* input = vtkPointSet::SafeDownCast(this->GetInput());
  vtkIdType numberOfPoints = 0;

  if (input)
  {
    numberOfPoints = input->GetNumberOfPoints();
  }

  // OpenVTKFile() will report any errors that happen
  ostream* outfilep = this->OpenVTKFile();
  if (!outfilep)
  {
    return;
  }

  ostream& outfile = *outfilep;

  for (vtkIdType i = 0; i < numberOfPoints; i++)
  {
    double p[3];
    input->GetPoint(i, p);
    outfile << std::setprecision(this->DecimalPrecision) << p[0] << " " << p[1] << " " << p[2]
            << std::endl;
  }

  // Close the file
  this->CloseVTKFile(outfilep);

  // Delete the file if an error occurred
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    unlink(this->FileName);
  }
}

void vtkSimplePointsWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DecimalPrecision: " << this->DecimalPrecision << "\n";
}
VTK_ABI_NAMESPACE_END
