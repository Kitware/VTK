/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridReader.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

vtkStandardNewMacro(vtkUnstructuredGridReader);

#ifdef read
#undef read
#endif

//----------------------------------------------------------------------------
vtkUnstructuredGridReader::vtkUnstructuredGridReader() = default;

//----------------------------------------------------------------------------
vtkUnstructuredGridReader::~vtkUnstructuredGridReader() = default;

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkUnstructuredGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkUnstructuredGridReader::GetOutput(int idx)
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridReader::SetOutput(vtkUnstructuredGrid* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridReader::ReadMeshSimple(const std::string& fname, vtkDataObject* doOutput)
{
  vtkIdType i, numPts = 0, numCells = 0;
  char line[256];
  vtkIdType npts, size = 0, ncells = 0;
  int piece, numPieces, skip1, read2, skip3, tmp;
  vtkCellArray* cells = nullptr;
  int* types = nullptr;
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(doOutput);

  vtkDebugMacro(<< "Reading vtk unstructured grid...");

  if (!this->OpenVTKFile(fname.c_str()) || !this->ReadHeader(fname.c_str()))
  {
    return 1;
  }

  // Read unstructured grid specific stuff
  //
  if (!this->ReadString(line))
  {
    vtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseVTKFile();
    return 1;
  }

  if (!strncmp(this->LowerCase(line), "dataset", (unsigned long)7))
  {
    // Make sure we're reading right type of geometry
    //
    if (!this->ReadString(line))
    {
      vtkErrorMacro(<< "Data file ends prematurely!");
      this->CloseVTKFile();
      return 1;
    }

    if (strncmp(this->LowerCase(line), "unstructured_grid", 17))
    {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseVTKFile();
      return 1;
    }

    // Might find points, cells, and cell types
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

        if (!this->ReadPointCoordinates(output, numPts))
        {
          this->CloseVTKFile();
          return 1;
        }
      }

      else if (!strncmp(line, "cells", 5))
      {
        if (this->FileMajorVersion >= 5)
        {
          // Just read all of the cells. The legacy path goes through the
          // streaming API, but hardcodes piece / numpieces to 0/1...
          vtkSmartPointer<vtkCellArray> tmpCells;
          if (!this->ReadCells(tmpCells))
          {
            this->CloseVTKFile();
            return 1;
          }

          cells = tmpCells;
          cells->Register(nullptr);
        }
        else // maj vers >= 5
        {    // we still want to support the pre-5.x cell format:
          piece = 0;
          numPieces = 1;
          if (!(this->Read(&ncells) && this->Read(&size)))
          {
            vtkErrorMacro(<< "Cannot read cells!");
            this->CloseVTKFile();
            return 1;
          }

          // the number of ints to read before we get to the piece.
          skip1 = piece * ncells / numPieces;
          // the number of ints to read as part of piece.
          read2 = ((piece + 1) * ncells / numPieces) - skip1;
          // the number of ints after the piece
          skip3 = ncells - skip1 - read2;

          const std::size_t connSize = static_cast<std::size_t>(size);
          std::vector<int> tempArray(connSize);
          std::vector<vtkIdType> idArray(static_cast<std::size_t>(size));

          if (!this->ReadCellsLegacy(size, tempArray.data(), skip1, read2, skip3))
          {
            this->CloseVTKFile();
            return 1;
          }

          for (std::size_t connIdx = 0; connIdx < connSize; connIdx++)
          {
            idArray[connIdx] = static_cast<vtkIdType>(tempArray[connIdx]);
          }

          cells = vtkCellArray::New();
          cells->ImportLegacyFormat(idArray.data(), size);
        }

        // Update the dataset
        if (cells && types)
        {
          output->SetCells(types, cells);
        }
      }

      else if (!strncmp(line, "cell_types", 10))
      {
        piece = 0;
        numPieces = 1;
        if (!this->Read(&ncells))
        {
          vtkErrorMacro(<< "Cannot read cell types!");
          this->CloseVTKFile();
          return 1;
        }
        // the number of ints to read before we get to the piece.
        skip1 = piece * ncells / numPieces;
        // the number of ints to read as part of piece.
        read2 = ((piece + 1) * ncells / numPieces) - skip1;
        // the number of ints after the piece
        skip3 = ncells - skip1 - read2;

        // cerr << skip1 << " --- " << read2 << " --- " << skip3 << endl;
        // allocate array for piece cell types
        types = new int[read2];
        if (this->GetFileType() == VTK_BINARY)
        {
          // suck up newline
          this->GetIStream()->getline(line, 256);
          // skip
          if (skip1 != 0)
          {
            this->GetIStream()->seekg((long)sizeof(int) * skip1, ios::cur);
          }
          this->GetIStream()->read((char*)types, sizeof(int) * read2);
          // skip
          if (skip3 != 0)
          {
            this->GetIStream()->seekg((long)sizeof(int) * skip3, ios::cur);
          }

          if (this->GetIStream()->eof())
          {
            vtkErrorMacro(<< "Error reading binary cell types!");
            this->CloseVTKFile();
            return 1;
          }
          vtkByteSwap::Swap4BERange(types, read2);
        }
        else // ascii
        {
          // skip types before piece
          for (i = 0; i < skip1; i++)
          {
            if (!this->Read(&tmp))
            {
              vtkErrorMacro(<< "Error reading cell types!");
              this->CloseVTKFile();
              return 1;
            }
          }
          // read types for piece
          for (i = 0; i < read2; i++)
          {
            if (!this->Read(types + i))
            {
              vtkErrorMacro(<< "Error reading cell types!");
              this->CloseVTKFile();
              return 1;
            }
          }
          // skip types after piece
          for (i = 0; i < skip3; i++)
          {
            if (!this->Read(&tmp))
            {
              vtkErrorMacro(<< "Error reading cell types!");
              this->CloseVTKFile();
              return 1;
            }
          }
        }
        if (cells && types)
        {
          output->SetCells(types, cells);
        }
      }

      else if (!strncmp(line, "cell_data", 9))
      {
        if (!this->Read(&numCells))
        {
          vtkErrorMacro(<< "Cannot read cell data!");
          this->CloseVTKFile();
          return 1;
        }

        if (ncells != numCells)
        {
          vtkErrorMacro(<< "Number of cells don't match!");
          this->CloseVTKFile();
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
          vtkErrorMacro(<< "Number of points don't match!");
          this->CloseVTKFile();
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
    // if ( ! (cells && types) )  vtkWarningMacro(<<"No topology read!");
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

  // Clean-up and get out
  //
  delete[] types;
  if (cells)
  {
    cells->Delete();
  }

  vtkDebugMacro(<< "Read " << output->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.\n");

  this->CloseVTKFile();
  return 1;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
