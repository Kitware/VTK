/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5HUT_H
#define __H5HUT_H

#if !defined(H5HUT_API_VERSION)
#define H5HUT_API_VERSION 2
#endif

// XXX(kitware) include mangling header
#include "vtk_h5hut_mangle.h"

#include "H5_attachments.h"
#include "H5_file.h"
#include "H5_model.h"
#include "H5_file_attribs.h"
#include "H5_step_attribs.h"
#include "H5_log.h"
#include "H5_err.h"

#include "H5Part_io.h"
#include "H5Part_model.h"

/**
  \ingroup h5block_model

  \note
  Different field sizes are allowed in the same step/iteration.

  \note
  The same layout can be used, if the size of the field matches the
  size of the layout.  If the size of the layout doesn't match the
  size of the field, an error will be indicated. 
 
  \note In write mode views might be reduced to make them
  non-overlaping, i.e. ghost-zones are eliminated. This may shrink
  views in an unexpected way.

  \todo
  check whether layout is reasonable
*/

#include "H5Block_attribs.h"
#include "H5Block_model.h"
#include "H5Block_io.h"

#include "H5Fed_adjacency.h"
#include "H5Fed_model.h"
#include "H5Fed_retrieve.h"
#include "H5Fed_store.h"
#include "H5Fed_tags.h"

#endif
