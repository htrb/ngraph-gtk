#!/usr/bin/wish
#
# fft.tcl written by S. ISHIZAKA 1999/07
#

proc dialog { w title text} {
  global dlgbutton
  toplevel $w -class Dialog
  wm title $w $title
  wm iconname $w Dialog
  wm geometry $w [wm geometry [winfo toplevel [winfo parent $w]]]
  wm geometry $w {}
  wm transient $w [winfo toplevel [winfo parent $w]]
  frame $w.top -relief raised -bd 1
  pack $w.top -side top -fill both
  frame $w.bot -relief raised -bd 1
  pack $w.bot -side bottom -fill both

  message $w.top.msg -width 3i -text $text
  pack $w.top.msg -side right -expand 1 -fill both -padx 3m -pady 3m

  button $w.bot.ok -text OK -command "set dlgbutton 1"
  frame $w.bot.default -relief sunken -bd 1
  raise $w.bot.ok
  pack $w.bot.default -side left -expand 1 -padx 2m -pady 2m
  pack $w.bot.ok -in $w.bot.default -side left -padx 1m -pady 1m \
       -ipadx 2m -ipady 1m
  button $w.bot.cancel -text Cancel -command "set dlgbutton 0"
  pack $w.bot.cancel -side left -expand 1 -padx 2m -pady 2m \
       -ipadx 2m -ipady 1m

  bind $w <Return> "$w.bot.ok flash; set dlgbutton ok"

  set oldFocus [focus]
  grab set $w
  focus $w
  tkwait variable dlgbutton
  destroy $w
  focus $oldFocus
  return $dlgbutton  
}

proc savescript {} {
  global script output

  if { $script != "" } {
    set f [ open $script w ]
    puts $f "new file"
    puts $f "file::x=1"
    puts $f "file::y=2"
    puts $f "file::file=$output"
    puts $f "file::type=line"
    puts $f "file::B=255"
    puts $f "new file"
    puts $f "file::file=$output"
    puts $f "file::x=1"
    puts $f "file::y=3"
    puts $f "file::type=line"
    puts $f "file::R=255"
    puts $f "new file"
    puts $f "file::file=$output"
    puts $f "file::x=1"
    puts $f "file::y=2"
    puts $f "file::type=staircase_x"
    puts $f "file::math_y='SQRT(%02^2+%03^2)'"
    puts $f "menu::modified=TRUE"
    close $f
  }
}

proc exitproc {} {
  global com input output setdata

  set errorlevel [catch {exec $com $input $output} msg]
  if { $errorlevel != 0 } {
    dialog .d $com $msg
    return
  }
  if { $setdata != 0} {
    savescript
  }
  exit
}

proc setupwindow {} {
  frame .top -relief raised -bd 1
  frame .bot -relief raised -bd 1
  pack .top -side top -fill both
  pack .bot -side bottom -fill both 

  frame .titlef -relief sunken -bd 1
  frame .inputf 
  frame .outputf 
  frame .setdataf
  pack .titlef -in .top -side top -fill x -pady 5m -padx 5m -anchor center
  pack .inputf -in .top -side top -fill x -padx 5m -anchor w
  pack .outputf -in .top -side top -fill x -padx 5m -anchor w
  pack .setdataf -in .top -side top -fill x -pady 1m -padx 5m -anchor w

  label .title -text "FFT version 1.00.00" 
  pack .title -in .titlef -side top -pady 1m -padx 5m -anchor center

  label .inputl -text " Input:"
  entry .input -width 20 -relief sunken -bd 2 -textvariable input
  pack .inputl -in .inputf -side left -anchor w
  pack .input -in .inputf -expand 1 -fill x -side left -anchor w

  label .outputl -text "Output:"
  entry .output -width 20 -relief sunken -bd 2 -textvariable output
  pack .outputl -in .outputf -side left -anchor w
  pack .output -in .outputf -expand 1 -fill x -side left -anchor w

  checkbutton .setdata -text "Set as data" -variable setdata
  pack .setdata -in .setdataf -side left -anchor w

  button .ok -text "OK" -command exitproc 
  frame .default -relief sunken -bd 1
  raise .ok
  pack .default -side left -expand 1 -in .bot -padx 2m -pady 2m
  pack .ok -in .default -side left -padx 1m -pady 1m -ipadx 2m -ipady 1m

  button .cancel -text "Cancel" -command exit
  pack .cancel -in .bot -side left -expand 1 -padx 2m -pady 2m \
       -ipadx 2m -ipady 1m
}

set com [lindex $argv 0]
set script [lindex $argv 1]
set input ""
set output "#fft#.dat"
set setdata 1

setupwindow
