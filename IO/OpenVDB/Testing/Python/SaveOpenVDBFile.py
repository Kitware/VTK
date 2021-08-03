import vtk

import vtk.test.Testing
from vtk.util.misc import vtkGetTempDir

import sys
import os


dc = vtk.vtkDummyController()
dc.SetGlobalController(dc)


source = vtk.vtkRTAnalyticSource()


VTK_TEMP_DIR = vtkGetTempDir()

writer = vtk.vtkOpenVDBWriter()
writer.SetInputConnection(source.GetOutputPort())


fileName = VTK_TEMP_DIR+'/OpenVDBWriter.vdb'
if os.path.exists(fileName):
    os.remove(fileName)
writer.SetFileName(fileName)
writer.Update()

if '--VDB_PRINT_EXE' in sys.argv:
    print("checking the VDB output file")
    vdb_print_index = sys.argv.index('--VDB_PRINT_EXE')+1
    stream = os.popen(sys.argv[vdb_print_index]+' '+fileName)

    # the output of the stream should look something like:
    # RTData       float (-10,-10,-10)->(10,10,10)       21x21x21  9.26KVox 2.33MB

    # we don't compare the last bit since that could be system or compile dependent
    data = stream.readline()
    pieces = data.split()
    if pieces[0] != 'RTData' or pieces[1] != 'float' or pieces[2] != '(-10,-10,-10)->(10,10,10)' or pieces[3] != '21x21x21':
        print("failure: result should look like 'RTData       float (-10,-10,-10)->(10,10,10)       21x21x21  9.26KVox 2.33MB' " +
              " but is " + data)
        sys.exit(1)

print("success")
