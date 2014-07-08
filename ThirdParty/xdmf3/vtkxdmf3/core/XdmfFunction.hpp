/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfFunction.hpp                                                    */
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

#ifndef XDMFFUNCTION_HPP_
#define XDMFFUNCTION_HPP_

// Includes
#include "XdmfCore.hpp"
#include "XdmfArray.hpp"
#include "XdmfArrayReference.hpp"

class XdmfArray;

/**
 * @brief Manipulates arrays based on expressions.
 *
 * The function class provides a way to manipulate XdmfArrays via predefined functions.
 */
class XDMFCORE_EXPORT XdmfFunction : public XdmfArrayReference {

public:


  /**
   * Function wrapper to allow for more flexibility when wrapping
   * functions to be used in the dynamic library.
   *
   * Not required to use the dynamic library because there are
   * methods that take function pointers.
   */
  class XdmfFunctionInternal {
    public:
      virtual ~XdmfFunctionInternal()
      {
      }

      virtual shared_ptr<XdmfArray>
      execute(std::vector<shared_ptr<XdmfArray> > valueVector) = 0;
  };

  /**
   * Binary Operator wrapper to allow for more flexibility when wrapping
   * operations to be used in the dynamic library.
   *
   * Not required to use the dynamic library because there are
   * methods that take function pointers.
   */
  class XdmfOperationInternal {
    public:
      virtual ~XdmfOperationInternal()
      {
      }

      virtual shared_ptr<XdmfArray>
      execute(shared_ptr<XdmfArray> val1,
              shared_ptr<XdmfArray> val2) = 0;
  };

  /**
   * Create a new XdmfFunction
   *
   * Example of use:
   *
   * C++
   * 
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#initialization
   * @until //#initialization
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//initialization
   * @until #//initialization
   *
   * @return    Constructed XdmfFunction.
   */
  static shared_ptr<XdmfFunction> New();

  /**
   * Create a new XdmfFunction
   *
   * Example of use:
   *
   * C++
   * 
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#initexpression
   * @until //#initexpression
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//initexpression
   * @until #//initexpression
   *
   * @param     newExpression   The expression that the function will evaluate
   * @param     newVariables    The arrays that the function will use
   *                            to evalute the expression
   * @return                    Constructed XdmfFunction.
   */
  static shared_ptr<XdmfFunction>
  New(std::string newExpression,
      std::map<std::string,
      shared_ptr<XdmfArray> > newVariables);

  virtual ~XdmfFunction();

  LOKI_DEFINE_VISITABLE(XdmfFunction, XdmfItem);

  static const std::string ItemTag;

  /*
   * Adds a specified function to the list of functions used while
   * evaluating strings
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#declarefunction
   * @until //#declarefunction
   * @skipline //#programstart
   * @until //#programstart
   * @skipline //#addFunction
   * @until //#addFunction
   * @skipline //#programend
   * @until //#programend
   * @skipline //#definefunction
   * @until //#definefunction
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//definefunction
   * @until #//definefunction
   * @skipline #//programstart
   * @until #//programstart
   * @skipline #//addFunction
   * @until #//addFunction
   *
   * @param     name            A string to be associated with the provided
   *                            function during string evaluation
   * @param     functionref     A pointer to the function to be associated
   *                            with the given string
   * @return                    The total number of functions currently usable
   */
  static int
  addFunction(std::string name,
              shared_ptr<XdmfArray>(*functionref)(std::vector<shared_ptr<XdmfArray> >));

  /**
   * Adds a specified function to the list of functions used while
   * evaluating strings.
   * This version allows for custom wrapping.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#declarefunctionclass
   * @until //#declarefunctionclass
   * @skipline //#programstart
   * @until //#programstart
   * @skipline //#addFunctionclass
   * @until //#addFunctionclass
   * @skipline //#programend
   * @until //#programend
   *
   * Python: This version of addFunction is not supported in Python
   *
   * @param     name            A string to be associated with the provided
   *                            function during string evaluation
   * @param     newFunction     A shared pointer to the function to be
   *                            associated with the given string
   * @return                    The total number of functions currently usable
   */
  static int
  addFunction(std::string name,
              shared_ptr<XdmfFunctionInternal> newFunction);

  /**
   * Adds an operation to the list of viable binary operators.
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#declareoperation
   * @until //#declareoperation
   * @skipline //#programstart
   * @until //#programstart
   * @skipline //#addOperation
   * @until //#addOperation
   * @skipline //#programend
   * @until //#programend
   * @skipline //#defineoperation
   * @until //#defineoperation
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//defineoperation
   * @until #//defineoperation
   * @skipline #//programstart
   * @until #//programstart
   * @skipline #//addOperation
   * @until #//addOperation
   *
   * @param     newoperator     The character to be associated with the provided
   *                            binary operation
   * @param     functionref     A pointer to the function to be associated with
   *                            the provided operator
   * @param     priority        Used to determine order of operations,
   *                            the higher the value the earlier it is evaluated
   * @return                    The number of viable operations
   */
  static int
  addOperation(char newoperator,
               shared_ptr<XdmfArray>(*functionref)(shared_ptr<XdmfArray>,
                                                   shared_ptr<XdmfArray>),
               int priority);

  /**
   * Adds an operation to the list of viable binary operators.
   * This version allows for custom wrapping.
   *
   * Example of use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#declareoperationclass
   * @until //#declareoperationclass
   * @skipline //#programstart
   * @until //#programstart
   * @skipline //#addOperationclass
   * @until //#addOperationclass
   * @skipline //#programend
   * @until //#programend
   *
   * Python: This version of addOperation is not supported in Python
   *
   * @param     newoperator     The character to be associated with the provided
   *                            binary operation
   * @param     newOperation    A pointer to the function to be associated
   *                            with the provided operator
   * @param     priority        Used to determine order of operations,
   *                            the higher the value the earlier it is evaluated
   * @return                    The number of viable operations
   */
  static int
  addOperation(char newoperator,
               shared_ptr<XdmfOperationInternal> newOperation,
               int priority);

  /**
   * Averages the values contained in all the provided arrays.
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#valueinit
   * @until //#valueinit
   * @skipline //#average
   * @until //#average
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//valueinit
   * @until #//valueinit
   * @skipline #//average
   * @until #//average
   *
   * @param     values  A vector containing the arrays to be used
   * @return            An XdmfArray containing one value which is the average
   *                    of all values contained within the provided arrays
   */
  static shared_ptr<XdmfArray>
  average(std::vector<shared_ptr<XdmfArray> > values);

  /**
   * Joins the two provided arrays together end to end.
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#valueinit
   * @until //#valueinit
   * @skipline //#chunk
   * @until //#chunk
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//valueinit
   * @until #//valueinit
   * @skipline #//chunk
   * @until #//chunk
   *
   * @param     val1    The first array being evaluated
   * @param     val2    The second array being evaluated
   * @return            The arrays joined end to end
   */
  static shared_ptr<XdmfArray>
  chunk(shared_ptr<XdmfArray> val1,
        shared_ptr<XdmfArray> val2);

  /**
   * Evaluates an expression based on the list of variables provided.
   * A list of valid operations is retrievable from the getSupportedOperations
   * static method.
   * None of the XdmfArrays provided are modified during the evaluation process.
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#declarefunction
   * @until //#declarefunction
   * @skipline //#declareoperation
   * @until //#declareoperation
   * @skipline //#programstart
   * @until //#programstart
   * @skipline //#valueinit
   * @until //#valueinit
   * @skipline //#addOperation
   * @until //#addOperation
   * @skipline //#addFunction
   * @until //#addFunction
   * @skipline //#evaluateExpression
   * @until //#evaluateExpression
   * @skipline //#programend
   * @until //#programend
   * @skipline //#definefunction
   * @until //#definefunction
   * @skipline //#defineoperation
   * @until //#defineoperation
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//definefunction
   * @until #//definefunction
   * @skipline #//defineoperation
   * @until #//defineoperation
   * @skipline #//programstart
   * @until #//programstart
   * @skipline #//valueinit
   * @until #//valueinit
   * @skipline #//addOperation
   * @until #//addOperation
   * @skipline #//addFunction
   * @until #//addFunction
   * @skipline #//evaluateExpression
   * @until #//evaluateExpression
   *
   * @param     expression      A string containing the expresion to be evaluated
   * @param     variables       A map of strings to their XdmfArray equivalent
   * @return                    A shared pointer to the XdmfArray resulting
   *                            from the expression
   */
  static shared_ptr<XdmfArray>
  evaluateExpression(std::string expression,
                     std::map<std::string, shared_ptr<XdmfArray> > variables);

  /**
   * Evaluates the operation specified using the two shared pointers to
   * XdmfArrays provided.
   * A list of valid operations is retrievable from the getSupportedOperations
   * static method.
   * None of the XdmfArrays provided are modified during the evaluation process.
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#declareoperation
   * @until //#declareoperation
   * @skipline //#programstart
   * @until //#programstart
   * @skipline //#valueinit
   * @until //#valueinit
   * @skipline //#addOperation
   * @until //#addOperation
   * @skipline //#evaluateOperation
   * @until //#evaluateOperation
   * @skipline //#programend
   * @until //#programend
   * @skipline //#defineoperation
   * @until //#defineoperation
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//defineoperation
   * @until #//defineoperation
   * @skipline #//programstart
   * @until #//programstart
   * @skipline #//valueinit
   * @until #//valueinit
   * @skipline #//addOperation
   * @until #//addOperation
   * @skipline #//evaluateOperation
   * @until #//evaluateOperation
   *
   * @param     val1            The first array being evaluated
   * @param     val2            The second array being evaluated
   * @param     operation       A character specifying the operation performed
   * @return                    A shared pointer to the Xdmf Array that results
   *                            from the calculation
   */
  static shared_ptr<XdmfArray>
  evaluateOperation(shared_ptr<XdmfArray> val1,
                    shared_ptr<XdmfArray> val2,
                    char operation);

  /**
   * Evaluates the function specified using the vector of XdmfArrays provided.
   * None of the XdmfArrays provided are modified during the evaluation process.
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#declarefunction
   * @until //#declarefunction
   * @skipline //#programstart
   * @until //#programstart
   * @skipline //#valueinit
   * @until //#valueinit
   * @skipline //#addFunction
   * @until //#addFunction
   * @skipline //#evaluateFunction
   * @until //#evaluateFunction
   * @skipline //#programend
   * @until //#programend
   * @skipline //#definefunction
   * @until //#definefunction
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//definefunction
   * @until #//definefunction
   * @skipline #//programstart
   * @until #//programstart
   * @skipline #//valueinit
   * @until #//valueinit
   * @skipline #//addFunction
   * @until #//addFunction
   * @skipline #//evaluateFunction
   * @until #//evaluateFunction
   *
   * @param     valueVector     A vector containing the arrays to be used
   * @param     functionName    The string associated with the function being called
   * @return                    The result of the function being called,
   *                            a scalar will be returned as an XdmfArray with one value
   */
  static shared_ptr<XdmfArray>
  evaluateFunction(std::vector<shared_ptr<XdmfArray> > valueVector,
                   std::string functionName);

  /**
   * Sets the expression that the function will evaluate.
   *
   * Example of use:
   *
   * C++
   * 
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#initexpression
   * @until //#initexpression
   * @skipline //#setExpression
   * @until //#setExpression
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//initexpression
   * @until #//initexpression
   * @skipline #//setExpression
   * @until #//setExpression
   *
   * @return    The expression that the function is currently using to evaluate
   */
  std::string getExpression();

  std::map<std::string, std::string> getItemProperties() const;

  virtual std::string getItemTag() const;

  /**
   * Gets the priority of operation whose associated character is provided.
   * Returns -1 if the operation is not supported.
   * The higher the value the earlier that operation is evaluated
   * during evaluateExpression.
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#getOperationPriority
   * @until //#getOperationPriority
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//getOperationPriority
   * @until #//getOperationPriority
   *
   * @param     operation       The character associated with the operation
   *                            to be checked
   * @return                    The priority of the operation
   */
  static int getOperationPriority(char operation);

  /**
   * Gets a string that contains all the characters of the supported operations.
   * Parenthesis are included for grouping purposes in expressions.
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#getSupportedOperations
   * @until //#getSupportedOperations
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//getSupportedOperations
   * @until #//getSupportedOperations
   *
   * @return    A string containing the characters for all supported operations
   */
  static const std::string getSupportedOperations();

  /**
   * Gets a string that contains all the characters of the supported operations.
   * Parenthesis are included for grouping purposes in expressions.
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#getSupportedFunctions
   * @until //#getSupportedFunctions
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//getSupportedFunctions
   * @until #//getSupportedFunctions
   *
   * @return    A vector containing the strings associated with all valid functions
   */
  static const std::vector<std::string> getSupportedFunctions();

  /**
   * Gets a string that contains all strings that are viable for use when mapping
   * to scalars (which are stored in XdmfArrays of size 1) for the
   * evaluateExpression function.
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#getValidDigitChars
   * @until //#getValidDigitChars
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//getValidDigitChars
   * @until #//getValidDigitChars
   *
   * @return    A string containing all valid variable characters
   */
  static const std::string getValidDigitChars();

  /**
   * Gets a string that contains all strings that are viable for use when mapping
   * to shared pointers of XdmfArrays for the evaluateExpression function.
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#getValidVariableChars
   * @until //#getValidVariableChars
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//getValidVariableChars
   * @until #//getValidVariableChars
   *
   * @return    A string containing all valid variable characters
   */
  static const std::string getValidVariableChars();

  /**
   * Gets the array associated with the provided string out of the function's
   * variable list.
   *
   * Example of use:
   *
   * C++
   * 
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#initexpression
   * @until //#initexpression
   * @skipline //#insertVariable
   * @until //#insertVariable
   * @skipline //#getVariable
   * @until //#getVariable
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//initexpression
   * @until #//initexpression
   * @skipline #//insertVariable
   * @until #//insertVariable
   * @skipline #//getVariable
   * @until #//getVariable
   *
   * @param     key     The string that is associated with the array to be retrieved
   * @return            The array that corresponds with the key provided.
   */
  shared_ptr<XdmfArray> getVariable(std::string key);

  /**
   * Gets a vector containing all the keys accociated with arrays for this function.
   *
   * Example of use:
   *
   * C++
   * 
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#initexpression
   * @until //#initexpression
   * @skipline //#insertVariable
   * @until //#insertVariable
   * @skipline //#getVariableList
   * @until //#getVariableList
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//initexpression
   * @until #//initexpression
   * @skipline #//insertVariable
   * @until #//insertVariable
   * @skipline #//getVariableList
   * @until #//getVariableList
   *
   * @return    A vector of all the keys for this function
   */
  std::vector<std::string> getVariableList();

  /**
   * Joins the two provided arrays while interspercing their values evenly.
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#valueinit
   * @until //#valueinit
   * @skipline //#interlace
   * @until //#interlace
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//valueinit
   * @until #//valueinit
   * @skipline #//interlace
   * @until #//interlace
   *
   * @param     val1    The first array being evaluated
   * @param     val2    The second array being evaluated
   * @return            The interlaced arrays
   */
  static shared_ptr<XdmfArray>
  interlace(shared_ptr<XdmfArray> val1,
            shared_ptr<XdmfArray> val2);

  /**
   * Adds a new variable to the list of variables that the Function will use.
   *
   * Example of use:
   *
   * C++
   * 
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#initexpression
   * @until //#initexpression
   * @skipline //#insertVariable
   * @until //#insertVariable
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//initexpression
   * @until #//initexpression
   * @skipline #//insertVariable
   * @until #//insertVariable
   *
   * @param     key     The string to be associated with the provided array
   * @param     value   The value of the variable when evaluated
   */
  void insertVariable(std::string key, shared_ptr<XdmfArray> value);

  /**
   * Parses the expression that the function contains and generates an array
   * containing the values that the function produces.
   *
   * Example of use:
   *
   * C++
   * 
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#initexpression
   * @until //#initexpression
   * @skipline //#read
   * @until //#read
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//initexpression
   * @until #//initexpression
   * @skipline #//read
   * @until #//read
   */
  virtual shared_ptr<XdmfArray> read();

  /**
   * Removes a variable from the function if it exists.
   *
   * Example of use:
   *
   * C++
   * 
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#initexpression
   * @until //#initexpression
   * @skipline //#insertVariable
   * @until //#insertVariable
   * @skipline //#removeVariable
   * @until //#removeVariable
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//initexpression
   * @until #//initexpression
   * @skipline #//insertVariable
   * @until #//insertVariable
   * @skipline #//removeVariable
   * @until #//removeVariable
   *
   * @param     key     The string to be associated with the provided array
   */
  void removeVariable(std::string key);

  /**
   * Sets the expression that the function will evaluate.
   *
   * Example of use:
   *
   * C++
   * 
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#initexpression
   * @until //#initexpression
   * @skipline //#setExpression
   * @until //#setExpression
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//initexpression
   * @until #//initexpression
   * @skipline #//setExpression
   * @until #//setExpression
   *
   * @param     newExpression   The expression that the function is to evaluate
   */
  void setExpression(std::string newExpression);

  /**
   * Adds together all the values contained in the provided arrays.
   *
   * Example of Use:
   *
   * C++
   *
   * @dontinclude ExampleXdmfFunction.cpp
   * @skipline //#valueinit
   * @until //#valueinit
   * @skipline //#sum
   * @until //#sum
   *
   * Python
   *
   * @dontinclude XdmfExampleFunction.py
   * @skipline #//valueinit
   * @until #//valueinit
   * @skipline #//sum
   * @until #//sum
   *
   * @param     values  A vector containing the arrays to be used
   * @return            An XdmfArray containing one value which is the total
   *                    of all the values contained within the provided arrays
   */
  static shared_ptr<XdmfArray>
  sum(std::vector<shared_ptr<XdmfArray> > values);

  void traverse(const shared_ptr<XdmfBaseVisitor> visitor);

protected:

  XdmfFunction();
  XdmfFunction(std::string newExpression,
               std::map<std::string,
               shared_ptr<XdmfArray> > newVariables);

private:

  XdmfFunction(const XdmfFunction &);  // Not implemented.
  void operator=(const XdmfFunction &);  // Not implemented.

  std::map<std::string, shared_ptr<XdmfArray> > mVariableList;
  std::string mExpression;

  static std::string mSupportedOperations;
  static const std::string mValidVariableChars;
  static const std::string mValidDigitChars;
  static std::map<char, int> mOperationPriority;


  static std::map<std::string, shared_ptr<XdmfFunctionInternal> > arrayFunctions;
  static std::map<char, shared_ptr<XdmfOperationInternal> > operations;

};

#endif /* XDMFFUNCTION_HPP_ */
