
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRCalculatorFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   vtkRCalculatorFilter
 *
 *
 *
 * This class functions as an array calculator for vtkDataArrays and VTKarray objects,
 * using GNU R as the calculation engine.
 *
 * @sa
 *  vtkRInterface vtkRadapter
 *
 * @par Thanks:
 *  Developed by Thomas Otahal at Sandia National Laboratories.
 *
*/

#ifndef vtkRCalculatorFilter_h
#define vtkRCalculatorFilter_h

#include "vtkFiltersStatisticsGnuRModule.h" // For export macro
#include "vtkDataObjectAlgorithm.h"

class vtkRInterface;
class vtkRCalculatorFilterInternals;
class vtkDataSet;
class vtkDoubleArray;
class vtkGraph;
class vtkTree;
class vtkTable;
class vtkCompositeDataSet;
class vtkArrayData;
class vtkStringArray;

class VTKFILTERSSTATISTICSGNUR_EXPORT vtkRCalculatorFilter : public vtkDataObjectAlgorithm
{

public:

  static vtkRCalculatorFilter *New();

  vtkTypeMacro(vtkRCalculatorFilter, vtkDataObjectAlgorithm );
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Copies vtkDataArray named NameOfVTKArray to R with variable
   * name NameOfRvar.  The array must exist in the input data set.

   * Note: for vtkArray use "0","1","2",... for NameOfVTKArray to specify the index of
   * the vtkArray to pass to R.
   */
  void PutArray(const char* NameOfVTKArray, const char* NameOfRvar);

  /**
   * Copies R variable NameOfRvar from R to the vtkDataArray named
   * NameOfVTKArray.  Will replace existing vtkDataArray with the same name.

   * Note: for vtkArray use any string for NameOfVTKArray.  The array will be appended
   * to the list of vtkArrays on the output.
   */
  void GetArray(const char* NameOfVTKArray, const char* NameOfRvar);

  /**
   * Clears the list of variables to be copied to R.
   */
  void RemoveAllPutVariables();

  /**
   * Clears the list of variables to be copied from R.
   */
  void RemoveAllGetVariables();

  //@{
  /**
   * For vtkTable input to the filter.  An R list variable is created for the
   * vtkTable input using PutTable().  The output of the filter can be set from
   * a list variable in R using GetTable()
   */
  void PutTable(const char* NameOfRvar);
  void GetTable(const char* NameOfRvar);
  //@}

  /**
   * For vtkTable input to the filter. An R list variable is created for each name
   * in the array provided using the vtkTables from the input to the filter.
   */
  void PutTables(vtkStringArray* NamesOfRVars);

  /**
   * For vtkTable output of the filter. If more the one name is provided a composite
   * dataset is created for the output of the filter and a vtkTable is added
   * for each R list variable in the array provided.
   */
  void GetTables(vtkStringArray* NamesOfRVars);

  //@{
  /**
   * For vtkTree input to the filter.  An R phylo tree variable is created for the
   * vtkTree input using PutTree().  The output of the filter can be set from
   * a phylo tree variable in R using GetTree()
   */
  void PutTree(const char* NameOfRvar);
  void GetTree(const char* NameOfRvar);
  //@}

  /**
   * For vtkTree input to the filter.  An R phylo tree variable is created for each
   * name in the array provided using the vtkTrees from the input to the filter.
   */
  void PutTrees(vtkStringArray* NamesOfRvars);

  /**
   * For vtkTree output of the filter. If more than one name is provided a composite
   * dataset is created for the output of the filter and a vtkTree is added for
   * each R phylo tree variable in the array provided.
   */
  void GetTrees(vtkStringArray* NamesOfRvars);


  //@{
  /**
   * Script executed by R.  Can also be set from a file.
   */
  vtkSetStringMacro(Rscript);
  vtkGetStringMacro(Rscript);
  //@}

  //@{
  /**
   * Provide the R script executed by R from an input file.
   */
  vtkSetStringMacro(ScriptFname);
  vtkGetStringMacro(ScriptFname);
  //@}

  //@{
  /**
   * Write R output to standard output.
   */
  vtkSetMacro(Routput,int);
  vtkGetMacro(Routput,int);
  //@}

  //@{
  /**
   * Pass VTK time information to R.
   * If turned turned on, the filter will create three variables in R.
   * The variables will be update automatically as time
   * changes in the VTK pipeline.
   * VTK_TIME_STEPS - array of all available time values.
   * VTK_TIME_RANGE- array of minimum and maximum time values.
   * VTK_CURRENT_TIME - floating point time value at the current time index.
   */
  vtkSetMacro(TimeOutput,int);
  vtkGetMacro(TimeOutput,int);
  //@}

  //@{
  /**
   * Create VTK_BLOCK_ID variable in R when processing composite data sets.
   */
  vtkSetMacro(BlockInfoOutput,int);
  vtkGetMacro(BlockInfoOutput,int);
  //@}

  /**
   * This is required to capture REQUEST_DATA_OBJECT requests.
   */
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

protected:

  int SetRscriptFromFile(const char* fname);

  virtual int RequestData(vtkInformation *vtkNotUsed(request),
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  /**
   * Creates the same output type as the input type.
   */
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  vtkRCalculatorFilter();
  ~vtkRCalculatorFilter();

private:

  vtkRCalculatorFilter(const vtkRCalculatorFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRCalculatorFilter&) VTK_DELETE_FUNCTION;

  // Implementation details
  vtkRCalculatorFilterInternals* rcfi;

  int ProcessInputDataSet(vtkDataSet* dsIn);
  int ProcessOutputDataSet(vtkDataSet* dsOut);

  int ProcessInputGraph(vtkGraph* gIn);
  int ProcessOutputGraph(vtkGraph* gOut);

  int ProcessInputArrayData(vtkArrayData * adIn);
  int ProcessOutputArrayData(vtkArrayData * adOut);

  int ProcessInputCompositeDataSet(vtkCompositeDataSet* cdsIn);
  int ProcessOutputCompositeDataSet(vtkCompositeDataSet * cdsOut);

  int ProcessInputTable(vtkTable* tOut);
  int ProcessInputTable(std::string& name, vtkTable* tIn);

  vtkTable* GetOutputTable(std::string& name);
  int ProcessOutputTable(vtkTable* tOut);

  int ProcessInputTree(vtkTree* tIn);
  int ProcessInputTree(std::string& name, vtkTree* tIn);

  vtkTree* GetOutputTree(std::string& name);
  int ProcessOutputTree(vtkTree* tOut);

  int ProcessInputDataObject(vtkDataObject *input);
  int ProcessOutputDataObject(vtkDataObject *input);
  int HasMultipleGets();
  int HasMultiplePuts();

  vtkRInterface* ri;
  char* Rscript;
  char* RfileScript;
  char* ScriptFname;
  int Routput;
  int TimeOutput;
  int BlockInfoOutput;
  char* OutputBuffer;
  vtkDoubleArray* CurrentTime;
  vtkDoubleArray* TimeRange;
  vtkDoubleArray* TimeSteps;
  vtkDoubleArray* BlockId;
  vtkDoubleArray* NumBlocks;

};

#endif

