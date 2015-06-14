#pragma once

#include <memory>
#include <string>

#define BOOST_BIND_NO_PLACEHOLDERS // so it doesn't auto-include _1, _2 etc.
#include <boost/signals2.hpp>

#include <framework/gui/widget.h>

namespace fw { namespace gui {
class gui;
class drawable;
class listbox_item;

/** A listbox is a scrollable list of items. */
class listbox : public widget {
private:
  std::shared_ptr<drawable> _background;
  widget *_item_container;
  std::vector<listbox_item *> _items;
  listbox_item *_selected_item;

  void update_thumb_button(bool adjust_height);
  bool on_down_button_click(widget *w);
  bool on_up_button_click(widget *w);
public:
  listbox(gui *gui);
  virtual ~listbox();

  /**
   * Add an item to the list. This is not quite the same as attach_child, so be sure to not use that. You must give
   * the widget you want to add an explicit height (in pixel, not percent). It's typically also best to give it an
   * (x,y) of (0,0) and a width of 100%.
   */
  void add_item(widget *w);

  /** Select the item with the given index. */
  void select_item(int index);

  int get_selected_index();
  widget *get_item(int index);
  widget *get_selected_item();

  /** Signaled when an item is selected. */
  boost::signals2::signal<void(int index)> sig_item_selected;

  void render();
};

} }
