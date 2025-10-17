#include "vtkCellData.h"
#include "vtkConduitSource.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDummyController.h"
#include "vtkLogger.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"

#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>

//----------------------------------------------------------------------------
void CreateCoords(
  unsigned int nptsX, unsigned int nptsY, unsigned int nptsZ, conduit_cpp::Node& res)
{
  conduit_cpp::Node coords = res["coordsets/coords"];
  conduit_cpp::Node coordVals = coords["values"];
  coords["type"] = "explicit";

  unsigned int npts = nptsX * nptsY;

  if (nptsZ > 1)
  {
    npts *= nptsZ;
  }

  std::vector<double> x;
  x.resize(npts);
  std::vector<double> y;
  y.resize(npts);
  std::vector<double> z;

  if (nptsZ > 1)
  {
    z.resize(npts);
  }

  double dx = 20.0 / double(nptsX - 1);
  double dy = 20.0 / double(nptsY - 1);

  double dz = 0.0;

  if (nptsZ > 1)
  {
    dz = 20.0 / double(nptsZ - 1);
  }

  unsigned int idx = 0;
  unsigned int outer = 1;
  if (nptsZ > 1)
  {
    outer = nptsZ;
  }

  for (unsigned int k = 0; k < outer; k++)
  {
    double cz = -10.0 + k * dz;

    for (unsigned int j = 0; j < nptsY; j++)
    {
      double cy = -10.0 + j * dy;

      for (unsigned int i = 0; i < nptsX; i++)
      {
        x[idx] = -10.0 + i * dx;
        y[idx] = cy;

        if (nptsZ > 1)
        {
          z[idx] = cz;
        }

        idx++;
      }
    }
  }

  coordVals["x"].set(x);
  coordVals["y"].set(y);
  if (nptsZ > 1)
  {
    coordVals["z"].set(z);
  }
}

//----------------------------------------------------------------------------
void CreateStructuredMesh(
  unsigned int nptsX, unsigned int nptsY, unsigned int nptsZ, conduit_cpp::Node& res)
{
  CreateCoords(nptsX, nptsY, nptsZ, res);

  res["topologies/mesh/type"] = "structured";
  res["topologies/mesh/coordset"] = "coords";
  res["topologies/mesh/elements/dims/i"] = nptsX - 1;
  res["topologies/mesh/elements/dims/j"] = nptsY - 1;
  if (nptsZ > 0)
  {
    res["topologies/mesh/elements/dims/k"] = nptsZ - 1;
  }
}

//----------------------------------------------------------------------------
void CreateFieldData(conduit_cpp::Node& fieldNode, unsigned int numberOfValues)
{
  fieldNode["association"] = "element";
  fieldNode["topology"] = "mesh";
  fieldNode["volume_dependent"] = "false";

  std::vector<double> values;
  values.resize(numberOfValues);
  for (unsigned int i = 0; i < numberOfValues; i++)
  {
    values[i] = i + 0.0;
  }
  fieldNode["values"].set(values);
}

//----------------------------------------------------------------------------
void CreateData(conduit_cpp::Node& meshNode)
{
  const unsigned int nptsX = 3;
  const unsigned int nptsY = 3;
  CreateStructuredMesh(nptsX, nptsY, 1, meshNode);

  unsigned int nElementX = nptsX - 1;
  unsigned int nElementY = nptsY - 1;
  unsigned int nElements = nElementX * nElementY;

  meshNode["topologies/mesh/type"] = "unstructured";
  meshNode["topologies/mesh/coordset"] = "coords";
  meshNode["topologies/mesh/elements/shape"] = "tri";

  std::vector<unsigned int> connectivity;
  connectivity.resize(nElements * 6);

  unsigned int idx = 0;
  for (unsigned int j = 0; j < nElementY; j++)
  {
    unsigned int yoff = j * (nElementX + 1);

    for (unsigned int i = 0; i < nElementX; i++)
    {
      // two tris per quad.
      connectivity[idx + 0] = yoff + i;
      connectivity[idx + 1] = yoff + i + (nElementX + 1);
      connectivity[idx + 2] = yoff + i + 1 + (nElementX + 1);

      connectivity[idx + 3] = yoff + i;
      connectivity[idx + 4] = yoff + i + 1;
      connectivity[idx + 5] = yoff + i + 1 + (nElementX + 1);

      idx += 6;
    }
  }

  meshNode["topologies/mesh/elements/connectivity"].set(connectivity);

  // Need also to define 'fields' for cell array
  unsigned int numberOfValues = nElements * 2;
  conduit_cpp::Node fieldsNode = meshNode["fields"];
  conduit_cpp::Node fieldNode0 = fieldsNode["field0"];
  CreateFieldData(fieldNode0, numberOfValues);
  conduit_cpp::Node fieldNode1 = fieldsNode["field1"];
  CreateFieldData(fieldNode1, numberOfValues);

  meshNode["state/metadata/vtk_fields/field0/attribute_type"] =
    vtkDataSetAttributes::GetAttributeTypeAsString(vtkDataSetAttributes::SCALARS);
  meshNode["state/metadata/vtk_fields/field1/attribute_type"] =
    vtkDataSetAttributes::GetAttributeTypeAsString(vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
int TestConduitSourceSameAttributeTypes(int argc, char** argv)
{
  vtkNew<vtkDummyController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  conduit_cpp::Node mesh;
  CreateData(mesh);
  // As we expect a warning here, we turn the logs off.
  vtkLogger::Verbosity currentVerbosity = vtkLogger::GetCurrentVerbosityCutoff();
  vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_OFF);
  vtkNew<vtkConduitSource> source;
  source->SetNode(conduit_cpp::c_node(&mesh));
  source->Update();

  // Turn back on logs so that the verification errors can be displayed.
  vtkLogger::SetStderrVerbosity(currentVerbosity);

  vtkPartitionedDataSet* pds = vtkPartitionedDataSet::SafeDownCast(source->GetOutput());
  vtkDataSet* outputData = pds->GetPartition(0);
  vtkCellData* cellData = outputData->GetCellData();

  bool testResult = true;
  if (!cellData)
  {
    vtkLogF(ERROR, "Could not find any cell data.");
    testResult = false;
  }

  int numberOfArrays = cellData->GetNumberOfArrays();
  if (numberOfArrays != 2)
  {
    vtkLogF(ERROR, "Unexpected number of arrays: expected 2 but got %i.", numberOfArrays);
    testResult = false;
  }

  vtkDataArray* scalars = cellData->GetScalars();
  if (!scalars)
  {
    vtkLogF(INFO, "No scalars array assigned in the data set.");
    testResult = false;
  }

  std::string scalarsFieldName = scalars->GetName();
  if (scalarsFieldName != "field0")
  {
    vtkLogF(
      INFO, "Unexpected scalar field name: expected field0 but got %s", scalarsFieldName.c_str());
    testResult = false;
  }

  controller->Finalize();

  return testResult ? EXIT_SUCCESS : EXIT_FAILURE;
}
