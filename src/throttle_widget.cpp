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

#include "throttle_widget.hpp"

ThrottleWidget::ThrottleWidget(int width, int height, bool invert_)
  : CustomWidget(width, height), 
  invert(invert_), pos(0.0)
{
}

bool ThrottleWidget::on_expose(const Cairo::RefPtr<Cairo::Context> cr) {
  const double p = 1.0 - (pos + 1.0) / 2.0;

  const int w  = _drawingarea.get_allocation().get_width()-10;
  const int h  = _drawingarea.get_allocation().get_height()-10;

  cr->translate(5, 5);

  // Outer Rectangle
  cr->set_source_rgb(0.0, 0.0, 0.0);
  cr->set_line_width(1.0);
  cr->rectangle(0, 0, w, h);
  cr->stroke();

  const int dh = h*p;
  cr->rectangle(0, h - dh, w, dh);
  cr->fill();

  return true;
}

void ThrottleWidget::set_pos(double p) {
  pos = (invert) ? -p : p;
  queue_draw();
}

/* EOF */
