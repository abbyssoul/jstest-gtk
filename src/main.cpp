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

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <gtkmm/main.h>

#include "joystick_test_widget.hpp"
#include "joystick_list_widget.hpp"
#include "joystick_map_widget.hpp"
#include "joystick_calibration_widget.hpp"
#include "joystick.hpp"
#include "main.hpp"
#include "xml_writer.hpp"

Main* Main::_instance = NULL;

Main::Main() {
  assert(_instance == NULL);

  _instance = this;
}

Main::~Main() {
  assert(dialogs.size() == 0);
}

void Main::show_device_list_dialog() {
  // if (list_dialog) {
  //   list_dialog->show();
  // } else 
  {
    std::shared_ptr<Gtk::Dialog> list_dialog = std::make_shared<JoystickListWidget>();

    dialogs.push_back(list_dialog);
    list_dialog->signal_hide().connect(sigc::bind(sigc::mem_fun(this, &Main::on_dialog_hide), std::weak_ptr<Gtk::Dialog>(list_dialog)));
    list_dialog->show_all();
  }
}

void Main::show_device_property_dialog(const std::string& filename) {
  std::shared_ptr<Joystick> joystick;

  std::map<std::string, std::shared_ptr<Joystick>>::iterator i = joysticks.find(filename);
  if (i == joysticks.end()) {
    joystick = std::make_shared<Joystick>(filename);
    joysticks.insert({filename, joystick});
    // joysticks.insert(std::pair<std::string, std::shared_ptr<Joystick>>(filename, joystick));
  } else {
    joystick = i->second;
  }

  std::shared_ptr<Gtk::Dialog> dialog = std::make_shared<JoystickTestWidget>(joystick);
  dialogs.push_back(dialog);
  dialog->signal_hide().connect(sigc::bind(sigc::mem_fun(this, &Main::on_dialog_hide), std::weak_ptr<Gtk::Dialog>(dialog)));
  dialog->show_all();
}

void Main::show_calibration_dialog(const std::shared_ptr<Joystick>& joystick) {
  std::shared_ptr<Gtk::Dialog> dialog = std::make_shared<JoystickCalibrationWidget>(joystick);

  dialogs.push_back(dialog);
  dialog->signal_hide().connect(sigc::bind(sigc::mem_fun(this, &Main::on_dialog_hide), std::weak_ptr<Gtk::Dialog>(dialog)));
  dialog->show_all();
}

void Main::show_mapping_dialog(const std::shared_ptr<Joystick>& joystick) {
  std::shared_ptr<Gtk::Dialog> dialog = std::make_shared<JoystickMapWidget>(joystick);

  dialog->signal_hide().connect(sigc::bind(sigc::mem_fun(this, &Main::on_dialog_hide), std::weak_ptr<Gtk::Dialog>(dialog)));
  dialog->show_all();
  dialogs.push_back(dialog);
}

void Main::on_dialog_hide(std::weak_ptr<Gtk::Dialog> dialog) {
  dialogs.erase(std::remove(dialogs.begin(), dialogs.end(), dialog.lock()), dialogs.end());

  if (dialogs.empty()) {
    Gtk::Main::quit();
  }
}

int Main::main(int argc, char** argv) {
  typedef std::vector<std::string> DeviceFiles;
  DeviceFiles device_files;
  std::string config_save_file;

  for(int i = 1; i < argc; ++i) {
    if (!strcmp("--help", argv[i]) || !strcmp("-h", argv[i])) {
      std::cout << "Usage: " << argv[0] << " [OPTIONS]... [DEVICE]..." << std::endl
      << "A graphical joystick tester." << std::endl
      << std::endl
      << "Options:" << std::endl
      << "  -h, --help      Display this help and exit" << std::endl
      << "  -v, --version   Display version information and exit" << std::endl
      << "  -l, --load CFG  Load load configuration from file and apply them" << std::endl
      << "  -s, --save CFG  Save current device configuration to file CFG" << std::endl
      << std::endl
      << "Report bugs to Ingo Ruhnke <grumbel@gmx.de>." << std::endl;
      
      return EXIT_SUCCESS;
    } else if (!strcmp("--load", argv[i]) || !strcmp("-l", argv[i])) {
      ++i;

      if (i < argc) {
        std::cerr << "Configuration file load is not yet implemented" << std::endl;
        return 0;
      } else {
        std::cerr << "Error: " << argv[i-1] << " expected an argument" << std::endl;
        exit(EXIT_FAILURE);
      }
    } else if (!strcmp("--save", argv[i]) || !strcmp("-s", argv[i])) {
      ++i;

      if (i < argc) {
        config_save_file = argv[i];
      } else {
        std::cerr << "Error: " << argv[i-1] << " expected an argument" << std::endl;
        exit(EXIT_FAILURE);
      }
    } else if (!strcmp("--version", argv[i]) || !strcmp("-v", argv[i])) {
      std::cout << "jstest-gtk 0.1.0" << std::endl;

      return EXIT_SUCCESS;
    } else if (argv[i][0] == '-') {
      std::cerr << "Error: " << argv[0] << ": unrecognized option '" << argv[i] << "'" << std::endl;
      
      return EXIT_FAILURE;
    } else {
      device_files.push_back(argv[i]);
    }
  }

  if (!config_save_file.empty()) {
    XMLWriter out(config_save_file);
    out.start_section("joysticks");
    
    for(DeviceFiles::iterator i = device_files.begin(); i != device_files.end(); ++i) {
      Joystick joystick(*i);
      joystick.write(out);
    }
    
    out.end_section("joysticks");
  } else {
    //FIXME:
    // Glib::set_application_name("Joystick Test");
    // Glib::set_prgname("jstest-gtk");

    // cfg_directory = Glib::build_filename(Glib::get_user_config_dir(), Glib::get_prgname());
    // if (access(cfg_directory.c_str(), R_OK | W_OK) != 0 &&
    //     mkdir(cfg_directory.c_str(), 0770) != 0) {
      
    //   throw std::runtime_error(cfg_directory + ": " + strerror(errno));
    // }


    Gtk::Main kit(&argc, &argv);

    if (device_files.empty()) {
      show_device_list_dialog();

    } else {
      
      for(DeviceFiles::iterator i = device_files.begin(); i != device_files.end(); ++i) {
        show_device_property_dialog(*i);
      }
    }
    
    Gtk::Main::run();
  }

  return 0;
}

int main(int argc, char** argv) {
  try {
    Main app;
    
    return app.main(argc, argv);
  } catch(std::exception& err) {
    std::cerr << "Error: " << err.what() << std::endl;
    
    return EXIT_FAILURE;
  }
}

/* EOF */
