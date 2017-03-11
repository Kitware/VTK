/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraturePointsGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkQuadraturePointsGenerator.h"

#include "vtkUnstructuredGrid.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkType.h"
#include "vtkDoubleArray.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationQuadratureSchemeDefinitionVectorKey.h"
#include "vtkObjectFactory.h"
#include "vtkDataSetAttributes.h"

#include "vtkQuadratureSchemeDefinition.h"
#include "vtkQuadraturePointsUtilities.hxx"

#include <sstream>
using std::ostringstream;

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkQuadraturePointsGenerator);

//-----------------------------------------------------------------------------
vtkQuadraturePointsGenerator::vtkQuadraturePointsGenerator()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkQuadraturePointsGenerator::~vtkQuadraturePointsGenerator()
{}

//-----------------------------------------------------------------------------
int vtkQuadraturePointsGenerator::FillInputPortInformation(
      int vtkNotUsed(port),
      vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkQuadraturePointsGenerator::RequestData(
      vtkInformation *,
      vtkInformationVector **input,
      vtkInformationVector *output)
{
  vtkDataObject *tmpDataObj;
  // Get the input.
  tmpDataObj
     = input[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  vtkUnstructuredGrid *usgIn = vtkUnstructuredGrid::SafeDownCast(tmpDataObj);
  // Get the output.
  tmpDataObj
     = output->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  vtkPolyData *pdOut = vtkPolyData::SafeDownCast(tmpDataObj);

  // Quick sanity check.
  if (usgIn == NULL || pdOut == NULL || usgIn->GetNumberOfCells() == 0
      || usgIn->GetNumberOfPoints() == 0 || usgIn->GetCellData() == NULL
      || usgIn->GetCellData()->GetNumberOfArrays() == 0)
  {
    vtkErrorMacro("Filter data has not been configured correctly. Aborting.");
    return 1;
  }

  // Generate points for the selected data array.
  // user specified the offsets array.
  this->Generate(usgIn, this->GetInputArrayToProcess(0, input), pdOut);

  return 1;
}

// ----------------------------------------------------------------------------
int vtkQuadraturePointsGenerator::GenerateField(
        vtkUnstructuredGrid *usgIn,
        vtkDataArray* data,
        vtkDataArray* offsets,
        vtkPolyData* pdOut)
{
  vtkInformation *info = offsets->GetInformation();
  vtkInformationQuadratureSchemeDefinitionVectorKey *key
    = vtkQuadratureSchemeDefinition::DICTIONARY();
  if (!key->Has(info))
  {
    vtkErrorMacro(
      << "Dictionary is not present in array "
      << offsets->GetName() << " " << offsets
     << " Aborting.");
    return 0;
  }

  int dictSize = key->Size(info);
  vtkQuadratureSchemeDefinition **dict
    = new vtkQuadratureSchemeDefinition *[dictSize];
  key->GetRange(info, dict, 0, 0, dictSize);

  vtkIdType nVerts = pdOut->GetNumberOfPoints();

  vtkIdType cellId;
  vtkIdType ncell = usgIn->GetNumberOfCells();
  int cellType;
  // first loop through all cells to check if a shallow copy is possible
  bool shallowok = true;
  vtkIdType previous = -1;

  int offsetType = offsets->GetDataType();
  void *pOffsets = offsets->GetVoidPointer(0);
  switch(offsetType)
  {
    vtkTemplateMacro(
      for (cellId = 0; cellId < ncell; cellId++)
      {
        vtkIdType offset = static_cast<vtkIdType>(((VTK_TT*)pOffsets)[cellId]);
        if (offset != previous + 1)
        {
          shallowok = false;
          break;
        }
        cellType = usgIn->GetCellType(cellId);

        if (dict[cellType] == NULL)
        {
          previous = offset;
        }
        else
        {
          previous = offset + dict[cellType]->GetNumberOfQuadraturePoints();
        }
      }

      if ( (previous+1) != nVerts )
      {
        shallowok = false;
      }

      if (shallowok)
      {
        // ok, all the original cells are here, we can shallow copy the array
        // from input to output
        pdOut->GetPointData()->AddArray(data);
      }
      else
      {
        // in this case, we have to duplicate the valid tuples
        vtkDataArray *V_out = data->NewInstance();
        V_out->SetName(data->GetName());
        V_out->SetNumberOfComponents(data->GetNumberOfComponents());
        V_out->CopyComponentNames( data );
        for (cellId = 0; cellId < ncell; cellId++)
        {
          vtkIdType offset = static_cast<vtkIdType>(((VTK_TT*)pOffsets)[cellId]);
          cellType = usgIn->GetCellType(cellId);

          // a simple check to see if a scheme really exists for this cell type.
          // should not happen if the cell type has not been modified.
          if (dict[cellType] == NULL)
          {
            continue;
          }

          int np = dict[cellType]->GetNumberOfQuadraturePoints();
          for (int id = 0; id < np; id++)
          {
            V_out->InsertNextTuple(offset + id, data);
          }
        }
        V_out->Squeeze();
        pdOut->GetPointData()->AddArray(V_out);
        V_out->Delete();
      }
    );
  }
  delete[] dict;
  return 1;
}

//-----------------------------------------------------------------------------
int vtkQuadraturePointsGenerator::Generate(
        vtkUnstructuredGrid *usgIn,
        vtkDataArray* offsets,
        vtkPolyData *pdOut)
{
  if (usgIn == NULL || offsets == NULL || pdOut == NULL)
  {
    vtkErrorMacro("configuration error");
    return 0;
  }

  // Strategy:
  // create the points then move the FieldData to PointData

  const char* offsetName = offsets->GetName();
  if (offsetName == NULL)
  {
    vtkErrorMacro("offset array has no name, Skipping");
    return 1;
  }

  // get the dictionary
  vtkInformation *info = offsets->GetInformation();
  vtkInformationQuadratureSchemeDefinitionVectorKey *key =
      vtkQuadratureSchemeDefinition::DICTIONARY();
  if (!key->Has(info))
  {
    vtkErrorMacro(
      << "Dictionary is not present in array "
      << offsets->GetName()
      << " Aborting.");
    return 0;
  }
  int dictSize = key->Size(info);
  vtkQuadratureSchemeDefinition **dict =
      new vtkQuadratureSchemeDefinition *[dictSize];
  key->GetRange(info, dict, 0, 0, dictSize);

  // Grab the point set.
  vtkDataArray *X = usgIn->GetPoints()->GetData();
  int X_type = X->GetDataType();
  void *pX = X->GetVoidPointer(0);

  // Create the result array.
  vtkDoubleArray *qPts = vtkDoubleArray::New();
  vtkIdType nCells = usgIn->GetNumberOfCells();
  qPts->Allocate(3* nCells ); // Expect at least one point per cell
  qPts->SetNumberOfComponents(3);

  // For all cells interpolate.
  switch (X_type)
  {
    vtkTemplateMacro(
      if (!Interpolate(
              usgIn,
              nCells,
              (VTK_TT*)pX,
              3,
              dict,
              qPts,
              (int*)NULL))
      {
        vtkWarningMacro("Failed to interpolate cell vertices "
            "to quadrature points. Aborting.");
      }
      );
  }
  delete[] dict;

  // Add the interpolated quadrature points to the output
  vtkIdType nVerts = qPts->GetNumberOfTuples();
  vtkPoints *p = vtkPoints::New();
  p->SetDataTypeToDouble();
  p->SetData(qPts);
  qPts->Delete();
  pdOut->SetPoints(p);
  p->Delete();
  // Generate vertices at the quadrature points
  vtkIdTypeArray *va = vtkIdTypeArray::New();
  va->SetNumberOfTuples(2* nVerts );
  vtkIdType *verts = va->GetPointer(0);
  for (int i = 0; i < nVerts; ++i)
  {
    verts[0] = 1;
    verts[1] = i;
    verts += 2;
  }
  vtkCellArray *cells = vtkCellArray::New();
  cells->SetCells(static_cast<vtkIdType> (nVerts), va);
  pdOut->SetVerts(cells);
  cells->Delete();
  va->Delete();

  // then loop over all fields to map the field array to the points
  int nArrays = usgIn->GetFieldData()->GetNumberOfArrays();
  for (int i = 0; i<nArrays; ++i)
  {
    vtkDataArray* array = usgIn->GetFieldData()->GetArray(i);
    if (array == NULL) continue;

    const char* arrayOffsetName = array->GetInformation()->Get(
        vtkQuadratureSchemeDefinition::QUADRATURE_OFFSET_ARRAY_NAME());
    if (arrayOffsetName == NULL)
    {
      // not an error, since non-quadrature point field data may
      //  be present.
      vtkDebugMacro(
        << "array " << array->GetName()
        << " has no offset array name, Skipping");
      continue;
    }

    if (strcmp(offsetName, arrayOffsetName) != 0)
    {
      // not an error, this array does not belong with the current
      // quadrature scheme definition.
      vtkDebugMacro(
        << "array " << array->GetName()
        << " has another offset array : "
        << arrayOffsetName << ", Skipping");
      continue;
    }

    this->GenerateField(usgIn, array, offsets, pdOut);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkQuadraturePointsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
