// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOBJWriter.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkTriangleStrip.h"
#include "vtkUnsignedCharArray.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <utility>

VTK_ABI_NAMESPACE_BEGIN
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
vtkUnsignedCharArray* GetColorArray(vtkOBJWriter* writer, vtkDataSetAttributes* dsa)
{
  if (!writer->GetWriteColorArray())
  {
    return nullptr;
  }
  else // we will color based on data
  {
    if (writer->GetColorArrayName().empty())
    {
      return nullptr;
    }
    if (vtkUnsignedCharArray* colorArray =
          vtkArrayDownCast<vtkUnsignedCharArray>(dsa->GetArray(writer->GetColorArrayName().data()));
        colorArray != nullptr)
    {
      int numComp = colorArray->GetNumberOfComponents();
      return (numComp == 3 || numComp == 4) ? colorArray : nullptr;
    }
    return nullptr;
  }
}

//----------------------------------------------------------------------------
void WritePoints(std::ostream& f, vtkPoints* pts, vtkDataArray* normals,
  vtkUnsignedCharArray* pointColors, const std::vector<vtkDataArray*>& tcoordsArray,
  std::vector<EndIndex>* endIndexes)
{
  vtkIdType nbPts = pts->GetNumberOfPoints();

  bool writeColors = false;
  if (pointColors && pointColors->GetNumberOfTuples() == nbPts)
  {
    writeColors = true;
  }
  // Positions
  for (vtkIdType i = 0; i < nbPts; i++)
  {
    double p[3];
    pts->GetPoint(i, p);
    f << vtk::format("v {} {} {}", p[0], p[1], p[2]);
    if (writeColors)
    {
      unsigned char color[4] = { 255, 255, 255, 255 };
      pointColors->GetTypedTuple(i, color);
      if (pointColors->GetNumberOfComponents() == 3) // RGB
      {
        f << vtk::format(" {} {} {}", static_cast<double>(color[0] / 255.0),
          static_cast<double>(color[1] / 255.0), static_cast<double>(color[2] / 255.0));
      }
      else if (pointColors->GetNumberOfComponents() == 4) // RGBA
      {
        f << vtk::format(" {} {} {} {}", static_cast<double>(color[0] / 255.0),
          static_cast<double>(color[1] / 255.0), static_cast<double>(color[2] / 255.0),
          static_cast<double>(color[3] / 255.0));
      }
    }
    f << "\n";
  }

  // Normals
  if (normals)
  {
    for (vtkIdType i = 0; i < nbPts; i++)
    {
      double p[3];
      normals->GetTuple(i, p);
      f << vtk::format("vn {} {} {}\n", p[0], p[1], p[2]);
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
            f << vtk::format("vt {} {}\n", p[0], p[1]);
            ++vtEndIndex;
            pointEndIndex = i + 1;
          }
        }
        endIndexes->emplace_back(vtEndIndex, pointEndIndex);
      }
      else
      {
        // there are no vertex textures (vt) for no_material
        endIndexes->emplace_back(-1, -1);
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
  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
vtkOBJWriter::~vtkOBJWriter()
{
  this->SetFileName(nullptr);
  this->SetTextureFileName(nullptr);
}

//------------------------------------------------------------------------------
bool vtkOBJWriter::WriteDataAndReturn()
{
  vtkPolyData* input = this->GetInputGeometry();
  vtkImageData* texture = this->GetInputTexture();

  if (input == nullptr)
  {
    vtkErrorMacro("No geometry to write!");
    this->SetErrorCode(vtkErrorCode::UnknownError);
    return false;
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
    return false;
  }

  if (this->FileName == nullptr)
  {
    vtkErrorMacro("Please specify FileName to write");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return false;
  }

  vtkIdType npts = 0;

  vtksys::ofstream f(this->FileName, vtksys::ofstream::out);
  if (f.fail())
  {
    vtkErrorMacro("Unable to open file: " << this->FileName);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return false;
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
    std::string textureName = textureFileName;
    if (this->UseRelativeTexturePath)
    {
      textureName = vtksys::SystemTools::GetFilenameName(textureFileName);
    }
    if (!::WriteMtl(baseName, textureName.c_str()))
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

  vtkUnsignedCharArray* pointColors = GetColorArray(this, input->GetPointData());

  std::vector<EndIndex> endIndexes;
  ::WritePoints(f, pts, normals, pointColors, tcoordsArray, &endIndexes);

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
      while (validCell && materialIds->GetValue(faceIndex) == matIndex)
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
  return true;
}

//------------------------------------------------------------------------------
void vtkOBJWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->GetFileName() ? this->GetFileName() : "(none)") << endl;
  os << indent << "Input: " << this->GetInputGeometry() << endl;

  os << indent << "Write Color Array: " << (this->WriteColorArray ? "off" : "on") << "\n";
  os << indent
     << "Color Array Name: " << (this->ColorArrayName.empty() ? this->ColorArrayName : "(none)")
     << "\n";

  os << indent << "Use Relative Texture Path: " << (this->UseRelativeTexturePath ? "on" : "off")
     << "\n";
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

VTK_ABI_NAMESPACE_END
