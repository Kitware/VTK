// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkDGOperation_h
#define vtkDGOperation_h

#include "vtkCompiler.h"              // For VTK_ALWAYS_EXPORT.
#include "vtkDGCell.h"                // For vtkDGCell::Source used in API.
#include "vtkDGOperationStateEntry.h" // For API.
#include "vtkSMPThreadLocal.h"        // For API.

VTK_ABI_NAMESPACE_BEGIN

class vtkDoubleArray;

/**\brief A base class for operator evaluators.
 */
class VTKFILTERSCELLGRID_EXPORT vtkDGOperationBase
{
public:
  /// A range of cell IDs handled by a vtkDGCell::Source instance.
  struct VTKFILTERSCELLGRID_EXPORT RangeKey
  {
    vtkTypeUInt64 Begin;
    vtkTypeUInt64 End;
    bool Contains(vtkTypeUInt64 cellId) const;
    bool ContainedBy(const RangeKey& other) const;
    bool operator<(const RangeKey& other) const;
  };
};

/**\brief Invoke an operator on DG cells/sides.
 *
 */
template <typename InputIterator, typename OutputIterator>
class VTK_ALWAYS_EXPORT vtkDGOperation : public vtkDGOperationBase
{
public:
  /// Type-alias for other objects matching our own type.
  using SelfType = vtkDGOperation<InputIterator, OutputIterator>;
  /// The signature for a function that evaluates iterators over a range.
  using OpEval = vtkDGCellRangeEvaluator<InputIterator, OutputIterator>;
  /// Iterators affect the API offered by vtkDGOperationStateEntry, which has an OpEval.
  using OpEvalEntry = vtkDGOperationStateEntry<InputIterator, OutputIterator>;
  /// Expose the base-class RangeKey struct.
  using vtkDGOperationBase::RangeKey;
  /// Container for functions that evaluate data on a single vtkDGCell::Source instance.
  using EvaluatorMap = std::map<RangeKey, OpEvalEntry>;

  /// Construct an operation object.
  ///
  /// The copy-constructor variant is what allows you to use
  /// vtkSMPThreadLocal<vtkDGOperation> in vtkSMPTools workers.
  vtkDGOperation() = default;
  vtkDGOperation(const SelfType& other);
  vtkDGOperation(
    vtkDGCell* cellType, vtkCellAttribute* cellAttribute, vtkStringToken operationName);

  void PrintSelf(std::ostream& os, vtkIndent indent);

  /// Prepare this instance of vtkDGOperation to evaluate \a operationName on the given \a cellType
  /// and \a cellAttribute.
  ///
  /// This populates the Evaluators ivar with functors valid for a range of cells corresponding
  /// to non-blanked vtkDGCell::Source instances.
  ///
  /// This method returns true upon success and false otherwise.
  /// This method returns false if \a operationName does not name an operator; other inputs
  /// are null pointers; or the cell attribute does not provide information on how the
  /// attribute should be evaluated on the given cell type.
  ///
  /// You should not call the parenthesis operator () if Prepare() returns false.
  ///
  /// If \a includeShape is true (the default), then any transformation of \a cellAttribute
  /// by the grid's shape-attribute that is needed will be factored into the evaluators.
  /// This avoids a double-lookup expense (i.e., once for \a cellAttribute and again for the
  /// shape attribute) when determining which entry in this->Evaluators to invoke for each ID.
  ///
  /// For HGrad function spaces, \a includeShape has no effect. For HDiv and HCurl function
  /// spaces, this will transform vector values from reference coordinates into world coordinates.
  bool Prepare(vtkDGCell* cellType, vtkCellAttribute* cellAttribute, vtkStringToken operationName,
    bool includeShape = true);

  /// Evaluate the prepared operator on the given \a cellIds at the given \a rst parameters,
  /// storing results in the \a result array.
  ///
  /// This method returns true upon success and false otherwise.
  /// If false is returned, partial results may have been written to \a result
  /// (for example, if an invalid cell ID is encountered after others have been
  /// processed).
  ///
  /// The \a begin and \a end integers specify a sub-range of the iterators
  /// to process (allowing this method to be invoked in vtkSMPTools-style loops.
  bool Evaluate(InputIterator& inIter, OutputIterator& outIter, vtkTypeUInt64 begin = 0,
    vtkTypeUInt64 end = ~0);

  /// Return a function that can be called on the named \a sideSpecId.
  ///
  /// This method may return a null evaluator if \a sideSpecId does not exist
  /// or if the \a sideSpecId is blanked.
  ///
  /// If \a sideSpecId == -1, then a function for \a cell->GetCellSpec()
  /// is returned (assuming the cells are not blanked).
  ///
  /// Note that the result may be null and, if non-null, should **only** be
  /// invoked with integers in the proper range. For example, if
  /// ```cpp
  /// auto a = cell->GetCellSource(sideSpecId).Offset;
  /// auto b = cell->GetCellSource(sideSpecId).Connectivity->GetNumberOfTuples();
  /// ```
  /// then you should only call the evaluator with IDs in [a, a+b[.
  OpEval GetEvaluatorForSideSpec(vtkDGCell* cell, int sideSpecId);

  /// Return the number of values generated per tuple each time an input cell-id
  /// and parameter-value are evaluated.
  int GetNumberOfResultComponents() const { return this->NumberOfResultComponents; }

  /// A vtkSMPTools worker class for evaluating an operation across
  /// a range of cells.
  struct VTK_ALWAYS_EXPORT Worker
  {
    InputIterator& InIter;
    OutputIterator& OutIter;
    vtkDGCell* DGCell{ nullptr };
    vtkCellAttribute* CellAtt{ nullptr };
    vtkStringToken OpName;

    Worker(InputIterator& inIter, OutputIterator& outIter, vtkDGCell* dgCell,
      vtkCellAttribute* cellAtt, vtkStringToken opName)
      : InIter(inIter)
      , OutIter(outIter)
      , DGCell(dgCell)
      , CellAtt(cellAtt)
      , OpName(opName)
    {
    }

    vtkSMPThreadLocal<vtkDGOperation<InputIterator, OutputIterator>> TLOp;

    void Initialize()
    {
      if (!TLOp.Local().Prepare(this->DGCell, this->CellAtt, this->OpName))
      {
        throw std::runtime_error("Could not initialize operation for evaluation.");
      }
    }

    void operator()(vtkIdType begin, vtkIdType end)
    {
      TLOp.Local()(this->InIter, this->OutIter, begin, end);
    }
  };

protected:
  void AddSource(vtkCellGrid* grid, vtkDGCell* cellType, std::size_t sideSpecIdx,
    vtkCellAttribute* cellAtt, const vtkCellAttribute::CellTypeInfo& cellTypeInfo,
    vtkDGOperatorEntry& op, bool includeShape);

  /// Return the first iterator to an entry of \a evaluators that covers \a cellKey.
  typename EvaluatorMap::const_iterator FindEvaluator(
    RangeKey cellKey, const EvaluatorMap& evaluators);

  int NumberOfResultComponents{ 0 };
  EvaluatorMap Evaluators;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGOperation_h
// VTK-HeaderTest-Exclude: vtkDGOperation.h
