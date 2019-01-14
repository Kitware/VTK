#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, os.path
import sys
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# List of types and corresponding file extensions.
types = [[ 'ImageData', 'vti'],
         ['RectilinearGrid', 'vtr'],
         ['StructuredGrid', 'vts'],
         ['PolyData', 'vtp'],
         ['UnstructuredGrid', 'vtu']]

# We intentionally cause vtkErrorMacro calls to be made below.  Dump
# errors to a file to prevent a window from coming up.
fow = vtk.vtkFileOutputWindow()
fow.SetFileName("TestEmptyXMLErrors.txt")
fow.SetFlush(0)
fow.SetInstance(fow)

# Prepare some test files.
f = open('emptyFile.vtk', 'wt')
f.close()
f = open('junkFile.vtk', 'wt')
f.write("v9np7598mapwcawoiur-,rjpmW9MJV28nun-q38ynq-9.8ugujqvt-8n3-nv8")
f.close()

# Test each writer/reader.
for t in types:
    type = t[0]
    ext = t[1]
    input = eval('vtk.vtk' + type + '()')

    writer = eval('vtk.vtkXML' + type + 'Writer()')
    writer.SetFileName('empty' + type + '.' + ext)
    sys.stdout.write('Attempting ' + type + ' write with no input.\n')
    writer.Write()
    sys.stdout.write('Attempting ' + type + ' write with empty input.\n')
    writer.SetInputData(input)
    writer.Write()

    reader = eval('vtk.vtkXML' + type + 'Reader()')
    reader.SetFileName('empty' + type + '.' + ext)
    sys.stdout.write('Attempting read from file with empty ' + type + '.\n')
    reader.Update()

    pwriter = eval('vtk.vtkXMLP' + type + 'Writer()')
    pwriter.SetFileName('emptyP' + type + '.p' + ext)
    sys.stdout.write('Attempting P' + type + ' write with no input.\n')
    pwriter.Write()
    sys.stdout.write('Attempting P' + type + ' write with empty input.\n')
    pwriter.SetInputData(input)
    pwriter.Write()

    preader = eval('vtk.vtkXMLP' + type + 'Reader()')
    preader.SetFileName('emptyP' + type + '.p' + ext)
    sys.stdout.write('Attempting read from file with empty P' + type + '.\n')
    preader.Update()

    reader.SetFileName("emptyFile.vtk")
    preader.SetFileName("emptyFile.vtk")

    sys.stdout.write('Attempting read ' + type + ' from empty file.\n')
    reader.Update()
    sys.stdout.write('Attempting read P' + type + ' from empty file.\n')
    preader.Update()

    reader.SetFileName("junkFile.vtk")
    preader.SetFileName("junkFile.vtk")

    sys.stdout.write('Attempting read ' + type + ' from junk file.\n')
    reader.Update()
    sys.stdout.write('Attempting read P' + type + ' from junk file.\n')
    preader.Update()

    del input
    del writer
    del reader
    del pwriter
    del preader

# Test the data set writers.
for t in types:
    type = t[0]
    ext = t[1]
    writer = vtk.vtkXMLDataSetWriter()
    pwriter = vtk.vtkXMLPDataSetWriter()
    input = eval('vtk.vtk' + type + '()')

    writer.SetFileName('empty' + type + 'DataSet.' + ext)
    sys.stdout.write('Attempting DataSet ' + type + ' write with no input.\n')
    writer.Write()
    sys.stdout.write('Attempting DataSet ' + type + ' write with empty input.\n')
    writer.SetInputData(input)
    writer.Write()

    pwriter.SetFileName('emptyP' + type + 'DataSet.p' + ext)
    sys.stdout.write('Attempting DataSet ' + type + ' write with no input.\n')
    pwriter.SetNumberOfPieces(1)
    pwriter.Write()
    sys.stdout.write('Attempting DataSet ' + type + ' write with empty input.\n')
    pwriter.SetInputData(input)
    pwriter.Write()

    del input
    del pwriter
    del writer

# Done with the file output window.
fow.SetInstance(None)
del fow

# Delete the test files.
for t in types:
    type = t[0]
    ext = t[1]

    os.remove('empty' + type + '.' + ext)
    os.remove('empty' + type + 'DataSet.' + ext)
    os.remove('emptyP' + type + '.p' + ext)
    assert not os.path.exists('emptyP' + type + '_0.' + ext)
    os.remove('emptyP' + type + 'DataSet.p' + ext)
    assert not os.path.exists('emptyP' + type + 'DataSet_0.' + ext)

os.remove('junkFile.vtk')
os.remove('emptyFile.vtk')
os.remove('TestEmptyXMLErrors.txt')
