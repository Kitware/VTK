#!/usr/bin/env python
from vtk import *
import os.path
import sys
from PyQt4.QtCore import *
from PyQt4.QtGui import *


data_dir = "../../../../VTKData/Data/Infovis/SQLite/"
if not os.path.exists(data_dir):
  data_dir = "../../../../../VTKData/Data/Infovis/SQLite/"
if not os.path.exists(data_dir):
  data_dir = "../../../../../../VTKData/Data/Infovis/SQLite/"
sqlite_file = data_dir + "temperatures.db"

# I'm sure there's a better way then these global vars
currentRow = 0;
numberOfRows = 1;
done = False;

psuedoStreamingData = vtkProgrammableFilter()

def streamData():
    global done
    global currentRow

    input = psuedoStreamingData.GetInput()
    output = psuedoStreamingData.GetOutput()

    # Copy just the columns names/types
    output.GetRowData().CopyStructure(input.GetRowData())

    # Loop through all the input data and grab the next bunch of rows
    startRow = currentRow
    endRow = startRow + numberOfRows
    if (endRow >= input.GetNumberOfRows()):
        endRow = input.GetNumberOfRows()
        done = True;
    print "streaming: ", startRow, "-", endRow

    for i in range(startRow, endRow):
        output.InsertNextRow(input.GetRow(i))

    currentRow = endRow;

psuedoStreamingData.SetExecuteMethod(streamData)

class Timer(QObject):
    def __init__(self, parent=None):
        super(Timer, self).__init__(parent)

        # Setup the data streaming timer
        self.timer = QTimer()
        QObject.connect(self.timer, SIGNAL("timeout()"), self.update)
        self.timer.start(100)

    def update(self):
        if (done):
            quit();
        psuedoStreamingData.Modified() # Is there a way to avoid this?
        psuedoStreamingData.GetExecutive().Push()
        printStats()


def printStats():
    sStats = ss.GetOutputDataObject( 1 )
    sPrimary = sStats.GetBlock( 0 )
    sDerived = sStats.GetBlock( 1 )
    sPrimary.Dump( 15 )
    sDerived.Dump( 15 )


if __name__ == "__main__":
    """ Main entry point of this python script """

    # Set up streaming executive
    streamingExec = vtkThreadedStreamingPipeline()
    vtkAlgorithm.SetDefaultExecutivePrototype(streamingExec)
    streamingExec.FastDelete()
    vtkThreadedStreamingPipeline.SetAutoPropagatePush(True)

    # Pull the table from the database
    databaseToTable = vtkSQLDatabaseTableSource()
    databaseToTable.SetURL("sqlite://" + sqlite_file)
    databaseToTable.SetQuery("select * from main_tbl")

    # Hook up the database to the streaming data filter
    psuedoStreamingData.SetInputConnection(databaseToTable.GetOutputPort())


    # Calculate offline(non-streaming) descriptive statistics
    print "# Calculate offline descriptive statistics:"
    ds = vtkDescriptiveStatistics()
    ds.SetInputConnection(databaseToTable.GetOutputPort())
    ds.AddColumn("Temp1")
    ds.AddColumn("Temp2")
    ds.Update()

    dStats = ds.GetOutputDataObject( 1 )
    dPrimary = dStats.GetBlock( 0 )
    dDerived = dStats.GetBlock( 1 )
    dPrimary.Dump( 15 )
    dDerived.Dump( 15 )


    # Stats filter to place 'into' the streaming filter
    inter = vtkDescriptiveStatistics()
    inter.AddColumn("Temp1")
    inter.AddColumn("Temp2")




    # Calculate online(streaming) descriptive statistics
    print "# Calculate online descriptive statistics:"
    ss = vtkStreamingStatistics()
    ss.SetStatisticsAlgorithm(inter)
    ss.SetInputConnection(psuedoStreamingData.GetOutputPort())

    # Spin up the timer
    app = QApplication(sys.argv)
    stream = Timer()

    sys.exit(app.exec_())
