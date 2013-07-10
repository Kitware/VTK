/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoJSONWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGeoJSONWriter.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkGeoJSONWriter);

#define VTK_GJWRITER_MAXPOINTS 32000

//------------------------------------------------------------------------------
vtkGeoJSONWriter::vtkGeoJSONWriter()
{
  this->FileName = NULL;
  this->OutputString = NULL;
  this->SetNumberOfOutputPorts(0);
  this->WriteToOutputString = false;
  this->ScalarFormat = 2;
  this->LookupTable = NULL;
}

//------------------------------------------------------------------------------
vtkGeoJSONWriter::~vtkGeoJSONWriter()
{
  this->SetFileName(NULL);
  delete[] this->OutputString;
  this->SetLookupTable(NULL);
}

//------------------------------------------------------------------------------
void vtkGeoJSONWriter::PrintSelf(ostream & os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: "
     << (this->FileName?this->FileName:"NONE") << endl;
  os << indent << "WriteToOutputString: "
     << (this->WriteToOutputString?"True":"False") << endl;
  os << indent << "ScalarFormat: " << this->ScalarFormat << endl;
}

//------------------------------------------------------------------------------
int vtkGeoJSONWriter::FillInputPortInformation(int port, vtkInformation *info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    }
  return 1;
}

//------------------------------------------------------------------------------
ostream *vtkGeoJSONWriter::OpenFile()
{
  vtkDebugMacro(<<"Opening file\n");

  ostream *fptr;

  if (!this->WriteToOutputString)
    {
    if (!this->FileName)
      {
      vtkErrorMacro(<< "No FileName specified! Can't write!");
      return NULL;
      }

    fptr = new ofstream(this->FileName, ios::out);
    }
  else
    {
    // Get rid of any old output string.
    if (this->OutputString)
      {
      delete [] this->OutputString;
      this->OutputString = NULL;
      this->OutputStringLength = 0;
      }
    fptr = new vtksys_ios::ostringstream;
    }

  if (fptr->fail())
    {
    vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
    delete fptr;
    return NULL;
    }

  return fptr;
}

//------------------------------------------------------------------------------
void vtkGeoJSONWriter::CloseFile(ostream *fp)
{
  vtkDebugMacro(<<"Closing file\n");

  if ( fp != NULL )
    {
    if (this->WriteToOutputString)
      {
      vtksys_ios::ostringstream *ostr =
        static_cast<vtksys_ios::ostringstream*>(fp);

      delete [] this->OutputString;
      this->OutputStringLength = static_cast<int>(ostr->str().size());
      //+1's account for null terminator
      this->OutputString = new char[ostr->str().size()+1];
      memcpy(this->OutputString, ostr->str().c_str(),
        this->OutputStringLength+1);
      }

    delete fp;
    }
}

//------------------------------------------------------------------------------
void vtkGeoJSONWriter::ConditionalComma(ostream *fp,
  vtkIdType cnt, vtkIdType limit)
{
  if (cnt+1 != limit)
    {
    *fp << ",";
    }
}

//------------------------------------------------------------------------------
void vtkGeoJSONWriter::WriteScalar(ostream *fp,
  vtkDataArray *da, vtkIdType ptId)
{
  if (this->ScalarFormat == 0)
  {
    return;
  }
  if (da)
  {
    double b = da->GetTuple1(ptId);
    if (this->ScalarFormat == 1)
    {
      vtkLookupTable *lut = this->GetLookupTable();
      if (!lut)
      {
        lut = vtkLookupTable::New();
        lut->SetNumberOfColors(256);
        lut->SetHueRange(0.0,0.667);
        lut->SetRange(da->GetRange());
        lut->Build();
        this->SetLookupTable(lut);
        lut->Delete();
      }
      unsigned char *color = lut->MapValue(b);
      *fp << ","
          << (double)color[0]/255.0 << ","
          << (double)color[1]/255.0 << ","
          << (double)color[2]/255;
    }
    else
    {
      if (vtkMath::IsNan(b))
        {
        *fp << "," << "null";
        }
      else
        {
        *fp << "," << b;
        }
    }
  }
}

//------------------------------------------------------------------------------
void vtkGeoJSONWriter::WriteData()
{
  ostream *fp;
  vtkPolyData *input = vtkPolyData::SafeDownCast(this->GetInput());

  vtkDebugMacro(<<"Writing vtk polygonal data to geojson file...");
  fp=this->OpenFile();
  if ( !fp )
    {
    return;
    }

  *fp << "{\n";
  *fp << "\"type\": \"Feature\",\n";
  vtkDataArray *da = input->GetPointData()->GetScalars();
  if (!da)
  {
    da = input->GetPointData()->GetArray(0);
  }
  if (da)
  {
    switch (this->ScalarFormat) {
    case 0:
      *fp << "\"properties\": {\"ScalarFormat\": \"none\"},\n";
      break;
    case 1:
      *fp << "\"properties\": {\"ScalarFormat\": \"rgb\"},\n";
      break;
    case 2:
      double rng[2];
      da->GetRange(rng);
      *fp << "\"properties\": {\"ScalarFormat\": \"values\", \"ScalarRange\": [" << rng[0] << "," << rng[1] << "] },\n";
      break;
    }
  }
  else
  {
    *fp << "\"properties\": {\"ScalarFormat\": \"none\"},\n";
  }
  *fp << "\"geometry\":\n";
  *fp << "{\n";
  *fp << "\"type\": \"GeometryCollection\",\n";
  *fp << "\"geometries\":\n";
  *fp << "[\n";

  vtkIdType cellLoc = 0;
  vtkIdType *cellPts = NULL;
  vtkIdType cellSize = 0;
  vtkIdType numlines, numpolys;
  numlines = input->GetLines()->GetNumberOfCells();
  numpolys = input->GetPolys()->GetNumberOfCells();

  //VERTS
  vtkCellArray *ca;
  ca = input->GetVerts();
  if (ca && ca->GetNumberOfCells())
    {
    bool done = false;
    vtkIdType inCell = 0;
    vtkIdType ptCnt = 0;
    do //loop to break into sections with < VTK_GJWRITER_MAXPOINTS points
      {
      *fp << "{\n";
      *fp << "\"type\": \"MultiPoint\",\n";
      *fp << "\"coordinates\":\n";
      *fp << "[\n";
      for (; inCell < ca->GetNumberOfCells() && ptCnt < VTK_GJWRITER_MAXPOINTS; inCell++)
        {
        ca->GetCell(cellLoc, cellSize, cellPts);
        cellLoc += cellSize+1;
        ptCnt += cellSize;
        vtkIdType inPt;
        for (inPt = 0; inPt < cellSize; inPt++)
          {
          double coords[3];
          input->GetPoint(cellPts[inPt], coords);
          *fp << "[" << coords[0] << "," << coords[1] << "," << coords[2];
          this->WriteScalar(fp, da, cellPts[inPt]);
          *fp << "]";
          this->ConditionalComma(fp, inPt, cellSize);
          }
        if (ptCnt<VTK_GJWRITER_MAXPOINTS)
          {
          this->ConditionalComma(fp, inCell, ca->GetNumberOfCells());
          }
        *fp << "\n";
        }
      *fp << "]"; //coordinates for this cell array
      if (inCell < ca->GetNumberOfCells())
        {
        ptCnt = 0;
        *fp << ",\n";
        }
      else
        {
        if (numlines || numpolys)
          {
          *fp << ",";
          }
        done = true;
        }
      } while (!done);
    }

  //lines
  ca = input->GetLines();
  if (ca && ca->GetNumberOfCells())
    {
    bool done = false;
    vtkIdType inCell = 0;
    vtkIdType ptCnt = 0;
    do //loop to break into sections with < VTK_GJWRITER_MAXPOINTS points
      {
      *fp << "{\n";
      *fp << "\"type\": \"MultiLineString\",\n";
      *fp << "\"coordinates\":\n";
      *fp << "[\n";
      for (; inCell < ca->GetNumberOfCells() && ptCnt < VTK_GJWRITER_MAXPOINTS; inCell++)
        {
        *fp << "[ "; //one cell
        ca->GetCell(cellLoc, cellSize, cellPts);
        cellLoc += cellSize+1;
        ptCnt += cellSize;
        vtkIdType inPt;
        for (inPt = 0; inPt < cellSize; inPt++)
          {
          double coords[3];
          input->GetPoint(cellPts[inPt], coords);
          *fp << "[" << coords[0] << "," << coords[1] << "," << coords[2];
          this->WriteScalar(fp, da, cellPts[inPt]);
          *fp << "]";
          this->ConditionalComma(fp, inPt, cellSize);
          }
        *fp << " ]";//one cell
        if (ptCnt<VTK_GJWRITER_MAXPOINTS)
          {
          this->ConditionalComma(fp, inCell, ca->GetNumberOfCells());
          }
        *fp << "\n";
        }
      *fp << "]"; //coordinates for this cell array
      *fp << "\n";
      *fp << "}\n"; //this cell array
      if (inCell < ca->GetNumberOfCells())
        {
        ptCnt = 0;
        *fp << ",\n";
        }
      else
        {
        if (numpolys)
          {
          *fp << ",";
          }
        done = true;
        }
      } while (!done);
    }

  //polygons
  ca = input->GetPolys();
  if (ca && ca->GetNumberOfCells())
    {
    bool done = false;
    vtkIdType inCell = 0;
    vtkIdType ptCnt = 0;
    do //loop to break into sections with < VTK_GJWRITER_MAXPOINTS points
      {
      *fp << "{\n";
      *fp << "\"type\": \"MultiPolygon\",\n";
      *fp << "\"coordinates\":\n";
      *fp << "[\n";
      for (; inCell < ca->GetNumberOfCells() && ptCnt < VTK_GJWRITER_MAXPOINTS; inCell++)
        {
        *fp << "[[ "; //one cell
        ca->GetCell(cellLoc, cellSize, cellPts);
        cellLoc += cellSize+1;
        ptCnt += cellSize;
        vtkIdType inPt;
        for (inPt = 0; inPt < cellSize; inPt++)
          {
          double coords[3];
          input->GetPoint(cellPts[inPt], coords);
          *fp << "[" << coords[0] << "," << coords[1] << "," << coords[2];
          this->WriteScalar(fp, da, cellPts[inPt]);
          *fp << "]";
          this->ConditionalComma(fp, inPt, cellSize);
          }
        *fp << " ]]";//one cell
        if (ptCnt<VTK_GJWRITER_MAXPOINTS)
          {
          this->ConditionalComma(fp, inCell, ca->GetNumberOfCells());
          }
        *fp << "\n";
        }
      *fp << "]"; //coordinates for this cell array
      *fp << "\n";
      *fp << "}\n"; //this cell array
      if (inCell < ca->GetNumberOfCells())
        {
        ptCnt = 0;
        *fp << ",\n";
        }
      else
        {
        done = true;
        }
      } while (!done);
    }

  *fp << "]\n";//feature.geometry.GeometryCollection.geometries
  *fp << "}\n";//feature.geometry
  *fp << "}\n";//feature

  fp->flush();
  if (fp->fail())
    {
    vtkErrorMacro("Problem writing result check disk space.");
    delete fp;
    fp = NULL;
    }

  this->CloseFile(fp);
}

//------------------------------------------------------------------------------
char *vtkGeoJSONWriter::RegisterAndGetOutputString()
{
  char *tmp = this->OutputString;

  this->OutputString = NULL;
  this->OutputStringLength = 0;

  return tmp;
}

//------------------------------------------------------------------------------
vtkStdString vtkGeoJSONWriter::GetOutputStdString()
{
  return vtkStdString(this->OutputString, this->OutputStringLength);
}

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkGeoJSONWriter, LookupTable, vtkLookupTable)
