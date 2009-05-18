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

# MapReduce word frequency example in Python
# Syntax: wordfreq.py file1 file2 ...
# (1) reads all files, parses into words separated by whitespace
# (2) counts occurrence of each word in all files
# (3) prints top 10 words

import sys
from mrmpi import mrmpi
try:
  import pypar
except:
  import pypar_serial as pypar

# read a file
# for each word in file, emit key = word, value = NULL

def fileread(itask,mr):
  text = open(files[itask]).read()
  words = text.split()
  for word in words: mr.add(word,None)

# count word occurrence 
# emit key = word, value = # of multi-values

def sum(key,mvalue,mr):
  mr.add(key,len(mvalue))

# compare two counts
# order values by count, largest first

def ncompare(key1,key2):
  if key1 < key2: return 1
  elif key1 > key2: return -1
  else: return 0

# process a word and its count
# depending on flag, emit KV or print it, up to limit

def output(itask,key,value,mr):
  count[0] += 1
  if count[0] > count[1]: return
  if count[2]: print value,key
  else: mr.add(key,value)

# main program

nprocs = pypar.size()
me = pypar.rank()

if len(sys.argv) < 2:
  print "Syntax: wordfreq.py file1 file2 ..."
  sys.exit()
files = sys.argv[1:]

mr = mrmpi()

pypar.barrier()
tstart = pypar.time()

nwords = mr.map(len(files),fileread)
mr.collate()
nunique = mr.reduce(sum)

pypar.barrier()
tstop = pypar.time()

mr.sort_values(ncompare)
count = [0,10,0]
mr.map_kv(mr,output)

mr.gather(1)
mr.sort_values(ncompare)
count = [0,10,1]
mr.map_kv(mr,output)

mr.destroy()

# output

if me == 0:
  print "%d total words, %d unique words" % (nwords,nunique)
  print "Time to process %d files on %d procs = %g (secs)" % \
        (len(files),nprocs,tstop-tstart);
  
pypar.finalize()
