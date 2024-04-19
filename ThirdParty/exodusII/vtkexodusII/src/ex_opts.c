/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"
#include "exodusII_int.h"

/*! \cond INTERNAL */
int ex__default_max_name_length = 32; /* For default compatibility with older clients */

#if defined(VERBOSE)
int exoptval = EX_VERBOSE; /* loud mode: set EX_VERBOSE */
#else
#if defined(DEBUG)
int exoptval = EX_VERBOSE | EX_DEBUG; /* debug mode: set EX_VERBOSE & EX_DEBUG */
#else
int exoptval = EX_DEFAULT; /* set default global options value to NOT print error msgs*/
#endif
#endif
/*! \endcond */

/*!
The function ex_opts() is used to set message reporting options.

\return Returns previous value for the message reporting option.

\param[in] options   Integer option value. Current options are shown in the
table below.

<table>
<tr><td> #EX_ABORT  </td><td> Causes fatal errors to force program
                                exit. (Default is false.) </td></tr>
<tr><td> #EX_DEBUG  </td><td> Causes certain messages to print
                                for debug use. (Default is false.)</td></tr>
<tr><td> #EX_VERBOSE</td><td> Causes all error messages to print when true,
                                otherwise no error messages will print. (Default
is false.)</td></tr>
</table>

\note Values may be OR'ed together to provide any combination
      of these capabilities.

For example, the following will cause all messages to print
and will cause the program to exit upon receipt of fatal error:

~~~{.c}
#include "exodusII.h"
ex_opts(EX_ABORT|EX_VERBOSE);
~~~

*/
int ex_opts(int options)
{
  EX_FUNC_ENTER();
  int oldval = exoptval;
  exoptval   = options;
  EX_FUNC_LEAVE(oldval);
}
