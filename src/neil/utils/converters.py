# convert between one scale to another(decibels to linear),(buzz note to midi note)
# or translate a value as a text string to display

#

import math, zzub

def db2linear(val, limit = -48.0):
    """
    Translates a dB volume to a linear amplitude.

    @param val: Volume in dB.utils
    @type val: float
    @param limit: If val is lower than limit, 0.0 will be returned.
    @type limit: float
    @return: Linear amplitude.
    @rtype: float
    """
    if val == 0.0:
        return 1.0
    if val <= limit:
        return 0.0
    return 10 ** (val / 20.0)

def linear2db(val, limit = -48.0):
    """
    Translates a linear amplitude to a dB volume.

    @param val: Linear amplitude between 0.0 and 1.0.
    @type val: float
    @param limit: If amplitude is zero or lower, limit will be returned.
    @type limit: float
    @return: Volume in dB.
    @rtype: float
    """
    if val <= 0.0:
        return limit
    return math.log(val) * 20.0 / math.log(10)

def format_time(t):
    """
    Translates a time value into a string of the format
    "h:mm:ss:ms".

    @param t: Relative time value.
    @type t: float
    @return: String of the format "h:mm:ss:ms".
    @rtype: str
    """
    h = int(t / 3600)
    m = int((t % 3600) / 60)
    s = t % 60
    ms = int((t - int(t))*10.0)
    return "%i:%02i:%02i:%i" % (h,m,s,ms)

def ticks_to_time(ticks,bpm,tpb):
    """
    Translates positions in ticks as returned by zzub
    to time values.

    @param ticks: Tick value as returned by zzub.
    @type ticks: int
    @param bpm: Beats per minutes.
    @type bpm: int
    @param tpb: Ticks per beats.
    @type tpb: int
    @return: Relative time value.
    @rtype: float
    """
    return (float(ticks)*60) / (bpm * tpb)


def bn2mn(v):
    """
    Converts a Buzz note value into a MIDI note value.

    @param v: Buzz note value.
    @type v: int
    @return: MIDI note value.
    @rtype: int
    """
    if v == zzub.zzub_note_value_off:
        return 255
    return ((v & 0xf0) >> 4)*12 + (v & 0xf)-1

def mn2bn(v):
    """
    Converts a MIDI note value into a Buzz note value.

    @param v: MIDI note value.
    @type v: int
    @return: Buzz note value.
    @rtype: int
    """
    if v == 255:
        return zzub.zzub_note_value_off
    return (int(v/12) << 4) | ((v%12)+1)

NOTES = ('?-','C-','C#','D-','D#','E-','F-','F#','G-','G#','A-','A#','B-')

def note2str(p,v):
    """
    Translates a Buzz note value into a string of the format
    "NNO", where NN is note, and O is octave.

    @param p: A parameter object. You can supply None here
    if the value is not associated with a plugin parameter.
    @type p: zzub.Parameter
    @return: A string of the format "NNO", where NN is note,
    and O is octave, or "..." for no value.
    @rtype: str
    """
    if p and (v == p.get_value_none()):
        return '...'
    if v == zzub.zzub_note_value_off:
        return 'off'
    o,n = (v & 0xf0) >> 4, v & 0xf
    return "%s%i" % (NOTES[n],o)

def switch2str(p,v):
    """
    Translates a Buzz switch value into a hexstring ready to
    be printed in a pattern view.

    @param p: A plugin parameter object.
    @type p: zzub.Parameter
    @param v: A Buzz switch value.
    @type v: int
    @return: A 1-digit hexstring or "." for no value.
    @rtype: str
    """
    if v == p.get_value_none():
        return '.'
    return "%1X" % v

def byte2str(p,v):
    """
    Translates a Buzz byte value into a hexstring ready to
    be printed in a pattern view.

    @param p: A plugin parameter object.
    @type p: zzub.Parameter
    @param v: A Buzz byte value.
    @type v: int
    @return: A 2-digit hexstring or ".." for no value.
    @rtype: str
    """
    if v == p.get_value_none():
        return '..'
    return "%02X" % v

def word2str(p,v):
    """
    Translates a Buzz word value into a hexstring ready to
    be printed in a pattern view.

    @param p: A plugin parameter object.
    @type p: zzub.Parameter
    @param v: A Buzz word value.
    @type v: int
    @return: A 4-digit hexstring or "...." for no value.
    @rtype: str
    """
    if v == p.get_value_none():
        return '....'
    return "%04X" % v



def buffersize_to_latency(bs, sr):
    """
    Translates buffer size to latency.

    @param bs: Size of buffer in samples.
    @type bs: int
    @param sr: Samples per second in Hz.
    @type sr: int
    @return: Latency in ms.
    @rtype: float
    """
    return (float(bs) / float(sr)) * 1000.0

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -




    

__all__ = [
    'db2linear', 
    'linear2db', 
    'format_time', 
    'ticks_to_time', 
    'bn2mn', 
    'mn2bn', 
    'note2str', 
    'switch2str', 
    'byte2str', 
    'word2str', 
    'buffersize_to_latency',
]