# Based on: https://forums.qhimm.com/index.php?topic=8969.msg122920#msg122920

import sys, struct, array, math, os, zlib
import ff7
import numpy as np

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

# Unproject mouse to 3D ray
def getRayFromMouse(mx, my, width, height, modelview, projection):
    # Convert to NDC
    x = 2.0 * mx / width - 1.0
    y = 1.0 - 2.0 * my / height
    z_near, z_far = -1.0, 1.0

    # Corrected multiplication order for the inverse matrix calculation
    inv_mat = np.linalg.inv(projection @ modelview)

    # Near point in NDC
    near_ndc = np.array([x, y, z_near, 1.0])
    # Transform near point to world space
    near_world = inv_mat @ near_ndc
    # Perform perspective division
    near_world /= near_world[3]

    # Far point in NDC
    far_ndc = np.array([x, y, z_far, 1.0])
    # Transform far point to world space
    far_world = inv_mat @ far_ndc
    # Perform perspective division
    far_world /= far_world[3]

    # Calculate ray origin and direction
    origin = near_world[:3]
    direction = far_world[:3] - near_world[:3]
    direction /= np.linalg.norm(direction)
    
    return origin, direction
    
class OpenGLObject:
    def __init__(self, model):
        self.model = model
        self.time = 0
        self.animation = 0
        self.draw_normals = False
        self.draw_skeleton = True
        self.part_map = {}
        self.parts_visible = []
        self.polys_visible = []

        for idx, part in enumerate(model.parts):
            self.part_map[part.bone_index] = idx
            self.parts_visible.append(True)
            self.polys_visible.append([True] * part.poly_count)

    def getPart(self, idx):
        return self.model.parts[idx]

    def nextAnim(self):
        self.animation = (self.animation + 1) % len(self.model.animations)
        self.time = 0

    def clickTriangle(self, ray_origin, ray_dir):
        closest_t = float('inf')
        hit_triangle = None

        # Skip skeleton related functions if no animations are in the model
        if len(self.model.animations) == 0:
            for idx, bone in enumerate(self.model.skeleton):
                if bone[2]:
                    part_idx = self.part_map[idx]
                    if self.parts_visible[self.part_map[idx]]:
                        for tri in self.model.parts[part_idx].triangles:
                            if self.polys_visible[part_idx][tri[4]]:
                                t = ff7.utils.rayTriangleIntersect(ray_origin, ray_dir, np.array(tri[0]), np.array(tri[1]), np.array(tri[2]))
                                if t and t < closest_t:
                                    closest_t = t
                                    hit_triangle = tri
        else:
            animation = self.model.animations[self.animation]
            self.time = (self.time + 1) % len(animation[0])
            
            parent = [-1]
            transform = [ff7.utils.rotateXMatrix(180)]

            for idx, bone in enumerate(self.model.skeleton):
                ((translation_x, translation_y, translation_z), (rotation_x, rotation_y, rotation_z)) = animation[idx][self.time]

                while parent[-1] != bone[1]:
                    parent.pop()
                    transform.pop()

                parent.append(idx)

                M = np.eye(4)
                M = M @ ff7.utils.translateMatrix(0, 0, bone[0])
                M = M @ ff7.utils.rotateYMatrix(rotation_y)
                M = M @ ff7.utils.rotateXMatrix(rotation_x)
                M = M @ ff7.utils.rotateZMatrix(rotation_z)
                M = M @ ff7.utils.translateMatrix(translation_x, translation_y, translation_z)

                current = transform[-1].copy() @ M
                transform.append(current)

                if bone[2]:
                    part_idx = self.part_map[idx]
                    if self.parts_visible[self.part_map[idx]]:
                        for tri in self.model.parts[part_idx].triangles:
                            if self.polys_visible[part_idx][tri[4]]:
                                v0 = ff7.utils.transformPoint(current, np.array(tri[0]))
                                v1 = ff7.utils.transformPoint(current, np.array(tri[1]))
                                v2 = ff7.utils.transformPoint(current, np.array(tri[2]))

                                t = ff7.utils.rayTriangleIntersect(ray_origin, ray_dir, v0, v1, v2)
                                if t and t < closest_t:
                                    closest_t = t
                                    hit_triangle = tri

            while parent[-1] != -1:
                parent.pop()
                transform.pop()

        if not hit_triangle == None:
            self.polys_visible[hit_triangle[3]][hit_triangle[4]] = False

    def drawPart(self, part, polys_visible):
        glEnable(GL_TEXTURE_2D)
        poly_index = 0

        # quadColorTex
        glBegin(GL_QUADS)
        for quad in part.quad_color_tex:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            for i in range(4):
                pos = part.vertices[quad[i][0]]
                texcoord = part.texcoords[quad[i][1]]
                col = quad[i][2]

                glTexCoord2i(texcoord[0], texcoord[1])
                glColor3ub(col[0], col[1] * 0, col[2] * 0)
                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()

        # triColorTex
        glBegin(GL_TRIANGLES)
        for tri in part.tri_color_tex:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            for i in range(3):
                pos = part.vertices[tri[i][0]]
                texcoord = part.texcoords[tri[i][1]]
                col = tri[i][2]

                glTexCoord2i(texcoord[0], texcoord[1])
                glColor3ub(col[0], col[1] * 0, col[2] * 0)
                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()
    
        # quadMonoTex
        glBegin(GL_QUADS)
        for quad in part.quad_mono_tex:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            col = quad[4]
            glColor3ub(col[0], col[1], col[2])
            for i in range(4):
                pos = part.vertices[quad[i][0]]
                texcoord = part.texcoords[quad[i][1]]

                glTexCoord2i(texcoord[0], texcoord[1])
                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()

        # triMonoTex
        glBegin(GL_TRIANGLES)
        for tri in part.tri_mono_tex:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            col = tri[3]
            glColor3ub(col[0], col[1], col[2])
            for i in range(3):
                pos = part.vertices[tri[i][0]]
                texcoord = part.texcoords[tri[i][1]]

                glTexCoord2i(texcoord[0], texcoord[1])
                glColor3ub(col[0], col[1] * 0, col[2] * 0)
                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()
        
        glDisable(GL_TEXTURE_2D)
        
        # triMono
        glBegin(GL_TRIANGLES)
        for tri in part.tri_mono:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            col = tri[3]
            glColor3ub(col[0], col[1], col[2])
            for i in range(3):
                pos = part.vertices[tri[i][0]]

                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()

        # quadMono
        glBegin(GL_QUADS)
        for quad in part.quad_mono:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            col = quad[4]
            glColor3ub(col[0], col[1] * 0, col[2] * 0)
            for i in range(4):
                pos = part.vertices[quad[i][0]]

                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()

        # triColor
        glBegin(GL_TRIANGLES)
        for tri in part.tri_color:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            for i in range(3):
                pos = part.vertices[tri[i][0]]
                col = tri[i][1]

                glColor3ub(col[0], col[1], col[2])
                glVertex3i(pos[0], pos[1], pos[2])
            poly_index += 1
        glEnd()
        
        # quadColor
        glBegin(GL_QUADS)
        for quad in part.quad_color:
            if not polys_visible[poly_index]:
                poly_index += 1
                continue
            for i in range(4):
                pos = part.vertices[quad[i][0]]
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

            for quad in part.quad_mono_tex:
                for i in range(4):
                    pos = part.vertices[quad[i][0]]
                    nrm = normals[quad[5]]

                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))

            for tri in part.tri_mono_tex:
                for i in range(3):
                    pos = part.vertices[tri[i][0]]
                    nrm = normals[tri[4]]

                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))

            for quad in part.quad_mono:
                for i in range(4):
                    pos = part.vertices[quad[i][0]]
                    nrm = normals[quad[5]]

                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))

            for tri in part.tri_mono:
                for i in range(3):
                    pos = part.vertices[tri[i][0]]
                    nrm = normals[tri[4]]

                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))

            for quad in part.quad_color_tex:
                for i in range(4):
                    pos = part.vertices[quad[i][0]]
                    nrm = normals[quad[i][3]]

                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))

            for tri in part.tri_color_tex:
                for i in range(3):
                    pos = part.vertices[tri[i][0]]
                    nrm = normals[tri[i][3]]

                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))

            for quad in part.quad_color:
                for i in range(4):
                    pos = part.vertices[quad[i][0]]
                    nrm = normals[quad[i][2]]
                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))

            for tri in part.tri_color:
                for i in range(3):
                    pos = part.vertices[tri[i][0]]
                    nrm = normals[tri[i][2]]
                    glVertex3i(pos[0], pos[1], pos[2])
                    glVertex3i(int(pos[0]+nrm[0]*50), int(pos[1]+nrm[1]*50), int(pos[2]+nrm[2]*50))
            
            glEnd()
        
    def draw(self):
        # Skip skeleton related functions if no animations are in the model
        if len(self.model.animations) == 0:
            for idx, bone in enumerate(self.model.skeleton):
                if bone[2]:
                    part_idx = self.part_map[idx]
                    if self.parts_visible[self.part_map[idx]]:
                        self.drawPart(self.model.parts[part_idx], self.polys_visible[part_idx])
        else:
            glMatrixMode(GL_MODELVIEW)
            glRotatef(180, 1, 0, 0)

            animation = self.model.animations[self.animation]
            self.time = (self.time + 1) % len(animation[0])
            
            parent = [-1]
            for idx, bone in enumerate(self.model.skeleton):
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
                        self.drawPart(self.model.parts[part_idx], self.polys_visible[part_idx])

            while parent[-1] != -1:
                parent.pop()
                glPopMatrix()
        
def drawPartsUI(parts_visible, polys_visible):
    """
    parts_visible: list[bool]  -> visibility of each part
    polys_visible: list[list[bool]] -> per-part poly visibilities
    """

    if imgui.button("Show All"):
        for part_idx, part_visible in enumerate(parts_visible):
            polys_visible[part_idx] = [True] * len(polys_visible[part_idx])

    imgui.same_line()

    if imgui.button("Hide All"):
        for part_idx, part_visible in enumerate(parts_visible):
            polys_visible[part_idx] = [False] * len(polys_visible[part_idx])

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
    
def drawGrid(size=2000, step=100):
    glDisable(GL_LIGHTING)
    glColor3f(0.4, 0.4, 0.4)
    glBegin(GL_LINES)
    for i in range(-size, size + 1, step):
        # Lines parallel to X axis
        glVertex3f(-size, 0, i)
        glVertex3f(size, 0, i)
        # Lines parallel to Z axis
        glVertex3f(i, 0, -size)
        glVertex3f(i, 0, size)
    glEnd()

    # Optional: draw center axes in color
    glBegin(GL_LINES)
    glColor3f(1, 0, 0)  # X axis
    glVertex3f(-size, 0, 0)
    glVertex3f(size, 0, 0)
    glColor3f(0, 0, 1)  # Z axis
    glVertex3f(0, 0, -size)
    glVertex3f(0, 0, size)
    glEnd()

def drawDebugRay(origin, direction, length=1000.0, color=(1.0, 0.0, 0.0)):
    glDisable(GL_LIGHTING)  # Optional, so color shows clearly
    glColor3f(*color)
    glBegin(GL_LINES)
    glVertex3f(*origin)
    end_point = origin + direction * length
    glVertex3f(*end_point)
    glEnd()

def setupImguiStyle():
    style = imgui.get_style()
    colors = style.colors

    colors[imgui.COLOR_TEXT] = (1.00, 1.00, 1.00, 1.00)
    colors[imgui.COLOR_TEXT_DISABLED] = (0.50, 0.50, 0.50, 1.00)
    colors[imgui.COLOR_WINDOW_BACKGROUND] = (0.04, 0.04, 0.04, 0.94)
    colors[imgui.COLOR_CHILD_BACKGROUND] = (0.00, 0.00, 0.00, 0.00)
    colors[imgui.COLOR_POPUP_BACKGROUND] = (0.08, 0.08, 0.08, 1.00)
    colors[imgui.COLOR_BORDER] = (0.43, 0.43, 0.50, 0.50)
    colors[imgui.COLOR_BORDER_SHADOW] = (0.00, 0.00, 0.00, 0.00)
    colors[imgui.COLOR_FRAME_BACKGROUND] = (0.15, 0.15, 0.15, 0.54)
    colors[imgui.COLOR_FRAME_BACKGROUND_HOVERED] = (0.48, 0.26, 0.98, 0.40)
    colors[imgui.COLOR_FRAME_BACKGROUND_ACTIVE] = (0.37, 0.00, 1.00, 1.00)
    colors[imgui.COLOR_TITLE_BACKGROUND] = (0.04, 0.04, 0.04, 1.00)
    colors[imgui.COLOR_TITLE_BACKGROUND_ACTIVE] = (0.21, 0.16, 0.48, 1.00)
    colors[imgui.COLOR_TITLE_BACKGROUND_COLLAPSED] = (0.00, 0.00, 0.00, 0.51)
    colors[imgui.COLOR_MENUBAR_BACKGROUND] = (0.11, 0.11, 0.11, 1.00)
    colors[imgui.COLOR_SCROLLBAR_BACKGROUND] = (0.02, 0.02, 0.02, 0.53)
    colors[imgui.COLOR_SCROLLBAR_GRAB] = (0.31, 0.31, 0.31, 1.00)
    colors[imgui.COLOR_SCROLLBAR_GRAB_HOVERED] = (0.41, 0.41, 0.41, 1.00)
    colors[imgui.COLOR_SCROLLBAR_GRAB_ACTIVE] = (0.51, 0.51, 0.51, 1.00)
    colors[imgui.COLOR_CHECK_MARK] = (0.45, 0.26, 0.98, 1.00)
    colors[imgui.COLOR_SLIDER_GRAB] = (0.41, 0.00, 1.00, 0.40)
    colors[imgui.COLOR_SLIDER_GRAB_ACTIVE] = (0.48, 0.26, 0.98, 0.52)
    colors[imgui.COLOR_BUTTON] = (0.20, 0.20, 0.20, 0.40)
    colors[imgui.COLOR_BUTTON_HOVERED] = (1.00, 1.00, 1.00, 0.04)
    colors[imgui.COLOR_BUTTON_ACTIVE] = (0.34, 0.06, 0.98, 1.00)
    colors[imgui.COLOR_HEADER] = (1.00, 1.00, 1.00, 0.04)
    colors[imgui.COLOR_HEADER_HOVERED] = (0.15, 0.15, 0.15, 0.80)
    colors[imgui.COLOR_HEADER_ACTIVE] = (1.00, 1.00, 1.00, 0.04)
    colors[imgui.COLOR_SEPARATOR] = (0.43, 0.43, 0.50, 0.50)
    colors[imgui.COLOR_SEPARATOR_HOVERED] = (0.10, 0.40, 0.75, 0.78)
    colors[imgui.COLOR_SEPARATOR_ACTIVE] = (0.10, 0.40, 0.75, 1.00)
    colors[imgui.COLOR_RESIZE_GRIP] = (1.00, 1.00, 1.00, 0.04)
    colors[imgui.COLOR_RESIZE_GRIP_HOVERED] = (1.00, 1.00, 1.00, 0.13)
    colors[imgui.COLOR_RESIZE_GRIP_ACTIVE] = (0.38, 0.38, 0.38, 1.00)
    colors[imgui.COLOR_TAB_HOVERED] = (0.40, 0.26, 0.98, 0.50)
    colors[imgui.COLOR_TAB] = (0.18, 0.20, 0.58, 0.73)
    colors[imgui.COLOR_TAB_ACTIVE] = (0.29, 0.20, 0.68, 1.00)

    if hasattr(imgui, 'COLOR_TAB_SELECTED'): 
        colors[imgui.COLOR_TAB_SELECTED] = (0.29, 0.20, 0.68, 1.00)
        colors[imgui.COLOR_TAB_SELECTED_OVERLINE] = (0.26, 0.59, 0.98, 1.00)
    if hasattr(imgui, 'COLOR_TAB_DIMMED'):
        colors[imgui.COLOR_TAB_DIMMED] = (0.07, 0.10, 0.15, 0.97)
        colors[imgui.COLOR_TAB_DIMMED_SELECTED] = (0.14, 0.26, 0.42, 1.00)
        colors[imgui.COLOR_TAB_DIMMED_SELECTED_OVERLINE] = (0.50, 0.50, 0.50, 0.00)

    colors[imgui.COLOR_PLOT_LINES] = (0.61, 0.61, 0.61, 1.00)
    colors[imgui.COLOR_PLOT_LINES_HOVERED] = (1.00, 0.43, 0.35, 1.00)
    colors[imgui.COLOR_PLOT_HISTOGRAM] = (0.90, 0.70, 0.00, 1.00)
    colors[imgui.COLOR_PLOT_HISTOGRAM_HOVERED] = (1.00, 0.60, 0.00, 1.00)
    colors[imgui.COLOR_TABLE_HEADER_BACKGROUND] = (0.19, 0.19, 0.20, 1.00)
    colors[imgui.COLOR_TABLE_BORDER_STRONG] = (0.31, 0.31, 0.35, 1.00)
    colors[imgui.COLOR_TABLE_BORDER_LIGHT] = (0.23, 0.23, 0.25, 1.00)
    colors[imgui.COLOR_TABLE_ROW_BACKGROUND] = (0.00, 0.00, 0.00, 0.00)
    colors[imgui.COLOR_TABLE_ROW_BACKGROUND_ALT] = (1.00, 1.00, 1.00, 0.06)

    if hasattr(imgui, 'COLOR_TEXT_LINK'):
        colors[imgui.COLOR_TEXT_LINK] = (0.26, 0.59, 0.98, 1.00)

    colors[imgui.COLOR_TEXT_SELECTED_BACKGROUND] = (1.00, 1.00, 1.00, 0.04)
    colors[imgui.COLOR_DRAG_DROP_TARGET] = (1.00, 1.00, 0.00, 0.90)

    if hasattr(imgui, 'COLOR_NAV_CURSOR'):
        colors[imgui.COLOR_NAV_CURSOR] = (0.26, 0.59, 0.98, 1.00)

    colors[imgui.COLOR_NAV_WINDOWING_HIGHLIGHT] = (1.00, 1.00, 1.00, 0.70)
    colors[imgui.COLOR_NAV_WINDOWING_DIM_BACKGROUND] = (0.80, 0.80, 0.80, 0.20)
    colors[imgui.COLOR_MODAL_WINDOW_DIM_BACKGROUND] = (0.80, 0.80, 0.80, 0.35)

def main(*argv):
    pygame.init()
    
    width = 1280
    height = 960

    screen = pygame.display.set_mode((width, height), HWSURFACE|DOUBLEBUF|OPENGL)
    pygame.display.set_caption("FF7 Model Viewer")

    # setup imgui
    imgui.create_context()
    impl = PygameRenderer()

    io = imgui.get_io()
    io.display_size = (width, height)
    setupImguiStyle()

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
    bcxMap["cloud"]     = ff7.models.loadModelFromBCX(ff7.retrieveFile(discPath, "FIELD", "CLOUD.BCX"))
    bcxMap["earith"]    = ff7.models.loadModelFromBCX(ff7.retrieveFile(discPath, "FIELD", "EARITH.BCX"))
    bcxMap["ballet"]    = ff7.models.loadModelFromBCX(ff7.retrieveFile(discPath, "FIELD", "BALLET.BCX"))
    bcxMap["tifa"]      = ff7.models.loadModelFromBCX(ff7.retrieveFile(discPath, "FIELD", "TIFA.BCX"))
    bcxMap["red"]       = ff7.models.loadModelFromBCX(ff7.retrieveFile(discPath, "FIELD", "RED.BCX"))
    bcxMap["cid"]       = ff7.models.loadModelFromBCX(ff7.retrieveFile(discPath, "FIELD", "CID.BCX"))
    bcxMap["yufi"]      = ff7.models.loadModelFromBCX(ff7.retrieveFile(discPath, "FIELD", "YUFI.BCX"))
    bcxMap["ketcy"]     = ff7.models.loadModelFromBCX(ff7.retrieveFile(discPath, "FIELD", "KETCY.BCX"))
    bcxMap["vincent"]   = ff7.models.loadModelFromBCX(ff7.retrieveFile(discPath, "FIELD", "VINCENT.BCX"))

    # Normals for Field Models
    fieldBin = ff7.models.loadFieldBin(ff7.retrieveFile(discPath, "FIELD", "FIELD.BIN"))
    global normals
    normals = []
    for i in range(240):
        n = struct.unpack_from("<hhh", fieldBin, 0x3f520 + i * 8)
        normals.append(ff7.utils.vecNormal(n))

    # Determine what type of file we're dealing with
    fileExt = os.path.splitext(os.path.basename(fileName))[1].lower()
    if fileExt == ".bcx":
        data = [ff7.models.loadModelFromBCX(ff7.retrieveFile(discPath, "FIELD", fileName))]
    elif fileExt == ".bsx":
        data = ff7.models.loadModelsFromBSX(ff7.retrieveFile(discPath, "FIELD", fileName))
    elif fileExt == ".lzs":
        data = [ff7.models.loadModelFromLZS(ff7.retrieveFile(discPath, "", fileName))]
    else:
        print("Unsupported file extension: " + fileExt)
        return

    clock = pygame.time.Clock()
    key_up = key_down = key_left = key_right = False
    rot_x = 0.0
    rot_y = 0.0
    cur_model = 0
    object = OpenGLObject(data[cur_model])
    wireframe = False

    # Camera State
    cam_target = [0.0, 0.0, 0.0]
    cam_distance = 1000.0
    cam_yaw = 0.0
    cam_pitch = 0.8
    right_dragging = False
    middle_dragging = False
    last_mouse = (0, 0)
    projection = None
    modelview = None

    # debugging
    last_ray_origin = None
    last_ray_dir = None

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

            elif event.type == MOUSEBUTTONDOWN and not io.want_capture_mouse:
                mx, my = event.pos
                if event.button == 1:       # Left click
                    ray_origin, ray_dir = getRayFromMouse(mx, my, width, height, modelview, projection)
                    object.clickTriangle(ray_origin, ray_dir)
                    last_ray_origin = ray_origin
                    last_ray_dir = ray_dir
                elif event.button == 2:     # Middle click
                    middle_dragging = True
                    last_mouse = event.pos
                elif event.button == 3:     # Right click
                    right_dragging = True
                    last_mouse = event.pos

            elif event.type == MOUSEBUTTONUP:
                if event.button == 2:
                    middle_dragging = False
                elif event.button == 3:
                    right_dragging = False

            elif event.type == MOUSEMOTION:
                mx, my = event.pos
                dx = mx - last_mouse[0]
                dy = my - last_mouse[1]
                last_mouse = (mx, my)

                # Orbit (Right mouse)
                if right_dragging:
                    sensitivity = 0.005
                    cam_yaw -= dx * sensitivity
                    cam_pitch += dy * sensitivity
                    cam_pitch = max(-math.pi/2 + 0.1, min(math.pi/2 - 0.1, cam_pitch))

                # Pan (Middle mouse)
                elif middle_dragging:
                    # Compute camera basis
                    x = cam_target[0] + cam_distance * math.cos(cam_pitch) * math.sin(cam_yaw)
                    y = cam_target[1] + cam_distance * math.sin(cam_pitch)
                    z = cam_target[2] + cam_distance * math.cos(cam_pitch) * math.cos(cam_yaw)
                    eye = np.array([x, y, z])
                    forward = cam_target - eye
                    forward /= np.linalg.norm(forward)
                    right = np.cross(forward, np.array([0, 1, 0]))
                    right /= np.linalg.norm(right)
                    up = np.cross(right, forward)

                    pan_speed = 1.0
                    cam_target += -right * dx * pan_speed
                    cam_target += up * dy * pan_speed

            elif event.type == MOUSEWHEEL and not io.want_capture_mouse:
                cam_distance += -20 if event.y > 0 else 20
                cam_distance = max(100.0, min(2000.0, cam_distance))

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
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()

        eye = [0, 0, 0]
        eye[0] = cam_target[0] + cam_distance * math.cos(cam_pitch) * math.sin(cam_yaw)
        eye[1] = cam_target[1] + cam_distance * math.sin(cam_pitch)
        eye[2] = cam_target[2] + cam_distance * math.cos(cam_pitch) * math.cos(cam_yaw)
        gluLookAt(eye[0], eye[1], eye[2], cam_target[0], cam_target[1], cam_target[2], 0, 1, 0)

        modelview = glGetDoublev(GL_MODELVIEW_MATRIX).T
        projection = glGetDoublev(GL_PROJECTION_MATRIX).T

        #print(modelview)

        # Draw Grid
        drawGrid()

        # Draw Model
        glPushMatrix()
        glRotatef(rot_y * 360.0 / 256.0, 0.0, 1.0, 0.0)
        glRotatef(rot_x * 360.0 / 256.0, 1.0, 0.0, 0.0)

        object.draw()
        glPopMatrix()

        #if last_ray_origin is not None:
        #    drawDebugRay(last_ray_origin, last_ray_dir, length=1000.0, color=(1, 0, 0))

        # Draw UI
        imgui.set_next_window_position(0, 00)  # below menu bar
        imgui.set_next_window_size(250, imgui.get_io().display_size.y)

        imgui.begin("Visibility", False, imgui.WINDOW_NO_MOVE | imgui.WINDOW_NO_RESIZE)
        drawPartsUI(object.parts_visible, object.polys_visible)
        imgui.end()

        imgui.render()
        impl.render(imgui.get_draw_data())

        # Finish rendering
        clock.tick(60)
        pygame.display.flip()

if __name__ == "__main__":
    main(*sys.argv[1:])