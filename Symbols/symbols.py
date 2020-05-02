"""
 compile the symbol bytes array from bitmap images
 the bitmap needs not be monochrome (but the symbols will look best when starting
 from monochrome images)
 
 to compile the default header, run it with
 
 python symbols.py fluo-on.gif fluo-off.gif transfer.gif transfer-ko.gif -O ../Controller/symbols.h
"""

import os
import sys
import math
try:  # import as appropriate for 2.x vs. 3.x
  import tkinter
except:
  import Tkinter as tkinter

def args():
  overwrite = False
  help = False
  h_fname = 'symbols.h'

  try:
    last_input = sys.argv.index('-o')
    if len(sys.argv) < last_input + 2:
      help = True
    else:
      h_fname = sys.argv[last_input + 1]
  except ValueError:
    try:
      last_input = sys.argv.index('-O')
      overwrite = True
      if len(sys.argv) >= last_input + 2:
        h_fname = sys.argv[last_input + 1]
    except ValueError:
      last_input = len(sys.argv)
  path, ext = os.path.splitext(h_fname)
  if ext != '.h':
    help = True
  array_name = os.path.basename(path)

  if help or last_input == 1:
    print('usage: python symbols.py <image>... [-o|-O [<header>]]')
    print('  <image> list of bitmap files in a format known to Tkinter')
    print('  options:')
    print('    -o <header>   specify .h file name')
    print('    -O [<header>] overwrites existing .h file')
    sys.exit(2)

  if not overwrite and os.path.exists(h_fname):
    print('error: "{}" already exists, use -O option to overwrite'.format(h_fname))
    sys.exit(3)

  input = []
  for i in range(1, last_input):
    if not os.path.exists(sys.argv[i]) or not os.path.isfile(sys.argv[i]):
      print('error: "{}"" not found or not a file'.format(sys.argv[i]))
      sys.exit(4)
    input.append(sys.argv[i])

  return ( input, h_fname, array_name )

def image(fname):
  try:
    return tkinter.PhotoImage(file=fname)
  except tkinter.TclError as error:
    print('error: {}'.format(error))
    path, ext = os.path.splitext(fname)
    if ext.lower() == '.png':
      print('PNG files may not be supported by this version of TK')
    sys.exit(5)

def monochrome(image, lines_count):
  result = []
  bits = image.height() % 8
  for x in range(image.width()):
    i = 0
    byte = 0
    for y in reversed(range(image.height())):
      pixel = image.get(x, y)
      if type(pixel) ==  type(0):
        pixel = [value, value, value]
      elif type(pixel) == type((0,0,0)):
        pixel = list(pixel)
      else:
        pixel = list(map(int, pixel.split()))
      avg = pixel[0] * 0.3 + pixel[1] * 0.59 + pixel[2] * 0.11
      bw = 0 if avg > 127 else 1
      byte = (byte << 1) | bw
      if y % 8 == 0:
        result.append(byte)
        byte = 0
        i += 1
    if bits != 0:
      result.append(byte << 8 - bits)
      i += 1
  bytes_count = int(math.floor((image.height() + 7) / 8))  # +7 to round up, if needed
  for k in range((lines_count - image.width()) * bytes_count):
    result.append(0)
  return result

def resize(fname, image, dim):
  dim = float(dim)
  if image.width() > dim or image.height() > dim:
    scale_w = int(math.ceil(image.width() / dim))
    scale_h = int(math.ceil(image.height() / dim))
    print('warning: downsampling {} (size: {}, {})'.format(fname, image.width(), image.height()))
    return image.subsample(max(scale_w, scale_h))
  else:
    return image

def compileImage(file, fname, image, lines_count, last):
  bytes = monochrome(image, lines_count)
  file.write('{:#04x}, '.format(image.width()))
  for byte in bytes[:-1]:
    file.write('{:#04x}, '.format(byte))
  st = '{:#04x}  // {}\n' if last else '{:#04x}, // {}\n  '
  file.write(st.format(bytes[-1], os.path.basename(fname)))

def compileHeader(h_fname, array_name, images):
  def_name = array_name.upper()
  def_name_h = '__{}_H'.format(def_name)
  last_fname, last_image = images[-1]
  bytes_count = int(math.floor((last_image.height() + 7) / 8))  # +7 to round up, if needed
  lines_count = max(map(lambda current: current[1].width(), images))
  with open(h_fname, 'w') as file:
    file.write('// {}\n'.format(os.path.basename(h_fname)))
    file.write('// generated file, do not edit\n')
    file.write('// run symbols.py to re-generate\n\n')
    file.write('#ifndef {}\n'.format(def_name_h))
    file.write('#define {}\n\n'.format(def_name_h))
    file.write('#define {}_HEIGHT  {}\n'.format(def_name, last_image.height()))
    file.write('#define {}_DEF_LEN (({} * {}) + 1)\n\n'.format(def_name, lines_count, bytes_count))
    file.write('static const byte {}[] PROGMEM = {{\n  '.format(array_name))
    for fname, image in images[:-1]:
      compileImage(file, fname, image, lines_count, False)
    compileImage(file, last_fname, last_image, lines_count, True)
    file.write('};\n\n')
    file.write('#endif\n')

tk = tkinter.Tk()

input, h_fname, array_name = args()
images = map(lambda fname: (fname, resize(fname, image(fname), 50)), input)
ffname, fimage = images[0]
for cfname, cimage in images:
  if cimage.height() != fimage.height():
    print('error: "{}" height is different from "{}" height'.format(cfname, ffname))
    sys.exit(7)
compileHeader(h_fname, array_name, images)
