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

* Fixed configuration:

  - Grey on black color scheme with using Linux console color set.

  - Converts terminal bell beeps in “urgent” X window flags. No visible
    or audible terminal bell, besides from that.

  - Non-blinking cursor.

  - Scrollback buffer, but no scrollbar displayed, to save screen real
    estate. Use ``Shift-PageUp`` and ``Shift-PageDown`` to scroll.

  - Keybindings to change font size: Use ``Super-+`` and ``Super--``
    to change sizes, ``Super-0`` to reset the font.

  - Mouse cursor auto-hide.

* Some build-time configuration. The following defines can be added in
  ``$CPPFLAGS``:

  - ``DWT_DEFAULT_FONT="FontName"`` (default: ``monospace 11``).

  - ``DWT_USE_POPOVER=TRUE`` (disabled by default): Uses
    `GtkPopover <https://developer.gnome.org/gtk3/stable/GtkPopover.html>`__
    to provide a contextual menu.

  - ``DWT_USE_USE_HEADER_BAR=TRUE`` (disabled by default): Uses
    a `GtkHeaderBar <https://developer.gnome.org/gtk3/stable/GtkHeaderBar.html>`__
    for the title bar of the window. The bar includes a “New Terminal”
    button, and terminal beeps make an “attention” icon display for an
    instant in the right side of the header bar.

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
