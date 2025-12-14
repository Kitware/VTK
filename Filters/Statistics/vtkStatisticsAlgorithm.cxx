// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkStatisticsAlgorithm.h"

#include "vtkDataAssembly.h"
#include "vtkDataObjectCollection.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStatisticalModel.h"
#include "vtkStatisticsAlgorithmPrivate.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkStringScanner.h"
#include "vtkStringToken.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVariantArray.h"

// Subclasses we register in NewFromAlgorithmParameters():
#include "vtkAutoCorrelativeStatistics.h"
#include "vtkContingencyStatistics.h"
#include "vtkCorrelativeStatistics.h"
#include "vtkDescriptiveStatistics.h"
#include "vtkHighestDensityRegionsStatistics.h"
#include "vtkKMeansStatistics.h"
#include "vtkMultiCorrelativeStatistics.h"
#include "vtkOrderStatistics.h"
#include "vtkVisualStatistics.h"

#include <sstream>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkCxxSetObjectMacro(vtkStatisticsAlgorithm, AssessNames, vtkStringArray);

namespace
{

/// Count the number of rows marked as ghosts.
struct GhostsCounter
{
  GhostsCounter(vtkUnsignedCharArray* ghosts, unsigned char ghostsToSkip)
    : Ghosts(ghosts)
    , GhostsToSkip(ghostsToSkip)
    , GlobalNumberOfGhosts(0)
  {
  }

  void Initialize() { this->NumberOfGhosts.Local() = 0; }

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    vtkIdType& numberOfGhosts = this->NumberOfGhosts.Local();
    for (vtkIdType id = startId; id < endId; ++id)
    {
      numberOfGhosts += (this->Ghosts->GetValue(id) & this->GhostsToSkip) != 0;
    }
  }

  void Reduce()
  {
    for (vtkIdType numberOfGhosts : this->NumberOfGhosts)
    {
      this->GlobalNumberOfGhosts += numberOfGhosts;
    }
  }

  vtkUnsignedCharArray* Ghosts;
  unsigned char GhostsToSkip;
  vtkIdType GlobalNumberOfGhosts;
  vtkSMPThreadLocal<vtkIdType> NumberOfGhosts;
};

} // anonymous namespace

//------------------------------------------------------------------------------
vtkStatisticsAlgorithm::vtkStatisticsAlgorithm()
{
  this->SetNumberOfInputPorts(3);
  this->SetNumberOfOutputPorts(3);

  // If not told otherwise, only run Learn option
  this->LearnOption = true;
  this->DeriveOption = true;
  this->AssessOption = false;
  this->TestOption = false;
  // Most engines have only 1 primary table.
  this->NumberOfPrimaryTables = 1;
  this->AssessNames = vtkStringArray::New();
  this->GhostsToSkip = 0xff;
  this->NumberOfGhosts = 0;
  this->SkipInvalidValues = true;
  this->Internals = new vtkStatisticsAlgorithmPrivate;
}

//------------------------------------------------------------------------------
vtkStatisticsAlgorithm::~vtkStatisticsAlgorithm()
{
  this->SetAssessNames(nullptr);
  delete this->Internals;
}

//------------------------------------------------------------------------------
void vtkStatisticsAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Learn: " << this->LearnOption << endl;
  os << indent << "Derive: " << this->DeriveOption << endl;
  os << indent << "Assess: " << this->AssessOption << endl;
  os << indent << "Test: " << this->TestOption << endl;
  os << indent << "NumberOfPrimaryTables: " << this->NumberOfPrimaryTables << endl;
  if (this->AssessNames)
  {
    this->AssessNames->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "GhostsToSkip: " << std::hex << this->GhostsToSkip << " (" << std::dec
     << this->GhostsToSkip << ")\n";
  os << indent << "NumberOfGhosts: " << this->NumberOfGhosts << "\n";
  os << indent << "SkipInvalidValues: " << (this->SkipInvalidValues ? "on" : "off") << "\n";
  os << indent << "Internals: " << this->Internals << endl;
}

//------------------------------------------------------------------------------
vtkStatisticalModel* vtkStatisticsAlgorithm::GetOutputModel()
{
  return vtkStatisticalModel::SafeDownCast(
    this->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));
}

//------------------------------------------------------------------------------
bool vtkStatisticsAlgorithm::ConfigureFromAlgorithmParameters(
  const std::string& algorithmParameters)
{
  std::string work = algorithmParameters;
  // Consume a parameter name, then call ConsumeNextAlgorithmParameter()
  // to set the value. If any string remains after the value is consumed,
  // repeat until the string is empty or a parameter cannot be consumed.
  for (; !work.empty();)
  {
    std::size_t parameterNameEnd = work.find('=');
    if (parameterNameEnd == std::string::npos)
    {
      vtkErrorMacro("Could not identify parameter name in \"" << work << "\".");
      return false;
    }
    vtkStringToken parameterName(work.substr(0, parameterNameEnd));
    work = work.substr(parameterNameEnd + 1, std::string::npos);
    std::size_t consumed = this->ConsumeNextAlgorithmParameter(parameterName, work);
    if (consumed == 0)
    {
      vtkErrorMacro("Could not identify parameter value for \"" << parameterName.Data()
                                                                << "\" in \"" << work << "\".");
      return false;
    }
    work = work.substr(consumed + 1, std::string::npos);
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkStatisticsAlgorithm::AppendAlgorithmParameters(std::string& algorithmParameters) const
{
  if (this->AssessNames && this->AssessNames->GetNumberOfValues() > 0)
  {
    if (!algorithmParameters.empty() && algorithmParameters.back() != '(')
    {
      algorithmParameters += ",";
    }
    algorithmParameters += "assess_names=(";
    vtkIdType nv = this->AssessNames->GetNumberOfValues() - 1;
    for (vtkIdType ii = 0; ii <= nv; ++ii)
    {
      auto strname = this->AssessNames->GetValue(ii);
      if (strname.find('"') == std::string::npos)
      {
        algorithmParameters += "\"" + strname + (ii < nv ? "\"," : "\"");
      }
      else if (strname.find('\'') == std::string::npos)
      {
        algorithmParameters += "'" + strname + (ii < nv ? "'," : "'");
      }
      else
      {
        vtkErrorMacro(
          "Cannot serialize assess names (" << strname << ") with both kinds of quotes.");
        continue;
      }
    }
    algorithmParameters += ")";
  }
  // Only specify non-default values for GhostsToSkip, SkipInvalidValues:
  if (this->GhostsToSkip != 0xff)
  {
    if (!algorithmParameters.empty() && algorithmParameters.back() != '(')
    {
      algorithmParameters += ",";
    }
    algorithmParameters += "ghosts_to_skip=" + vtk::to_string(static_cast<int>(this->GhostsToSkip));
  }
  if (!this->SkipInvalidValues)
  {
    if (!algorithmParameters.empty() && algorithmParameters.back() != '(')
    {
      algorithmParameters += ",";
    }
    algorithmParameters +=
      "skip_invalid_values=" + vtk::to_string(static_cast<int>(this->SkipInvalidValues));
  }
}

//------------------------------------------------------------------------------
std::size_t vtkStatisticsAlgorithm::ConsumeNextAlgorithmParameter(
  vtkStringToken parameterName, const std::string& algorithmParameters)
{
  using namespace vtk::literals;
  std::size_t consumed = 0;
  switch (parameterName.GetHash())
  {
    case "assess_names"_hash:
    {
      vtkNew<vtkStringArray> tuple;
      tuple->SetName("AssessNames");
      consumed = this->ConsumeStringTuple(algorithmParameters, tuple);
      if (consumed > 0)
      {
        this->SetAssessNames(tuple);
      }
    }
    break;
    case "ghosts_to_skip"_hash:
    {
      int value;
      if ((consumed = this->ConsumeInt(algorithmParameters, value)))
      {
        this->SetGhostsToSkip(static_cast<unsigned char>(value));
      }
    }
    break;
    case "skip_invalid_values"_hash:
    {
      int value;
      if ((consumed = this->ConsumeInt(algorithmParameters, value)))
      {
        this->SetSkipInvalidValues(static_cast<bool>(value));
      }
    }
    break;
    default:
      break;
  }
  return consumed;
}

//------------------------------------------------------------------------------
int vtkStatisticsAlgorithm::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == INPUT_DATA)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    return 1;
  }
  else if (port == INPUT_MODEL)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStatisticalModel");
    return 1;
  }
  else if (port == LEARN_PARAMETERS)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    return 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
int vtkStatisticsAlgorithm::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == OUTPUT_DATA)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
    return 1;
  }
  else if (port == OUTPUT_MODEL)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkStatisticalModel");
    return 1;
  }
  else if (port == OUTPUT_TEST)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
    return 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
void vtkStatisticsAlgorithm::SetColumnStatus(const char* namCol, int status)
{
  this->Internals->SetBufferColumnStatus(namCol, status);
}

//------------------------------------------------------------------------------
void vtkStatisticsAlgorithm::ResetAllColumnStates()
{
  this->Internals->ResetBuffer();
}

//------------------------------------------------------------------------------
int vtkStatisticsAlgorithm::RequestSelectedColumns()
{
  return this->Internals->AddBufferToRequests();
}

//------------------------------------------------------------------------------
void vtkStatisticsAlgorithm::ResetRequests()
{
  this->Internals->ResetRequests();
}

//------------------------------------------------------------------------------
vtkIdType vtkStatisticsAlgorithm::GetNumberOfRequests()
{
  return this->Internals->GetNumberOfRequests();
}

//------------------------------------------------------------------------------
vtkIdType vtkStatisticsAlgorithm::GetNumberOfColumnsForRequest(vtkIdType request)
{
  return this->Internals->GetNumberOfColumnsForRequest(request);
}

//------------------------------------------------------------------------------
const char* vtkStatisticsAlgorithm::GetColumnForRequest(vtkIdType r, vtkIdType c)
{
  static vtkStdString columnName;
  if (this->Internals->GetColumnForRequest(r, c, columnName))
  {
    return columnName.c_str();
  }
  return nullptr;
}

//------------------------------------------------------------------------------
int vtkStatisticsAlgorithm::GetColumnForRequest(vtkIdType r, vtkIdType c, vtkStdString& columnName)
{
  return this->Internals->GetColumnForRequest(r, c, columnName) ? 1 : 0;
}

//------------------------------------------------------------------------------
void vtkStatisticsAlgorithm::AddColumn(const char* namCol)
{
  if (this->Internals->AddColumnToRequests(namCol))
  {
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkStatisticsAlgorithm::AddColumnPair(const char* namColX, const char* namColY)
{
  if (this->Internals->AddColumnPairToRequests(namColX, namColY))
  {
    this->Modified();
  }
}

//------------------------------------------------------------------------------
bool vtkStatisticsAlgorithm::SetParameter(
  const char* vtkNotUsed(parameter), int vtkNotUsed(index), vtkVariant vtkNotUsed(value))
{
  return false;
}

//------------------------------------------------------------------------------
bool vtkStatisticsAlgorithm::CopyRequests(vtkStatisticsAlgorithmPrivate* requests)
{
  if (!requests)
  {
    return false;
  }
  if (this->Internals->Copy(*requests))
  {
    this->Modified();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
std::string vtkStatisticsAlgorithm::GetAlgorithmParameters() const
{
  std::string result = this->GetClassName();
  result += "(";
  this->AppendAlgorithmParameters(result);
  result += ")";
  return result;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkStatisticsAlgorithm> vtkStatisticsAlgorithm::NewFromAlgorithmParameters(
  const std::string& algorithmParameters)
{
  static bool once = false;
  if (!once)
  {
    once = true;
    vtkStatisticsAlgorithm::RegisterAlgorithm<vtkAutoCorrelativeStatistics>();
    vtkStatisticsAlgorithm::RegisterAlgorithm<vtkContingencyStatistics>();
    vtkStatisticsAlgorithm::RegisterAlgorithm<vtkCorrelativeStatistics>();
    vtkStatisticsAlgorithm::RegisterAlgorithm<vtkDescriptiveStatistics>();
    vtkStatisticsAlgorithm::RegisterAlgorithm<vtkHighestDensityRegionsStatistics>();
    vtkStatisticsAlgorithm::RegisterAlgorithm<vtkKMeansStatistics>();
    vtkStatisticsAlgorithm::RegisterAlgorithm<vtkMultiCorrelativeStatistics>();
    vtkStatisticsAlgorithm::RegisterAlgorithm<vtkOrderStatistics>();
    vtkStatisticsAlgorithm::RegisterAlgorithm<vtkVisualStatistics>();
  }
  vtkSmartPointer<vtkStatisticsAlgorithm> result;
  std::size_t parameterStart = algorithmParameters.find('(');
  std::string className;
  std::string parameterList;
  if (parameterStart == std::string::npos)
  {
    className = algorithmParameters;
  }
  else
  {
    className = algorithmParameters.substr(0, parameterStart);
    if (algorithmParameters.back() != ')')
    {
      vtkGenericWarningMacro("Missing closing parenthesis for algorithm parameters.");
      return result;
    }
    parameterList = algorithmParameters.substr(parameterStart + 1, algorithmParameters.size() - 1);
  }
  auto& cmap = vtkStatisticsAlgorithm::GetConstructorMap();
  auto it = cmap.find(vtkStringToken(className));
  result = (it != cmap.end() ? it->second() : nullptr);
  if (result)
  {
    if (!result->ConfigureFromAlgorithmParameters(parameterList))
    {
      vtkGenericWarningMacro("Cannot parse parameters.");
      result = nullptr; // Destroy the algorithm.
    }
  }
  else
  {
    vtkGenericWarningMacro("Can not create algorithm of type \"" << className << "\".");
  }
  return result;
}

//------------------------------------------------------------------------------
int vtkStatisticsAlgorithm::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Extract inputs
  vtkTable* inData = vtkTable::GetData(inputVector[INPUT_DATA], 0);
  auto* inModel = vtkStatisticalModel::GetData(inputVector[INPUT_MODEL], 0);
  vtkTable* inParameters = vtkTable::GetData(inputVector[LEARN_PARAMETERS], 0);

  // Extract outputs
  vtkTable* outData = vtkTable::GetData(outputVector, OUTPUT_DATA);
  auto* outModel = vtkStatisticalModel::GetData(outputVector, OUTPUT_MODEL);
  vtkTable* outTest = vtkTable::GetData(outputVector, OUTPUT_TEST);

  // If input data table is not null then shallow copy it to output and count ghosts
  // if they are present (so that Learn, Derive, Test, and Assess can use it to
  // adjust sample counts as needed).
  if (inData)
  {
    outData->ShallowCopy(inData);
    outData->GetRowData()->RemoveArray(vtkDataSetAttributes::GhostArrayName());

    // Only count ghosts if GhostsToSkip has 1+ bits set and we have ghost marks.
    vtkUnsignedCharArray* ghosts = inData->GetRowData()->GetGhostArray();
    if (ghosts && this->GhostsToSkip)
    {
      ::GhostsCounter counter(ghosts, this->GhostsToSkip);
      vtkSMPTools::For(0, ghosts->GetNumberOfValues(), counter);
      this->NumberOfGhosts = counter.GlobalNumberOfGhosts;
    }
    else
    {
      this->NumberOfGhosts = 0;
    }
  }

  // If there are any columns selected in the buffer which have not been
  // turned into a request by RequestSelectedColumns(), add them now.
  // There should be no effect if vtkStatisticsAlgorithmPrivate::Buffer is empty.
  // This is here to accommodate the simpler user interfaces in OverView for
  // univariate and bivariate algorithms which will not call RequestSelectedColumns()
  // on their own.
  this->RequestSelectedColumns();

  // Calculate primary statistics if requested
  if (this->LearnOption)
  {
    // First, learn primary statistics from data; otherwise, only use input model as output model
    this->Learn(inData, inParameters, outModel);

    // Second, aggregate learned models with input model if one is present
    if (inModel)
    {
      vtkDataObjectCollection* models = vtkDataObjectCollection::New();
      models->AddItem(inModel);
      models->AddItem(outModel);
      this->Aggregate(models, outModel);
      models->Delete();
    }
  }
  else
  {
    // No input data and no input model result in an error condition
    if (!inModel)
    {
      vtkErrorMacro("No model available AND no Learn phase requested. Cannot proceed with "
                    "statistics algorithm.");
      return 1;
    }

    // Since no learn phase was requested, the output model is equal to the input one
    outModel->ShallowCopy(inModel);
  }

  // Calculate derived statistics if requested
  if (this->DeriveOption)
  {
    this->Derive(outModel);
  }

  // Assess data with respect to statistical model if requested
  if (this->AssessOption)
  {
    this->Assess(inData, outModel, outData);
  }

  // Calculate test statistics if requested
  if (this->TestOption)
  {
    this->Test(inData, outModel, outTest);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkStatisticsAlgorithm::Assess(
  vtkTable* inData, vtkStatisticalModel* inMeta, vtkTable* outData, int numVariables)
{
  if (!inData)
  {
    return;
  }

  if (!inMeta)
  {
    return;
  }

  // Loop over requests
  for (std::set<std::set<vtkStdString>>::const_iterator rit = this->Internals->Requests.begin();
       rit != this->Internals->Requests.end(); ++rit)
  {
    // Storage for variable names of the request (smart pointer because of several exit points)
    vtkSmartPointer<vtkStringArray> varNames = vtkSmartPointer<vtkStringArray>::New();
    varNames->SetNumberOfValues(numVariables);

    // Each request must contain numVariables columns of interest (additional columns are ignored)
    bool invalidRequest = false;
    int v = 0;
    for (std::set<vtkStdString>::const_iterator it = rit->begin();
         v < numVariables && it != rit->end(); ++v, ++it)
    {
      // Try to retrieve column with corresponding name in input data
      std::string const& varName = *it;

      // If requested column does not exist in input, ignore request
      if (!inData->GetColumnByName(varName.c_str()))
      {
        vtkWarningMacro(
          "InData table does not have a column " << varName << ". Ignoring request containing it.");

        invalidRequest = true;
        break;
      }

      // If column with corresponding name was found, store name
      varNames->SetValue(v, varName);
    }
    if (invalidRequest)
    {
      continue;
    }

    // If request is too short, it must also be ignored
    if (v < numVariables)
    {
      vtkWarningMacro("Only " << v << " variables in the request while " << numVariables
                              << "are needed. Ignoring request.");

      continue;
    }

    // Store names to be able to use SetValueByName, and create the outData columns
    vtkIdType nAssessments = this->AssessNames->GetNumberOfValues();
    std::vector<std::string> names(nAssessments);
    vtkIdType nRowData = inData->GetNumberOfRows();
    for (vtkIdType a = 0; a < nAssessments; ++a)
    {
      // Prepare string for numVariables-tuple of variable names
      std::ostringstream assessColName;
      assessColName << this->AssessNames->GetValue(a) << "(";
      for (int i = 0; i < numVariables; ++i)
      {
        // Insert comma before each variable name, save the first one
        if (i > 0)
        {
          assessColName << ",";
        }
        assessColName << varNames->GetValue(i);
      }
      assessColName << ")";

      names[a] = assessColName.str();

      // Create assessment columns with names <AssessmentName>(var1,...,varN)
      vtkDoubleArray* assessColumn = vtkDoubleArray::New();
      assessColumn->SetName(names[a].c_str());
      assessColumn->SetNumberOfTuples(nRowData);
      outData->AddColumn(assessColumn);
      assessColumn->Delete();
    }

    // Select assess functor
    AssessFunctor* dfunc = nullptr;
    this->SelectAssessFunctor(outData, inMeta, varNames, dfunc);

    if (dfunc)
    {
      // Assess each entry of the column
      vtkDoubleArray* assessResult = vtkDoubleArray::New();
      for (vtkIdType r = 0; r < nRowData; ++r)
      {
        // Apply functor
        (*dfunc)(assessResult, r);
        for (vtkIdType a = 0; a < nAssessments; ++a)
        {
          // Store each assessment value in corresponding assessment column
          outData->SetValueByName(r, names[a].c_str(), assessResult->GetValue(a));
        }
      }

      assessResult->Delete();
    }

    delete dfunc;
  }
}

//------------------------------------------------------------------------------
std::size_t vtkStatisticsAlgorithm::ConsumeDouble(const std::string& source, double& value)
{
  std::size_t consumed = 0;
  auto result = vtk::scan_value<double>(source);
  if (result)
  {
    value = result->value();
    consumed = source.size() - result->range().size();
  }
  return consumed;
}

//------------------------------------------------------------------------------
std::size_t vtkStatisticsAlgorithm::ConsumeString(const std::string& source, std::string& value)
{
  std::size_t consumed = 0;
  if (source.empty())
  {
    return consumed;
  }

  value = "";
  char quote = source[0];
  // Consume until we find the matching \a quote
  for (++consumed; consumed < source.size(); ++consumed)
  {
    if (source[consumed] == quote)
    {
      // The value will not include quotes but the return
      // value must account for them:
      return consumed + 1;
    }
    value.push_back(source[consumed]);
  }
  // We reached the end of the string with no terminating quote. Fail.
  consumed = 0;
  return consumed;
}

//------------------------------------------------------------------------------
std::size_t vtkStatisticsAlgorithm::ConsumeStringTuple(
  const std::string& source, std::vector<std::string>& tuple)
{
  std::string work = source;
  std::string value;
  std::size_t consumed = 0;
  std::size_t stringSize;
  std::size_t ii;
  tuple.clear();
  for (ii = 0; ii < work.size(); work = work.substr(stringSize, std::string::npos))
  {
    if ((work[ii] == '(' && tuple.empty()) || (work[ii] == ',' && !tuple.empty()))
    {
      work = work.substr(1, std::string::npos);
      ++consumed;

      // Handle an empty tuple with no strings
      if (work[ii] == ')')
      {
        ++consumed;
        return consumed;
      }

      stringSize = ConsumeString(work, value);
      if (stringSize == 0)
      {
        return 0;
      }
      consumed += stringSize;
      tuple.push_back(value);
    }
    else if (work[ii] == ')' && consumed > 0)
    {
      ++consumed;
      return consumed;
    }
    else
    {
      return 0;
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
std::size_t vtkStatisticsAlgorithm::ConsumeStringTuple(
  const std::string& source, vtkStringArray* tuple)
{
  tuple->SetNumberOfValues(0);
  std::vector<std::string> strvec;
  std::size_t sz = vtkStatisticsAlgorithm::ConsumeStringTuple(source, strvec);
  if (sz == 0)
  {
    return sz;
  }
  tuple->SetNumberOfValues(static_cast<vtkIdType>(strvec.size()));
  vtkIdType ii = 0;
  for (const auto& str : strvec)
  {
    tuple->SetValue(ii++, str.c_str());
  }
  return sz;
}

//------------------------------------------------------------------------------
std::size_t vtkStatisticsAlgorithm::ConsumeDoubleTuple(
  const std::string& source, std::vector<double>& tuple)
{
  std::string work = source;
  double value;
  std::size_t consumed = 0;
  std::size_t stringSize;
  std::size_t ii;
  tuple.clear();
  for (ii = 0; ii < work.size(); work = work.substr(stringSize, std::string::npos))
  {
    if ((work[ii] == '(' && tuple.empty()) || (work[ii] == ',' && !tuple.empty()))
    {
      work = work.substr(1, std::string::npos);
      ++consumed;

      // Handle an empty tuple with no strings
      if (work[ii] == ')')
      {
        ++consumed;
        return consumed;
      }

      stringSize = vtkStatisticsAlgorithm::ConsumeDouble(work, value);
      if (stringSize == 0)
      {
        return 0;
      }
      consumed += stringSize;
      tuple.push_back(value);
    }
    else if (work[ii] == ')' && consumed > 0)
    {
      ++consumed;
      return consumed;
    }
    else
    {
      return 0;
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
std::size_t vtkStatisticsAlgorithm::ConsumeDoubleTuples(
  const std::string& source, std::vector<std::vector<double>>& tuples)
{
  std::string work = source;
  std::vector<double> tuple;
  std::size_t consumed = 0;
  std::size_t stringSize;
  std::size_t ii;
  tuple.clear();
  for (ii = 0; ii < work.size(); work = work.substr(stringSize, std::string::npos))
  {
    if ((work[ii] == '(' && tuple.empty()) || (work[ii] == ',' && !tuple.empty()))
    {
      work = work.substr(1, std::string::npos);
      ++consumed;

      // Handle an empty tuple with no strings
      if (work[ii] == ')')
      {
        ++consumed;
        return consumed;
      }

      stringSize = vtkStatisticsAlgorithm::ConsumeDoubleTuple(work, tuple);
      if (stringSize == 0)
      {
        return 0;
      }
      consumed += stringSize;
      tuples.push_back(tuple);
    }
    else if (work[ii] == ')' && consumed > 0)
    {
      ++consumed;
      return consumed;
    }
    else
    {
      return 0;
    }
  }
  return 0;
}

std::size_t vtkStatisticsAlgorithm::ConsumeStringToDoublesMap(
  const std::string& source, std::map<std::string, std::vector<double>>& map)
{
  std::string work = source;
  std::string key;
  std::vector<double> tuple;
  std::size_t consumed = 0;
  std::size_t keySize;
  std::size_t stringSize;
  std::size_t ii;
  tuple.clear();
  for (ii = 0; ii < work.size(); work = work.substr(stringSize, std::string::npos))
  {
    if ((work[ii] == '{' && tuple.empty()) || (work[ii] == ',' && !tuple.empty()))
    {
      work = work.substr(1, std::string::npos);
      ++consumed;

      // Handle an empty map with no key-value pairs:
      if (work[ii] == '}')
      {
        ++consumed;
        return consumed;
      }

      keySize = vtkStatisticsAlgorithm::ConsumeString(work, key);
      if (keySize == 0)
      {
        return 0;
      }
      consumed += keySize;
      if (work[ii + keySize] != ':')
      {
        return 0;
      }
      // Consume the ":" between key and value.
      ++consumed;
      work = work.substr(1 + keySize, std::string::npos);

      stringSize = vtkStatisticsAlgorithm::ConsumeDoubleTuple(work, tuple);
      if (stringSize == 0)
      {
        return 0;
      }
      consumed += stringSize;
      map[key] = tuple;
    }
    else if (work[ii] == '}' && consumed > 0)
    {
      ++consumed;
      return consumed;
    }
    else
    {
      return 0;
    }
  }
  return 0;
}

std::size_t vtkStatisticsAlgorithm::ConsumeInt(const std::string& source, int& value)
{
  std::size_t consumed = 0;
  auto result = vtk::scan_int<int>(source, 10);
  if (result)
  {
    value = result->value();
    consumed = source.size() - result->range().size();
  }
  return consumed;
}

vtkStatisticsAlgorithm::AlgorithmConstructorMap& vtkStatisticsAlgorithm::GetConstructorMap()
{
  return token_NAMESPACE::singletons().get<AlgorithmConstructorMap>();
}

VTK_ABI_NAMESPACE_END
