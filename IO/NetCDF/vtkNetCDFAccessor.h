// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkNetCDFAccessor_h
#define vtkNetCDFAccessor_h

#include "vtkIONetCDFModule.h" // For export macro
#include "vtkObject.h"
#include <stddef.h> // for ptrdiff_t

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkDoubleArray;

class VTKIONETCDF_EXPORT vtkNetCDFAccessor : public vtkObject
{
public:
  vtkTypeMacro(vtkNetCDFAccessor, vtkObject);
  static vtkNetCDFAccessor* New();

  virtual int close(int ncid);
  virtual int open(const char* path, int omode, int* ncidp);
  virtual const char* strerror(int ncerr1);

  virtual int inq_attlen(int ncid, int varid, const char* name, size_t* lenp);

  virtual int inq_dimlen(int ncid, int dimid, size_t* lenp);
  virtual int inq_dimname(int ncid, int dimid, char* name);

  virtual int inq_nvars(int ncid, int* nvarsp);
  virtual int inq_ndims(int ncid, int* ndimsp);

  virtual int inq_vardimid(int ncid, int varid, int* dimidsp);
  virtual int inq_varid(int ncid, const char* name, int* varidp);
  virtual int inq_varname(int ncid, int varid, char* name);
  virtual int inq_varndims(int ncid, int varid, int* ndimsp);
  virtual int inq_vartype(int ncid, int varid, int* typep);

  virtual int get_att_text(int ncid, int varid, const char* name, char* value);
  virtual int get_att_double(int ncid, int varid, const char* name, double* value);
  virtual int get_att_float(int ncid, int varid, const char* name, float* value);

  virtual int get_vars(int ncid, int varid, const size_t* startp, const size_t* countp,
    const ptrdiff_t* stridep, int vtkType, vtkIdType numberOfComponents, vtkIdType numberOfTuples,
    vtkDataArray* dataArray);
  virtual int get_vars(int ncid, int varid, const size_t* startp, const size_t* countp,
    const ptrdiff_t* stridep, void* ip);

  virtual int get_vars_double(int ncid, int varid, const size_t* startp, const size_t* countp,
    const ptrdiff_t* stridep, double* ip);
  virtual int get_var_double(int ncid, int varid, double* ip);

  virtual bool GetCoordinates(int ncFD, int varId, std::vector<std::string>& coordName);
  virtual bool NeedsFileName();

  bool ReadTextAttribute(int ncFD, int varId, const char* name, std::string& result);

  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkNetCDFAccessor() = default;
  ~vtkNetCDFAccessor() override = default;

private:
  vtkNetCDFAccessor(const vtkNetCDFAccessor&) = delete;
  void operator=(const vtkNetCDFAccessor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
