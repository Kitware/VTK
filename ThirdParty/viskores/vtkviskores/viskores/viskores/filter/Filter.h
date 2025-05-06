//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_filter_Filter_h
#define viskores_filter_Filter_h

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/PartitionedDataSet.h>

#include <viskores/filter/FieldSelection.h>
#include <viskores/filter/TaskQueue.h>
#include <viskores/filter/viskores_filter_core_export.h>

namespace viskores
{
namespace filter
{
/// @brief Base class for all filters.
///
/// This is the base class for all filters. To add a new filter, one can subclass this and
/// implement relevant methods.
///
/// **FilterUsage Usage**
///
/// To execute a filter, one typically calls the `auto result = filter.Execute(input)`. Typical
/// usage is as follows:
///
/// ```cpp
/// // create the concrete subclass (e.g. Contour).
/// viskores::filter::contour::Contour contour;
///
/// // select fields to map to the output, if different from default which is to map all input
/// // fields.
/// contour.SetFieldsToPass({"var1", "var2"});
///
/// // execute the filter on viskores::cont::DataSet.
/// viskores::cont::DataSet dsInput = ...
/// auto outputDS = contour.Execute(dsInput);
///
/// // or, execute on a viskores::cont::PartitionedDataSet
/// viskores::cont::PartitionedDataSet mbInput = ...
/// auto outputMB = contour.Execute(mbInput);
/// ```
///
/// `Execute` methods take in the input DataSet or PartitionedDataSet to process and return the
/// result. The type of the result is same as the input type, thus `Execute(DataSet&)` returns
/// a DataSet while `Execute(PartitionedDataSet&)` returns a PartitionedDataSet.
///
/// `Execute` simply calls the pure virtual function `DoExecute(DataSet&)` which is the main
/// extension point of the Filter interface. Filter developer needs to override
/// `DoExecute(DataSet)` to implement the business logic of filtering operations on a single
/// DataSet.
///
/// The default implementation of `Execute(PartitionedDataSet&)` is merely provided for
/// convenience. Internally, it calls `DoExecutePartitions(PartitionedDataSet)` to iterate DataSets
/// of a PartitionedDataSet and pass each individual DataSets to `DoExecute(DataSet&)`,
/// possibly in a multi-threaded setting. Developer of `DoExecute(DataSet&)` needs to indicate
/// the thread-safeness of `DoExecute(DataSet&)` by overriding the `CanThread()` virtual method
/// which by default returns `true`.
///
/// In the case that filtering on a PartitionedDataSet can not be simply implemented as a
/// for-each loop on the component DataSets, filter implementor needs to override the
/// `DoExecutePartitions(PartitionedDataSet&)`. See the implementation of
/// `FilterParticleAdvection::Execute(PartitionedDataSet&)` for an example.
///
/// **Creating results and mapping fields**
///
/// For subclasses that map input fields into output fields, the implementation of its
/// `DoExecute(DataSet&)` should create the `DataSet` to be returned with a call to
/// `Filter::CreateResult` or a similar method (such as
/// `Filter::CreateResultField`).
///
/// ```cpp
/// VISKORES_CONT DataSet SomeFilter::DoExecute(const viskores::cont::DataSet& input)
/// {
///   viskores::cont::UnknownCellSet outCellSet;
///   outCellSet = ... // Generation of the new CellSet
///
///   // Mapper is a callable object (function object, lambda, etc.) that takes an input Field
///   // and maps it to an output Field and then add the output Field to the output DataSet
///   auto mapper = [](auto& outputDs, const auto& inputField) {
///      auto outputField = ... // Business logic for mapping input field to output field
///      output.AddField(outputField);
///   };
///   // This passes coordinate systems directly from input to output. If the points of
///   // the cell set change at all, they will have to be mapped by hand.
///   return this->CreateResult(input, outCellSet, mapper);
/// }
/// ```
///
/// In addition to creating a new `DataSet` filled with the proper cell structure and coordinate
/// systems, `CreateResult` iterates through each `FieldToPass` in the input DataSet and calls the
/// FieldMapper to map the input Field to output Field. For simple filters that just pass on input
/// fields to the output DataSet without any computation, an overload of
/// `CreateResult(const viskores::cont::DataSet& input)` is also
/// provided as a convenience that uses the default mapper which trivially adds input Field to
/// output DataSet (via a shallow copy).
///
/// **FilterThreadSafety CanThread**
///
/// By default, the implementation of `DoExecute(DataSet&)` should model a *pure function*, i.e. it
/// does not have any mutable shared state. This makes it thread-safe by default and allows
/// the default implementation of `DoExecutePartitions(PartitionedDataSet&)` to be simply a parallel
/// for-each, thus facilitates multi-threaded execution without any lock.
///
/// Many legacy (Viskores 1.x) filter implementations needed to store states between the mesh generation
/// phase and field mapping phase of filter execution, for example, parameters for field
/// interpolation. The shared mutable states were mostly stored as mutable data members of the
/// filter class (either in terms of ArrayHandle or some kind of Worket). The new filter interface,
/// by combining the two phases into a single call to `DoExecute(DataSet&)`, we have eliminated most
/// of the cases that require such shared mutable states. New implementations of filters that
/// require passing information between these two phases can now use local variables within the
/// `DoExecute(DataSet&)`. For example:
///
/// ```cpp
/// struct SharedState; // shared states between mesh generation and field mapping.
/// VISKORES_CONT DataSet ThreadSafeFilter::DoExecute(const viskores::cont::DataSet& input)
/// {
///   // Mutable states that was a data member of the filter is now a local variable.
///   // Each invocation of Execute(DataSet) in the multi-threaded execution of
///   // Execute(PartitionedDataSet&) will have a copy of `states` on each thread's stack
///   // thus making it thread-safe.
///   SharedStates states;
///
///   viskores::cont::CellSetExplicit<> cellSet;
///   cellSet = ... // Generation of the new DataSet and store interpolation parameters in `states`
///
///   // Lambda capture of `states`, effectively passing the shared states to the Mapper.
///   auto mapper = [&states](auto& outputDs, const auto& inputField) {
///      auto outputField = ... // Use `states` for mapping input field to output field
///      output.AddField(outputField);
///   };
///   this->CreateOutput(input, cellSet, mapper);
///
///   return output;
/// }
/// ```
///
/// In the rare cases that filter implementation can not be made thread-safe, the implementation
/// needs to override the `CanThread()` virtual method to return `false`. The default
/// `Execute(PartitionedDataSet&)` implementation will fallback to a serial for loop execution.
///
/// _FilterThreadScheduling DoExecute_
///
/// The default multi-threaded execution of `Execute(PartitionedDataSet&)` uses a simple FIFO queue
/// of DataSet and pool of *worker* threads. Implementation of Filter subclass can override the
/// `DoExecutePartitions(PartitionedDataSet)` virtual method to provide implementation specific
/// scheduling policy. The default number of *worker* threads in the pool are determined by the
/// `DetermineNumberOfThreads()` virtual method using several backend dependent heuristic.
/// Implementations of Filter subclass can also override
/// `DetermineNumberOfThreads()` to provide implementation specific heuristic.
///
class VISKORES_FILTER_CORE_EXPORT Filter
{
public:
  VISKORES_CONT Filter();

  VISKORES_CONT virtual ~Filter();

  /// @brief Executes the filter on the input and produces a result dataset.
  ///
  /// On success, this the dataset produced. On error, `viskores::cont::ErrorExecution` will be thrown.
  VISKORES_CONT viskores::cont::DataSet Execute(const viskores::cont::DataSet& input);

  /// @brief Executes the filter on the input PartitionedDataSet and produces a result PartitionedDataSet.
  ///
  /// On success, this the dataset produced. On error, `viskores::cont::ErrorExecution` will be thrown.
  VISKORES_CONT viskores::cont::PartitionedDataSet Execute(
    const viskores::cont::PartitionedDataSet& input);

  /// @brief Specify which fields get passed from input to output.
  ///
  /// After a filter successfully executes and returns a new data set, fields are mapped from
  /// input to output. Depending on what operation the filter does, this could be a simple shallow
  /// copy of an array, or it could be a computed operation. You can control which fields are
  /// passed (and equivalently which are not) with this parameter.
  ///
  /// By default, all fields are passed during execution.
  ///
  VISKORES_CONT void SetFieldsToPass(const viskores::filter::FieldSelection& fieldsToPass);
  /// @copydoc SetFieldsToPass
  VISKORES_CONT void SetFieldsToPass(viskores::filter::FieldSelection&& fieldsToPass);

  VISKORES_DEPRECATED(2.0)
  VISKORES_CONT void SetFieldsToPass(const viskores::filter::FieldSelection& fieldsToPass,
                                     viskores::filter::FieldSelection::Mode mode);

  /// @copydoc SetFieldsToPass
  VISKORES_CONT void SetFieldsToPass(
    std::initializer_list<std::string> fields,
    viskores::filter::FieldSelection::Mode mode = viskores::filter::FieldSelection::Mode::Select);

  /// @copydoc SetFieldsToPass
  VISKORES_CONT void SetFieldsToPass(
    std::initializer_list<std::pair<std::string, viskores::cont::Field::Association>> fields,
    viskores::filter::FieldSelection::Mode mode = viskores::filter::FieldSelection::Mode::Select);


  /// @copydoc SetFieldsToPass
  VISKORES_CONT void SetFieldsToPass(
    const std::string& fieldname,
    viskores::cont::Field::Association association,
    viskores::filter::FieldSelection::Mode mode = viskores::filter::FieldSelection::Mode::Select);

  /// @copydoc SetFieldsToPass
  VISKORES_CONT void SetFieldsToPass(const std::string& fieldname,
                                     viskores::filter::FieldSelection::Mode mode)
  {
    this->SetFieldsToPass(fieldname, viskores::cont::Field::Association::Any, mode);
  }

  /// @copydoc SetFieldsToPass
  VISKORES_CONT
  const viskores::filter::FieldSelection& GetFieldsToPass() const { return this->FieldsToPass; }
  /// @copydoc SetFieldsToPass
  VISKORES_CONT
  viskores::filter::FieldSelection& GetFieldsToPass() { return this->FieldsToPass; }

  /// @brief Specify whether to always pass coordinate systems.
  ///
  /// `viskores::cont::CoordinateSystem`s in a `DataSet` are really just point fields marked as being a
  /// coordinate system. Thus, a coordinate system is passed if and only if the associated
  /// field is passed.
  ///
  /// By default, the filter will pass all fields associated with a coordinate system
  /// regardless of the `FieldsToPass` marks the field as passing. If this option is set
  /// to `false`, then coordinate systems will only be passed if it is marked so by
  /// `FieldsToPass`.
  VISKORES_CONT void SetPassCoordinateSystems(bool flag) { this->PassCoordinateSystems = flag; }
  /// @copydoc SetPassCoordinateSystems
  VISKORES_CONT bool GetPassCoordinateSystems() const { return this->PassCoordinateSystems; }

  /// @brief Specifies the name of the output field generated.
  ///
  /// Not all filters create an output field.
  VISKORES_CONT void SetOutputFieldName(const std::string& name) { this->OutputFieldName = name; }

  /// @copydoc SetOutputFieldName
  VISKORES_CONT const std::string& GetOutputFieldName() const { return this->OutputFieldName; }

  /// @brief Specifies a field to operate on.
  ///
  /// The number of input fields (or whether the filter operates on input fields at all)
  /// is specific to each particular filter.
  VISKORES_CONT void SetActiveField(
    const std::string& name,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any)
  {
    this->SetActiveField(0, name, association);
  }

  /// @copydoc SetActiveField
  VISKORES_CONT void SetActiveField(
    viskores::IdComponent index,
    const std::string& name,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any)
  {
    auto index_st = static_cast<std::size_t>(index);
    this->ResizeIfNeeded(index_st);
    this->ActiveFieldNames[index_st] = name;
    this->ActiveFieldAssociation[index_st] = association;
  }

  /// @copydoc SetActiveField
  VISKORES_CONT const std::string& GetActiveFieldName(viskores::IdComponent index = 0) const
  {
    VISKORES_ASSERT((index >= 0) &&
                    (index < static_cast<viskores::IdComponent>(this->ActiveFieldNames.size())));
    return this->ActiveFieldNames[index];
  }

  /// @copydoc SetActiveField
  VISKORES_CONT viskores::cont::Field::Association GetActiveFieldAssociation(
    viskores::IdComponent index = 0) const
  {
    return this->ActiveFieldAssociation[index];
  }

  /// Specifies the coordinate system index to make active to use when processing the input
  /// `viskores::cont::DataSet`. This is used primarily by the Filter to select the
  /// coordinate system to use as a field when `UseCoordinateSystemAsField` is true.
  VISKORES_CONT void SetActiveCoordinateSystem(viskores::Id coord_idx)
  {
    this->SetActiveCoordinateSystem(0, coord_idx);
  }

  /// @copydoc SetActiveCoordinateSystem
  VISKORES_CONT void SetActiveCoordinateSystem(viskores::IdComponent index, viskores::Id coord_idx)
  {
    auto index_st = static_cast<std::size_t>(index);
    this->ResizeIfNeeded(index_st);
    this->ActiveCoordinateSystemIndices[index_st] = coord_idx;
  }

  /// @copydoc SetActiveCoordinateSystem
  VISKORES_CONT viskores::Id GetActiveCoordinateSystemIndex(viskores::IdComponent index = 0) const
  {
    auto index_st = static_cast<std::size_t>(index);
    return this->ActiveCoordinateSystemIndices[index_st];
  }

  /// Specifies whether to use point coordinates as the input field. When true, the values
  /// for the active field are ignored and the active coordinate system is used instead.
  VISKORES_CONT void SetUseCoordinateSystemAsField(bool val)
  {
    SetUseCoordinateSystemAsField(0, val);
  }

  /// @copydoc SetUseCoordinateSystemAsField
  VISKORES_CONT void SetUseCoordinateSystemAsField(viskores::IdComponent index, bool val)
  {
    auto index_st = static_cast<std::size_t>(index);
    this->ResizeIfNeeded(index_st);
    this->UseCoordinateSystemAsField[index] = val;
  }

  /// @copydoc SetUseCoordinateSystemAsField
  VISKORES_CONT
  bool GetUseCoordinateSystemAsField(viskores::IdComponent index = 0) const
  {
    VISKORES_ASSERT((index >= 0) &&
                    (index < static_cast<viskores::IdComponent>(this->ActiveFieldNames.size())));
    return this->UseCoordinateSystemAsField[index];
  }

  /// @brief Return the number of active fields currently set.
  ///
  /// The general interface to `Filter` allows a user to set an arbitrary number
  /// of active fields (indexed 0 and on). This method returns the number of active
  /// fields that are set. Note that the filter implementation is free to ignore
  /// any active fields it does not support. Also note that an active field can be
  /// set to be either a named field or a coordinate system.
  viskores::IdComponent GetNumberOfActiveFields() const
  {
    VISKORES_ASSERT(this->ActiveFieldNames.size() == this->UseCoordinateSystemAsField.size());
    return static_cast<viskores::IdComponent>(this->UseCoordinateSystemAsField.size());
  }

  /// @brief Returns whether the filter can execute on partitions in concurrent threads.
  ///
  /// If a derived class's implementation of `DoExecute` cannot run on multiple threads,
  /// then the derived class should override this method to return false.
  VISKORES_CONT virtual bool CanThread() const;

  VISKORES_CONT void SetThreadsPerCPU(viskores::Id numThreads)
  {
    this->NumThreadsPerCPU = numThreads;
  }
  VISKORES_CONT void SetThreadsPerGPU(viskores::Id numThreads)
  {
    this->NumThreadsPerGPU = numThreads;
  }

  VISKORES_CONT viskores::Id GetThreadsPerCPU() const { return this->NumThreadsPerCPU; }
  VISKORES_CONT viskores::Id GetThreadsPerGPU() const { return this->NumThreadsPerGPU; }

  VISKORES_CONT bool GetRunMultiThreadedFilter() const
  {
    return this->CanThread() && this->RunFilterWithMultipleThreads;
  }

  VISKORES_CONT void SetRunMultiThreadedFilter(bool val)
  {
    if (this->CanThread())
      this->RunFilterWithMultipleThreads = val;
    else
    {
      std::string msg =
        "Multi threaded filter not supported for " + std::string(typeid(*this).name());
      VISKORES_LOG_S(viskores::cont::LogLevel::Info, msg);
    }
  }

  // FIXME: Is this actually materialize? Are there different kinds of Invoker?
  /// Specify the viskores::cont::Invoker to be used to execute worklets by
  /// this filter instance. Overriding the default allows callers to control
  /// which device adapters a filter uses.
  void SetInvoker(viskores::cont::Invoker inv) { this->Invoke = inv; }

protected:
  viskores::cont::Invoker Invoke;

  /// @brief Create the output data set for `DoExecute`.
  ///
  /// This form of `CreateResult` will create an output data set with the same cell
  /// structure and coordinate system as the input and pass all fields (as requested
  /// by the `Filter` state).
  ///
  /// @param[in] inDataSet The input data set being modified (usually the one passed into
  /// `DoExecute`). The returned `DataSet` is filled with the cell set, coordinate system, and
  /// fields of `inDataSet` (as selected by the `FieldsToPass` state of the filter).
  ///
  VISKORES_CONT viskores::cont::DataSet CreateResult(
    const viskores::cont::DataSet& inDataSet) const;

  /// @brief Create the output data set for `DoExecute`
  ///
  /// This form of `CreateResult` will create an output data set with the same cell
  /// structure and coordinate system as the input and pass all fields (as requested
  /// by the `Filter` state). Additionally, it will add the provided field to the
  /// result.
  ///
  /// @param[in] inDataSet The input data set being modified (usually the one passed
  ///     into `DoExecute`). The returned `DataSet` is filled with fields of `inDataSet`
  ///     (as selected by the `FieldsToPass` state of the filter).
  /// @param[in] resultField A `Field` that is added to the returned `DataSet`.
  ///
  VISKORES_CONT viskores::cont::DataSet CreateResultField(
    const viskores::cont::DataSet& inDataSet,
    const viskores::cont::Field& resultField) const;

  /// @brief Create the output data set for `DoExecute`
  ///
  /// This form of `CreateResult` will create an output data set with the same cell
  /// structure and coordinate system as the input and pass all fields (as requested
  /// by the `Filter` state). Additionally, it will add a field matching the provided
  /// specifications to the result.
  ///
  /// @param[in] inDataSet The input data set being modified (usually the one passed
  ///     into `DoExecute`). The returned `DataSet` is filled with fields of `inDataSet`
  ///     (as selected by the `FieldsToPass` state of the filter).
  /// @param[in] resultFieldName The name of the field added to the returned `DataSet`.
  /// @param[in] resultFieldAssociation The association of the field (e.g. point or cell)
  ///     added to the returned `DataSet`.
  /// @param[in] resultFieldArray An array containing the data for the field added to the
  ///     returned `DataSet`.
  ///
  VISKORES_CONT viskores::cont::DataSet CreateResultField(
    const viskores::cont::DataSet& inDataSet,
    const std::string& resultFieldName,
    viskores::cont::Field::Association resultFieldAssociation,
    const viskores::cont::UnknownArrayHandle& resultFieldArray) const
  {
    return this->CreateResultField(
      inDataSet,
      viskores::cont::Field{ resultFieldName, resultFieldAssociation, resultFieldArray });
  }

  /// @brief Create the output data set for `DoExecute`
  ///
  /// This form of `CreateResult` will create an output data set with the same cell
  /// structure and coordinate system as the input and pass all fields (as requested
  /// by the `Filter` state). Additionally, it will add a point field matching the
  /// provided specifications to the result.
  ///
  /// @param[in] inDataSet The input data set being modified (usually the one passed
  ///     into `DoExecute`). The returned `DataSet` is filled with fields of `inDataSet`
  ///     (as selected by the `FieldsToPass` state of the filter).
  /// @param[in] resultFieldName The name of the field added to the returned `DataSet`.
  /// @param[in] resultFieldArray An array containing the data for the field added to the
  ///     returned `DataSet`.
  ///
  VISKORES_CONT viskores::cont::DataSet CreateResultFieldPoint(
    const viskores::cont::DataSet& inDataSet,
    const std::string& resultFieldName,
    const viskores::cont::UnknownArrayHandle& resultFieldArray) const
  {
    return this->CreateResultField(
      inDataSet,
      viskores::cont::Field{
        resultFieldName, viskores::cont::Field::Association::Points, resultFieldArray });
  }

  /// @brief Create the output data set for `DoExecute`
  ///
  /// This form of `CreateResult` will create an output data set with the same cell
  /// structure and coordinate system as the input and pass all fields (as requested
  /// by the `Filter` state). Additionally, it will add a cell field matching the
  /// provided specifications to the result.
  ///
  /// @param[in] inDataSet The input data set being modified (usually the one passed
  ///     into `DoExecute`). The returned `DataSet` is filled with fields of `inDataSet`
  ///     (as selected by the `FieldsToPass` state of the filter).
  /// @param[in] resultFieldName The name of the field added to the returned `DataSet`.
  /// @param[in] resultFieldArray An array containing the data for the field added to the
  ///     returned `DataSet`.
  ///
  VISKORES_CONT viskores::cont::DataSet CreateResultFieldCell(
    const viskores::cont::DataSet& inDataSet,
    const std::string& resultFieldName,
    const viskores::cont::UnknownArrayHandle& resultFieldArray) const
  {
    return this->CreateResultField(inDataSet,
                                   viskores::cont::Field{ resultFieldName,
                                                          viskores::cont::Field::Association::Cells,
                                                          resultFieldArray });
  }


  /// @brief Create the output data set for `DoExecute`.
  ///
  /// This form of `CreateResult` will create an output PartitionedDataSet with the
  /// same partitions and pass all PartitionedDataSet fields (as requested by the
  /// `Filter` state).
  ///
  /// @param[in] input The input data set being modified (usually the one passed into
  /// `DoExecute`).
  /// @param[in] resultPartitions The output data created by the filter. Fields from the input are
  /// passed onto the return result partition as requested by the `Filter` state.
  ///
  VISKORES_CONT viskores::cont::PartitionedDataSet CreateResult(
    const viskores::cont::PartitionedDataSet& input,
    const viskores::cont::PartitionedDataSet& resultPartitions) const;

  /// @brief Create the output data set for `DoExecute`.
  ///
  /// This form of `CreateResult` will create an output PartitionedDataSet with the
  /// same partitions and pass all PartitionedDataSet fields (as requested by the
  /// `Filter` state).
  ///
  /// @param[in] input The input data set being modified (usually the one passed into
  /// `DoExecute`).
  /// @param[in] resultPartitions The output data created by the filter. Fields from the input are
  /// passed onto the return result partition as requested by the `Filter` state.
  /// @param[in] fieldMapper A function or functor that takes a `PartitionedDataSet` as its first
  ///     argument and a `Field` as its second argument. The `PartitionedDataSet` is the data being
  ///     created and will eventually be returned by `CreateResult`. The `Field` comes from `input`.
  ///
  template <typename FieldMapper>
  VISKORES_CONT viskores::cont::PartitionedDataSet CreateResult(
    const viskores::cont::PartitionedDataSet& input,
    const viskores::cont::PartitionedDataSet& resultPartitions,
    FieldMapper&& fieldMapper) const
  {
    viskores::cont::PartitionedDataSet output(resultPartitions.GetPartitions());
    this->MapFieldsOntoOutput(input, this->GetFieldsToPass(), output, fieldMapper);
    return output;
  }

  /// @brief Create the output data set for `DoExecute`.
  ///
  /// This form of `CreateResult` will create an output data set with the given `CellSet`. You must
  /// also provide a field mapper function, which is a function that takes the output `DataSet`
  /// being created and a `Field` from the input and then applies any necessary transformations to
  /// the field array and adds it to the `DataSet`.
  ///
  /// @param[in] inDataSet The input data set being modified (usually the one passed
  ///     into `DoExecute`). The returned `DataSet` is filled with fields of `inDataSet`
  ///     (as selected by the `FieldsToPass` state of the filter).
  /// @param[in] resultCellSet The `CellSet` of the output will be set to this.
  /// @param[in] fieldMapper A function or functor that takes a `DataSet` as its first
  ///     argument and a `Field` as its second argument. The `DataSet` is the data being
  ///     created and will eventually be returned by `CreateResult`. The `Field` comes from
  ///     `inDataSet`. The function should map the `Field` to match `resultCellSet` and then
  ///     add the resulting field to the `DataSet`. If the mapping is not possible, then
  ///     the function should do nothing.
  ///
  template <typename FieldMapper>
  VISKORES_CONT viskores::cont::DataSet CreateResult(
    const viskores::cont::DataSet& inDataSet,
    const viskores::cont::UnknownCellSet& resultCellSet,
    FieldMapper&& fieldMapper) const
  {
    viskores::cont::DataSet outDataSet;
    outDataSet.SetCellSet(resultCellSet);
    this->MapFieldsOntoOutput(inDataSet, this->GetFieldsToPass(), outDataSet, fieldMapper);
    return outDataSet;
  }

  /// @brief Create the output data set for `DoExecute`.
  ///
  /// This form of `CreateResult` will create an output data set with the given `CellSet`
  /// and `CoordinateSystem`. You must also provide a field mapper function, which is a
  /// function that takes the output `DataSet` being created and a `Field` from the input
  /// and then applies any necessary transformations to the field array and adds it to
  /// the `DataSet`.
  ///
  /// @param[in] inDataSet The input data set being modified (usually the one passed
  ///     into `DoExecute`). The returned `DataSet` is filled with fields of `inDataSet`
  ///     (as selected by the `FieldsToPass` state of the filter).
  /// @param[in] resultCellSet The `CellSet` of the output will be set to this.
  /// @param[in] resultCoordSystem This `CoordinateSystem` will be added to the output.
  /// @param[in] fieldMapper A function or functor that takes a `DataSet` as its first
  ///     argument and a `Field` as its second argument. The `DataSet` is the data being
  ///     created and will eventually be returned by `CreateResult`. The `Field` comes from
  ///     `inDataSet`. The function should map the `Field` to match `resultCellSet` and then
  ///     add the resulting field to the `DataSet`. If the mapping is not possible, then
  ///     the function should do nothing.
  ///
  template <typename FieldMapper>
  VISKORES_CONT viskores::cont::DataSet CreateResultCoordinateSystem(
    const viskores::cont::DataSet& inDataSet,
    const viskores::cont::UnknownCellSet& resultCellSet,
    const viskores::cont::CoordinateSystem& resultCoordSystem,
    FieldMapper&& fieldMapper) const
  {
    viskores::cont::DataSet outDataSet;
    outDataSet.SetCellSet(resultCellSet);
    viskores::filter::FieldSelection fieldSelection = this->GetFieldsToPass();
    if (this->GetPassCoordinateSystems() || fieldSelection.HasField(resultCoordSystem))
    {
      outDataSet.AddCoordinateSystem(resultCoordSystem);
      fieldSelection.AddField(resultCoordSystem, viskores::filter::FieldSelection::Mode::Exclude);
    }
    this->MapFieldsOntoOutput(inDataSet, fieldSelection, outDataSet, fieldMapper);
    return outDataSet;
  }

  /// @brief Create the output data set for `DoExecute`.
  ///
  /// This form of `CreateResult` will create an output data set with the given `CellSet`
  /// and `CoordinateSystem`. You must also provide a field mapper function, which is a
  /// function that takes the output `DataSet` being created and a `Field` from the input
  /// and then applies any necessary transformations to the field array and adds it to
  /// the `DataSet`.
  ///
  /// @param[in] inDataSet The input data set being modified (usually the one passed
  ///     into `DoExecute`). The returned `DataSet` is filled with fields of `inDataSet`
  ///     (as selected by the `FieldsToPass` state of the filter).
  /// @param[in] resultCellSet The `CellSet` of the output will be set to this.
  /// @param[in] coordsName The name of the coordinate system to be added to the output.
  /// @param[in] coordsData The array containing the coordinates of the points.
  /// @param[in] fieldMapper A function or functor that takes a `DataSet` as its first
  ///     argument and a `Field` as its second argument. The `DataSet` is the data being
  ///     created and will eventually be returned by `CreateResult`. The `Field` comes from
  ///     `inDataSet`. The function should map the `Field` to match `resultCellSet` and then
  ///     add the resulting field to the `DataSet`. If the mapping is not possible, then
  ///     the function should do nothing.
  ///
  template <typename FieldMapper>
  VISKORES_CONT viskores::cont::DataSet CreateResultCoordinateSystem(
    const viskores::cont::DataSet& inDataSet,
    const viskores::cont::UnknownCellSet& resultCellSet,
    const std::string& coordsName,
    const viskores::cont::UnknownArrayHandle& coordsData,
    FieldMapper&& fieldMapper) const
  {
    return this->CreateResultCoordinateSystem(
      inDataSet,
      resultCellSet,
      viskores::cont::CoordinateSystem{ coordsName, coordsData },
      fieldMapper);
  }

  /// @brief Retrieve an input field from a `viskores::cont::DataSet` object.
  ///
  /// When a filter operates on fields, it should use this method to get the input fields that
  /// the use has selected with `SetActiveField()` and related methods.
  VISKORES_CONT const viskores::cont::Field& GetFieldFromDataSet(
    const viskores::cont::DataSet& input) const
  {
    return this->GetFieldFromDataSet(0, input);
  }

  /// @copydoc GetFieldFromDataSet
  VISKORES_CONT const viskores::cont::Field& GetFieldFromDataSet(
    viskores::IdComponent index,
    const viskores::cont::DataSet& input) const
  {
    if (this->UseCoordinateSystemAsField[index])
    {
      // Note that we cannot use input.GetCoordinateSystem because that does not return
      // a reference to a field. Instead, get the field name for the coordinate system
      // and return the field.
      const std::string& coordSystemName =
        input.GetCoordinateSystemName(this->GetActiveCoordinateSystemIndex(index));
      return input.GetPointField(coordSystemName);
    }
    else
    {
      return input.GetField(this->GetActiveFieldName(index),
                            this->GetActiveFieldAssociation(index));
    }
  }

  VISKORES_CONT virtual viskores::cont::DataSet DoExecute(
    const viskores::cont::DataSet& inData) = 0;
  VISKORES_CONT virtual viskores::cont::PartitionedDataSet DoExecutePartitions(
    const viskores::cont::PartitionedDataSet& inData);

  /// @brief Convenience method to get the array from a filter's input scalar field.
  ///
  /// A field filter typically gets its input fields using the internal `GetFieldFromDataSet`.
  /// To use this field in a worklet, it eventually needs to be converted to an
  /// `viskores::cont::ArrayHandle`. If the input field is limited to be a scalar field,
  /// then this method provides a convenient way to determine the correct array type.
  /// Like other `CastAndCall` methods, it takes as input a `viskores::cont::Field` (or
  /// `viskores::cont::UnknownArrayHandle`) and a function/functor to call with the appropriate
  /// `viskores::cont::ArrayHandle` type.
  ///
  template <typename Functor, typename... Args>
  VISKORES_CONT void CastAndCallScalarField(const viskores::cont::UnknownArrayHandle& fieldArray,
                                            Functor&& functor,
                                            Args&&... args) const
  {
    fieldArray.CastAndCallForTypesWithFloatFallback<viskores::TypeListFieldScalar,
                                                    VISKORES_DEFAULT_STORAGE_LIST>(
      std::forward<Functor>(functor), std::forward<Args>(args)...);
  }
  /// @copydoc CastAndCallScalarField
  template <typename Functor, typename... Args>
  VISKORES_CONT void CastAndCallScalarField(const viskores::cont::Field& field,
                                            Functor&& functor,
                                            Args&&... args) const
  {
    this->CastAndCallScalarField(
      field.GetData(), std::forward<Functor>(functor), std::forward<Args>(args)...);
  }

  /// @brief Convenience method to get the array from a filter's input vector field.
  ///
  /// A field filter typically gets its input fields using the internal `GetFieldFromDataSet`.
  /// To use this field in a worklet, it eventually needs to be converted to an
  /// `viskores::cont::ArrayHandle`. If the input field is limited to be a vector field with
  /// vectors of a specific size, then this method provides a convenient way to determine
  /// the correct array type. Like other `CastAndCall` methods, it takes as input a
  /// `viskores::cont::Field` (or `viskores::cont::UnknownArrayHandle`) and a function/functor to
  /// call with the appropriate `viskores::cont::ArrayHandle` type. You also have to provide the
  /// vector size as the first template argument. For example
  /// `CastAndCallVecField<3>(field, functor);`.
  ///
  template <viskores::IdComponent VecSize, typename Functor, typename... Args>
  VISKORES_CONT void CastAndCallVecField(const viskores::cont::UnknownArrayHandle& fieldArray,
                                         Functor&& functor,
                                         Args&&... args) const
  {
    using VecList =
      viskores::ListTransform<viskores::TypeListFieldScalar, ScalarToVec<VecSize>::template type>;
    fieldArray.CastAndCallForTypesWithFloatFallback<VecList, VISKORES_DEFAULT_STORAGE_LIST>(
      std::forward<Functor>(functor), std::forward<Args>(args)...);
  }
  /// @copydoc CastAndCallVecField
  template <viskores::IdComponent VecSize, typename Functor, typename... Args>
  VISKORES_CONT void CastAndCallVecField(const viskores::cont::Field& field,
                                         Functor&& functor,
                                         Args&&... args) const
  {
    this->CastAndCallVecField<VecSize>(
      field.GetData(), std::forward<Functor>(functor), std::forward<Args>(args)...);
  }

  /// This method is like `CastAndCallVecField` except that it can be used for a
  /// field of unknown vector size (or scalars). This method will call the given
  /// functor with an `viskores::cont::ArrayHandleRecombineVec`.
  ///
  /// Note that there are limitations with using `viskores::cont::ArrayHandleRecombineVec`
  /// within a worklet. Because the size of the vectors are not known at compile time,
  /// you cannot just create an intermediate `viskores::Vec` of the correct size. Typically,
  /// you must allocate the output array (for example, with
  /// `viskores::cont::ArrayHandleRuntimeVec`), and the worklet must iterate over the
  /// components and store them in the prealocated output.
  ///
  template <typename Functor, typename... Args>
  VISKORES_CONT void CastAndCallVariableVecField(
    const viskores::cont::UnknownArrayHandle& fieldArray,
    Functor&& functor,
    Args&&... args) const
  {
    if (fieldArray.IsBaseComponentType<viskores::Float32>())
    {
      functor(fieldArray.ExtractArrayFromComponents<viskores::Float32>(),
              std::forward<Args>(args)...);
    }
    else if (fieldArray.IsBaseComponentType<viskores::Float64>())
    {
      functor(fieldArray.ExtractArrayFromComponents<viskores::Float64>(),
              std::forward<Args>(args)...);
    }
    else
    {
      // Field component type is not directly supported. Copy to floating point array.
      viskores::cont::UnknownArrayHandle floatArray = fieldArray.NewInstanceFloatBasic();
      viskores::cont::ArrayCopy(fieldArray, floatArray);
      functor(floatArray.ExtractArrayFromComponents<viskores::FloatDefault>(),
              std::forward<Args>(args)...);
    }
  }
  /// @copydoc CastAndCallVariableVecField
  template <typename Functor, typename... Args>
  VISKORES_CONT void CastAndCallVariableVecField(const viskores::cont::Field& field,
                                                 Functor&& functor,
                                                 Args&&... args) const
  {
    this->CastAndCallVariableVecField(
      field.GetData(), std::forward<Functor>(functor), std::forward<Args>(args)...);
  }

private:
  template <typename FieldMapper>
  VISKORES_CONT void MapFieldsOntoOutput(const viskores::cont::DataSet& input,
                                         const viskores::filter::FieldSelection& fieldSelection,
                                         viskores::cont::DataSet& output,
                                         FieldMapper&& fieldMapper) const
  {
    // Basic field mapping
    for (viskores::IdComponent cc = 0; cc < input.GetNumberOfFields(); ++cc)
    {
      auto field = input.GetField(cc);
      if (fieldSelection.IsFieldSelected(field))
      {
        fieldMapper(output, field);
      }
    }

    // Check if the ghost levels have been copied. If so, set so on the output.
    if (input.HasGhostCellField())
    {
      const std::string& ghostFieldName = input.GetGhostCellFieldName();
      if (output.HasCellField(ghostFieldName) && (output.GetGhostCellFieldName() != ghostFieldName))
      {
        output.SetGhostCellFieldName(ghostFieldName);
      }
    }

    for (viskores::IdComponent csIndex = 0; csIndex < input.GetNumberOfCoordinateSystems();
         ++csIndex)
    {
      auto coords = input.GetCoordinateSystem(csIndex);
      if (!output.HasCoordinateSystem(coords.GetName()))
      {
        if (!output.HasPointField(coords.GetName()) && this->GetPassCoordinateSystems())
        {
          fieldMapper(output, coords);
        }
        if (output.HasPointField(coords.GetName()))
        {
          output.AddCoordinateSystem(coords.GetName());
        }
      }
    }
  }

  template <typename FieldMapper>
  VISKORES_CONT void MapFieldsOntoOutput(const viskores::cont::PartitionedDataSet& input,
                                         const viskores::filter::FieldSelection& fieldSelection,
                                         viskores::cont::PartitionedDataSet& output,
                                         FieldMapper&& fieldMapper) const
  {
    for (viskores::IdComponent cc = 0; cc < input.GetNumberOfFields(); ++cc)
    {
      auto field = input.GetField(cc);
      if (fieldSelection.IsFieldSelected(field))
      {
        fieldMapper(output, field);
      }
    }
  }

  template <viskores::IdComponent VecSize>
  struct ScalarToVec
  {
    template <typename T>
    using type = viskores::Vec<T, VecSize>;
  };

  VISKORES_CONT
  virtual viskores::Id DetermineNumberOfThreads(const viskores::cont::PartitionedDataSet& input);

  void ResizeIfNeeded(size_t index_st);

  viskores::filter::FieldSelection FieldsToPass = viskores::filter::FieldSelection::Mode::All;
  bool PassCoordinateSystems = true;
  bool RunFilterWithMultipleThreads = false;
  viskores::Id NumThreadsPerCPU = 4;
  viskores::Id NumThreadsPerGPU = 8;

  std::string OutputFieldName;

  std::vector<std::string> ActiveFieldNames;
  std::vector<viskores::cont::Field::Association> ActiveFieldAssociation;
  std::vector<bool> UseCoordinateSystemAsField;
  std::vector<viskores::Id> ActiveCoordinateSystemIndices;
};

class VISKORES_DEPRECATED(2.2, "Inherit from `viskores::cont::Filter` directly.") FilterField
  : public viskores::filter::Filter
{
};

}
} // namespace viskores::filter

#endif
