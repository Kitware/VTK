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
        stream = os.popen('nm -gC --defined %s' % lib)
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

exemptions = args.exemptions.split(';') if args.exemptions else []

bad_sym = []

# Special symbols can be ignored
# exemptions.extend(["__bss_start", "_edata", "_end"])

RESERVED_NAMES = []
if not isWindows():
    external_data_syms = ["_edata", "_etext", "_end"]
    static_data_syms = ["__bss_start", "__bss_end"]
    RESERVED_NAMES += external_data_syms + static_data_syms


def is_reserved_name(sym):
    return sym in RESERVED_NAMES


for o in args.files.split(';'):
    print("-- Checking object file: ", o)
    for sym in LibrarySymbols(o):
        if is_reserved_name(sym):
            continue

        if args.prefix not in sym:
            print("-- Found symbol", sym)
            if not any(exempt in sym for exempt in exemptions):
                bad_sym.append(sym)
            else:
                print("Exempt")

if bad_sym:
    print('Found symbols that are missing mangling.')
    for sym in bad_sym:
        print("  + %s" % sym)
    exit(1)
