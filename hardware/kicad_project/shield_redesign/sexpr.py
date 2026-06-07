import re, math

def find_blocks(s, token):
    out, idx = [], 0
    while True:
        k = s.find(token, idx)
        if k < 0: break
        d, j = 0, k
        while j < len(s):
            c = s[j]
            if c == '(': d += 1
            elif c == ')':
                d -= 1
                if d == 0: break
            j += 1
        out.append((k, j+1)); idx = j+1
    return out

def block_at(b):
    m = re.search(r'\n\t\t\(at ([-\d.]+) ([-\d.]+)(?: ([-\d.]+))?\)', b) or \
        re.search(r'\(at ([-\d.]+) ([-\d.]+)(?: ([-\d.]+))?\)', b)
    return (float(m.group(1)), float(m.group(2)), float(m.group(3) or 0)) if m else None

def prop(b, name):
    m = re.search(r'"%s" "([^"]*)"' % re.escape(name), b)
    return m.group(1) if m else None

def rot(px, py, deg):
    a = math.radians(deg)
    return (px*math.cos(a) - py*math.sin(a), px*math.sin(a) + py*math.cos(a))
