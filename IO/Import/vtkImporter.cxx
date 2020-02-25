/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImporter.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"

#include <sstream>

vtkCxxSetObjectMacro(vtkImporter, RenderWindow, vtkRenderWindow);

vtkImporter::vtkImporter()
{
  this->Renderer = nullptr;
  this->RenderWindow = nullptr;
}

vtkImporter::~vtkImporter()
{
  this->SetRenderWindow(nullptr);

  if (this->Renderer)
  {
    this->Renderer->UnRegister(nullptr);
    this->Renderer = nullptr;
  }
}

void vtkImporter::ReadData()
{
  // this->Import actors, cameras, lights and properties
  this->ImportActors(this->Renderer);
  this->ImportCameras(this->Renderer);
  this->ImportLights(this->Renderer);
  this->ImportProperties(this->Renderer);
}

void vtkImporter::Read()
{
  vtkRenderer* renderer;

  // if there is no render window, create one
  if (this->RenderWindow == nullptr)
  {
    vtkDebugMacro(<< "Creating a RenderWindow\n");
    this->RenderWindow = vtkRenderWindow::New();
  }

  // Get the first renderer in the render window
  renderer = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  if (renderer == nullptr)
  {
    vtkDebugMacro(<< "Creating a Renderer\n");
    this->Renderer = vtkRenderer::New();
    renderer = this->Renderer;
    this->RenderWindow->AddRenderer(renderer);
  }
  else
  {
    if (this->Renderer)
    {
      this->Renderer->UnRegister(nullptr);
    }
    this->Renderer = renderer;
    this->Renderer->Register(this);
  }

  if (this->ImportBegin())
  {
    this->ReadData();
    this->ImportEnd();
  }
}

void vtkImporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Render Window: ";
  if (this->RenderWindow)
  {
    os << this->RenderWindow << "\n";
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Renderer: ";
  if (this->Renderer)
  {
    os << this->Renderer << "\n";
  }
  else
  {
    os << "(none)\n";
  }
}

//----------------------------------------------------------------------------
std::string vtkImporter::GetArrayDescription(vtkDataArray* array, vtkIndent indent)
{
  std::stringstream ss;
  ss << indent;
  if (array->GetName())
  {
    ss << array->GetName() << " : ";
  }
  ss << array->GetDataTypeAsString() << " : ";

  int nComp = array->GetNumberOfComponents();
  double range[2];
  for (int j = 0; j < nComp; j++)
  {
    array->GetRange(range, j);
    ss << "[" << range[0] << ", " << range[1] << "] ";
  }
  ss << "\n";
  return ss.str();
}

//----------------------------------------------------------------------------
std::string vtkImporter::GetDataSetDescription(vtkDataSet* ds, vtkIndent indent)
{
  std::stringstream ss;
  ss << indent << "Number of points: " << ds->GetNumberOfPoints() << "\n";

  vtkPolyData* pd = vtkPolyData::SafeDownCast(ds);
  if (pd)
  {
    ss << indent << "Number of polygons: " << pd->GetNumberOfPolys() << "\n";
    ss << indent << "Number of lines: " << pd->GetNumberOfLines() << "\n";
    ss << indent << "Number of vertices: " << pd->GetNumberOfVerts() << "\n";
  }
  else
  {
    ss << indent << "Number of cells: " << ds->GetNumberOfCells() << "\n";
  }

  vtkPointData* pointData = ds->GetPointData();
  vtkCellData* cellData = ds->GetCellData();
  int nbPointData = pointData->GetNumberOfArrays();
  int nbCellData = cellData->GetNumberOfArrays();

  ss << indent << nbPointData << " point data array(s):\n";
  for (vtkIdType i = 0; i < nbPointData; i++)
  {
    vtkDataArray* array = pointData->GetArray(i);
    ss << vtkImporter::GetArrayDescription(array, indent.GetNextIndent());
  }

  ss << indent << nbCellData << " cell data array(s):\n";
  for (vtkIdType i = 0; i < nbCellData; i++)
  {
    vtkDataArray* array = cellData->GetArray(i);
    ss << vtkImporter::GetArrayDescription(array, indent.GetNextIndent());
  }
  return ss.str();
}

//----------------------------------------------------------------------------
vtkIdType vtkImporter::GetNumberOfAnimations()
{
  return -1;
}

//----------------------------------------------------------------------------
bool vtkImporter::GetTemporalInformation(vtkIdType vtkNotUsed(animationIdx),
  int& vtkNotUsed(nbTimeSteps), double vtkNotUsed(timeRange)[2],
  vtkDoubleArray* vtkNotUsed(timeSteps))
{
  return false;
}

//----------------------------------------------------------------------------
void vtkImporter::UpdateTimeStep(double vtkNotUsed(timeStep))
{
  this->Update();
}
