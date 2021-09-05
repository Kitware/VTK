/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenVDBWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVDBWriter.h"

#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"

#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <openvdb/openvdb.h>
#include <openvdb/points/PointConversion.h>

namespace
{
std::string GetVDBGridName(const char* arrayName, int component, int numberOfComponents)
{
  std::string vdbName = arrayName;
  if (numberOfComponents != 1 && numberOfComponents != 3)
  {
    vdbName = vdbName + "_" + std::to_string(component);
  }
  return vdbName;
}

std::string MakeValidAttributeName(std::string& name)
{
  if (name.empty())
  {
    std::string output = "notspecified";
    return output;
  }
  std::string output;
  for (auto it = name.begin(); it != name.end(); it++)
  {
    if (!(isalnum(*it) || (*it == '_') || (*it == '|') || (*it == ':')))
    {
      output.append("_");
    }
    else
    {
      output.append(1, *it);
    }
  }

  return output;
}

void WriteVDBGrids(std::vector<openvdb::GridBase::Ptr>& grids,
  vtkMultiProcessController* controller, const char* fileName, bool writeAllTimeSteps,
  vtkIdType numberOfTimeSteps, vtkIdType currentTimeIndex)
{
  std::ostringstream oss;
  oss << std::setw(5) << std::setfill('0') << currentTimeIndex;
  if (!controller || controller->GetNumberOfProcesses() == 1)
  {
    if (writeAllTimeSteps && numberOfTimeSteps > 1)
    {
      std::string path = vtksys::SystemTools::GetFilenamePath(fileName);
      std::string fileNameBase = vtksys::SystemTools::GetFilenameWithoutExtension(fileName);
      std::string ext = vtksys::SystemTools::GetFilenameExtension(fileName);
      std::string newFileName = path + "/" + fileNameBase + "_" + oss.str() + ext;
      openvdb::io::File(newFileName).write(grids);
    }
    else
    {
      openvdb::io::File(fileName).write(grids);
    }
  }
  else
  {
    std::string path = vtksys::SystemTools::GetFilenamePath(fileName);
    std::string fileNameBase = vtksys::SystemTools::GetFilenameWithoutExtension(fileName);
    std::string ext = vtksys::SystemTools::GetFilenameExtension(fileName);
    if (writeAllTimeSteps && numberOfTimeSteps > 1)
    {
      std::string newFileName = path + "/" + fileNameBase + "_" +
        std::to_string(controller->GetLocalProcessId()) + "_" + oss.str() + ext;
      openvdb::io::File(newFileName).write(grids);
    }
    else
    {
      std::string newFileName =
        path + "/" + fileNameBase + "_" + std::to_string(controller->GetLocalProcessId()) + ext;
      openvdb::io::File(newFileName).write(grids);
    }
  }
}

} // end anonymous namespace

class vtkOpenVDBWriterInternals
{
public:
  vtkOpenVDBWriterInternals(vtkOpenVDBWriter* writer)
  {
    this->Writer = writer;
    this->CurrentTimeIndex = 0;
    this->NumberOfTimeSteps = 1;
  }
  vtkOpenVDBWriter* Writer;
  vtkIdType CurrentTimeIndex;
  vtkIdType NumberOfTimeSteps;

  openvdb::points::PointDataGrid::Ptr ProcessPointSet(
    vtkPointSet* pointSet, const char* gridName, bool isAPolyData)
  {
    vtkIdType numPoints = pointSet->GetNumberOfPoints();

    // compute colors, if any
    vtkNew<vtkPointData> pointData;
    pointData->ShallowCopy(pointSet->GetPointData());
    vtkNew<vtkCellData> cellData;
    cellData->ShallowCopy(pointSet->GetCellData());
    if (this->Writer->LookupTable && this->Writer->EnableColoring)
    {
      int fieldAssociation = 0;
      vtkAbstractArray* scalars =
        this->Writer->GetInputAbstractArrayToProcess(0, pointSet, fieldAssociation);
      vtkDiscretizableColorTransferFunction* dctf =
        vtkDiscretizableColorTransferFunction::SafeDownCast(this->Writer->LookupTable);
      bool enableOpacityMapping = dctf->GetEnableOpacityMapping();
      dctf->SetEnableOpacityMapping(this->Writer->EnableAlpha);
      vtkUnsignedCharArray* rgba =
        this->Writer->LookupTable->MapScalars(scalars, VTK_COLOR_MODE_MAP_SCALARS, -1);
      if (rgba && rgba->GetNumberOfTuples() == numPoints)
      {
        this->Writer->SetRGBA(numPoints, rgba, pointData);
      }
      fieldAssociation = 1;
      scalars = this->Writer->GetInputAbstractArrayToProcess(0, pointSet, fieldAssociation);
      dctf = vtkDiscretizableColorTransferFunction::SafeDownCast(this->Writer->LookupTable);
      rgba = this->Writer->LookupTable->MapScalars(scalars, VTK_COLOR_MODE_MAP_SCALARS, -1);
      vtkIdType numCells = pointSet->GetNumberOfCells();
      if (rgba && rgba->GetNumberOfTuples() == numCells)
      {
        this->Writer->SetRGBA(numCells, rgba, cellData);
      }
      dctf->SetEnableOpacityMapping(enableOpacityMapping);
    }

    std::vector<openvdb::Vec3R> positions;
    double coord[3];
    for (vtkIdType i = 0; i < pointSet->GetNumberOfPoints(); i++)
    {
      pointSet->GetPoint(i, coord);
      positions.push_back(openvdb::Vec3R(
        static_cast<float>(coord[0]), static_cast<float>(coord[1]), static_cast<float>(coord[2])));
    }

    // The VDB Point-Partioner is used when bucketing points and requires a
    // specific interface. For convenience, we use the PointAttributeVector
    // wrapper around an stl vector wrapper here, however it is also possible to
    // write one for a custom data structure in order to match the interface
    // required.
    openvdb::points::PointAttributeVector<openvdb::Vec3R> positionsWrapper(positions);
    // This method computes a voxel-size to match the number of
    // points / voxel requested. Although it won't be exact, it typically offers
    // a good balance of memory against performance.
    int pointsPerVoxel = 8;
    float voxelSize = openvdb::points::computeVoxelSize(positionsWrapper, pointsPerVoxel);

    // voxelSize can't be too small or the OpenVDB library segfaults. 10e-5 is too small
    if (voxelSize < 0.0001)
    {
      voxelSize = 0.0001;
    }

    // Create a transform using this voxel-size.
    openvdb::math::Transform::Ptr transform =
      openvdb::math::Transform::createLinearTransform(voxelSize);
    // Create a PointIndexGrid. This can be done automatically on creation of
    // the grid, however as this index grid is required for the position and
    // radius attributes, we create one we can use for both attribute creation.
    openvdb::tools::PointIndexGrid::Ptr pointIndexGrid =
      openvdb::tools::createPointIndexGrid<openvdb::tools::PointIndexGrid>(
        positionsWrapper, *transform);
    // Create a PointDataGrid containing these four points and using the point
    // index grid. This requires the positions wrapper.
    openvdb::points::PointDataGrid::Ptr grid =
      openvdb::points::createPointDataGrid<openvdb::points::NullCodec,
        openvdb::points::PointDataGrid>(*pointIndexGrid, positionsWrapper, *transform);

    // Set the name of the grid
    std::string vdbName = gridName;
    grid->setName(gridName);

    // VDB attributes need to have unique names
    std::set<std::string> vdbFieldNames;

    for (int array = 0; array < pointData->GetNumberOfArrays(); array++)
    {
      vtkDataArray* data = pointData->GetArray(array);
      const char* arrayName = data->GetName();
      const int numberOfComponents = data->GetNumberOfComponents();
      for (int component = 0; component < numberOfComponents; component++)
      {
        if (numberOfComponents == 3 && component > 0)
        {
          continue;
        }
        std::string vdbFieldName = GetVDBGridName(arrayName, component, numberOfComponents);
        // a variety of characters aren't allowed in the attribute name
        vdbFieldName = MakeValidAttributeName(vdbFieldName);
        // also attribute names need to be unique in the VDB channel
        if (vdbFieldNames.find(vdbFieldName) == vdbFieldNames.end())
        {
          vdbFieldNames.insert(vdbFieldName);
        }
        else
        {
          int counter = 1;
          std::string nextName = vdbFieldName + "_" + std::to_string(counter);
          bool found = vdbFieldNames.find(nextName) != vdbFieldNames.end();
          while (found)
          {
            counter++;
            nextName = vdbFieldName + "_" + std::to_string(counter);
            found = vdbFieldNames.find(nextName) != vdbFieldNames.end();
          }
          vdbFieldName = nextName;
          vdbFieldNames.insert(vdbFieldName);
        }

        // Append a data->GetName() attribute to the grid to hold the radius.
        // This attribute storage uses a unit range codec to reduce the memory
        // storage requirements down from 4-bytes to just 1-byte per value. This is
        // only possible because accuracy of the radius is not that important to us
        // and the values are always within unit range (0.0 => 1.0).
        // Note that this attribute type is not registered by default so needs to be
        // explicitly registered.
        // using Codec = openvdb::points::FixedPointCodec</*1-byte=*/false,
        //         openvdb::points::UnitRange>;
        // openvdb::points::TypedAttributeArray<float, Codec>::registerType();
        if (numberOfComponents == 3)
        {
          std::vector<openvdb::Vec3f> values;
          for (vtkIdType i = 0; i < numPoints; i++)
          {
            double tuple[3];
            data->GetTuple(i, tuple);
            openvdb::Vec3f ftuple(tuple[0], tuple[1], tuple[2]);
            values.push_back(ftuple);
          }
          openvdb::NamePair vectorAttribute = openvdb::points::TypedAttributeArray<openvdb::Vec3f,
            openvdb::points::NullCodec>::attributeType();
          openvdb::points::appendAttribute(grid->tree(), vdbFieldName, vectorAttribute);
          // Create a wrapper around the values vector.
          openvdb::points::PointAttributeVector<openvdb::Vec3f> valuesWrapper(values);
          // Populate the data->GetName() attribute on the points
          openvdb::points::populateAttribute<openvdb::points::PointDataTree,
            openvdb::tools::PointIndexTree, openvdb::points::PointAttributeVector<openvdb::Vec3f>>(
            grid->tree(), pointIndexGrid->tree(), vdbFieldName, valuesWrapper);
        }
        else
        {
          std::vector<float> values;
          for (vtkIdType i = 0; i < numPoints; i++)
          {
            values.push_back(static_cast<float>(data->GetComponent(i, component)));
          }
          openvdb::NamePair scalarAttribute = openvdb::points::TypedAttributeArray<float,
            openvdb::points::NullCodec>::attributeType();
          openvdb::points::appendAttribute(grid->tree(), vdbFieldName, scalarAttribute);
          // Create a wrapper around the values vector.
          openvdb::points::PointAttributeVector<float> valuesWrapper(values);
          // Populate the data->GetName() attribute on the points
          openvdb::points::populateAttribute<openvdb::points::PointDataTree,
            openvdb::tools::PointIndexTree, openvdb::points::PointAttributeVector<float>>(
            grid->tree(), pointIndexGrid->tree(), vdbFieldName, valuesWrapper);
        }
      } // iterate over number of components
    }   // iterate over point arrays

    vtkBoundingBox bounds(pointSet->GetBounds());
    vtkBoundingBox globalBounds(bounds);

    if (this->Writer->Controller && this->Writer->Controller->GetNumberOfProcesses() > 1)
    {
      this->Writer->Controller->AllReduce(bounds, globalBounds);
    }

    double tmp[3];
    bounds.GetCenter(tmp);
    grid->insertMeta("center", openvdb::Vec3SMetadata(openvdb::Vec3f(tmp)));
    globalBounds.GetCenter(tmp);
    grid->insertMeta("global center", openvdb::Vec3SMetadata(openvdb::Vec3f(tmp)));
    grid->insertMeta("min bounds",
      openvdb::Vec3SMetadata(
        openvdb::Vec3f(bounds.GetBound(0), bounds.GetBound(2), bounds.GetBound(4))));
    grid->insertMeta("max bounds",
      openvdb::Vec3SMetadata(
        openvdb::Vec3f(bounds.GetBound(1), bounds.GetBound(3), bounds.GetBound(5))));
    grid->insertMeta("global min bounds",
      openvdb::Vec3SMetadata(openvdb::Vec3f(
        globalBounds.GetBound(0), globalBounds.GetBound(2), globalBounds.GetBound(4))));
    grid->insertMeta("global max bounds",
      openvdb::Vec3SMetadata(openvdb::Vec3f(
        globalBounds.GetBound(1), globalBounds.GetBound(3), globalBounds.GetBound(5))));
    if (pointSet->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
    {
      double time = pointSet->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
      grid->insertMeta("time", openvdb::DoubleMetadata(time));
    }

    // enum GridClass {
    //     GRID_UNKNOWN = 0,
    //     GRID_LEVEL_SET,
    //     GRID_FOG_VOLUME,
    //     GRID_STAGGERED
    // };
    if (isAPolyData)
    {
      grid->setGridClass(openvdb::GRID_LEVEL_SET);
    }
    else
    {
      grid->setGridClass(openvdb::GRID_FOG_VOLUME);
    }

    return grid;
  }
}; // end of vtkOpenVDBWriterInternals class

vtkStandardNewMacro(vtkOpenVDBWriter);
vtkCxxSetObjectMacro(vtkOpenVDBWriter, Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkOpenVDBWriter, LookupTable, vtkScalarsToColors);

//-----------------------------------------------------------------------------
vtkOpenVDBWriter::vtkOpenVDBWriter()
{
  // openvdb::initialize() can be called multiple times
  openvdb::initialize();
  this->FileName = nullptr;
  this->WriteAllTimeSteps = false;
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->LookupTable = nullptr;
  this->EnableColoring = false;
  this->EnableAlpha = false;
  this->Internals = new vtkOpenVDBWriterInternals(this);
}

//-----------------------------------------------------------------------------
vtkOpenVDBWriter::~vtkOpenVDBWriter()
{
  this->SetFileName(nullptr);
  this->SetController(nullptr);
  delete this->Internals;
}

//-----------------------------------------------------------------------------
int vtkOpenVDBWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkOpenVDBWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      (this->Controller ? this->Controller->GetNumberOfProcesses() : 1));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      (this->Controller ? this->Controller->GetLocalProcessId() : 0));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

    if (this->WriteAllTimeSteps)
    {
      double* inTimes = inputVector[0]->GetInformationObject(0)->Get(
        vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      if (inTimes && this->WriteAllTimeSteps)
      {
        double timeReq = inTimes[this->Internals->CurrentTimeIndex];
        inputVector[0]->GetInformationObject(0)->Set(
          vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
      }
    }
    return 1;
  }
  else if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      // reset the CurrentTimeIndex in case we're writing out all of the time steps
      this->Internals->CurrentTimeIndex = 0;
      this->Internals->NumberOfTimeSteps =
        inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    }
    else
    {
      this->Internals->NumberOfTimeSteps = 1;
    }
  }
  else if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_DATA()))
  {
    if (this->WriteAllTimeSteps && this->Internals->CurrentTimeIndex == 0)
    {
      // Tell the pipeline to start looping.
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }
  }

  int retVal = this->Superclass::ProcessRequest(request, inputVector, outputVector);

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_DATA()))
  {
    if (this->WriteAllTimeSteps &&
      this->Internals->CurrentTimeIndex == this->Internals->NumberOfTimeSteps)
    {
      // Tell the pipeline to stop looping.
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->Internals->CurrentTimeIndex = 0;
    }
  }

  return retVal;
}

//-----------------------------------------------------------------------------
void vtkOpenVDBWriter::WriteData()
{
  if (vtkImageData* imageData = vtkImageData::SafeDownCast(this->GetInput()))
  {
    this->WriteImageData(imageData);
  }
  else if (vtkPointSet* pointSet = vtkPointSet::SafeDownCast(this->GetInput()))
  {
    this->WritePointSet(pointSet);
  }
  this->Internals->CurrentTimeIndex++;
}

//-----------------------------------------------------------------------------
void vtkOpenVDBWriter::WriteImageData(vtkImageData* imageData)
{
  std::vector<openvdb::GridBase::Ptr> grids;

  double dx(0), dy(0), dz(0);
  imageData->GetSpacing(dx, dy, dz);

  if ((dx < 0.0001 && dx > 0.) || (dy < 0.0001 && dy > 0.) || (dz < 0.0001 && dz > 0.))
  {
    vtkWarningMacro("Cell size is too small for VDB tolerances. Increasing to avoid segfault.");
    while ((dx < 0.0001 && dx > 0.) || (dy < 0.0001 && dy > 0.) || (dz < 0.0001 && dz > 0.))
    {
      dx *= 2;
      dy *= 2;
      dz *= 2;
    }
  }

  // mat and linearTransform are used to transform our voxel geometry to the proper shape
  openvdb::math::Mat4d mat(dx, 0., 0., 0., 0., dy, 0., 0., 0., 0., dz, 0., 0., 0., 0., 1.);

  openvdb::math::Transform::Ptr linearTransform =
    openvdb::math::Transform::createLinearTransform(mat);

  int extent[6], wholeExtent[6];
  imageData->GetExtent(extent);
  if (this->Controller)
  {
    this->Controller->AllReduce(extent, wholeExtent, 6, vtkCommunicator::MAX_OP);
  }
  imageData->GetExtent(extent);
  int pointExtent[6] = { extent[0], extent[1], extent[2], extent[3], extent[4], extent[5] };

  // since we don't want duplicate data in parallel for the pointdata
  // we chop of the upper points if they are not on the boundary and let
  // other processes handle that data
  for (int i = 0; i < 3; i++)
  {
    if (extent[2 * i + 1] != wholeExtent[2 * i + 1])
    {
      pointExtent[2 * i + 1] = extent[2 * i + 1] - 1;
    }
  }

  vtkBoundingBox bounds(imageData->GetBounds());
  vtkBoundingBox globalBounds(bounds);

  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    this->Controller->AllReduce(bounds, globalBounds);
  }

  // compute colors, if any
  vtkNew<vtkPointData> pointData;
  pointData->ShallowCopy(imageData->GetPointData());
  vtkNew<vtkCellData> cellData;
  cellData->ShallowCopy(imageData->GetCellData());
  if (this->LookupTable && this->EnableColoring)
  {
    int fieldAssociation = 0;
    vtkAbstractArray* scalars =
      this->GetInputAbstractArrayToProcess(0, imageData, fieldAssociation);
    vtkDiscretizableColorTransferFunction* dctf =
      vtkDiscretizableColorTransferFunction::SafeDownCast(this->LookupTable);
    bool enableOpacityMapping = dctf->GetEnableOpacityMapping();
    dctf->SetEnableOpacityMapping(this->EnableAlpha);
    vtkUnsignedCharArray* rgba =
      this->LookupTable->MapScalars(scalars, VTK_COLOR_MODE_MAP_SCALARS, -1);
    vtkIdType numPoints = imageData->GetNumberOfPoints();
    if (rgba && rgba->GetNumberOfTuples() == numPoints)
    {
      this->SetRGBA(numPoints, rgba, pointData);
    }
    fieldAssociation = 1;
    scalars = this->GetInputAbstractArrayToProcess(0, imageData, fieldAssociation);
    dctf = vtkDiscretizableColorTransferFunction::SafeDownCast(this->LookupTable);
    rgba = this->LookupTable->MapScalars(scalars, VTK_COLOR_MODE_MAP_SCALARS, -1);
    vtkIdType numCells = imageData->GetNumberOfCells();
    if (rgba && rgba->GetNumberOfTuples() == numCells)
    {
      this->SetRGBA(numCells, rgba, cellData);
    }
    dctf->SetEnableOpacityMapping(enableOpacityMapping);
  }

  vtkUnsignedCharArray* pointGhostType =
    vtkUnsignedCharArray::SafeDownCast(pointData->GetArray(vtkDataSetAttributes::GhostArrayName()));
  if (pointGhostType && pointGhostType->GetRange(0)[1] == 0)
  {
    pointGhostType = nullptr; // no ghosts
  }
  vtkUnsignedCharArray* cellGhostType =
    vtkUnsignedCharArray::SafeDownCast(cellData->GetArray(vtkDataSetAttributes::GhostArrayName()));
  if (cellGhostType && cellGhostType->GetRange(0)[1] == 0)
  {
    cellGhostType = nullptr; // no ghosts
  }
  bool needToUpdateBounds = false;
  if (pointGhostType || cellGhostType)
  {
    needToUpdateBounds = true;
    bounds.Reset();
  }

  vtkBoundingBox boundingBox;
  for (int array = 0; array < pointData->GetNumberOfArrays(); array++)
  {
    vtkDataArray* data = pointData->GetArray(array);
    const char* arrayName = data->GetName();
    const int numberOfComponents = data->GetNumberOfComponents();
    for (int component = 0; component < numberOfComponents; component++)
    {
      if (numberOfComponents == 3 && component > 0)
      {
        continue;
      }
      // Vec3SGrid is single precision and Vec3DGrid is for double precision
      openvdb::Vec3SGrid::Ptr vecGrid = openvdb::Vec3SGrid::create();
      // see
      // https://www.openvdb.org/documentation/doxygen/namespaceopenvdb_1_1v8__0.html#ae93f92d10730a52ed3b207d5811f6a6e
      if (strcmp(arrayName, "color"))
      {
        vecGrid->setVectorType(openvdb::VEC_CONTRAVARIANT_RELATIVE);
      }
      else
      {
        vecGrid->setVectorType(openvdb::VEC_INVARIANT);
      }
      vecGrid->setGridClass(openvdb::GRID_FOG_VOLUME);

      openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create();
      // enum GridClass {
      //     GRID_UNKNOWN = 0,
      //     GRID_LEVEL_SET,
      //     GRID_FOG_VOLUME,
      //     GRID_STAGGERED
      // };
      grid->setGridClass(openvdb::GRID_FOG_VOLUME);
      std::string vdbName = GetVDBGridName(arrayName, component, numberOfComponents);
      grid->setName(vdbName);
      vecGrid->setName(vdbName);
      openvdb::FloatGrid::Accessor accessor = grid->getAccessor();
      openvdb::Vec3SGrid::Accessor vecAccessor = vecGrid->getAccessor();
      openvdb::Coord ijk;

      int &i = ijk[0], &j = ijk[1], &k = ijk[2];
      int vtkijk[3];
      for (k = pointExtent[4]; k <= pointExtent[5]; ++k)
      {
        vtkijk[2] = k;
        for (j = pointExtent[2]; j <= pointExtent[3]; ++j)
        {
          vtkijk[1] = j;
          for (i = pointExtent[0]; i <= pointExtent[1]; ++i)
          {
            vtkijk[0] = i;
            vtkIdType pointId = imageData->ComputePointId(vtkijk);
            if (needToUpdateBounds && pointGhostType && pointGhostType->GetTuple1(pointId) == 0)
            {
              double coords[3];
              imageData->GetPoint(pointId, coords);
              boundingBox.AddPoint(coords);
            }

            if (!pointGhostType || pointGhostType->GetTuple1(pointId) == 0)
            {
              if (numberOfComponents == 3)
              {
                double tuple[3];
                data->GetTuple(pointId, tuple);
                openvdb::Vec3f ftuple(tuple[0], tuple[1], tuple[2]);
                vecAccessor.setValue(ijk, ftuple);
              }
              else
              {
                accessor.setValue(ijk, data->GetComponent(pointId, component));
              }
            }
          }
        }
      }

      grid->setTransform(linearTransform);
      vecGrid->setTransform(linearTransform);

      if (numberOfComponents == 3)
      {
        grids.push_back(vecGrid);
      }
      else
      {
        grids.push_back(grid);
      }
    }
  }

  // find the cell size so that we can determine the location of its center
  // and then figure out the bounds but only do this if we need to
  // update the bounds
  double halfCellSize[3] = { dx / 2., dy / 2., dz / 2. };

  for (int array = 0; array < cellData->GetNumberOfArrays(); array++)
  {
    vtkDataArray* data = cellData->GetArray(array);
    const char* arrayName = data->GetName();
    const int numberOfComponents = data->GetNumberOfComponents();
    for (int component = 0; component < numberOfComponents; component++)
    {
      if (numberOfComponents == 3 && component > 0)
      {
        continue;
      }
      // Vec3SGrid is single precision and Vec3DGrid is for double precision
      openvdb::Vec3SGrid::Ptr vecGrid = openvdb::Vec3SGrid::create();
      // see
      // https://www.openvdb.org/documentation/doxygen/namespaceopenvdb_1_1v8__0.html#ae93f92d10730a52ed3b207d5811f6a6e
      if (strcmp(arrayName, "color"))
      {
        vecGrid->setVectorType(openvdb::VEC_CONTRAVARIANT_RELATIVE);
      }
      else
      {
        vecGrid->setVectorType(openvdb::VEC_INVARIANT);
      }
      vecGrid->setGridClass(openvdb::GRID_FOG_VOLUME);

      openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create();
      // enum GridClass {
      //     GRID_UNKNOWN = 0,
      //     GRID_LEVEL_SET,
      //     GRID_FOG_VOLUME,
      //     GRID_STAGGERED
      // };
      grid->setGridClass(openvdb::GRID_FOG_VOLUME);
      std::string vdbName = GetVDBGridName(arrayName, component, numberOfComponents);
      grid->setName(vdbName);
      vecGrid->setName(vdbName);
      openvdb::FloatGrid::Accessor accessor = grid->getAccessor();
      openvdb::Vec3SGrid::Accessor vecAccessor = vecGrid->getAccessor();
      openvdb::Coord ijk;

      int &i = ijk[0], &j = ijk[1], &k = ijk[2];
      for (k = extent[4]; k < extent[5]; ++k)
      {
        for (j = extent[2]; j < extent[3]; ++j)
        {
          for (i = extent[0]; i < extent[1]; ++i)
          {
            int vtkijk[3] = { i, j, k };
            vtkIdType cellId = imageData->ComputeCellId(vtkijk);
            if (needToUpdateBounds && cellGhostType && cellGhostType->GetTuple1(cellId) == 0)
            {
              double coords[3];
              vtkIdType pointId = imageData->ComputePointId(vtkijk);
              imageData->GetPoint(pointId, coords);
              for (int c = 0; c < 3; c++)
              {
                coords[c] += halfCellSize[c];
              }
              bounds.AddPoint(coords);
            }

            if (!cellGhostType || cellGhostType->GetTuple1(cellId) == 0)
            {
              if (numberOfComponents == 3)
              {
                double tuple[3];
                data->GetTuple(cellId, tuple);
                openvdb::Vec3f ftuple(tuple[0], tuple[1], tuple[2]);
                vecAccessor.setValue(ijk, ftuple);
              }
              else
              {
                accessor.setValue(ijk, data->GetComponent(cellId, component));
              }
            }
          }
        }
      }

      grid->setTransform(linearTransform);
      vecGrid->setTransform(linearTransform);

      if (numberOfComponents == 3)
      {
        grids.push_back(vecGrid);
      }
      else
      {
        grids.push_back(grid);
      }
    } // iterator over number of components
  }   // iterate over arrays

  // if no point data or no cell data then we just add in a value of 1 for the voxels for the cells
  if (grids.empty())
  {
    openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create();
    // enum GridClass {
    //     GRID_UNKNOWN = 0,
    //     GRID_LEVEL_SET,
    //     GRID_FOG_VOLUME,
    //     GRID_STAGGERED
    // };
    grid->setGridClass(openvdb::GRID_FOG_VOLUME);
    grid->setName("empty");
    openvdb::FloatGrid::Accessor accessor = grid->getAccessor();
    openvdb::Coord ijk;

    int &i = ijk[0], &j = ijk[1], &k = ijk[2];
    // if we're here we don't have any ghost info since it would be stored in point or cell data
    for (k = extent[4]; k < extent[5]; ++k)
    {
      for (j = extent[2]; j < extent[3]; ++j)
      {
        for (i = extent[0]; i < extent[1]; ++i)
        {
          accessor.setValue(ijk, 1.);
        }
      }
      grid->setTransform(linearTransform);
    }
    grids.push_back(grid);
  }

  for (auto& grid : grids)
  {
    // meta-information to help orient the grid back to the original geometric location
    double tmp[3];
    bounds.GetCenter(tmp);
    grid->insertMeta("center", openvdb::Vec3SMetadata(openvdb::Vec3f(tmp)));
    globalBounds.GetCenter(tmp);
    grid->insertMeta("global center", openvdb::Vec3SMetadata(openvdb::Vec3f(tmp)));
    grid->insertMeta("min bounds",
      openvdb::Vec3SMetadata(
        openvdb::Vec3f(bounds.GetBound(0), bounds.GetBound(2), bounds.GetBound(4))));
    grid->insertMeta("max bounds",
      openvdb::Vec3SMetadata(
        openvdb::Vec3f(bounds.GetBound(1), bounds.GetBound(3), bounds.GetBound(5))));
    grid->insertMeta("global min bounds",
      openvdb::Vec3SMetadata(openvdb::Vec3f(
        globalBounds.GetBound(0), globalBounds.GetBound(2), globalBounds.GetBound(4))));
    grid->insertMeta("global max bounds",
      openvdb::Vec3SMetadata(openvdb::Vec3f(
        globalBounds.GetBound(1), globalBounds.GetBound(3), globalBounds.GetBound(5))));
    if (imageData->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
    {
      double time = imageData->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
      grid->insertMeta("time", openvdb::DoubleMetadata(time));
    }
  }

  WriteVDBGrids(grids, this->Controller, this->FileName, this->WriteAllTimeSteps,
    this->Internals->NumberOfTimeSteps, this->Internals->CurrentTimeIndex);
}

//-----------------------------------------------------------------------------
void vtkOpenVDBWriter::WritePointSet(vtkPointSet* pointSet)
{
  openvdb::points::PointDataGrid::Ptr pointsGrid =
    this->Internals->ProcessPointSet(pointSet, "Points", pointSet->IsA("vtkPolyData"));

  std::vector<openvdb::GridBase::Ptr> grids;
  grids.push_back(pointsGrid);

  if (pointSet->GetCellData()->GetNumberOfArrays() != 0 ||
    pointSet->GetPointData()->GetNumberOfArrays() == 0)
  {
    // need to use the CellCenters filter to get the center of each cell
    vtkNew<vtkCellCenters> cellCenters;
    cellCenters->SetInputData(pointSet);
    cellCenters->SetVertexCells(true);
    cellCenters->SetCopyArrays(true);
    cellCenters->Update();

    vtkPointSet* newPointSet = vtkPointSet::SafeDownCast(cellCenters->GetOutput());

    grids.push_back(
      this->Internals->ProcessPointSet(newPointSet, "Cells", pointSet->IsA("vtkPolyData")));
  }

  WriteVDBGrids(grids, this->Controller, this->FileName, this->WriteAllTimeSteps,
    this->Internals->NumberOfTimeSteps, this->Internals->CurrentTimeIndex);
}

//-----------------------------------------------------------------------------
void vtkOpenVDBWriter::SetRGBA(
  vtkIdType num, vtkUnsignedCharArray* rgbaArray, vtkDataSetAttributes* attributes)
{
  vtkIdType i;

  int numComp = rgbaArray->GetNumberOfComponents();
  if (numComp == 3)
  { // have unsigned char array of three components, copy it
    rgbaArray->SetName("color");
    attributes->AddArray(rgbaArray);
    if (this->EnableAlpha)
    {
      vtkWarningMacro("No alpha channel to set even though requested");
    }
    return;
  }
  else if (numComp == 4)
  {
    // have unsigned char array of four components (RGBA), copy it without the `A`.
    vtkSmartPointer<vtkFloatArray> colors = vtkSmartPointer<vtkFloatArray>::New();
    colors->SetNumberOfComponents(3);
    colors->SetNumberOfTuples(num);
    colors->SetName("color");
    float* c = colors->WritePointer(0, 3 * num);
    vtkSmartPointer<vtkFloatArray> alpha = vtkSmartPointer<vtkFloatArray>::New();
    alpha->SetNumberOfComponents(1);
    alpha->SetNumberOfTuples(num);
    alpha->SetName("alpha");
    float* a = alpha->WritePointer(0, num);
    const unsigned char* rgba = rgbaArray->GetPointer(0);
    for (i = 0; i < num; i++)
    {
      *c++ = (*rgba++) / 255.;
      *c++ = (*rgba++) / 255.;
      *c++ = (*rgba++) / 255.;
      *a++ = (*rgba++) / 255.;
    }
    attributes->AddArray(colors);
    if (this->EnableAlpha)
    {
      attributes->AddArray(alpha);
    }
    return;
  }
  // no lookup table
  return;
}

//-----------------------------------------------------------------------------
void vtkOpenVDBWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "none") << endl;
  os << indent << "WriteAllTimeSteps: " << this->WriteAllTimeSteps << endl;
  if (this->Controller)
  {
    os << indent << "Controller: " << this->Controller << endl;
  }
  else
  {
    os << indent << "Controller: (none)" << endl;
  }
  if (this->LookupTable)
  {
    os << indent << "LookupTable: " << this->LookupTable << endl;
  }
  else
  {
    os << indent << "LookupTable: (none)" << endl;
  }
  os << indent << "EnableColoring: " << this->EnableColoring << endl;
  os << indent << "EnableAlpha: " << this->EnableAlpha << endl;
}
