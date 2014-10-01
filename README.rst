==========================
DWT - Dumb Window Terminal
==========================

DWT is a simple, sleek emulator based on the VTE_ widget. Its main features
are being nice-looking (according to my personal taste), not being (very)
configurable, and being a good companion for tiling window managers like
DWM_ (or my own XDWM_ fork).

Screenshot
==========

.. image:: /screenshots/popover.png

(Non-)Features
==============

* Mostly-fixed configuration:

  - Grey on black color scheme with using Linux console color set.

  - Converts terminal bell beeps in “urgent” X window flags. No visible
    or audible terminal bell, besides from that.

  - Non-blinking cursor.

  - Scrollback buffer, but no scrollbar displayed, to save screen real
    estate. Use ``Shift-PageUp`` and ``Shift-PageDown`` to scroll.

  - Keybindings to change font size: Use ``Super-+`` and ``Super--``
    to change sizes, ``Super-0`` to reset the font.

  - Mouse cursor auto-hide.

* XTerm-style configurable window title.

* Clickable URLs. Because on the Internet era being able to quickly open
  a browser is a must-have feature.

* Single process, multiple terminal windows: the first time ``dwt`` is
  invoked, it will start a new process; in subsequent times, it will
  just create new windows in the existing process.

* Uses current GTK+ widgets and code conventions. Apart from the popover
  and header bar widgets, modern facilities like ``GAction``, property
  bindings, and ``GtkApplication`` are used.


Configuration
=============

Some settings are configurable. Each setting is located in a file under
``$XDG_CONFIG_HOME/dwt/``. The following settings can be defined:

``allow-bold``
  If this file exists, bold fonts are allowed. This can be overriden
  with the ``-b`` / ``--bold`` command line flag.

``show-title``
  If this file exists, maximized windows keep a title bar instead of hiding
  it. This can be overriden with the ``-H`` / ``--title-on-maximize``
  command line flag.

``no-header-bar``
  If this file exists, a DWT will let the window manager decorate the
  windows, instead of using a header bar provided by itself. This can be
  overriden with the ``-N`` / ``--no-header-bar`` command line flag.

``scrollback``
  The number of lines saved in the scrollback buffer. The first line of the
  file is interpreted as an unsigned integer. This can be overriden with the
  ``-s`` / ``--scrollback`` command line flag.

``font``
  Name and characteristics of the font used by the terminal windows. Only
  the first line of the file is read. This can be overriden with the ``-f``
  / ``--font`` command line flag.


.. _VTE: http://developer.gnome.org/vte/
.. _DWM: http://dwm.suckless.org/
.. _XDWM: https://github.com/aperezdc/xdwm
