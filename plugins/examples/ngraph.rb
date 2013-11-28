#! /usr/bin/ruby

require 'ngraph'

shell = Ngraph::Shell.new
sys = Ngraph::System[0]

if (ARGV[0] == '-i')
  ARGV.shift
  init_script = ARGV.shift
else
  init_script = Ngraph.get_initialize_file("Ngraph.nsc")
end

if (init_script)
  ARGV.unshift(init_script)
  shell.shell(ARGV) 
  Ngraph.execute_loginshell(sys.login_shell, shell) if (sys.login_shell)
else
  Ngraph.execute_loginshell(nil, shell)
end

Ngraph.save_shell_history
