/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBJWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOBJWriter.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkNumberToString.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTriangleStrip.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <utility>

namespace
{
//------------------------------------------------------------------------------
void WriteFaces(std::ostream& f, vtkCellArray* faces, bool withNormals, bool withTCoords)
{
  vtkIdType npts;
  const vtkIdType* indx;
  for (faces->InitTraversal(); faces->GetNextCell(npts, indx);)
  {
    f << "f";
    for (vtkIdType i = 0; i < npts; i++)
    {
      f << " " << indx[i] + 1;
      if (withTCoords)
      {
        f << "/" << indx[i] + 1;
        if (withNormals)
        {
          f << "/" << indx[i] + 1;
        }
      }
      else if (withNormals)
      {
        f << "//" << indx[i] + 1;
      }
    }
    f << "\n";
  }
}

//----------------------------------------------------------------------------
void WriteLines(std::ostream& f, vtkCellArray* lines)
{
  vtkIdType npts;
  const vtkIdType* indx;
  for (lines->InitTraversal(); lines->GetNextCell(npts, indx);)
  {
    f << "l";
    for (vtkIdType i = 0; i < npts; i++)
    {
      f << " " << indx[i] + 1;
    }
    f << "\n";
  }
}

struct EndIndex
{
  EndIndex(vtkIdType vtEndIndex, vtkIdType pointEndIndex)
    : VtEndIndex(vtEndIndex)
    , PointEndIndex(pointEndIndex)
  {
  }
  // index of the point after last point for that material in vt array
  vtkIdType VtEndIndex;
  // index of the point after last point for that material in the point array
  // for that material
  vtkIdType PointEndIndex;
};
//----------------------------------------------------------------------------
void WritePoints(std::ostream& f, vtkPoints* pts, vtkDataArray* normals,
  const std::vector<vtkDataArray*>& tcoordsArray, std::vector<EndIndex>* endIndexes)
{
  vtkNumberToString convert;
  vtkIdType nbPts = pts->GetNumberOfPoints();

  // Positions
  for (vtkIdType i = 0; i < nbPts; i++)
  {
    double p[3];
    pts->GetPoint(i, p);
    f << "v " << convert(p[0]) << " " << convert(p[1]) << " " << convert(p[2]) << "\n";
  }

  // Normals
  if (normals)
  {
    for (vtkIdType i = 0; i < nbPts; i++)
    {
      double p[3];
      normals->GetTuple(i, p);
      f << "vn " << convert(p[0]) << " " << convert(p[1]) << " " << convert(p[2]) << "\n";
    }
  }

  // Textures
  if (!tcoordsArray.empty())
  {
    vtkIdType vtEndIndex = 0;
    vtkIdType pointEndIndex = 0;
    for (size_t tcoordsIndex = 0; tcoordsIndex < tcoordsArray.size(); ++tcoordsIndex)
    {
      f << "# tcoords array " << tcoordsIndex << "\n";
      vtkDataArray* tcoords = tcoordsArray[tcoordsIndex];
      if (tcoords)
      {
        for (vtkIdType i = 0; i < nbPts; i++)
        {
          double p[2];
          tcoords->GetTuple(i, p);
          if (p[0] != -1.0)
          {
            f << "vt " << convert(p[0]) << " " << convert(p[1]) << "\n";
            ++vtEndIndex;
            pointEndIndex = i + 1;
          }
        }
        endIndexes->push_back(EndIndex(vtEndIndex, pointEndIndex));
      }
      else
      {
        // there are no vertex textures (vt) for no_material
        endIndexes->push_back(EndIndex(-1, -1));
      }
    }
  }
}

//----------------------------------------------------------------------------
bool WriteMtl(const std::string& baseName, const char* textureFileName)
{
  std::string mtlFileName = baseName + ".mtl";
  vtksys::ofstream fmtl(mtlFileName.c_str(), vtksys::ofstream::out);
  if (fmtl.fail())
  {
    return false;
  }

  // remove directories
  mtlFileName = vtksys::SystemTools::GetFilenameName(mtlFileName);

  // set material
  std::string mtlName = vtksys::SystemTools::GetFilenameName(baseName);
  fmtl << "newmtl " << mtlName << "\n";
  fmtl << "map_Kd " << textureFileName << "\n";
  return true;
}
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOBJWriter);

//------------------------------------------------------------------------------
vtkOBJWriter::vtkOBJWriter()
{
  this->FileName = nullptr;
  this->TextureFileName = nullptr;
  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
vtkOBJWriter::~vtkOBJWriter()
{
  this->SetFileName(nullptr);
  this->SetTextureFileName(nullptr);
}

//------------------------------------------------------------------------------
void vtkOBJWriter::WriteData()
{
  vtkPolyData* input = this->GetInputGeometry();
  vtkImageData* texture = this->GetInputTexture();

  if (input == nullptr)
  {
    vtkErrorMacro("No geometry to write!");
    this->SetErrorCode(vtkErrorCode::UnknownError);
    return;
  }

  vtkPoints* pts = input->GetPoints();
  vtkCellArray* polys = input->GetPolys();
  vtkCellArray* strips = input->GetStrips();
  vtkCellArray* lines = input->GetLines();
  vtkDataArray* normals = input->GetPointData()->GetNormals();
  std::vector<vtkDataArray*> tcoordsArray;
  vtkStringArray* mtllibArray =
    vtkStringArray::SafeDownCast(input->GetFieldData()->GetAbstractArray("MaterialLibraries"));
  vtkStringArray* matNames =
    vtkStringArray::SafeDownCast(input->GetFieldData()->GetAbstractArray("MaterialNames"));
  if (matNames)
  {
    for (int i = 0; i < matNames->GetNumberOfTuples(); ++i)
    {
      std::string matName = matNames->GetValue(i);
      vtkDataArray* tcoords = input->GetPointData()->GetArray(matName.c_str());
      // for no_material we store a nullptr
      tcoordsArray.push_back(tcoords);
    }
  }
  else
  {
    vtkDataArray* tcoords = input->GetPointData()->GetTCoords();
    if (tcoords)
    {
      tcoordsArray.push_back(tcoords);
    }
  }

  if (pts == nullptr)
  {
    vtkErrorMacro("No data to write!");
    this->SetErrorCode(vtkErrorCode::UnknownError);
    return;
  }

  if (this->FileName == nullptr)
  {
    vtkErrorMacro("Please specify FileName to write");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return;
  }

  vtkIdType npts = 0;

  vtksys::ofstream f(this->FileName, vtksys::ofstream::out);
  if (f.fail())
  {
    vtkErrorMacro("Unable to open file: " << this->FileName);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return;
  }

  // Write header
  f << "# Generated by Visualization Toolkit\n";

  if (texture && this->TextureFileName)
  {
    // resolve conflict
    vtkWarningMacro("Both a vtkImageData on port 1 and the TextureFileName are set. "
                    "Using TextureFileName.");
    texture = nullptr;
  }

  std::vector<std::string> comp;
  vtksys::SystemTools::SplitPath(vtksys::SystemTools::GetFilenamePath(this->FileName), comp);
  comp.push_back(vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName));
  std::string baseName = vtksys::SystemTools::JoinPath(comp);
  if (texture || this->TextureFileName)
  {
    std::string textureFileName = texture ? baseName + ".png" : this->TextureFileName;
    if (!::WriteMtl(baseName, textureFileName.c_str()))
    {
      vtkErrorMacro("Unable to create material file");
    }
    if (texture)
    {
      vtkNew<vtkPNGWriter> pngWriter;
      pngWriter->SetInputData(texture);
      pngWriter->SetFileName(textureFileName.c_str());
      pngWriter->Write();
    }
    // write mtllib line
    std::string mtlFileName = baseName + ".mtl";
    std::string mtlName = vtksys::SystemTools::GetFilenameName(mtlFileName);
    f << "mtllib " + mtlName + "\n";
  }
  if (mtllibArray)
  {
    for (int i = 0; i < mtllibArray->GetNumberOfTuples(); ++i)
    {
      f << "mtllib " + mtllibArray->GetValue(i) + "\n";
    }
  }

  std::vector<EndIndex> endIndexes;
  ::WritePoints(f, pts, normals, tcoordsArray, &endIndexes);

  // Decompose any triangle strips into triangles
  vtkNew<vtkCellArray> polyStrips;
  if (strips->GetNumberOfCells() > 0)
  {
    const vtkIdType* ptIds = nullptr;
    for (strips->InitTraversal(); strips->GetNextCell(npts, ptIds);)
    {
      vtkTriangleStrip::DecomposeStrip(npts, ptIds, polyStrips);
    }
  }

  // Write material if a texture is specified
  if (texture || this->TextureFileName)
  {
    // declare material in obj file
    std::string mtlName = vtksys::SystemTools::GetFilenameName(baseName);
    f << "usemtl " << mtlName << "\n";
  }
  if (matNames)
  {
    vtkIdType cellNpts;
    const vtkIdType* indx;
    polys->InitTraversal();
    int validCell = polys->GetNextCell(cellNpts, indx);
    vtkIdType faceIndex = 0;
    vtkIntArray* materialIds =
      vtkIntArray::SafeDownCast(input->GetCellData()->GetArray("MaterialIds"));
    for (vtkIdType matIndex = 0; matIndex < matNames->GetNumberOfTuples(); ++matIndex)
    {
      std::string matName = matNames->GetValue(matIndex);
      vtkDataArray* tcoords = input->GetPointData()->GetArray(matName.c_str());
      if (tcoords)
      {
        f << "usemtl " << matName << "\n";
      }
      while (materialIds->GetValue(faceIndex) == matIndex && validCell)
      {
        f << "f";
        for (vtkIdType i = 0; i < cellNpts; i++)
        {
          f << " " << indx[i] + 1;
          if (tcoords)
          {
            EndIndex endIndex = endIndexes[matIndex];
            vtkIdType vtIndex = endIndex.VtEndIndex - endIndex.PointEndIndex + indx[i];
            f << "/" << vtIndex + 1;
          }
        }
        f << "\n";
        ++faceIndex;
        validCell = polys->GetNextCell(cellNpts, indx);
      }
    }
  }
  else
  {
    // Write triangle strips
    ::WriteFaces(f, polyStrips, normals != nullptr, !tcoordsArray.empty());

    // Write polygons.
    if (polys)
    {
      ::WriteFaces(f, polys, normals != nullptr, !tcoordsArray.empty());
    }

    // Write lines.
    if (lines)
    {
      ::WriteLines(f, lines);
    }
  }

  f.close();
}

//------------------------------------------------------------------------------
void vtkOBJWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->GetFileName() ? this->GetFileName() : "(none)") << endl;
  os << indent << "Input: " << this->GetInputGeometry() << endl;

  vtkImageData* texture = this->GetInputTexture();
  if (texture)
  {
    os << indent << "Texture:" << endl;
    texture->PrintSelf(os, indent.GetNextIndent());
  }
}

//------------------------------------------------------------------------------
vtkPolyData* vtkOBJWriter::GetInputGeometry()
{
  return vtkPolyData::SafeDownCast(this->GetInput(0));
}

//------------------------------------------------------------------------------
vtkImageData* vtkOBJWriter::GetInputTexture()
{
  return vtkImageData::SafeDownCast(this->GetInput(1));
}

//------------------------------------------------------------------------------
vtkDataSet* vtkOBJWriter::GetInput(int port)
{
  return vtkDataSet::SafeDownCast(this->Superclass::GetInput(port));
}

//------------------------------------------------------------------------------
int vtkOBJWriter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
  }
  return 0;
}
