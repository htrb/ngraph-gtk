#! /usr/bin/ruby
puts("set +e")

while (l = gets)
  a = l.chomp.split("#")
  print <<EOF
RESULT=`dexpr '#{a[0]}'`
if [ "$RESULT" != "#{a[1]}" ]
then
  echo error '#{a[0]}' '#{a[1]}' "$RESULT".
fi

EOF
end

puts("del system")
