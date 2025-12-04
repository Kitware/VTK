// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkStatisticsAlgorithm
 * @brief   Base class for statistics algorithms
 *
 *
 * All statistics algorithms can conceptually be operated with several operations:
 * * Learn: given an input data set, calculate a minimal statistical model (e.g.,
 *   sums, raw moments, joint probabilities).
 * * Derive: given an input minimal statistical model, derive the full model
 *   (e.g., descriptive statistics, quantiles, correlations, conditional
 *    probabilities).
 *   NB: It may be, or not be, a problem that a full model was not derived. For
 *   instance, when doing parallel calculations, one only wants to derive the full
 *   model after all partial calculations have completed. On the other hand, one
 *   can also directly provide a full model, that was previously calculated or
 *   guessed, and not derive a new one.
 * * Assess: given an input data set, input statistics, and some form of
 *   threshold, assess a subset of the data set.
 * * Test: perform at least one statistical test.
 * Therefore, a vtkStatisticsAlgorithm has the following ports
 * * 3 optional input ports:
 *   * Data (vtkTable)
 *   * Parameters to the learn operation (vtkTable)
 *   * Input model (vtkStatisticalModel)
 * * 3 output ports:
 *   * Data (input annotated with assessments when the Assess operation is ON).
 *   * Output model (identical to the input model when Learn operation is OFF).
 *   * Output of statistical tests. Some engines do not offer such tests yet, in
 *     which case this output will always be empty even when the Test operation is ON.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
 * for implementing this class.
 * Updated by Philippe Pebay, Kitware SAS 2012
 */

#ifndef vtkStatisticsAlgorithm_h
#define vtkStatisticsAlgorithm_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkStringToken.h"             // For constructor map
#include "vtkTableAlgorithm.h"

#include <token/Singletons.h> // For Schwarz counter.

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObjectCollection;
class vtkStatisticalModel;
class vtkStdString;
class vtkStringArray;
class vtkStringToken;
class vtkVariant;
class vtkVariantArray;
class vtkDoubleArray;
class vtkStatisticsAlgorithmPrivate;

class VTKFILTERSSTATISTICS_EXPORT vtkStatisticsAlgorithm : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkStatisticsAlgorithm, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * enumeration values to specify input port types
   */
  enum InputPorts
  {
    INPUT_DATA = 0,       //!< Port 0 is for learn data
    LEARN_PARAMETERS = 1, //!< Port 1 is for learn parameters (initial guesses, etc.)
    INPUT_MODEL = 2       //!< Port 2 is for a priori models
  };

  /**
   * enumeration values to specify output port types
   */
  enum OutputIndices
  {
    OUTPUT_DATA = 0,  //!< Output 0 mirrors the input data, plus optional assessment columns
    OUTPUT_MODEL = 1, //!< Output 1 contains any generated model
    OUTPUT_TEST = 2   //!< Output 2 contains result of statistical test(s)
  };

  /**
   * A convenience method for setting learn input parameters (if one is expected or allowed).
   * It is equivalent to calling SetInputConnection( 1, params );
   */
  virtual void SetLearnOptionParameterConnection(vtkAlgorithmOutput* params)
  {
    this->SetInputConnection(vtkStatisticsAlgorithm::LEARN_PARAMETERS, params);
  }

  /**
   * A convenience method for setting learn input parameters (if one is expected or allowed).
   * It is equivalent to calling SetInputData( 1, params );
   */
  virtual void SetLearnOptionParameters(vtkDataObject* params)
  {
    this->SetInputData(vtkStatisticsAlgorithm::LEARN_PARAMETERS, params);
  }

  /**
   * A convenience method for setting the input model connection (if one is expected or allowed).
   * It is equivalent to calling SetInputConnection( 2, model );
   */
  virtual void SetInputModelConnection(vtkAlgorithmOutput* model)
  {
    this->SetInputConnection(vtkStatisticsAlgorithm::INPUT_MODEL, model);
  }

  /**
   * A convenience method for setting the input model (if one is expected or allowed).
   * It is equivalent to calling SetInputData( 2, model );
   */
  virtual void SetInputModel(vtkDataObject* model)
  {
    this->SetInputData(vtkStatisticsAlgorithm::INPUT_MODEL, model);
  }

  /**
   * A convenience method for fetching an output model that
   * returns the proper type so downcasting is not required.
   */
  virtual vtkStatisticalModel* GetOutputModel();

  ///@{
  /**
   * Set/Get the Learn operation.
   */
  vtkSetMacro(LearnOption, bool);
  vtkGetMacro(LearnOption, bool);
  vtkBooleanMacro(LearnOption, bool);
  ///@}

  ///@{
  /**
   * Set/Get the Derive operation.
   */
  vtkSetMacro(DeriveOption, bool);
  vtkGetMacro(DeriveOption, bool);
  vtkBooleanMacro(DeriveOption, bool);
  ///@}

  ///@{
  /**
   * Set/Get the Assess operation.
   */
  vtkSetMacro(AssessOption, bool);
  vtkGetMacro(AssessOption, bool);
  vtkBooleanMacro(AssessOption, bool);
  ///@}

  ///@{
  /**
   * Set/Get the Test operation.
   */
  vtkSetMacro(TestOption, bool);
  vtkGetMacro(TestOption, bool);
  vtkBooleanMacro(TestOption, bool);
  ///@}

  ///@{
  /**
   * Set/Get the number of tables in the primary model.
   */
  vtkSetMacro(NumberOfPrimaryTables, vtkIdType);
  vtkGetMacro(NumberOfPrimaryTables, vtkIdType);
  ///@}

  ///@{
  /**
   * Set/get assessment names.
   */
  virtual void SetAssessNames(vtkStringArray*);
  vtkGetObjectMacro(AssessNames, vtkStringArray);
  ///@}

  ///@{
  /**
   * A base class for a functor that assesses data.
   */
  class AssessFunctor
  {
  public:
    virtual void operator()(vtkDoubleArray*, vtkIdType) = 0;
    virtual ~AssessFunctor() = default;
  };
  ///@}

  /**
   * Provide a limit on the number of columns per request for this algorithm subclass.
   *
   * Subclasses may override if they require requests to have N or fewer columns per request.
   * This is used by other filters (such as vtkGenerateStatistics) to decide how to configure
   * an algorithm when more columns have been specified than are supported.
   * The result of this call is not used by this class or its subclasses; it is provided
   * solely for other filters and user interfaces.
   *
   * A value of 0 indicates no limit on the number of columns per request.
   */
  virtual int GetMaximumNumberOfColumnsPerRequest() const { return 0; }

  /**
   * Add or remove a column from the current analysis request.
   * Once all the column status values are set, call RequestSelectedColumns()
   * before selecting another set of columns for a different analysis request.
   * The way that columns selections are used varies from algorithm to algorithm.

   * Note: the set of selected columns is maintained in vtkStatisticsAlgorithmPrivate::Buffer
   * until RequestSelectedColumns() is called, at which point the set is appended
   * to vtkStatisticsAlgorithmPrivate::Requests.
   * If there are any columns in vtkStatisticsAlgorithmPrivate::Buffer at the time
   * RequestData() is called, RequestSelectedColumns() will be called and the
   * selection added to the list of requests.
   */
  virtual void SetColumnStatus(const char* namCol, int status);

  /**
   * Set the status of each and every column in the current request to OFF (0).
   */
  virtual void ResetAllColumnStates();

  /**
   * Use the current column status values to produce a new request for statistics
   * to be produced when RequestData() is called. See SetColumnStatus() for more information.
   */
  virtual int RequestSelectedColumns();

  /**
   * Empty the list of current requests.
   */
  virtual void ResetRequests();

  /**
   * Return the number of requests.
   * This does not include any request that is in the column-status buffer
   * but for which RequestSelectedColumns() has not yet been called (even though
   * it is possible this request will be honored when the filter is run -- see SetColumnStatus()
   * for more information).
   */
  virtual vtkIdType GetNumberOfRequests();

  /**
   * Return the number of columns for a given request.
   */
  virtual vtkIdType GetNumberOfColumnsForRequest(vtkIdType request);

  /**
   * Provide the name of the \a c-th column for the \a r-th request.

   * For the version of this routine that returns an integer,
   * if the request or column does not exist because \a r or \a c is out of bounds,
   * this routine returns 0 and the value of \a columnName is unspecified.
   * Otherwise, it returns 1 and the value of \a columnName is set.

   * For the version of this routine that returns const char*,
   * if the request or column does not exist because \a r or \a c is out of bounds,
   * the routine returns nullptr. Otherwise it returns the column name.
   * This version is not thread-safe.
   */
  virtual const char* GetColumnForRequest(vtkIdType r, vtkIdType c);

  virtual int GetColumnForRequest(vtkIdType r, vtkIdType c, vtkStdString& columnName);

  /**
   * Convenience method to create a request with a single column name \p namCol in a single
   * call; this is the preferred method to select columns, ensuring selection consistency
   * (a single column per request).
   * Warning: no name checking is performed on \p namCol; it is the user's
   * responsibility to use valid column names.
   */
  void AddColumn(const char* namCol);

  /**
   * Convenience method to create a request with a single column name pair
   * (\p namColX, \p namColY) in a single call; this is the preferred method to select
   * columns pairs, ensuring selection consistency (a pair of columns per request).

   * Unlike SetColumnStatus(), you need not call RequestSelectedColumns() after AddColumnPair().

   * Warning: \p namColX and \p namColY are only checked for their validity as strings;
   * no check is made that either are valid column names.
   */
  void AddColumnPair(const char* namColX, const char* namColY);

  /**
   * A convenience method (in particular for access from other applications) to
   * set parameter values of Learn mode.
   * Return true if setting of requested parameter name was executed, false otherwise.
   * NB: default method (which is sufficient for most statistics algorithms) does not
   * have any Learn parameters to set and always returns false.
   */
  virtual bool SetParameter(const char* parameter, int index, vtkVariant value);

  /**
   * Given a \a collection of models, calculate an aggregate \a model.
   *
   * If the algorithm does not support aggregation, it may return false.
   */
  virtual bool Aggregate(vtkDataObjectCollection* collection, vtkStatisticalModel* model) = 0;

  ///@{
  /**
   * Copy requests for analysis from another container into this algorithm.
   *
   * This method will return true if the algorithm is marked as modified
   * because the requests were different and false otherwise.
   */
  bool CopyRequests(vtkStatisticsAlgorithmPrivate* requests);
  ///@}

  ///@{
  /**
   * If there is a ghost array in the input, then ghosts matching `GhostsToSkip` mask
   * will be skipped. It is set to 0xff by default (every ghost type is skipped).
   *
   * @sa
   * vtkDataSetAttributes
   * vtkFieldData
   * vtkPointData
   * vtkCellData
   */
  vtkSetMacro(GhostsToSkip, unsigned char);
  vtkGetMacro(GhostsToSkip, unsigned char);
  ///@}

  ///@{
  /**
   * If set, invalid values (NaN or, depending on the circumstances, positive
   * and negative infinity) should cause input samples to be skipped during the
   * Learn phase (i.e., the construction of a model).
   *
   * Not all statistics algorithms use this setting yet.
   *
   * The default is true (skip invalid values).
   */
  vtkSetMacro(SkipInvalidValues, bool);
  vtkGetMacro(SkipInvalidValues, bool);
  vtkBooleanMacro(SkipInvalidValues, bool);
  ///@}

  ///@{
  /**
   * Provide a serialization of this object's internal state so it can be
   * recreated by a vtkStatisticalModel as needed.
   *
   * Subclasses must override this method to encode ivar values after
   * the class name. Use parentheses after the class name to hold a
   * dictionary of ivar name and value pairs. For example, an instance
   * of vtkDescriptiveStatistics might return
   *
   * ```json
   * vtkDescriptiveStatistics(SampleEstimate=True,SignedDeviations=False)
   * ```
   *
   * It is unnecessary but allowed to specify ivar values that are the default.
   *
   * If VTK's serialization-deserialization (SerDes) support ever becomes
   * mandatory, this method should be replaced with it.
   */
  virtual std::string GetAlgorithmParameters() const;
  ///@}

  ///@{
  /**
   * Return a new instance of a subclass named and configured by the
   * \a algorithmParameters.
   *
   * This will return a null object for unknown subclasses (not registered
   * with the VTK object factory.
   *
   * If VTK's serialization-deserialization (SerDes) support ever becomes
   * mandatory, this method should be replaced with it.
   */
  static vtkSmartPointer<vtkStatisticsAlgorithm> NewFromAlgorithmParameters(
    const std::string& algorithmParameters);

  /**
   * Register a subclass of this algorithm.
   *
   * Any algorithm registered with this method can be constructed by
   * NewFromAlgorithmParameters(). Algorithms that are not registered
   * cannot be.
   * Some features of `vtkGenerateStatistics` and planned downstream filters
   * will not work unless subclasses are registered.
   */
  template <typename Algorithm>
  static void RegisterAlgorithm()
  {
    vtkNew<Algorithm> alg;
    vtkStringToken className(alg->GetClassName());
    vtkStatisticsAlgorithm::GetConstructorMap()[className] = []()
    { return vtkSmartPointer<Algorithm>::New(); };
  }

protected:
  vtkStatisticsAlgorithm();
  ~vtkStatisticsAlgorithm() override;

  using AlgorithmConstructor = std::function<vtkSmartPointer<vtkStatisticsAlgorithm>()>;
  using AlgorithmConstructorMap = std::unordered_map<vtkStringToken, AlgorithmConstructor>;

  /// Loop over \a algorithmParameters until all are consumed or an error occurs.
  virtual bool ConfigureFromAlgorithmParameters(const std::string& algorithmParameters);

  /**
   * Subclasses must override this method if they have any internal ivars
   * that affect the behavior of RequestData (specifically Learn/Derive at
   * this point).
   *
   * The algorithm should append a string holding comma-separated parameters.
   * If the tail of the input \a algorithmParameters string is an open-parenthesis
   * character, then this method need not start by appending a comma before
   * adding its ivars.
   * Any other tailing character indicates a comma is required if any ivars must be
   * added. This method should *not* add a terminating close-parenthesis
   * character (because that is added by GetAlgorithmParameters()).
   *
   * Implementations should call their superclass's implementation before
   * proceeding themselves.
   */
  virtual void AppendAlgorithmParameters(std::string& algorithmParameters) const;

  /**
   * Consume a single parameter value, setting the value on this instance of the class.
   *
   * Subclasses must override this method if they have any internal ivars
   * that affect the behavior of RequestData (specifically Learn/Derive at
   * this point).
   *
   * If the \a parameterName is not recognized by this method or any of its
   * superclasses, your implementation should return a value of 0 (indicating
   * refusal to consume the \a parameterName). Otherwise, your implementation
   * should return the number of bytes consumed from \a algorithmParameters
   * to obtain a value for \a parameterName (not including any terminating
   * comma or closing-parenthesis).
   */
  virtual std::size_t ConsumeNextAlgorithmParameter(
    vtkStringToken parameterName, const std::string& algorithmParameters);

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Execute the calculations required by the Learn option, given some input Data
   */
  virtual void Learn(vtkTable*, vtkTable*, vtkStatisticalModel*) = 0;

  /**
   * Execute the calculations required by the Derive option.
   */
  virtual void Derive(vtkStatisticalModel*) = 0;

  /**
   * Execute the calculations required by the Assess option.
   */
  virtual void Assess(vtkTable*, vtkStatisticalModel*, vtkTable*) = 0;

  /**
   * A convenience implementation for generic assessment with variable number of variables.
   */
  void Assess(vtkTable*, vtkStatisticalModel*, vtkTable*, int);

  /**
   * Execute the calculations required by the Test option.
   */
  virtual void Test(vtkTable*, vtkStatisticalModel*, vtkTable*) = 0;

  /**
   * A pure virtual method to select the appropriate assessment functor.
   */
  virtual void SelectAssessFunctor(
    vtkTable* outData, vtkDataObject* inMeta, vtkStringArray* rowNames, AssessFunctor*& dfunc) = 0;

  /**
   * Turn a quoted string value into std::string, returning the number of bytes consumed.
   *
   * The "tuple" should be a parenthesized list of single- or double-quoted strings,
   * e.g., "('foo', 'bar', 'baz')" or '("foo", "bar", "baz")'.
   * Escaped quotes inside strings are **not** currently supported.
   *
   * This is used by ConsumeNextAlgorithmParameter() but available for subclasses to use as well.
   */
  static std::size_t ConsumeString(const std::string& source, std::string& value);

  ///@{
  /**
   * Turn a tuple of strings into a vtkStringArray, returning the number of bytes consumed.
   *
   * The "tuple" should be a parenthesized list of single-quoted strings, e.g., "('foo', 'bar',
   * 'baz')". Escaped single-quotes inside strings are **not** currently supported.
   *
   * This is used by ConsumeNextAlgorithmParameter() but available for subclasses to use as well.
   */
  static std::size_t ConsumeStringTuple(const std::string& source, std::vector<std::string>& tuple);
  static std::size_t ConsumeStringTuple(const std::string& source, vtkStringArray* tuple);
  ///@}

  /**
   * Turn tuples (or respectively tuples of tuples) of numbers into a vector (or respectively
   * a vector of vectors) of doubles, returning the number of bytes consumed.
   *
   * The "tuple" should be a parenthesized list of numbers, e.g., "(2.3,3.4,4.5)" (or
   * respectively, parenthesized tuples, e.g., "((1, 0), (0,1))").
   *
   * This is available for subclasses to use in their ConsumeNextAlgorithmParameter() overrides.
   */
  static std::size_t ConsumeDouble(const std::string& source, double& value);
  static std::size_t ConsumeDoubleTuple(const std::string& source, std::vector<double>& tuple);
  static std::size_t ConsumeDoubleTuples(
    const std::string& source, std::vector<std::vector<double>>& tuple);
  static std::size_t ConsumeStringToDoublesMap(
    const std::string& source, std::map<std::string, std::vector<double>>& map);

  /// Turn a string holding an integer value into an integer, returning the
  /// number of bytes consumed by the integer.
  static std::size_t ConsumeInt(const std::string& source, int& value);

  /// Return a map of registered algorithm types.
  static AlgorithmConstructorMap& GetConstructorMap();

  vtkIdType NumberOfPrimaryTables;
  bool LearnOption;
  bool DeriveOption;
  bool AssessOption;
  bool TestOption;
  vtkStringArray* AssessNames;
  unsigned char GhostsToSkip;
  vtkIdType NumberOfGhosts;
  bool SkipInvalidValues;
  vtkStatisticsAlgorithmPrivate* Internals;

private:
  vtkStatisticsAlgorithm(const vtkStatisticsAlgorithm&) = delete;
  void operator=(const vtkStatisticsAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
