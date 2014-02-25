/*
**  jstest-gtk - A graphical joystick tester
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**  
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**  
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <gtkmm/stock.h>

#include "evdev_helper.hpp"
#include "joystick.hpp"
#include "joystick_map_widget.hpp"

JoystickMapWidget::JoystickMapWidget(const std::shared_ptr<Joystick>& joystick)
  : Gtk::Dialog("Mapping: " + joystick->get_name()),
    label("Change the order of axis and button. The order applies directly to the "
          "joystick kernel driver, so it will work in any game, it is however not "
          "persistant across reboots."),
    axis_map(joystick, RemapWidget::REMAP_AXIS),
    button_map(joystick, RemapWidget::REMAP_BUTTON)
{
  set_border_width(5);
  label.set_line_wrap();

  hbox.add(axis_map);
  hbox.add(button_map);

  get_vbox()->pack_start(label, Gtk::PACK_SHRINK);
  get_vbox()->pack_start(hbox, Gtk::PACK_EXPAND_WIDGET);

  add_button(Gtk::Stock::REVERT_TO_SAVED, 1);
  add_button(Gtk::Stock::CLOSE, 0);

  const auto& button_mapping = joystick->get_button_mapping();
  for(auto i = button_mapping.begin(); i != button_mapping.end(); ++i) {
    std::ostringstream s;
    s << (i - button_mapping.begin()) << ": " << btn2str(*i);
    button_map.add_entry(*i, s.str());
  }

  const auto& axis_mapping = joystick->get_axis_mapping();
  for(auto i = axis_mapping.begin(); i != axis_mapping.end(); ++i) {
    std::ostringstream s;
    s << (i - axis_mapping.begin()) << ": " << abs2str(*i);
    axis_map.add_entry(*i, s.str());
  }

  signal_response().connect(sigc::mem_fun(this, &JoystickMapWidget::on_response));
}

void JoystickMapWidget::on_response(int v) {
  if (v == 0) {
    hide();
  } else if (v == 1) {
    button_map.on_clear();
    axis_map.on_clear();
  }
}

/* EOF */
