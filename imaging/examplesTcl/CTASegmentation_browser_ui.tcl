catch {load vtktcl}
catch {load vtktcl}
#- TOP LEVEL ------------------------------
toplevel .file 
#wm withdraw .file
wm title .file {Filename Browser}
wm resizable .file 0 0
wm withdraw .
wm withdraw .file

set file_apply_op         ""
global prefix
#- Directory string------------------------

frame .file.a -relief ridge -borderwidth 2 
frame .file.b -relief ridge -borderwidth 2 

frame .file.a.f1 -bd 0 
frame .file.a.f2 -bd 0 
frame .file.a.f3 -bd 0 
frame .file.a.f4 -bd 0 

label .file.a.f1.l1 \
	-text "Directory:"

entry .file.a.f1.e1 -font -*-fixed-bold-r-normal--13-* \
	 -highlightthickness 0 

.file.a.f1.e1 insert end [pwd]

pack .file.a.f1.l1 -side left -in .file.a.f1 \
	-expand 0 -fill none

pack .file.a.f1.e1 -side left -in .file.a.f1 \
	-expand 1 -fill both -padx 3 -pady 2


label .file.a.f3.l1 \
	-text "File:"

entry .file.a.f3.e1 -font -*-fixed-bold-r-normal--13-* \
	-highlightthickness 0 \

.file.a.f3.e1 insert end [pwd]
 
pack .file.a.f3.l1  -side left -in .file.a.f3 \
	-expand 0 -fill none
pack .file.a.f3.e1 -side left -in .file.a.f3 \
	-expand 1 -fill both -padx 3 -pady 2


frame .file.a.f2.f1  -borderwidth 2 \
    -relief ridge

frame .file.a.f2.f2  -borderwidth 2 -relief ridge

frame .file.a.f2.f1.f1 

listbox .file.a.f2.f1.f1.dirList \
    -font -*-fixed-bold-r-normal--13-*  \
     -xscrollcommand {.file.a.f2.f1.sbH set}\
    -yscrollcommand {.file.a.f2.f1.f1.sbV set} -width 35
scrollbar .file.a.f2.f1.f1.sbV \
    -command {.file.a.f2.f1.f1.dirList yview}

pack .file.a.f2.f1.f1.dirList .file.a.f2.f1.f1.sbV -side left \
    -in .file.a.f2.f1.f1  -expand 0 -fill y \
    -ipadx 0 -ipady 0 -padx 5 -pady 5

scrollbar .file.a.f2.f1.sbH \
     -command {.file.a.f2.f1.f1.dirList xview}\
    -orient horizontal 
pack .file.a.f2.f1.f1 .file.a.f2.f1.sbH -expand 0 -fill both \
    -ipadx 0 -ipady 0 -padx 5 -pady 5 -side top -in .file.a.f2.f1


frame .file.a.f2.f2.f1 -bg grey75

listbox .file.a.f2.f2.f1.fileList \
    -font -*-fixed-bold-r-normal--13-* \
    -xscrollcommand {.file.a.f2.f2.sbH set}\
    -yscrollcommand {.file.a.f2.f2.f1.sbV set} -width 35
scrollbar .file.a.f2.f2.f1.sbV \
    -command {.file.a.f2.f2.f1.fileList yview}

pack .file.a.f2.f2.f1.fileList .file.a.f2.f2.f1.sbV -side left \
    -in .file.a.f2.f2.f1  -expand 0 -fill y \
    -ipadx 0 -ipady 0 -padx 5 -pady 5

scrollbar .file.a.f2.f2.sbH \
    -command {.file.a.f2.f2.f1.fileList xview}\
    -orient horizontal  
pack .file.a.f2.f2.f1 .file.a.f2.f2.sbH -expand 0 -fill both \
    -ipadx 0 -ipady 0 -padx 5 -pady 5 -side top -in .file.a.f2.f2



pack .file.a.f2.f1 .file.a.f2.f2 -in .file.a.f2 -side left \
	-padx 2 -pady 2

button .file.a.f4.b1 \
	-bd 3 -relief raised -highlightthickness 0 \
	-text Apply -command apply_it

pack .file.a.f4.b1 -side left -padx 12 -ipadx 12 \
	-fill x -expand 1

button .file.b.b1 \
 	-bg grey75 \
	-bd 3 -relief raised -highlightthickness 0 \
	-text Dismiss -command "wm withdraw .file"

pack .file.b.b1 .file.b -expand 1 -fill both

pack .file.a .file.b -in .file -side top -padx 5 -pady 5

pack .file.a.f1 .file.a.f2 .file.a.f3 .file.a.f4 -in .file.a \
    -side top -padx 5 -pady 5 -expand 1 -fill both 


bind .file.a.f2.f1.f1.dirList <Double-Button-1> {

    set current [.file.a.f1.e1 get]
    set index [.file.a.f2.f1.f1.dirList curselection]

    set dir [.file.a.f2.f1.f1.dirList get $index]

    if { $dir == "../ (Up a directory)" } {
        set newDir [file dirname $current]
    } else {
        set newDir $current/$dir
    }

    .file.a.f1.e1 delete 0 end
    .file.a.f1.e1 insert 0 $newDir

    .file.a.f3.e1 delete 0 end
    .file.a.f3.e1 insert 0 $newDir

    listDirectories $newDir
    listFiles $newDir
}


bind .file.a.f2.f2.f1.fileList <ButtonRelease-1> {

    set current [.file.a.f1.e1 get]
    set index [.file.a.f2.f2.f1.fileList curselection]

    set thefile [.file.a.f2.f2.f1.fileList get $index]
    set fullpath $current/$thefile

    .file.a.f3.e1 delete 0 end
    .file.a.f3.e1 insert 0 $fullpath
}

bind .file.a.f2.f2.f1.fileList <Double-Button-1> { 
    set current [.file.a.f1.e1 get]
    set index [.file.a.f2.f2.f1.fileList curselection]

    set thefile [.file.a.f2.f2.f1.fileList get $index]
    set fullpath $current/$thefile

    .file.a.f3.e1 delete 0 end
    .file.a.f3.e1 insert 0 $fullpath

    apply_it 
}

##-List directories for current string
bind .file.a.f1.e1 <KeyPress-Return> {
    set dir [.file.a.f1.e1 get]
    listDirectories $dir
}


##-Procedure generates list of directories----
proc listDirectories {dir} {

    .file.a.f2.f1.f1.dirList delete 0 end

    set dirList [lsort [glob -nocomplain $dir/*/]]
    .file.a.f2.f1.f1.dirList insert end "../ (Up a directory)"
    foreach i $dirList {
        .file.a.f2.f1.f1.dirList insert end [file tail $i]
    }

}

##-Procedure generates list of files----
proc listFiles {dir} {

    .file.a.f2.f2.f1.fileList delete 0 end

    set fileList [lsort [glob -nocomplain $dir/*]]
    foreach i $fileList {
        if { [ file isfile $i ] == 1 } {
	    .file.a.f2.f2.f1.fileList insert end [file tail $i]
	}
    }

}

proc PopupFileControls { } {
  wm deiconify .file
}

proc SetFileApplyString { apply_string } {
 .file.a.f4.b1 configure -text $apply_string
}

proc apply_it { } {
    global file_apply_op viewer reader
    global prefix
    set fullpath [.file.a.f3.e1 get]
    set  idx [string last . $fullpath]
    set  prefix [string range $fullpath 0 [expr $idx -1]]
    puts $prefix
    wm withdraw .file
}

listDirectories [pwd]
listFiles [pwd]
