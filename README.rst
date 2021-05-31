==========================
DWT - Dumb Window Terminal
==========================

**DWT is no longer maintained** — you might want to use my
`Termite fork`__ instead.

.. __: https://github.com/aperezdc/termite

DWT is a simple, sleek emulator based on the VTE_ widget. Its main features
are being nice-looking (according to my personal taste), not being (very)
configurable, and being a good companion for tiling window managers like
DWM_ (or my own XDWM_ fork). That being said, it works flawlessly and looks
slick in any GTK+-based environment.

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


.. _VTE: http://developer.gnome.org/vte/
.. _DWM: http://dwm.suckless.org/
.. _XDWM: https://github.com/aperezdc/xdwm
