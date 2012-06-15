class Spline
  def initialize(pos)
    @n = 0
    @x = []
    @y = []
    @t = []

    (pos.size/ 2).times { |i|
      next if (i > 0 && @x[-1] == pos[i * 2] && @y[-1] == pos[i * 2 + 1])

      @x.push(pos[i * 2])
      @y.push(pos[i * 2 + 1])

      if (i == 0)
        @t[i] = 0
      elsif (@x.size > 1)
        t1 = @x[-1] - @x[-2]
        t2 = @y[-1] - @y[-2]
        @t.push(@t.last + Math.sqrt(t1 ** 2 + t2 ** 2))
      end
    }

    @n = @x.size

    (1 .. @n - 1).each { |i|
      @t[i] /= @t[@n - 1]
    }

    @xz = []
    @yz = []

    maketable(@t, @x, @xz)
    maketable(@t, @y, @yz)
  end

  def maketable(x, y, z)
    h = []
    d = []

    z[0] = 0
    z[@n - 1] = 0
    (@n - 1).times { |i|
      h[i    ] =  x[i + 1] - x[i]
      d[i + 1] = (y[i + 1] - y[i]) / h[i].to_f
    }

    z[1] = d[2] - d[1] - h[0] * z[0]
    d[1] = 2 * (x[2] - x[0])

    (1 .. @n - 3).each { |i|
      t = h[i] / d[i].to_f
      z[i + 1] = d[i + 2] - d[i + 1] - z[i] * t
      d[i + 1] = 2 * (x[i + 2] - x[i]) - h[i] * t
    }
    z[@n - 2] -= h[@n - 2] * z[@n - 1]
    (@n - 2).downto(1) { |i|
      z[i] = (z[i] - h[i] * z[i + 1]) / d[i].to_f
    }
  end

  def spline_sub(x, y, z, t)
    i = 0
    j = @n - 1
    while (i < j)
      k = (i + j) / 2
      if (x[k] < t)
        i = k + 1
      else
        j = k
      end
    end
    i -= 1 if (i > 0)
    h = x[i + 1] - x[i]
    d = t - x[i]
    return (((z[i + 1] - z[i]) * d / h.to_f + z[i] * 3) * d + ((y[i + 1] - y[i]) / h.to_f - (z[i] * 2 + z[i + 1]) * h)) * d + y[i]
  end

  def spline(t)
    x = spline_sub(@t, @x, @xz, t)
    y = spline_sub(@t, @y, @yz, t)
    return [x, y]
  end
end


def coord_conv(v)
  (v * COORD_CONV * 100).to_i
end

def unit_conv(v)
  (v * COORD_CONV * 100 / RESOLUTION).to_i
end

def set_line_attribute(f, type, width, cap, join, len)
  len = coord_conv(len)
  style = [
           [],
           [len, len],
           [50, len],
           [len, 100, 50, 100],
           [len, 100, 50, 100, 50, 100],
           [len, 100, 50, 100, 50, 100, 50, 100],
          ]

  
  n = style[type].size
  f.print("A,#{n + 5},#{n},#{coord_conv(width) + 1},#{cap},#{join},1000")
  if (n > 0)
    f.print(",#{style[type].join(',')}")
  end
  f.print("\n")
end

def set_color(f, col)
  return unless (col)
  f.puts("G,4,#{col.join(',')},255")
end

def draw_ellipse(f, a)
  return if (a.size != 16)
end

def draw_poly_line(f, a, b)
  return if (a.size != 16)

  prm = a.map{|i| i.to_i}

  return if (prm[1] == 5)

  n = prm[15] * 2
  fill = (prm[8] != -1)

  close_path = false
  if (b[0] == b[-2] && b[1] == b[-1])
    n -= 2
    close_path = true
  end

  if (prm[1] != 1 || fill)
    if (fill)
      set_color(f, COLOR[prm[5]])
      f.puts("D,#{n + 2},#{n / 2},1,#{b[0, n].join(',')}")
    end

    if (prm[3] > 0)
      set_color(f, COLOR[prm[4]])
      set_line_attribute(f, prm[2], prm[3] / 80.0, prm[11], prm[10], prm[9]) if (fill)
      f.puts("D,#{n + 2},#{n / 2},0,#{b[0, n].join(',')}")
    end
  else
    set_color(f, COLOR[prm[4]])
    set_line_attribute(f, prm[2], prm[3] / 80.0, prm[11], prm[10], prm[9])
    if (close_path)
      f.puts("D,#{n + 2},#{n / 2},0,#{b[0, n].join(',')}")
    else
      f.puts("R,#{n + 1},#{n / 2},#{b[0, n].join(',')}")
    end
  end
end

SPLINE_DIV = 10

def uniq_pos(pos)
  n = pos.size / 2
  a = []
  n.times {|i|
    x = pos[i * 2]
    y = pos[i * 2 + 1]
    if (i == 0)
      a.push(x)
      a.push(y)
    else
      if (a[-2] != x || a[-1] != y)
        a.push(x)
        a.push(y)
      end
    end
  }
  return a
end

def draw_spline(f, a, b)
  return if (a.size != 14)

  prm = a.map{|i| i.to_i}

  n = prm[13] * 2
  fill = (prm[8] != -1)

  pos = []
  subpath = []

  state = b[n, n / 2]

  state.each_with_index {|v, i|
    if (v == 0) 
      m = subpath.size / 2
      if (m > 2)
        s = Spline.new(subpath)
        (1 .. m * SPLINE_DIV - 1).each { |j|
          x, y = s.spline(j * 1.0 / m / SPLINE_DIV)
          pos.push(x.to_i)
          pos.push(y.to_i)
        }
      elsif (m == 2)
        pos.push(subpath[2])
        pos.push(subpath[3])
      end
      pos.push(b[i * 2])
      pos.push(b[i * 2 + 1])
      subpath.clear
      subpath.push(b[i * 2])
      subpath.push(b[i * 2  + 1])
    else
      if (subpath[-1] != b[i * 2 + 1] || subpath[-2] != b[i * 2])
        subpath.push(b[i * 2])
        subpath.push(b[i * 2  + 1])
      end
    end
  }

  pos = uniq_pos(pos)
  n = pos.size

  close_path = false
  if (pos[0] == pos[-2] && pos[1] == pos[-1])
    n -= 2
    close_path = true
  end

  return if (n < 4)

  if (prm[1] % 2 != 0 || fill)
    if (fill)
      set_color(f, COLOR[prm[5]])
      f.puts("D,#{n + 2},#{n / 2},1,#{pos.join(',')}")
    end

    if (prm[3] > 0)
      set_color(f, COLOR[prm[4]])
      set_line_attribute(f, prm[2], prm[3] / 80.0, prm[10], 2, prm[9]) if (fill)
      f.puts("D,#{n + 2},#{n / 2},0,#{pos.join(',')}")
    end
  else
    set_color(f, COLOR[prm[4]])
    set_line_attribute(f, prm[2], prm[3] / 80.0, prm[10], 2, prm[9])
    if (close_path)
      f.puts("D,#{n + 2},#{n / 2},0,#{pos.join(',')}")
    else
      f.puts("R,#{n + 1},#{n / 2},#{pos.join(',')}")
    end
  end
end

PS_FONT = [
           ["Serif", 0], #       0      Times Roman
           ["Serif", 2], #       1      Times Italic
           ["Serif", 1], #       2      Times Bold
           ["Serif", 3], #       3      Times Bold Italic
           ["Sans-serif", 0], #  4      AvantGarde Book
           ["Sans-serif", 2], #  5      AvantGarde Book Oblique
           ["Sans-serif", 1], #  6      AvantGarde Demi
           ["Sans-serif", 3], #  7      AvantGarde Demi Oblique
           ["Serif", 0], #       8      Bookman Light
           ["Serif", 2], #       9      Bookman Light Italic
           ["Serif", 1], #      10      Bookman Demi
           ["Serif", 3], #      11      Bookman Demi Italic
           ["Monospace", 0], #  12      Courier
           ["Monospace", 2], #  13      Courier Oblique
           ["Monospace", 1], #  14      Courier Bold
           ["Monospace", 3], #  15      Courier Bold Oblique
           ["Sans-serif", 0], # 16      Helvetica
           ["Sans-serif", 2], # 17      Helvetica Oblique
           ["Sans-serif", 1], # 18      Helvetica Bold
           ["Sans-serif", 3], # 19      Helvetica Bold Oblique
           ["Sans-serif", 0], # 20      Helvetica Narrow
           ["Sans-serif", 2], # 21      Helvetica Narrow Oblique
           ["Sans-serif", 1], # 22      Helvetica Narrow Bold
           ["Sans-serif", 3], # 23      Helvetica Narrow Bold Oblique
           ["Serif", 0], #      24      New Century Schoolbook Roman
           ["Serif", 2], #      25      New Century Schoolbook Italic
           ["Serif", 1], #      26      New Century Schoolbook Bold
           ["Serif", 3], #      27      New Century Schoolbook Bold Italic
           ["Serif", 0], #      28      Palatino Roman
           ["Serif", 2], #      29      Palatino Italic
           ["Serif", 1], #      30      Palatino Bold
           ["Serif", 3], #      31      Palatino Bold Italic
           ["Serif", 0], #      32      Symbol
           ["Serif", 2], #      33      Zapf Chancery Medium Italic
           ["Serif", 0], #      34      Zapf Dingbats
           nil
          ]

LATEX_FONT = [
              nil, #               0      Default font
              ["Serif", 0], #      1      Roman
              ["Serif", 1], #      2      Bold
              ["Serif", 2], #      3      Italic
              ["Sans-serif", 0], # 4      Sans Serif
              ["Monospace", 3],  # 5      Typewriter
             ]

PAPER_SIZE = {
  "Letter"  => [21600,  27900],
  "Legal"   => [21600,  35600],
  "Ledger"  => [43200,  27900],
  "Tabloid" => [27900,  43200],
  "A"       => [22900,  30500],
  "B"       => [30500,  45700],
  "C"       => [45700,  61000],
  "D"       => [61000,  91400],
  "E"       => [91400, 121900],
  "A4"      => [21000,  29700],
  "A3"      => [29700,  42000],
  "A2"      => [42000,  59400],
  "A1"      => [59400,  84100],
  "A0"      => [84100, 118900],
  "B5"      => [17600,  25000],
}

def draw_text(f, a)
  return if (a.size != 14)

  prm = a.map{|i| i.to_i}

  flag = prm[8]
  f = prm[5]
  if (flag & 4 == 0)
    font = LATEX_FONT[f]
  else
    font = PS_FONT[f]
  end

  set_color(f, COLOR[prm[2]])

  if (font)
    style = font[1]
    f.puts("F#{font[0]}")
  else
    style = 0
  end

  s = a[13]

  f.puts("H,4,#{prm[6] * 100},0,#{(a[7].to_f / Math::PI * 180 * 100).to_i},#{style}")
  f.puts("M,2,#{unit_conv(prm[11])},#{unit_conv(prm[12])}")
  f.puts("S#{s[0, s.size - 4]}")
end

def draw_arc(f, a)
end

COLOR = [
         [0, 0, 0],
         [0, 0, 0xff],
         [0, 0xff, 0],
         [0, 0xff, 0xff],
         [0xff, 0, 0],
         [0xff, 0, 0xff],
         [0xff, 0xff, 0],
         [0xff, 0xff, 0xff],
         [0, 0, 0x90],
         [0, 0, 0xB0],
         [0, 0, 0xD0],
         [0x87, 0xCE, 0xFF],
         [0, 0x90, 0],
         [0, 0xB0, 0],
         [0, 0xD0, 0],
         [0, 0x90, 0x90],
         [0, 0xB0, 0xB0],
         [0, 0xD0, 0xD0],
         [0x90, 0, 0x90],
         [0xB0, 0, 0xB0],
         [0xD0, 0, 0xD0],
         [0x80, 0x30, 0],
         [0xA0, 0x40, 0],
         [0xC0, 0x60, 0],
         [0xFF, 0x80, 0x80],
         [0xFF, 0xA0, 0xA0],
         [0xFF, 0xC0, 0xC0],
         [0xFF, 0xE0, 0xE0],
         [0xFF, 0xD7, 0xE00],
        ]

exit(1) if (ARGV.size != 2)

f = IO.readlines(ARGV[0])
l = f.select {|s| s[0] != ?# } 

exit(1) if (l.size < 9)

head = l[0, 8].map {|s| s.chomp}

RESOLUTION = head[7].split[0].to_i
COORD_CONV = (head[2] == "Metric") ? 1 : 25.4

psize = PAPER_SIZE[head[3]]
psize = [21000, 29700] unless (psize)

File.open(ARGV[1], "w") {|f|
  f.puts('%Ngraph GRAF')
  f.puts('%Creator: fig2gra.rb')
  f.puts("I,5,0,0,#{psize.join(',')},10000")
  f.puts("V,5,0,0,#{psize.join(',')},0")

  i = 8
  while (l[i])
    a = l[i].split
    cmd = a[0].to_i

    b = []
    i += 1
    while (l[i] && l[i][0, 1] == "\t")
      b += l[i].split.map {|v| unit_conv(v.to_i)}
      i += 1
    end

    case cmd
    when 0
      s = a[2][1, 6]
      c = [s[0, 2].hex, s[2, 2].hex, s[4, 2].hex]
      COLOR[a[1].to_i] = c
    when 1
      draw_ellipse(f, a)
    when 2
      draw_poly_line(f, a, b)
    when 3
      draw_spline(f, a, b)
    when 4
      draw_text(f, a)
    when 5
      draw_arc(f, a)
    when 6
    end
  end

  f.puts('E,0')
}
