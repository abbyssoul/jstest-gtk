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

#ifndef HEADER_JSTEST_GTK_MAIN_HPP
#define HEADER_JSTEST_GTK_MAIN_HPP

#include <vector>
#include <map>
#include <memory>


#include <gtkmm/dialog.h>

class Joystick;
class JoystickListWidget;

class Main {
public:
  static Main* current() { return _instance; }

public:
  Main();
  ~Main();

  void show_device_list_dialog();
  void show_device_property_dialog(const std::string& filename);
  void show_calibration_dialog(const std::shared_ptr<Joystick>& joystick);
  void show_mapping_dialog(const std::shared_ptr<Joystick>& joystick);

  int main(int argc, char** argv);

  std::string get_cfg_directory() const { return cfg_directory; }

private:
  void on_dialog_hide(std::weak_ptr<Gtk::Dialog> dialog);

private:
  static Main* _instance;

private:
  // JoystickListWidget*        list_dialog;
  // std::vector<Joystick*>      joysticks;

  std::map<std::string, std::shared_ptr<Joystick>>      joysticks;
  std::vector<std::shared_ptr<Gtk::Dialog>>   dialogs;

  std::string cfg_directory;
};

#endif
/* EOF */
