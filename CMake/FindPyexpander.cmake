##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

# - Finds the pyexpander macro tool.
# Use this module by invoking find_package.
#
# This module finds the expander.py command distributed with pyexpander.
# pyexpander can be downloaded from http://pyexpander.sourceforge.net.
# The following variables are defined:
#
# PYEXPANDER_FOUND   - True if pyexpander is found
# PYEXPANDER_COMMAND - The pyexpander executable
#
# Note that on some platforms (such as Windows), you cannot execute a python
# script directly. Thus, it could be safer to execute the Python interpreter
# with PYEXPANDER_COMMAND as an argument. See FindPythonInterp.cmake for help
# in finding the Python interpreter.

find_program(PYEXPANDER_COMMAND NAMES expander.py expander3.py)

mark_as_advanced(PYEXPANDER_COMMAND)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Pyexpander DEFAULT_MSG PYEXPANDER_COMMAND)
