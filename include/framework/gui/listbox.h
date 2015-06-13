#pragma once

#include <memory>
#include <string>

#include <framework/gui/widget.h>

namespace fw { namespace gui {
class gui;
class drawable;
class listbox_item;

/** A listbox is a scrollable list of items. */
class listbox : public widget {
private:
  std::shared_ptr<drawable> _background;
  std::vector<listbox_item *> _items;

public:
  listbox(gui *gui);
  virtual ~listbox();

  /**
   * Add an item to the list. This is not quite the same as attach_child, so be sure to not use that. You must give
   * the widget you want to add an explicit height (in pixel, not percent). It's typically also best to give it an
   * (x,y) of (0,0) and a width of 100%.
   */
  void add_item(widget *w);

  void render();
};

} }
