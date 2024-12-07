// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImporter.h"

#include "vtkAbstractArray.h"
#include "vtkActorCollection.h"
#include "vtkCellData.h"
#include "vtkCollection.h"
#include "vtkDataSet.h"
#include "vtkLightCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
vtkCxxSetObjectMacro(vtkImporter, RenderWindow, vtkRenderWindow);

//------------------------------------------------------------------------------
vtkImporter::vtkImporter() = default;

//------------------------------------------------------------------------------
vtkImporter::~vtkImporter()
{
  this->SetRenderWindow(nullptr);

  if (this->Renderer)
  {
    this->Renderer->UnRegister(nullptr);
    this->Renderer = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkImporter::ReadData()
{
  // this->Import actors, cameras, lights and properties
  // Do not check for UpdateStatus but try to import all that is possible
  this->ImportActors(this->Renderer);
  this->ImportCameras(this->Renderer);
  this->ImportLights(this->Renderer);
  this->ImportProperties(this->Renderer);
}

//------------------------------------------------------------------------------
bool vtkImporter::Update()
{
  this->UpdateStatus = vtkImporter::UpdateStatusEnum::SUCCESS;

  // if there is no render window, create one
  if (this->RenderWindow == nullptr)
  {
    vtkDebugMacro(<< "Creating a RenderWindow\n");
    this->RenderWindow = vtkRenderWindow::New();
  }

  // Get the first renderer in the render window
  vtkRenderer* renderer = this->RenderWindow->GetRenderers()->GetFirstRenderer();
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
  else
  {
    this->UpdateStatus = vtkImporter::UpdateStatusEnum::FAILURE;
  }

  return this->UpdateStatus == vtkImporter::UpdateStatusEnum::SUCCESS;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
std::string vtkImporter::GetArrayDescription(vtkAbstractArray* array, vtkIndent indent)
{
  std::stringstream ss;
  ss << indent;
  if (array->GetName())
  {
    ss << array->GetName() << " : ";
  }
  ss << array->GetDataTypeAsString() << " : ";

  vtkIdType nbTuples = array->GetNumberOfTuples();

  if (nbTuples == 1)
  {
    ss << array->GetVariantValue(0).ToString();
  }
  else
  {
    int nComp = array->GetNumberOfComponents();
    double range[2];
    for (int j = 0; j < nComp; j++)
    {
      vtkDataArray* dataArray = vtkDataArray::SafeDownCast(array);
      if (dataArray)
      {
        dataArray->GetRange(range, j);
        ss << "[" << range[0] << ", " << range[1] << "] ";
      }
      else
      {
        ss << "[range unavailable] ";
      }
    }
  }
  ss << "\n";
  return ss.str();
}

//------------------------------------------------------------------------------
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
  vtkFieldData* fieldData = ds->GetFieldData();
  int nbPointData = pointData->GetNumberOfArrays();
  int nbCellData = cellData->GetNumberOfArrays();
  int nbFieldData = fieldData->GetNumberOfArrays();

  ss << indent << nbPointData << " point data array(s):\n";
  for (vtkIdType i = 0; i < nbPointData; i++)
  {
    vtkAbstractArray* array = pointData->GetAbstractArray(i);
    ss << vtkImporter::GetArrayDescription(array, indent.GetNextIndent());
  }

  ss << indent << nbCellData << " cell data array(s):\n";
  for (vtkIdType i = 0; i < nbCellData; i++)
  {
    vtkAbstractArray* array = cellData->GetAbstractArray(i);
    ss << vtkImporter::GetArrayDescription(array, indent.GetNextIndent());
  }

  ss << indent << nbFieldData << " field data array(s):\n";
  for (vtkIdType i = 0; i < nbFieldData; i++)
  {
    vtkAbstractArray* array = fieldData->GetAbstractArray(i);
    if (array)
    {
      ss << vtkImporter::GetArrayDescription(array, indent.GetNextIndent());
    }
  }

  return ss.str();
}

//------------------------------------------------------------------------------
vtkIdType vtkImporter::GetNumberOfAnimations()
{
  return -1;
}

//------------------------------------------------------------------------------
bool vtkImporter::GetTemporalInformation(vtkIdType vtkNotUsed(animationIdx),
  double vtkNotUsed(frameRate), int& vtkNotUsed(nbTimeSteps), double vtkNotUsed(timeRange)[2],
  vtkDoubleArray* vtkNotUsed(timeSteps))
{
  return false;
}

//------------------------------------------------------------------------------
void vtkImporter::UpdateTimeStep(double timeValue)
{
  this->UpdateAtTimeValue(timeValue);
}

//------------------------------------------------------------------------------
bool vtkImporter::UpdateAtTimeValue(double vtkNotUsed(timeValue))
{
  return this->Update();
}
VTK_ABI_NAMESPACE_END
