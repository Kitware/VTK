/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFacetReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFacetReader.h"

#include "vtkPolyData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkErrorCode.h"
#include "vtkCellType.h"
#include "vtkCellArray.h"
#include "vtkGarbageCollector.h"
#include "vtkAppendPolyData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include "vtkUnsignedIntArray.h"
#include "vtkDoubleArray.h"
#include "vtkSmartPointer.h"

#include <vtksys/ios/sstream>
#include <sys/stat.h>
#include <string>
#include <vector>

vtkStandardNewMacro(vtkFacetReader);

//------------------------------------------------------------------------------
// Due to a buggy stream library on the HP and another on Mac OS X, we
// need this very carefully written version of getline.  Returns true
// if any data were read before the end-of-file was reached.
// 
static bool GetLineFromStream(istream& is,
  std::string& line, bool *has_newline = 0)
{
  const int bufferSize = 1024;
  char buffer[bufferSize];
  line = "";
  bool haveData = false;
  if ( has_newline )
    {
    *has_newline = false;
    }

  // If no characters are read from the stream, the end of file has
  // been reached.
  while((is.getline(buffer, bufferSize), is.gcount() > 0))
    {
    haveData = true;
    line.append(buffer);

    // If newline character was read, the gcount includes the
    // character, but the buffer does not.  The end of line has been
    // reached.
    if(strlen(buffer) < static_cast<size_t>(is.gcount()))
      {
      if ( has_newline )
        {
        *has_newline = true;
        }
      break;
      }

    // The fail bit may be set.  Clear it.
    is.clear(is.rdstate() & ~ios::failbit);
    }
  return haveData;
}

//----------------------------------------------------------------------------
vtkFacetReader::vtkFacetReader()
{
  this->FileName  = NULL;
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkFacetReader::~vtkFacetReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
}

//-----------------------------------------------------------------------------
int vtkFacetReader::CanReadFile(const char *filename)
{
  struct stat fs;
  if (stat(filename, &fs))
    {
    // Specified filename not found
    return 0;
    }

  ifstream ifs(filename, ios::in);
  if (!ifs)
    {
    // Specified filename not found
    return 0;
    }

  std::string line;
  // Read first row
  if (!GetLineFromStream(ifs, line))
    {
    // Cannot read file comment
    return 0;
    }

  // File starts with FACET FILE
  return (line.find("FACET FILE") == 0);
}

//----------------------------------------------------------------------------
int vtkFacetReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if ( !this->FileName )
    {
    vtkErrorMacro("No filename specified");
    return 1;
    }

  struct stat fs;
  if ( stat(this->FileName, &fs) )
    {
    this->SetErrorCode(vtkErrorCode::FileNotFoundError);
    vtkErrorMacro("Specified filename not found");
    return 1;
    }

  ifstream ifs(this->FileName, ios::in);
  if (!ifs)
    {
    this->SetErrorCode(vtkErrorCode::FileNotFoundError);
    vtkErrorMacro("Specified filename not found");
    return 1;
    }

  vtkDebugMacro( << "Reading Facet file");
  std::string line;

  // Read first row
  if ( !GetLineFromStream(ifs, line) )
    {
    vtkErrorMacro("Cannot read file comment");
    return 1;
    }

  // Read number of parts
  int num_parts = 0;
  if ( !GetLineFromStream(ifs, line) ||
    sscanf(line.c_str(), "%d", &num_parts) != 1 ||
    num_parts < 0 )
    {
    vtkErrorMacro("Bad number of parts line");
    return 1;
    }

  vtkDebugMacro("Number of parts is: " << num_parts);

  // Buffers for various information from file
  std::vector<vtkIdType> pointList;
  std::vector<char> stringBuffer;

  // We will need append individual parts together. Once multiblock is
  // supported, this should go out.
  vtkSmartPointer<vtkAppendPolyData> appendPtr = vtkSmartPointer<vtkAppendPolyData>::New();
  
  // Block garbage collection so that appends will not take too long.
  vtkGarbageCollector::DeferredCollectionPush();

  int part;
  int error = 0;

  // Loop thrugh individual parts
  for ( part = 0; part < num_parts || error; part ++ )
    {
    std::string partName;
    vtkDebugMacro("Reading part: " << part);

    // Read part name
    if ( !GetLineFromStream(ifs, partName) )
      {
      vtkErrorMacro("Cannot read part name");
      error = 1;
      break;
      }
    vtkDebugMacro("Part name: " << partName.c_str());

    // Read cell/point index and geometry information including the number of
    // points. cell/point index for points is always 0
    int cell_point_index = -1;
    int numpts = -1, tmp;
    if ( !GetLineFromStream(ifs, line) ||
      sscanf(line.c_str(), "%d", &cell_point_index) != 1 || 
      cell_point_index != 0 ||
      !GetLineFromStream(ifs, line) ||
      sscanf(line.c_str(), "%d %d %d", &numpts, &tmp, &tmp) != 3 || 
      numpts < 0 )
      {
      vtkErrorMacro("Problem reading number of points");
      error = 1;
      break;
      }

    vtkIdType num_points = numpts;
    vtkIdType point;
    vtkSmartPointer<vtkPoints> myPointsPtr = vtkSmartPointer<vtkPoints>::New();

    // Read individual points
    for ( point = 0; point < num_points; point ++ )
      {
      // Read point
      double x = 0, y = 0, z = 0;
      if ( !GetLineFromStream(ifs, line) ||
        sscanf(line.c_str(), "%lf %lf %lf", &x, &y, &z) != 3 )
        {
        vtkErrorMacro("Problem reading point: " << point);
        error = 1;
        break;
        }
      myPointsPtr->InsertNextPoint(x, y, z);
      }
    if ( error )
      {
      break;
      }

    // Read cell point index
    if ( !GetLineFromStream(ifs, line) ||
      sscanf(line.c_str(), "%d", &cell_point_index) != 1 || 
      cell_point_index != 1 )
      {
      vtkErrorMacro("Cannot read cell/point index or it is not 1");
        error = 1;
      break;
      }

    // Read part name
    if ( !GetLineFromStream(ifs, line) ||
      partName != line )
      {
      vtkErrorMacro("Cannot read part name or the part name does not match");
        error = 1;
      break;
      }

    // Read topology information
    int numcells = -1, numpointpercell = -1;
    if ( !GetLineFromStream(ifs, line) ||
      sscanf(line.c_str(), "%d %d", &numcells, &numpointpercell) != 2 || 
      numcells < 0 || numpointpercell < 0 )
      {
      vtkErrorMacro("Problem reading number of cells and points per cell");
        error = 1;
      break;
      }

    vtkIdType num_cells = numcells;
    vtkIdType num_points_per_cell = numpointpercell;
    if ( pointList.size() < static_cast<size_t>(num_points_per_cell) )
      {
      pointList.resize(num_points_per_cell);
      }

    // We need arrays to store material and part number
    vtkSmartPointer<vtkUnsignedIntArray> materialArrayPtr = vtkSmartPointer<vtkUnsignedIntArray>::New();
    materialArrayPtr->SetName("Material");
    materialArrayPtr->SetNumberOfComponents(1);
    materialArrayPtr->SetNumberOfTuples(num_cells);

    vtkSmartPointer<vtkUnsignedIntArray> relativePartArrayPtr = vtkSmartPointer<vtkUnsignedIntArray>::New();
    relativePartArrayPtr->SetName("RelativePartNumber");
    relativePartArrayPtr->SetNumberOfComponents(1);
    relativePartArrayPtr->SetNumberOfTuples(num_cells);

    vtkSmartPointer<vtkCellArray> myCellsPtr = vtkSmartPointer<vtkCellArray>::New();

    // Read cells
    vtkIdType cell;
    for ( cell = 0; cell < num_cells; cell ++ )
      {
      // Read cell
      if ( !GetLineFromStream(ifs, line) )
        {
        vtkErrorMacro("Cannot read cell: " << cell);
        error = 1;
        break;
        }

      // Read specified number of points from cell information
      if ( stringBuffer.size() < line.size()+1 )
        {
        stringBuffer.resize(line.size()+1);
        }
      char* strPtr = &(*stringBuffer.begin());
      strcpy(strPtr, line.c_str());
      std::string str(strPtr, stringBuffer.size());
      vtksys_ios::istringstream lineStream(str);
      vtkIdType kk;
      int material = -1, relativePartNumber = -1;
      for ( kk = 0; kk < num_points_per_cell; kk ++ )
        {
        int val;
        if ( !(lineStream >> val) )
          {
          vtkErrorMacro("Cannot extract cell points for cell: " << cell);
          error = 1;
          break;
          }
        // point indices start with 0, while cell descriptions have point
        // indices starting with 1
        pointList[kk] = val -1;
        }

      // Extract material and part number
      if ( !(lineStream >> material >> relativePartNumber) )
        {
        vtkErrorMacro("Cannot extract cell material and part for cell: "
          << cell);
        error = 1;
        break;
        }
      materialArrayPtr->SetTuple1(cell, material);
      relativePartArrayPtr->SetTuple1(cell, relativePartNumber);

      myCellsPtr->InsertNextCell(num_points_per_cell, &(*pointList.begin()));
      }
    if ( error )
      {
      break;
      }

    vtkIdType cc;

    // Create another array with absolute part number
    vtkUnsignedIntArray* partNumberArray = vtkUnsignedIntArray::New();
    partNumberArray->SetName("PartNumber");
    partNumberArray->SetNumberOfComponents(1);
    partNumberArray->SetNumberOfTuples(num_cells);
    for ( cc = 0; cc < partNumberArray->GetNumberOfTuples(); cc ++ )
      {
      partNumberArray->SetTuple1(cc, part);
      }
    
    // Create part and store it
    vtkPolyData* partGrid = vtkPolyData::New();
    switch ( num_points_per_cell )
      {
    case 1:
      partGrid->SetVerts(myCellsPtr);
      break;
    case 2:
      partGrid->SetLines(myCellsPtr);
      break;
    case 3:
      partGrid->SetPolys(myCellsPtr);
      break;
    default:
      partGrid->SetPolys(myCellsPtr);
      break;
      }
    partGrid->SetPoints(myPointsPtr);
    partGrid->GetCellData()->AddArray(partNumberArray);
    partGrid->GetCellData()->AddArray(materialArrayPtr);
    partGrid->GetCellData()->AddArray(relativePartArrayPtr);
    partGrid->GetCellData()->SetScalars(materialArrayPtr);
    appendPtr->AddInput(partGrid);

    partNumberArray->Delete();
    partGrid->Delete();
    }

  if ( !error )
    {
    // If everything ok, use append.
    appendPtr->Update();
    output->ShallowCopy(appendPtr->GetOutput());
    }

  // Release garbage collection
  vtkGarbageCollector::DeferredCollectionPop();  
  vtkDebugMacro("Done reading file: " << this->FileName);

  return 1;
}

//----------------------------------------------------------------------------
void vtkFacetReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: " 
    << (this->FileName ? this->FileName : "(none)") << "\n";
}


