#pragma once

#include <memory>
#include <string>

#include <framework/signals.h>
#include <framework/gui/builder.h>
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/widget.h>
#include "linear_layout.h"

namespace fw::gui {
class ListboxItem;

/**
 * A Listbox is a scrollable list of items.
 */
class Listbox : public Widget {
public:
  Listbox();
  virtual ~Listbox();

  void OnAttachedToParent(Widget &parent) override;

  static std::unique_ptr<Property> item_selected(std::function<void(int index)> on_selected);
  static std::unique_ptr<Property> item_activated(std::function<void(int index)> on_activated);

  // Add an item to the list. This is not quite the same as attach_child, so be sure to not use
  // that. You must give the widget you want to add an explicit height (in pixel, not percent).
  // It's typically also best to give it an (x,y) of (0,0) and a width of 100%.
  void AddItem(std::shared_ptr<Widget> w);

  template <IsSubclassOfWidget T>
  inline void AddItem(std::shared_ptr<T> w) {
    AddItem(std::dynamic_pointer_cast<Widget>(w));
  }
  template <IsSubclassOfWidget T>
  inline void AddItem(Builder<T> &builder) {
    AddItem(builder.Build(this->shared_from_this()));
  }

  /** Removes all the items in the listbox. */
  void Clear();

  /** Select the item with the given index. */
  void SelectItem(int index);

  /** Activates the item with the given index. Basically just fires the signal. */
  void ActivateItem(int index);

  int GetSelectedIndex();
  std::shared_ptr<Widget> GetItem(int index);
  std::shared_ptr<Widget> GetSelectedItem();

  MeasuredSize OnMeasure(MeasureSpec width_spec, MeasureSpec height_spec) override;

  /** Signaled when an item is selected. */
  fw::Signal<int /*index*/> sig_item_selected;

  /** Signaled when an item is double-clicked. */
  fw::Signal<int /*index*/> sig_item_activated;

  void render() override;
private:
  friend class ListboxItemSelectedProperty;
  friend class ListboxItemActivatedProperty;

  std::shared_ptr<Drawable> background_;
  std::shared_ptr<LinearLayout> item_container_;
  std::vector<std::shared_ptr<ListboxItem>> items_;
  std::weak_ptr<ListboxItem> selected_item_;
  bool scrollbar_visible_;

  void UpdateThumbButton(bool adjust_height);
  bool OnDownButtonClick(Widget& w);
  bool OnUpButtonClick(Widget& w);
};

}
