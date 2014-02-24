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

#include "custom_widget.h"

CustomWidget::CustomWidget(int width, int height)
  : Gtk::Alignment(Gtk::ALIGN_CENTER, Gtk::ALIGN_START, 0.0f, 0.0f)
{
	_drawingarea.set_size_request(width, height);
	_drawingarea.signal_draw().connect(sigc::mem_fun(this, &CustomWidget::on_expose));
 	
 	add(_drawingarea);
}

CustomWidget::~CustomWidget() { 
 	// NoOp
 }

/* EOF */
