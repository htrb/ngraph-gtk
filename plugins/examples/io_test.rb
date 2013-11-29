#! /usr/bin/ngraph
require 'ngraph'

test_str = "test str..."
test_str2 = "123"
TEST_FILE = Ngraph::System[0].temp_file

Ngraph::Io.new {|io|

  # gets and puts test
  io.mode = "w+"
  io.open(TEST_FILE)
  io.puts(test_str)
  io.rewind
  str = io.gets.chomp
  if (str != test_str)
    puts("error. (gets and puts)")
  else
    puts("OK. (gets and puts)")
  end

  # getc and putc test
  TEST_CHAR = 48
  io.putc(TEST_CHAR)
  io.whence = Ngraph::Io::Whence::CUR
  io.seek(-1)
  ch = io.getc
  if (ch.ord != TEST_CHAR)
    puts("error. (getc and putc)")
  else
    puts("OK. (getc and putc)")
  end

  # read and write test
  test_str = "123"
  io.write(test_str)
  io.whence = Ngraph::Io::Whence::CUR
  io.seek(-3)
  str = io.read(3)
  if (str != test_str)
    puts("error. (read and write)")
    puts("$str")
    puts("$test_str")
  else
    puts("OK. (read and write)")
  end

  # flush test
  io.flush

  # tell test
  if (io.tell != 16)
    puts("error. (tell)")
  else
    puts("OK. (tell)")
  end

  io.getc

  # eof test
  if io.eof
  then
    puts("OK. (eof)")
  else
    puts("error. (eof)")
  end

  io.close

  # popen test
  io.mode = "r"
  io.popen("ls io_test.rb")
  str = io.gets
  if (str != "io_test.rb")
    puts("error. (popen)")
  else
    puts("OK. (popen)")
  end
}
