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

#include "xml_parser.hpp"
#include "xml_reader.hpp"

XMLReader::XMLReader(std::shared_ptr<XMLListNode> root_)
  : root(root_)
{
}

std::string XMLReader::get_name() const  {
  return root ? root->get_name() : "";
}

std::shared_ptr<XMLNode> XMLReader::get_node(const std::string& name) const {
  if (!root) {
    return std::shared_ptr<XMLNode>();
  }

  for (auto i : root->children) {
    if (i->get_name() == name) {
      return i;
    }
  }
  
    return std::shared_ptr<XMLNode>();
}

XMLReader XMLReader::get_section(const std::string& name) const {
  return XMLReader(std::static_pointer_cast<XMLListNode>(get_node(name)));
}

std::vector<XMLReader> XMLReader::get_sections() const {
  std::vector<XMLReader> lst;
  if (!root) {
    return lst;
  }

  for (auto i : root->children) {
    lst.push_back(XMLReader(std::static_pointer_cast<XMLListNode>(i)));
  }

  return lst;
}

std::vector<std::string>
XMLReader::get_string_list(const std::string& name) const {
  std::vector<std::string> lst;

  const auto node = std::dynamic_pointer_cast<XMLListNode>(get_node(name));
  if (!node) {
    return lst;
  }

  for(auto i : node->children) {
    if (auto data = std::dynamic_pointer_cast<XMLDataNode>(i)) {
      lst.push_back(data->data);
    }
  }
 
  return lst;
}

bool XMLReader::read(const std::string& name, bool& value) const {
  const auto node = std::dynamic_pointer_cast<XMLDataNode>(get_node(name));
  if (node) {
    value = (node->data != "0");
    return true;
  } 

  return false;
}

bool XMLReader::read(const std::string& name, int& value) const {
  const auto node = std::dynamic_pointer_cast<XMLDataNode>(get_node(name));
  if (node) {
    value = atoi(node->data.c_str());
    return true;
  }

  return false;
}

bool XMLReader::read(const std::string& name, std::string& value) const {
  const auto node = std::dynamic_pointer_cast<XMLDataNode>(get_node(name));
  if (node) {
    value = node->data;
    return true;
  }
  
  return false;
}

/* EOF */
