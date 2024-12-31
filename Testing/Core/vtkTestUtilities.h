// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTestUtilities
 * @brief   Utility functions used for regression testing.
 *
 * vtkTestUtilities provides methods to perform common testing operations.
 * These include getting a command line argument or an environment variable,
 * or a default value. Particularly, there are specialized methods to get the
 * root directory for VTK Data, expanding a filename with this root directory.
 *
 * It also provides methods for testing whether two `vtkDataObjects`,
 * `vtkFieldData`, or `vtkAbstractArray` are equal up to numerical precision.
 *
 * Near-equality is defined as follows for floating point tuples u and v represented as vectors:
 *
 * \f[ || u - v ||^2 < k \epsilon (||u||^2 + ||v||^2) \f]
 *
 * Where \f$\epsilon\f$ is the smallest
 * positive floating-point number such that \f$ 1.0 + \epsilon \neq 1.0\f]$, and \f$k\f$ is what we
 * call the tolerance factor in this class, which is a `double` greater or equal than 1.0. The
 * tolerance factor shall not be strictly smaller than 1.0 as we would be below numerical precision.
 * A typical range for it is between 1.0 and 100.0.
 */

#ifndef vtkTestUtilities_h
#define vtkTestUtilities_h

#include "vtkSystemIncludes.h"
#include "vtkTestingCoreModule.h" // for export macro

#ifdef __EMSCRIPTEN__
#include "vtkEmscriptenTestUtilities.h"
#endif

#if defined(_MSC_VER)           /* Visual C++ (and Intel C++) */
#pragma warning(disable : 4996) // 'function': was declared deprecated
#endif

VTK_ABI_NAMESPACE_BEGIN

class vtkAbstractArray;
class vtkDataObject;
class vtkDataSet;
class vtkFieldData;
class vtkUnsignedCharArray;

struct VTKTESTINGCORE_EXPORT vtkTestUtilities
{
  /**
   * Function necessary for accessing the root directory for VTK data. Try the
   * -D command line argument or VTK_DATA_ROOT or a default value. The returned
   * string has to be deleted (with delete[]) by the user.
   */
  static inline char* GetDataRoot(int argc, char* argv[]);

  /**
   * Given a file name, this function returns a new string which is (in theory)
   * the full path. This path is constructed by prepending the file name with a
   * command line argument (-D path) or VTK_DATA_ROOT env. variable. If slash
   * is true, appends a slash to the resulting string. The returned string has
   * to be deleted (with delete[]) by the user.
   */
  static inline VTK_FILEPATH char* ExpandDataFileName(
    int argc, char* argv[], VTK_FILEPATH const char* fname, int slash = 0);
  /**
   * Function returning either a command line argument, an environment variable
   * or a default value. The returned string has to be deleted (with delete[])
   * by the user.
   */
  static inline char* GetArgOrEnvOrDefault(
    const char* arg, int argc, char* argv[], const char* env, const char* def);

  ///@{
  /**
   * Given a file name, this function returns a new string which is (in theory)
   * the full path. This path is constructed by prepending the file name with a
   * command line argument, an environment variable or a default value. If
   * slash is true, appends a slash to the resulting string. The returned
   * string has to be deleted (with delete[]) by the user.
   */
  static inline VTK_FILEPATH char* ExpandFileNameWithArgOrEnvOrDefault(const char* arg, int argc,
    char* argv[], const char* env, const char* def, VTK_FILEPATH const char* fname, int slash = 0);
  ///@}

  /**
   * Returns true if the 2 input `vtkDataObject` are identical. For the `vtkDataSet` case, this
   * function is invariant to point ordering and cell ordering, as well as point permutations within
   * cells, as long as they span the same topology. If the 2 inputs do not share the same typing,
   * this function will return false. In the case of `vtkDataSet` inputs, the cell types must match.
   *
   * Some ghost elements are discarded from the comparison, however the ghost arrays
   * (accessible through `vtkFieldData::GetGhostArray()`) must match. Within a `vtkFieldData` stored
   * inside an input `vtkDataObject`, ghosts elements are discarded if their bit representations
   * intersect `vtkFieldData::GetGhostsToSkip()`.
   *
   * @sa vtkCellData
   * @sa vtkDataSetAttributes
   * @sa vtkFieldData
   * @sa vtkPointData
   */
  static bool CompareDataObjects(
    vtkDataObject* do1, vtkDataObject* do2, double toleranceFactor = 1.0);

  /**
   * Returns true if the 2 input `vtkDataSet` share the same point positions and `vtkPointData` at
   * those positions. This function is invariant to the point ordering between the 2 inputs.
   *
   * Some ghost elements are discarded from the comparison, however the ghost arrays
   * (accessible through `vtkFieldData::GetGhostArray()`) must match. Within a `vtkFieldData` stored
   * inside an input `vtkDataObject`, ghosts elements are discarded if their bit representations
   * intersect `vtkFieldData::GetGhostsToSkip()`.
   *
   * @sa vtkDataSetAttributes
   * @sa vtkFieldData
   * @sa vtkPointData
   */
  static bool ComparePoints(vtkDataSet* ds1, vtkDataSet* ds2, double toleranceFactor = 1.0);

  /**
   * Returns true if the 2 input `vtkDataObject`'s cells are identical, assuming the input have
   * cells. This function supports `vtkDataSet` and `vtkHyperTreeGrid` inputs, and is
   * invariant to their cell ordering, as well as point permutations within
   * cells, as long as they span the same topology.
   *
   * Some ghost elements are discarded from the comparison, however the ghost arrays
   * (accessible through `vtkFieldData::GetGhostArray()`) must match. Within a `vtkFieldData` stored
   * inside an input `vtkDataObject`, ghosts elements are discarded if their bit representations
   * intersect `vtkFieldData::GetGhostsToSkip()`.
   *
   * @sa vtkCellData
   * @sa vtkDataSetAttributes
   * @sa vtkFieldData
   */
  static bool CompareCells(vtkDataObject* do1, vtkDataObject* do2, double toleranceFactor = 1.0);

  /**
   * Returns true if the 2 input `vtkFieldData` are identical.
   *
   * Some ghost elements are discarded from the comparison, however the ghost arrays
   * (accessible through `vtkFieldData::GetGhostArray()`) must match. Within a `vtkFieldData` stored
   * inside an input `vtkDataObject`, ghosts elements are discarded if their bit representations
   * intersect `vtkFieldData::GetGhostsToSkip()`.
   *
   * @sa vtkCellData
   * @sa vtkDataSetAttributes
   * @sa vtkFieldData
   */
  static bool CompareFieldData(vtkFieldData* fd1, vtkFieldData* fd2, double toleranceFactor = 1.0);

  /**
   * Returns true if the 2 input `vtkAbstractArray` are identical. The 2 arrays are assumed
   * to be ordered in the same manner. One can provide an input ghost array accompanied with
   * a bit mask `ghostsToSkip`, which will make this function skip corresponding tuples
   * in the input arrays.
   */
  static bool CompareAbstractArray(vtkAbstractArray* array1, vtkAbstractArray* array2,
    double toleranceFactor = 1.0, vtkUnsignedCharArray* ghosts = nullptr,
    unsigned char ghostsToSkip = 0);

#ifdef __EMSCRIPTEN__
  static void PreloadDataFile(const char* fileName, const char* sandboxName);
#endif
};

inline char* vtkTestUtilities::GetDataRoot(int argc, char* argv[])
{
  return vtkTestUtilities::GetArgOrEnvOrDefault(
    "-D", argc, argv, "VTK_DATA_ROOT", "../../../../VTKData");
}

inline char* vtkTestUtilities::ExpandDataFileName(
  int argc, char* argv[], const char* fname, int slash)
{
#ifdef __EMSCRIPTEN__
  // determine where the file is located on the host file system using args/env.
  char* hostPath = vtkTestUtilities::ExpandFileNameWithArgOrEnvOrDefault(
    "-D", argc, argv, "VTK_DATA_ROOT", "../../../../VTKData", fname, slash);
  // preload file from host into the sandbox
  const size_t n = strlen(fname) + 1;
  char* sandboxPath = new char[n];
  strncpy(sandboxPath, fname, n);
  vtkEmscriptenTestUtilities::PreloadDataFile(hostPath, sandboxPath);
  delete[] hostPath;
  return sandboxPath;
#else
  return vtkTestUtilities::ExpandFileNameWithArgOrEnvOrDefault(
    "-D", argc, argv, "VTK_DATA_ROOT", "../../../../VTKData", fname, slash);
#endif
}

inline char* vtkTestUtilities::GetArgOrEnvOrDefault(
  const char* arg, int argc, char* argv[], const char* env, const char* def)
{
  int index = -1;

  for (int i = 0; i < argc; i++)
  {
    if (strcmp(arg, argv[i]) == 0 && i < argc - 1)
    {
      index = i + 1;
    }
  }

  char* value;

  if (index != -1)
  {
    value = new char[strlen(argv[index]) + 1];
    strcpy(value, argv[index]);
  }
  else
  {
    char* foundenv = getenv(env);
    if (foundenv)
    {
      value = new char[strlen(foundenv) + 1];
      strcpy(value, foundenv);
    }
    else if (def)
    {
      value = new char[strlen(def) + 1];
      strcpy(value, def);
    }
    else
    {
      value = nullptr;
    }
  }

  return value;
}

inline char* vtkTestUtilities::ExpandFileNameWithArgOrEnvOrDefault(const char* arg, int argc,
  char* argv[], const char* env, const char* def, const char* fname, int slash)
{
  char* fullName;

  char* value = vtkTestUtilities::GetArgOrEnvOrDefault(arg, argc, argv, env, def);
  if (value)
  {
    fullName = new char[strlen(value) + strlen(fname) + 2 + static_cast<size_t>(slash ? 1 : 0)];
    fullName[0] = 0;
    strcat(fullName, value);
    size_t len = strlen(fullName);
    fullName[len] = '/';
    fullName[len + 1] = 0;
    strcat(fullName, fname);
  }
  else
  {
    fullName = new char[strlen(fname) + 1 + static_cast<size_t>(slash ? 1 : 0)];
    strcpy(fullName, fname);
  }

  if (slash)
  {
    strcat(fullName, "/");
  }

  delete[] value;

  return fullName;
}

VTK_ABI_NAMESPACE_END
#endif // vtkTestUtilities_h
// VTK-HeaderTest-Exclude: vtkTestUtilities.h
