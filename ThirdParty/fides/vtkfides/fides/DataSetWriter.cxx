//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/DataSetWriter.h>
#include <fides/predefined/DataModelFactory.h>
#include <fides/predefined/DataModelHelperFunctions.h>
#include <fides/predefined/InternalMetadataSource.h>

#include <ios>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include <fides_rapidjson.h>
// clang-format off
#include FIDES_RAPIDJSON(rapidjson/error/en.h)
#include FIDES_RAPIDJSON(rapidjson/filereadstream.h)
#include FIDES_RAPIDJSON(rapidjson/schema.h)
#include FIDES_RAPIDJSON(rapidjson/stringbuffer.h)
#include FIDES_RAPIDJSON(rapidjson/prettywriter.h)
// clang-format on

#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/UnknownCellSet.h>

#include <fides/CellSet.h>
#include <fides/CoordinateSystem.h>
#include <fides/DataSource.h>
#include <fides/Field.h>

#ifdef FIDES_USE_MPI
#include <vtk_mpi.h>
#endif

template <typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& v)
{
  if (!v.empty())
  {
    out << "[";
    std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, ", "));
    out << "]";
  }
  return out;
}

namespace fides
{
namespace io
{

namespace
{

// When VTK converts to VTK-m DataSet, it may do a cast of some arrays from 32-bit to 64-bit Integers.
// I don't think we need to handle any other cases than what are in these lists.
using CellSetSingleTypeList =
  vtkm::List<vtkm::cont::CellSetSingleType<>,
             vtkm::cont::CellSetSingleType<
               vtkm::cont::StorageTagCast<vtkm::Int32, vtkm::cont::StorageTagBasic>>>;

using CellSetExplicitList =
  vtkm::List<vtkm::cont::CellSetExplicit<>,
             vtkm::cont::CellSetExplicit<
               vtkm::cont::StorageTagBasic,
               vtkm::cont::StorageTagCast<vtkm::Int32, vtkm::cont::StorageTagBasic>,
               vtkm::cont::StorageTagCast<vtkm::Int32, vtkm::cont::StorageTagBasic>>>;

using FullCellSetExplicitList = vtkm::ListAppend<CellSetSingleTypeList, CellSetExplicitList>;

struct GetDataSetTypeFunctor
{
  template <typename ConnectivityStorage>
  VTKM_CONT void operator()(const vtkm::cont::CellSetSingleType<ConnectivityStorage>&,
                            unsigned char& type,
                            DataSetWriter& self)
  {
    type = self.DATASET_TYPE_UNSTRUCTURED_SINGLE;
  }

  template <typename ShapesStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VTKM_CONT void operator()(
    const vtkm::cont::CellSetExplicit<ShapesStorage, ConnectivityStorage, OffsetsStorage>&,
    unsigned char& type,
    DataSetWriter& self)
  {
    type = self.DATASET_TYPE_UNSTRUCTURED;
  }

  VTKM_CONT void operator()(const vtkm::cont::CellSet&, unsigned char& type, DataSetWriter& self)
  {
    // in this case we didn't find an appropriate dataset type
    type = self.DATASET_TYPE_ERROR;
  }
};

// Create an ADIOS Variable based on the type of the ArrayHandle
struct DefineVariableFunctor
{
  // in the case where we have an array that is casted, we'll just have adios use
  // the original type, because when we read back in, Fides shouldn't care about
  // the types (and if there's an issue, it's likely a bug). So this way, we don't
  // have to actually create an array of the casted type in order to have adios write it
  template <typename TCast, typename TOrig>
  VTKM_CONT void operator()(
    const vtkm::cont::ArrayHandle<TCast,
                                  vtkm::cont::StorageTagCast<TOrig, vtkm::cont::StorageTagBasic>>&,
    const std::vector<size_t>& shape,
    const std::vector<size_t>& offset,
    const std::vector<size_t>& size,
    adios2::IO& io,
    const std::string& name)
  {
    io.template DefineVariable<TOrig>(name, shape, offset, size);
  }

  template <typename T, typename S>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, S>&,
                            const std::vector<size_t>& shape,
                            const std::vector<size_t>& offset,
                            const std::vector<size_t>& size,
                            adios2::IO& io,
                            const std::string& name)
  {
    using ComponentType = typename vtkm::VecTraits<T>::ComponentType;
    io.template DefineVariable<ComponentType>(name, shape, offset, size);
  }
};

// For CellSets we have an extra step to do before we can define variables for
// the necessary ArrayHandle(s)
struct DefineCellsVariableFunctor
{
  template <typename S>
  VTKM_CONT void operator()(const vtkm::cont::CellSetSingleType<S>& cellSet,
                            const std::vector<size_t>& shape,
                            const std::vector<size_t>& offset,
                            const std::vector<size_t>& size,
                            adios2::IO& io,
                            const std::string& name)
  {
    const auto& conn =
      cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});
    DefineVariableFunctor functor;
    functor(conn, shape, offset, size, io, name);
  }

  template <typename S>
  VTKM_CONT void operator()(const vtkm::cont::CellSetExplicit<S>& cellSet,
                            const std::vector<size_t>& shape,
                            const std::vector<size_t>& offset,
                            const std::vector<size_t>& size,
                            adios2::IO& io,
                            const std::string& name)
  {
    const auto& conn =
      cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});
    DefineVariableFunctor functor;
    functor(conn, shape, offset, size, io, name);
  }

  template <typename ConnType>
  VTKM_CONT void operator()(
    const vtkm::cont::CellSetExplicit<
      vtkm::cont::StorageTagBasic,
      vtkm::cont::StorageTagCast<ConnType, vtkm::cont::StorageTagBasic>,
      vtkm::cont::StorageTagCast<vtkm::Int32, vtkm::cont::StorageTagBasic>>& cellSet,
    const std::vector<size_t>& shape,
    const std::vector<size_t>& offset,
    const std::vector<size_t>& size,
    adios2::IO& io,
    const std::string& name)
  {
    const auto& conn =
      cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});
    DefineVariableFunctor functor;
    functor(conn, shape, offset, size, io, name);
  }

  VTKM_CONT void operator()(const vtkm::cont::CellSet& cellSet,
                            const std::vector<size_t>&,
                            const std::vector<size_t>&,
                            const std::vector<size_t>&,
                            adios2::IO&,
                            const std::string&)
  {
    std::string err = std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " +
      vtkm::cont::TypeToString(typeid(cellSet)) + " is not supported";
    throw std::runtime_error(err);
  }
};

struct WriteExplicitCoordsFunctor
{
  template <typename T>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& array,
                            adios2::IO& io,
                            adios2::Engine& engine,
                            size_t& cOffset,
                            size_t totalNumberOfCoords)
  {
    auto coordsVar = io.template InquireVariable<T>("coordinates");
    coordsVar.SetShape({ totalNumberOfCoords, 3 });

    vtkm::cont::ArrayHandleBasic<vtkm::Vec<T, 3>> arr(array);
    const vtkm::Vec<T, 3>* buffVec = arr.GetReadPointer();
    const T* buff = &buffVec[0][0];

    std::size_t numCoords = static_cast<std::size_t>(array.GetNumberOfValues());
    // This is a way you can write chunks in.
    // Instead of buffering the entire dataset, and then writing it,
    // you can buffer subsets, and specify a "Box" offset.
    adios2::Box<adios2::Dims> sel({ cOffset, 0 }, { numCoords, 3 });

    coordsVar.SetSelection(sel);
    engine.template Put<T>(coordsVar, buff);

    cOffset += numCoords;
  }

  template <typename T, typename S>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, S>& array,
                            adios2::IO&,
                            adios2::Engine&,
                            size_t&,
                            size_t)
  {
    std::string err = std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " +
      vtkm::cont::TypeToString(typeid(array)) + " is not supported";
    throw std::runtime_error(err);
  }
};

template <typename T>
vtkm::cont::ArrayHandle<T> GetSourceConnectivityArray(const vtkm::cont::ArrayHandle<T>& conn)
{
  return conn;
}

template <typename TOrig, typename TCast>
vtkm::cont::ArrayHandle<TOrig> GetSourceConnectivityArray(
  const vtkm::cont::ArrayHandle<TCast,
                                vtkm::cont::StorageTagCast<TOrig, vtkm::cont::StorageTagBasic>>&
    conn)
{
  const vtkm::cont::ArrayHandleCast<TCast, vtkm::cont::ArrayHandle<TOrig>>& casted = conn;
  return casted.GetSourceArray();
}

struct WriteSingleTypeCellsFunctor
{
  template <typename ConnectivityStorage>
  VTKM_CONT void operator()(const vtkm::cont::CellSetSingleType<ConnectivityStorage>& cellSet,
                            adios2::IO& io,
                            adios2::Engine& engine,
                            size_t& offset,
                            size_t totalNumberOfConnIds)
  {
    const auto& conn =
      cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});
    const auto& sourceConn = GetSourceConnectivityArray(conn);

    std::size_t numConn = static_cast<std::size_t>(sourceConn.GetNumberOfValues());

    using ConnType = typename std::remove_reference<decltype(sourceConn)>::type::ValueType;
    auto connVar = io.template InquireVariable<ConnType>("connectivity");
    connVar.SetShape({ totalNumberOfConnIds });

    adios2::Box<adios2::Dims> sel({ offset }, { numConn });
    connVar.SetSelection(sel);

    vtkm::cont::ArrayHandleBasic<ConnType> arr(sourceConn);
    const ConnType* buff = arr.GetReadPointer();
    engine.Put<ConnType>(connVar, buff);

    offset += numConn;
  }

  VTKM_CONT void operator()(const vtkm::cont::CellSet& cellSet,
                            adios2::IO&,
                            adios2::Engine&,
                            size_t&,
                            size_t)
  {
    std::string err = std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " +
      vtkm::cont::TypeToString(typeid(cellSet)) + " is not yet supported";
    throw std::runtime_error(err);
  }
};

struct CheckCellSetExplicitTypeFunctor
{
  template <typename ShapesStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VTKM_CONT void operator()(
    const vtkm::cont::CellSetExplicit<ShapesStorage, ConnectivityStorage, OffsetsStorage>&,
    bool& isType)
  {
    isType = true;
  }

  VTKM_CONT void operator()(const vtkm::cont::CellSet&, bool& isType) { isType = false; }
};

struct ComputeNumConnsFunctor
{
  template <typename ShapesStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VTKM_CONT void operator()(
    const vtkm::cont::CellSetExplicit<ShapesStorage, ConnectivityStorage, OffsetsStorage>& cellSet,
    vtkm::Id& numConn)
  {
    const auto& conn =
      cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});
    numConn += conn.GetNumberOfValues();
  }

  VTKM_CONT void operator()(const vtkm::cont::CellSet& cellSet, vtkm::Id&) const
  {
    std::string err = std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " +
      vtkm::cont::TypeToString(typeid(cellSet)) + " is not supported";
    throw std::runtime_error(err);
  }
};

struct WriteExplicitCellsFunctor
{
  template <typename ShapesStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VTKM_CONT void operator()(
    const vtkm::cont::CellSetExplicit<ShapesStorage, ConnectivityStorage, OffsetsStorage>& cellSet,
    std::size_t& cellOffset,
    std::size_t& connOffset,
    std::vector<vtkm::IdComponent>& numVerts,
    std::size_t& numVertsOffset,
    vtkm::Id totalNumberOfConns,
    adios2::Engine& engine,
    adios2::IO& io)
  {
    size_t numCells = static_cast<size_t>(cellSet.GetNumberOfCells());

    const auto& shapes =
      cellSet.GetShapesArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});

    vtkm::cont::ArrayHandleBasic<uint8_t> shapes_arr(shapes);
    const uint8_t* buffer = shapes_arr.GetReadPointer();
    adios2::Box<adios2::Dims> shapesSelection({ cellOffset }, { numCells });

    auto shapesVar = io.InquireVariable<uint8_t>("cell_types");
    shapesVar.SetSelection(shapesSelection);
    engine.Put<uint8_t>(shapesVar, buffer);

    // Each offset must be converted to a number of vertices. See
    // CellSetExplicit::PostRead
    auto const& offsets =
      cellSet.GetOffsetsArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});
    auto rp = offsets.ReadPortal();

    for (vtkm::Id i = 0; static_cast<size_t>(i) < numCells; i++)
    {
      numVerts[numVertsOffset + i] = rp.Get(i + 1) - rp.Get(i);
    }

    adios2::Box<adios2::Dims> vertsVarSel({ cellOffset }, { numCells });
    auto vertsVar = io.InquireVariable<vtkm::IdComponent>("num_verts");
    vertsVar.SetSelection(vertsVarSel);
    engine.Put(vertsVar, &(numVerts[numVertsOffset]));
    cellOffset += numCells;
    numVertsOffset += numCells;

    const auto& conn =
      cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});
    const auto& sourceConn = GetSourceConnectivityArray(conn);

    std::size_t numConn = static_cast<std::size_t>(sourceConn.GetNumberOfValues());

    using ConnType = typename std::remove_reference<decltype(sourceConn)>::type::ValueType;
    auto connVar = io.template InquireVariable<ConnType>("connectivity");
    connVar.SetShape({ static_cast<size_t>(totalNumberOfConns) });

    adios2::Box<adios2::Dims> connSelection({ connOffset }, { numConn });
    connVar.SetSelection(connSelection);

    // Now get the buffer:
    vtkm::cont::ArrayHandleBasic<ConnType> conn_arr(sourceConn);
    const ConnType* buff4 = conn_arr.GetReadPointer();
    engine.Put<ConnType>(connVar, buff4);
    connOffset += numConn;
  }

  VTKM_CONT void operator()(const vtkm::cont::CellSet& cellSet,
                            std::size_t&,
                            std::size_t&,
                            std::vector<vtkm::IdComponent>&,
                            std::size_t&,
                            vtkm::Id,
                            adios2::Engine&,
                            adios2::IO&)
  {
    std::string err = std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " +
      vtkm::cont::TypeToString(typeid(cellSet)) + " is not supported";
    throw std::runtime_error(err);
  }
};

struct WriteFieldFunctor
{
  template <typename T>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& array,
                            adios2::IO& io,
                            adios2::Engine& engine,
                            const std::string& name,
                            size_t totalSize,
                            size_t offset,
                            size_t numValues)
  {
    auto var = io.template InquireVariable<T>(name);
    var.SetShape({ totalSize, 3 });

    adios2::Box<adios2::Dims> sel({ offset, 0 }, { numValues, 3 });
    var.SetSelection(sel);

    vtkm::cont::ArrayHandleBasic<vtkm::Vec<T, 3>> arr(array);
    const vtkm::Vec<T, 3>* buffVec = arr.GetReadPointer();
    const T* buff = &buffVec[0][0];
    engine.template Put<T>(var, buff);
  }

  template <typename T>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T>& array,
                            adios2::IO& io,
                            adios2::Engine& engine,
                            const std::string& name,
                            size_t totalSize,
                            size_t offset,
                            size_t numValues)
  {
    auto var = io.template InquireVariable<T>(name);
    var.SetShape({ totalSize });

    adios2::Box<adios2::Dims> sel({ offset }, { numValues });
    var.SetSelection(sel);

    vtkm::cont::ArrayHandleBasic<T> arr(array);
    const T* buff = arr.GetReadPointer();
    engine.template Put<T>(var, buff);
  }

  template <typename T, typename S>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, S>& array,
                            adios2::IO&,
                            adios2::Engine&,
                            const std::string&,
                            size_t,
                            size_t,
                            size_t)
  {
    std::string err = std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " +
      vtkm::cont::TypeToString(typeid(array)) + " is not supported";
    throw std::runtime_error(err);
  }
};

} // end anon namespace

class DataSetWriter::GenericWriter
{
public:
  GenericWriter(const vtkm::cont::PartitionedDataSet& dataSets,
                const std::string& fname,
                const std::string& outputMode,
                const bool& appendMode = false)
    : DataSets(dataSets)
    , OutputFileName(fname)
#ifdef FIDES_USE_MPI
    , Adios(MPI_COMM_WORLD)
#else
    , Adios()
#endif
    , FieldsToWriteSet(false)
  {
#ifdef FIDES_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &this->Rank);
    MPI_Comm_size(MPI_COMM_WORLD, &this->NumRanks);
#endif

    this->IO = this->Adios.DeclareIO(outputMode);
    this->IO.SetEngine(outputMode);
    this->Engine = this->IO.Open(this->OutputFileName,
                                 (appendMode ? adios2::Mode::Append : adios2::Mode::Write));
  }

  void Close()
  {
    this->Engine.Close();
    this->CloseCalled = true;
  }

  virtual ~GenericWriter()
  {
    assert(this->CloseCalled && "Error: DataSetWriter::Close() not called.");
  }

  void SetWriteFields(std::set<std::string>& writeFields)
  {
    this->FieldsToWriteSet = true;
    this->FieldsToWrite = writeFields;
  }

  void Write()
  {
    this->ComputeGlobalBlockInfo();

    bool firstTime = false;
    if (!this->VariablesDefined)
    {
      this->DefineDataModelVariables();
      this->DefineFieldVariables();
      this->VariablesDefined = true;
      firstTime = true;
    }

    this->Engine.BeginStep(adios2::StepMode::Append);

    if (firstTime)
    {
      this->WriteSchema();
    }

    this->WriteCoordinates();
    this->WriteCells();
    this->WriteFields();
    this->Engine.PerformPuts();

    this->Engine.EndStep();
  }

  virtual void WriteCoordinates() = 0;
  virtual void WriteCells() = 0;
  virtual void DefineDataModelVariables() = 0;

  virtual void WriteFields()
  {
    for (const auto& varName : this->PointCenteredFieldVars)
    {
      std::size_t ptsOffset = this->DataSetPointsOffset;
      for (vtkm::Id i = 0; i < this->DataSets.GetNumberOfPartitions(); i++)
      {
        const auto& ds = this->DataSets.GetPartition(i);
        std::size_t numPoints = static_cast<std::size_t>(ds.GetNumberOfPoints());

        if (!ds.HasPointField(varName))
        {
          throw std::runtime_error("Variable " + varName + " not in datasset.");
        }
        auto field = ds.GetField(varName).GetData();
        field.CastAndCallForTypes<vtkm::TypeListCommon, VTKM_DEFAULT_STORAGE_LIST>(
          WriteFieldFunctor{},
          this->IO,
          this->Engine,
          varName,
          this->TotalNumberOfPoints,
          ptsOffset,
          numPoints);
        ptsOffset += numPoints;
      }
    }

    for (const auto& varName : this->CellCenteredFieldVars)
    {
      std::size_t cellsOffset = this->DataSetCellsOffset;

      for (vtkm::Id i = 0; i < this->DataSets.GetNumberOfPartitions(); i++)
      {
        const auto& ds = this->DataSets.GetPartition(i);
        std::size_t numCells = static_cast<std::size_t>(ds.GetCellSet().GetNumberOfCells());

        if (!ds.HasCellField(varName))
        {
          throw std::runtime_error("Variable " + varName + " not in datasset.");
        }

        auto field = ds.GetField(varName).GetData();
        field.CastAndCallForTypes<vtkm::TypeListCommon, VTKM_DEFAULT_STORAGE_LIST>(
          WriteFieldFunctor{},
          this->IO,
          this->Engine,
          varName,
          this->TotalNumberOfCells,
          cellsOffset,
          numCells);
        cellsOffset += numCells;
      }
    }
  }

  void WriteSchema()
  {
    int rankWithDS = 0;
    for (int i = 0; i < this->NumRanks; i++)
    {
      if (this->DataSetsPerRank[static_cast<size_t>(i)] > 0)
      {
        rankWithDS = i;
        break;
      }
    }

    if (this->Rank == rankWithDS)
    {
      std::set<std::string> varsToWrite;
      auto ds = this->DataSets.GetPartition(0);
      auto dm = fides::predefined::DataModelFactory::GetInstance().CreateDataModel(ds);

      if (this->FieldsToWriteSet)
        dm->SetFieldsToWrite(this->FieldsToWrite);
      auto& doc = dm->GetDOM(false);
      auto attrMap = dm->GetAttributes();
      rapidjson::StringBuffer buf;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
      doc.Accept(writer);
      std::string schema = buf.GetString();
      this->IO.DefineAttribute<std::string>("fides/schema", schema);
      for (auto& attr : attrMap)
      {
        if (attr.second.size() == 1)
        {
          this->IO.DefineAttribute<std::string>(attr.first, attr.second[0]);
        }
        else
        {
          this->IO.DefineAttribute<std::string>(attr.first, attr.second.data(), attr.second.size());
        }
      }
    }
  }

  void DefineFieldVariables()
  {
    std::size_t numPoints = 0, numCells = 0;
    vtkm::IdComponent numFields = 0;

    vtkm::cont::DataSet ds0;
    if (this->DataSets.GetNumberOfPartitions() > 0)
    {
      ds0 = this->DataSets.GetPartition(0);
      numFields = ds0.GetNumberOfFields();
    }

    // Determine total number of points/cells.
    for (vtkm::Id i = 0; i < this->DataSets.GetNumberOfPartitions(); i++)
    {
      const auto& ds = this->DataSets.GetPartition(i);
      if (ds.GetNumberOfFields() != numFields)
      {
        throw std::runtime_error("DataSets with different number of fields not supported.");
      }

      numPoints += static_cast<std::size_t>(ds.GetNumberOfPoints());
      numCells += static_cast<std::size_t>(ds.GetCellSet().GetNumberOfCells());
    }

    for (vtkm::Id i = 0; i < numFields; i++)
    {
      const auto& field = ds0.GetField(i);
      const auto& name = field.GetName();
      if (!this->ShouldWriteVariable(name))
        continue;

      // CoordinatesSystems are handled at WriteCoordinates
      if (ds0.HasCoordinateSystem(name))
      {
        continue;
      }

      std::size_t numComponents = static_cast<std::size_t>(field.GetData().GetNumberOfComponents());
      std::vector<std::size_t> shape, offset, size;

      if (field.GetAssociation() == vtkm::cont::Field::Association::Points)
      {
        if (numComponents == 1)
        {
          shape = { this->TotalNumberOfPoints };
          offset = { this->DataSetPointsOffset };
          size = { numPoints };
        }
        else
        {
          shape = { this->TotalNumberOfPoints, numComponents };
          offset = { this->DataSetPointsOffset, 0 };
          size = { numPoints, numComponents };
        }

        field.GetData().CastAndCallForTypes<vtkm::TypeListCommon, VTKM_DEFAULT_STORAGE_LIST>(
          DefineVariableFunctor{}, shape, offset, size, this->IO, name);
        this->PointCenteredFieldVars.push_back(name);
      }
      else if (field.GetAssociation() == vtkm::cont::Field::Association::Cells)
      {
        if (numComponents == 1)
        {
          shape = { this->TotalNumberOfCells };
          offset = { this->DataSetCellsOffset };
          size = { numCells };
        }
        else
        {
          shape = { this->TotalNumberOfCells, numComponents };
          offset = { this->DataSetCellsOffset, 0 };
          size = { numCells, numComponents };
        }
        field.GetData().CastAndCallForTypes<vtkm::TypeListCommon, VTKM_DEFAULT_STORAGE_LIST>(
          DefineVariableFunctor{}, shape, offset, size, this->IO, name);
        this->CellCenteredFieldVars.push_back(name);
      }
    }
  }

  void SetDataSets(vtkm::cont::PartitionedDataSet dataSets) { this->DataSets = dataSets; }

protected:
  bool ShouldWriteVariable(const std::string& var) const
  {
    bool ret = true;
    if (this->FieldsToWriteSet)
    {
      //See if it's in our set of fields.
      ret = this->FieldsToWrite.find(var) != this->FieldsToWrite.end();
    }
    return ret;
  }

  void ComputeGlobalBlockInfo()
  {
    this->NumberOfDataSets = static_cast<std::size_t>(this->DataSets.GetNumberOfPartitions());

    this->DataSetsPerRank.clear();
    this->DataSetsPerRank.resize(static_cast<size_t>(this->NumRanks), 0);
    this->DataSetsPerRank[static_cast<size_t>(this->Rank)] =
      static_cast<int>(this->NumberOfDataSets);

#ifdef FIDES_USE_MPI
    MPI_Allreduce(
      MPI_IN_PLACE, this->DataSetsPerRank.data(), this->NumRanks, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
#endif

    int tot = std::accumulate(this->DataSetsPerRank.begin(), this->DataSetsPerRank.end(), 0);
    this->TotalNumberOfDataSets = static_cast<std::size_t>(tot);

    this->DataSetOffset = 0;
    for (size_t i = 0; i < static_cast<size_t>(this->Rank); i++)
    {
      this->DataSetOffset += static_cast<size_t>(this->DataSetsPerRank[i]);
    }

    // Need to determine the point and cell offsets for each block.
    std::vector<int> numPoints(static_cast<size_t>(this->NumRanks), 0);
    std::vector<int> numCells(static_cast<size_t>(this->NumRanks), 0);

    for (std::size_t i = 0; i < this->NumberOfDataSets; i++)
    {
      const auto& ds = this->DataSets.GetPartition(static_cast<vtkm::Id>(i));
      numPoints[static_cast<size_t>(this->Rank)] += ds.GetNumberOfPoints();
      numCells[static_cast<size_t>(this->Rank)] += ds.GetCellSet().GetNumberOfCells();
    }

#ifdef FIDES_USE_MPI
    MPI_Allreduce(MPI_IN_PLACE, numPoints.data(), this->NumRanks, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, numCells.data(), this->NumRanks, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
#endif

    tot = std::accumulate(numPoints.begin(), numPoints.end(), 0);
    this->TotalNumberOfPoints = static_cast<std::size_t>(tot);
    tot = std::accumulate(numCells.begin(), numCells.end(), 0);
    this->TotalNumberOfCells = static_cast<std::size_t>(tot);

    this->DataSetPointsOffset = 0;
    this->DataSetCellsOffset = 0;
    for (size_t i = 0; i < static_cast<size_t>(this->Rank); i++)
    {
      this->DataSetPointsOffset += static_cast<size_t>(numPoints[i]);
      this->DataSetCellsOffset += static_cast<size_t>(numCells[i]);
    }

    this->ComputeDataModelSpecificGlobalBlockInfo();
  }

  virtual void ComputeDataModelSpecificGlobalBlockInfo() = 0;

  vtkm::cont::PartitionedDataSet DataSets;
  std::string OutputFileName;
  adios2::ADIOS Adios;
  adios2::IO IO;
  adios2::Engine Engine;

  std::vector<std::string> PointCenteredFieldVars;
  std::vector<std::string> CellCenteredFieldVars;
  bool FieldsToWriteSet;
  std::set<std::string> FieldsToWrite;

  int Rank = 0;
  int NumRanks = 1;
  std::vector<int> DataSetsPerRank;
  std::size_t TotalNumberOfDataSets = 0;
  std::size_t TotalNumberOfPoints = 0;
  std::size_t TotalNumberOfCells = 0;
  std::size_t NumberOfDataSets = 0;
  std::size_t DataSetOffset = 0;
  std::size_t DataSetPointsOffset = 0;
  std::size_t DataSetCellsOffset = 0;
  bool VariablesDefined = false;
  bool CloseCalled = false;
};

class DataSetWriter::UniformDataSetWriter : public DataSetWriter::GenericWriter
{
  using UniformCoordType = vtkm::cont::ArrayHandleUniformPointCoordinates;
  using UniformCellType = vtkm::cont::CellSetStructured<3>;

public:
  UniformDataSetWriter(const vtkm::cont::PartitionedDataSet& dataSets,
                       const std::string& fname,
                       const std::string& outputMode,
                       const bool& appendMode = false)
    : GenericWriter(dataSets, fname, outputMode, appendMode)
  {
  }

  void DefineDataModelVariables() override
  {
    std::vector<std::size_t> shape = { 3 * this->TotalNumberOfDataSets };
    std::vector<std::size_t> offset = { 3 * this->DataSetOffset };
    std::vector<std::size_t> size = { 3 * this->NumberOfDataSets };


    this->DimsVar = this->IO.DefineVariable<std::size_t>("dims", shape, offset, size);
    this->OriginsVar = this->IO.DefineVariable<double>("origin", shape, offset, size);
    this->SpacingsVar = this->IO.DefineVariable<double>("spacing", shape, offset, size);
  }

  void WriteCoordinates() override
  {
    this->DimsValues.clear();
    this->OriginsValues.clear();
    this->SpacingsValues.clear();
    this->DimsValues.resize(static_cast<size_t>(this->DataSets.GetNumberOfPartitions() * 3));
    this->OriginsValues.resize(static_cast<size_t>(this->DataSets.GetNumberOfPartitions() * 3));
    this->SpacingsValues.resize(static_cast<size_t>(this->DataSets.GetNumberOfPartitions() * 3));

    std::vector<std::size_t> shape = { 3 * this->TotalNumberOfDataSets };
    this->DimsVar.SetShape(shape);
    this->OriginsVar.SetShape(shape);
    this->SpacingsVar.SetShape(shape);

    for (std::size_t i = 0; i < static_cast<size_t>(this->DataSets.GetNumberOfPartitions()); i++)
    {
      const auto& ds = this->DataSets.GetPartition(static_cast<vtkm::Id>(i));
      const auto& ucoords = ds.GetCoordinateSystem().GetData().AsArrayHandle<UniformCoordType>();
      auto origin = ucoords.ReadPortal().GetOrigin();
      auto spacing = ucoords.ReadPortal().GetSpacing();
      const auto& cellSet = ds.GetCellSet().AsCellSet<UniformCellType>();
      auto dim = cellSet.GetPointDimensions();

      for (int j = 0; j < 3; j++)
      {
        this->DimsValues[i * 3 + static_cast<size_t>(j)] = static_cast<std::size_t>(dim[j]);
        this->OriginsValues[i * 3 + static_cast<size_t>(j)] = origin[j];
        this->SpacingsValues[i * 3 + static_cast<size_t>(j)] = spacing[j];
      }

      adios2::Box<adios2::Dims> sel({ i * 3 + (3 * this->DataSetOffset) }, { 3 });
      this->DimsVar.SetSelection(sel);
      this->OriginsVar.SetSelection(sel);
      this->SpacingsVar.SetSelection(sel);
      this->Engine.Put<std::size_t>(this->DimsVar, &this->DimsValues[i * 3]);
      this->Engine.Put<double>(this->OriginsVar, &this->OriginsValues[i * 3]);
      this->Engine.Put<double>(this->SpacingsVar, &this->SpacingsValues[i * 3]);
    }
  }

  // Nothing to do for structured cells
  void WriteCells() override {}

protected:
  // Nothing to do for uniform grids.
  void ComputeDataModelSpecificGlobalBlockInfo() override {}

private:
  adios2::Variable<std::size_t> DimsVar;
  adios2::Variable<double> OriginsVar, SpacingsVar;
  std::vector<std::size_t> DimsValues;
  std::vector<double> OriginsValues, SpacingsValues;
};

class DataSetWriter::RectilinearDataSetWriter : public DataSetWriter::GenericWriter
{
  using RectCoordType =
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>;
  using RectCellType = vtkm::cont::CellSetStructured<3>;

public:
  RectilinearDataSetWriter(const vtkm::cont::PartitionedDataSet& dataSets,
                           const std::string& fname,
                           const std::string& outputMode,
                           const bool& appendMode = false)
    : GenericWriter(dataSets, fname, outputMode, appendMode)
  {
  }

  void DefineDataModelVariables() override
  {
    std::vector<std::size_t> shape, offset, size;

    shape = { 3 * this->TotalNumberOfDataSets };
    offset = { 3 * this->DataSetOffset };
    size = { 3 * this->NumberOfDataSets };

    this->DimsVar = this->IO.DefineVariable<std::size_t>("dims", shape, offset, size);

    shape = { this->TotalNumberOfXCoords };
    offset = { this->XCoordsOffset };
    size = { this->NumXCoords };
    this->XCoordsVar = this->IO.DefineVariable<vtkm::FloatDefault>("x_array", shape, offset, size);

    shape = { this->TotalNumberOfYCoords };
    offset = { this->YCoordsOffset };
    size = { this->NumYCoords };
    this->YCoordsVar = this->IO.DefineVariable<vtkm::FloatDefault>("y_array", shape, offset, size);

    shape = { this->TotalNumberOfZCoords };
    offset = { this->ZCoordsOffset };
    size = { this->NumZCoords };
    this->ZCoordsVar = this->IO.DefineVariable<vtkm::FloatDefault>("z_array", shape, offset, size);
  }

  void WriteCoordinates() override
  {
    std::size_t xcOffset = this->XCoordsOffset;
    std::size_t ycOffset = this->YCoordsOffset;
    std::size_t zcOffset = this->ZCoordsOffset;

    this->DimsValues.clear();
    this->DimsValues.resize(this->DataSets.GetNumberOfPartitions() * 3);

    this->XCoordsVar.SetShape({ this->TotalNumberOfXCoords });
    this->YCoordsVar.SetShape({ this->TotalNumberOfYCoords });
    this->ZCoordsVar.SetShape({ this->TotalNumberOfZCoords });
    this->DimsVar.SetShape({ 3 * this->TotalNumberOfDataSets });

    for (vtkm::Id i = 0; i < this->DataSets.GetNumberOfPartitions(); i++)
    {
      const auto& ds = this->DataSets.GetPartition(i);
      const auto& coords = ds.GetCoordinateSystem().GetData().AsArrayHandle<RectCoordType>();

      vtkm::cont::ArrayHandleBasic<vtkm::FloatDefault> xc(coords.GetFirstArray());
      vtkm::cont::ArrayHandleBasic<vtkm::FloatDefault> yc(coords.GetSecondArray());
      vtkm::cont::ArrayHandleBasic<vtkm::FloatDefault> zc(coords.GetThirdArray());
      std::size_t numXc = static_cast<std::size_t>(xc.GetNumberOfValues());
      std::size_t numYc = static_cast<std::size_t>(yc.GetNumberOfValues());
      std::size_t numZc = static_cast<std::size_t>(zc.GetNumberOfValues());

      const vtkm::FloatDefault* xBuff = xc.GetReadPointer();
      const vtkm::FloatDefault* yBuff = yc.GetReadPointer();
      const vtkm::FloatDefault* zBuff = zc.GetReadPointer();

      adios2::Box<adios2::Dims> xSel({ xcOffset }, { numXc });
      adios2::Box<adios2::Dims> ySel({ ycOffset }, { numYc });
      adios2::Box<adios2::Dims> zSel({ zcOffset }, { numZc });

      this->XCoordsVar.SetSelection(xSel);
      this->YCoordsVar.SetSelection(ySel);
      this->ZCoordsVar.SetSelection(zSel);

      this->Engine.Put<vtkm::FloatDefault>(this->XCoordsVar, xBuff);
      this->Engine.Put<vtkm::FloatDefault>(this->YCoordsVar, yBuff);
      this->Engine.Put<vtkm::FloatDefault>(this->ZCoordsVar, zBuff);

      adios2::Box<adios2::Dims> sel({ i * 3 + (3 * this->DataSetOffset) }, { 3 });

      this->DimsVar.SetSelection(sel);
      this->DimsValues[i * 3 + 0] = numXc;
      this->DimsValues[i * 3 + 1] = numYc;
      this->DimsValues[i * 3 + 2] = numZc;
      this->Engine.Put<std::size_t>(this->DimsVar, &this->DimsValues[i * 3]);

      xcOffset += numXc;
      ycOffset += numYc;
      zcOffset += numZc;
    }
  }

  void WriteCells() override {}

protected:
  void ComputeDataModelSpecificGlobalBlockInfo() override
  {
    std::size_t numDS = static_cast<std::size_t>(this->DataSets.GetNumberOfPartitions());

    std::vector<int> numCoordinates(this->NumRanks * 3, 0);

    this->NumXCoords = 0;
    this->NumYCoords = 0;
    this->NumZCoords = 0;
    for (std::size_t i = 0; i < numDS; i++)
    {
      const auto& ds = this->DataSets.GetPartition(i);
      const auto& coords = ds.GetCoordinateSystem().GetData().AsArrayHandle<RectCoordType>();

      auto coordsPortal = coords.ReadPortal();
      this->NumXCoords += coordsPortal.GetFirstPortal().GetNumberOfValues();
      this->NumYCoords += coordsPortal.GetSecondPortal().GetNumberOfValues();
      this->NumZCoords += coordsPortal.GetThirdPortal().GetNumberOfValues();
    }
    numCoordinates[this->Rank * 3 + 0] = this->NumXCoords;
    numCoordinates[this->Rank * 3 + 1] = this->NumYCoords;
    numCoordinates[this->Rank * 3 + 2] = this->NumZCoords;

#ifdef FIDES_USE_MPI
    MPI_Allreduce(
      MPI_IN_PLACE, numCoordinates.data(), this->NumRanks * 3, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
#endif
    this->TotalNumberOfXCoords = 0;
    this->TotalNumberOfYCoords = 0;
    this->TotalNumberOfZCoords = 0;
    for (int i = 0; i < this->NumRanks; i++)
    {
      this->TotalNumberOfXCoords += numCoordinates[i * 3 + 0];
      this->TotalNumberOfYCoords += numCoordinates[i * 3 + 1];
      this->TotalNumberOfZCoords += numCoordinates[i * 3 + 2];
    }
    this->XCoordsOffset = 0;
    this->YCoordsOffset = 0;
    this->ZCoordsOffset = 0;
    for (int i = 0; i < this->Rank; i++)
    {
      this->XCoordsOffset += numCoordinates[i * 3 + 0];
      this->YCoordsOffset += numCoordinates[i * 3 + 1];
      this->ZCoordsOffset += numCoordinates[i * 3 + 2];
    }
  }

private:
  std::vector<std::size_t> DimsValues;
  adios2::Variable<vtkm::FloatDefault> XCoordsVar, YCoordsVar, ZCoordsVar;
  adios2::Variable<std::size_t> DimsVar;
  std::size_t TotalNumberOfXCoords = 0;
  std::size_t TotalNumberOfYCoords = 0;
  std::size_t TotalNumberOfZCoords = 0;
  std::size_t NumXCoords = 0;
  std::size_t NumYCoords = 0;
  std::size_t NumZCoords = 0;
  std::size_t XCoordsOffset = 0;
  std::size_t YCoordsOffset = 0;
  std::size_t ZCoordsOffset = 0;
};

class DataSetWriter::UnstructuredSingleTypeDataSetWriter : public DataSetWriter::GenericWriter
{
public:
  UnstructuredSingleTypeDataSetWriter(const vtkm::cont::PartitionedDataSet& dataSets,
                                      const std::string& fname,
                                      const std::string& outputMode,
                                      const bool& appendMode = false)
    : GenericWriter(dataSets, fname, outputMode, appendMode)
  {
  }

  void DefineDataModelVariables() override
  {
    std::vector<std::size_t> shape, offset, size;

    // TotalNumberOfCoords = 3*numpoints; but summed over all datasets you have
    // on your rank.
    shape = { this->TotalNumberOfCoords, 3 };
    offset = { this->CoordOffset, 0 };
    size = { this->NumCoords, 3 };
    const auto& coords = this->DataSets.GetPartition(0).GetCoordinateSystem().GetData();
    coords.CastAndCall(DefineVariableFunctor{}, shape, offset, size, this->IO, "coordinates");

    shape = { this->TotalNumberOfConnIds };
    offset = { this->CellConnOffset };
    size = { this->NumCells * this->NumPointsInCell };
    const auto& cells = this->DataSets.GetPartition(0).GetCellSet();
    cells.template CastAndCallForTypes<CellSetSingleTypeList>(
      DefineCellsVariableFunctor{}, shape, offset, size, this->IO, "connectivity");
  }

  void WriteCoordinates() override
  {
    std::size_t cOffset = this->CoordOffset;

    for (vtkm::Id i = 0; i < this->DataSets.GetNumberOfPartitions(); i++)
    {
      const auto& ds = this->DataSets.GetPartition(i);
      const auto& coords = ds.GetCoordinateSystem().GetData();
      coords.CastAndCall(
        WriteExplicitCoordsFunctor{}, this->IO, this->Engine, cOffset, this->TotalNumberOfCoords);
    }
  }

  void WriteCells() override
  {
    std::size_t offset = this->CellConnOffset;

    for (vtkm::Id i = 0; i < this->DataSets.GetNumberOfPartitions(); i++)
    {
      const auto& ds = this->DataSets.GetPartition(i);
      ds.GetCellSet().template CastAndCallForTypes<CellSetSingleTypeList>(
        WriteSingleTypeCellsFunctor{}, this->IO, this->Engine, offset, this->TotalNumberOfConnIds);
    }
  }

protected:
  void ComputeDataModelSpecificGlobalBlockInfo() override
  {
    std::size_t numDS = static_cast<std::size_t>(this->DataSets.GetNumberOfPartitions());
    std::vector<int> numCoordinates(this->NumRanks, 0);
    std::vector<int> numCells(this->NumRanks, 0);
    std::vector<int> numPtsInCell(this->NumRanks, 0);
    std::vector<int> cellShape(this->NumRanks, 0);

    this->NumCoords = 0;
    this->NumPointsInCell = 0;
    this->CellShape = -1;
    this->TotalNumberOfCoords = 0;
    this->TotalNumberOfCells = 0;

    for (std::size_t i = 0; i < numDS; i++)
    {
      const auto& ds = this->DataSets.GetPartition(i);
      this->NumCoords += ds.GetCoordinateSystem().GetNumberOfPoints();

      const auto& cellSet = ds.GetCellSet();
      this->NumCells += cellSet.GetNumberOfCells();
      if (i == 0)
      {
        this->NumPointsInCell = cellSet.GetNumberOfPointsInCell(0);
        this->CellShape = cellSet.GetCellShape(0);
      }
      else
      {
        if (static_cast<size_t>(cellSet.GetNumberOfPointsInCell(0)) != this->NumPointsInCell)
        {
          throw std::runtime_error("Number of points in cell for "
                                   "CellSetSingleType is not consistent.");
        }
        if (cellSet.GetCellShape(0) != this->CellShape)
        {
          throw std::runtime_error("Cell shape for CellSetSingleType is not consistent. 00");
        }
      }
    }

    numCoordinates[this->Rank] = this->NumCoords;
    numCells[this->Rank] = this->NumCells;
    numPtsInCell[this->Rank] = this->NumPointsInCell;
    cellShape[this->Rank] = static_cast<int>(this->CellShape);

#ifdef FIDES_USE_MPI
    MPI_Allreduce(
      MPI_IN_PLACE, numCoordinates.data(), this->NumRanks, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, numCells.data(), this->NumRanks, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(
      MPI_IN_PLACE, numPtsInCell.data(), this->NumRanks, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, cellShape.data(), this->NumRanks, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
#endif

    for (int i = 0; i < this->NumRanks; i++)
    {
      this->TotalNumberOfCoords += numCoordinates[i];
      this->TotalNumberOfCells += numCells[i];

      if (numCells[i] > 0 && this->NumCells > 0) // if there are cells, they must be consistent.
      {
        if (static_cast<size_t>(numPtsInCell[i]) != this->NumPointsInCell)
        {
          throw std::runtime_error("Number of points in cell for "
                                   "CellSetSingleType is not consistent.");
        }
        if (cellShape[i] != this->CellShape)
        {
          throw std::runtime_error("Cell shape for CellSetSingleType is not consistent.");
        }
      }
    }
    this->TotalNumberOfConnIds = this->TotalNumberOfCells * this->NumPointsInCell;

    this->CoordOffset = 0;
    this->CellConnOffset = 0;
    for (int i = 0; i < this->Rank; i++)
    {
      this->CoordOffset += numCoordinates[i];
      this->CellConnOffset += (numCells[i] * this->NumPointsInCell);
    }
  }

private:
  size_t NumCoords = 0;
  size_t TotalNumberOfCoords = 0;
  size_t NumCells = 0;
  size_t TotalNumberOfCells = 0;
  size_t TotalNumberOfConnIds = 0;
  size_t NumPointsInCell = 0;
  vtkm::Id CellShape = 0;
  size_t CoordOffset = 0;
  size_t CellConnOffset = 0;
};

class DataSetWriter::UnstructuredExplicitDataSetWriter : public DataSetWriter::GenericWriter
{
public:
  UnstructuredExplicitDataSetWriter(const vtkm::cont::PartitionedDataSet& dataSets,
                                    const std::string& fname,
                                    const std::string& outputMode,
                                    const bool& appendMode = false)
    : GenericWriter(dataSets, fname, outputMode, appendMode)
  {
    // Validate that every partition has the same type:
    for (auto const& ds : dataSets)
    {
      bool isType;
      ds.GetCellSet().CastAndCallForTypes<CellSetExplicitList>(CheckCellSetExplicitTypeFunctor{},
                                                               isType);
      if (!isType)
      {
        std::string err = std::string(__FILE__) + ":" + std::to_string(__LINE__);
        err += ": The CellSet of each partition of the PartitionedDataSet is "
               "constrained to be have the type CellSetExplicit.";
        throw std::runtime_error(err);
      }
    }
  }

  void DefineDataModelVariables() override
  {
    // The total number of points in all partitions.
    // The mental model should be that each partition is a piece of a larger
    // geometry.
    std::vector<std::size_t> shape = { static_cast<size_t>(this->TotalNumberOfCoords), 3 };
    const auto& coords = this->DataSets.GetPartition(0).GetCoordinateSystem().GetData();
    coords.CastAndCall(
      DefineVariableFunctor{}, shape, adios2::Dims(), adios2::Dims(), this->IO, "coordinates");

    // Now the shapes array.
    this->ShapesVar = this->IO.DefineVariable<uint8_t>(
      "cell_types", { static_cast<size_t>(this->TotalNumberOfCells) });
    // VTK-m stores offsets, but Fides stores the number of vertices/cell.
    this->VertsVar = this->IO.DefineVariable<vtkm::IdComponent>(
      "num_verts", { static_cast<size_t>(this->TotalNumberOfCells) });

    shape = { static_cast<size_t>(this->TotalNumberOfConns) };
    const auto& cells = this->DataSets.GetPartition(0).GetCellSet();
    cells.template CastAndCallForTypes<CellSetExplicitList>(DefineCellsVariableFunctor{},
                                                            shape,
                                                            adios2::Dims(),
                                                            adios2::Dims(),
                                                            this->IO,
                                                            "connectivity");
  }

  void WriteCoordinates() override
  {
    std::size_t cOffset = this->CoordOffset;

    for (vtkm::Id i = 0; i < this->DataSets.GetNumberOfPartitions(); i++)
    {
      const auto& ds = this->DataSets.GetPartition(i);
      const auto& coords = ds.GetCoordinateSystem().GetData();
      coords.CastAndCall(
        WriteExplicitCoordsFunctor{}, this->IO, this->Engine, cOffset, this->TotalNumberOfCoords);
    }
  }

  void WriteCells() override
  {
    this->NumVerts.clear();
    this->NumVerts.resize(static_cast<size_t>(this->NumCells), -1);

    //Update the shape size for this step.
    this->ShapesVar.SetShape({ this->TotalNumberOfCells });
    this->VertsVar.SetShape({ this->TotalNumberOfCells });

    size_t cellOffset = this->CellOffset;
    size_t connOffset = this->ConnOffset;
    size_t numVertsOffset = 0;
    for (auto const& ds : this->DataSets)
    {
      const vtkm::cont::UnknownCellSet& dCellSet = ds.GetCellSet();
      dCellSet.CastAndCallForTypes<CellSetExplicitList>(WriteExplicitCellsFunctor{},
                                                        cellOffset,
                                                        connOffset,
                                                        this->NumVerts,
                                                        numVertsOffset,
                                                        this->TotalNumberOfConns,
                                                        this->Engine,
                                                        this->IO);
    }
  }

protected:
  void ComputeDataModelSpecificGlobalBlockInfo() override
  {
    std::vector<int> numCoordinates(this->NumRanks, 0);
    std::vector<int> numCells(this->NumRanks, 0);
    std::vector<int> cellShape(this->NumRanks, 0);
    std::vector<int> numConns(this->NumRanks, 0);

    this->NumCoords = 0;
    this->NumCells = 0;
    this->TotalNumberOfCoords = 0;
    this->TotalNumberOfCells = 0;
    for (const auto& ds : this->DataSets)
    {
      const auto& coords = ds.GetCoordinateSystem().GetData();
      this->NumCoords += coords.GetNumberOfValues();

      const auto& cellSet = ds.GetCellSet();
      this->NumCells += cellSet.GetNumberOfCells();
    }

    this->NumConns = 0;
    this->TotalNumberOfConns = 0;
    for (auto const& ds : DataSets)
    {
      auto const& dCellSet = ds.GetCellSet();
      dCellSet.CastAndCallForTypes<CellSetExplicitList>(ComputeNumConnsFunctor{}, this->NumConns);
    }

    numCoordinates[this->Rank] = this->NumCoords;
    numCells[this->Rank] = this->NumCells;
    numConns[this->Rank] = this->NumConns;
#ifdef FIDES_USE_MPI
    MPI_Allreduce(
      MPI_IN_PLACE, numCoordinates.data(), this->NumRanks, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, numCells.data(), this->NumRanks, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, numConns.data(), this->NumRanks, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
#endif

    for (int i = 0; i < this->NumRanks; i++)
    {
      this->TotalNumberOfCoords += numCoordinates[i];
      this->TotalNumberOfCells += numCells[i];
      this->TotalNumberOfConns += numConns[i];
    }

    this->CoordOffset = 0;
    this->CellOffset = 0;
    this->ConnOffset = 0;
    for (int i = 0; i < this->Rank; i++)
    {
      this->CoordOffset += numCoordinates[i];
      this->CellOffset += numCells[i];
      this->ConnOffset += numConns[i];
    }
  }

private:
  adios2::Variable<uint8_t> ShapesVar;
  adios2::Variable<vtkm::IdComponent> VertsVar;
  vtkm::Id NumCoords = 0;
  vtkm::Id NumCells = 0;
  vtkm::Id CoordOffset = 0;
  vtkm::Id TotalNumberOfCoords = 0;
  vtkm::Id CellOffset = 0;
  vtkm::Id NumConns = 0;
  vtkm::Id ConnOffset = 0;
  vtkm::Id TotalNumberOfConns = 0;
  std::vector<vtkm::IdComponent> NumVerts;
};

DataSetWriter::DataSetWriter(const std::string& outputFile)
  : OutputFile(outputFile)
  , WriteFieldSet(false)
{
}

unsigned char DataSetWriter::GetDataSetType(const vtkm::cont::DataSet& ds)
{
  using UniformCoordType = vtkm::cont::ArrayHandleUniformPointCoordinates;
  using RectilinearCoordType =
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>;

  const vtkm::cont::CoordinateSystem& coords = ds.GetCoordinateSystem();
  const vtkm::cont::UnknownCellSet& cellSet = ds.GetCellSet();

  //Check for structred cellset.
  if (cellSet.IsType<vtkm::cont::CellSetStructured<1>>() ||
      cellSet.IsType<vtkm::cont::CellSetStructured<2>>() ||
      cellSet.IsType<vtkm::cont::CellSetStructured<3>>())
  {
    if (coords.GetData().IsType<UniformCoordType>())
    {
      return DATASET_TYPE_UNIFORM;
    }
    else if (coords.GetData().IsType<RectilinearCoordType>())
    {
      return DATASET_TYPE_RECTILINEAR;
    }
    else
    {
      return DATASET_TYPE_ERROR;
    }
  }
  else
  {
    vtkm::cont::UncertainCellSet<FullCellSetExplicitList> uncertainCS(ds.GetCellSet());
    unsigned char type;
    uncertainCS.CastAndCall(GetDataSetTypeFunctor{}, type, *this);
    return type;
  }
}

void DataSetWriter::SetDataSetType(const vtkm::cont::PartitionedDataSet& dataSets)
{
  int rank = 0, numRanks = 1;
#ifdef FIDES_USE_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numRanks);
#endif

  //Make sure all the datasets are the same.
  std::vector<unsigned char> myDataSetTypes;
  for (const auto& ds : dataSets)
  {
    myDataSetTypes.push_back(this->GetDataSetType(ds));
  }

  unsigned char dataSetType = this->DATASET_TYPE_NONE;
  for (const auto& v : myDataSetTypes)
  {
    dataSetType |= v;
  }
  // if not equal to one of the valid dataset types, it's an error.
  if (!(dataSetType == DATASET_TYPE_NONE || dataSetType == DATASET_TYPE_UNIFORM ||
        dataSetType == DATASET_TYPE_RECTILINEAR ||
        dataSetType == DATASET_TYPE_UNSTRUCTURED_SINGLE ||
        dataSetType == DATASET_TYPE_UNSTRUCTURED))
  {
    dataSetType = DATASET_TYPE_ERROR;
  }

  std::vector<unsigned char> allDataSetTypes(numRanks, DATASET_TYPE_NONE);
  allDataSetTypes[rank] = dataSetType;
#ifdef FIDES_USE_MPI
  MPI_Allreduce(
    MPI_IN_PLACE, allDataSetTypes.data(), numRanks, MPI_UNSIGNED_CHAR, MPI_BOR, MPI_COMM_WORLD);
#endif

  //If we OR these values all together, we will get the global dataset type.
  //There can be NONE, but all non NONE should be the same. If not, it's an error.
  unsigned char globalDataSetType = DATASET_TYPE_NONE;
  for (const auto& v : allDataSetTypes)
  {
    globalDataSetType |= v;
  }

  if (!(globalDataSetType == DATASET_TYPE_NONE || globalDataSetType == DATASET_TYPE_UNIFORM ||
        globalDataSetType == DATASET_TYPE_RECTILINEAR ||
        globalDataSetType == DATASET_TYPE_UNSTRUCTURED_SINGLE ||
        globalDataSetType == DATASET_TYPE_UNSTRUCTURED))
  {
    globalDataSetType = DATASET_TYPE_ERROR;
  }

  //Set the dataSet type.
  this->DataSetType = globalDataSetType;
}

void DataSetWriter::Write(const vtkm::cont::PartitionedDataSet& dataSets,
                          const std::string& outputMode)
{
  this->SetDataSetType(dataSets);

  if (this->DataSetType == DATASET_TYPE_NONE)
  {
    //Nobody has anything, so just return.
    return;
  }
  else if (this->DataSetType == DATASET_TYPE_UNIFORM)
  {
    UniformDataSetWriter writeImpl(dataSets, this->OutputFile, outputMode);
    if (this->WriteFieldSet)
      writeImpl.SetWriteFields(this->FieldsToWrite);

    writeImpl.Write();
    writeImpl.Close();
  }
  else if (this->DataSetType == DATASET_TYPE_RECTILINEAR)
  {
    RectilinearDataSetWriter writeImpl(dataSets, this->OutputFile, outputMode);
    if (this->WriteFieldSet)
      writeImpl.SetWriteFields(this->FieldsToWrite);
    writeImpl.Write();
    writeImpl.Close();
  }
  else if (this->DataSetType == DATASET_TYPE_UNSTRUCTURED_SINGLE)
  {
    UnstructuredSingleTypeDataSetWriter writeImpl(dataSets, this->OutputFile, outputMode);
    if (this->WriteFieldSet)
      writeImpl.SetWriteFields(this->FieldsToWrite);
    writeImpl.Write();
    writeImpl.Close();
  }
  else if (this->DataSetType == DATASET_TYPE_UNSTRUCTURED)
  {
    UnstructuredExplicitDataSetWriter writeImpl(dataSets, this->OutputFile, outputMode);
    if (this->WriteFieldSet)
      writeImpl.SetWriteFields(this->FieldsToWrite);
    writeImpl.Write();
    writeImpl.Close();
  }
  else
  {
    throw std::runtime_error("Unsupported dataset type");
  }
}

DataSetAppendWriter::DataSetAppendWriter(const std::string& outputFile)
  : DataSetWriter(outputFile)
  , IsInitialized(false)
  , Writer(nullptr)
{
}

void DataSetAppendWriter::Write(const vtkm::cont::PartitionedDataSet& dataSets,
                                const std::string& outputMode)
{
  if (!this->IsInitialized)
    this->Initialize(dataSets, outputMode);

  //Make sure we're being consistent.
  unsigned char dsType = DATASET_TYPE_NONE;
  for (const auto& ds : dataSets)
    dsType |= this->GetDataSetType(ds);

  if (!(dsType == this->DATASET_TYPE_NONE || dsType == this->DataSetType))
  {
    throw std::runtime_error("Unsupported dataset type");
  }

  this->Writer->SetDataSets(dataSets);
  this->Writer->Write();
}

void DataSetAppendWriter::Close()
{
  this->IsInitialized = false;
  this->Writer->Close();
  this->Writer.reset();
}

void DataSetAppendWriter::Initialize(const vtkm::cont::PartitionedDataSet& dataSets,
                                     const std::string& outputMode)
{
  this->SetDataSetType(dataSets);
  if (this->DataSetType == DATASET_TYPE_UNIFORM)
  {
    this->Writer.reset(
      new DataSetWriter::UniformDataSetWriter(dataSets, this->OutputFile, outputMode, true));
  }
  else if (this->DataSetType == DATASET_TYPE_RECTILINEAR)
  {
    this->Writer.reset(
      new DataSetWriter::RectilinearDataSetWriter(dataSets, this->OutputFile, outputMode, true));
  }
  else if (this->DataSetType == DATASET_TYPE_UNSTRUCTURED_SINGLE)
  {
    this->Writer.reset(
      new UnstructuredSingleTypeDataSetWriter(dataSets, this->OutputFile, outputMode, true));
  }
  else if (this->DataSetType == DATASET_TYPE_UNSTRUCTURED)
  {
    this->Writer.reset(
      new UnstructuredExplicitDataSetWriter(dataSets, this->OutputFile, outputMode, true));
  }
  else
  {
    throw std::runtime_error("Unsupported dataset type");
  }

  if (this->WriteFieldSet)
    this->Writer->SetWriteFields(this->FieldsToWrite);

  this->IsInitialized = true;
}


} // end namespace io
} // end namespace fides
