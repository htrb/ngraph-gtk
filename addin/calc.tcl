#!/usr/bin/wish

#
# calc.tcl written by S. ISHIZAKA 1998/01
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

proc loadsettings {} {
  global minimum maximum division incmin incmax formula output
  if { [file exist $output] } {
    set f [ open $output r ]
    gets $f line1
    gets $f line2
    close $f
    scan $line1 "-s%s -mx%s -my%s" s1 s2 s3
    scan $line2 "%s %f %f %d %d %d" tag min max div imin imax
    if { $tag == "%CALC.NSC" } {
      set minimum $min
      set maximum $max
      set division $div
      set incmin $imin
      set incmax $imax
      set formula $s3
    }
  }
}

proc savedata {} {
  global minimum maximum division incmin incmax output formula
  set min $minimum
  set max $maximum
  set div $division
  if {$incmin != 0} {
    set st 0
  } else {
    set st 1
  }
  if {$incmax != 0} {
    set ed $div
  } else {
    set ed [ expr $div-1 ]
  }
  if { [file exist $output] } {
    if {[dialog .d {File exists} "Overwrite existing $output?"] != 1} {
      return 0
    }
  }
  set f [ open $output w ]
  puts $f "-s2 -mxX -my$formula"
  puts $f "%CALC.NSC $min $max $div $incmin $incmax"
  for {set i $st} {$i <= $ed} {incr i 1} {
    set x [ expr double($min)+(double($max)-double($min))/double($div)*$i ]
    set line "$x $x"
    puts $f $line
  }
  close $f
  return 1
}

proc savescript {} {
  global script output formula
  if { $script != "" } {
    set f [ open $script w ]
    puts $f "new file"
    puts $f "file::file=$output"    
    puts $f "file::head_skip=2"
    puts $f "file::type=line"
    puts $f "file::math_y='$formula'"
    close $f
  }
}

proc exitproc {} {
  global setdata
  if { [ savedata ] == 1 } {
    if { $setdata != 0} {
      savescript
    }
    exit
  }
}

proc setupwindow {} {
  frame .top -relief raised -bd 1
  frame .bot -relief raised -bd 1
  pack .top -side top -fill both
  pack .bot -side bottom -fill both 

  frame .titlef -relief sunken -bd 1
  frame .outputf 
  frame .formulaf
  frame .minimumf
  frame .maximumf
  frame .divisionf
  frame .setdataf
  pack .titlef -in .top -side top -fill x -pady 5m -padx 5m -anchor center
  pack .outputf -in .top -side top -fill x -padx 5m -anchor w
  pack .formulaf .minimumf .maximumf .divisionf .setdataf \
       -in .top -side top -fill x -pady 1m -padx 5m -anchor w

  label .title -text "Calc version 1.00.02" 
  pack .title -in .titlef -side top -pady 1m -padx 5m -anchor center

  label .title2 -text "making a data file" 
  pack .title2 -in .titlef -side top -pady 1m -padx 5m -anchor center

  label .outputl -text " Output:"
  entry .output -width 20 -relief sunken -bd 2 -textvariable output
  pack .outputl -in .outputf -side left -anchor w
  pack .output -in .outputf -expand 1 -fill x -side left -anchor w

  label .formulal -text "Formula:"
  entry .formula -width 20 -relief sunken -bd 2 -textvariable formula
  button .load -text "LOAD" -command loadsettings
  pack .formulal .formula .load -in .formulaf -side left -anchor w

  label .minl -text "Minimum:"
  entry .min -width 15 -relief sunken -bd 2 -textvariable minimum
  checkbutton .includemin -text "Include min" -variable incmin
  pack .minl .min .includemin -in .minimumf -side left -anchor w

  label .maxl -text "Maximum:"
  entry .max -width 15 -relief sunken -bd 2 -textvariable maximum
  checkbutton .includemax -text "Include max" -variable incmax
  pack .maxl .max .includemax -in .maximumf -side left -anchor w

  label .divl -text "Division:"
  entry .div -width 10 -relief sunken -bd 2 -textvariable division
  pack .divl .div -in .divisionf -side left -anchor w

  checkbutton .setdata -text "Open as data" -variable setdata
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


set script $argv
set min 0.0
set max 1.0
set div 100
set incmin 1
set incmax 1
set output "#calc#.dat"
set minimum $min
set maximum $max
set division $div
set setdata 1
set formula "X"

loadsettings
setupwindow
