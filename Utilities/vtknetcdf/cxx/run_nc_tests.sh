#!/bin/sh

# This shell script runs some netCDF C++ API tests for classic and
# 64-bit offset format.

# $Id: run_nc_tests.sh,v 1.2 2006/07/31 15:17:20 ed Exp $

set -e
echo ""
echo "*** Testing C++ API test program output."

echo "*** dumping classic format file to nctst_classic.cdl and comparing..."
../ncdump/ncdump -n ref_nctst nctst_classic.nc > nctst_classic.cdl
diff -w nctst_classic.cdl $srcdir/ref_nctst.cdl

echo "*** dumping 64-bit offset format file to nctst_64bit_offset.cdl and comparing..."
../ncdump/ncdump -n ref_nctst nctst_64bit_offset.nc > nctst_64bit_offset.cdl
diff -w nctst_64bit_offset.cdl $srcdir/ref_nctst_64bit_offset.cdl

echo "*** All tests of C++ API test output passed!"
exit 0
