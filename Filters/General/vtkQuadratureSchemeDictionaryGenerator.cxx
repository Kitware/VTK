/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadratureSchemeDictionaryGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkQuadratureSchemeDictionaryGenerator.h"
#include "vtkQuadratureSchemeDefinition.h"

#include "vtkUnstructuredGrid.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkType.h"
#include "vtkDoubleArray.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationQuadratureSchemeDefinitionVectorKey.h"
#include "vtkObjectFactory.h"

#include <sstream>
#include <string>
#include "vtkSmartPointer.h"
using std::ostringstream;
using std::string;

// Here are some default shape functions weights which
// we will use to create dictionaries in a gvien data set.
// Unused weights are commented out to avoid compiler warnings.
namespace
{
// double W_T_11_A[]={
//     3.33333333333334e-01, 3.33333333333333e-01, 3.33333333333333e-01};

double W_T_32_A[] = { 1.66666666666660e-01, 6.66666666666670e-01,
    1.66666666666670e-01, 6.66666666666660e-01, 1.66666666666670e-01,
    1.66666666666670e-01, 1.66666666666660e-01, 1.66666666666670e-01,
    6.66666666666670e-01 };

// double W_T_32_B[]={
//     5.00000000000000e-01, 5.00000000000000e-01, 0.00000000000000e+00,
//     5.00000000000000e-01, 0.00000000000000e+00, 5.00000000000000e-01,
//     0.00000000000000e+00, 5.00000000000000e-01, 5.00000000000000e-01};

double W_QT_43_A[] = { -1.11111111111111e-01, -1.11111111111111e-01,
    -1.11111111111111e-01, 4.44444444444445e-01, 4.44444444444444e-01,
    4.44444444444445e-01, -1.20000000000000e-01, 1.20000000000000e-01,
    -1.20000000000000e-01, 4.80000000000000e-01, 4.80000000000000e-01,
    1.60000000000000e-01, 1.20000000000000e-01, -1.20000000000000e-01,
    -1.20000000000000e-01, 4.80000000000000e-01, 1.60000000000000e-01,
    4.80000000000000e-01, -1.20000000000000e-01, -1.20000000000000e-01,
    1.20000000000000e-01, 1.60000000000000e-01, 4.80000000000000e-01,
    4.80000000000000e-01 };

double W_Q_42_A[] = { 6.22008467928145e-01, 1.66666666666667e-01,
    4.46581987385206e-02, 1.66666666666667e-01, 1.66666666666667e-01,
    4.46581987385206e-02, 1.66666666666667e-01, 6.22008467928145e-01,
    1.66666666666667e-01, 6.22008467928145e-01, 1.66666666666667e-01,
    4.46581987385206e-02, 4.46581987385206e-02, 1.66666666666667e-01,
    6.22008467928145e-01, 1.66666666666667e-01 };

double W_QQ_93_A[] = { 4.32379000772438e-01, -1.00000000000001e-01,
    -3.23790007724459e-02, -1.00000000000001e-01, 3.54919333848301e-01,
    4.50806661517046e-02, 4.50806661517046e-02, 3.54919333848301e-01,
    -1.00000000000001e-01, -1.00000000000001e-01, -1.00000000000001e-01,
    -1.00000000000001e-01, 2.00000000000003e-01, 1.12701665379260e-01,
    2.00000000000003e-01, 8.87298334620740e-01, -1.00000000000001e-01,
    -3.23790007724459e-02, -1.00000000000001e-01, 4.32379000772438e-01,
    4.50806661517046e-02, 4.50806661517046e-02, 3.54919333848301e-01,
    3.54919333848301e-01, -1.00000000000001e-01, -1.00000000000001e-01,
    -1.00000000000001e-01, -1.00000000000001e-01, 8.87298334620740e-01,
    2.00000000000003e-01, 1.12701665379260e-01, 2.00000000000003e-01,
    -2.50000000000000e-01, -2.50000000000000e-01, -2.50000000000000e-01,
    -2.50000000000000e-01, 5.00000000000000e-01, 5.00000000000000e-01,
    5.00000000000000e-01, 5.00000000000000e-01, -1.00000000000001e-01,
    -1.00000000000001e-01, -1.00000000000001e-01, -1.00000000000001e-01,
    1.12701665379260e-01, 2.00000000000003e-01, 8.87298334620740e-01,
    2.00000000000003e-01, -1.00000000000001e-01, 4.32379000772438e-01,
    -1.00000000000001e-01, -3.23790007724459e-02, 3.54919333848301e-01,
    3.54919333848301e-01, 4.50806661517046e-02, 4.50806661517046e-02,
    -1.00000000000001e-01, -1.00000000000001e-01, -1.00000000000001e-01,
    -1.00000000000001e-01, 2.00000000000003e-01, 8.87298334620740e-01,
    2.00000000000003e-01, 1.12701665379260e-01, -3.23790007724459e-02,
    -1.00000000000001e-01, 4.32379000772438e-01, -1.00000000000001e-01,
    4.50806661517046e-02, 3.54919333848301e-01, 3.54919333848301e-01,
    4.50806661517046e-02 };

// double W_E_41_A[]={
//      2.50000000000000e-01, 2.50000000000000e-01, 2.50000000000000e-01, 2.50000000000000e-01};

double W_E_42_A[] = { 6.25000000000000e-01, 1.25000000000000e-01,
    1.25000000000000e-01, 1.25000000000000e-01, 1.25000000000000e-01,
    5.62500000000000e-01, 1.87500000000000e-01, 1.25000000000000e-01,
    1.25000000000000e-01, 1.87500000000000e-01, 5.62500000000000e-01,
    1.25000000000000e-01, 1.25000000000000e-01, 6.25000000000000e-02,
    6.25000000000000e-02, 7.50000000000000e-01 };

// double W_QE_41_A[]={
//     -1.25000000000000e-01, -1.25000000000000e-01, -1.25000000000000e-01, -1.25000000000000e-01, 2.50000000000000e-01, 2.50000000000000e-01, 2.50000000000000e-01, 2.50000000000000e-01, 2.50000000000000e-01, 2.50000000000000e-01};

double W_QE_42_A[] = { 1.56250000000000e-01, -9.37500000000000e-02,
    -9.37500000000000e-02, -9.37500000000000e-02, 3.12500000000000e-01,
    6.25000000000000e-02, 3.12500000000000e-01, 3.12500000000000e-01,
    6.25000000000000e-02, 6.25000000000000e-02, -9.37500000000000e-02,
    7.03125000000000e-02, -1.17187500000000e-01, -9.37500000000000e-02,
    2.81250000000000e-01, 4.21875000000000e-01, 9.37500000000000e-02,
    6.25000000000000e-02, 2.81250000000000e-01, 9.37500000000000e-02,
    -9.37500000000000e-02, -1.17187500000000e-01, 7.03125000000000e-02,
    -9.37500000000000e-02, 9.37500000000000e-02, 4.21875000000000e-01,
    2.81250000000000e-01, 6.25000000000000e-02, 9.37500000000000e-02,
    2.81250000000000e-01, -9.37500000000000e-02, -5.46875000000000e-02,
    -5.46875000000000e-02, 3.75000000000000e-01, 3.12500000000000e-02,
    1.56250000000000e-02, 3.12500000000000e-02, 3.75000000000000e-01,
    1.87500000000000e-01, 1.87500000000000e-01 };

}
;

vtkStandardNewMacro(vtkQuadratureSchemeDictionaryGenerator);

//-----------------------------------------------------------------------------
vtkQuadratureSchemeDictionaryGenerator::vtkQuadratureSchemeDictionaryGenerator()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkQuadratureSchemeDictionaryGenerator::~vtkQuadratureSchemeDictionaryGenerator()
{
}

//-----------------------------------------------------------------------------
int vtkQuadratureSchemeDictionaryGenerator::FillInputPortInformation(int port,
    vtkInformation *info)
{
  switch (port)
  {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
      break;
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkQuadratureSchemeDictionaryGenerator::FillOutputPortInformation(int port,
    vtkInformation *info)
{
  switch (port)
  {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
      break;
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkQuadratureSchemeDictionaryGenerator::RequestData(vtkInformation *,
    vtkInformationVector **input, vtkInformationVector *output)
{
  vtkDataObject *tmpDataObj;
  // Get the inputs
  tmpDataObj = input[0]->GetInformationObject(0)->Get(
      vtkDataObject::DATA_OBJECT());
  vtkUnstructuredGrid *usgIn = vtkUnstructuredGrid::SafeDownCast(tmpDataObj);
  // Get the outputs
  tmpDataObj = output->GetInformationObject(0)->Get(
      vtkDataObject::DATA_OBJECT());
  vtkUnstructuredGrid *usgOut = vtkUnstructuredGrid::SafeDownCast(tmpDataObj);

  // Quick sanity check.
  if (usgIn == NULL || usgOut == NULL || usgIn->GetNumberOfPoints() == 0
      || usgIn->GetPointData()->GetNumberOfArrays() == 0)
  {
    vtkWarningMacro("Filter data has not been configured correctly. Aborting.");
    return 1;
  }

  // Copy the unstructured grid on the input
  usgOut->ShallowCopy(usgIn);

  // Interpolate the data arrays, but no points. Results
  // are stored in field data arrays.
  this->Generate(usgOut);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkQuadratureSchemeDictionaryGenerator::Generate(
    vtkUnstructuredGrid *usgOut)
{
  // Get the dictionary key.
  vtkInformationQuadratureSchemeDefinitionVectorKey *key =
      vtkQuadratureSchemeDefinition::DICTIONARY();

  // Get the cell types used by the data set.
  vtkCellTypes *cellTypes = vtkCellTypes::New();
  usgOut->GetCellTypes(cellTypes);
  // add a definition to the dictionary for each cell type.
  int nCellTypes = cellTypes->GetNumberOfTypes();

  // create the offset array and store the dictionnary within
  vtkIdTypeArray* offsets = vtkIdTypeArray::New();
  string basename = "QuadratureOffset";
  string finalname = basename;
  vtkDataArray* data = usgOut->GetCellData()->GetArray(basename.c_str());
  ostringstream interpolatedName;
  int i = 0;
  while (data != NULL)
  {
    interpolatedName << basename << i;
    data = usgOut->GetCellData()->GetArray(interpolatedName.str().c_str());
    finalname = interpolatedName.str();
    i++;
  }

  offsets->SetName(finalname.c_str());
  usgOut->GetCellData()->AddArray(offsets);
  vtkInformation *info = offsets->GetInformation();

  for (int typeId = 0; typeId < nCellTypes; ++typeId)
  {
    int cellType = cellTypes->GetCellType(typeId);
    // Initiaze a definition for this particular cell type.
    vtkSmartPointer<vtkQuadratureSchemeDefinition> def = vtkSmartPointer<vtkQuadratureSchemeDefinition>::New();
    switch (cellType)
    {
      case VTK_TRIANGLE:
        def->Initialize(VTK_TRIANGLE, 3, 3, W_T_32_A);
        break;
      case VTK_QUADRATIC_TRIANGLE:
        def->Initialize(VTK_QUADRATIC_TRIANGLE, 6, 4, W_QT_43_A);
        break;
      case VTK_QUAD:
        def->Initialize(VTK_QUAD, 4, 4, W_Q_42_A);
        break;
      case VTK_QUADRATIC_QUAD:
        def->Initialize(VTK_QUADRATIC_QUAD, 8, 9, W_QQ_93_A);
        break;
      case VTK_TETRA:
        def->Initialize(VTK_TETRA, 4, 4, W_E_42_A);
        break;
      case VTK_QUADRATIC_TETRA:
        def->Initialize(VTK_QUADRATIC_TETRA, 10, 4, W_QE_42_A);
        break;
      default:
        cerr << "Error: Cell type " << cellType << " found "
            << "with no definition provided. Add a definition " << " in "
            << __FILE__ << ". Aborting." << endl;
        return 0;
    }

    // The definition must apear in the dictionary associated
    // with the offset array
    key->Set(info, def, cellType);
  }

  int dictSize = key->Size(info);
  vtkQuadratureSchemeDefinition **dict =
      new vtkQuadratureSchemeDefinition *[dictSize];
  key->GetRange(info, dict, 0, 0, dictSize);

  offsets->SetNumberOfTuples(usgOut->GetNumberOfCells());
  vtkIdType offset = 0;
  for (vtkIdType cellid = 0; cellid < usgOut->GetNumberOfCells(); cellid++)
  {
    offsets->SetValue(cellid, offset);
    vtkCell* cell = usgOut->GetCell(cellid);
    int cellType = cell->GetCellType();
    vtkQuadratureSchemeDefinition * celldef = dict[cellType];
    offset += celldef->GetNumberOfQuadraturePoints();
  }
  offsets->Delete();
  cellTypes->Delete();
  delete[] dict;
  return 1;
}

//-----------------------------------------------------------------------------
void vtkQuadratureSchemeDictionaryGenerator::PrintSelf(ostream& os,
    vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "No state." << endl;
}
