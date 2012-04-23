/*
 * Copyright (c) 2005 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Governement
 * retains certain rights in this software.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.  
 * 
 *     * Neither the name of Sandia Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include "exodusII.h"
#include "exodusII_int.h"

/*! \cond INTERNAL */
int ex_max_name_length = 32; /* For default compatibility with older clients */

#if defined(VERBOSE)
int exoptval = EX_VERBOSE;   /* loud mode: set EX_VERBOSE */
#else
#if defined (DEBUG)
int exoptval = EX_VERBOSE | EX_DEBUG;/* debug mode: set EX_VERBOSE & EX_DEBUG */
#else
int exoptval = EX_DEFAULT;  /* set default global options value to NOT print error msgs*/
#endif
#endif
/*! \endcond */

/*!
The function ex_opts() is used to set message reporting options.

\return In case of an error, ex_opts() returns a negative number; a warning
        will return a positive number.

\param[in] options   Integer option value. Current options are shown in the table below.

<table>
<tr><td> \c EX_ABORT  </td><td> Causes fatal errors to force program 
                                exit. (Default is false.) </td></tr>
<tr><td> \c EX_DEBUG  </td><td> Causes certain messages to print 
                                for debug use. (Default is false.)</td></tr>
<tr><td> \c EX_VERBOSE</td><td> Causes all error messages to print when true, 
                                otherwise no error messages will print. (Default is false.)</td></tr>
</table>

\note Values may be OR'ed together to provide any combination 
      of these capabilities.

For example, the following will cause all messages to print 
and will cause the program to exit upon receipt of fatal error:

\code
#include "exodusII.h"
ex_opts(EX_ABORT|EX_VERBOSE);
\endcode

*/
void ex_opts (int options)      
{
  exerrval = 0; /* clear error code */
  exoptval = options;
}
