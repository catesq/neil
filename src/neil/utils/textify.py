#verify/prepare text for display in ui controls
from string import ascii_letters, digits


import zzub

def prepstr(s, fix_underscore=False):
    """
    prepstr ensures that a string is always
    ready to be displayed in a GUI control by wxWidgets.

    @param s: Text to be prepared.
    @type s: str
    @return: Correctly encoded text.
    @rtype: str
    """
    # s = s.decode('latin-1')
    if fix_underscore:
        s = s.replace('_','__')
    return s

def fixbn(v):
    """
    Occasionally, invalid note inputs are being made,
    either by user error or invalid paste or loading operations.
    This function fixes a Buzz note value so it has
    always a correct value.

    @param v: Buzz note value.
    @type v: int
    @return: Corrected Buzz note value.
    @rtype: int
    """
    if v == zzub.zzub_note_value_off:
        return v
    o,n = ((v & 0xf0) >> 4), (v & 0xf)
    o = min(max(o,0),9)
    n = min(max(n,1),12)
    return (o << 4) | n

def filenameify(text):
    """
    Replaces characters in a text in such a way
    that it's feasible to use it as a filename. The
    result will be lowercase and all special chars
    replaced by underscores.

    @param text: The original text.
    @type text: str
    @return: The filename.
    @rtype: str
    """
    return ''.join([(c in (ascii_letters+digits) and c) or '_' for c in text.lower()])


__all__ = [
    'prepstr',
    'fixbn',
    'filenameify'
]