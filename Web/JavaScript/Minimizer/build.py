#==============================================================================
#
#  Program:   ParaView
#  Module:    build.py
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#==============================================================================
try:
  import argparse
except ImportError:
  # since  Python 2.6 and earlier don't have argparse, we simply provide
  # the source for the same as _argparse and we use it instead.
  import _argparse as argparse

import shutil
import StringIO
import string
import os
from datetime import date

import rjsmin
import rcssmin

parser = argparse.ArgumentParser(description="Concatenation and minimize Javascript files")
parser.add_argument('-b', help="Javascript banner")
parser.add_argument('-v', help="Version string to add to the header")
parser.add_argument('-i',  nargs='*', help="Files to concatenate and minimize")
parser.add_argument('-o',  help="Output file")
parser.add_argument('-m',  help="Minimized output file")

args = parser.parse_args()

output = StringIO.StringIO()

isJavaScript = (args.o[-3:] == '.js')

# read in files
for file in args.i:
  with open(file, 'r') as fp:
    output.write(fp.read())

# Generate banner
with open(args.b, 'r') as fp:
  template = string.Template(fp.read())
  d = date.today()
  vars = dict(version=args.v,
              date=d.strftime("%Y-%m-%d"),
              year=d.strftime("%Y"))
  banner = template.substitute(vars)

# write output to file
dest_dir = os.path.dirname(args.m)
if not os.path.exists(dest_dir):
  try:
    os.makedirs(dest_dir);
  except OSError, e:
    if e.errno != 17:
      raise

with open(args.m,"w") as fp:
  fp.write(banner)
  fp.write(output.getvalue())

# write minimized output to file
dest_dir = os.path.dirname(args.o)
if not os.path.exists(dest_dir):
  try:
    os.makedirs(dest_dir);
  except OSError, e:
    if e.errno != 17:
      raise

if isJavaScript:
  with open(args.o,"w") as fp:
    fp.write(banner)
    fp.write(rjsmin.jsmin(output.getvalue()))
else:
  with open(args.o,"w") as fp:
    fp.write(banner)
    fp.write(rcssmin.cssmin(output.getvalue()))
