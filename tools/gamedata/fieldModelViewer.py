# Based on: https://forums.qhimm.com/index.php?topic=8969.msg122920#msg122920

import sys, struct, array, math, os, zlib
import ff7

from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GL.ARB.texture_rectangle import *
from OpenGL.GL.ARB.multitexture import *

import imgui
from imgui.integrations.pygame import PygameRenderer
import pygame
from pygame.locals import *

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
    
class OpenGLObject:
    def __init__(self, object):
        self.object = object
        self.time = 0
        self.animation = 0
        self.draw_normals = False
        self.draw_skeleton = True
        self.part_map = {}
        self.exclude_part = -1
        self.focus_part = -1
        self.focus_poly = -1
        model = object["model"]
        self.parts_visible = []
        self.polys_visible = []
        for idx, part in enumerate(model["parts"]):
            self.part_map[part["bone_index"]] = idx
            self.parts_visible.append(True)
            self.polys_visible.append([True] * part["poly_count"])

    def getPart(self, idx):
        model = self.object["model"]
        return model["parts"][idx]

    def nextAnim(self):
        self.animation = (self.animation + 1) % len(self.object["animations"])
        self.time = 0

    def nextExcludePart(self):
        self.exclude_part += 1
        if self.exclude_part >= len(self.part_map):
            self.exclude_part = -1
        print("Exclude Part: " + str(self.focus_part))
        
    def nextFocusPart(self):
        self.focus_part += 1
        self.focus_poly = -1
        if self.focus_part >= len(self.part_map):
            self.focus_part = -1
        print("Focus Part: " + str(self.focus_part))

    def prevFocusPoly(self):
        if self.focus_poly > -1:
            self.focus_poly -= 1
        print("Focus Poly: " + str(self.focus_poly))

    def nextFocusPoly(self):
        if self.focus_part == -1:
            return
        part = self.getPart(self.focus_part)
        self.focus_poly += 1
        if self.focus_poly >= part["poly_count"]:
            self.focus_poly = -1
        print("Focus Poly: " + str(self.focus_poly))

    def drawPart(self, part, polys_visible):

        # Packing order:
        # quadColorTex;
        # triColorTex;
        # quadMonoTex;
        # triMonoTex;
        # triMono;
        # quadMono;
        # triColor;
        # quadColor;

        poly_index = 0

        glEnable(GL_TEXTURE_2D)

        # quadColorTex;
        glBegin(GL_QUADS)
        for quad in part["quad_color_tex"]:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            for i in range(4):
                pos = part["vertices"][quad[i][0]]
                texcoord = part["texcoords"][quad[i][1]]
                col = quad[i][2]

                glTexCoord2i(texcoord[0], texcoord[1])
                glColor3ub(col[0], col[1] * 0, col[2] * 0)
                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()

        # triColorTex;
        glBegin(GL_TRIANGLES)
        for tri in part["tri_color_tex"]:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            for i in range(3):
                pos = part["vertices"][tri[i][0]]
                texcoord = part["texcoords"][tri[i][1]]
                col = tri[i][2]

                glTexCoord2i(texcoord[0], texcoord[1])
                glColor3ub(col[0], col[1] * 0, col[2] * 0)
                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()
    
        # quadMonoTex
        glBegin(GL_QUADS)
        for quad in part["quad_mono_tex"]:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            col = quad[4]
            glColor3ub(col[0], col[1], col[2])
            for i in range(4):
                pos = part["vertices"][quad[i][0]]
                texcoord = part["texcoords"][quad[i][1]]

                glTexCoord2i(texcoord[0], texcoord[1])
                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()

        # triMonoTex
        glBegin(GL_TRIANGLES)
        for tri in part["tri_mono_tex"]:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            col = tri[3]
            glColor3ub(col[0], col[1], col[2])
            for i in range(3):
                pos = part["vertices"][tri[i][0]]
                texcoord = part["texcoords"][tri[i][1]]

                glTexCoord2i(texcoord[0], texcoord[1])
                glColor3ub(col[0], col[1] * 0, col[2] * 0)
                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()
        
        glDisable(GL_TEXTURE_2D)
        
        # triMono
        glBegin(GL_TRIANGLES)
        for tri in part["tri_mono"]:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            col = tri[3]
            glColor3ub(col[0], col[1], col[2])
            for i in range(3):
                pos = part["vertices"][tri[i][0]]

                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()

        # quadMono
        glBegin(GL_QUADS)
        for quad in part["quad_mono"]:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            col = quad[4]
            glColor3ub(col[0], col[1] * 0, col[2] * 0)
            for i in range(4):
                pos = part["vertices"][quad[i][0]]

                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()

        # triColor
        glBegin(GL_TRIANGLES)
        for tri in part["tri_color"]:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            for i in range(3):
                pos = part["vertices"][tri[i][0]]
                col = tri[i][1]

                glColor3ub(col[0], col[1], col[2])
                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()
        
        # quadColor
        glBegin(GL_QUADS)
        for quad in part["quad_color"]:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            for i in range(4):
                pos = part["vertices"][quad[i][0]]
                col = quad[i][1]

                glColor3ub(col[0], col[1], col[2])
                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()

        # draw normals (I'm not sure about the mono primitives. Maybe they're not lit at all?)
        if self.draw_normals:
            global normals
            glBegin(GL_LINES)
            glColor3f(1,1,0)

            for quad in part["quad_mono_tex"]:
                for i in range(4):
                    pos = part["vertices"][quad[i][0]]
                    nrm = normals[quad[5]]

                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))

            for tri in part["tri_mono_tex"]:
                for i in range(3):
                    pos = part["vertices"][tri[i][0]]
                    nrm = normals[tri[4]]

                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))

            for quad in part["quad_mono"]:
                for i in range(4):
                    pos = part["vertices"][quad[i][0]]
                    nrm = normals[quad[5]]

                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))

            for tri in part["tri_mono"]:
                for i in range(3):
                    pos = part["vertices"][tri[i][0]]
                    nrm = normals[tri[4]]

                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))

            for quad in part["quad_color_tex"]:
                for i in range(4):
                    pos = part["vertices"][quad[i][0]]
                    nrm = normals[quad[i][3]]

                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))

            for tri in part["tri_color_tex"]:
                for i in range(3):
                    pos = part["vertices"][tri[i][0]]
                    nrm = normals[tri[i][3]]

                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))

            for quad in part["quad_color"]:
                for i in range(4):
                    pos = part["vertices"][quad[i][0]]
                    nrm = normals[quad[i][2]]
                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))

            for tri in part["tri_color"]:
                for i in range(3):
                    pos = part["vertices"][tri[i][0]]
                    nrm = normals[tri[i][2]]
                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))
            
            glEnd()
        
    def draw(self):
        glMatrixMode(GL_MODELVIEW)

        glRotatef(180, 1, 0, 0)

        model = self.object["model"]
        animation = self.object["animations"][self.animation]
        skeleton = model["skeleton"]
        self.time = (self.time + 1) % len(animation[0])
        
        parent = [-1]
        for idx, bone in enumerate(model["skeleton"]):
            ((translation_x, translation_y, translation_z), (rotation_x, rotation_y, rotation_z)) = animation[idx][self.time]

            while parent[-1] != bone[1]:
                parent.pop()
                glPopMatrix()
            parent.append(idx)
            glPushMatrix()

            if self.draw_skeleton:
                glBegin(GL_LINES)
                glColor3f(1.0, 0.0, 1.0)
                glVertex3i(0, 0, 0)
                glVertex3i(0, 0, bone[0])
                glColor3f(1.0, 0.0, 0.0)
                glVertex3i(0, 0, 0)
                glVertex3i(50, 0, 0)
                glColor3f(0.0, 1.0, 0.0)
                glVertex3i(0, 0, 0)
                glVertex3i(0, 50, 0)
                glColor3f(0.0, 0.0, 1.0)
                glVertex3i(0, 0, 0)
                glVertex3i(0, 0, 50)
                glEnd()

            glTranslatef(0, 0, bone[0])            
            glRotatef(rotation_y, 0, 1, 0)
            glRotatef(rotation_x, 1, 0, 0)
            glRotatef(rotation_z, 0, 0, 1)
            glTranslatef(translation_x, translation_y, translation_z)

            if bone[2]:
                part_idx = self.part_map[idx]
                if self.parts_visible[self.part_map[idx]]:
                    self.drawPart(model["parts"][part_idx], self.polys_visible[part_idx])

        while parent[-1] != -1:
            parent.pop()
            glPopMatrix()
        
def draw_parts_ui(parts_visible, polys_visible):
    """
    parts_visible: list[bool]  -> visibility of each part
    polys_visible: list[list[bool]] -> per-part poly visibilities
    """
    for part_idx, part_visible in enumerate(parts_visible):
        if imgui.tree_node(f"Part {part_idx}"):
            changed, new_part_visible = imgui.checkbox(f"Visible##part_{part_idx}", part_visible)
            parts_visible[part_idx] = new_part_visible

            if not part_visible and new_part_visible:
                polys_visible[part_idx] = [True] * len(polys_visible[part_idx])

            # If part is visible, show its polys
            if new_part_visible:
                for poly_idx, poly_visible in enumerate(polys_visible[part_idx]):
                    # Unique ID per part + poly
                    changed, new_poly_visible = imgui.checkbox(
                        f"Poly {poly_idx}##part_{part_idx}_poly_{poly_idx}", poly_visible
                    )
                    polys_visible[part_idx][poly_idx] = new_poly_visible
            else:
                # If the part is hidden, set all polys to hidden too
                polys_visible[part_idx] = [False] * len(polys_visible[part_idx])

            imgui.tree_pop()
    
def main(*argv):
    pygame.init()
    
    width = 1280
    height = 960

    screen = pygame.display.set_mode((width, height), HWSURFACE|DOUBLEBUF|OPENGL)
    pygame.display.set_caption("FF7 Field Model Viewer")

    # setup imgui
    imgui.create_context()
    impl = PygameRenderer()

    io = imgui.get_io()
    io.display_size = (width, height)

    glViewport(0, 0, 1280, height)
    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    kFOVy = 0.57735
    kZNear = 10.0
    kZFar = 3000.0
    aspect = (width / height) * kZNear * kFOVy
    glFrustum(-aspect, aspect, -(kZNear * kFOVy), (kZNear * kFOVy), kZNear, kZFar)

    glEnable(GL_DEPTH_TEST)
    glDepthFunc(GL_LEQUAL)                
    glDisable(GL_CULL_FACE)
    glDisable(GL_LIGHTING)
    glDisable(GL_BLEND)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)
        
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE)

    discPath = os.path.abspath(argv[0])
    fileName = argv[1]

    bcxMap = {}
    bcxMap["cloud"]     = ff7.bcx.loadBcx(ff7.retrieveFile(discPath, "FIELD", "CLOUD.BCX"))
    bcxMap["earith"]    = ff7.bcx.loadBcx(ff7.retrieveFile(discPath, "FIELD", "EARITH.BCX"))
    bcxMap["ballet"]    = ff7.bcx.loadBcx(ff7.retrieveFile(discPath, "FIELD", "BALLET.BCX"))
    bcxMap["tifa"]      = ff7.bcx.loadBcx(ff7.retrieveFile(discPath, "FIELD", "TIFA.BCX"))
    bcxMap["red"]       = ff7.bcx.loadBcx(ff7.retrieveFile(discPath, "FIELD", "RED.BCX"))
    bcxMap["cid"]       = ff7.bcx.loadBcx(ff7.retrieveFile(discPath, "FIELD", "CID.BCX"))
    bcxMap["yufi"]      = ff7.bcx.loadBcx(ff7.retrieveFile(discPath, "FIELD", "YUFI.BCX"))
    bcxMap["ketcy"]     = ff7.bcx.loadBcx(ff7.retrieveFile(discPath, "FIELD", "KETCY.BCX"))
    bcxMap["vincent"]   = ff7.bcx.loadBcx(ff7.retrieveFile(discPath, "FIELD", "VINCENT.BCX"))

    # get normals
    fieldBin = ff7.bcx.loadFieldBin(ff7.retrieveFile(discPath, "FIELD", "FIELD.BIN"))
    global normals
    normals = []
    for i in range(240):
        n = struct.unpack_from("<hhh", fieldBin, 0x3f520 + i * 8)
        normals.append(vecNormal(n))

    # load BCX or BSX file
    if os.path.splitext(os.path.basename(fileName))[1].lower() == ".bcx":
        data = [ff7.bcx.loadBcx(ff7.retrieveFile(discPath, "FIELD", fileName))]
    else:
        data = ff7.bcx.loadBsx(ff7.retrieveFile(discPath, "FIELD", fileName))

    clock = pygame.time.Clock()
    key_up = key_down = key_left = key_right = False
    rot_x = 0.0
    rot_y = 0.0
    cur_model = 0
    object = OpenGLObject(data[cur_model])
    wireframe = False
    while True:
        for event in pygame.event.get():
            if event.type == QUIT:
                exit()
            elif event.type == KEYDOWN:
                if event.key == pygame.K_UP:
                    key_up = True
                elif event.key == pygame.K_DOWN:
                    key_down = True
                elif event.key == pygame.K_LEFT:
                    key_left = True
                elif event.key == pygame.K_RIGHT:
                    key_right = True
                elif event.key == pygame.K_ESCAPE:
                    exit()
                elif event.key == pygame.K_a:
                    object.nextAnim()
                elif event.key == pygame.K_e:
                    object.nextExcludePart()
                elif event.key == pygame.K_f:
                    object.nextFocusPart()
                elif event.key == pygame.K_o:
                    object.prevFocusPoly()
                elif event.key == pygame.K_p:
                    object.nextFocusPoly()
                elif event.key == pygame.K_n:
                    object.draw_normals = not object.draw_normals
                elif event.key == pygame.K_s:
                    object.draw_skeleton = not object.draw_skeleton
                elif event.key == pygame.K_m:
                    cur_model = (cur_model + 1) % len(data)
                    del object
                    object = OpenGLObject(data[cur_model])
                elif event.key == pygame.K_w:
                    wireframe = not wireframe
                    if wireframe:
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
                    else:
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)
            elif event.type == KEYUP:
                if event.key == pygame.K_UP:
                    key_up = False
                elif event.key == pygame.K_DOWN:
                    key_down = False
                elif event.key == pygame.K_LEFT:
                    key_left = False
                elif event.key == pygame.K_RIGHT:
                    key_right = False
            impl.process_event(event)
        impl.process_inputs()
                    
        if key_up:
            rot_x += 1.0
        elif key_down:
            rot_x -= 1.0
            
        if key_left:
            rot_y += 1.0
        elif key_right:
            rot_y -= 1.0

        imgui.new_frame()

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)        

        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        glFrustum(-aspect, aspect, -(kZNear * kFOVy), (kZNear * kFOVy), kZNear, 100000.0)
        glTranslatef(0, -450, -1000)
        glRotatef(rot_y * 360.0 / 256.0, 0.0, 1.0, 0.0)
        glRotatef(rot_x * 360.0 / 256.0, 1.0, 0.0, 0.0)

        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()

        object.draw()
    
        imgui.set_next_window_position(0, 00)  # below menu bar
        imgui.set_next_window_size(250, imgui.get_io().display_size.y)

        imgui.begin("Visibility", False, imgui.WINDOW_NO_MOVE | imgui.WINDOW_NO_RESIZE)
        draw_parts_ui(object.parts_visible, object.polys_visible)
        imgui.end()

        imgui.render()
        impl.render(imgui.get_draw_data())

        clock.tick(60)
        pygame.display.flip()

if __name__ == "__main__":
    main(*sys.argv[1:])