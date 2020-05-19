// clang-format off

/*!  \mainpage IOSS API Documentation

\section avail Availability

The IOSS library source code is available on Github at
https://github.com/gsjaardema/seacas

For bug reports, documentation errors, and enhancement suggestions, contact:
- Gregory D. Sjaardema
- WEB:   https://github.com/gsjaardema/seacas/issues
- EMAIL: gdsjaar@sandia.gov
- EMAIL: gsjaardema@gmail.com
- PHONE: (505) 844-2701 (office)

\section properties Properties

## General Properties

  Property | Value    | Description
 ----------|:--------:|------------
 LOGGING   | on/[off] | enable/disable logging of field input/output
 LOWER_CASE_VARIABLE_NAMES | [on]/off | Convert all variable names on database to lowercase; replace ' ' with '_'
 USE_GENERIC_CANONICAL_NAMES | on/[off]  | use `block_{id}` as canonical name of an element block instead of the name (if any) stored on the database. The database name will be an alias.
 MINIMIZE_OPEN_FILES | on/[off] | If on, then close file after each timestep and then reopen on next output

## Auto-Decomposition-Related Properties

 Property        | Value  | Description
-----------------|:------:|-----------------------------------------------------------
MODEL_DECOMPOSITION_METHOD | {method} | Decompose a DB with type `MODEL` using `method`
RESTART_DECOMPOSITION_METHOD | {method} | Decompose a DB with type `RESTART_IN` using `method`
DECOMPOSITION_METHOD | {method} | Decompose all input DB using `method`
PARALLEL_CONSISTENCY | [on]/off | On if the client will call Ioss functions consistently on all processors. If off, then the auto-decomp and auto-join cannot be used.
RETAIN_FREE_NODES | [on]/off | In auto-decomp, will nodes not connected to any elements be retained.
LOAD_BALANCE_THRESHOLD | {real} [1.4] | CGNS-Structured only -- Load imbalance permitted Load on Proc / Avg Load

### Valid values for Decomposition Method

Method     | Description
:---------:|-------------------
rcb        | recursive coordinate bisection
rib        | recursive inertial bisection
hsfc       | hilbert space-filling curve
metis_sfc  | metis space-filling-curve
kway       | metis kway graph-based
kway_geom  | metis kway graph-based method with geometry speedup
linear     | elements in order first n/p to proc 0, next to proc 1.
cyclic     | elements handed out to id % proc_count
random     | elements assigned randomly to processors in a way that preserves balance (do not use for a real run)
external   | Files are decomposed externally into a file-per-processor in a parallel run.

## Output File Composition -- Single File output from parallel run instead of file-per-processor

 Property        | Value
-----------------|:------:
COMPOSE_RESTART  | on/[off]
COMPOSE_RESULTS  | on/[off]
PARALLEL_IO_MODE | netcdf4, hdf5, pnetcdf, (mpiio and mpiposix are deprecated)

## Properties Related to byte size of reals and integers

 Property              | Value  | Description
-----------------------|:------:|-----------------------------------------------------------
 INTEGER_SIZE_DB       | [4] / 8 | byte size of integers stored on the database.
 INTEGER_SIZE_API      | [4] / 8 | byte size of integers used in api functions.
 REAL_SIZE_DB          | 4 / [8] | byte size of floating point stored on the database.
 REAL_SIZE_API         | 4 / [8] | byte size of floating point used in api functions.

## Properties related to underlying file type (exodus only)

 Property              | Value  | Description
-----------------------|:------:|-----------------------------------------------------------
 FILE_TYPE            | [netcdf], netcdf4, netcdf-4, hdf5 | Underlying file type (bits on disk format)
 COMPRESSION_LEVEL     | [0]-9    | In the range [0..9]. A value of 0 indicates no compression, will automatically set `file_type=netcdf4`, recommend <=4
 COMPRESSION_SHUFFLE   | on/[off] |to enable/disable hdf5's shuffle compression algorithm.
 MAXIMUM_NAME_LENGTH   | [32]     | Maximum length of names that will be returned/passed via api call.
 APPEND_OUTPUT         | on/[off] | Append output to end of existing output database
 APPEND_OUTPUT_AFTER_STEP | {step}| Max step to read from an input db or a db being appended to (typically used with APPEND_OUTPUT)
 APPEND_OUTPUT_AFTER_TIME | {time}| Max time to read from an input db or a db being appended to (typically used with APPEND_OUTPUT)

## Properties for the heartbeat output
 Property              | Value  | Description
-----------------------|:------:|-----------------------------------------------------------
  FLUSH_INTERVAL       | int   | Minimum time interval between flushing heartbeat data to disk.  Default is 10 seconds
  TIME_STAMP_FORMAT    | [%H:%M:%S] | Format used to format time stamp.  See strftime man page
  SHOW_TIME_STAMP      | on/off | Should the output lines be preceded by the timestamp
  PRECISION            | 0..16 [5] | Precision used for floating point output.
  FIELD_WIDTH          | 0.. |  Width of an output field. If 0, then use natural width.
  SHOW_LABELS          | on/[off]  | Should each field be preceded by its name (ke=1.3e9, ie=2.0e9)
  SHOW_LEGEND          | [on]/off  | Should a legend be printed at the beginning of the output showing the field names for each column of data.
  SHOW_TIME_FIELD      | on/[off]  | Should the current analysis time be output as the first field.


## Experimental

 Property              | Value  | Description
-----------------------|:------:|-----------------------------------------------------------
MEMORY_READ        | on/[off]   | experimental
MEMORY_WRITE       | on/[off]   | experimental
ENABLE_FILE_GROUPS | on/[off]   | experimental

## Debugging / Profiling

  Property | Value    | Description
 ----------|:--------:|------------
 LOGGING   | on/[off] | enable/disable logging of field input/output
 ENABLE_TRACING | on/[off] | show memory and elapsed time during some IOSS calls (mainly decomp).
 DECOMP_SHOW_PROGRESS | on/[off] | use `ENABLE_TRACING`.
 DECOMP_SHOW_HWM      | on/[off] | show high-water memory during autodecomp
 IOSS_TIME_FILE_OPEN_CLOSE | on/[off] | show elapsed time during parallel-io file open/close/create
 CHECK_PARALLEL_CONSISTENCY | on/[off] | check Ioss::GroupingEntity parallel consistency

## Setting properties via an environment variable

Although the properties are usually accessed internally in the
application calling the IOSS library, it is possible to set the
properties externally prior to running the application via the setting
of the environment variable `IOSS_PROPERTIES`.  The value of the
variable is one or more colon-separated property/property-value pairs.
For example, to set the `DECOMPOSITION_METHOD` and the `FILE_TYPE`
externally, the following would be used:
```
    export IOSS_PROPERTIES="DECOMPOSITION_METHOD=rib:FILE_TYPE=netcdf4"
```
If the environment variable is set correctly, there should be an
informational message output during running of the application similar
to:
```
	IOSS: Adding property 'DECOMPOSITION_METHOD' with value 'rib'
	IOSS: Adding property 'FILE_TYPE' with value 'netcdf4'
```

\section license License
The IOSS library is licensed under the BSD open source license.

     Copyright (c) 1999-2017 National Technology & Engineering Solutions
     of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
     NTESS, the U.S. Government retains certain rights in this software.

     Redistribution and use in source and binary forms, with or without
     modification, are permitted provided that the following conditions are
     met:

     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

     * Redistributions in binary form must reproduce the above
       copyright notice, this list of conditions and the following
       disclaimer in the documentation and/or other materials provided
       with the distribution.

     * Neither the name of NTESS nor the names of its
       contributors may be used to endorse or promote products derived
       from this software without specific prior written permission.

     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
     A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
     OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
     LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
     DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
     THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

// clang-format on
