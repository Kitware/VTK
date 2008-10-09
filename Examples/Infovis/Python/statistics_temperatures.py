from vtk import *

data_dir = "../../../../../VTKData/Data/Infovis/SQLite/"
sqlite_file = data_dir + "temperatures.db"
database = vtkSQLDatabase.CreateFromURL("sqlite://" + sqlite_file)
database.Open("")

query = database.GetQueryInstance()
query.SetQuery("select * from main_tbl")

data = vtkRowQueryToTable()
data.SetQuery(query)

# Start with descriptive statistics
print "# Start with descriptive statistics:"
ds = vtkDescriptiveStatistics()
ds.AddInputConnection(data.GetOutputPort())
ds.AddColumn("Temp1")
ds.AddColumn("Temp2")
ds.Update()

dStats = ds.GetOutput(1)
dStats.Dump( 10 )
print

print "# Now calculate 5-point statistics:"
# Now calculate 5-point statistics
os = vtkOrderStatistics()
os.AddInputConnection(data.GetOutputPort())
os.AddColumn("Temp1")
os.AddColumn("Temp2")
os.Update()

oStats = os.GetOutput(1)
oStats.Dump( 14 )
print

print "# Continue with deciles:"
# Continue with deciles
os.SetNumberOfIntervals(10)
os.Update()

oStats = os.GetOutput(1)
oStats.Dump( 11 )
print

print "# Finally, calculate correlation and linear regression:"
# Finally, calculate correlation and linear regression
cs = vtkCorrelativeStatistics()
cs.AddInputConnection(data.GetOutputPort())
cs.AddColumnPair("Temp1","Temp2")
cs.SetAssess(1)
cs.Update()

cStats = cs.GetOutput(1)
cStats.Dump( 10 )
print

print "# And report corresponding joint probabilities:"
# And report corresponding joint probabilities
cData = cs.GetOutput(0)
cData.Dump( 14 )
print
