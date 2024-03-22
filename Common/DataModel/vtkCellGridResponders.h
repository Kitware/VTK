// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridResponders
 * @brief   A container that holds objects able to respond to queries
 *          specialized for particular vtkCellMetadata types.
 *
 * This class holds sets of responders for vtkCellGridQuery and for
 * vtkCellAttributeQuery.
 *
 * ## Cell-grid query responders
 *
 * vtkCellGridResponders holds a list of objects statically registered to
 * the vtkCellMetadata subclass. These objects respond to
 * queries for information (e.g., a bounding box) or processing
 * (e.g., rendering, picking, generating isocontours) for one cell
 * type (and subclasses of that cell type if no more specific
 * responder is registered).
 *
 * Application code (such as a plugin) can register subclasses of
 * vtkCellGridResponse which accept the API of a particular
 * vtkCellGridQuery for that cell type.
 * Then, when a query is passed to the cell, this collection will
 * identify matching responders for the query and invoke them until
 * one returns true (indicating success).
 * Multiple matches can exist as a responder can be registered to a
 * common base cell class and/or to handle common base query classes.
 *
 * If a given cell type cannot respond to a query, its superclasses
 * are asked to respond. If no superclass can respond to the query,
 * then query's superclasses are searched for responders.
 *
 * Because queries can be registered to cell types at any time,
 * existing cell types can be extended to support new features
 * by additional libraries.
 *
 * ## Cell-attribute calculators
 *
 * In order to support the evaluation of vtkCellAttribute data
 * on any vtkCellMetadata (cell type), this class also holds
 * "calculators" grouped by both attribute and cell type.
 * This API is different that vtkCellGridQuery because
 * vtkCellAttribute is not subclassed by attribute type but
 * rather uses vtkStringToken data to determine the nature of the
 * attribute (e.g., Lagrange; Nedelec; Raviart-Thomas; etc.).
 *
 * It is also different in that these objects are expected to be
 * used inside vtkCellGridQuery to evaluate a single cell at a
 * time rather than longer-running queries across all cells.
 *
 * @sa vtkCellMetadata vtkCellGrid vtkCellAttribute
 */

#ifndef vtkCellGridResponders_h
#define vtkCellGridResponders_h

#include "vtkCellAttributeCalculator.h" // For API.
#include "vtkCommonDataModelModule.h"   // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // For return values
#include "vtkStringToken.h"  // For API.
#include "vtkTypeName.h"     // For RegisterQueryResponder.

#include <unordered_map>
#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGridQuery;
class vtkCellGridResponderBase;
class vtkCellMetadata;

class VTKCOMMONDATAMODEL_EXPORT vtkCellGridResponders : public vtkObject
{
public:
  static vtkCellGridResponders* New();

  /// A map of tag names (such as the cell's type-name) to values of the tag
  /// accepted or provided for that tag.
  using TagSet = std::unordered_map<vtkStringToken, std::unordered_set<vtkStringToken>>;

  /// A record of a registered calculator along with the tags it requires
  /// and values of those tags it will accept.
  struct CalculatorForTagSet
  {
    TagSet MatchingTags;
    vtkSmartPointer<vtkCellAttributeCalculator> CalculatorPrototype;

    bool Matches(const TagSet& providedTags) const;
  };

  vtkTypeMacro(vtkCellGridResponders, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Register \a responder for processing a cell's data.
   */
  template <typename CellType, typename QueryType, typename ResponderType>
  void RegisterQueryResponder(ResponderType* responder)
  {
    vtkStringToken queryTypeKey = vtk::TypeName<QueryType>();
    vtkStringToken cellTypeKey = vtk::TypeName<CellType>();
    this->Responders[queryTypeKey][cellTypeKey] = responder;
  }

  /**
   * Invoke a responder for the given query and cell type.
   *
   * If none exists, return false.
   */
  bool Query(vtkCellMetadata* cellType, vtkCellGridQuery* query);

  /**
   * Register a vtkCellAttributeCalculator subclass.
   *
   * Calculators are meant to be lightweight objects that perform
   * a computation on the combination of a single cell type and a
   * single cell-attribute type – the latter identified by a set
   * of \a tags indicating relevant properties of the cell-attribute.
   */
  template <typename CellType, typename CalculatorType>
  bool RegisterCalculator(vtkStringToken tags, vtkCellAttributeCalculator* calculator)
  {
    if (!calculator)
    {
      vtkErrorMacro("Could not register null cell-attribute calculator.");
      return false;
    }

    vtkStringToken calculatorBaseKey = vtk::TypeName<CalculatorType>();
    vtkStringToken cellTypeKey = vtk::TypeName<CellType>();
    if (!dynamic_cast<CalculatorType*>(calculator))
    {
      vtkErrorMacro("Could not register cell-attribute calculator "
        << calculator->GetClassName() << " as it does not inherit " << calculatorBaseKey.Data()
        << ".");
      return false;
    }

    this->Calculators[calculatorBaseKey][cellTypeKey][tags] = calculator;
    return true;
  }

  template <typename CellType, typename CalculatorType>
  bool RegisterCalculator(vtkCellAttributeCalculator* calculator, const TagSet& tags)
  {
    using namespace vtk::literals;
    if (!calculator)
    {
      vtkErrorMacro("Could not register null cell-attribute calculator.");
      return false;
    }

    vtkStringToken calculatorBaseKey = vtk::TypeName<CalculatorType>();
    vtkStringToken cellTypeKey = vtk::TypeName<CellType>();
    if (!dynamic_cast<CalculatorType*>(calculator))
    {
      vtkErrorMacro("Could not register cell-attribute calculator "
        << calculator->GetClassName() << " as it does not inherit " << calculatorBaseKey.Data()
        << ".");
      return false;
    }

    TagSet tagsIncludingType = tags;
    tagsIncludingType["Type"_token] = { cellTypeKey };
    this->CalculatorRegistry[calculatorBaseKey].push_back(
      CalculatorForTagSet{ tagsIncludingType, calculator });
    return true;
  }

  /// Fetch an instance of an attribute calculator for the given tags.
  vtkSmartPointer<vtkCellAttributeCalculator> AttributeCalculator(vtkStringToken calculatorType,
    vtkCellMetadata* cellType, vtkCellAttribute* attrib, const TagSet& tags) const;

  template <typename CalculatorType>
  vtkSmartPointer<CalculatorType> AttributeCalculator(
    vtkCellMetadata* cellType, vtkCellAttribute* cellAttribute) const
  {
    auto calculatorKey = vtk::TypeName<CalculatorType>();
    auto baseResult = this->AttributeCalculator(calculatorKey, cellType, cellAttribute);
    auto result = CalculatorType::SafeDownCast(baseResult);
    return result;
  }

  template <typename CalculatorType>
  vtkSmartPointer<CalculatorType> AttributeCalculator(
    vtkCellMetadata* cellType, vtkCellAttribute* attrib, const TagSet& tags) const
  {
    auto calculatorKey = vtk::TypeName<CalculatorType>();
    auto baseResult = this->AttributeCalculator(calculatorKey, cellType, attrib, tags);
    auto result = CalculatorType::SafeDownCast(baseResult);
    return result;
  }

  /// Return a cache object given a key.
  ///
  /// Objects held in this cache may live for the duration of an application, not a
  /// single query.
  ///
  /// Cached objects are expected to invalidate themselves (say by using vtkCommand
  /// to observe events that should invalidate them).
  vtkSmartPointer<vtkObject> GetCacheData(std::size_t key);

  template <typename CacheType>
  vtkSmartPointer<CacheType> GetCacheDataAs(std::size_t key, bool createIfAbsent = false)
  {
    vtkSmartPointer<CacheType> castCache;
    auto rawCache = this->GetCacheData(key);
    if (!rawCache)
    {
      if (createIfAbsent)
      {
        castCache = vtkSmartPointer<CacheType>::New();
        this->Caches[key] = castCache;
      }
      return castCache;
    }
    castCache = dynamic_cast<CacheType*>(rawCache.GetPointer());
    return castCache;
  }

  /// Add a cache entry mapping \a key to \a value.
  ///
  /// If \a overwrite is true, this will release any pre-existing value
  /// (which may force its destruction).
  ///
  /// This method returns true upon success and false otherwise.
  /// Failure will occur if \a overwrite is false and either
  /// + there is a pre-existing object at \a key that it not \a value; or
  /// + \a value is null (adding empty entries is forbidden).
  ///
  /// Note that by passing a null \a value with \a overwrite set to true,
  /// you can clear an existing cache entry.
  bool SetCacheData(std::size_t key, vtkSmartPointer<vtkObject> value, bool overwrite = false);

protected:
  vtkCellGridResponders() = default;
  ~vtkCellGridResponders() override = default;

  std::unordered_map<vtkStringToken,
    std::unordered_map<vtkStringToken, vtkSmartPointer<vtkCellGridResponderBase>>>
    Responders;

  std::unordered_map<std::size_t, vtkSmartPointer<vtkObject>> Caches;

  /// A map from a calculator base class to a set of registered prototypes.
  std::unordered_map<vtkStringToken, std::vector<CalculatorForTagSet>> CalculatorRegistry;

  /// Nested maps from cell-attribute-query-type to cell-type to cell-attribute-tag-sets to
  /// concrete query implementation.
  ///
  /// For example, AttributeQueries["vtkCellAttributeEvaluator"]["vtkDGTet"][{"HCurl", "I1"}]
  /// might return an object that subclasses vtkCellAttributeEvaluator and implements
  /// evaluation for HCurl fields defined on tetrahedral cells.
  ///
  /// The set of string tokens stored as the third key in the nested map is matched
  /// for any field that contains **all** of the tags.
  std::unordered_map<vtkStringToken,
    std::unordered_map<vtkStringToken,
      std::unordered_map<vtkStringToken, vtkSmartPointer<vtkCellAttributeCalculator>>>>
    Calculators;

private:
  vtkCellGridResponders(const vtkCellGridResponders&) = delete;
  void operator=(const vtkCellGridResponders&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
