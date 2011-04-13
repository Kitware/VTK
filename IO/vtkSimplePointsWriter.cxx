/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSimplePointsWriter.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSimplePointsWriter.h"

#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

#include <fstream>
#include <iomanip>

vtkStandardNewMacro(vtkSimplePointsWriter);

vtkSimplePointsWriter::vtkSimplePointsWriter()
{
  std::ofstream fout; // only used to extract the default precision
  this->DecimalPrecision = fout.precision();
}

void vtkSimplePointsWriter::WriteData()
{
  vtkPolyData *input = this->GetInput();

  std::ofstream fout(this->FileName);

  for(vtkIdType i = 0; i < input->GetNumberOfPoints(); i++)
    {
    double p[3];
    input->GetPoint(i,p);
    fout << std::setprecision(this->DecimalPrecision) << p[0] << " " << p[1] << " " << p[2] << std::endl;
    }

  fout.close();
}

void vtkSimplePointsWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DecimalPrecision: " << this->DecimalPrecision << "\n";
}
