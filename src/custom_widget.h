
#ifndef HEADER_JSTEST_GTK_CUSTOM_WIDGET_HPP
#define HEADER_JSTEST_GTK_CUSTOM_WIDGET_HPP

#include <gtkmm/drawingarea.h>
#include <gtkmm/alignment.h>

class CustomWidget : public Gtk::Alignment {
public:
	CustomWidget(int width, int height);
	virtual ~CustomWidget();

protected:
	virtual bool on_expose(const Cairo::RefPtr<Cairo::Context> cr) = 0;

private:
	CustomWidget(const CustomWidget&);
	CustomWidget& operator=(const CustomWidget&);

protected:
	Gtk::DrawingArea _drawingarea;
};

#endif

/* EOF */
