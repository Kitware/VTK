/*=========================================================================

  Program: Visualization Toolkit
  Module: vtkProStarReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notice for more information.

=========================================================================*/

#include "vtkProStarReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"

#include <cctype>
#include <cstring>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <utility>

vtkStandardNewMacro(vtkProStarReader);

// Internal Classes/Structures
struct vtkProStarReader::idMapping : public std::map<vtkIdType, vtkIdType>
{};

//----------------------------------------------------------------------------
vtkProStarReader::vtkProStarReader()
{
  this->FileName = NULL;
  this->ScaleFactor = 1;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkProStarReader::~vtkProStarReader()
{
  delete [] this->FileName;
}

//----------------------------------------------------------------------------
int vtkProStarReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if (!this->FileName)
  {
    vtkErrorMacro("FileName has to be specified!");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
  }

  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->FileName)
  {
    idMapping mapPointId; // inverse mapping (STAR-CD pointId -> index)

    if (this->ReadVrtFile(output, mapPointId))
    {
      this->ReadCelFile(output, mapPointId);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkProStarReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "ScaleFactor: " << this->ScaleFactor << endl;
}

//----------------------------------------------------------------------------
int vtkProStarReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{
  if (!this->FileName)
  {
    vtkErrorMacro("FileName has to be specified!");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
FILE* vtkProStarReader::OpenFile(const char *ext)
{
  std::string fullName = this->FileName;
  const char *dot = strrchr(this->FileName, '.');

  if
    (
        dot != NULL
     && (
            strcmp(dot, ".cel") == 0
         || strcmp(dot, ".vrt") == 0
         || strcmp(dot, ".inp") == 0
        )
    )
  {
    fullName.resize(dot - this->FileName);
  }

  fullName += ext;
  FILE *in = fopen(fullName.c_str(), "r");
  if (in == NULL)
  {
    vtkErrorMacro(<<"Error opening file: " << fullName);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
  }

  return in;
}

//
// read in the points from the .vrt file
/*---------------------------------------------------------------------------*\
Line 1:
PROSTAR_VERTEX [newline]

Line 2:
<version> 0 0 0 0 0 0 0 [newline]

Body:
<vertexId> <x> <y> <z> [newline]

\*---------------------------------------------------------------------------*/
bool vtkProStarReader::ReadVrtFile(vtkUnstructuredGrid *output,
                                   idMapping& mapPointId)
{
  mapPointId.clear();
  FILE *in = this->OpenFile(".vrt");
  if (in == NULL)
  {
    return false;
  }

  const int MAX_LINE = 1024;
  char rawLine[MAX_LINE];

  int lineLabel, errorCount = 0;
  if (
         fgets(rawLine, MAX_LINE, in) != NULL
      && strncmp(rawLine, "PROSTAR_VERTEX", 14) == 0
      && fgets(rawLine, MAX_LINE, in) != NULL
      && sscanf(rawLine, "%d", &lineLabel) == 1
      && lineLabel >= 4000
     )
  {
    vtkDebugMacro(<<"Got PROSTAR_VERTEX header");
  }
  else
  {
    vtkErrorMacro(<<"Error reading header for PROSTAR_VERTEX file");
    ++errorCount;
  }

  vtkPoints *points = vtkPoints::New();
  // don't know the number of points a priori -- just pick some number
  points->Allocate(10000,20000);

  int lineNr = 2;
  float xyz[3];
  vtkIdType nodeCount = 0;

  while (!errorCount && fgets(rawLine, MAX_LINE, in) != NULL)
  {
    ++lineNr;
    if (sscanf(rawLine, "%d %f %f %f", &lineLabel, xyz, xyz+1, xyz+2) == 4)
    {
      xyz[0] *= this->ScaleFactor;
      xyz[1] *= this->ScaleFactor;
      xyz[2] *= this->ScaleFactor;

      points->InsertNextPoint(xyz);
      vtkIdType nodeId = lineLabel;
      mapPointId.insert(std::make_pair(nodeId, nodeCount));
      ++nodeCount;
    }
    else
    {
      vtkErrorMacro(<<"Error reading point at line " << lineNr);
      ++errorCount;
    }
  }

  points->Squeeze();
  output->SetPoints(points);
  points->Delete();

  vtkDebugMacro(<<"Read points: " << lineNr << " errors: " << errorCount);

  fclose(in);
  return errorCount == 0;
}

//
// read in the cells from the .cel file
/*---------------------------------------------------------------------------*\
Line 1:
PROSTAR_CELL [newline]

Line 2:
<version> 0 0 0 0 0 0 0 [newline]

Body:
<cellId> <shapeId> <nLabels> <cellTableId> <typeId> [newline]
<cellId> <int1> .. <int8>
<cellId> <int9> .. <int16>

with shapeId:
* 1 = point
* 2 = line
* 3 = shell
* 11 = hexa
* 12 = prism
* 13 = tetra
* 14 = pyramid
* 255 = polyhedron

with typeId
* 1 = fluid
* 2 = solid
* 3 = baffle
* 4 = shell
* 5 = line
* 6 = point

For primitive cell shapes, the number of vertices will never exceed 8 (hexa)
and corresponds to <nLabels>.
For polyhedral, <nLabels> includes an index table comprising beg/end pairs
for each cell face.

\*---------------------------------------------------------------------------*/
bool vtkProStarReader::ReadCelFile(vtkUnstructuredGrid *output,
                                   const idMapping& mapPointId)
{
  FILE *in = this->OpenFile(".cel");
  if (in == NULL)
  {
    return false;
  }

  const int MAX_LINE = 1024;
  char rawLine[MAX_LINE];

  int lineLabel, errorCount = 0;
  if (
         fgets(rawLine, MAX_LINE, in) != NULL
      && strncmp(rawLine, "PROSTAR_CELL", 12) == 0
      && fgets(rawLine, MAX_LINE, in) != NULL
      && sscanf(rawLine, "%d", &lineLabel) == 1
      && lineLabel >= 4000
     )
  {
    vtkDebugMacro(<<"Got PROSTAR_CELL header");
  }
  else
  {
    vtkErrorMacro(<<"Error reading header for PROSTAR_CELL file");
    ++errorCount;
  }

  // don't know the number of cells a priori -- just pick some number
  output->Allocate(10000,20000);

  // add a cellTableId array
  vtkIntArray *cellTableId = vtkIntArray::New();
  cellTableId->Allocate(10000,20000);
  cellTableId->SetName("cellTableId");


  int shapeId, nLabels, tableId, typeId;
  std::vector<vtkIdType> starLabels;
  starLabels.reserve(256);

  // face-stream for a polyhedral cell
  // [numFace0Pts, id1, id2, id3, numFace1Pts, id1, id2, id3, ...]
  std::vector<vtkIdType> faceStream;
  faceStream.reserve(256);


  // use string buffer for easier parsing
  std::istringstream strbuf;

  int lineNr = 2;
  while (!errorCount && fgets(rawLine, MAX_LINE, in) != NULL)
  {
    ++lineNr;
    if (sscanf(rawLine, "%d %d %d %d %d", &lineLabel, &shapeId, &nLabels, &tableId, &typeId) == 5)
    {
      starLabels.clear();
      starLabels.reserve(nLabels);

      // read indices - max 8 per line
      for (int index = 0; !errorCount && index < nLabels; ++index)
      {
        int vrtId;
        if ((index % 8) == 0)
        {
          if (fgets(rawLine, MAX_LINE, in) != NULL)
          {
            ++lineNr;
            strbuf.clear();
            strbuf.str(rawLine);
            strbuf >> lineLabel;
          }
          else
          {
            vtkErrorMacro(<<"Error reading PROSTAR_CELL file at line "<<lineNr);
            ++errorCount;
          }
        }

        strbuf >> vrtId;
        starLabels.push_back(vrtId);
      }

      // special treatment for polyhedra
      if (shapeId == starcdPoly)
      {
        const vtkIdType nFaces = starLabels[0] - 1;

        // build face-stream
        // [numFace0Pts, id1, id2, id3, numFace1Pts, id1, id2, id3, ...]
        // point Ids are global
        faceStream.clear();
        faceStream.reserve(nLabels);

        for (vtkIdType faceI = 0; faceI < nFaces; ++faceI)
        {
          // traverse beg/end indices
          const int beg = starLabels[faceI];
          const int end = starLabels[faceI+1];

          // number of points for this face
          faceStream.push_back(end - beg);

          for (int idxI = beg; idxI < end; ++idxI)
          {
            // map orig vertex id -> point label
            faceStream.push_back(mapPointId.find(starLabels[idxI])->second);
          }
        }

        output->InsertNextCell(VTK_POLYHEDRON, nFaces, &(faceStream[0]));
        cellTableId->InsertNextValue(tableId);
      }
      else
      {
        // map orig vertex id -> point label
        for (int i=0; i < nLabels; ++i)
        {
          starLabels[i] = mapPointId.find(starLabels[i])->second;
        }

        switch (shapeId)
        {
          // 0-D
          case starcdPoint:
          output->InsertNextCell(VTK_VERTEX, 1, &(starLabels[0]));
          cellTableId->InsertNextValue(tableId);
          break;

          // 1-D
          case starcdLine:
          output->InsertNextCell(VTK_LINE, 2, &(starLabels[0]));
          cellTableId->InsertNextValue(tableId);
          break;

          // 2-D
          case starcdShell:
          switch (nLabels)
          {
            case 3:
            output->InsertNextCell(VTK_TRIANGLE, 3, &(starLabels[0]));
            break;
            case 4:
            output->InsertNextCell(VTK_QUAD, 4, &(starLabels[0]));
            break;
            default:
            output->InsertNextCell(VTK_POLYGON, nLabels, &(starLabels[0]));
            break;
          }
          cellTableId->InsertNextValue(tableId);
          break;

          // 3-D
          case starcdHex:
          output->InsertNextCell(VTK_HEXAHEDRON, 8, &(starLabels[0]));
          cellTableId->InsertNextValue(tableId);
          break;

          case starcdPrism:
          // the VTK definition has outwards normals for the triangles!!
          std::swap(starLabels[1], starLabels[2]);
          std::swap(starLabels[4], starLabels[5]);
          output->InsertNextCell(VTK_WEDGE, 6, &(starLabels[0]));
          cellTableId->InsertNextValue(tableId);
          break;

          case starcdTet:
          output->InsertNextCell(VTK_TETRA, 4, &(starLabels[0]));
          cellTableId->InsertNextValue(tableId);
          break;

          case starcdPyr:
          output->InsertNextCell(VTK_PYRAMID, 5, &(starLabels[0]));
          cellTableId->InsertNextValue(tableId);
          break;

          default: break;
        }
      }
    }
    else
    {
      vtkErrorMacro(<<"Error reading cell at line " << lineNr);
      ++errorCount;
    }
  }

  output->Squeeze();
  cellTableId->Squeeze();

  // now add the cellTableId array
  output->GetCellData()->AddArray(cellTableId);
  if (!output->GetCellData()->GetScalars())
  {
    output->GetCellData()->SetScalars(cellTableId);
  }
  cellTableId->Delete();

  vtkDebugMacro(<<"Read cells: " << lineNr << " errors: " << errorCount);

  fclose(in);
  return errorCount == 0;
}
