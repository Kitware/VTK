# RTest.tcl - a little application to run regression tests
# 	Written by Will
#

# Start by setting environment variables - most of them are found 
# automatically given a "normal" setup.
# Read default values if present; otherwise we'll make a guess. This
# environment variables interact with RTestGUI.tcl and
# rtImageTclExamples.tcl.
#
#    TCL_EXECUTABLE -       Where the Tcl/Tk executable is, defaults to
#                           c:/Program Files/Tcl/bin/wish82.exe
#    BIN_DIR -              Where to find this script and supporting Tcl
#                           stuff, dll's, etc. Default is "".
#    VTK_ROOT -             vtk root directory, 
#                           defaults to where abc/vtk/def is found in the
#                           current directory, minus /vtk/def (to yield abc).
#                           In last case, defaults to current drive.
#    VTK_VALID_IMAGE_PATH - where the valid images are, 
#                           defaults to $VTK_ROOT/vtkbaseline
#    VTK_RESULTS_PATH -     where to put regression test images, 
#                           defaults to "d:/rtResults"
#    VTK_REGRESSION_LOG -   where to send log messages, 
#                           defaults to $VTK_RESULTS_PATH/rt.log
#    VTK_PLATFORM -         the OS of the computer, defaults to "WinNT"
######################### Check environment #################
#######

set currentDirectory [pwd]
if { [catch {set tmp $env(BIN_DIR) }] != 0} {
   if { [file exists "RTestGUI.tcl"] == 0 } {
      # Starting from .bat file
      set env(BIN_DIR) $currentDirectory/bin
      set env(TCL_EXECUTABLE) $env(BIN_DIR)/wish82.exe
      set env(TK_LIBRARY) $env(BIN_DIR)/tk8.2
      set env(TCL_LIBRARY) $env(BIN_DIR)/tcl8.2
   } else {
      # Starting in vtk/examplesTcl
      set env(BIN_DIR) $currentDirectory
      set env(TCL_EXECUTABLE) "c:/Program Files/Tcl/bin/wish82.exe"
   }
}

# Announce the application - make sure splash is available
if { [file exists $env(BIN_DIR)/RTest.gif] } {
   wm overrideredirect . 1
   frame .f -borderwidth 0
   image create photo ColorWheel -file $env(BIN_DIR)/RTest.gif
   label .f.l -image ColorWheel -highlightthickness 0
   pack .f.l -fill both
   pack .f
   update
   set splash 1
} else {
   wm withdraw .
   set splash 0
}

# Set the VTK_ROOT environment variable (if currently "d:\foo\vtk", then
# VTK_ROOT is "d:\foo")
if { [catch {set tmp $env(VTK_ROOT) }] != 0} {
   # See whether we can figure it out
   set idx [string first "/vtk/" $currentDirectory]
   if { $idx >= 0 } {
      set env(VTK_ROOT) [string range $currentDirectory 0 $idx]
   } else {
      set env(VTK_ROOT) $currentDirectory
   }
}

# Set the valid image location
if { [catch {set tmp $env(VTK_VALID_IMAGE_PATH) }] != 0} {
   # Create default
   set env(VTK_VALID_IMAGE_PATH) $env(VTK_ROOT)/vtkbaseline/images
   if { [file isdir $env(VTK_VALID_IMAGE_PATH)] == 0 } {
      tk_messageBox -icon error -message "Can't find vtkimages!"
      exit
   }
}

# Set the vtkdata location
if { [catch {set tmp $env(VTK_DATA) }] != 0} {
   # Create default
   set env(VTK_DATA) $env(VTK_ROOT)/vtkdata
   if { [file isdir $env(VTK_DATA)] == 0 } {
      tk_messageBox -icon error -message "Can't find vtkdata!"
      exit
   }
}

# Set the Tcl auxiliary files location
if { [catch {set tmp $env(VTK_TCL) }] != 0} {
   # Create default
   set env(VTK_TCL) $env(VTK_ROOT)/vtk/examplesTcl
   if { [file isdir $env(VTK_TCL)] == 0 } {
      tk_messageBox -icon error -message "Can't find vtk Tcl!"
      exit
   }
}

# Specify where to put the images
if { [catch {set tmp $env(VTK_RESULTS_PATH) }] != 0} {
   # Create default
   set env(VTK_RESULTS_PATH) $env(VTK_ROOT)/rtResults
}

# Create rtResults directory if it doesn't exist
if { [file isdir $env(VTK_RESULTS_PATH)] == 0 } {
   set created 0
   set response [tk_messageBox -icon question -type yesno -default yes \
         -message "Results directory does not exist, Ok to create it?"]

   if { $response } {
      set types {
        {{All Files }                           *             }
      }
      set dirname [tk_getSaveFile -initialdir "$env(VTK_ROOT)" \
              -title "Create Directory" \
              -initialfile rtResults -filetypes $types]
      if { $dirname != "" } {
         set $env(VTK_RESULTS_PATH) $dirname
         file mkdir $dirname
         set created 1
      }
   }

   if { $created == 0 } {
      tk_messageBox -icon error \
            -message "Working directory is required!"
      exit
   }
}

# Specify where to put the regression test log file
if { [catch {set tmp $env(VTK_REGRESSION_LOG) }] != 0} {
   # Create default
   set env(VTK_REGRESSION_LOG) $env(VTK_RESULTS_PATH)/rt.log
}

if { [catch {set tmp $env(VTK_PLATFORM) }] != 0} {
   # Create default
   set env(VTK_PLATFORM) "WinNT"
}

# Make sure the splash shows for at least 1 second
after 1000
if { $splash } {
   wm withdraw .
}

####################################### Invoke GUI and application
##

exec $env(TCL_EXECUTABLE) $env(BIN_DIR)/RTestGUI.tcl
exit
