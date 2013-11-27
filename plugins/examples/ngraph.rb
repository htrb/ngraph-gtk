#! /usr/bin/ruby

require 'ngraph'

shell = Ngraph::Shell.new

if (ARGV[0] == '-i')
  init_script = ARGV[1]
else
  init_script = Ngraph.get_initialize_file("Ngraph.nsc")
end

if (init_script)
  shell.shell(init_script) 
  Ngraph.execute_loginshell(Ngraph::System[0].login_shell, shell)
else
  Ngraph.execute_loginshell(nil, shell)
end

Ngraph.save_shell_history
