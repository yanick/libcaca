/*
 *   ttyvaders     Textmode shoot'em up
 *   Copyright (c) 2002 Sam Hocevar <sam@zoy.org>
 *                 All Rights Reserved
 *
 *   $Id$
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <stdlib.h>

#include "common.h"

/* Init tunnel */
tunnel * create_tunnel(game *g, int w, int h)
{
    int i;
    tunnel *t = malloc(sizeof(tunnel));
    if(t == NULL)
        exit(1);

    t->left = malloc(h*sizeof(int));
    if(t->left == NULL)
        exit(1);
    t->right = malloc(h*sizeof(int));
    if(t->right == NULL)
        exit(1);
    t->w = w;
    t->h = h;

    if(t->w >= g->w)
    {
        for(i = 0; i < g->h; i++)
        {
            t->left[i] = -10;
            t->right[i] = g->w + 10;
        }
    }
    else
    {
        t->left[0] = (g->w - w) / 2;
        t->right[0] = (g->w + w) / 2;
        /* Yeah, sub-efficient, but less code to do :-) */
        for(i = 0; i < g->h; i++)
        {
            update_tunnel(g, t);
        }
    }

    return t;
}

void free_tunnel(tunnel *t)
{
    free(t->left);
    free(t->right);
    free(t);
}

void draw_tunnel(game *g, tunnel *t)
{
    int i, j;
    char c;

    caca_set_color(EE_GREEN);

    /* Left border */
    for(i = 0; i < g->h ; i++)
    {
        if(t->left[i] <= -10)
            continue;

        if(i + 1 == g->h || t->left[i] > t->left[i+1])
            c = (i == 0 || t->left[i] > t->left[i-1]) ? '>' : '/';
        else
            c = (i == 0 || t->left[i] > t->left[i-1]) ? '\\' : '<';

        caca_putchar(t->left[i] + 1, i, c);

        if(i + 1 < g->h)
            for(j = 1; j < t->left[i+1] - t->left[i]; j++)
                caca_putchar(t->left[i] + j + 1, i, '_');
    }

    /* Right border */
    for(i = 0; i < g->h ; i++)
    {
        if(t->right[i] >= g->w + 10)
            continue;

        if(i + 1 == g->h || t->right[i] > t->right[i+1])
            c = (i == 0 || t->right[i] > t->right[i-1]) ? '>' : '/';
        else
            c = (i == 0 || t->right[i] > t->right[i-1]) ? '\\' : '<';

        if(i + 1 < g->h)
            for(j = 1; j < t->right[i] - t->right[i+1]; j++)
                caca_putchar(t->right[i+1] + j - 1, i, '_');

        caca_putchar(t->right[i] - 1, i, c);
    }

    caca_set_color(EE_RED);

    /* Left concrete */
    for(i = 0; i < g->h ; i++)
        for(j = 0 ; j <= t->left[i]; j++)
            caca_putchar(j, i, '#');

    /* Right concrete */
    for(i = 0; i < g->h ; i++)
        for(j = t->right[i] ; j < g->w ; j++)
            caca_putchar(j, i, '#');
}

void update_tunnel(game *g, tunnel *t)
{
    static int const delta[] = { -3, -2, -1, 1, 2, 3 };
    int i,j,k;

    /* Slide tunnel one block vertically */
    for(i = t->h - 1; i--;)
    {
        t->left[i+1] = t->left[i];
        t->right[i+1] = t->right[i];
    }

    /* Generate new values */
    i = delta[caca_rand(0,5)];
    j = delta[caca_rand(0,5)];

    /* Check in which direction we need to alter tunnel */
    if(t->right[1] - t->left[1] < t->w)
    {
        /* Not wide enough, make sure i <= j */
        if(i > j)
        {
            k = j; j = i; i = k;
        }
    }
    else if(t->right[1] - t->left[1] - 2 > t->w)
    {
        /* Too wide, make sure i >= j */
        if(i < j)
        {
            k = j; j = i; i = k;
        }
    }
    else
    {
        /* No need to mess with i and j: width is OK */
    }

    /* If width doesn't exceed game size, update coords */
    if(t->w <= g->w || t->right[1] - t->left[1] < t->w)
    {
        t->left[0] = t->left[1] + i;
        t->right[0] = t->right[1] + j;
    }
    else
    {
        t->left[0] = -10;
        t->right[0] = g->w + 10;
    }

    if(t->w > g->w)
    {
        if(t->left[0] < 0 && t->right[0] < g->w - 2)
        {
             t->left[0] = t->left[1] + 1;
        }

        if(t->left[0] > 1 && t->right[0] > g->w - 1)
        {
             t->right[0] = t->right[1] - 1;
        }
    }
    else
    {
        if(t->left[0] < 0)
        {
            t->left[0] = t->left[1] + 1;
        }

        if(t->right[0] > g->w - 1)
        {
            t->right[0] = t->right[1] - 1;
        }
    }
}

