
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
 LOWER_CASE_VARIABLE_NAMES | [on]/off | Convert all variable names read from input database to lowercase; replace ' ' with '_'
 USE_GENERIC_CANONICAL_NAMES | on/[off]  | use `block_{id}` as canonical name of an element block instead of the name (if any) stored on the database. The database name will be an alias.
 IGNORE_DATABASE_NAMES | on/[off] | Do not read any element block, nodeset, ... names if they exist on the database.  Use only the canonical generated names (entitytype + _ + id)
 IGNORE_ATTRIBUTE_NAMES   | on/[off] | Do not read the attribute names that may exist on an input database. Instead for an element block with N attributes, the fields will be named `attribute_1` ... `attribute_N`
 MINIMIZE_OPEN_FILES | on/[off] | If on, then close file after each timestep and then reopen on next output
 SERIALIZE_IO | integer | The number of files that will be read/written to simultaneously in a  parallel file-per-rank run.

## Auto-Decomposition-Related Properties

 Property        | Value  | Description
-----------------|:------:|-----------------------------------------------------------
MODEL_DECOMPOSITION_METHOD | {method} | Decompose a DB with type `MODEL` using `method`
RESTART_DECOMPOSITION_METHOD | {method} | Decompose a DB with type `RESTART_IN` using `method`
DECOMPOSITION_METHOD | {method} | Decompose all input DB using `method`
PARALLEL_CONSISTENCY | [on]/off | On if the client will call Ioss functions consistently on all processors. If off, then the auto-decomp and auto-join cannot be used.
RETAIN_FREE_NODES | [on]/off | In auto-decomp, will nodes not connected to any elements be retained.
LOAD_BALANCE_THRESHOLD | {real} [1.4] | CGNS-Structured only -- Load imbalance permitted Load on Proc / Avg Load
DECOMPOSITION_EXTRA | {name},{multiplier} | Specify the name of the element map or variable used if the decomposition method is `map` or `variable`.  If it contains a comma, the value following the comma is used to scale (divide) the values in the map/variable.  If it is 'auto', then all values will be scaled by `max_value/processorCount`

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
map        | the specified element map contains the mapping of elements to processor. Uses 'processor_id' map by default; otherwise specify name with `DECOMPOSITION_EXTRA` property
variable   | the specified element variable contains the mapping of elements to processor. Uses 'processor_id' variable by default; otherwise specify name with `DECOMPOSITION_EXTRA` property
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

## Properties related to field interpretation
 Property                 |   Value  | Description
--------------------------|:--------:|-----------------------------------------------------------
 ENABLE_FIELD_RECOGNITION | [on]/off | Does the IOSS library combine scalar fields into higher-order fields (tensor, vector) based on suffix interpretation.
 FIELD_SUFFIX_SEPARATOR   | char / '_'| The character that is used to separate the base field name from the suffix.  Default is underscore.
 FIELD_STRIP_TRAILING_BLANK | on / [off] | If `FIELD_SUFFIX_SEPARATOR` is empty and there are fields that end with an underscore, then strip the underscore. (`a_x`, `a_y`, `a_z` is vector field `a`).
 IGNORE_ATTRIBUTE_NAMES   | on/[off] | Do not read the attribute names that may exist on an input database. Instead for an element block with N attributes, the fields will be named `attribute_1` ... `attribute_N`
## Properties related to underlying file type (exodus only)

## Output Database-Related Properties
 Property        | Value  | Description
-----------------|:------:|-----------------------------------------------------------
 OMIT_QA_RECORDS | on/[off] | Do not output any QA records to the output database.
 OMIT_INFO_RECORDS | on/[off] | Do not output any INFO records to the output database.
 RETAIN_EMPTY_BLOCKS | on/[off] | If an element block is completely empty (on all ranks) should it be written to the output database.
 VARIABLE_NAME_CASE | upper/lower | Should all output field names be converted to uppercase or lowercase. Default is leave as is.
 FILE_TYPE             | [netcdf], netcdf4, netcdf-4, hdf5 | Underlying file type (bits on disk format)
 COMPRESSION_METHOD    | [zlib], szip | The compression method to use.  `szip` only available if HDF5 is built with that supported.
 COMPRESSION_LEVEL     | [0]-9    | If zlib: In the range [0..9]. A value of 0 indicates no compression, will automatically set `file_type=netcdf4`, recommend <=4
 COMPRESSION_LEVEL     | 4-32 | If szip: An even number in the range 4-32, will automatically set `file_type=netcdf4`.
 COMPRESSION_SHUFFLE   | on/[off] |to enable/disable hdf5's shuffle compression algorithm.
 MAXIMUM_NAME_LENGTH   | [32]     | Maximum length of names that will be returned/passed via api call.
 APPEND_OUTPUT         | on/[off] | Append output to end of existing output database
 APPEND_OUTPUT_AFTER_STEP | {step}| Max step to read from an input db or a db being appended to (typically used with APPEND_OUTPUT)
 APPEND_OUTPUT_AFTER_TIME | {time}| Max time to read from an input db or a db being appended to (typically used with APPEND_OUTPUT)
 FILE_PER_STATE        | on/[off] | Put data for each output timestep into a separate file.
 CYCLE_COUNT           | {cycle}  | If using FILE_PER_STATE, then use {cycle} different files and then overwrite.
 OVERLAY_COUNT         | {overlay}| If using FILE_PER_STATE, then put {overlay} timesteps worth of data into each file before going to next file.
 ENABLE_DATAWARP       | on/[off] | If the system supports Cray DataWarp (burst buffer), should it be used for buffering output files.

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
MINIMAL_NEMESIS_INFO | on/[off] | special case, omit all nemesis data except for nodal communication map
OMIT_EXODUS_NUM_MAPS | on/[off] | special case, do not output the node and element numbering map.


## Debugging / Profiling

  Property | Value    | Description
 ----------|:--------:|------------
 LOGGING   | on/[off] | enable/disable logging of field input/output
 ENABLE_TRACING | on/[off] | show memory and elapsed time during some IOSS calls (mainly decomp).
 DECOMP_SHOW_PROGRESS | on/[off] | use `ENABLE_TRACING`.
 DECOMP_SHOW_HWM      | on/[off] | show high-water memory during autodecomp
 IOSS_TIME_FILE_OPEN_CLOSE | on/[off] | show elapsed time during parallel-io file open/close/create
 CHECK_PARALLEL_CONSISTENCY | on/[off] | check Ioss::GroupingEntity parallel consistency
 TIME_STATE_INPUT_OUTPUT | on/[off] | show the elapsed time for reading/writing each timestep's data

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

     Copyright(C) 1999-2021 National Technology & Engineering Solutions
     of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
     NTESS, the U.S. Government retains certain rights in this software.

     See packages/seacas/LICENSE for details

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
