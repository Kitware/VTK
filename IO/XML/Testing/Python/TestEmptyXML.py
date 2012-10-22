#!/usr/bin/env python

# List of types and corresponding file extensions.
types =
    {ImageData vti}
    {RectilinearGrid vtr}
    {StructuredGrid vts}
    {PolyData vtp}
    {UnstructuredGrid vtu}

# We intentionally cause vtkErrorMacro calls to be made below.  Dump
# errors to a file to prevent a window from coming up.
fow = vtk.vtkFileOutputWindow()
fow.SetFileName("TestEmptyXMLErrors.txt")
fow.SetFlush(0)
fow.SetInstance(fow)
# Prepare some test files.
file.delete("-force", "junkFile.vtk")
file.delete("-force", "emptyFile.vtk")
f = open("emptyFile.vtk", w)
f.close()
f = open("junkFile.vtk", w)
puts.f("v9np7598mapwcawoiur-,rjpmW9MJV28nun-q38ynq-9 8ugujqvt-8n3-nv8")
f.close()
# Test each writer/reader.
for pair in types.split():
    type = lindex(pair,0)
    ext = lindex(pair,1)
    locals()[get_variable_name("vtk", type, "")].input()
    locals()[get_variable_name("vtkXML", type, "Writer")].writer()
    writer.SetFileName("empty" + str(locals()[get_variable_name("", type, ".", ext, "")]) + "")
    puts."Attempting " + str(locals()[get_variable_name("", type, "")]) + " write with no input."()
    catch.catch(globals(),"""writer.Write()""")
    puts."Attempting " + str(locals()[get_variable_name("", type, "")]) + " write with empty input."()
    writer.SetInputData(input)
    writer.Write()
    locals()[get_variable_name("vtkXML", type, "Reader")].reader()
    reader.SetFileName("empty" + str(locals()[get_variable_name("", type, ".", ext, "")]) + "")
    puts."Attempting read from file with empty " + str(locals()[get_variable_name("", type, ".")]) + ""()
    reader.Update()
    locals()[get_variable_name("vtkXMLP", type, "Writer")].pwriter()
    pwriter.SetFileName("emptyP" + str(locals()[get_variable_name("", type, ".p", ext, "")]) + "")
    pwriter.SetNumberOfPieces(1)
    puts."Attempting P" + str(locals()[get_variable_name("", type, "")]) + " write with no input."()
    catch.catch(globals(),"""pwriter.Write()""")
    puts."Attempting P" + str(locals()[get_variable_name("", type, "")]) + " write with empty input."()
    pwriter.SetInputData(input)
    pwriter.Write()
    locals()[get_variable_name("vtkXMLP", type, "Reader")].preader()
    preader.SetFileName("emptyP" + str(locals()[get_variable_name("", type, ".p", ext, "")]) + "")
    puts."Attempting read from file with empty P" + str(locals()[get_variable_name("", type, ".")]) + ""()
    preader.Update()
    reader.SetFileName("emptyFile.vtk")
    preader.SetFileName("emptyFile.vtk")
    puts."Attempting read " + str(locals()[get_variable_name("", type, "")]) + " from empty file."()
    reader.Update()
    puts."Attempting read P" + str(locals()[get_variable_name("", type, "")]) + " from empty file."()
    preader.Update()
    reader.SetFileName("junkFile.vtk")
    preader.SetFileName("junkFile.vtk")
    puts."Attempting read " + str(locals()[get_variable_name("", type, "")]) + " from junk file."()
    reader.Update()
    puts."Attempting read P" + str(locals()[get_variable_name("", type, "")]) + " from junk file."()
    preader.Update()
    del input
    del writer
    del reader
    del pwriter
    del preader

    pass
# Test the data set writers.
for pair in types.split():
    type = lindex(pair,0)
    ext = lindex(pair,1)
    writer = vtk.vtkXMLDataSetWriter()
    pwriter = vtk.vtkXMLPDataSetWriter()
    locals()[get_variable_name("vtk", type, "")].input()
    writer.SetFileName("empty" + str(locals()[get_variable_name("", type, "DataSet.", ext, "")]) + "")
    puts."Attempting DataSet " + str(locals()[get_variable_name("", type, "")]) + " write with no input."()
    catch.catch(globals(),"""writer.Write()""")
    puts."Attempting DataSet " + str(locals()[get_variable_name("", type, "")]) + " write with empty input."()
    writer.SetInputData(input)
    writer.Write()
    pwriter.SetFileName("emptyP" + str(locals()[get_variable_name("", type, "DataSet.p", ext, "")]) + "")
    pwriter.SetNumberOfPieces(1)
    puts."Attempting DataSet P" + str(locals()[get_variable_name("", type, "")]) + " write with no input."()
    catch.catch(globals(),"""pwriter.Write()""")
    puts."Attempting DataSet P" + str(locals()[get_variable_name("", type, "")]) + " write with empty input."()
    pwriter.SetInputData(input)
    pwriter.Write()
    del input
    del pwriter
    del writer

    pass
# Done with file output window.
fow.SetInstance(None)
del fow
# Delete the test files.
for pair in types.split():
    type = lindex(pair,0)
    ext = lindex(pair,1)
    file.delete("-force", "empty" + str(locals()[get_variable_name("", type, ".", ext, "")]) + "")
    file.delete("-force", "empty" + str(locals()[get_variable_name("", type, "DataSet.", ext, "")]) + "")
    file.delete("-force", "emptyP" + str(locals()[get_variable_name("", type, ".p", ext, "")]) + "")
    file.delete("-force", "emptyP" + str(locals()[get_variable_name("", type, "0.", ext, "")]) + "")
    file.delete("-force", "emptyP" + str(locals()[get_variable_name("", type, "DataSet.p", ext, "")]) + "")
    file.delete("-force", "emptyP" + str(locals()[get_variable_name("", type, "DataSet0.", ext, "")]) + "")

    pass
file.delete("-force", "junkFile.vtk")
file.delete("-force", "emptyFile.vtk")
file.delete("-force", "TestEmptyXMLErrors.txt")
exit
# --- end of script --
