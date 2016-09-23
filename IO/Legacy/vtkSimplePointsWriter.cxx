/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimplePointsWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSimplePointsWriter.h"

#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkErrorCode.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

#include <iomanip>

vtkStandardNewMacro(vtkSimplePointsWriter);

vtkSimplePointsWriter::vtkSimplePointsWriter()
{
  std::ofstream fout; // only used to extract the default precision
  this->DecimalPrecision = fout.precision();
}

void vtkSimplePointsWriter::WriteData()
{
  vtkPointSet *input = vtkPointSet::SafeDownCast(this->GetInput());
  vtkIdType numberOfPoints = 0;

  if (input)
  {
    numberOfPoints = input->GetNumberOfPoints();
  }

  // OpenVTKFile() will report any errors that happen
  ostream *outfilep = this->OpenVTKFile();
  if (!outfilep)
  {
    return;
  }

  ostream &outfile = *outfilep;

  for(vtkIdType i = 0; i < numberOfPoints; i++)
  {
    double p[3];
    input->GetPoint(i,p);
    outfile << std::setprecision(this->DecimalPrecision)
            << p[0] << " " << p[1] << " " << p[2] << std::endl;
  }

  // Close the file
  this->CloseVTKFile(outfilep);

  // Delete the file if an error occurred
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    vtkErrorMacro("Ran out of disk space; deleting file: "
                  << this->FileName);
    unlink(this->FileName);
  }
}

void vtkSimplePointsWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DecimalPrecision: " << this->DecimalPrecision << "\n";
}
