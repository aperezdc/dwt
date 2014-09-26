=====
 dwt
=====

------------------------------
Simple terminal emulator for X
------------------------------

:Author: Adrian Perez <aperez@igalia.com>
:Manual section: 1


SYNOPSIS
========

``dwt -e``


DESCRIPTION
===========

The ``dwt`` program is a terminal emulator for the the X Window System. It
provides DEC VT102/VT220 emulation.


USAGE
=====

Command line options:

-e COMMAND, --command=COMMAND
              Run the given *COMMAND* in the terminal window, instead of the
              user shell. If the command has multple arguments, make sure to
              quote it properly: the whole command line has to be passed as
              a single command line option.

-w PATH, --workdir=PATH
              Change the active directory to the given *PATH* before running
              the shell (or any other command) inside the terminal window.

-t TITLE, --title=TITLE
              Defines the initial *TITLE* of the terminal window (which, by
              default, is "dwt"). Even when a title is specified,
              applications may override it using `xterm(1)`-compatible
              escape sequences.

-f FONT, --font=FONT
              Sets the font used by the terminal emulator. The font name is
              interpreted using FontConfig. The default is "terminus 11".

-s BYTES, --scrollback=BYTES
              Sets the size of the scrollback buffer. The default is 1024
              bytes (1 kB).

-b, --bold    Allow usage of bold font variants.

-N, --no-header-bar
              Disable using a custom header bar widget, and let the window
              manager use its own decorations instead. This may be useful
              to make ``dwt`` blend better with your desktop environment.

-H, --title-on-maximize
              Keep a title bar in maximized windows. The default is to
              disable the title bar.

-h, --help    Show a summary of available options.


EXAMPLES
========

Run Vim in a new terminal window, to edit a file with spaces in its file
name::

  dwt --command='vim "~/foo bar.txt"

Launch a new terminal window, specifying an alternative title for it::

  dwt --title='Dev Ops'


ENVIRONMENT
===========

If the ``SHELL`` environment variable is defined, it is assumed that it
contains the path to the shell that will be run as default command in the
terminal window. If not defined, `getpwuid(3)` will be used to determine the
user shell.

If present, the value of the ``DWT_APPLICATION_ID`` will be used as the
unique identifier for the application. If a ``dwt`` process with the given
identifier is already running, it will be instructed to create a new
terminal window. This means that all the terminal windows created by
launching ``dwt`` with the same identifier live in the same process. To
disable this behaviour, use ``none`` as identifier.


SEE ALSO
========

`xterm(1)`

