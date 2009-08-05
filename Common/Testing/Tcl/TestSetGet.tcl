for {set i  0} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      set auto_path "$auto_path [lindex $argv [expr $i +1]]"
   }
}

package require vtk
vtkTimerLog timer
vtkObject a
a GlobalWarningDisplayOff
a Delete

set exceptions {
vtkLODProp3D-GetPickLODID
vtkObject-GetSuperClassName
vtkPropAssembly-GetBounds
vtkRenderWindow-GetEventPending
vtkSQLiteDatabase-GetQueryInstance
vtkMySQLDatabase-GetQueryInstance
vtkPostgreSQLDatabase-GetQueryInstance
vtkODBCDatabase-GetQueryInstance
vtkUniformVariables-GetCurrentName
vtkXOpenGLRenderWindow-GetEventPending
vtkXMesaRenderWindow-GetEventPending
vtkMPICommunicator-GetWorldCommunicator
vtkMPICommunicator-GetLocalProcessId
vtkMPICommunicator-GetNumberOfProcesses
}

proc TestOne {cname} {
   global exceptions
   $cname b 
   puts "Testing Class $cname"
   set methods [b ListMethods]
   # look for a Get Set pair
   set len [llength $methods]
   for {set i 0} {$i < $len} {incr i} {
      if {[regsub {^Get([A-za-z0-9]*)} [lindex $methods $i] {\1} name]} {
         if {($i == $len - 1) || ($i < $len - 1 && [lindex $methods [expr $i + 1]] != "with")} {
            if {[lsearch $exceptions "$cname-[lindex $methods $i]"] == -1} {
               # invoke the GetMethod
	       puts "  Invoking Get$name"
               set tmp [b Get$name]
               # find matching set method
               for {set j 0} {$j < $len} {incr j} {
                  if {[regexp "^Set$name" [lindex $methods $j]]} {
                     if {$j < $len - 3 && [lindex $methods [expr $j + 2]] == "1"} {
                        puts "    Invoking Set$name"
                        catch {b Set$name $tmp}
                     }
                     if {$j < $len - 3 && [lindex $methods [expr $j + 2]] > 1} {
                        puts "    Invoking Set$name"
                        catch {eval b Set$name $tmp}
                     }
                  }
               }
            }
         }
      }
   }
  puts "Testing DescribeMethods Class $cname"
  # $object DescribeMethods with no arguments returns a list of methods for the object.
  # $object DescribeMethods <MethodName> returns a list containing the following:
  # MethodName {arglist} {description} {c++ signature} DefiningSuperclass
  set Methods [b DescribeMethods]
  # Find the Get methods
  foreach GetMethod [lsearch -inline -all -glob $Methods Get*] {
    # See how many arguments it requires, and only test get methods with 0 arguments
    if { [llength [lindex [b DescribeMethods $GetMethod] 1]] > 0 } { continue }
    # check the exceptions list
    if {[lsearch $exceptions "$cname-$GetMethod"] != -1} { continue }
    puts "  Invoking $GetMethod"
    set tmp [b $GetMethod]
    set SetMethodSearch Set[string range $GetMethod 3 end]
    foreach SetMethod [lsearch -inline -all $Methods $SetMethodSearch] {
      puts "    Invoking $SetMethod"
      catch { eval b $SetMethod $tmp }
      catch { b $SetMethod $tmp }
    }
  }
  
  
  # Test the PrintRevisions method.
  b PrintRevisions
  b Delete
}

set classExceptions {
   vtkCommand
   vtkFileOutputWindow
   vtkIndent
   vtkOutputWindow
   vtkParallelFactory
   vtkPlanes
   vtkProjectedPolyDataRayBounder
   vtkRayCaster
   vtkTimeStamp
   vtkTkImageViewerWidget
   vtkTkImageWindowWidget
   vtkTkRenderWidget
   vtkImageDataToTkPhoto
   vtkViewRays
   vtkWin32OutputWindow
   vtkWin32ProcessOutputWindow
   vtkXMLFileOutputWindow
}

proc rtSetGetTest { fileid } { 
   global classExceptions
   set totalTime 0.0
   # for every class
   set all [lsort [info command vtk*]]
   foreach a $all {
      if {[lsearch $classExceptions $a] == -1} {
         # test some set get methods
         timer StartTimer
         
         TestOne $a
         
         timer StopTimer
	     set elapsedTime [timer GetElapsedTime]
	     set totalTime [expr $totalTime + $elapsedTime]
	   
	     if { $elapsedTime > 1.0 } {
	       puts "Elapsed Time: $elapsedTime and took longer than 1 second."
	     } else {
	       puts "Elapsed Time: $elapsedTime"
	     }
         puts "Total Elapsed Time: $totalTime"
      }
   }
}

# All tests should end with the following...

puts "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)"
rtSetGetTest stdout

timer Delete

exit
