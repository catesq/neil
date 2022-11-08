from neil.utils import fixbn, bn2mn, mn2bn, note2str, switch2str, byte2str, word2str
import config
t2c = [
        note2str,
        switch2str,
        byte2str,
        word2str,
]
t2w = [3, 1, 2, 4]
t2si = [2, 1, 2, 4]
t2siofs = [[0, 2], [0], [0, 1], [0, 1, 2, 3]]

sc2note = {
        90: (0, 0),
        83: (0, 1),
        88: (0, 2),
        68: (0, 3),
        67: (0, 4),
        86: (0, 5),
        71: (0, 6),
        66: (0, 7),
        72: (0, 8),
        78: (0, 9),
        74: (0, 10),
        77: (0, 11),
        44: (1, 0),
        81: (1, 0),
        50: (1, 1),
        87: (1, 2),
        51: (1, 3),
        69: (1, 4),
        82: (1, 5),
        53: (1, 6),
        84: (1, 7),
        54: (1, 8),
        89: (1, 9),
        55: (1, 10),
        85: (1, 11),
        73: (2, 0),
        57: (2, 1),
        79: (2, 2),
        48: (2, 3),
        80: (2, 4),
}


def get_str_from_param(p, v):
    """
    Extracts a string representation from value in context of a parameter.

    @param p: Parameter.
    @type p: zzub.Parameter
    @param v: Value.
    @type v: int
    """
    return t2c[p.get_type()](p, v)


def get_length_from_param(p):
    """
    Gets length of a parameter.

    @param p: Parameter.
    @type p: zzub.Parameter
    """
    return t2w[p.get_type()]


def get_subindexcount_from_param(p):
    """
    Gets subindex count of a parameter.

    @param p: Parameter.
    @type p: zzub.Parameter
    """
    return t2si[p.get_type()]


def get_subindexoffsets_from_param(p):
    return t2siofs[p.get_type()]

def key_to_note(k):
    """
    uses the active keymap to determine note and
    octave from a pressed key.

    @param k: Pressed key.
    @type k: int
    @return: a tuple consisting of octave and note or None.
    @rtype: (int,int)
    """
    keymap = config.get_config().get_keymap()
    rows = keymap.split('|')
    if k < 128:
        k = chr(k).lower().upper()
    else:
        k = chr(k)
    for row, index in zip(rows, range(len(rows))):
        if k in row:
            note = row.index(k)
            return index + int(note / 12), note % 12
    return None
