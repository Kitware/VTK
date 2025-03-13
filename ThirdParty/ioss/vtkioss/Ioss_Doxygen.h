
// clang-format off

/*!  \mainpage IOSS API Documentation

\section avail Availability

The IOSS library source code is available on Github at
https://github.com/sandialabs/seacas

For bug reports, documentation errors, and enhancement suggestions, contact:
- Gregory D. Sjaardema
- WEB:   https://github.com/sandialabs/seacas
- EMAIL: gdsjaar@sandia.gov
- EMAIL: gsjaardema@gmail.com

\section db_types Database Types

The IOSS system supports multiple database formats.  The default format is the Sandia-developed Exodus format.  The list below shows all supported
database input and/or output formats.  Not all of these may be available on all builds; the supported databases on a particular installation can be
determined by running the `io_info --config` program.

Type             | Input/Output  | Description
-----------------|---------------|--------------------------
exodus           | Input/Output  | Sandia-developed database system for unstructured mesh input/output (https://sandialabs.github.io/seacas-docs/sphinx/html/index.html#exodus-library)
cgns             | Input/Output  | CFD General Notation System (https://cgns.github.io/)
generated        | Input         | Generate an unstructured hex/shell mesh using a token string (Iogn::GeneratedMesh::GeneratedMesh)
textmesh         | Input         | Generate an unstructured mesh using a token string
heartbeat        | Output        | A text based output for global values
pamgen           | Input         | (https://trilinos.github.io/pamgen.html)
gen_struc        | Input         | Generate a structured mesh using a token string (IxJxK)
catalyst_exodus  | Output        | Visualization pipeline, exodus-based
catalyst_cgns    | Output        | Visualization pipeline, cgns-based
null             | Output        | No data written to disk, no calculations done by ioss
exonull          | Output        | No data written to disk, but uses all of the exodus io database infrastructure/calculations
adios            | Input/Output  | Adaptable Input/Output system, (https://adios2.readthedocs.io/en/latest/)
faodel           | Input/Output  | (https://github.com/faodel/faodel)
exodusii         | Input/Output  | alias for exodus
genesis          | Input/Output  | alias for exodus
par_cgns         | Input/Output  | alias for parallel CGNS

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

## Properties related to field and sideset/surface interpretation
 Property                 |   Value  | Description
--------------------------|:--------:|-----------------------------------------------------------
 ENABLE_FIELD_RECOGNITION | [on]/off | Does the IOSS library combine scalar fields into higher-order fields (tensor, vector) based on suffix interpretation.
 IGNORE_REALN_FIELDS      | [off]/on | Do not recognize var_1, var_2, ..., var_n as an n-component field.  Keep as n scalar fields.  Currently ignored for composite fields.
 FIELD_SUFFIX_SEPARATOR   | char / '_'| The character that is used to separate the base field name from the suffix.  Default is underscore.
 FIELD_STRIP_TRAILING_UNDERSCORE | on / [off] | If `FIELD_SUFFIX_SEPARATOR` is empty and there are fields that end with an underscore, then strip the underscore. (`a_x`, `a_y`, `a_z` is vector field `a`).
 IGNORE_ATTRIBUTE_NAMES   | on/[off] | Do not read the attribute names that may exist on an input database. Instead for an element block with N attributes, the fields will be named `attribute_1` ... `attribute_N`
 SURFACE_SPLIT_TYPE       | {type} | Specify how to split sidesets into homogeneous sideblocks. Either an integer or string: 1 or `TOPOLOGY`, 2 or `BLOCK`, 3 or `NO_SPLIT`.  Default is `TOPOLOGY` if not specified.
 DUPLICATE_FIELD_NAME_BEHAVIOR | {behavior} | Determine how to handle duplicate incompatible fields on a database.  Valid values are `IGNORE`, `WARNING`, or `ERROR` (default).  An incompatible field is two or more fields with the same name, but different sizes or roles or types.

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
 CYCLE_COUNT           | {cycle}  | If using FILE_PER_STATE, then use {cycle} different files and then overwrite. Otherwise, there will be a maximum of {cycle} time steps in the file. See below.
 OVERLAY_COUNT         | {overlay}| If using FILE_PER_STATE, then put {overlay} timesteps worth of data into each file before going to next file. Otherwise, each output step in the file will be overwritten {overlay} times. See below.
 ENABLE_DATAWARP       | on/[off] | If the system supports Cray DataWarp (burst buffer), should it be used for buffering output files.

### Cycle and Overlay Behavior:
(Properties `CYCLE_COUNT`, `OVERLAY_COUNT`, and `FILE_PER_STATE`)
The `overlay` specifies the number of output steps which will be
overlaid on top of the currently written step before advancing to the
next step on the database.

For example, if output every 0.1 seconds and the overlay count is
specified as 2, then IOSS will write time 0.1 to step 1 of the
database.  It will then write 0.2 and 0.3 also to step 1.  It will
then increment the database step and write 0.4 to step 2 and overlay
0.5 and 0.6 on step 2. At the end of the analysis, (assuming it runs
to completion), the database would have times 0.3, 0.6, 0.9,
... However, if there were a problem during the analysis, the last
step on the database would contain an intermediate step.

The `cycle_count` specifies the number of restart steps which will be
written to the restart database before previously written steps are
overwritten.  For example, if the `cycle` count is 5 and output is
written every 0.1 seconds, IOSS will write data at times 0.1, 0.2,
0.3, 0.4, 0.5 to the database. It will then overwrite the first step
with data from time 0.6, the second with time 0.7.  At time 0.8, the
database would contain data at times 0.6, 0.7, 0.8, 0.4, 0.5.  Note
that time will not necessarily be monotonically increasing on a
database that specifies the cycle count.

The cycle count and overlay count can both be used at the same time
also.  The basic formula is:
```
   db_step = (((output_step - 1) / overlay) % cycle) + 1
```
where `output_step` is the step that this would have been on the
database in a normal write (1,2,3,....) and `db_step` is the step
number that this will be written to.

If you only want the last step available on the database,
use `set_cycle_count(1)`.

If `FILE_PER_STATE` is specified, then `cycle` specifies the number of
separate files which will be created and `overlay` specifies how many
timesteps will be written to each file.  If we have `cycle=2` and
`overlay=3` and the code is outputting every 0.1 seconds, we will get
0.1, 0.2, 0.3 in the first file; 0.4, 0.5, 0.6 in the second
file. Then, the first file will be reopened and steps 0.7, 0.8, and
0.9 will be written to the first file.


## Properties for the heartbeat output
 Property              | Value  | Description
-----------------------|:------:|-----------------------------------------------------------
  FILE_FORMAT          | [default], spyhis, csv, ts_csv, text, ts_text | predefined formats for heartbeat output. The ones starting with `ts_` output timestamps.
  FLUSH_INTERVAL       | int   | Minimum time interval between flushing heartbeat data to disk.  Default is 10 seconds.  Set to 0 to flush every step (bad performance)
  HEARTBEAT_FLUSH_INTERVAL | int   | Minimum time interval between flushing heartbeat data to disk.  Default is 10 seconds (Same as FLUSH_INTERVAL, but doesn't affect other database types)
  TIME_STAMP_FORMAT    | [%H:%M:%S] | Format used to format time stamp.  See strftime man page
  SHOW_TIME_STAMP      | on/off | Should the output lines be preceded by the timestamp
  FIELD_SEPARATOR      | [, ]   | separator to be used between output fields.
  FULL_PRECISION       | on/[off] | output will contain as many digits as needed to fully represent the doubles value.  FIELD_WIDTH will be ignored for doubles if this is specified.
  PRECISION            | -1..16 [5] | Precision used for floating point output. If set to `-1`, then the output will contain as many digits as needed to fully represent the doubles value.  FIELD_WIDTH will be ignored for doubles if precision is set to -1.
  FIELD_WIDTH          | 0.. |  Width of an output field. If 0, then use natural width.
  SHOW_LABELS          | on/[off]  | Should each field be preceded by its name (ke=1.3e9, ie=2.0e9)
  SHOW_LEGEND          | [on]/off  | Should a legend be printed at the beginning of the output showing the field names for each column of data.
  SHOW_TIME_FIELD      | on/[off]  | Should the current analysis time be output as the first field.

## Experimental / Special Purpose

 Property              | Value  | Description
-----------------------|:------:|-----------------------------------------------------------
MEMORY_READ        | on/[off]   | experimental. Read a file into memory at open time, operate on it without disk accesses.
MEMORY_WRITE       | on/[off]   | experimental. Open and read a file into memory or create and optionally write it back out to disk when nc_close() is called.
ENABLE_FILE_GROUPS | on/[off]   | experimental.  Opens database in netcdf-4 non-classic mode which is what is required to support groups at netCDF level.
MINIMAL_NEMESIS_INFO | on/[off] | special case, omit all nemesis data except for nodal communication map
OMIT_EXODUS_NUM_MAPS | on/[off] | special case, do not output the node and element numbering map.
EXODUS_CALL_GET_ALL_TIMES| [on] / off | special case -- should the `ex_get_all_times()` function be called.  See below.

* `EXODUS_CALL_GET_ALL_TIMES`: Typically only used in `isSerialParallel`
mode and the client is responsible for making sure that the step times
are handled correctly.  All databases will know about the number of
timesteps, but if the `ex_get_all_times()` function call is skipped, then
the times on that database will all be zero.  The use case is that in `isSerialParallel`,
each call to `ex_get_all_times()` for all files is performed
sequentially, so if you have hundreds to thousands of files, the time
for the call is additive and since timesteps are record variables in
netCDF, accessing the data for all timesteps involves lseeks
throughout the file.


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
