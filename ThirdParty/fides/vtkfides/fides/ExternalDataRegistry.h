//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_io_ExternalDataRegistry_H_
#define fides_io_ExternalDataRegistry_H_

#include <fides/DataContainer.h>

#include <memory>
#include <string>

namespace fides
{
namespace io
{

class FIDES_EXPORT ExternalDataRegistry
{
public:
  static ExternalDataRegistry& Instance();
  ~ExternalDataRegistry();

  /// Register a data container and receive a retrieval token.
  /// Ownership semantics (shared vs external) are handled by the
  /// DataContainer wrapper prior to registration.
  /// \param data Shared pointer to the opaque DataContainer
  /// \return Token that can be used to retrieve the data from the registry
  std::string Register(std::shared_ptr<fides::DataContainer> data);

  /// Retrieve the previously registered data container
  /// @param token Token received at data registration time
  /// @return Associated DataContainer
  std::shared_ptr<fides::DataContainer> Get(const std::string& token) const;

  /// Unregister previously registered data container
  /// @param token Token received at data registration time
  void Unregister(const std::string& token);

private:
  ExternalDataRegistry();

  class InternalImpl;
  std::unique_ptr<InternalImpl> Internals;
};

}
}

#endif // fides_io_ExternalDataRegistry_H_
