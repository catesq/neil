



def cmp_view(a, b):
    a_order = (hasattr(a, '__view__') and a.__view__.get('order',0)) or 0
    b_order = (hasattr(b, '__view__') and b.__view__.get('order',0)) or 0
    return a_order <= b_order