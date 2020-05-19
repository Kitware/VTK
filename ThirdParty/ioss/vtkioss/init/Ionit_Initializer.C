// Copyright(C) 1999-2017, 2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ionit_Initializer.h"
#include <Ioss_CodeTypes.h>

#if defined(SEACAS_HAVE_EXODUS)
#include <exodus/Ioex_IOFactory.h>
#endif

#include <gen_struc/Iogs_DatabaseIO.h>
#include <generated/Iogn_DatabaseIO.h>
#include <heartbeat/Iohb_DatabaseIO.h>

#ifdef HAVE_SEACASIOSS_ADIOS2
#include <adios/Ioad_Initializer.h>
#endif

#if defined(SEACAS_HAVE_PAMGEN)
#include <pamgen/Iopg_DatabaseIO.h>
#endif

#if defined(SEACAS_HAVE_DATAWAREHOUSE)
#include <data_warehouse/Iodw_DatabaseIO.h>
#endif

#if defined(SEACAS_HAVE_CGNS)
#include <cgns/Iocgns_IOFactory.h>
#endif

#include <Ioss_ConcreteVariableType.h>
#include <Ioss_Initializer.h>
#include <transform/Iotr_Initializer.h>
#ifndef _MSC_VER
//#include <visualization/Iovs_IOFactory.h>
#endif

namespace {
#if defined(IOSS_THREADSAFE)
  std::mutex m_;
#endif
} // namespace

namespace Ioss {
  namespace Init {
    Initializer &Initializer::initialize_ioss()
    {
      static Initializer ionit;
      return ionit;
    }

    /** \brief Initialize the Ioss library.
     *
     *  Calls appropriate internal functions and methods to
     *  initialize the Ioss library. Initializes all database
     *  types.
     */
    Initializer::Initializer()
    {
      IOSS_FUNC_ENTER(m_);

#if defined(SEACAS_HAVE_EXODUS)
      Ioex::IOFactory::factory(); // Exodus
#endif
#if defined(SEACAS_HAVE_PAMGEN)
      Iopg::IOFactory::factory(); // Pamgen
#endif
#if defined(SEACAS_HAVE_DATAWAREHOUSE)
      Iodw::IOFactory::factory(); // DataWarehouse
#endif
#if defined(SEACAS_HAVE_CGNS)
      Iocgns::IOFactory::factory();
#endif

#ifndef _MSC_VER
      //Iovs::IOFactory::factory(); // Visualization
#endif
      Iohb::IOFactory::factory(); // HeartBeat
      Iogn::IOFactory::factory(); // Generated
      Iogs::IOFactory::factory(); // Structured Mesh Generator
      Ioss::StorageInitializer();
      Ioss::Initializer();
      Iotr::Initializer();
#ifdef HAVE_SEACASIOSS_ADIOS2
      Ioad::Initializer(); // ADIOS2
#endif
    }

    Initializer::~Initializer()
    {
      try {
        IOSS_FUNC_ENTER(m_);
        Ioss::IOFactory::clean();
        // Put code here that should run after sierra is finished
        // executing...
      }
      catch (...) {
      }
    }
  } // namespace Init
} // namespace Ioss
