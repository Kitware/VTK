// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIVWriter.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStringFormatter.h"

#include <vtksys/SystemTools.hxx>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkIVWriter);

//------------------------------------------------------------------------------
bool vtkIVWriter::WriteDataAndReturn()
{
  FILE* fp;

  // make sure the user specified a FileName
  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Please specify FileName to use");
    return false;
  }

  // try opening the files
  fp = vtksys::SystemTools::Fopen(this->FileName, "w");
  if (!fp)
  {
    vtkErrorMacro(<< "unable to open OpenInventor file: " << this->FileName);
    return false;
  }

  //
  //  Write header
  //
  vtkDebugMacro("Writing OpenInventor file");
  vtk::print(fp, "#Inventor V2.0 ascii\n");
  vtk::print(fp, "# OpenInventor file written by the visualization toolkit\n\n");
  this->WritePolyData(this->GetInput(), fp);
  if (fclose(fp))
  {
    vtkErrorMacro(<< this->FileName << " did not close successfully. Check disk space.");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkIVWriter::WritePolyData(vtkPolyData* pd, FILE* fp)
{
  vtkPoints* points;
  vtkIdType i;
  vtkCellArray* cells;
  vtkIdType npts = 0;
  const vtkIdType* indx = nullptr;
  vtkUnsignedCharArray* colors = nullptr;

  points = pd->GetPoints();

  // create colors for vertices
  vtkDataArray* scalars = pd->GetPointData()->GetScalars();

  if (scalars)
  {
    vtkLookupTable* lut;
    if ((lut = scalars->GetLookupTable()) == nullptr)
    {
      lut = vtkLookupTable::New();
      lut->Build();
    }
    colors = lut->MapScalars(scalars, VTK_COLOR_MODE_DEFAULT, 0);
    if (!scalars->GetLookupTable())
    {
      lut->Delete();
    }
  }

  vtk::print(fp, "Separator {{\n");

  // Point data (coordinates)
  vtk::print(fp, "\tCoordinate3 {{\n");
  vtk::print(fp, "\t\tpoint [\n");
  vtk::print(fp, "\t\t\t");
  for (i = 0; i < points->GetNumberOfPoints(); i++)
  {
    double xyz[3];
    points->GetPoint(i, xyz);
    vtk::print(fp, "{:g} {:g} {:g}, ", xyz[0], xyz[1], xyz[2]);
    if (!((i + 1) % 2))
    {
      vtk::print(fp, "\n\t\t\t");
    }
  }
  vtk::print(fp, "\n\t\t]");
  vtk::print(fp, "\t}}\n");

  // Per vertex coloring
  vtk::print(fp, "\tMaterialBinding {{\n");
  vtk::print(fp, "\t\tvalue PER_VERTEX_INDEXED\n");
  vtk::print(fp, "\t}}\n");

  // Colors, if any
  if (colors)
  {
    vtk::print(fp, "\tMaterial {{\n");
    vtk::print(fp, "\t\tdiffuseColor [\n");
    vtk::print(fp, "\t\t\t");
    for (i = 0; i < colors->GetNumberOfTuples(); i++)
    {
      unsigned char* rgba;
      rgba = colors->GetPointer(4 * i);
      vtk::print(fp, "{:g} {:g} {:g}, ", rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f);
      if (!((i + 1) % 2))
      {
        vtk::print(fp, "\n\t\t\t");
      }
    }
    vtk::print(fp, "\n\t\t]\n");
    vtk::print(fp, "\t}}\n");
    colors->Delete();
  }

  // write out polys if any
  if (pd->GetNumberOfPolys() > 0)
  {
    vtk::print(fp, "\tIndexedFaceSet {{\n");
    vtk::print(fp, "\t\tcoordIndex [\n");
    cells = pd->GetPolys();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      vtk::print(fp, "\t\t\t");
      for (i = 0; i < npts; i++)
      {
        // treating vtkIdType as int
        vtk::print(fp, "{:d}, ", (int)indx[i]);
      }
      vtk::print(fp, "-1,\n");
    }
    vtk::print(fp, "\t\t]\n");
    vtk::print(fp, "\t}}\n");
  }

  // write out lines if any
  if (pd->GetNumberOfLines() > 0)
  {
    vtk::print(fp, "\tIndexedLineSet {{\n");
    vtk::print(fp, "\t\tcoordIndex  [\n");

    cells = pd->GetLines();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      vtk::print(fp, "\t\t\t");
      for (i = 0; i < npts; i++)
      {
        // treating vtkIdType as int
        vtk::print(fp, "{:d}, ", (int)indx[i]);
      }
      vtk::print(fp, "-1,\n");
    }
    vtk::print(fp, "\t\t]\n");
    vtk::print(fp, "\t}}\n");
  }

  // write out verts if any
  if (pd->GetNumberOfVerts() > 0)
  {
    vtk::print(fp, "\tIndexdedPointSet {{\n");
    vtk::print(fp, "\t\tcoordIndex [");
    cells = pd->GetVerts();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      vtk::print(fp, "\t\t\t");
      for (i = 0; i < npts; i++)
      {
        // treating vtkIdType as int
        vtk::print(fp, "{:d}, ", (int)indx[i]);
      }
      vtk::print(fp, "-1,\n");
    }
    vtk::print(fp, "\t\t]\n");
    vtk::print(fp, "\t}}\n");
  }

  // write out tstrips if any
  if (pd->GetNumberOfStrips() > 0)
  {

    vtk::print(fp, "\tIndexedTriangleStripSet {{\n");
    vtk::print(fp, "\t\tcoordIndex [\n");
    cells = pd->GetStrips();
    for (cells->InitTraversal(); cells->GetNextCell(npts, indx);)
    {
      vtk::print(fp, "\t\t\t");
      for (i = 0; i < npts; i++)
      {
        // treating vtkIdType as int
        vtk::print(fp, "{:d}, ", (int)indx[i]);
      }
      vtk::print(fp, "-1,\n");
    }
    vtk::print(fp, "\t\t]\n");
    vtk::print(fp, "\t}}\n");
  }

  vtk::print(fp, "}}\n"); // close the Shape
}

//------------------------------------------------------------------------------
void vtkIVWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkPolyData* vtkIVWriter::GetInput()
{
  return vtkPolyData::SafeDownCast(this->Superclass::GetInput());
}

//------------------------------------------------------------------------------
vtkPolyData* vtkIVWriter::GetInput(int port)
{
  return vtkPolyData::SafeDownCast(this->Superclass::GetInput(port));
}

//------------------------------------------------------------------------------
int vtkIVWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}
VTK_ABI_NAMESPACE_END
