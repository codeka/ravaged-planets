#pragma once

#include <memory>
#include <string>

#include <framework/gui/widget.h>

namespace fw::gui {
class Gui;
class Drawable;
class ListboxItem;

/** A Listbox is a scrollable list of items. */
class Listbox : public Widget {
private:
  friend class ListboxItemSelectedProperty;
  friend class ListboxItemActivatedProperty;

  std::shared_ptr<Drawable> background_;
  Widget *item_container_;
  std::vector<ListboxItem *> items_;
  ListboxItem *selected_item_;
  bool scrollbar_visible_;

  void update_thumb_button(bool adjust_height);
  bool on_down_button_click(Widget *w);
  bool on_up_button_click(Widget *w);
public:
  Listbox(Gui *gui);
  virtual ~Listbox();

  static Property *item_selected(std::function<void(int index)> on_selected);
  static Property *item_activated(std::function<void(int index)> on_activated);

  // Add an item to the list. This is not quite the same as attach_child, so be sure to not use that. You must give
  // the widget you want to add an explicit height (in pixel, not percent). It's typically also best to give it an
  // (x,y) of (0,0) and a width of 100%.
  void add_item(Widget *w);

  /** Removes all the items in the listbox. */
  void clear();

  /** Select the item with the given index. */
  void select_item(int index);

  /** Activates the item with the given index. Basically just fires the signal. */
  void activate_item(int index);

  int get_selected_index();
  Widget *get_item(int index);
  Widget *get_selected_item();

  /** Signaled when an item is selected. */
  boost::signals2::signal<void(int index)> sig_item_selected;

  /** Signaled when an item is double-clicked. */
  boost::signals2::signal<void(int index)> sig_item_activated;

  void render();
};

}
