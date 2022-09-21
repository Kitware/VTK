#!/usr/bin/env python

import os
import argparse as cli

def isWindows():
    return os.name == 'nt'

def LibrarySymbols(lib):
    sym_list = []
    if isWindows():
        stream = os.popen('dumpbin /SYMBOLS /EXPORTS %s' % lib)
        for line in stream:
            sym = line.split('|')[1].strip()
            sym_list.append(sym)
    else:
        stream = os.popen('nm %s' % lib)
        sym_codes = ['B', 'D', 'R', 'T']
        for line in stream:
            if len(line) < 20:
                continue

            if line[17] in sym_codes:
                sym = line[19:].strip()
                sym_list.append(sym)
    return sym_list


parser = cli.ArgumentParser()

parser.add_argument(
    '--files',
    help='List of binary files to parse symbols from')
parser.add_argument(
    '--prefix',
    help='Mangling prefix.')
parser.add_argument(
    '--exemptions',
    help='Ignore mangling for exempt symbols.')
args = parser.parse_args()

if not args.prefix:
    print('No prefix to check.')
    exit(0)

exemptions = args.exemptions.split(';')

bad_sym = []

for o in args.files.split(';'):
    for sym in LibrarySymbols(o):
        if not args.prefix in sym:
            if not any(exempt in sym for exempt in exemptions):
                bad_sym.append(sym)

if bad_sym:
    print('Found symbols that are missing mangling.')
    for sym in bad_sym:
        print("  + %s" % sym)
    exit(1)
