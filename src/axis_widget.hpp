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

#ifndef HEADER_JSTEST_GTK_AXIS_WIDGET_HPP
#define HEADER_JSTEST_GTK_AXIS_WIDGET_HPP

#include "custom_widget.h"

class AxisWidget : public CustomWidget {
public:
	AxisWidget(int width, int height);
	virtual ~AxisWidget() { }

	void set_x_axis(double x);
	void set_y_axis(double x);

protected:
	virtual bool on_expose(const Cairo::RefPtr<Cairo::Context> cr);

private:
	double x;
	double y;
};

#endif

/* EOF */
