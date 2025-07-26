// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStatisticalModel
 * @brief   a base class for statistical modeling of other data
 *
 * vtkStatisticalModel is a subclass of vtkDataObject that holds vtkTables
 * describing statistical models created by vtkStatisticsAlgorithm filters.
 * It may hold any number of tables but each table will have one of the
 * following roles: "learned", "derived", or "test" (the "assess" option of
 * statistical algorithms does not store model data but rather applies it
 * to samples).
 * vtkStatisticalModel also provides methods to configure a vtkStatisticsAlgorithm
 * instance with the same options used to create the model.
 *
 * @sa
 * vtkTable vtkStatisticsAlgorithm
 */

#ifndef vtkStatisticalModel_h
#define vtkStatisticalModel_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"
#include "vtkNew.h"           // For vtkNew
#include "vtkSmartPointer.h"  // For vtkSmartPointer
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <map>

VTK_ABI_NAMESPACE_BEGIN
class vtkTable;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkStatisticalModel : public vtkDataObject
{
public:
  static vtkStatisticalModel* New();
  vtkTypeMacro(vtkStatisticalModel, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Models are composite objects and need to check each table for MTime.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Restore data object to initial state.
   */
  void Initialize() override;

  /**
   * Return true if the model is empty (i.e., has no tables).
   */
  bool IsEmpty();

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value). THIS METHOD
   * IS THREAD SAFE.
   */
  unsigned long GetActualMemorySize() override;

  /**
   * Return the type of data object.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_STATISTICAL_MODEL; }

  ///@{
  /**
   * Shallow and Deep copy.
   */
  void ShallowCopy(vtkDataObject* src) override;
  void DeepCopy(vtkDataObject* src) override;
  ///@}

  /// The types of tables that a model may store.
  enum TableType
  {
    Learned = 0, //!< Raw model data accumulated from samples.
    Derived = 1, //!< Quantities dependent on learned data.
    Test = 2     //!< Information summarizing a test of model fitness.
  };

  /**
   * Given a numeric value (a TableType enumerant), return a human-presentable
   * string describing it.
   */
  static const char* GetTableTypeName(int tableType);
  /**
   * Given a string value (a human-presentable enumerant), return a TableType enumerant
   * integer matching it (or -1 if \a tableType is invalid).
   */
  static int GetTableTypeValue(const std::string& tableType);

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkStatisticalModel* GetData(vtkInformation* info);
  static vtkStatisticalModel* GetData(vtkInformationVector* vv, int ii = 0);
  ///@}

  /**
   * Get the number of table types.
   *
   * This is the number of entries in the TableType enumeration.
   * You may assume that the enumeration is sequential starting at 0.
   */
  static int GetNumberOfTableTypes() { return 3; }

  /**
   * Get the number of tables across all types.
   */
  int GetNumberOfTables();

  /**
   * Get the number of tables of the given \a type.
   *
   * The \a type should be drawn from the TableType enumeration.
   */
  int GetNumberOfTables(int type);

  /**
   * Get the specified table.
   *
   * This may return null if \a index is out of range (see GetNumberOfTables()).
   */
  vtkTable* GetTable(int type, int index);

  /**
   * Get the name of the specified table (if any is set).
   */
  std::string GetTableName(int type, int index);

  ///@{
  /**
   * Find a table by its \a type and \a tableName.
   *
   * If \a index is passed, the index of the first matching
   * table will be set upon return.
   */
  vtkTable* FindTableByName(int type, const std::string& tableName);
  vtkTable* FindTableByName(int type, const std::string& tableName, int& index);
  ///@}

  /**
   * Set the \a number of model tables of the given \a type.
   *
   * This method returns true if this instance of vtkStatisticalModel was modified.
   */
  bool SetNumberOfTables(int type, int number);

  /**
   * Set a specified \a table at the given \a type and \a index.
   *
   * This method returns true if this instance of vtkStatisticalModel was modified.
   */
  bool SetTable(int type, int index, vtkTable* table, const std::string& tableName);

  /**
   * Set the \a name of the specified table (if the table exists).
   *
   * If no such table exists or the table's name matches \a name, this method will
   * return false. If the table name was changed, it returns true.
   */
  bool SetTableName(int type, int index, const std::string& name);

  ///@{
  /**
   * Set/get a serialization of the statistical algorithm used to create
   * the model tables in this instance.
   *
   * This can be used to recreate a subclass of vtkStatisticsAlgorithm
   * for further analysis (such as assessment or testing).
   *
   * This method returns true if this instance of vtkStatisticalModel was modified.
   */
  vtkSetStringMacro(AlgorithmParameters);
  vtkGetStringMacro(AlgorithmParameters);
  void SetAlgorithmParameters(const std::string& algorithmParameters)
  {
    if (algorithmParameters.empty())
    {
      this->SetAlgorithmParameters(nullptr);
    }
    else
    {
      this->SetAlgorithmParameters(algorithmParameters.c_str());
    }
  }
  ///@}

protected:
  // Constructor with no model tables.
  vtkStatisticalModel();
  ~vtkStatisticalModel() override;

  using TableMap = std::map<int, std::vector<vtkSmartPointer<vtkTable>>>;
  TableMap ModelTables;

  char* AlgorithmParameters{ nullptr };

private:
  vtkStatisticalModel(const vtkStatisticalModel&) = delete;
  void operator=(const vtkStatisticalModel&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkStatisticalModel_h
