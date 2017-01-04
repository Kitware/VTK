/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLWriterC.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLWriterC.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkXMLRectilinearGridWriter.h"
#include "vtkXMLStructuredGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"

// Function to allocate a vtkDataArray and point it at the given data.
// The data are not copied.
static
vtkSmartPointer<vtkDataArray>
vtkXMLWriterC_NewDataArray(const char* method, const char* name, int dataType,
                           void* data, vtkIdType numTuples, int numComponents);

// Function to allocate a vtkCellArray and point it at the given
// cells.  The cells are not copied.
static
vtkSmartPointer<vtkCellArray>
vtkXMLWriterC_NewCellArray(const char* method, vtkIdType ncells,
                           vtkIdType* cells, vtkIdType cellsSize);

// Function to implement vtkXMLWriterC_SetPointData and
// vtkXMLWriterC_SetCellData without duplicate code.
static
void vtkXMLWriterC_SetDataInternal(vtkXMLWriterC* self, const char* name,
                                   int dataType, void* data,
                                   vtkIdType numTuples, int numComponents,
                                   const char* role, const char* method,
                                   int isPoints);

extern "C"
{

//----------------------------------------------------------------------------
// Define the interface structure.  Note this is a C struct so it has no
// real constructor or destructor.
struct vtkXMLWriterC_s
{
  vtkSmartPointer<vtkXMLWriter> Writer;
  vtkSmartPointer<vtkDataObject> DataObject;
  int Writing;
};

//----------------------------------------------------------------------------
vtkXMLWriterC* vtkXMLWriterC_New(void)
{
  if(vtkXMLWriterC* self = new vtkXMLWriterC)
  {
    // Initialize the object.
    self->Writer = 0;
    self->DataObject = 0;
    self->Writing = 0;
    return self;
  }
  else
  {
    vtkGenericWarningMacro("Failed to allocate a vtkXMLWriterC object.");
    return 0;
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_Delete(vtkXMLWriterC* self)
{
  if(self)
  {
    // Finalize the object.
    self->Writer = 0;
    self->DataObject = 0;
    delete self;
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_SetDataObjectType(vtkXMLWriterC* self, int objType)
{
  if(!self) { return; }
  if(!self->DataObject)
  {
    // Create the writer and data object.
    switch(objType)
    {
      case VTK_POLY_DATA:
      {
        self->DataObject = vtkSmartPointer<vtkPolyData>::New();
        self->Writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
      }; break;
      case VTK_UNSTRUCTURED_GRID:
      {
        self->DataObject = vtkSmartPointer<vtkUnstructuredGrid>::New();
        self->Writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
      }; break;
      case VTK_STRUCTURED_GRID:
      {
        self->DataObject = vtkSmartPointer<vtkStructuredGrid>::New();
        self->Writer = vtkSmartPointer<vtkXMLStructuredGridWriter>::New();
      }; break;
      case VTK_RECTILINEAR_GRID:
      {
        self->DataObject = vtkSmartPointer<vtkRectilinearGrid>::New();
        self->Writer = vtkSmartPointer<vtkXMLRectilinearGridWriter>::New();
      }; break;
      case VTK_IMAGE_DATA:
      {
        self->DataObject = vtkSmartPointer<vtkImageData>::New();
        self->Writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
      }; break;
    }

    // Set the data object as input to the writer.
    if(self->Writer && self->DataObject)
    {
      self->Writer->SetInputData(self->DataObject);
    }
    else
    {
      vtkGenericWarningMacro(
        "Failed to allocate data object and writer for type " << objType << "."
        );
    }
  }
  else
  {
    vtkGenericWarningMacro("vtkXMLWriterC_SetDataObjectType called twice.");
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_SetDataModeType(vtkXMLWriterC* self, int datamodetype)
{
  if(!self) { return; }
  if(self->Writer)
  {
    // Invoke the writer.
    switch(datamodetype)
    {
      case vtkXMLWriter::Ascii:
      case vtkXMLWriter::Binary:
      case vtkXMLWriter::Appended:
        self->Writer->SetDataMode(datamodetype);
        break;
      default:
        vtkGenericWarningMacro(
          "vtkXMLWriterC_SetDataModeType : unknown DataMode: " << datamodetype
          );
    }
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_SetDataModeType called before vtkXMLWriterC_SetDataObjectType."
      );
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_SetExtent(vtkXMLWriterC* self, int extent[6])
{
  if(!self) { return; }
  if(vtkImageData* imData =
     vtkImageData::SafeDownCast(self->DataObject))
  {
    imData->SetExtent(extent);
  }
  else if(vtkStructuredGrid* sGrid =
          vtkStructuredGrid::SafeDownCast(self->DataObject))
  {
    sGrid->SetExtent(extent);
  }
  else if(vtkRectilinearGrid* rGrid =
          vtkRectilinearGrid::SafeDownCast(self->DataObject))
  {
    rGrid->SetExtent(extent);
  }
  else if(self->DataObject)
  {
    vtkGenericWarningMacro("vtkXMLWriterC_SetExtent called for "
                           << self->DataObject->GetClassName()
                           << " data object.");
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_SetExtent called before vtkXMLWriterC_SetDataObjectType."
      );
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_SetPoints(vtkXMLWriterC* self, int dataType,
                             void* data, vtkIdType numPoints)
{
  if(!self) { return; }
  if(vtkPointSet* dataObject = vtkPointSet::SafeDownCast(self->DataObject))
  {
    // Create the vtkDataArray that will reference the points.
    if(vtkSmartPointer<vtkDataArray> array =
       vtkXMLWriterC_NewDataArray("SetPoints", 0,
                                  dataType, data, numPoints, 3))
    {
      // Store the point array in the data object's points.
      if(vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New())
      {
        points->SetNumberOfPoints(numPoints);
        points->SetData(array);
        dataObject->SetPoints(points);
      }
      else
      {
        vtkGenericWarningMacro(
          "vtkXMLWriterC_SetPoints failed to create a vtkPoints object."
          );
      }
    }
  }
  else if(self->DataObject)
  {
    vtkGenericWarningMacro("vtkXMLWriterC_SetPoints called for "
                           << self->DataObject->GetClassName()
                           << " data object.");
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_SetPoints called before vtkXMLWriterC_SetDataObjectType."
      );
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_SetOrigin(vtkXMLWriterC* self, double origin[3])
{
  if(!self) { return; }
  if(vtkImageData* dataObject = vtkImageData::SafeDownCast(self->DataObject))
  {
    dataObject->SetOrigin(origin);
  }
  else if(self->DataObject)
  {
    vtkGenericWarningMacro("vtkXMLWriterC_SetOrigin called for "
                           << self->DataObject->GetClassName()
                           << " data object.");
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_SetOrigin called before vtkXMLWriterC_SetDataObjectType."
      );
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_SetSpacing(vtkXMLWriterC* self, double spacing[3])
{
  if(!self) { return; }
  if(vtkImageData* dataObject = vtkImageData::SafeDownCast(self->DataObject))
  {
    dataObject->SetSpacing(spacing);
  }
  else if(self->DataObject)
  {
    vtkGenericWarningMacro("vtkXMLWriterC_SetSpacing called for "
                           << self->DataObject->GetClassName()
                           << " data object.");
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_SetSpacing called before vtkXMLWriterC_SetDataObjectType."
      );
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_SetCoordinates(vtkXMLWriterC* self, int axis,
                                  int dataType, void* data,
                                  vtkIdType numCoordinates)
{
  if(!self) { return; }
  if(vtkRectilinearGrid* dataObject =
     vtkRectilinearGrid::SafeDownCast(self->DataObject))
  {
    // Check the axis number.
    if(axis < 0 || axis > 2)
    {
      vtkGenericWarningMacro(
        "vtkXMLWriterC_SetCoordinates called with invalid axis "
        << axis << ".  Use 0 for X, 1 for Y, and 2 for Z."
        );
    }

    // Create the vtkDataArray that will reference the coordinates.
    if(vtkSmartPointer<vtkDataArray> array =
       vtkXMLWriterC_NewDataArray("SetCoordinates", 0, dataType, data,
                                  numCoordinates, 1))
    {
      switch(axis)
      {
        case 0:
          dataObject->SetXCoordinates(array);
          break;
        case 1:
          dataObject->SetYCoordinates(array);
          break;
        case 2:
          dataObject->SetZCoordinates(array);
          break;
      }
    }
  }
  else if(self->DataObject)
  {
    vtkGenericWarningMacro("vtkXMLWriterC_SetCoordinates called for "
                           << self->DataObject->GetClassName()
                           << " data object.");
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_SetCoordinates called before vtkXMLWriterC_SetDataObjectType."
      );
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_SetCellsWithType(vtkXMLWriterC* self,
                                    int cellType, vtkIdType ncells,
                                    vtkIdType* cells, vtkIdType cellsSize)
{
  if(!self) { return; }
  if(vtkPolyData* dataObject =
     vtkPolyData::SafeDownCast(self->DataObject))
  {
    // Create a cell array to reference the cells.
    if(vtkSmartPointer<vtkCellArray> cellArray =
       vtkXMLWriterC_NewCellArray("SetCellsWithType", ncells, cells,
                                  cellsSize))
    {
      // Store the cell array in the data object.
      if(cellType == VTK_VERTEX || cellType == VTK_POLY_VERTEX)
      {
        dataObject->SetVerts(cellArray);
      }
      else if(cellType == VTK_LINE || cellType == VTK_POLY_LINE)
      {
        dataObject->SetLines(cellArray);
      }
      else if(cellType == VTK_TRIANGLE || cellType == VTK_TRIANGLE_STRIP)
      {
        dataObject->SetStrips(cellArray);
      }
      else // if(cellType == VTK_POLYGON || cellType == VTK_QUAD)
      {
        dataObject->SetPolys(cellArray);
      }
    }
  }
  else if(vtkUnstructuredGrid* uGrid =
          vtkUnstructuredGrid::SafeDownCast(self->DataObject))
  {
    // Create a cell array to reference the cells.
    if(vtkSmartPointer<vtkCellArray> cellArray =
       vtkXMLWriterC_NewCellArray("SetCellsWithType", ncells, cells,
                                  cellsSize))
    {
      // Store the cell array in the data object.
      uGrid->SetCells(cellType, cellArray);
    }
  }
  else if(self->DataObject)
  {
    vtkGenericWarningMacro("vtkXMLWriterC_SetCellsWithType called for "
                           << self->DataObject->GetClassName()
                           << " data object.");
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_SetCellsWithType called before vtkXMLWriterC_SetDataObjectType."
      );
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_SetCellsWithTypes(vtkXMLWriterC* self,
                                    int* cellTypes, vtkIdType ncells,
                                    vtkIdType* cells, vtkIdType cellsSize)
{
  if(!self) { return; }
  if(vtkUnstructuredGrid* dataObject =
     vtkUnstructuredGrid::SafeDownCast(self->DataObject))
  {
    // Create a cell array to reference the cells.
    if(vtkSmartPointer<vtkCellArray> cellArray =
       vtkXMLWriterC_NewCellArray("SetCellsWithType", ncells, cells,
                                  cellsSize))
    {
      // Store the cell array in the data object.
      dataObject->SetCells(cellTypes, cellArray);
    }
  }
  else if(self->DataObject)
  {
    vtkGenericWarningMacro("vtkXMLWriterC_SetCellsWithTypes called for "
                           << self->DataObject->GetClassName()
                           << " data object.");
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_SetCellsWithTypes called before vtkXMLWriterC_SetDataObjectType."
      );
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_SetPointData(vtkXMLWriterC* self, const char* name,
                                int dataType, void* data,
                                vtkIdType numTuples, int numComponents,
                                const char* role)
{
  vtkXMLWriterC_SetDataInternal(self, name, dataType, data, numTuples,
                                numComponents, role, "SetPointData", 1);
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_SetCellData(vtkXMLWriterC* self, const char* name,
                               int dataType, void* data,
                               vtkIdType numTuples, int numComponents,
                               const char* role)
{
  vtkXMLWriterC_SetDataInternal(self, name, dataType, data, numTuples,
                                numComponents, role, "SetCellData", 0);
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_SetFileName(vtkXMLWriterC* self, const char* fileName)
{
  if(!self) { return; }
  if(self->Writer)
  {
    // Store the file name.
    self->Writer->SetFileName(fileName);
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_SetFileName called before vtkXMLWriterC_SetDataObjectType."
      );
  }
}

//----------------------------------------------------------------------------
int vtkXMLWriterC_Write(vtkXMLWriterC* self)
{
  if(!self) { return 0; }
  if(self->Writer)
  {
    // Invoke the writer.
    return self->Writer->Write();
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_Write called before vtkXMLWriterC_SetDataObjectType."
      );
    return 0;
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_SetNumberOfTimeSteps(vtkXMLWriterC* self, int numTimeSteps)
{
  if(!self) { return; }
  if(self->Writer)
  {
    // Set the number of time steps on the writer.
    self->Writer->SetNumberOfTimeSteps(numTimeSteps);
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_SetNumberOfTimeSteps called before vtkXMLWriterC_SetDataObjectType."
      );
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_Start(vtkXMLWriterC* self)
{
  if(!self) { return; }
  if(self->Writing)
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_Start called multiple times without vtkXMLWriterC_Stop."
      );
  }
  else if(self->Writer)
  {
    // Check the conditions.
    if(self->Writer->GetNumberOfTimeSteps() == 0)
    {
      vtkGenericWarningMacro(
        "vtkXMLWriterC_Start called with no time steps."
        );
    }
    else if(self->Writer->GetFileName() == 0)
    {
      vtkGenericWarningMacro(
        "vtkXMLWriterC_Start called before vtkXMLWriterC_SetFileName."
        );
    }
    else
    {
      // Tell the writer to start writing.
      self->Writer->Start();
      self->Writing = 1;
    }
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_Start called before vtkXMLWriterC_SetDataObjectType."
      );
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_WriteNextTimeStep(vtkXMLWriterC* self, double timeValue)
{
  if(!self) { return; }
  if(!self->Writing)
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_WriteNextTimeStep called before vtkXMLWriterC_Start."
      );
  }
  else if(self->Writer)
  {
    // Tell the writer to write this time step.
    self->Writer->WriteNextTime(timeValue);
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_Stop called before vtkXMLWriterC_SetDataObjectType."
      );
  }
}

//----------------------------------------------------------------------------
void vtkXMLWriterC_Stop(vtkXMLWriterC* self)
{
  if(!self) { return; }
  if(!self->Writing)
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_Stop called before vtkXMLWriterC_Start."
      );
  }
  else if(self->Writer)
  {
    // Tell the writer to stop writing.
    self->Writer->Stop();
    self->Writing = 0;
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_Stop called before vtkXMLWriterC_SetDataObjectType."
      );
  }
}

} /* extern "C" */

//----------------------------------------------------------------------------
static
vtkSmartPointer<vtkDataArray>
vtkXMLWriterC_NewDataArray(const char* method, const char* name, int dataType,
                           void* data, vtkIdType numTuples, int numComponents)
{
  // Create the vtkDataArray that will reference the data.
  vtkSmartPointer<vtkDataArray> array =
    vtkDataArray::CreateDataArray(dataType);
  if(array)
  {
    array->Delete();
  }
  if(!array || array->GetDataType() != dataType)
  {
    vtkGenericWarningMacro("vtkXMLWriterC_" << method
                           << " could not allocate array of type "
                           << dataType << ".");
    return 0;
  }

  // Set the number of components.
  array->SetNumberOfComponents(numComponents);

  // Set the name if one was given.
  array->SetName(name);

  // Point the array at the given data.  It is not copied.
  array->SetVoidArray(data, numTuples*numComponents, 1);

  // Return the array.
  return array;
}

//----------------------------------------------------------------------------
static
vtkSmartPointer<vtkCellArray>
vtkXMLWriterC_NewCellArray(const char* method, vtkIdType ncells,
                           vtkIdType* cells, vtkIdType cellsSize)
{
  // Create a vtkIdTypeArray to reference the cells.
  vtkSmartPointer<vtkIdTypeArray> array =
    vtkSmartPointer<vtkIdTypeArray>::New();
  if(!array)
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_" << method << " failed to allocate a vtkIdTypeArray."
      );
    return 0;
  }
  array->SetArray(cells, ncells*cellsSize, 1);

  // Create the cell array.
  vtkSmartPointer<vtkCellArray> cellArray =
    vtkSmartPointer<vtkCellArray>::New();
  if(!cellArray)
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_" << method << " failed to allocate a vtkCellArray."
      );
    return 0;
  }
  cellArray->SetCells(ncells, array);
  return cellArray;
}

//----------------------------------------------------------------------------
static
void vtkXMLWriterC_SetDataInternal(vtkXMLWriterC* self, const char* name,
                                   int dataType, void* data,
                                   vtkIdType numTuples, int numComponents,
                                   const char* role, const char* method,
                                   int isPoints)
{
  if(!self) { return; }
  if(vtkDataSet* dataObject = vtkDataSet::SafeDownCast(self->DataObject))
  {
    if(vtkSmartPointer<vtkDataArray> array =
       vtkXMLWriterC_NewDataArray(method, name, dataType, data, numTuples,
                                  numComponents))
    {
      // Store either in point data or cell data.
      vtkDataSetAttributes* dsa;
      if (isPoints)
      {
        dsa = dataObject->GetPointData();
      }
      else
      {
        dsa = dataObject->GetCellData();
      }

      // Store the data array with the requested role.
      if(role && strcmp(role, "SCALARS") == 0)
      {
        dsa->SetScalars(array);
      }
      else if(role && strcmp(role, "VECTORS") == 0)
      {
        dsa->SetVectors(array);
      }
      else if(role && strcmp(role, "NORMALS") == 0)
      {
        dsa->SetNormals(array);
      }
      else if(role && strcmp(role, "TENSORS") == 0)
      {
        dsa->SetTensors(array);
      }
      else if(role && strcmp(role, "TCOORDS") == 0)
      {
        dsa->SetTCoords(array);
      }
      else
      {
        dsa->AddArray(array);
      }
    }
  }
  else if(self->DataObject)
  {
    vtkGenericWarningMacro("vtkXMLWriterC_" << method << " called for "
                           << self->DataObject->GetClassName()
                           << " data object.");
  }
  else
  {
    vtkGenericWarningMacro(
      "vtkXMLWriterC_" << method
      << " called before vtkXMLWriterC_SetDataObjectType."
      );
  }
}
