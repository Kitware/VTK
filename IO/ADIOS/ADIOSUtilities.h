#ifndef __ADIOSUtilities_h
#define __ADIOSUtilities_h

#include <stdint.h>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <vtkType.h>
#include <adios.h>
#include <adios_read.h>


class ADIOSUtilities
{
private:
  template<typename T>
  static inline std::string ToString(const T &value)
  {
    std::stringstream ss;
    ss << value;
    return ss.str();
  }

public:

  // Description:
  // Map C and C++ primitive datatypes into ADIOS datatypes
  template<typename TN>
  struct TypeNativeToADIOS
  {
    static ADIOS_DATATYPES T;
  };

  // Description:
  // Map C and C++ primitive datatypes into VTK datatypes
  template<typename TN>
  struct TypeNativeToVTK
  {
    static int T;
  };

  // Description:
  // Map VTK datatypes into ADIOS datatypes
  static ADIOS_DATATYPES TypeVTKToADIOS(int tv);

  // Description:
  // Map ADIOS datatypes into VTK datatypes
  static int TypeADIOSToVTK(ADIOS_DATATYPES ta);

  // Description:
  // Map type sizes
  static size_t TypeSize(ADIOS_DATATYPES ta);

  static const int64_t ADIOS_INVALID_INT64;

  // Definition
  // Test error codes for expected value
  template<typename T>
  static inline void TestReadErrorEq(const T &expected, const T &actual)
  {
    if(expected != actual)
      {
      throw std::runtime_error(adios_errmsg());
      }
  }

  // Definition
  // Test error codes for unexpected value
  template<typename T>
  static inline void TestReadErrorNe(const T &notExpected, const T &actual)
  {
    if(notExpected == actual)
      {
      throw std::runtime_error(adios_errmsg());
      }
  }

  // Definition
  // Test error codes for expected value
  template<typename T>
  static inline void TestWriteErrorEq(const T &expected, const T &actual)
  {
    if(expected != actual)
      {
      throw std::runtime_error(adios_get_last_errmsg());
      }
  }

  // Definition
  // Test error codes for unexpected value
  template<typename T>
  static inline void TestWriteErrorNe(const T &notExpected, const T &actual)
  {
    if(notExpected == actual)
      {
      throw std::runtime_error(adios_get_last_errmsg());
      }
  }
};
#endif
// VTK-HeaderTest-Exclude: ADIOSUtilities.h
