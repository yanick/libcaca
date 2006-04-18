/*
 *  libcaca       Colour ASCII-Art library
 *  Copyright (c) 2002-2006 Sam Hocevar <sam@zoy.org>
 *                All Rights Reserved
 *
 *  $Id$
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the Do What The Fuck You Want To
 *  Public License, Version 2, as published by Sam Hocevar. See
 *  http://sam.zoy.org/wtfpl/COPYING for more details.
 */

/*
 *  This file contains the libcaca OpenGL input and output driver
 */

#include "config.h"

#if defined(USE_GL)

#ifdef HAVE_OPENGL_GL_H
#   include <OpenGL/gl.h>
#   include <GLUT/glut.h>
#else
#   include <GL/gl.h>
#   include <GL/glut.h>
#   include <GL/freeglut_ext.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "caca.h"
#include "caca_internals.h"
#include "cucul.h"
#include "cucul_internals.h"

/*
 * Global variables
 */

static caca_t *gl_kk; /* FIXME: we ought to get rid of this */

/*
 * Local functions
 */
static void gl_handle_keyboard(unsigned char, int, int);
static void gl_handle_special_key(int, int, int);
static void gl_handle_reshape(int, int);
static void gl_handle_mouse(int, int, int, int);
static void gl_handle_mouse_motion(int, int);
#ifdef HAVE_GLUTCLOSEFUNC
static void gl_handle_close(void);
#endif
static void _display(void);

struct driver_private
{
    int window;
    unsigned int width, height;
    unsigned int new_width, new_height;
    float font_width, font_height;
    float incx, incy;
    int id[128 - 32];
    unsigned char close;
    unsigned char bit;
    unsigned char mouse_changed, mouse_clicked;
    unsigned int mouse_x, mouse_y;
    unsigned int mouse_button, mouse_state;

    unsigned char key;
    int special_key;

    float sw, sh;
};

static int gl_init_graphics(caca_t *kk)
{
    char *empty_texture;
    char const *geometry;
    char *argv[2] = { "", NULL };
    unsigned int width = 0, height = 0;
    int argc = 1;
    int i;

    kk->drv.p = malloc(sizeof(struct driver_private));

    gl_kk = kk;

#if defined(HAVE_GETENV)
    geometry = getenv("CACA_GEOMETRY");
    if(geometry && *geometry)
        sscanf(geometry, "%ux%u", &width, &height);
#endif

    if(width && height)
        _cucul_set_size(kk->c, width, height);

    kk->drv.p->font_width = 9;
    kk->drv.p->font_height = 15;

    kk->drv.p->width = kk->c->width * kk->drv.p->font_width;
    kk->drv.p->height = kk->c->height * kk->drv.p->font_height;

#ifdef HAVE_GLUTCLOSEFUNC
    kk->drv.p->close = 0;
#endif
    kk->drv.p->bit = 0;

    kk->drv.p->mouse_changed = kk->drv.p->mouse_clicked = 0;
    kk->drv.p->mouse_button = kk->drv.p->mouse_state = 0;

    kk->drv.p->key = 0;
    kk->drv.p->special_key = 0;

    kk->drv.p->sw = 9.0f / 16.0f;
    kk->drv.p->sh = 15.0f / 16.0f;

    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(kk->drv.p->width, kk->drv.p->height);
    kk->drv.p->window = glutCreateWindow("caca for GL");

    gluOrtho2D(0, kk->drv.p->width, kk->drv.p->height, 0);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glutKeyboardFunc(gl_handle_keyboard);
    glutSpecialFunc(gl_handle_special_key);
    glutReshapeFunc(gl_handle_reshape);
    glutDisplayFunc(_display);

#ifdef HAVE_GLUTCLOSEFUNC
    glutCloseFunc(gl_handle_close);
#endif

    glutMouseFunc(gl_handle_mouse);
    glutMotionFunc(gl_handle_mouse_motion);
    glutPassiveMotionFunc(gl_handle_mouse_motion);

    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, kk->drv.p->width, kk->drv.p->height, 0);

    glMatrixMode(GL_MODELVIEW);

    glClear(GL_COLOR_BUFFER_BIT);

    empty_texture = malloc(16 * 16 * 4);
    if(empty_texture == NULL)
        return -1;

    memset(empty_texture, 0xff, 16 * 16 * 4);
    glEnable(GL_TEXTURE_2D);

    for(i = 32; i < 128; i++)
    {
        glGenTextures(1, (GLuint*)&kk->drv.p->id[i - 32]);
        glBindTexture(GL_TEXTURE_2D, kk->drv.p->id[i - 32]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                     16, 16, 0, GL_RGB, GL_UNSIGNED_BYTE, empty_texture);
    }

    for(i = 32; i < 128; i++)
    {
        glDisable(GL_TEXTURE_2D);
        glClear(GL_COLOR_BUFFER_BIT);

        glColor3f(1, 1, 1);
        glRasterPos2f(0, 15);
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, i);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, kk->drv.p->id[i - 32]);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                         0, kk->drv.p->height - 16, 16, 16, 0);

#ifdef HAVE_GLUTCHECKLOOP
        glutCheckLoop();
#else
        glutMainLoopEvent();
#endif
        glutPostRedisplay();
    }

    return 0;
}

static int gl_end_graphics(caca_t *kk)
{
    glutDestroyWindow(kk->drv.p->window);
    free(kk->drv.p);
    return 0;
}

static int gl_set_window_title(caca_t *kk, char const *title)
{
    glutSetWindowTitle(title);
    return 0;
}

static unsigned int gl_get_window_width(caca_t *kk)
{
    return kk->drv.p->width;
}

static unsigned int gl_get_window_height(caca_t *kk)
{
    return kk->drv.p->height;
}

static void gl_display(caca_t *kk)
{
    unsigned int x, y, line;

    glClear(GL_COLOR_BUFFER_BIT);

    line = 0;
    for(y = 0; y < kk->drv.p->height; y += kk->drv.p->font_height)
    {
        uint32_t *attr = kk->c->attr + line * kk->c->width;

        for(x = 0; x < kk->drv.p->width; x += kk->drv.p->font_width)
        {
            uint16_t bg = _cucul_argb32_to_rgb12bg(*attr++);
            glDisable(GL_TEXTURE_2D);
            glColor3b(((bg & 0xf00) >> 8) * 8,
                      ((bg & 0x0f0) >> 4) * 8,
                      (bg & 0x00f) * 8);
            glBegin(GL_QUADS);
                glVertex2f(x, y);
                glVertex2f(x + kk->drv.p->font_width, y);
                glVertex2f(x + kk->drv.p->font_width,
                           y + kk->drv.p->font_height);
                glVertex2f(x, y + kk->drv.p->font_height);
            glEnd();
        }

        line++;
    }

    /* 2nd pass, avoids changing render state too much */
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_ONE, GL_ONE);

    line = 0;
    for(y = 0; y < kk->drv.p->height; y += kk->drv.p->font_height)
    {
        uint32_t *attr = kk->c->attr + line * kk->c->width;
        uint32_t *chars = kk->c->chars + line * kk->c->width;

        for(x = 0; x < kk->drv.p->width; x += kk->drv.p->font_width)
        {
            uint32_t c = *chars++;

            if(c > 0x00000020 && c < 0x00000080)
            {
                uint16_t fg = _cucul_argb32_to_rgb12fg(*attr);
                glBindTexture(GL_TEXTURE_2D, kk->drv.p->id[c - 32]);
                glColor3b(((fg & 0xf00) >> 8) * 8,
                          ((fg & 0x0f0) >> 4) * 8,
                          (fg & 0x00f) * 8);
                glBegin(GL_QUADS);
                    glTexCoord2f(0, kk->drv.p->sh);
                    glVertex2f(x, y);
                    glTexCoord2f(kk->drv.p->sw, kk->drv.p->sh);
                    glVertex2f(x + kk->drv.p->font_width, y);
                    glTexCoord2f(kk->drv.p->sw, 0);
                    glVertex2f(x + kk->drv.p->font_width,
                               y + kk->drv.p->font_height);
                    glTexCoord2f(0, 0);
                    glVertex2f(x, y + kk->drv.p->font_height);
                glEnd();
            }

            attr++;
        }
        line++;
    }
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

#ifdef HAVE_GLUTCHECKLOOP
    glutCheckLoop();
#else
    glutMainLoopEvent();
#endif
    glutSwapBuffers();
    glutPostRedisplay();
}

static void gl_handle_resize(caca_t *kk)
{
    kk->drv.p->width = kk->drv.p->new_width;
    kk->drv.p->height = kk->drv.p->new_height;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glViewport(0, 0, kk->drv.p->width, kk->drv.p->height);
    gluOrtho2D(0, kk->drv.p->width, kk->drv.p->height, 0);
    glMatrixMode(GL_MODELVIEW);
}

static int gl_get_event(caca_t *kk, caca_event_t *ev)
{
#ifdef HAVE_GLUTCHECKLOOP
    glutCheckLoop();
#else
    glutMainLoopEvent();
#endif

#ifdef HAVE_GLUTCLOSEFUNC
    if(kk->drv.p->close)
    {
        kk->drv.p->close = 0;
        ev->type = CACA_EVENT_QUIT;
        return 1;
    }
#endif

    if(kk->resize.resized)
    {
        ev->type = CACA_EVENT_RESIZE;
        ev->data.resize.w = kk->c->width;
        ev->data.resize.h = kk->c->height;
        return 1;
    }

    if(kk->drv.p->mouse_changed)
    {
        ev->type = CACA_EVENT_MOUSE_MOTION;
        ev->data.mouse.x = kk->mouse.x;
        ev->data.mouse.y = kk->mouse.y;
        kk->drv.p->mouse_changed = 0;

        if(kk->drv.p->mouse_clicked)
        {
            _push_event(kk, ev);
            ev->type = CACA_EVENT_MOUSE_PRESS;
            ev->data.mouse.button = kk->drv.p->mouse_button;
            kk->drv.p->mouse_clicked = 0;
        }

        return 1;
    }

    if(kk->drv.p->key != 0)
    {
        ev->type = CACA_EVENT_KEY_PRESS;
        ev->data.key.ch = kk->drv.p->key;
        ev->data.key.ucs4 = (uint32_t)kk->drv.p->key;
        ev->data.key.utf8[0] = kk->drv.p->key;
        ev->data.key.utf8[1] = '\0';
        kk->drv.p->key = 0;
        return 1;
    }

    if(kk->drv.p->special_key != 0)
    {
        switch(kk->drv.p->special_key)
        {
            case GLUT_KEY_F1 : ev->data.key.ch = CACA_KEY_F1; break;
            case GLUT_KEY_F2 : ev->data.key.ch = CACA_KEY_F2; break;
            case GLUT_KEY_F3 : ev->data.key.ch = CACA_KEY_F3; break;
            case GLUT_KEY_F4 : ev->data.key.ch = CACA_KEY_F4; break;
            case GLUT_KEY_F5 : ev->data.key.ch = CACA_KEY_F5; break;
            case GLUT_KEY_F6 : ev->data.key.ch = CACA_KEY_F6; break;
            case GLUT_KEY_F7 : ev->data.key.ch = CACA_KEY_F7; break;
            case GLUT_KEY_F8 : ev->data.key.ch = CACA_KEY_F8; break;
            case GLUT_KEY_F9 : ev->data.key.ch = CACA_KEY_F9; break;
            case GLUT_KEY_F10: ev->data.key.ch = CACA_KEY_F10; break;
            case GLUT_KEY_F11: ev->data.key.ch = CACA_KEY_F11; break;
            case GLUT_KEY_F12: ev->data.key.ch = CACA_KEY_F12; break;
            case GLUT_KEY_LEFT : ev->data.key.ch = CACA_KEY_LEFT; break;
            case GLUT_KEY_RIGHT: ev->data.key.ch = CACA_KEY_RIGHT; break;
            case GLUT_KEY_UP   : ev->data.key.ch = CACA_KEY_UP; break;
            case GLUT_KEY_DOWN : ev->data.key.ch = CACA_KEY_DOWN; break;
            default: ev->type = CACA_EVENT_NONE; return 0;
        }

        ev->type = CACA_EVENT_KEY_PRESS;
        ev->data.key.ucs4 = 0;
        ev->data.key.utf8[0] = '\0';

        kk->drv.p->special_key = 0;
        return 1;
    }

    ev->type = CACA_EVENT_NONE;
    return 0;
}


static void gl_set_mouse(caca_t *kk, int flag)
{
    if(flag)
        glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);
    else
        glutSetCursor(GLUT_CURSOR_NONE);
}

/*
 * XXX: following functions are local
 */

static void gl_handle_keyboard(unsigned char key, int x, int y)
{
    caca_t *kk = gl_kk;

    kk->drv.p->key = key;
}

static void gl_handle_special_key(int key, int x, int y)
{
    caca_t *kk = gl_kk;

    kk->drv.p->special_key = key;
}

static void gl_handle_reshape(int w, int h)
{
    caca_t *kk = gl_kk;

    if(kk->drv.p->bit) /* Do not handle reshaping at the first time */
    {
        kk->drv.p->new_width = w;
        kk->drv.p->new_height = h;

        kk->resize.w = w / kk->drv.p->font_width;
        kk->resize.h = (h / kk->drv.p->font_height) + 1;

        kk->resize.resized = 1;
    }
    else
        kk->drv.p->bit = 1;
}

static void gl_handle_mouse(int button, int state, int x, int y)
{
    caca_t *kk = gl_kk;

    kk->drv.p->mouse_clicked = 1;
    kk->drv.p->mouse_button = button;
    kk->drv.p->mouse_state = state;
    kk->drv.p->mouse_x = x / kk->drv.p->font_width;
    kk->drv.p->mouse_y = y / kk->drv.p->font_height;
    kk->mouse.x = kk->drv.p->mouse_x;
    kk->mouse.y = kk->drv.p->mouse_y;
    kk->drv.p->mouse_changed = 1;
}

static void gl_handle_mouse_motion(int x, int y)
{
    caca_t *kk = gl_kk;
    kk->drv.p->mouse_x = x / kk->drv.p->font_width;
    kk->drv.p->mouse_y = y / kk->drv.p->font_height;
    kk->mouse.x = kk->drv.p->mouse_x;
    kk->mouse.y = kk->drv.p->mouse_y;
    kk->drv.p->mouse_changed = 1;
}

#ifdef HAVE_GLUTCLOSEFUNC
static void gl_handle_close(void)
{
    caca_t *kk = gl_kk;
    kk->drv.p->close = 1;
}
#endif

static void _display(void)
{
    caca_t *kk = gl_kk;
    gl_display(kk);
}


/*
 * Driver initialisation
 */

int gl_install(caca_t *kk)
{
#if defined(HAVE_GETENV) && defined(GLUT_XLIB_IMPLEMENTATION)
    if(!getenv("DISPLAY") || !*(getenv("DISPLAY")))
        return -1;
#endif

    kk->drv.driver = CACA_DRIVER_GL;

    kk->drv.init_graphics = gl_init_graphics;
    kk->drv.end_graphics = gl_end_graphics;
    kk->drv.set_window_title = gl_set_window_title;
    kk->drv.get_window_width = gl_get_window_width;
    kk->drv.get_window_height = gl_get_window_height;
    kk->drv.display = gl_display;
    kk->drv.handle_resize = gl_handle_resize;
    kk->drv.get_event = gl_get_event;
    kk->drv.set_mouse = gl_set_mouse;

    return 0;
}

#endif /* USE_GL */

