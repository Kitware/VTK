/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataReader.h"

#include "vtkCellArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vector>

vtkStandardNewMacro(vtkPolyDataReader);

//----------------------------------------------------------------------------
vtkPolyDataReader::vtkPolyDataReader() = default;

//----------------------------------------------------------------------------
vtkPolyDataReader::~vtkPolyDataReader() = default;

//----------------------------------------------------------------------------
vtkPolyData* vtkPolyDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkPolyDataReader::GetOutput(int idx)
{
  return vtkPolyData::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void vtkPolyDataReader::SetOutput(vtkPolyData* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
int vtkPolyDataReader::ReadMeshSimple(const std::string& fname, vtkDataObject* doOutput)
{
  vtkIdType numPts = 0;
  char line[256];
  vtkIdType npts, size = 0, ncells;
  vtkPolyData* output = vtkPolyData::SafeDownCast(doOutput);

  // Helper function to handle legacy cell data fallback:
  auto readCellArray = [&](vtkSmartPointer<vtkCellArray>& cellArray) -> bool {
    if (this->FileMajorVersion >= 5)
    { // Cells are written as offsets + connectivity arrays:
      return this->ReadCells(cellArray) != 0;
    }
    else
    { // Import cells from legacy format:
      if (!(this->Read(&ncells) && this->Read(&size)))
      {
        return false;
      }

      std::size_t connSize = static_cast<std::size_t>(size);
      std::vector<int> tempArray(connSize);
      std::vector<vtkIdType> idArray(connSize);

      if (!this->ReadCellsLegacy(size, tempArray.data()))
      {
        this->CloseVTKFile();
        return 1;
      }

      // Convert to id type
      for (std::size_t connIdx = 0; connIdx < connSize; connIdx++)
      {
        idArray[connIdx] = static_cast<vtkIdType>(tempArray[connIdx]);
      }

      cellArray = vtkSmartPointer<vtkCellArray>::New();
      cellArray->ImportLegacyFormat(idArray.data(), size);
      return true;
    } // end legacy cell read
  };

  vtkDebugMacro(<< "Reading vtk polygonal data...");

  if (!(this->OpenVTKFile(fname.c_str())) || !this->ReadHeader(fname.c_str()))
  {
    return 1;
  }
  //
  // Read polygonal data specific stuff
  //
  if (!this->ReadString(line))
  {
    vtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseVTKFile();
    return 1;
  }

  if (!strncmp(this->LowerCase(line), "dataset", (unsigned long)7))
  {
    //
    // Make sure we're reading right type of geometry
    //
    if (!this->ReadString(line))
    {
      vtkErrorMacro(<< "Data file ends prematurely!");
      this->CloseVTKFile();
      return 1;
    }

    if (strncmp(this->LowerCase(line), "polydata", 8))
    {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseVTKFile();
      return 1;
    }
    //
    // Might find points, vertices, lines, polygons, or triangle strips
    //
    while (true)
    {
      if (!this->ReadString(line))
      {
        break;
      }

      if (!strncmp(this->LowerCase(line), "field", 5))
      {
        vtkFieldData* fd = this->ReadFieldData();
        output->SetFieldData(fd);
        fd->Delete(); // ?
      }
      else if (!strncmp(line, "points", 6))
      {
        if (!this->Read(&numPts))
        {
          vtkErrorMacro(<< "Cannot read number of points!");
          this->CloseVTKFile();
          return 1;
        }

        this->ReadPointCoordinates(output, numPts);
      }
      else if (!strncmp(line, "vertices", 8))
      {
        vtkSmartPointer<vtkCellArray> cells;
        if (!readCellArray(cells))
        {
          vtkErrorMacro("Error reading vertices.");
          this->CloseVTKFile();
          return 1;
        }
        output->SetVerts(cells);
        vtkDebugMacro("Read " << cells->GetNumberOfCells() << " vertices");
      }

      else if (!strncmp(line, "lines", 5))
      {
        vtkSmartPointer<vtkCellArray> cells;
        if (!readCellArray(cells))
        {
          vtkErrorMacro("Error reading lines.");
          this->CloseVTKFile();
          return 1;
        }
        output->SetLines(cells);
        vtkDebugMacro("Read " << cells->GetNumberOfCells() << " lines");
      }

      else if (!strncmp(line, "polygons", 8))
      {
        vtkSmartPointer<vtkCellArray> cells;
        if (!readCellArray(cells))
        {
          vtkErrorMacro("Error reading polygons.");
          this->CloseVTKFile();
          return 1;
        }
        output->SetPolys(cells);
        vtkDebugMacro("Read " << cells->GetNumberOfCells() << " polygons");
      }

      else if (!strncmp(line, "triangle_strips", 15))
      {
        vtkSmartPointer<vtkCellArray> cells;
        if (!readCellArray(cells))
        {
          vtkErrorMacro("Error reading triangle_strips.");
          this->CloseVTKFile();
          return 1;
        }
        output->SetStrips(cells);
        vtkDebugMacro("Read " << cells->GetNumberOfCells() << " triangle strips");
      }

      else if (!strncmp(line, "cell_data", 9))
      {
        if (!this->Read(&ncells))
        {
          vtkErrorMacro(<< "Cannot read cell data!");
          this->CloseVTKFile();
          return 1;
        }

        if (ncells != output->GetNumberOfCells())
        {
          vtkErrorMacro(<< "Number of cells don't match number data values!");
          return 1;
        }

        this->ReadCellData(output, ncells);
        break; // out of this loop
      }

      else if (!strncmp(line, "point_data", 10))
      {
        if (!this->Read(&npts))
        {
          vtkErrorMacro(<< "Cannot read point data!");
          this->CloseVTKFile();
          return 1;
        }

        if (npts != numPts)
        {
          vtkErrorMacro(<< "Number of points don't match number data values!");
          return 1;
        }

        this->ReadPointData(output, npts);
        break; // out of this loop
      }

      else
      {
        vtkErrorMacro(<< "Unrecognized keyword: " << line);
        this->CloseVTKFile();
        return 1;
      }
    }

    if (!output->GetPoints())
      vtkWarningMacro(<< "No points read!");
    if (!(output->GetVerts() || output->GetLines() || output->GetPolys() || output->GetStrips()))
      vtkWarningMacro(<< "No topology read!");
  }

  else if (!strncmp(line, "cell_data", 9))
  {
    vtkWarningMacro(<< "No geometry defined in data file!");
    if (!this->Read(&ncells))
    {
      vtkErrorMacro(<< "Cannot read cell data!");
      this->CloseVTKFile();
      return 1;
    }

    this->ReadCellData(output, ncells);
  }

  else if (!strncmp(line, "point_data", 10))
  {
    vtkWarningMacro(<< "No geometry defined in data file!");
    if (!this->Read(&numPts))
    {
      vtkErrorMacro(<< "Cannot read point data!");
      this->CloseVTKFile();
      return 1;
    }

    this->ReadPointData(output, numPts);
  }

  else
  {
    vtkErrorMacro(<< "Unrecognized keyword: " << line);
  }
  this->CloseVTKFile();

  return 1;
}

//----------------------------------------------------------------------------
int vtkPolyDataReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
