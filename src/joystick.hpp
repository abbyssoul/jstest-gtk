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

#ifndef HEADER_JSTEST_GTK_JOYSTICK_HPP
#define HEADER_JSTEST_GTK_JOYSTICK_HPP

#include <sigc++/signal.h>
#include <sigc++/connection.h>
#include <glibmm/main.h>
#include <linux/joystick.h>
#include "joystick_description.hpp"

class XMLReader;
class XMLWriter;

class Joystick {
public:
  struct CalibrationData {
    bool calibrate;
    bool invert;
    int  center_min;
    int  center_max;
    int  range_min;
    int  range_max;
  };

public:
  Joystick(const std::string& filename);
  ~Joystick();

  int get_fd() const { return fd; }

  void update();
  bool on_in(Glib::IOCondition cond);

  std::string get_filename() const  { return _description.filename; }
  std::string get_name() const      { return _description.name; }
  size_t get_axis_count() const     { return _description. axis_count; }
  size_t get_button_count() const   { return _description.button_count; }

  sigc::signal<void, size_t, int>  axis_move;
  sigc::signal<void, size_t, bool> button_move;

  int get_axis_state(size_t id);

  static std::vector<JoystickDescription> get_joysticks();

  std::vector<CalibrationData> get_calibration();
  void set_calibration(const std::vector<CalibrationData>& data);
  void reset_calibration();

  /** Clears all calibration data, note that this will mean raw USB
      input values, not values scaled to -32767/32767 */
  void clear_calibration();

  std::vector<int> get_button_mapping();
  std::vector<int> get_axis_mapping();

  void set_button_mapping(const std::vector<int>& mapping);
  void set_axis_mapping(const std::vector<int>& mapping);
  
  /** Corrects calibration data after remaping axes */
  void correct_calibration(const std::vector<int>& mapping_old, const std::vector<int>& mapping_new);
  
  void write(XMLWriter& out);
  void load(const XMLReader& reader);

  /** Get the evdev that this joystick device is based on. This call
      is just a guess, not guranteed to be the exact same device, but
      for our uses that should be enough. */
  std::string get_evdev() const;

private:
  Joystick(const Joystick&);
  Joystick& operator=(const Joystick&);

private:
  int fd;

  JoystickDescription _description;
  std::string orig_name;

  std::vector<int> axis_state;
  std::vector<CalibrationData> orig_calibration_data;

  sigc::connection connection;
};

#endif
/* EOF */
