# Based on: https://forums.qhimm.com/index.php?topic=8969.msg122920#msg122920

import sys, struct, array, math, os, zlib
from . import lzss

class ModelPart:
    def __init__(self):
        self.vertices = []
        self.texcoords = []
        self.poly_count = 0
        self.bone_index = 0

        self.quad_color_tex = []
        self.tri_color_tex = []
        self.quad_mono_tex = []
        self.tri_mono_tex = []
        self.tri_mono = []
        self.quad_mono = []
        self.tri_color = []
        self.quad_color = []

class Model:
    def __init__(self):
        self.poly_count = 0
        self.parts = []
        self.skeleton = []
        self.animations = []
        self.triangles = []

    def loadModelFromBCX(self, data, num_bones, offset_skeleton, num_parts, offset_parts):
        for i in range(num_bones):
            (length, parent, has_mesh) = struct.unpack_from("<hbB", data, offset_skeleton + i * 4)
            self.skeleton.append((length, parent, has_mesh != 0))

        for i in range(num_parts):
            (u1, bone_index, num_vertex, num_texcoord, num_quad_color_tex, num_tri_color_tex, num_quad_mono_tex, num_tri_mono_tex, num_tri_mono, num_quad_mono, num_tri_color, num_quad_color, u3, offset_poly, offset_texcoord, offset_flags, offset_control, u4, offset_vertex) = struct.unpack_from("<BBBBBBBBBBBBHHHHHHH", data, offset_parts + i * 0x20)
            
            part = ModelPart()
            part.bone_index = bone_index
            
            for j in range(num_vertex):
                (x, y, z) = struct.unpack_from("<hhh", data, offset_vertex + 4 + j * 8)
                part.vertices.append((x,y,z))

                start = offset_vertex + 4 + j * 8
                end = start + struct.calcsize("<hhh")  # 6 bytes total
                    
            for j in range(num_texcoord):
                (u, v) = struct.unpack_from("<BB", data, offset_vertex + offset_texcoord + j * 2)
                part.texcoords.append((u,v))
            
            cur_poly = offset_vertex + offset_poly
            cur_flags = offset_vertex + offset_flags
            cur_control = offset_vertex + offset_control

            for j in range(num_quad_color_tex):
                control = struct.unpack_from("<B", data, cur_control)
                cur_control += 1
            
                (av, bv, cv, dv, ar, ag, ab, an, br, bg, bb, bn, cr, cg, cb, cn, dr, dg, db, dn, at, bt, ct, dt) = struct.unpack_from("<BBBBBBBBBBBBBBBBBBBBBBBB", data, cur_poly)
                poly = ((av, at, (ar, ag, ab), an), (bv, bt, (br, bg, bb), bn), (dv, dt, (dr, dg, db), dn), (cv, ct, (cr, cg, cb), cn))

                part.quad_color_tex.append(poly)
                cur_poly += 0x18

            for j in range(num_tri_color_tex):
                control = struct.unpack_from("<B", data, cur_control)
                cur_control += 1
            
                (av, bv, cv, xv, ar, ag, ab, an, br, bg, bb, bn, cr, cg, cb, cn, at, bt, ct, xt) = struct.unpack_from("<BBBBBBBBBBBBBBBBBBBB", data, cur_poly)
                poly = ((av, at, (ar, ag, ab), an), (bv, bt, (br, bg, bb), bn), (cv, ct, (cr, cg, cb), cn), xv, xt)

                part.tri_color_tex.append(poly)
                cur_poly += 0x14

            for j in range(num_quad_mono_tex):
                control = struct.unpack_from("<B", data, cur_control)
                cur_control += 1
            
                (av, bv, cv, dv, r, g, b, n, at, bt, ct, dt) = struct.unpack_from("<BBBBBBBBBBBB", data, cur_poly)
                poly = ((av, at), (bv, bt), (dv, dt), (cv, ct), (r, g, b), n)

                part.quad_mono_tex.append(poly)
                cur_poly += 0x0C

            for j in range(num_tri_mono_tex):
                control = struct.unpack_from("<B", data, cur_control)
                cur_control += 1
            
                (av, bv, cv, xv, r, g, b, n, at, bt, ct, xt) = struct.unpack_from("<BBBBBBBBBBBB", data, cur_poly)
                poly = ((av, at), (bv, bt), (cv, ct), (r, g, b), n, xv, xt)

                part.tri_mono_tex.append(poly)
                cur_poly += 0x0C

            for j in range(num_tri_mono):
                (av, bv, cv, xv, r, g, b, n) = struct.unpack_from("<BBBBBBBB", data, cur_poly)
                poly = ((av,), (bv,), (cv,), (r, g, b), n, xv)

                part.tri_mono.append(poly)
                cur_poly += 8

            for j in range(num_quad_mono):
                (av, bv, cv, dv, r, g, b, n) = struct.unpack_from("<BBBBBBBB", data, cur_poly)
                poly = ((av,), (bv,), (dv,), (cv,), (r, g, b), n)

                part.quad_mono.append(poly)
                cur_poly += 8

            for j in range(num_tri_color):
                (av, bv, cv, xv, ar, ag, ab, an, br, bg, bb, bn, cr, cg, cb, cn) = struct.unpack_from("<BBBBBBBBBBBBBBBB", data, cur_poly)
                poly = ((av,(ar, ag, ab), an), (bv, (br, bg, bb), bn), (cv, (cr, cg, cb), cn), xv)

                part.tri_color.append(poly)
                cur_poly += 0x10

            for j in range(num_quad_color):
                (av, bv, cv, dv, ar, ag, ab, an, br, bg, bb, bn, cr, cg, cb, cn, dr, dg, db, dn) = struct.unpack_from("<BBBBBBBBBBBBBBBBBBBB", data, cur_poly)
                poly = ((av,(ar, ag, ab), an), (bv, (br, bg, bb), bn), (dv, (dr, dg, db), dn), (cv, (cr, cg, cb), cn))

                part.quad_color.append(poly)
                cur_poly += 0x14

            part.poly_count = len(part.quad_color_tex) + len(part.tri_color_tex) + len(part.quad_mono_tex) + len(part.tri_mono_tex) + len(part.tri_mono) + len(part.quad_mono) + len(part.tri_color) + len(part.quad_color)
            self.poly_count += part.poly_count
            
            self.parts.append(part)
    
    def loadAnimationsFromBCX(self, data, num_animations, offset_animations):
        for i in range(num_animations):
            (num_frames, num_channel, num_frames_translation, num_static_translation, num_frames_rotation, offset_frames_translation, offset_static_translation, offset_frames_rotation, offset_data) = struct.unpack_from("<HBBBBHHHI", data, offset_animations + i * 0x10)
            offset_data &= 0x7FFFFFFF

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
            self.animations.append(animation)

    def loadModelFromLZS(self, data, ptr):
        numBones = struct.unpack_from("<L", data, ptr)[0]
        rootBone = struct.unpack_from("<Q", data, ptr + 4)[0]

        for b in range(numBones):
            parentBone, boneLength, offsetToBoneObject = struct.unpack_from("<HhL", data, ptr + 12 + (8 * b))
            self.skeleton.append((boneLength, parentBone, offsetToBoneObject != 0))

            if offsetToBoneObject == 0:
                continue

            boneObjectPtr = ptr + offsetToBoneObject

            part = ModelPart()
            part.bone_index = b
            
            # Load Vertices
            vertexPoolSize = struct.unpack_from("<L", data, boneObjectPtr)[0]
            boneObjectPtr += 4
            num_vertex = int(vertexPoolSize / 8)

            for j in range(num_vertex):
                (x, y, z, _) = struct.unpack_from("<hhhh", data, boneObjectPtr)
                part.vertices.append((x,y,z))
                boneObjectPtr += 8

            part.texcoords = []

            # Note: in battle model format the vertex indices are given in bytes rather than indexes, so we devide by 8.
            # Texcoords are not stored in a central table but per poly so we append and reference them as we encounter them.

            part_index = len(self.parts)
            poly_index = 0

            # Load Polys
            num_tri_mono_tex, texTriFlags = struct.unpack_from("<HH", data, boneObjectPtr)
            boneObjectPtr += 4
            for j in range(num_tri_mono_tex):
                (ai, bi, ci, xi, au, av, flags, bu, bv, cu, cv) = struct.unpack_from("<HHHHBBHBBBB", data, boneObjectPtr)
                part.texcoords.append((au, av))
                part.texcoords.append((bu, bv))
                part.texcoords.append((cu, cv))
                at = len(part.texcoords) - 3
                bt = len(part.texcoords) - 2
                ct = len(part.texcoords) - 1
                r = g = b = 255
                poly = ((int(ai / 8), at), (int(bi / 8), bt), (int(ci / 8), ct), (r, g, b), 0, 0, 0)

                part.tri_mono_tex.append(poly)
                boneObjectPtr += 16

                self.triangles.append([part.vertices[int(ai / 8)], part.vertices[int(bi / 8)], part.vertices[int(ci / 8)], part_index, poly_index])
                poly_index += 1

            num_quad_mono_tex, texQuadFlags = struct.unpack_from("<HH", data, boneObjectPtr)
            boneObjectPtr += 4
            for j in range(num_quad_mono_tex):
                (ai, bi, ci, di, au, av, flags, bu, bv, cu, cv, du, dv) = struct.unpack_from("<HHHHBBHBBBBBB", data, boneObjectPtr)
                part.texcoords.append((au, av))
                part.texcoords.append((bu, bv))
                part.texcoords.append((cu, cv))
                part.texcoords.append((du, dv))
                at = len(part.texcoords) - 4
                bt = len(part.texcoords) - 3
                ct = len(part.texcoords) - 2
                dt = len(part.texcoords) - 1
                r = g = b = 255
                poly = ((int(ai / 8), at), (int(bi / 8), bt), (int(di / 8), dt), (int(ci / 8), ct), (r, g, b), 0)
            
                part.quad_mono_tex.append(poly)
                boneObjectPtr += 20

                self.triangles.append([part.vertices[int(ai / 8)], part.vertices[int(bi / 8)], part.vertices[int(ci / 8)], part_index, poly_index])
                self.triangles.append([part.vertices[int(ai / 8)], part.vertices[int(ci / 8)], part.vertices[int(di / 8)], part_index, poly_index])
                poly_index += 1

            num_tri_color, colTriFlags = struct.unpack_from("<HH", data, boneObjectPtr)
            boneObjectPtr += 4
            for j in range(num_tri_color):
                (ai, bi, ci, xi, ar, ag, ab, an, br, bg, bb, bn, cr, cg, cb, cn) = struct.unpack_from("<HHHHBBBBBBBBBBBB", data, boneObjectPtr)
                poly = ((int(ai / 8),(ar, ag, ab), an), (int(bi / 8), (br, bg, bb), bn), (int(ci / 8), (cr, cg, cb), cn), xi)

                part.tri_color.append(poly)
                boneObjectPtr += 20

                self.triangles.append([part.vertices[int(ai / 8)], part.vertices[int(bi / 8)], part.vertices[int(ci / 8)], part_index, poly_index])
                poly_index += 1

            num_quad_color, colQuadFlags = struct.unpack_from("<HH", data, boneObjectPtr)
            boneObjectPtr += 4
            for j in range(num_quad_color):
                (ai, bi, ci, di, ar, ag, ab, an, br, bg, bb, bn, cr, cg, cb, cn, dr, dg, db, dn) = struct.unpack_from("<HHHHBBBBBBBBBBBBBBBB", data, boneObjectPtr)
                poly = ((int(ai / 8),(ar, ag, ab), an), (int(bi / 8), (br, bg, bb), bn), (int(di / 8), (dr, dg, db), dn), (int(ci / 8), (cr, cg, cb), cn))

                part.quad_color.append(poly)
                boneObjectPtr += 24

                self.triangles.append([part.vertices[int(ai / 8)], part.vertices[int(bi / 8)], part.vertices[int(ci / 8)], part_index, poly_index])
                self.triangles.append([part.vertices[int(ai / 8)], part.vertices[int(ci / 8)], part.vertices[int(di / 8)], part_index, poly_index])
                poly_index += 1

            part.poly_count = len(part.quad_color_tex) + len(part.tri_color_tex) + len(part.quad_mono_tex) + len(part.tri_mono_tex) + len(part.tri_mono) + len(part.quad_mono) + len(part.tri_color) + len(part.quad_color)
            self.poly_count += part.poly_count
            
            self.parts.append(part)

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

def loadModelFromBCX(file):
    data = loadLzs(file)
    (size, header_offset) = struct.unpack_from("<II", data, 0)
    assert(size == len(data))
    (u1, num_bones, num_parts, num_animations, u2, u3, u4, offset_skeleton) = struct.unpack_from("<HBBB19sHHI", data, header_offset)
    offset_skeleton &= 0x7FFFFFFF
    offset_parts = offset_skeleton + num_bones * 4
    offset_animations = offset_parts + num_parts * 32

    model = Model()
    model.loadModelFromBCX(data, num_bones, offset_skeleton, num_parts, offset_parts)
    model.loadAnimationsFromBCX(data, num_animations, offset_animations)
    return model
    
def loadModelsFromBSX(file, bcxMap):
    # get compressed file
    data = loadLzs(file)
    (size, header_offset) = struct.unpack_from("<II", data, 0)
    assert(size == len(data))

    modelList = []
    (u1, num_models) = struct.unpack_from("<II", data, header_offset)
    for i in range(num_models):
        model_offset = header_offset + 0x10 + i * 0x30
        (model_id,u2,u3,offset_skeleton,u4,u5,u6,u7,u8,u9,u10,u11,u12,u13,u14,u15,u16,u17,u18,u19,u20,num_bones,u22,u23,u24,u25,u26,u27,u28,u29,u30,u31,u32,num_parts,u34,u35,u36,u37,u38,u39,u40,u41,u42,u43,u44,num_animations) = struct.unpack_from("<HBBHBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", data, model_offset)
        offset_skeleton += model_offset
        offset_parts = offset_skeleton + num_bones * 4
        offset_animations = offset_parts + num_parts * 32

        if model_id == 0x0100:
            object = bcxMap["cloud"]
        elif model_id == 0x0201:
            object = bcxMap["earith"]
        elif model_id == 0x0302:
            object = bcxMap["ballet"]
        elif model_id == 0x0403:
            object = bcxMap["tifa"]
        elif model_id == 0x0504:
            object = bcxMap["red"]
        elif model_id == 0x0605:
            object = bcxMap["cid"]
        elif model_id == 0x0706:
            object = bcxMap["yufi"]
        elif model_id == 0x0807:
            object = bcxMap["ketcy"]
        elif model_id == 0x0908:
            object = bcxMap["vincent"]    
        elif num_parts > 0:
            object = Model()
            object.loadModelFromBCX(data, num_bones, offset_skeleton, num_parts, offset_parts)
        else:
            print("unknown model id: %x" % model_id)
        
        object.loadAnimationsFromBCX(data, num_animations, offset_animations)
        #object.model_id = model_id
        #object.model_index = i
        
        modelList.append(object)

    return modelList

def loadModelFromLZS(file):
    # decompress the file
    cmpData = file.read()
    compressedSize = struct.unpack_from("<L", cmpData)[0]
    data = bytearray(lzss.decompress(cmpData[4:4 + compressedSize]))

    # We only want the main model data section for now which is section 0
    numSections = struct.unpack_from("<L", data, 0)[0]
    ptr = struct.unpack_from("<L", data, 4)[0]

    model = Model()
    model.loadModelFromLZS(data, ptr)
    return model

def loadFieldBin(f):
    # read compressed data
    buf = f.read()
    
    (size,) = struct.unpack_from("<I", buf, 0)

    # decompress
    data = zlib.decompress(buf[8:], 16 | 15)
    if len(data) != size:
        del buf
        del data
        return None
    return data