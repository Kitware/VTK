//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_rendering_raytracing_Loggable_h
#define viskores_rendering_raytracing_Loggable_h

#include <sstream>
#include <stack>

#include <viskores/Types.h>
#include <viskores/rendering/viskores_rendering_export.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

class VISKORES_RENDERING_EXPORT Logger
{
public:
  ~Logger();
  static Logger* GetInstance();
  void OpenLogEntry(const std::string& entryName);
  void CloseLogEntry(const viskores::Float64& entryTime);
  void Clear();
  template <typename T>
  void AddLogData(const std::string key, const T& value)
  {
    this->Stream << key << " " << value << "\n";
  }

  std::stringstream& GetStream();

protected:
  Logger();
  Logger(Logger const&);
  std::stringstream Stream;
  static class Logger* Instance;
  std::stack<std::string> Entries;
};
}
}
} // namespace viskores::rendering::raytracing
#endif
