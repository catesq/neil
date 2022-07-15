
#__seq_keys =

class Seq:
    keys =  '0123456789abcdefghijklmnopqrstuvwxyz'
    map = {
        **dict(zip(keys, range(0x10, len(keys) + 0x10))),
        **{chr(45):0x00, chr(44):0x01}
    }


#seq.map =
#seq.map[chr(45)] = 0x00
#seq.map[chr(44)] = 0x01
