#!/usr/bin/env python

"""Utility script to convert an old VTK file format to the new VTK XML
file format (serial format).  The output XML file will contain *all*
the existing scalars, vectors and tensors in the input file.

This requires VTK 4.x or above.

Created May 2003, Prabhu Ramachandran <prabhu@aero.iitm.ernet.in>

Licence: VTK License.

"""

import sys
import vtk
import os.path
import getopt


def getReaderWriter(file_name, out_dir=None):
    r = vtk.vtkDataSetReader()
    r.SetFileName(file_name)
    f_base = os.path.splitext(file_name)[0]
    r.Update()
    reader = None
    writer = None
    xmlsuffix = '.xml'
    map = {'StructuredPoints': '.vti', 'StructuredGrid': '.vts',
           'RectilinearGrid': '.vtr', 'UnstructuredGrid': '.vtu',
           'PolyData': '.vtp'}
    for i in ['StructuredPoints', 'StructuredGrid', 'RectilinearGrid',
              'UnstructuredGrid', 'PolyData']:
        if eval('r.IsFile%s()'%i):
            reader = eval('vtk.vtk%sReader()'%i)
            if i == 'StructuredPoints':
                writer = eval('vtk.vtkXMLImageDataWriter()')
            else:
                writer = eval('vtk.vtkXML%sWriter()'%i)
            xmlsuffix = map[i]
            break
    if not reader:
        return None, None
    
    reader.SetFileName(file_name)
    reader.Update()

    out_file = f_base + xmlsuffix
    if out_dir:
        out_file = os.path.join(out_dir,
                                os.path.basename(f_base) + xmlsuffix)
    writer.SetFileName(out_file)
    return reader, writer


def _getAttr(reader, lst, attr='Scalars'):
    p_a = []
    c_a = []
    
    for i in lst:
        eval('reader.Set%sName(i)'%attr)
        reader.Update()
        o = reader.GetOutput()
        pd = o.GetPointData()
        cd = o.GetCellData()
        s = eval('pd.Get%s()'%attr)
        if s and (s not in p_a):
            p_a.append(s)
        s = eval('cd.Get%s()'%attr)
        if s and (s not in c_a):
            c_a.append(s)
    return p_a, c_a
    

def setAllAttributes(reader):
    s_name = []
    v_name = []
    t_name = []
    for i in range(reader.GetNumberOfScalarsInFile()):
        s_name.append(reader.GetScalarsNameInFile(i))
    for i in range(reader.GetNumberOfVectorsInFile()):
        v_name.append(reader.GetVectorsNameInFile(i))
    for i in range(reader.GetNumberOfTensorsInFile()):
        t_name.append(reader.GetTensorsNameInFile(i))
    
    p_s, c_s = _getAttr(reader, s_name, 'Scalars')
    p_v, c_v = _getAttr(reader, v_name, 'Vectors')
    p_t, c_t = _getAttr(reader, t_name, 'Tensors')

    o = reader.GetOutput()
    pd = o.GetPointData()
    for i in p_s + p_v + p_t:
        pd.AddArray(i)

    cd = o.GetCellData()
    for i in c_s + c_v + c_t:
        cd.AddArray(i)

    return o


def usage():
    msg = """usage: vtk2xml.py [options] vtk_file1 vtk_file2 ...\n
This program converts VTK's old file format to the new XML format.

The default mode is to store the data as appended (compressed and
base64 encoded).  Change this behaviour with the provided options.
This code requires VTK 4.x or above to run.

options:
  -h, --help         Show this help message and exit.

  -b, --binary       Store data as binary (compressed and base64 encoded).

  -a, --ascii        Store data as ascii.

  -n, --no-encode    Do not base64 encode the data.  This violates the
                     XML specification but makes reading and writing fast
                     and files smaller.

  -d, --output-dir <directory>
                     Output directory where the files should be generated.
                     Defaults to the same directory as the input file.
"""
    return msg


def main():    
    options = "bahnd:"
    long_opts = ['binary', 'ascii', 'help', 'no-encode', 'output-dir=']

    try:
        opts, args = getopt.getopt(sys.argv[1:], options, long_opts)
    except getopt.error, msg:
        print msg
        print usage()
        print '-'*70
        print msg
        sys.exit(1)
   
    mode = 'p'
    encode = 1
    out_dir = None
    for o, a in opts:
        if o in ('-h', '--help'):
            print usage()
            sys.exit(0)
        if o in ('-b', '--binary'):
            mode = 'b'
        if o in ('-a', '--ascii'):
            mode = 'a'
        if o in ('-n', '--no-encode'):
            encode = 0
        if o in ('-d', '--output-dir'):
            out_dir = a
            if not os.path.exists(out_dir):
                print "Error: Directory %s does not exist!"%out_dir
                sys.exit(1)

    if len(args) < 1:
        print "\nError: Incorrect number of arguments\n"
        print usage()
        sys.exit(1)
    
    for i in args:        
        r, w = getReaderWriter(i, out_dir)
        if not r:
            print "\nError: Could not convert file: %s"%i
            print "Unsupported data format!\n"
        else:
            o = setAllAttributes(r)
            w.SetInput(o)
            # set output modes
            if mode == 'a':
                w.SetDataModeToAscii()
            elif mode == 'b':
                w.SetDataModeToBinary()
            else:
                w.SetDataModeToAppended()

            w.SetEncodeAppendedData(encode)
            w.Write()


if __name__ == "__main__":
    main()
