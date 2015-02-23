#!/usr/bin/env python
from vtk import *
import os.path

# Set database parameters
data_dir = "../../../../VTKData/Data/Infovis/SQLite/"
if not os.path.exists( data_dir):
  data_dir = "../../../../../VTKData/Data/Infovis/SQLite/"
if not os.path.exists( data_dir):
  data_dir = "../../../../../../VTKData/Data/Infovis/SQLite/"
sqlite_file = data_dir + "temperatures.db"
databaseToTable = vtkSQLDatabaseTableSource()
databaseToTable.SetURL("sqlite://" + sqlite_file)

# Pull the first data set from the database
databaseToTable.SetQuery("select * from main_tbl where CompId==2")

# Calculate primary descriptive statistics for first batch
print "# Calculate primary model of descriptive statistics for first data set:"
ds1 = vtkDescriptiveStatistics()
ds1.AddInputConnection( databaseToTable.GetOutputPort() )
ds1.AddColumn("Temp1")
ds1.AddColumn("Temp2")
ds1.SetLearnOption( 1 )
ds1.SetDeriveOption( 0 )
ds1.SetAssessOption( 0 )
ds1.SetTestOption( 0 )
ds1.Update()

# Show primary descriptive statistics for first batch
dStats1 = ds1.GetOutputDataObject( 1 )
dPrimary1 = dStats1.GetBlock( 0 )
dPrimary1.Dump( 15 )
print

# Pull the second data set from the database
databaseToTable.SetQuery("select * from main_tbl where CompId==3")

# Calculate primary descriptive statistics for second batch
print "# Calculate primary model of descriptive statistics for second data set:"
ds2 = vtkDescriptiveStatistics()
ds2.AddInputConnection( databaseToTable.GetOutputPort() )
ds2.AddColumn("Temp1")
ds2.AddColumn("Temp2")
ds2.SetLearnOption( 1 )
ds2.SetDeriveOption( 0 )
ds2.SetAssessOption( 0 )
ds2.SetTestOption( 0 )
ds2.Update()

# Show primary descriptive statistics for second batch
dStats2 = ds2.GetOutputDataObject( 1 )
dPrimary2 = dStats2.GetBlock( 0 )
dPrimary2.Dump( 15 )
print

# Finally aggregate both models to get a new primary model for the whole ensemble
print "# Aggregate  both primary models:"
collection = vtkDataObjectCollection()
collection.AddItem( dStats1 )
collection.AddItem( dStats2 )
ds = vtkDescriptiveStatistics()
aggregated = vtkMultiBlockDataSet()
ds.Aggregate( collection, aggregated )
dPrimary = aggregated.GetBlock( 0 )
dPrimary.Dump( 15 )
print

# Calculate derived model for whole ensemble
print "# Now calculating derived statistics for whole ensemble:"
ds.SetInputData( 2, aggregated )
ds.SetLearnOption( 0 )
ds.SetDeriveOption( 1 )
ds.SetAssessOption( 0 )
ds.SetTestOption( 0 )
ds.Update()
dStats = ds.GetOutputDataObject( 1 )
dDerived = dStats.GetBlock( 1 )
dDerived.Dump( 15 )
print

# Pull entire data set from the database
databaseToTable.SetQuery("select * from main_tbl")

# Verify with calculation for whole ensemble at once
print "# Finally verifying by directly calculating statistics for whole ensemble:"
ds0 = vtkDescriptiveStatistics()
ds0.AddInputConnection( databaseToTable.GetOutputPort() )
ds0.AddColumn("Temp1")
ds0.AddColumn("Temp2")
ds0.SetLearnOption( 1 )
ds0.SetDeriveOption( 1 )
ds0.SetAssessOption( 0 )
ds0.SetTestOption( 0 )
ds0.Update()

# Show all descriptive statistics for whole ensemble
dStats0 = ds0.GetOutputDataObject( 1 )
dPrimary0 = dStats0.GetBlock( 0 )
dPrimary0.Dump( 15 )
dDerived0 = dStats0.GetBlock( 1 )
dDerived0.Dump( 15 )
print
