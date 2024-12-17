// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkXArrayAccessor_h
#define vtkXArrayAccessor_h

#include "vtkNetCDFAccessor.h"

#include "vtkCommand.h"        // for vtkBaseTypeMacro
#include "vtkIONetCDFModule.h" // For export macro
#include "vtkVariant.h"        // for vtkVariant

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;
class vtkTexture;
class vtkDataArray;

class VTKIONETCDF_EXPORT vtkXArrayAccessor : public vtkNetCDFAccessor
{
public:
  vtkTypeMacro(vtkXArrayAccessor, vtkNetCDFAccessor);
  static vtkXArrayAccessor* New();

  int close(int ncid) override;
  int open(const char* path, int omode, int* ncidp) override;
  const char* strerror(int ncerr1) override;

  int inq_attlen(int ncid, int varid, const char* name, size_t* lenp) override;

  int inq_dimlen(int ncid, int dimid, size_t* lenp) override;
  int inq_dimname(int ncid, int dimid, char* name) override;

  int inq_nvars(int ncid, int* nvarsp) override;
  int inq_ndims(int ncid, int* ndimsp) override;

  int inq_vardimid(int ncid, int varid, int* dimidsp) override;
  int inq_varid(int ncid, const char* name, int* varidp) override;
  int inq_varname(int ncid, int varid, char* name) override;
  int inq_varndims(int ncid, int varid, int* ndimsp) override;
  int inq_vartype(int ncid, int varid, int* typep) override;

  int get_att_text(int ncid, int varid, const char* name, char* value) override;
  int get_att_double(int ncid, int varid, const char* name, double* value) override;
  int get_att_float(int ncid, int varid, const char* name, float* value) override;

  bool GetCoordinates(int ncFD, int varId, std::vector<std::string>& coordName) override;
  bool NeedsFileName() override;

  // shallow copy
  int get_vars(int ncid, int varid, const size_t* startp, const size_t* countp,
    const ptrdiff_t* stridep, int vtkType, vtkIdType numberOfComponents, vtkIdType numberOfTuples,
    vtkDataArray* dataArray) override;
  // deep copy
  int get_vars(int ncid, int varid, const size_t* startp, const size_t* countp,
    const ptrdiff_t* stridep, void* ip) override;
  // deep copy
  int get_vars_double(int ncid, int varid, const size_t* startp, const size_t* countp,
    const ptrdiff_t* stridep, double* ip) override;
  int get_var_double(int ncid, int varid, double* ip) override;

  ///@{
  /**
   * Set dimensions and their length
   */
  void SetDim(const std::vector<std::string>& v);
  void SetDimLen(const std::vector<size_t>& v);
  ///@}

  ///@{
  /**
   * Set variables, variable attributes and types for the variable arrays.
   */
  void SetVar(const std::vector<std::string>& v, const std::vector<int>& is_coord);
  void SetVarValue(size_t varIndex, void* value);
  void SetAtt(size_t varIndex, std::string attributeName, const vtkVariant& var);
  void SetVarType(size_t varIndex, int nctype);
  /**
   * Check if this coordinate has one dim with the same name as the
   * coordinate.
   */
  bool IsCOARDSCoordinate(std::string);
  ///@}

  /**
   * Set dimensions for a variable
   */
  void SetVarDims(size_t varIndex, const std::vector<size_t>& dims);
  void SetVarCoords(size_t varIndex, const std::vector<size_t>& coords);

  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  void PrintVarValue(const char* name, int varid);
  void PrintVarValue(const char* name, int varid, const size_t* startp, const size_t* countp);
  bool IsContiguous(int varid, const size_t* startp, const size_t* countp);
  void GetContiguousStartSize(
    int varid, const size_t* startp, const size_t* countp, char*& arrayStart, vtkIdType& arraySize);
  void Copy(int varid, const size_t* startp, const size_t* countp, char* dst);
  std::vector<size_t> GetDimIncrement(int varid);
  bool DecrementAndUpdate(size_t varid, std::vector<size_t>& count, const size_t* startp,
    const size_t* count_p, const std::vector<size_t>& dimIncrement, char*& src);

  vtkXArrayAccessor() = default;
  ~vtkXArrayAccessor() override = default;

private:
  ///@{
  /**
   * Variables, their attributes and their dimensions indexed by varid
   * which is the index in the Var vector.
   * VarIndex sorts the names and points into the Var vector
   */
  std::vector<std::string> Var;
  std::vector<int> IsCoord;
  std::vector<char*> VarValue;
  std::map<std::string, size_t> VarIndex;
  std::vector<std::map<std::string, vtkVariant>> Att;
  std::vector<int> VarType;
  ///@}

  ///@{
  /**
   * Dimensions and their length
   * DimIndex sorts the names and points into the Dim vector
   */
  std::vector<std::string> Dim;
  std::map<std::string, size_t> DimIndex;
  std::vector<size_t> DimLen;
  ///@}
  /**
   * Var index -> vector of Dim or Coords indexes
   */
  std::vector<std::vector<size_t>> VarDims;
  std::vector<std::vector<size_t>> VarCoords;

  vtkXArrayAccessor(const vtkXArrayAccessor&) = delete;
  void operator=(const vtkXArrayAccessor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
