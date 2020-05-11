// Copyright(C) 1999-2017 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of NTESS nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef IOSS_Ioss_IOFactory_h
#define IOSS_Ioss_IOFactory_h

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <string>

#include <Ioss_DBUsage.h>
#include <Ioss_PropertyManager.h>

#include <map>
#include <vector>

/** \brief The main namespace for the Ioss library.
 */
namespace Ioss {

  class IOFactory;

  using NameList     = std::vector<std::string>;
  using IOFactoryMap = std::map<std::string, IOFactory *, std::less<std::string>>;

  class DatabaseIO;

  /** \brief The main public user interface for creating Ioss::DatabaseIO objects.
   */
  class IOFactory
  {
  public:
    virtual ~IOFactory() = default;
    static DatabaseIO *create(const std::string &type, const std::string &filename,
                              DatabaseUsage db_usage, MPI_Comm communicator = MPI_COMM_WORLD,
                              const Ioss::PropertyManager &properties = Ioss::PropertyManager());

    static int  describe(NameList *names);
    static void clean();
    static void show_configuration();

  protected:
    explicit IOFactory(const std::string &type);

    virtual DatabaseIO *make_IO(const std::string &filename, DatabaseUsage db_usage,
                                MPI_Comm                     communicator,
                                const Ioss::PropertyManager &properties) const = 0;

    virtual void show_config() const {}

    static void alias(const std::string &base, const std::string &syn);

  private:
    static IOFactoryMap *registry();
  };
} // namespace Ioss
#endif
