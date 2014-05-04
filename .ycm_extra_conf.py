#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#
# Copyright © 2014 Adrian Perez <aperez@igalia.com>
#
# Distributed under terms of the MIT license.

from subprocess import check_output
from shlex import split as sh_split

def FlagsForFile(path, **kwarg):
    flags = sh_split(check_output(["make", "print-flags"]))
    flags.extend(("-Qunused-arguments",
                  "-DDWT_USE_HEADER_BAR=TRUE",
                  "-DDWT_USE_POPOVER=TRUE",
                  "-DDWT_USE_OVERLAY=TRUE"))
    return { 'flags': flags, 'do_cache': True }

