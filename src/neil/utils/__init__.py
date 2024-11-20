# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Neil Development Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

"""
Provides utility functions needed all over the place,
which have no specific module or class they belong to.
"""

import math, os
import struct


# avoid import the ui submodule here to break a recursive import
# where ui imports from the components module and the components modules imports utils

from .base import *
from .colors import *
from .paths import *
from .plugin import *
from .converters import *
from .textify import *


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


sizes = Sizes()


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


# line is from la_xy to lb_xy, point is pt_xy
def distance_from_line(la_xy, lb_xy, pt_xy):
    x1, y1 = la_xy
    x2, y2 = lb_xy
    x0, y0 = pt_xy

    return abs((y2 - y1) * x0 - (x2 - x1) * y0 + x2 * y1 - y2 * x1) / math.sqrt((y2 - y1) ** 2 + (x2 - x1) ** 2)



# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -







def read_int(f):
    """
    Reads an 32bit integer from a binary file.
    """
    return struct.unpack('<I', f.read(4))[0]

def read_string(f):
    """
    Reads a pascal string (32bit len, data) from a binary file.
    """
    size = read_int(f)
    return str(f.read(size), 'utf-8')

def write_int(f,v):
    """
    Writes a 32bit integer to a binary file.
    """
    f.write(struct.pack('<I', v))

def write_string(f,s):
    """
    Writes a pascal string (32bit len, data) to a binary file.
    """
    s = str(s).encode('utf-8')
    write_int(f, len(s))
    f.write(s)

def blend_float(rgb1, rgb2, weight = 0.5):
    return [
        rgb1[0] * weight + rgb2[0] * (1 - weight),
        rgb1[1] * weight + rgb2[1] * (1 - weight),
        rgb1[2] * weight + rgb2[2] * (1 - weight)
    ]

def blend(color1, color2, weight = 0.5):
    """
        Blend (lerp) two Gdk.Colors
    """
    return Gdk.Color (
        color1.red_float   * weight + color2.red_float   * (1 - weight),
        color1.green_float * weight + color2.green_float * (1 - weight),
        color1.blue_float  * weight + color2.blue_float  * (1 - weight))

def from_hsb(h=0.0,s=1.0,b=1.0):
    """
    Converts hue/saturation/brightness into red/green/blue components.
    """
    if not s:
        return b,b,b
    scaledhue = (h%1.0)*6.0
    index = int(scaledhue)
    fraction = scaledhue - index
    p = b * (1.0 - s)
    q = b * (1.0 - s*fraction)
    t = b * (1.0 - s*(1.0 - fraction))
    if index == 0:
        return b,t,p
    elif index == 1:
        return q,b,p
    elif index == 2:
        return p,b,t
    elif index == 3:
        return p,q,b
    elif index == 4:
        return t,p,b
    elif index == 5:
        return b,p,q
    return b,p,q

def to_hsb(r,g,b):
    """
    Converts red/green/blue into hue/saturation/brightness components.
    """
    if (r == g) and (g == b):
        h = 0.0
        s = 0.0
        b = r
    else:
        v = float(max(r,g,b))
        temp = float(min(r,g,b))
        diff = v - temp
        if v == r:
            h = (g - b)/diff
        elif v == g:
            h = (b - r)/diff + 2
        else:
            h = (r - g)/diff + 4
        if h < 0:
            h += 6
        h = h / 6.0
        s = diff / v
        b = v
    return h,s,b

# is a pixel (px, py) inside the rectangle (x1, y1, x2, y2)  
# where x1,y1 is top left and x2,y2 is bottom right
def box_contains(px, py, rect):
    return (px >= rect[0] and px <= rect[2] and py >= rect[1] and py <= rect[3])


def format_filesize(size):
    if (size / (1<<40)):
        return "%.2f TB" % (float(size) / (1<<40))
    elif (size / (1<<30)):
        return "%.2f GB" % (float(size) / (1<<30))
    elif (size / (1<<20)):
        return "%.2f MB" % (float(size) / (1<<20))
    elif (size / (1<<10)):
        return "%.2f KB" % (float(size) / (1<<10))
    else:
        return "%i bytes" % size



def diff(oldlist, newlist):
    """
    Compares two lists and returns a list of elements that were
    added, and a list of elements that were removed.
    """
    return [x for x in newlist if x not in oldlist],[x for x in oldlist if x not in newlist] # add, remove



from itertools import islice, chain, repeat

def partition(iterable, part_len):
    """
    Partitions a list into specified slices
    """
    itr = iter(iterable)
    while 1:
        item = tuple(islice(itr, part_len))
        if len(item) < part_len:
            raise StopIteration
        yield item

def padded_partition(iterable, part_len, pad_val=None):
    """
    Partitions a list, with optional padding character support.
    """
    padding = repeat(pad_val, part_len-1)
    itr = chain(iter(iterable), padding)
    return partition(itr, part_len)


def get_new_pattern_name(plugin):
    """
    Finds an unused pattern name.
    """
    patternid = 0
    while True:
        s = "%02i" % patternid
        found = False
        for p in plugin.get_pattern_list():
            if p.get_name() == s:
                found = True
                patternid += 1
                break
        if not found:
            break
    return s

class CancelException(Exception):
    """
    Is being thrown when the user hits cancel in a sequence of
    modal UI dialogs.
    """


def camelcase_to_unixstyle(s):
    o = ''
    for c in s:
        if c.isupper() and o and not o.endswith('_'):
            o += '_'
        o += c.lower()
    return o





def show_manual():
    """
    Invoke yelp program with the Neil manual.
    """
    import webbrowser
    path = docpath("manual")
    webbrowser.open_new("%s/index.html" % path)


def show_machine_manual(name):
    """
    Invoke yelp program to display the manual of a specific machine.
    Parameter name is the long name of the machine in all lower caps
    and with spaces replaced by underscores.
    """
    import webbrowser
    path = docpath("manual")
    if os.path.isfile((path + "/%s.html") % name):
        webbrowser.open_new("%s/%s.html" % (path, name))
        return True
    else:
        return False




def synchronize_list(old_list, new_list, insert_entry_func=None, del_entry_func=None, swap_entry_func=None):
    """
    synchronizes a list with another using the smallest number of operations required.
    both lists need to contain only unique elements, that is: no list may contain the same element twice.

    if no functions are supplied for insert, del and swap operations, all operations will be directly
    performed on old_list.
    """
    def insert_entry(i,o):
        old_list.insert(i,o)
    def del_entry(i):
        del old_list[i]
    def swap_entry(i,j):
        a = old_list[i]
        b = old_list[j]
        del_entry_func(j)
        del_entry_func(i)
        insert_entry_func(i,b)
        insert_entry_func(j,a)
    if not insert_entry_func:
        insert_entry_func = insert_entry
    if not del_entry_func:
        del_entry_func = del_entry
    if not swap_entry_func:
        swap_entry_func = swap_entry
    original_list = list(old_list) # make a copy we synchronize changes with
    # remove all indices that are gone, in reverse order so
    # we don't shift indices around
    for index,item in reversed(list(enumerate(original_list))):
        if not item in new_list:
            del original_list[index]
            del_entry_func(index)
    # insert all new items from first to last, so indices
    # shift properly to fit.
    for index,item in enumerate(new_list):
        if not item in original_list:
            original_list.insert(index, item)
            insert_entry_func(index, item)
    # now both lists are in sync, but wrongly sorted
    assert len(original_list) == len(new_list)
    for i,item in enumerate(original_list):
        if new_list[i] == item:
            continue # already in the right place
        # find the new location
        j = new_list.index(item)
        # move entry
        if (i > j):
            i,j = j,i
        a = original_list[i]
        b = original_list[j]
        del original_list[j]
        del original_list[i]
        original_list.insert(i,b)
        original_list.insert(j,a)
        swap_entry_func(i,j)


# TODO: these used to exists at some point. why not now?
# 'add_accelerator',
# 'show_plugin_manual',

if __name__ == '__main__':
    oldlist = [1,2,6,3,4,5]
    newlist = [1,3,4,5,2]
    def insert_entry(i,o):
        print("insert",i,o)
        oldlist.insert(i,o)
    def del_entry(i):
        print("del",i)
        del oldlist[i]
    def swap_entry(i,j):
        print("swap",i,j)
        a,b = oldlist[i],oldlist[j]
        del oldlist[j]
        del oldlist[i]
        oldlist.insert(i,b)
        oldlist.insert(j,a)
    print(oldlist, newlist)
    synchronize_list(oldlist,newlist,insert_entry,del_entry,swap_entry)
    print(oldlist, newlist)


    sizes = sizer(test1=2,test2="test1 * 2")
    print(sizes.get('test1'), sizes.get('test2'))
