import sys, struct, array, math, os, zlib
import numpy as np

def loadLzs(f):
    # read compressed data
    buf = f.read()
    
    (size,) = struct.unpack_from("<I", buf, 0)
    if size + 4 != len(buf):
        del buf
        return None

    ibuf = array.array("B", buf)
    obuf = array.array("B")
    
    # decompress;
    iofs = 4
    cmd = 0
    bit = 0
    while iofs < len(ibuf):
        if bit == 0:
            cmd = ibuf[iofs]
            bit = 8
            iofs += 1
        if cmd & 1:
            obuf.append(ibuf[iofs])
            iofs += 1
        else:
            a = ibuf[iofs]
            iofs += 1
            b = ibuf[iofs]
            iofs += 1
            o = a | ((b & 0xF0) << 4)
            l = (b & 0xF) + 3;

            rofs = len(obuf) - ((len(obuf) - 18 - o) & 0xFFF)
            for j in range(l):
                if rofs < 0:
                    obuf.append(0)
                else:
                    obuf.append(obuf[rofs])
                rofs += 1
        cmd >>= 1
        bit -= 1

    return obuf.tobytes()

def signExtend(value, bits):
    sign_bit = 1 << (bits - 1)
    return (value & (sign_bit - 1)) - (value & sign_bit)

def getUintFromBits(data, n_bits, bit_offset):
    value = 0
    for i in range(n_bits):
        byte_index = (bit_offset + i) // 8
        bit_index = 7 - ((bit_offset + i) % 8)  # <-- MSB first!
        bit = (data[byte_index] >> bit_index) & 1
        value = (value << 1) | bit
    return value

def getIntFromBits(data, n_bits, bit_offset):
    return signExtend(getUintFromBits(data, n_bits, bit_offset), n_bits)

def translateMatrix(tx, ty, tz):
    M = np.eye(4)
    M[:3, 3] = [tx, ty, tz]
    return M

def rotateXMatrix(deg):
    rad = np.radians(deg)
    c, s = np.cos(rad), np.sin(rad)
    M = np.eye(4)
    M[1, 1] = c
    M[1, 2] = -s
    M[2, 1] = s
    M[2, 2] = c
    return M

def rotateYMatrix(deg):
    rad = np.radians(deg)
    c, s = np.cos(rad), np.sin(rad)
    M = np.eye(4)
    M[0, 0] = c
    M[0, 2] = s
    M[2, 0] = -s
    M[2, 2] = c
    return M

def rotateZMatrix(deg):
    rad = np.radians(deg)
    c, s = np.cos(rad), np.sin(rad)
    M = np.eye(4)
    M[0, 0] = c
    M[0, 1] = -s
    M[1, 0] = s
    M[1, 1] = c
    return M

def transformPoint(mat4, point3):
    # Convert to homogeneous (x, y, z, 1)
    p = np.append(point3, 1.0)

    # Apply the matrix
    transformed = mat4 @ p

    # Divide by w (for perspective transforms)
    if transformed[3] != 0:
        transformed /= transformed[3]

    return transformed[:3]

def vecDot(a,b):
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]

def vecNormal(a):
    lenSqr = vecDot(a,a)
    if lenSqr > 0.0:
        ool = 1.0 / math.sqrt(lenSqr)
        return (a[0] * ool, a[1] * ool, a[2] * ool)
    else:
        return (0,0,0)
    
# Ray–Triangle Intersection (Möller–Trumbore)
def rayTriangleIntersect(ray_origin, ray_dir, v0, v1, v2):
    EPSILON = 1e-8
    edge1 = v1 - v0
    edge2 = v2 - v0
    h = np.cross(ray_dir, edge2)
    a = np.dot(edge1, h)
    if -EPSILON < a < EPSILON:
        return None  # Parallel
    f = 1.0 / a
    s = ray_origin - v0
    u = f * np.dot(s, h)
    if u < 0.0 or u > 1.0:
        return None
    q = np.cross(s, edge1)
    v = f * np.dot(ray_dir, q)
    if v < 0.0 or u + v > 1.0:
        return None
    t = f * np.dot(edge2, q)
    if t > EPSILON:
        return t  # Intersection distance
    return None