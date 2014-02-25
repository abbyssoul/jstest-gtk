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

#include <assert.h>
#include <math.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sstream>
#include <unistd.h>
#include <linux/joystick.h>
#include <glibmm/main.h>
#include <glibmm/convert.h>

#include <giomm/file.h>

#include "evdev_helper.hpp"
#include "xml_writer.hpp"
#include "xml_reader.hpp"
#include "joystick.hpp"

// Path to input devices
static const char* dev_path = "/dev/input/";

Joystick::Joystick(const std::string& filename_) 
  : _description(filename_, "", 0, 0)
{
  if ((fd = open(_description.filename.c_str(), O_RDONLY)) < 0) {
    std::ostringstream str;
    str << get_filename() << ": " << strerror(errno);
    throw std::runtime_error(str.str());
  } else {
      // ok
    uint8_t num_axis   = 0;
    uint8_t num_button = 0;
    ioctl(fd, JSIOCGAXES,    &num_axis);
    ioctl(fd, JSIOCGBUTTONS, &num_button);
    _description.axis_count   = num_axis;
    _description.button_count = num_button;

      // Get Name 
    char name_c_str[1024];
    if (ioctl(fd, JSIOCGNAME(sizeof(name_c_str)), name_c_str) < 0) {
      std::ostringstream str;
      str << get_filename() << ": " << strerror(errno);
      throw std::runtime_error(str.str());
    } else {
      
      orig_name = name_c_str;

      try {
        _description.name = Glib::convert_with_fallback(name_c_str, "UTF-8", "ISO-8859-1");
      } catch(Glib::ConvertError& err) {
        std::cerr << err.what() << std::endl;
      }
    }

    axis_state.resize(_description.axis_count);
  }

  orig_calibration_data = get_calibration();
  
  connection = Glib::signal_io().connect(sigc::mem_fun(this, &Joystick::on_in), fd, Glib::IO_IN);
}

Joystick::~Joystick() {
  connection.disconnect();
  close(fd);
}

bool Joystick::on_in(Glib::IOCondition) {
  update();

  return true;
}

void Joystick::update() {
  struct js_event event;

  const ssize_t len = read(fd, &event, sizeof(event));
  if (len < 0) {
    std::ostringstream str;
    str << get_filename() << ": " << strerror(errno);
    
    throw std::runtime_error(str.str());
  } else if (len == sizeof(event)) { // ok
    if (event.type & JS_EVENT_AXIS) {
        //std::cout << "Axis: " << event.number << " -> " << event.value << std::endl;
      axis_state[event.number] = event.value;
      axis_move(event.number, event.value);
    } else if (event.type & JS_EVENT_BUTTON) {
        //std::cout << "Button: " << event.number << " -> " << event.value << std::endl;
      button_move(event.number, event.value);
    }
  } else {
    throw std::runtime_error("Joystick::update(): unknown read error");
  }
}

std::vector<JoystickDescription> Joystick::get_joysticks() {
  static const std::string js = "js";

  // FIXME: Use dir()
  Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(dev_path);
  Glib::RefPtr<Gio::FileEnumerator> child_enumeration = file->enumerate_children(G_FILE_ATTRIBUTE_STANDARD_NAME);
 
  std::vector<JoystickDescription> results;

  Glib::RefPtr<Gio::FileInfo> file_info;
  while (file_info = child_enumeration->next_file()) {
    if (file_info->get_name().find(js) != 0) { // File name does not start with js
      continue;
    }

    // file_names.push_back(file_info->get_name());
    std::ostringstream str;
    str << dev_path << "/" << file_info->get_name();

    try {
      const Joystick joystick(str.str());

      results.push_back(JoystickDescription(joystick.get_filename(),
        joystick.get_name(),                                                  
        joystick.get_axis_count(),
        joystick.get_button_count()));
    } catch(std::exception& err) {
        // ok
    }
  }  

  return results;
}

Joystick::CalibrationData corr2cal(const struct js_corr& corr_) {
  struct js_corr corr = corr_;

  Joystick::CalibrationData data;

  if (corr.type) {
    data.calibrate = true;
    data.invert    = (corr.coef[2] < 0 && corr.coef[3] < 0);
    data.center_min = corr.coef[0];
    data.center_max = corr.coef[1];

    if (data.invert) {
      corr.coef[2] = -corr.coef[2];
      corr.coef[3] = -corr.coef[3];
    }

    // Need to use double and rint(), since calculation doesn't end
    // up on clean integer positions (i.e. 0.9999 can happen)
    data.range_min = rint(data.center_min - ((32767.0 * 16384) / corr.coef[2]));
    data.range_max = rint((32767.0 * 16384) / corr.coef[3] + data.center_max);
  } else {
    data.calibrate  = false;
    data.invert     = false;
    data.center_min = 0;
    data.center_max = 0;
    data.range_min  = 0;
    data.range_max  = 0;
  }

  return data;
}

std::vector<Joystick::CalibrationData> Joystick::get_calibration() {
  std::vector<js_corr> corr(get_axis_count());

  if (ioctl(fd, JSIOCGCORR, &*corr.begin()) < 0) {
    std::ostringstream str;
    str << get_filename() << ": " << strerror(errno);
    throw std::runtime_error(str.str());
  } else {
    std::vector<CalibrationData> data;
    std::transform(corr.begin(), corr.end(), std::back_inserter(data), corr2cal);
    return data;
  }
}

js_corr cal2corr(const Joystick::CalibrationData& data) {
  js_corr corr;

  if (data.calibrate &&
    (data.center_min - data.range_min)  != 0 &&
    (data.range_max  - data.center_max) != 0)
  {
    corr.type = 1;
    corr.prec = 0;
    corr.coef[0] = data.center_min;
    corr.coef[1] = data.center_max;

    corr.coef[2] = (32767 * 16384) / (data.center_min - data.range_min);
    corr.coef[3] = (32767 * 16384) / (data.range_max  - data.center_max);

    if (data.invert) {
      corr.coef[2] = -corr.coef[2];
      corr.coef[3] = -corr.coef[3];
    }
  } else {
    corr.type = 0;
    corr.prec = 0;
    memset(corr.coef, 0, sizeof(corr.coef));
  }

  return corr;
}

void Joystick::set_calibration(const std::vector<CalibrationData>& data) {
  std::vector<struct js_corr> corr;

  std::transform(data.begin(), data.end(), std::back_inserter(corr), cal2corr);
  if (ioctl(fd, JSIOCSCORR, &*corr.begin()) < 0) {
    std::ostringstream str;
    str << get_filename() << ": " << strerror(errno);
    throw std::runtime_error(str.str());
  }
}

void Joystick::clear_calibration() {
  std::vector<CalibrationData> data;

  for(size_t i = 0; i < get_axis_count(); ++i) {
    CalibrationData cal;

    cal.calibrate  = false;
    cal.invert     = false;
    cal.center_min = 0;
    cal.center_max = 0;
    cal.range_min  = 0;
    cal.range_max  = 0;

    data.push_back(cal);
  }

  set_calibration(data);
}

void Joystick::reset_calibration() {
  set_calibration(orig_calibration_data);
}

std::vector<int> Joystick::get_button_mapping() {
  uint16_t btnmap[KEY_MAX - BTN_MISC + 1];

  if (ioctl(fd, JSIOCGBTNMAP, btnmap) < 0) {
    std::ostringstream str;
    str << get_filename() << ": " << strerror(errno);
    throw std::runtime_error(str.str());
  } else {
    std::vector<int> mapping;
    std::copy(btnmap, btnmap + get_button_count(), std::back_inserter(mapping));
    return mapping;
  }
}

std::vector<int> Joystick::get_axis_mapping() {
  uint8_t axismap[ABS_MAX + 1];

  if (ioctl(fd, JSIOCGAXMAP, axismap) < 0) {
    std::ostringstream str;
    str << get_filename() << ": " << strerror(errno);
    
    throw std::runtime_error(str.str());

  } else {
    std::vector<int> mapping;
    std::copy(axismap, axismap + get_axis_count(), std::back_inserter(mapping));
    
    return mapping;
  }
}

void Joystick::set_button_mapping(const std::vector<int>& mapping) {
  assert(mapping.size() == get_button_count());

  uint16_t btnmap[KEY_MAX - BTN_MISC + 1];
  memset(btnmap, 0, sizeof(btnmap));
  std::copy(mapping.begin(), mapping.end(), btnmap);

  if (ioctl(fd, JSIOCSBTNMAP, btnmap) < 0) {
    std::ostringstream str;
    str << get_filename() << ": " << strerror(errno);
    
    throw std::runtime_error(str.str());
  }
}

int Joystick::get_axis_state(size_t id) {
  return (id < axis_state.size()) ? axis_state[id] : 0;
}

void Joystick::set_axis_mapping(const std::vector<int>& mapping) {
  assert(mapping.size() == get_axis_count());

  uint8_t axismap[ABS_MAX + 1];
  std::copy(mapping.begin(), mapping.end(), axismap);

  if (ioctl(fd, JSIOCSAXMAP, axismap) < 0) {
    std::ostringstream str;
    str << get_filename() << ": " << strerror(errno);
    throw std::runtime_error(str.str());
  }
}

void Joystick::correct_calibration(const std::vector<int>& mapping_old, const std::vector<int>& mapping_new) {
  int axes[ABS_MAX + 1]; // axes[name] -> old_idx
  for (std::vector<int>::const_iterator i = mapping_old.begin(); i != mapping_old.end(); ++i) {
    axes[*i] = i - mapping_old.begin();
  }

  const auto callib_old = get_calibration();
  std::vector<CalibrationData> callib_new;
  for (auto i : mapping_new) {
    callib_new.push_back(callib_old[axes[i]]);
  }

  set_calibration(callib_new);
}

void Joystick::write(XMLWriter& out) {
  out.start_section("joystick");
  out.write("name",   get_name());
  out.write("device", get_filename());

  { // write CalibrationData
    const auto data = get_calibration();

    out.start_section("calibration");
    for(auto i : data) {
      out.start_section("axis");
        //out.write("id",         i - data.begin());
      out.write("calibrate",  i.calibrate);
      out.write("center-min", i.center_min);
      out.write("center-max", i.center_max);
      out.write("range-min",  i.range_min);
      out.write("range-max",  i.range_max);
      out.write("invert",     i.invert);

      out.end_section("axis");
    }
    
    out.end_section("calibration");
  }

  {
    const auto mapping = get_axis_mapping();
    out.start_section("axis-map");
    for(auto i : mapping) {
      out.write("axis", abs2str(i));
    }
    out.end_section("axis-map");
  }

  {
    const auto mapping = get_button_mapping();
    out.start_section("button-map");
    for (auto i : mapping) {
      out.write("button", btn2str(i));
    }   
    out.end_section("button-map");
  }

  out.end_section("joystick");
}

void Joystick::load(const XMLReader& root_reader) {
  std::string cfg_name;

  if (!(root_reader.read("name", cfg_name) && get_name() != cfg_name))
    return;

  // Read calibration data
  if (auto reader = root_reader.get_section("calibration")) {
    const auto& sections = reader.get_sections();

    std::vector<CalibrationData> calibration_data;
    for (auto i : sections) {
      CalibrationData data;

      //i->read("axis", );
      //i->read("precision", );
      i.read("invert",     data.invert);
      i.read("center-min", data.center_min);
      i.read("center-max", data.center_max);
      i.read("range-min",  data.range_min);
      i.read("range-max",  data.range_max);

      calibration_data.push_back(data);
    }

    set_calibration(calibration_data);
  }

  { // Read axis mapping
    const std::vector<std::string>& cfg_axis_map = root_reader.get_string_list("axis-map");
    std::vector<int> mapping;
    
    for (auto i : cfg_axis_map) {
      int type = 0;
      int code = 0;
      str2event(i, type, code);
      mapping.push_back(code);
    }

    set_axis_mapping(mapping);
  }

  { // Read button mapping
    const auto& cfg_button_map = root_reader.get_string_list("button-map");
    std::vector<int> mapping;
    
    for (auto i : cfg_button_map) {
      int type = 0;
      int code = 0;
      str2event(i, type, code);
      mapping.push_back(code);
    }

    set_button_mapping(mapping);
  }

}

std::string Joystick::get_evdev() const {
  static const std::string event = "event";

  Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(dev_path);
  Glib::RefPtr<Gio::FileEnumerator> child_enumeration = file->enumerate_children(G_FILE_ATTRIBUTE_STANDARD_NAME);
 
// See /usr/share/doc/linux-doc-2.6.28/devices.txt.gz
  Glib::RefPtr<Gio::FileInfo> file_info;
  while (file_info = child_enumeration->next_file()) {
    if (file_info->get_name().find(event) != 0) { // File name does not start with js
      continue;
    }
    
    // file_names.push_back(file_info->get_name());
    std::ostringstream str;
    str << dev_path << "/" << file_info->get_name();

    const int evdev_fd  = open(str.str().c_str(), O_RDONLY);
    if (evdev_fd >= 0) {
      char evdev_name[256];
      if (ioctl(evdev_fd, EVIOCGNAME(sizeof(evdev_name)), evdev_name) < 0) {
        std::cerr << str.str() << ": " << strerror(errno) << std::endl;
      } else {
        if (orig_name == evdev_name) {// Found a device that matches, so return it
          close(evdev_fd);
          return str.str();
        }
      }

      close(evdev_fd);
    }
  }  

  throw std::runtime_error("couldn't find evdev for " + get_filename());
}

#ifdef __TEST__

// g++ -D__TEST__ joystick.cpp evdev_helper.cpp xml_writer.cpp xml_reader.cpp -o joystick-test `pkg-config --cflags --libs gtkmm-2.4 sigc++-2.0`  

int main(int argc, char** argv) {
  
  for(int i = 1; i < argc; ++i) {
    const Joystick joystick(argv[i]);

    std::cout << "Filename: '" << joystick.get_filename() << "'" << std::endl;
    std::cout << "Name:     '" << joystick.get_name() << "'" << std::endl;
    std::cout << "Axis:     "  << joystick.get_axis_count() << "'" << std::endl;
    std::cout << "Button:   "  << joystick.get_button_count() << "'" << std::endl;
    std::cout << "Evdev:    '" << joystick.get_evdev() << "'" << std::endl;
  }
  
  return EXIT_SUCCESS;
}
#endif

/* EOF */
