/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfFunction.cpp                                                    */
/*                                                                           */
/*  Author:                                                                  */
/*     Andrew Burns                                                          */
/*     andrew.j.burns2@us.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2013 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/


#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfFunction.hpp"
#include <stack>
#include <cmath>
#include <boost/assign.hpp>
#include "XdmfError.hpp"

class XdmfFunctionInternalImpl : public XdmfFunction::XdmfFunctionInternal {
  public:
    static shared_ptr<XdmfFunctionInternalImpl>
    New(shared_ptr<XdmfArray> (*newInternal)(std::vector<shared_ptr<XdmfArray> >))
    {
      shared_ptr<XdmfFunctionInternalImpl> p (new XdmfFunctionInternalImpl(newInternal));
      return p;
    }

    ~XdmfFunctionInternalImpl()
    {
    }

    virtual shared_ptr<XdmfArray> execute(std::vector<shared_ptr<XdmfArray> > valueVector)
    {
      return (*mInternalFunction)(valueVector);
    }
  private:
    XdmfFunctionInternalImpl(shared_ptr<XdmfArray> (*newInternal)(std::vector<shared_ptr<XdmfArray> >))
    {
      mInternalFunction = newInternal;
    }

    shared_ptr<XdmfArray> (*mInternalFunction)(std::vector<shared_ptr<XdmfArray> >);
};

class XdmfOperationInternalImpl : public XdmfFunction::XdmfOperationInternal {
  public:
    static shared_ptr<XdmfOperationInternalImpl>
    New(shared_ptr<XdmfArray> (*newInternal)(shared_ptr<XdmfArray>, shared_ptr<XdmfArray>))
    {
      shared_ptr<XdmfOperationInternalImpl> p (new XdmfOperationInternalImpl(newInternal));
      return p;
    }

    ~XdmfOperationInternalImpl()
    {
    }

    virtual shared_ptr<XdmfArray> execute(shared_ptr<XdmfArray> val1,
                                          shared_ptr<XdmfArray> val2)
    {
      return (*mInternalOperation)(val1, val2);
    }
  private:
    XdmfOperationInternalImpl(shared_ptr<XdmfArray> (*newInternal)(shared_ptr<XdmfArray>,
                                                                   shared_ptr<XdmfArray>))
    {
      mInternalOperation = newInternal;
    }

    shared_ptr<XdmfArray> (*mInternalOperation)(shared_ptr<XdmfArray>, shared_ptr<XdmfArray>);
};

std::string XdmfFunction::mSupportedOperations = "-+/*|#()";
const std::string XdmfFunction::mValidVariableChars =
  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_:.";
const std::string XdmfFunction::mValidDigitChars = "1234567890.";
// List the priorities for the operations, based on the order of operations
// The index of the corresponding operation in validOperationChars
// is the same as the index of its priority in this array
std::map<char, int> XdmfFunction::mOperationPriority = 
	boost::assign::map_list_of ('-', 4)
                                   ('+', 4)
                                   ('/', 3)
                                   ('*', 3)
                                   ('|', 2)
                                   ('#', 1)
                                   ('(', 0)
                                   (')', 0);
// The higher the value, the earlier the operation is
// evaluated in the order of operations
// With the exception of parenthesis which are evaluated
// as soon as the closing parenthesis is found

// Note, it doesn't handle overloaded functions well.
// Will generate errors unless overload methods are typecast.
std::map<std::string, shared_ptr<XdmfFunction::XdmfFunctionInternal> >
  XdmfFunction::arrayFunctions =
    boost::assign::map_list_of
      ("ABS", XdmfFunctionInternalImpl::New((shared_ptr<XdmfArray> (*)(std::vector<shared_ptr<XdmfArray> >))
                                            XdmfFunction::abs))
      ("ABS_TOKEN", XdmfFunctionInternalImpl::New((shared_ptr<XdmfArray> (*)(std::vector<shared_ptr<XdmfArray> >))
                                            XdmfFunction::abs))
      ("ACOS", XdmfFunctionInternalImpl::New((shared_ptr<XdmfArray> (*)(std::vector<shared_ptr<XdmfArray> >))
                                            XdmfFunction::arccos))
      ("ASIN", XdmfFunctionInternalImpl::New((shared_ptr<XdmfArray> (*)(std::vector<shared_ptr<XdmfArray> >))
                                            XdmfFunction::arcsin))
      ("ATAN", XdmfFunctionInternalImpl::New((shared_ptr<XdmfArray> (*)(std::vector<shared_ptr<XdmfArray> >))
                                            XdmfFunction::arctan))
      ("AVE", XdmfFunctionInternalImpl::New((shared_ptr<XdmfArray> (*)(std::vector<shared_ptr<XdmfArray> >))
                                            XdmfFunction::average))
      ("COS", XdmfFunctionInternalImpl::New((shared_ptr<XdmfArray> (*)(std::vector<shared_ptr<XdmfArray> >))
                                            XdmfFunction::cos))
      ("EXP", XdmfFunctionInternalImpl::New((shared_ptr<XdmfArray> (*)(std::vector<shared_ptr<XdmfArray> >))
                                            XdmfFunction::exponent))
      ("JOIN", XdmfFunctionInternalImpl::New((shared_ptr<XdmfArray> (*)(std::vector<shared_ptr<XdmfArray> >))
                                            XdmfFunction::join))
      ("LOG", XdmfFunctionInternalImpl::New((shared_ptr<XdmfArray> (*)(std::vector<shared_ptr<XdmfArray> >))
                                            XdmfFunction::log))
      ("SIN", XdmfFunctionInternalImpl::New((shared_ptr<XdmfArray> (*)(std::vector<shared_ptr<XdmfArray> >))
                                            XdmfFunction::sin))
      ("SQRT", XdmfFunctionInternalImpl::New((shared_ptr<XdmfArray> (*)(std::vector<shared_ptr<XdmfArray> >))
                                            XdmfFunction::sqrt))
      ("SUM", XdmfFunctionInternalImpl::New((shared_ptr<XdmfArray> (*)(std::vector<shared_ptr<XdmfArray> >))
                                            XdmfFunction::sum))
      ("TAN", XdmfFunctionInternalImpl::New((shared_ptr<XdmfArray> (*)(std::vector<shared_ptr<XdmfArray> >))
                                            XdmfFunction::tan));

std::map<char, shared_ptr<XdmfFunction::XdmfOperationInternal> >
  XdmfFunction::operations =
    boost::assign::map_list_of
      ('-', XdmfOperationInternalImpl::New(XdmfFunction::subtraction))
      ('+', XdmfOperationInternalImpl::New(XdmfFunction::addition))
      ('*', XdmfOperationInternalImpl::New(XdmfFunction::multiplication))
      ('/', XdmfOperationInternalImpl::New(XdmfFunction::division))
      ('|', XdmfOperationInternalImpl::New(XdmfFunction::chunk))
      ('#', XdmfOperationInternalImpl::New(XdmfFunction::interlace));

shared_ptr<XdmfFunction>
XdmfFunction::New()
{
  shared_ptr<XdmfFunction> p(new XdmfFunction());
  return p;
}

shared_ptr<XdmfFunction>
XdmfFunction::New(std::string newExpression,
                  std::map<std::string, shared_ptr<XdmfArray> > newVariables)
{
  shared_ptr<XdmfFunction> p(new XdmfFunction(newExpression, newVariables));
  return p;
}

XdmfFunction::XdmfFunction():
  mExpression("")
{
}

XdmfFunction::XdmfFunction(std::string newExpression,
                           std::map<std::string, shared_ptr<XdmfArray> > newVariables):
  mVariableList(newVariables),
  mExpression(newExpression)
{
}

XdmfFunction::~XdmfFunction()
{
}

const std::string XdmfFunction::ItemTag = "Function";

shared_ptr<XdmfArray>
XdmfFunction::abs(std::vector<shared_ptr<XdmfArray> > values)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  // Only working with the first array provided
  if (values.size() < 1) {
    XdmfError::message(XdmfError::FATAL,
                       "Error: No Array Passed to Function abs");
  }
  for (unsigned int i = 0; i < values[0]->getSize(); ++i) {
    returnArray->pushBack(std::abs(values[0]->getValue<double>(i)));
  }
  return returnArray;
}

int
XdmfFunction::addFunction(std::string name,
                          shared_ptr<XdmfArray>(*functionref)(std::vector<shared_ptr<XdmfArray> >))
{
  shared_ptr<XdmfFunctionInternalImpl> newFunction =
     XdmfFunctionInternalImpl::New(functionref);
  return XdmfFunction::addFunction(name, newFunction);
}

int
XdmfFunction::addFunction(std::string name,
                          shared_ptr<XdmfFunctionInternal> newFunction)
{
  // Check to ensure that the name has valid characters
  for (unsigned int i = 0; i < name.size(); ++i) {
    // If the character is not found in the list of valid characters
    if (mValidVariableChars.find(name[i]) == std::string::npos) {
      // Then throw an error
      XdmfError::message(XdmfError::FATAL,
                         "Error: Function Name Contains Invalid Character(s)");
    }
  }
  size_t origsize = arrayFunctions.size();
  arrayFunctions[name] = newFunction;
  // If no new functions were added
  if (origsize == arrayFunctions.size()) {
    // Toss a warning, it's nice to let people know that they're doing this
    XdmfError::message(XdmfError::WARNING,
                       "Warning: Function Overwritten");
  }
  return arrayFunctions.size();
}

int
XdmfFunction::addOperation(char newoperator,
                           shared_ptr<XdmfArray>(*operationref)(shared_ptr<XdmfArray>,
                                                                shared_ptr<XdmfArray>),
                           int priority)
{
  shared_ptr<XdmfOperationInternalImpl> newOperation =
     XdmfOperationInternalImpl::New(operationref);
  return XdmfFunction::addOperation(newoperator,
                                    newOperation,
                                    priority);
}

int
XdmfFunction::addOperation(char newoperator,
                           shared_ptr<XdmfOperationInternal> newOperation,
                           int priority)
{
  if (newoperator == '(' || newoperator == ')') {
    XdmfError::message(XdmfError::FATAL,
                       "Error: Parenthesis can not be redefined");
  }
  if (mValidVariableChars.find(newoperator) != std::string::npos
      || mValidDigitChars.find(newoperator) != std::string::npos) {
    XdmfError::message(XdmfError::FATAL,
                       "Error: Operation Overlaps with Variables");
  }
  // Give warning if the operation already exists
  size_t origsize = operations.size();
  // Place reference in the associated location
  operations[newoperator] = newOperation;
  if (origsize == operations.size()) {
    // It's nice to let people know they're doing this
    // So they don't get surprised about changes in behavior
    XdmfError::message(XdmfError::WARNING,
                       "Warning: Operation Overwritten");
    // Overwrite the existing info for that operation
    // Add the priority to the specified location in the priority array
    mOperationPriority[newoperator] = priority;
  }
  else {
    // Create new operation
    // Add operation to the supported character string
    mSupportedOperations.push_back(newoperator);
    mOperationPriority[newoperator] = priority;
  }
  return operations.size();
}

shared_ptr<XdmfArray>
XdmfFunction::addition(shared_ptr<XdmfArray> val1, shared_ptr<XdmfArray> val2)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  for (unsigned int i = 0; i < val1->getSize() || i < val2->getSize(); ++i) {
    if (val1->getSize() == val2->getSize()) {
      returnArray->pushBack(val1->getValue<double>(i) + val2->getValue<double>(i));
    }
    else if (val1->getSize() == 1) {
      returnArray->pushBack(val1->getValue<double>(0) + val2->getValue<double>(i));
    }
    else if (val2->getSize() == 1) {
      returnArray->pushBack(val1->getValue<double>(i) + val2->getValue<double>(0));
    }
    else {
      XdmfError::message(XdmfError::FATAL,
                         "Error: Array Size Mismatch in Function addition");
    }
  }
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::arcsin(std::vector<shared_ptr<XdmfArray> > values)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  // Only working with the first array provided
  if (values.size() < 1) {
    XdmfError::message(XdmfError::FATAL,
                       "Error: No Array Passed to Function arcsin");
  }
  for (unsigned int i = 0; i < values[0]->getSize(); ++i) {
    returnArray->pushBack(asin(values[0]->getValue<double>(i)));
  }
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::arccos(std::vector<shared_ptr<XdmfArray> > values)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  // Only working with the first array provided
  if (values.size() < 1) {
    XdmfError::message(XdmfError::FATAL,
                       "Error: No Array Passed to Function arccos");
  }
  for (unsigned int i = 0; i < values[0]->getSize(); ++i) {
    returnArray->pushBack(acos(values[0]->getValue<double>(i)));
  }
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::arctan(std::vector<shared_ptr<XdmfArray> > values)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  // Only working with the first array provided
  if (values.size() < 1) {
    XdmfError::message(XdmfError::FATAL,
                       "Error: No Array Passed to Function arctan");
  }
  for (unsigned int i = 0; i < values[0]->getSize(); ++i) {
    returnArray->pushBack(atan(values[0]->getValue<double>(i)));
  }
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::average(std::vector<shared_ptr<XdmfArray> > values)
{
  double total = sum(values)->getValue<double>(0);;
  int totalSize = 0;
  for (unsigned int i = 0; i < values.size(); ++i)
  {
    totalSize += values[i]->getSize();
  }
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  returnArray->insert(0, total/totalSize);
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::cos(std::vector<shared_ptr<XdmfArray> > values)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  // Only working with the first array provided
  if (values.size() < 1) {
    XdmfError::message(XdmfError::FATAL,
                       "Error: No Array Passed to Function cos");
  }
  for (unsigned int i = 0; i < values[0]->getSize(); ++i) {
    returnArray->pushBack(std::cos(values[0]->getValue<double>(i)));
  }
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::chunk(shared_ptr<XdmfArray> val1, shared_ptr<XdmfArray> val2)
{
  // Join chunk (add the new array to the end of the first one)
  // Joins into new array and returns it
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  // Determining what type to class it as in order to not lose data
  // and to still have the smallest data type of the two
  shared_ptr<const XdmfArrayType> resultType =
    XdmfArrayType::comparePrecision(val1->getArrayType(),
                                    val2->getArrayType());

  if (resultType == XdmfArrayType::Int8()) {
    char sampleValue = 0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::Int16()) {
    short sampleValue = 0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::Int32()) {
    int sampleValue = 0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::Int64()) {
    long sampleValue = 0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::UInt8()) {
    unsigned char sampleValue = 0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::UInt16()) {
    unsigned short sampleValue = 0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::UInt32()) {
    unsigned int sampleValue = 0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::Float32()) {
    float sampleValue = 0.0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::Float64()) {
    double sampleValue = 0.0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::String()) {
    std::string sampleValue = "";
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else {
    // error type not supported
    XdmfError::message(XdmfError::FATAL, "Invalid type during Chunk");
  }
  returnArray->insert(0, val1, 0, val1->getSize(),  1, 1);
  returnArray->insert(val1->getSize(), val2, 0, val2->getSize(), 1, 1);
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::exponent(std::vector<shared_ptr<XdmfArray> > values)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  // Only working with the first array provided
  if (values.size() < 2) {
    XdmfError::message(XdmfError::FATAL,
                       "Error: Two Arrays Needed for Function exponent");
  }
  for (unsigned int i = 0; i < values[0]->getSize() || i < values[1]->getSize(); ++i) {
    if (values[0]->getSize() == values[1]->getSize()) {
      returnArray->pushBack(std::pow(values[0]->getValue<double>(i), values[1]->getValue<double>(i)));
    }
    else if (values[0]->getSize() == 1) {
      returnArray->pushBack(std::pow(values[0]->getValue<double>(0), values[1]->getValue<double>(i)));
    }
    else if (values[1]->getSize() == 1) {
      returnArray->pushBack(std::pow(values[0]->getValue<double>(i), values[1]->getValue<double>(0)));
    }
    else {
      XdmfError::message(XdmfError::FATAL,
                         "Error: Array Size Mismatch in Function exponent");
    }
  }
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::division(shared_ptr<XdmfArray> val1, shared_ptr<XdmfArray> val2)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  for (unsigned int i = 0; i < val1->getSize() || i < val2->getSize(); ++i) {
    if (val1->getSize() == val2->getSize()) {
      returnArray->pushBack(val1->getValue<double>(i) / val2->getValue<double>(i));
    }
    else if (val1->getSize() == 1) {
      returnArray->pushBack(val1->getValue<double>(0) / val2->getValue<double>(i));
    }
    else if (val2->getSize() == 1) {
      returnArray->pushBack(val1->getValue<double>(i) / val2->getValue<double>(0));
    }
    else {
      XdmfError::message(XdmfError::FATAL,
                         "Error: Array Size Mismatch in Function division");
    }
  }
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::evaluateExpression(std::string expression,
                                 std::map<std::string,
                                   shared_ptr<XdmfArray> > variables)
{
  std::stack<shared_ptr<XdmfArray> > valueStack;
  std::stack<char> operationStack;

  // String is parsed left to right
  // Elements of the same priority are evaluated right to left
  for (unsigned int i = 0; i < expression.size(); ++i) {
    bool hyphenIsDigit = false;
    // hyphen is a special case since it can be used to annotate negative numbers
    if (expression[i] == '-') {
      if (i == 0) {
        //would have to be a digit, otherwise it would be a unpaired operation
        hyphenIsDigit = true;
      }
      else if (mValidDigitChars.find(expression[i+1]) != std::string::npos) {
        // If value after is a valid digit,
        // check value before
        // If a digit, it's an operation
        // If a variable, it's an operation
        // If an operation, it's a digit character
        if (mSupportedOperations.find(expression[i-1]) != std::string::npos) {
          hyphenIsDigit = true;
        }
        else if (expression[i-1] <= ' ') {
          // If whitespace is in front of the hyphen it is presumed to be a negative sign
          // This is to handle passing negative values to functions properly
          hyphenIsDigit = true;
        }
      }
    }
    // Found to be a digit
    if (mValidDigitChars.find(expression[i]) != std::string::npos ||
        (expression[i] == '-' && hyphenIsDigit)) {
      // Progress until a non-digit is found
      int valueStart = i;
      if (i + 1 < expression.size()) {
        while (mValidDigitChars.find(expression[i+1]) != std::string::npos) {
          i++;
        }
      }
      // Push back to the value stack
      shared_ptr<XdmfArray> valueArray = XdmfArray::New();
      // Use this to convert to double
      valueArray->insert(0, atof(expression.substr(valueStart, i + 1 - valueStart).c_str()));
      valueStack.push(valueArray);
    }
    else if (mValidVariableChars.find(expression[i]) != std::string::npos) {
      // Found to be a variable
      int valueStart = i;
      // Progress until a nonvariable value is found
      if (i+1 < expression.size()){
        while (mValidVariableChars.find(expression[i+1]) != std::string::npos) {
          i++;
        }
      }
      // Convert to equivalent
      if (variables.find(expression.substr(valueStart, i + 1 - valueStart))
          == variables.end()) {
        if (arrayFunctions.find(expression.substr(valueStart, i + 1 - valueStart))
            == arrayFunctions.end()) {
          XdmfError::message(XdmfError::FATAL,
                             "Error: Invalid Variable in evaluateExpression "
                             + expression.substr(valueStart, i + 1 - valueStart));
        }
        else {
          std::string currentFunction =
            expression.substr(valueStart, i + 1 - valueStart);
          // Check if next character is an open parenthesis
          if (i+1 >= expression.size()) {
            if (expression[i+1] != '(') {
              XdmfError::message(XdmfError::FATAL,
                                 "Error: No values supplied to function "
                                 + expression.substr(valueStart, i + 1 - valueStart));
            }
          }
          // If it is grab the string between paranthesis

          if (i + 2 >= expression.size()) {
            XdmfError::message(XdmfError::FATAL,
                               "Error: Missing closing parethesis to function "
                               + expression.substr(valueStart, i + 1 - valueStart));
          }
          i = i + 2;
          valueStart = i;
          int numOpenParenthesis = 0;
          while ((expression[i] != ')' || numOpenParenthesis) && i < expression.size()) {
            if (expression[i] == '(') {
              numOpenParenthesis++;
            }
            else if (expression[i] == ')') {
              numOpenParenthesis--;
            }
            i++;
          }
          std::string functionParameters = expression.substr(valueStart, i - valueStart);
          std::vector<shared_ptr<XdmfArray> > parameterVector;
          // Split that string at commas
          size_t parameterSplit = 0;
          while (parameterSplit != std::string::npos) {
            parameterSplit = 0;
            parameterSplit = functionParameters.find_first_of(",", parameterSplit);
            // Feed the substrings to the parse function
            if (parameterSplit == std::string::npos) {
              parameterVector.push_back(evaluateExpression(functionParameters, variables));
            }
            else {
              parameterVector.push_back(
                evaluateExpression(functionParameters.substr(0, parameterSplit),
                                   variables));
              functionParameters = functionParameters.substr(parameterSplit+1);
            }
          }
          valueStack.push(evaluateFunction(parameterVector, currentFunction));
        }
      }
      else {
        // Push equivalent to value stack
        valueStack.push(variables.find(expression.substr(valueStart, i + 1 - valueStart))->second);
      }
    }
    else if (mSupportedOperations.find(expression[i]) != std::string::npos) {
      // Found to be an operation
      // Pop operations off the stack until one of a lower or equal importance is found
      if (operationStack.size() > 0) {
        if (expression[i] == ')') {
          // To close a parenthesis pop off all operations until another parentheis is found
          while (operationStack.size() > 0 && operationStack.top() != '(') {
            // Must be at least two values for this loop to work properly
            if (valueStack.size() < 2) {
              XdmfError::message(XdmfError::FATAL,
                                 "Error: Not Enough Values in evaluateExpression");
            }
            else {
              shared_ptr<XdmfArray> val2 = valueStack.top();
              valueStack.pop();
              shared_ptr<XdmfArray> val1 = valueStack.top();
              valueStack.pop();
              valueStack.push(evaluateOperation(val1, val2, operationStack.top()));
              operationStack.pop();
            }
          }
          operationStack.pop();
        }
        else if (expression[i] == '(') {
          // Just add it if it's a start parenthesis
          // Nothing happens here in that case
          // Addition happens after the if statement
        }
        else {
          int operationLocation = getOperationPriority(expression[i]);
          int topOperationLocation = getOperationPriority(operationStack.top());
          // See order of operations to determine importance
          while (operationStack.size() > 0 && operationLocation < topOperationLocation) {
            // Must be at least two values for this loop to work properly
            if (valueStack.size() < 2) {
              XdmfError::message(XdmfError::FATAL,
                                 "Error: Not Enough Values in evaluateExpression");
            }
            else {
              shared_ptr<XdmfArray> val2 = valueStack.top();
              valueStack.pop();
              shared_ptr<XdmfArray> val1 = valueStack.top();
              valueStack.pop();
              valueStack.push(evaluateOperation(val1, val2, operationStack.top()));
              operationStack.pop();
              if (operationStack.size() == 0) {
                break;
              }
              topOperationLocation = getOperationPriority(operationStack.top());
            }
          }
        }
      }
      if (expression[i] != ')') {
        // Add the operation to the operation stack
        operationStack.push(expression[i]);
      }
    }
    // If not a value or operation the character is ignored
  }

  // Empty what's left in the stacks before finishing
  while (valueStack.size() > 1 && operationStack.size() > 0) {
    if (valueStack.size() < 2) {
      // Must be at least two values for this loop to work properly
      XdmfError::message(XdmfError::FATAL,
                         "Error: Not Enough Values in evaluateExpression");
    }
    else {
      if(operationStack.top() == '(') {
        XdmfError::message(XdmfError::WARNING,
                           "Warning: Unpaired Parenthesis");
      }
      else {
        shared_ptr<XdmfArray> val2 = valueStack.top();
        valueStack.pop();
        shared_ptr<XdmfArray> val1 = valueStack.top();
        valueStack.pop();
        if (operationStack.size() == 0) {
          XdmfError::message(XdmfError::FATAL,
                             "Error: Not Enough Operators in evaluateExpression");
        }
        else {
          valueStack.push(evaluateOperation(val1, val2, operationStack.top()));
          operationStack.pop();
        }
      }
    }
  }

  // Throw error if there's extra operations
  if (operationStack.size() > 0) {
    XdmfError::message(XdmfError::WARNING,
                       "Warning: Left Over Operators in evaluateExpression");
  }

  if (valueStack.size() > 1) {
    XdmfError::message(XdmfError::WARNING,
                       "Warning: Left Over Values in evaluateExpression");
  }

  // Ensure that an array is returned
  // Will error out if this is not done.
  if (valueStack.size() > 0) {
    return valueStack.top();
  }
  else {
    return XdmfArray::New();
  }
}

shared_ptr<XdmfArray>
XdmfFunction::evaluateOperation(shared_ptr<XdmfArray> val1,
                             shared_ptr<XdmfArray> val2,
                             char operation)
{
  if (operations.find(operation) != operations.end()) {
    return operations[operation]->execute(val1, val2);
  }
  else {
    return shared_ptr<XdmfArray>();
  }
}

shared_ptr<XdmfArray>
XdmfFunction::evaluateFunction(std::vector<shared_ptr<XdmfArray> > valueVector,
                            std::string functionName)
{
  if (arrayFunctions.find(functionName) != arrayFunctions.end()) {
    return arrayFunctions[functionName]->execute(valueVector);
  }
  else {
    return shared_ptr<XdmfArray>();
  }
}

std::string
XdmfFunction::getExpression() const
{
  return mExpression;
}

std::string
XdmfFunction::getItemTag() const
{
  return ItemTag;
}

std::map<std::string, std::string>
XdmfFunction::getItemProperties() const
{
  std::map<std::string, std::string> functionProperties;

  functionProperties["Expression"] = mExpression;
  functionProperties["ConstructedType"] = mConstructedType;

  for (std::map<std::string, std::string>::const_iterator constructedIt = mConstructedProperties.begin();
       constructedIt != mConstructedProperties.end();
       ++constructedIt) {
    functionProperties[constructedIt->first] = constructedIt->second;
  }

  std::stringstream variableStream;

  for (std::map<std::string, shared_ptr<XdmfArray> >::const_iterator variableIter = mVariableList.begin();
       variableIter != mVariableList.end();
       ++variableIter) {
    variableStream << "|" << variableIter->first;
  }

  functionProperties["VariableNames"] = variableStream.str();

  return functionProperties;
}

int
XdmfFunction::getOperationPriority(char operation)
{
  size_t operationLocation = mSupportedOperations.find(operation);
  if (operationLocation != std::string::npos) {
    return mOperationPriority[operation];
  }
  else {
    return -1;
  }
}


const std::string
XdmfFunction::getSupportedOperations()
{
        return mSupportedOperations;
}

const std::vector<std::string>
XdmfFunction::getSupportedFunctions()
{
  std::vector<std::string> returnVector;
  for (std::map<std::string, shared_ptr<XdmfFunctionInternal> >::iterator functionWalker
       = arrayFunctions.begin();
       functionWalker != arrayFunctions.end();
       ++functionWalker) {
    returnVector.push_back(functionWalker->first);
  }
  return returnVector;
}

const std::string
XdmfFunction::getValidDigitChars()
{
        return mValidDigitChars;
}

const std::string
XdmfFunction::getValidVariableChars()
{
        return mValidVariableChars;
}

shared_ptr<XdmfArray>
XdmfFunction::getVariable(std::string key)
{
  if (mVariableList.count(key) > 0) {
    return mVariableList[key];
  }
  else {
    return shared_ptr<XdmfArray>();
  }
}

std::vector<std::string>
XdmfFunction::getVariableList()
{
  std::vector<std::string> keyAccumulator;
  for (std::map<std::string, shared_ptr<XdmfArray> >::iterator it = mVariableList.begin();
       it != mVariableList.end();
       ++it) {
    keyAccumulator.push_back(it->first);
  }
  return keyAccumulator;
}

shared_ptr<XdmfArray>
XdmfFunction::interlace(shared_ptr<XdmfArray> val1, shared_ptr<XdmfArray> val2)
{
  // Join interlace (evenly space the second array within the first one)
  // Builds a new array
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  // Resize to the combined size of both arrays
  // Determining what type to class it as in order to not lose data
  // and to still have the smallest data type of the two
  shared_ptr<const XdmfArrayType> resultType =
    XdmfArrayType::comparePrecision(val1->getArrayType(), val2->getArrayType());

  if (resultType == XdmfArrayType::Int8()) {
    char sampleValue = 0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::Int16()) {
    short sampleValue = 0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::Int32()) {
    int sampleValue = 0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::Int64()) {
    long sampleValue = 0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::UInt8()) {
    unsigned char sampleValue = 0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::UInt16()) {
    unsigned short sampleValue = 0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::UInt32()) {
    unsigned int sampleValue = 0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::Float32()) {
    float sampleValue = 0.0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::Float64()) {
    double sampleValue = 0.0;
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else if (resultType == XdmfArrayType::String()) {
    std::string sampleValue = "";
    returnArray->resize(val1->getSize()+val2->getSize(), sampleValue);
  }
  else {
    // error type not supported
    XdmfError::message(XdmfError::FATAL, "Invalid type during Interlace");
  }

  // Determine ratio of array sizes
  int arrayRatio1 = (int)floor(static_cast<double>(val1->getSize())/val2->getSize());
  int arrayRatio2 = (int)floor(static_cast<double>(val2->getSize())/val1->getSize());
  if (arrayRatio1 < 1) {
    arrayRatio1 = 1;
  }
  if (arrayRatio2 < 1) {
    arrayRatio2 = 1;
  }
  // Stride is equal to the ratios rounded up and added together
  int stride = arrayRatio1+arrayRatio2;
  int arrayExcess1 = 0;
  int arrayExcess2 = 0;
  for (int i = 0; i < stride; ++i) {
    // Add the values of each array
    // using strides to interlace and starting index to offset
    // first array gets the first value of the new array
    if (i<arrayRatio1) {
      int amountWritten = val1->getSize()/arrayRatio1;
      if (((amountWritten * arrayRatio1) + i) < (int)val1->getSize()) {
        amountWritten++;
      }
      if (amountWritten > floor(val2->getSize()/arrayRatio2)) {
        arrayExcess1 += amountWritten - (int)floor(val2->getSize()/arrayRatio2);
        amountWritten = (int)floor(val2->getSize()/arrayRatio2);
      }
      returnArray->insert(i, val1, i, amountWritten, stride, arrayRatio1);
    }
    else {
      // Second array takes the rest
      int amountWritten = val2->getSize()/arrayRatio2;
      if (((amountWritten * arrayRatio2) + i) < (int)val2->getSize()) {
        amountWritten++;
      }
      if (amountWritten > floor(val1->getSize()/arrayRatio1)) {
        arrayExcess2 += amountWritten - (int)floor(val1->getSize()/arrayRatio1);
        amountWritten = (int)floor(val1->getSize()/arrayRatio1);
      }
      returnArray->insert(i, val2, i-arrayRatio1, amountWritten, stride, arrayRatio2);
    }
  }
  if (arrayExcess1 > 0) {
    returnArray->insert(val1->getSize()+val2->getSize()-arrayExcess1,
                        val1,
                        val1->getSize()-arrayExcess1,
                        arrayExcess1,
                        1,
                        1);
  }
  else if (arrayExcess2 > 0) {
    returnArray->insert(val1->getSize()+val2->getSize()-arrayExcess2,
                        val2,
                        val2->getSize()-arrayExcess2,
                        arrayExcess2,
                        1,
                        1);
  }
  // After all inserts are done, add the excess values to the end of the array
  return returnArray;
}

void
XdmfFunction::insertVariable(std::string key, shared_ptr<XdmfArray> value)
{
  mVariableList[key] = value;
}

shared_ptr<XdmfArray>
XdmfFunction::join(std::vector<shared_ptr<XdmfArray> > values)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  for (unsigned int i = 0; i < values.size(); ++i) {
    returnArray->insert(returnArray->getSize(),
                        values[i],
                        0,
                        values[i]->getSize(),
                        1,
                        1);
  }
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::log(std::vector<shared_ptr<XdmfArray> > values)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  // Only working with the first array provided
  if (values.size() < 1) {
    XdmfError::message(XdmfError::FATAL,
                       "Error: No Array Passed to Function log");
  }
  for (unsigned int i = 0; i < values[0]->getSize(); ++i) {
    if (values.size() > 1) {
      if (values[0]->getSize() == values[1]->getSize()) {
        returnArray->pushBack(std::log(values[0]->getValue<double>(i))/std::log(values[1]->getValue<double>(i)));
      }
      else if (values[1]->getSize() == 1) {
        returnArray->pushBack(std::log(values[0]->getValue<double>(i))/std::log(values[1]->getValue<double>(0)));
      }
      else {
        XdmfError::message(XdmfError::FATAL,
                           "Error: Array Size Missmatch in Function Log");
      }
    }
    else {
      returnArray->pushBack(std::log(values[0]->getValue<double>(i)));
    }
  }
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::multiplication(shared_ptr<XdmfArray> val1, shared_ptr<XdmfArray> val2)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  for (unsigned int i = 0; i < val1->getSize() || i < val2->getSize(); ++i) {
    if (val1->getSize() == val2->getSize()) {
      returnArray->pushBack(val1->getValue<double>(i) * val2->getValue<double>(i));
    }
    else if (val1->getSize() == 1) {
      returnArray->pushBack(val1->getValue<double>(0) * val2->getValue<double>(i));
    }
    else if (val2->getSize() == 1) {
      returnArray->pushBack(val1->getValue<double>(i) * val2->getValue<double>(0));
    }
    else {
      XdmfError::message(XdmfError::FATAL,
                         "Error: Array Size Mismatch in Function multiplication");
    }
  }
  return returnArray;
}


shared_ptr<XdmfArray>
XdmfFunction::read() const
{
  return evaluateExpression(mExpression, mVariableList);
}

void
XdmfFunction::removeVariable(std::string key)
{
  std::map<std::string, shared_ptr<XdmfArray> >::iterator removeWalker =
    mVariableList.find(key);
  if (removeWalker != mVariableList.end()) {
    mVariableList.erase(removeWalker);
  }
}

void
XdmfFunction::setExpression(std::string newExpression)
{
  mExpression = newExpression;
}

shared_ptr<XdmfArray>
XdmfFunction::sin(std::vector<shared_ptr<XdmfArray> > values)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  // Only working with the first array provided
  if (values.size() < 1) {
    XdmfError::message(XdmfError::FATAL,
                       "Error: No Array Passed to Function sin");
  }
  for (unsigned int i = 0; i < values[0]->getSize(); ++i) {
    returnArray->pushBack(std::sin(values[0]->getValue<double>(i)));
  }
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::sqrt(std::vector<shared_ptr<XdmfArray> > values)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  // Only working with the first array provided
  if (values.size() < 1) {
    XdmfError::message(XdmfError::FATAL,
                       "Error: No Array Passed to Function sqrt");
  }
  for (unsigned int i = 0; i < values[0]->getSize(); ++i) {
    returnArray->pushBack(std::sqrt(values[0]->getValue<double>(i)));
  }
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::subtraction(shared_ptr<XdmfArray> val1, shared_ptr<XdmfArray> val2)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  for (unsigned int i = 0; i < val1->getSize() || i < val2->getSize(); ++i) {
    if (val1->getSize() == val2->getSize()) {
      returnArray->pushBack(val1->getValue<double>(i) - val2->getValue<double>(i));
    }
    else if (val1->getSize() == 1) {
      returnArray->pushBack(val1->getValue<double>(0) - val2->getValue<double>(i));
    }
    else if (val2->getSize() == 1) {
      returnArray->pushBack(val1->getValue<double>(i) - val2->getValue<double>(0));
    }
    else {
      XdmfError::message(XdmfError::FATAL,
                         "Error: Array Size Mismatch in Function subtraction");
    }
  }
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::sum(std::vector<shared_ptr<XdmfArray> > values)
{
  double total = 0.0;
  for (unsigned int i = 0; i < values.size(); ++i) {
    for (unsigned int j = 0; j < values[i]->getSize(); ++j) {
      total += values[i]->getValue<double>(j);
    }
  }
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  returnArray->insert(0, total);
  return returnArray;
}

shared_ptr<XdmfArray>
XdmfFunction::tan(std::vector<shared_ptr<XdmfArray> > values)
{
  shared_ptr<XdmfArray> returnArray = XdmfArray::New();
  // Only working with the first array provided
  if (values.size() < 1) {
    XdmfError::message(XdmfError::FATAL,
                       "Error: No Array Passed to Function tan");
  }
  for (unsigned int i = 0; i < values[0]->getSize(); ++i) {
    returnArray->pushBack(std::tan(values[0]->getValue<double>(i)));
  }
  return returnArray;
}

void
XdmfFunction::traverse(const shared_ptr<XdmfBaseVisitor> visitor)
{
  XdmfItem::traverse(visitor);

  shared_ptr<XdmfArray> spacerarray = XdmfArray::New();
  spacerarray->pushBack((int)0);
  spacerarray->accept(visitor);

  for (std::map<std::string, shared_ptr<XdmfArray> >::iterator it = mVariableList.begin();
       it != mVariableList.end();
       ++it) {
    it->second->accept(visitor);
  }
}
