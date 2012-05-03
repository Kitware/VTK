#!/usr/local/bin/python

# ----------------------------------------------------------------------
#   MR-MPI = MapReduce-MPI library
#   http://www.cs.sandia.gov/~sjplimp/mapreduce.html
#   Steve Plimpton, sjplimp@sandia.gov, Sandia National Laboratories
#
#   Copyright (2009) Sandia Corporation.  Under the terms of Contract
#   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
#   certain rights in this software.  This software is distributed under 
#   the modified Berkeley Software Distribution (BSD) License.
#
#   See the README file in the top-level MapReduce directory.
# -------------------------------------------------------------------------

# MapReduce random RMAT matrix generation example in C++
# Syntax: rmat.py N Nz a b c d frac seed {outfile}
#   2^N = # of rows in RMAT matrix
#   Nz = non-zeroes per row
#   a,b,c,d = RMAT params (must sum to 1.0)
#   frac = RMAT randomization param (frac < 1, 0 = no randomization)
#   seed = RNG seed (positive int)
#   outfile = output RMAT matrix to this filename (optional)

import sys, random
from mrmpi import mrmpi
try:
  import pypar
except:
  import pypar_serial as pypar

# generate RMAT matrix entries
# emit one KV per edge: key = edge, value = NULL

def generate(itask,mr):
  for m in xrange(ngenerate):
    delta = order / 2
    a1 = a; b1 = b; c1 = c; d1 = d
    i = j = 0
    
    for ilevel in xrange(nlevels):
      rn = random.random()
      if rn < a1:
        pass
      elif rn < a1+b1:
	j += delta
      elif rn < a1+b1+c1:
	i += delta
      else:
	i += delta
	j += delta
      
      delta /= 2
      if fraction > 0.0:
	a1 += a1*fraction * (drand48() - 0.5)
	b1 += b1*fraction * (drand48() - 0.5)
	c1 += c1*fraction * (drand48() - 0.5)
	d1 += d1*fraction * (drand48() - 0.5)
	total = a1+b1+c1+d1
	a1 /= total
	b1 /= total
	c1 /= total
	d1 /= total

    mr.add((i,j),None)

# eliminate duplicate edges
# input: one KMV per edge, MV has multiple entries if duplicates exist
# output: one KV per edge: key = edge, value = NULL

def cull(key,mvalue,mr):
  mr.add(key,None)

# write edges to a file unique to this processor

def output(key,mvalue,mr):
  print >>fp,key[0]+1,key[1]+1,1

# enumerate nonzeroes in each row
# input: one KMV per edge
# output: one KV per edge: key = row I, value = NULL

def nonzero(key,mvalue,mr):
  mr.add(key[0],None)

# count nonzeroes in each row
# input: one KMV per row, MV has entry for each nonzero
# output: one KV: key = # of nonzeroes, value = NULL

def degree(key,mvalue,mr):
  mr.add(len(mvalue),None);

# count rows with same # of nonzeroes
# input: one KMV per nonzero count, MV has entry for each row
# output: one KV: key = # of nonzeroes, value = # of rows

def histo(key,mvalue,mr):
  mr.add(key,len(mvalue))

# compare two counts
# order values by count, largest first

def ncompare(one,two):
  if one > two: return -1;
  elif one < two: return 1;
  else: return 0;

# print # of rows with a specific # of nonzeroes

def stats(itask,key,value,mr):
  global total
  total += value;
  print "%d rows with %d nonzeroes" % (value,key)

# main program

nprocs = pypar.size()
me = pypar.rank()

if len(sys.argv) != 9 and len(sys.argv) != 10:
  if me == 0: print "Syntax: N Nz a b c d frac seed {outfile}"
  sys.exit()

nlevels = int(sys.argv[1])
nnonzero = int(sys.argv[2])
a = float(sys.argv[3])
b = float(sys.argv[4])
c = float(sys.argv[5])
d = float(sys.argv[6])
fraction = float(sys.argv[7])
seed = int(sys.argv[8])
if len(sys.argv) == 10: outfile = sys.argv[9]
else: outfile = None

if a+b+c+d != 1.0:
  if me == 0: print "ERROR: a,b,c,d must sum to 1"
  sys.exit()

if fraction >= 1.0:
  if me == 0: print "ERROR: fraction must be < 1"
  sys.exit()

random.seed(seed+me)
order = 1 << nlevels

mr = mrmpi()

# loop until desired number of unique nonzero entries

pypar.barrier()
tstart = pypar.time()

niterate = 0
ntotal = (1 << nlevels) * nnonzero
nremain = ntotal
while nremain:
  niterate += 1
  ngenerate = nremain/nprocs
  if me < nremain % nprocs: ngenerate += 1
  mr.map(nprocs,generate,None,1)
  nunique = mr.collate()
  if nunique == ntotal: break
  mr.reduce(cull)
  nremain = ntotal - nunique

pypar.barrier()
tstop = pypar.time()

# output matrix if requested

if outfile:
  fp = open(outfile + "." + str(me),"w")
  if not fp:
    print "ERROR: Could not open output file"
    sys.exit()
  mr2 = mr.copy()
  mr2.reduce(output)
  fp.close()
  mr2.destroy()

# stats to screen
# include stats on number of nonzeroes per row

if me == 0:
  print order,"rows in matrix"
  print ntotal,"nonzeroes in matrix"

mr.reduce(nonzero)
mr.collate()
mr.reduce(degree)
mr.collate()
mr.reduce(histo)
mr.gather(1)
mr.sort_keys(ncompare)
total = 0
mr.map_kv(mr,stats)
if me == 0: print order-total,"rows with 0 nonzeroes"

if me == 0:
  print "%g secs to generate matrix on %d procs in %d iterations" % \
        (tstop-tstart,nprocs,niterate)

mr.destroy()
  
pypar.finalize()
