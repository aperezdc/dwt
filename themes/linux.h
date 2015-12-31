/*
 * themes/linux.h
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef THEMES_LINUX_H
#define THEMES_LINUX_H

/*
 * Set of colors as used by GNOME-Terminal for the “Linux” color scheme:
 * http://git.gnome.org/browse/gnome-terminal/tree/src/terminal-profile.c
 */
{
    .name = "linux",
    .fg = { 0.8, 0.8, 0.8, 1 },
    .bg = {   0,   0,   0, 1 },
    .colors = {
        { 0,        0,        0,        1 },
        { 0.666667, 0,        0,        1 },
        { 0,        0.666667, 0,        1 },
        { 0.666667, 0.333333, 0,        1 },
        { 0,        0,        0.666667, 1 },
        { 0.666667, 0,        0.666667, 1 },
        { 0,        0.666667, 0.666667, 1 },
        { 0.666667, 0.666667, 0.666667, 1 },
        { 0.333333, 0.333333, 0.333333, 1 },
        { 1,        0.333333, 0.333333, 1 },
        { 0.333333, 1,        0.333333, 1 },
        { 1,        1,        0.333333, 1 },
        { 0.333333, 0.333333, 1,        1 },
        { 1,        0.333333, 1,        1 },
        { 0.333333, 1,        1,        1 },
        { 1,        1,        1,        1 },
    }
},


#endif /* !THEMES_LINUX_H */
