# RTest.tcl - a little application to run regression tests
# 	Written by Will Schroeder
#

# Start by setting environment variables - most of them are found 
# automatically given a "normal" setup.
# Read default values if present; otherwise we'll make a guess. This
# environment variables interact with RTestGUI.tcl and
# rtImageTclExamples.tcl.
#
#    TCL_EXECUTABLE -       Where the Tcl/Tk executable is, defaults to
#                           [info nameofexecutable]
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
      set env(TCL_EXECUTABLE) [info nameofexecutable]
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
      set dirname [tk_getSaveFile -initialdir "$env(VTK_ROOT)" \
              -title "Location of vtkbaseline/images" ]
      if { $dirname != "" && [file isdir $env(VTK_VALID_IMAGE_PATH)] != 0 } {
         set $env(VTK_VALID_IMAGE_PATH) $dirname
      } else {
	  exit
      }
   }
}

# Set the vtkdata location
if { [catch {set tmp $env(VTK_DATA) }] != 0} {
   # Create default
   set env(VTK_DATA) $env(VTK_ROOT)/vtkdata
   if { [file isdir $env(VTK_DATA)] == 0 } {
      set dirname [tk_getSaveFile -initialdir "$env(VTK_ROOT)" \
              -title "Location of vtkdata/" ]
      if { $dirname != "" && [file isdir $env(VTK_DATA)] != 0 } {
         set $env(VTK_DATA) $dirname
      } else {
	  exit
      }
   }
}

# Set the Tcl auxiliary files location
if { [catch {set tmp $env(VTK_TCL) }] != 0} {
   # Create default
   set env(VTK_TCL) $env(VTK_ROOT)/vtk/examplesTcl
   if { [file isdir $env(VTK_TCL)] == 0 } {
      set dirname [tk_getSaveFile -initialdir "$env(VTK_TCL)" \
              -title "Location of vtk/examplesTcl/" ]
      if { $dirname != "" && [file isdir $env(VTK_TCL)] != 0 } {
         set $env(VTK_TCL) $dirname
      } else {
	  exit
      }
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

proc DestroySelf {} {
  destroy .plat
}

# Make sure the splash shows for at least 1 second
after 1000

# Check operating system to get correct images
#
if { [catch {set tmp $env(VTK_PLATFORM) }] != 0} {
    # Offer the user a choice
    set vtkPlatform ""
    toplevel .plat
    wm title .plat "OS Choice"
    wm protocol .plat WM_DELETE_WINDOW {wm withdraw .plat}
    
    frame .plat.f1
    label .plat.f1.l -text "Choose operating system"
    pack .plat.f1.l -padx 5 -pady 5 -fill both -expand t

    frame .plat.f2
    frame .plat.f2.fl
    radiobutton .plat.f2.fl.generic -variable vtkPlatform -text Generic \
	    -value ""
    radiobutton .plat.f2.fl.windows -variable vtkPlatform -text Windows \
	    -value WinNT
    radiobutton .plat.f2.fl.linuxRH -variable vtkPlatform -text LinuxRH \
	    -value linuxRH52
    radiobutton .plat.f2.fl.irix6n32 -variable vtkPlatform -text Irix6n32 \
	    -value irix6n32
    frame .plat.f2.fr
    radiobutton .plat.f2.fl.irix6x64 -variable vtkPlatform -text Irix6x64 \
	    -value irix6x64
    radiobutton .plat.f2.fr.irix65 -variable vtkPlatform -text Irix65 \
	    -value irix65
    radiobutton .plat.f2.fr.solaris26 -variable vtkPlatform -text Solaris26 \
	    -value solaris26
    radiobutton .plat.f2.fr.solaris26m6 -variable vtkPlatform -text Solaris26m6 \
	    -value solaris26m6
    radiobutton .plat.f2.fr.solaris27 -variable vtkPlatform -text Solaris27 \
	    -value solaris27
    radiobutton .plat.f2.fr.hp -variable vtkPlatform -text Hp \
	    -value hp

    pack .plat.f2.fl.generic .plat.f2.fl.windows .plat.f2.fl.linuxRH \
	    .plat.f2.fl.irix6n32 .plat.f2.fl.irix6x64 \
            -padx 3 -pady 3 -side top -anchor w -expand 0 -fill y

    pack .plat.f2.fr.irix65 .plat.f2.fr.solaris26 \
	    .plat.f2.fr.solaris26m6 .plat.f2.fr.solaris27 .plat.f2.fr.hp \
            -padx 3 -pady 3 -side top -anchor w -expand 0 -fill y

    pack .plat.f2.fl .plat.f2.fr \
	    -side left -expand 1 -fill both

    frame .plat.fb
    button .plat.fb.apply -text Apply -command DestroySelf
    button .plat.fb.cancel -text Cancel \
	    -command {set vtkPlatform ""; DestroySelf}
    pack .plat.fb.apply .plat.fb.cancel -side left \
	    -expand 1 -fill x
    pack .plat.f1 .plat.f2 .plat.fb -side top -fill both -expand 1

    update
    tkwait window .plat

    set env(VTK_PLATFORM) $vtkPlatform
}

# Can quit advertising now
if { $splash } {
   wm withdraw .
}

####################################### Invoke GUI and application
##

exec $env(TCL_EXECUTABLE) $env(BIN_DIR)/RTestGUI.tcl
exit
