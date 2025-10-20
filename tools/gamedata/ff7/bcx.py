import sys, struct, array, math, os, zlib

bcxMap = {}
normals = []

def vecDot(a,b):
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]

def vecNormal(a):
    lenSqr = vecDot(a,a)
    if lenSqr > 0.0:
        ool = 1.0 / math.sqrt(lenSqr)
        return (a[0] * ool, a[1] * ool, a[2] * ool)
    else:
        return (0,0,0)

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

def loadFieldBin(name):
    # read compressed data
    f = open(name, "rb")
    buf = f.read()
    f.close()
    
    (size,) = struct.unpack_from("<I", buf, 0)

    # decompress
    data = zlib.decompress(buf[8:], 16 | 15)
    if len(data) != size:
        del buf
        del data
        return None
    return data
  
def readModel(data, num_bones, offset_skeleton, num_parts, offset_parts):
    model = {}
    
    skeleton = []
    for i in range(num_bones):
        (length, parent, has_mesh) = struct.unpack_from("<hbB", data, offset_skeleton + i * 4)
        skeleton.append((length, parent, has_mesh != 0))
    model["skeleton"] = skeleton

    parts = []
    for i in range(num_parts):
        part = {}
        (u1, bone_index, num_vertex, num_texcoord, num_quad_color_tex, num_tri_color_tex, num_quad_mono_tex, num_tri_mono_tex, num_tri_mono, num_quad_mono, num_tri_color, num_quad_color, u3, offset_poly, offset_texcoord, offset_flags, offset_control, u4, offset_vertex) = struct.unpack_from("<BBBBBBBBBBBBHHHHHHH", data, offset_parts + i * 0x20)
        # print (u1, bone_index, num_vertex, num_texcoord, num_quad_color_tex, num_tri_color_tex, num_quad_mono_tex, num_tri_mono_tex, num_tri_mono, num_quad_mono, num_tri_color, num_quad_color, u3, offset_poly, offset_texcoord, offset_flags, offset_control, u4, offset_vertex)
        #print "offset_vertex %x %x %x" % (offset_vertex, num_vertex, offset_vertex + 4 + num_vertex * 8)
        #print "offset_texcoord %x %x %x" % (offset_vertex + offset_texcoord, num_texcoord, offset_vertex + offset_texcoord + num_texcoord * 2)
        part["bone_index"] = bone_index
        # print (u1, u3, u4)
        
        vertices = []
        for j in range(num_vertex):
            (x, y, z) = struct.unpack_from("<hhh", data, offset_vertex + 4 + j * 8)
            vertices.append((x,y,z))

            start = offset_vertex + 4 + j * 8
            end = start + struct.calcsize("<hhh")  # 6 bytes total

            # Print the raw bytes in 0xFF notation
            #print("Bytes:", " ".join(f"0x{b:02X}" for b in data[start:end]))
        part["vertices"] = vertices
                
        texcoords = []
        for j in range(num_texcoord):
            (u, v) = struct.unpack_from("<BB", data, offset_vertex + offset_texcoord + j * 2)
            texcoords.append((u,v))
        part["texcoords"] = texcoords
        
        cur_poly = offset_vertex + offset_poly
        cur_flags = offset_vertex + offset_flags
        cur_control = offset_vertex + offset_control

        # print "u4 : %4.4x" % (offset_vertex + u4)
        # print "cur_poly : %4.4x" % (cur_poly)
        # print "cur_flags : %4.4x" % (cur_flags)
        # print "cur_control : %4.4x" % (cur_control)

        quad_color_tex = []
        for j in range(num_quad_color_tex):
            control = struct.unpack_from("<B", data, cur_control)
            cur_control += 1
        
            (av, bv, cv, dv, ar, ag, ab, an, br, bg, bb, bn, cr, cg, cb, cn, dr, dg, db, dn, at, bt, ct, dt) = struct.unpack_from("<BBBBBBBBBBBBBBBBBBBBBBBB", data, cur_poly)
            poly = ((av, at, (ar, ag, ab), an), (bv, bt, (br, bg, bb), bn), (dv, dt, (dr, dg, db), dn), (cv, ct, (cr, cg, cb), cn))
            #print(f"quad_color_tex 0x{ar:02X} 0x{ag:02X} 0x{ab:02X} 0x{an:02X}, 0x{br:02X} 0x{bg:02X} 0x{bb:02X} 0x{bn:02X}, 0x{cr:02X} 0x{cg:02X} 0x{cb:02X} 0x{cn:02X}, 0x{dr:02X} 0x{dg:02X} 0x{db:02X} 0x{dn:02X}")

            # print "quad_color_tex %i %i %i %i / %i %i %i %i" % (av, bv, cv, dv, an, bn, cn, dn)

            quad_color_tex.append(poly)
            cur_poly += 0x18
        part["quad_color_tex"] = quad_color_tex

        tri_color_tex = []
        for j in range(num_tri_color_tex):
            control = struct.unpack_from("<B", data, cur_control)
            cur_control += 1
        
            (av, bv, cv, xv, ar, ag, ab, an, br, bg, bb, bn, cr, cg, cb, cn, at, bt, ct, xt) = struct.unpack_from("<BBBBBBBBBBBBBBBBBBBB", data, cur_poly)
            poly = ((av, at, (ar, ag, ab), an), (bv, bt, (br, bg, bb), bn), (cv, ct, (cr, cg, cb), cn), xv, xt)
            #print(f"tri_color_tex 0x{ar:02X} 0x{ag:02X} 0x{ab:02X} 0x{an:02X}, 0x{br:02X} 0x{bg:02X} 0x{bb:02X} 0x{bn:02X}, 0x{cr:02X} 0x{cg:02X} 0x{cb:02X} 0x{cn:02X}")

            tri_color_tex.append(poly)
            cur_poly += 0x14
        part["tri_color_tex"] = tri_color_tex

        quad_mono_tex = []
        for j in range(num_quad_mono_tex):
            control = struct.unpack_from("<B", data, cur_control)
            cur_control += 1
        
            (av, bv, cv, dv, r, g, b, n, at, bt, ct, dt) = struct.unpack_from("<BBBBBBBBBBBB", data, cur_poly)
            #print(f"quad_mono_tex 0x{r:02X} 0x{g:02X} 0x{b:02X} 0x{n:02X}")
            poly = ((av, at), (bv, bt), (dv, dt), (cv, ct), (r, g, b), n)
            quad_mono_tex.append(poly)
            cur_poly += 0x0C
        part["quad_mono_tex"] = quad_mono_tex

        tri_mono_tex = []
        for j in range(num_tri_mono_tex):
            control = struct.unpack_from("<B", data, cur_control)
            cur_control += 1
        
            (av, bv, cv, xv, r, g, b, n, at, bt, ct, xt) = struct.unpack_from("<BBBBBBBBBBBB", data, cur_poly)
            poly = ((av, at), (bv, bt), (cv, ct), (r, g, b), n, xv, xt)
            #print(f"tri_mono_tex 0x{r:02X} 0x{g:02X} 0x{b:02X} 0x{n:02X}")
            # print "tri_mono_tex", x, xv, xt
            tri_mono_tex.append(poly)
            cur_poly += 0x0C
        part["tri_mono_tex"] = tri_mono_tex

        tri_mono = []
        for j in range(num_tri_mono):
            (av, bv, cv, xv, r, g, b, n) = struct.unpack_from("<BBBBBBBB", data, cur_poly)
            poly = ((av,), (bv,), (cv,), (r, g, b), n, xv)
            #print(f"tri_mono 0x{r:02X} 0x{g:02X} 0x{b:02X} 0x{n:02X}")
            # print "tri_mono", x, xv
            tri_mono.append(poly)
            cur_poly += 8
        part["tri_mono"] = tri_mono

        quad_mono = []
        for j in range(num_quad_mono):
            (av, bv, cv, dv, r, g, b, n) = struct.unpack_from("<BBBBBBBB", data, cur_poly)
            poly = ((av,), (bv,), (dv,), (cv,), (r, g, b), n)
            #print(f"quad_mono 0x{r:02X} 0x{g:02X} 0x{b:02X} 0x{n:02X}")
            # print "quad_mono", x
            quad_mono.append(poly)
            cur_poly += 8
        part["quad_mono"] = quad_mono

        tri_color = []
        for j in range(num_tri_color):
            (av, bv, cv, xv, ar, ag, ab, an, br, bg, bb, bn, cr, cg, cb, cn) = struct.unpack_from("<BBBBBBBBBBBBBBBB", data, cur_poly)
            poly = ((av,(ar, ag, ab), an), (bv, (br, bg, bb), bn), (cv, (cr, cg, cb), cn), xv)
            #print(f"tri_color 0x{ar:02X} 0x{ag:02X} 0x{ab:02X} 0x{an:02X}, 0x{br:02X} 0x{bg:02X} 0x{bb:02X} 0x{bn:02X}, 0x{cr:02X} 0x{cg:02X} 0x{cb:02X} 0x{cn:02X}")

            tri_color.append(poly)
            cur_poly += 0x10
        part["tri_color"] = tri_color

        quad_color = []
        for j in range(num_quad_color):
            (av, bv, cv, dv, ar, ag, ab, an, br, bg, bb, bn, cr, cg, cb, cn, dr, dg, db, dn) = struct.unpack_from("<BBBBBBBBBBBBBBBBBBBB", data, cur_poly)
            poly = ((av,(ar, ag, ab), an), (bv, (br, bg, bb), bn), (dv, (dr, dg, db), dn), (cv, (cr, cg, cb), cn))
            #print(f"quad_color 0x{ar:02X} 0x{ag:02X} 0x{ab:02X} 0x{an:02X}, 0x{br:02X} 0x{bg:02X} 0x{bb:02X} 0x{bn:02X}, 0x{cr:02X} 0x{cg:02X} 0x{cb:02X} 0x{cn:02X}, 0x{dr:02X} 0x{dg:02X} 0x{db:02X} 0x{dn:02X}")

            quad_color.append(poly)
            cur_poly += 0x14
        part["quad_color"] = quad_color
        
        parts.append(part)
                
    model["parts"] = parts
            
    return model
    
def readAnimations(data, num_animations, offset_animations):
    animations = []
    for i in range(num_animations):
        (num_frames, num_channel, num_frames_translation, num_static_translation, num_frames_rotation, offset_frames_translation, offset_static_translation, offset_frames_rotation, offset_data) = struct.unpack_from("<HBBBBHHHI", data, offset_animations + i * 0x10)
        offset_data &= 0x7FFFFFFF
        # print "animation:", (offset_animations + i * 0x10, num_frames, num_channel, num_frames_translation, num_static_translation, num_frames_rotation, offset_frames_translation, offset_static_translation, offset_frames_rotation, offset_data)

        animation = []
        for k in range(num_channel):
            (flag, rx, ry, rz, tx, ty, tz) = struct.unpack_from("<BBBBBBB", data, offset_data + 0x04 + k * 8)

            channel = []
            for j in range(num_frames):

                # rotation
                if flag & 0x01:
                    rotation_x = 360.0 * float(data[offset_data + offset_frames_rotation + rx * num_frames + j]) / 255.0
                else:
                    rotation_x = 360.0 * rx / 255.0

                if flag & 0x02:
                    rotation_y = 360.0 * float(data[offset_data + offset_frames_rotation + ry * num_frames + j]) / 255.0
                else:
                    rotation_y = 360.0 * ry / 255.0

                if flag & 0x04:
                    rotation_z = 360.0 * float(data[offset_data + offset_frames_rotation + rz * num_frames + j]) / 255.0
                else:
                    rotation_z = 360.0 * rz / 255.0

                # translation
                if flag & 0x10:
                    (translation_x,) = struct.unpack_from("<h", data, offset_data + offset_frames_translation + tx * num_frames * 2 + j * 2)
                elif tx != 0xFF:
                    (translation_x,) = struct.unpack_from("<h", data, offset_data + offset_static_translation + tx * 2)
                else:
                    translation_x = 0

                if flag & 0x20:
                    (translation_y,) = struct.unpack_from("<h", data, offset_data + offset_frames_translation + ty * num_frames * 2 + j * 2);
                elif ty != 0xFF:
                    (translation_y,) = struct.unpack_from("<h", data, offset_data + offset_static_translation + ty * 2)
                else:
                    translation_y = 0

                if flag & 0x40:
                    (translation_z,) = struct.unpack_from("<h", data, offset_data + offset_frames_translation + tz * num_frames * 2 + j * 2)
                elif tz != 0xFF:
                    (translation_z,) = struct.unpack_from("<h", data, offset_data + offset_static_translation + tz * 2)
                else:
                    translation_z = 0

                channel.append(((translation_x, translation_y, translation_z), (rotation_x, rotation_y, rotation_z)))

            animation.append(channel)
        animations.append(animation)

    return animations

def loadBcx(file):
    data = loadLzs(file)
    (size, header_offset) = struct.unpack_from("<II", data, 0)
    assert(size == len(data))
    (u1, num_bones, num_parts, num_animations, u2, u3, u4, offset_skeleton) = struct.unpack_from("<HBBB19sHHI", data, header_offset)
    offset_skeleton &= 0x7FFFFFFF
    offset_parts = offset_skeleton + num_bones * 4
    offset_animations = offset_parts + num_parts * 32
    # print "header", (header_offset, u1, num_bones, num_parts, num_animations, u2, u3, u4, offset_skeleton, offset_parts, offset_animations)

    object = {}
    object["model"] = readModel(data, num_bones, offset_skeleton, num_parts, offset_parts)
    object["animations"] = readAnimations(data, num_animations, offset_animations)
    
    return object
    
def loadBsx(file):
    # get compressed file
    data = loadLzs(file)
    (size, header_offset) = struct.unpack_from("<II", data, 0)
    assert(size == len(data))

    # decode individual objects
    object_list = []
    (u1, num_models) = struct.unpack_from("<II", data, header_offset)
    for i in range(num_models):
        model_offset = header_offset + 0x10 + i * 0x30
        (model_id,u2,u3,offset_skeleton,u4,u5,u6,u7,u8,u9,u10,u11,u12,u13,u14,u15,u16,u17,u18,u19,u20,num_bones,u22,u23,u24,u25,u26,u27,u28,u29,u30,u31,u32,num_parts,u34,u35,u36,u37,u38,u39,u40,u41,u42,u43,u44,num_animations) = struct.unpack_from("<HBBHBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", data, model_offset)
        # print (model_id,u2,u3,offset_skeleton,u4,u5,u6,u7,u8,u9,u10,u11,u12,u13,u14,u15,u16,u17,u18,u19,u20,num_bones,u22,u23,u24,u25,u26,u27,u28,u29,u30,u31,u32,num_parts,u34,u35,u36,u37,u38,u39,u40,u41,u42,u43,u44,num_animations)
        offset_skeleton += model_offset
        offset_parts = offset_skeleton + num_bones * 4
        offset_animations = offset_parts + num_parts * 32

        if model_id == 0x0100:
            object = loadBcx(bcxMap["cloud"])
        elif model_id == 0x0302:
            object = loadBcx(bcxMap["ballet"])
        elif model_id == 0x0403:
            object = loadBcx(bcxMap["tifa"])
        elif model_id == 0x0605:
            object = loadBcx(bcxMap["cid"])
        elif model_id == 0x0706:
            object = loadBcx(bcxMap["yufi"])
        elif num_parts > 0:
            object = {}
            object["model"] = readModel(data, num_bones, offset_skeleton, num_parts, offset_parts)
            object["animations"] = []
        else:
            print("unknown model id: %x" % model_id)
        
        object["animations"].extend(readAnimations(data, num_animations, offset_animations))
        
        object_list.append(object)

    return object_list