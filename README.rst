==========================
DWT - Dumb Window Terminal
==========================

DWT is a simple, no-frill terminal emulator based on the VTE_ widget, whose
main feature is being feature-less, thus simple, lean, mean and a very good
companion for tiling window managers like DWM_ (or my own XDWM_ fork).

(Non-)Features
==============

* Fixed configuration:

  - Grey on black color scheme with using Linux console color set.

  - Converts terminal bell beeps in “urgent” X window flags. No visible
    or audible terminal bell, besides from that.

  - Non-blinking cursor.

  - Scrollback buffer, but no scrollbar displayed, to save screen real
    estate. Use ``Shift-PageUp`` and ``Shift-PageDown`` to scroll.

  - Mouse cursor auto-hide.

* Keyboard accelerators configurable at build time:

  - Just edit ``dwt-accels.h`` before building.

* XTerm-style configurable window title.

* Clickable URLs. Because on the Internet era being able to quickly open
  a browser is a must-have feature.

.. _VTE: http://developer.gnome.org/vte/
.. _DWM: http://dwm.suckless.org/
.. _XDWM: https://github.com/aperezdc/xdwm
