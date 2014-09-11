/*
    Copyright (C) 2001 Paul Davis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <sigc++/bind.h>
#include <boost/scoped_ptr.hpp>

#include "ardour/tempo.h"
#include "ardour/profile.h"

#include "canvas/rectangle.h"
#include "canvas/container.h"
#include "canvas/line.h"
#include "canvas/polygon.h"
#include "canvas/text.h"
#include "canvas/canvas.h"
#include "canvas/scroll_group.h"
#include "canvas/utils.h"
#include "canvas/debug.h"

#include "ardour_ui.h"
/*
 * ardour_ui.h include was moved to the top of the list
 * due to a conflicting definition of 'Rect' between
 * Apple's MacTypes.h and GTK.
 */
#include "marker.h"
#include "public_editor.h"
#include "utils.h"
#include "rgb_macros.h"

#include <gtkmm2ext/utils.h>

#include "i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace ARDOUR_UI_UTILS;
using namespace Gtkmm2ext;

PBD::Signal1<void,Marker*> Marker::CatchDeletion;

const double Marker::_marker_height = 17.0;
const char * Marker::default_new_marker_prefix = N_("MARKER");

static const double name_padding = 10.0;

Marker::Marker (PublicEditor& ed, ArdourCanvas::Container& parent, guint32 rgba, const string& annotation,
		Type type, framepos_t frame, bool handle_events)

	: editor (ed)
	, _parent (&parent)
        , group (0)
        , mark (0)
        , _name_item (0)
	, _track_canvas_line (0)
        , _scene_change_rect (0)
        , _scene_change_text (0)
	, _type (type)
	, _selected (false)
	, _shown (false)
	, _line_shown (false)
	, _color (rgba)
	, _left_label_limit (DBL_MAX)
	, _right_label_limit (DBL_MAX)
	, _label_offset (0)
        , _have_scene_change (false)
{
	frame_position = frame;
	unit_position = editor.sample_to_pixel (frame);
	unit_position -= _shift;

	group = new ArdourCanvas::Container (&parent, ArdourCanvas::Duple (unit_position, 0));
	CANVAS_DEBUG_NAME (group, string_compose ("Marker::group for %1", annotation));

	_name_background = new ArdourCanvas::Rectangle (group);
	CANVAS_DEBUG_NAME (_name_background, string_compose ("Marker::_name_background for %1", annotation));

        if (!ARDOUR::Profile->get_trx()) {
                add_polygon (type);
                CANVAS_DEBUG_NAME (mark, string_compose ("Marker::mark for %1", annotation));
        } else {
                _label_offset = name_padding;
        }

	set_color_rgba (rgba);

        if (type == Mark) {
                _line_shown = true;
        }
        
	/* setup name pixbuf sizes */
	name_font = Pango::FontDescription (ARDOUR_UI::config()->get_canvasvar_SmallFont());

	Gtk::Label foo;

	Glib::RefPtr<Pango::Layout> layout = foo.create_pango_layout (X_("Hg")); /* ascender + descender */
	int width;

	layout->set_font_description (name_font);
	Gtkmm2ext::get_ink_pixel_size (layout, width, name_height);
	
	_name_item = new ArdourCanvas::Text (group);
	CANVAS_DEBUG_NAME (_name_item, string_compose ("Marker::_name_item for %1", annotation));
	_name_item->set_font_description (name_font);
        /* white with 95% opacity */
	_name_item->set_color (ArdourCanvas::rgba_to_color (1.0,1.0,1.0,0.95));
	_name_item->set_position (ArdourCanvas::Duple (_label_offset, (_marker_height / 2.0) - (name_height / 2.0)));

	set_name (annotation.c_str());

	editor.ZoomChanged.connect (sigc::mem_fun (*this, &Marker::reposition));

	/* events will be handled by both the group and the mark itself, so
	 * make sure they can both be used to lookup this object.
	 */

	group->set_data ("marker", this);
        if (mark) {
                mark->set_data ("marker", this);
        }
	
	if (handle_events) {
		group->Event.connect (sigc::bind (sigc::mem_fun (editor, &PublicEditor::canvas_marker_event), group, this));
	}
}

Marker::~Marker ()
{
	CatchDeletion (this); /* EMIT SIGNAL */

	/* destroying the parent group destroys its contents, namely any polygons etc. that we added */
	delete group;
	delete _track_canvas_line;
}

void
Marker::add_polygon (Type type)
{
        boost::scoped_ptr<ArdourCanvas::Points> points (new ArdourCanvas::Points());

	/* Shapes we use:

	  Mark:

	   (0,0) -> (6,0)
	     ^        |
	     |	      V
           (0,5)    (6,5)
	       \    /
               (3,_marker_height)


	   TempoMark:
	   MeterMark:

               (3,0)
              /      \
	   (0,5) -> (6,5)
	     ^        |
	     |	      V
           (0,10)<-(6,10)


           Start:

	   0,0\
       	    |  \
            |   \ 6,6
	    |	/
            |  /
           0,12

	   End:

	       /12,0
	      /     |
             /      |
	   6,6      |
             \      |
              \     |
               \    |
	       12,12

             PunchIn:

	     0,0 ------> _marker_height,0
	      |       /
	      |      /
	      |     /
	      |    /
	      |   /
	      |  /
	     0,_marker_height

	     PunchOut

	   0,0 -->-_marker_height,0
	    \       |
	     \      |
	      \     |
	       \    |
	        \   |
	         \  |
	         _marker_height,_marker_height


	*/

        switch (type) {
        case Mark:
                points->push_back (ArdourCanvas::Duple (0.0, 0.0));
                points->push_back (ArdourCanvas::Duple (6.0, 0.0));
                points->push_back (ArdourCanvas::Duple (6.0, 5.0));
                points->push_back (ArdourCanvas::Duple (3.0, _marker_height));
                points->push_back (ArdourCanvas::Duple (0.0, 5.0));
		points->push_back (ArdourCanvas::Duple (0.0, 0.0));
                
		_shift = 3;
		_label_offset = 8.0;
		break;

	case Tempo:
	case Meter:

		points->push_back (ArdourCanvas::Duple (3.0, 0.0));
		points->push_back (ArdourCanvas::Duple (6.0, 5.0));
		points->push_back (ArdourCanvas::Duple (6.0, 10.0));
		points->push_back (ArdourCanvas::Duple (0.0, 10.0));
		points->push_back (ArdourCanvas::Duple (0.0, 5.0));
		points->push_back (ArdourCanvas::Duple (3.0, 0.0));

		_shift = 3;
		_label_offset = 8.0;
		break;

	case SessionStart:
	case RangeStart:

		points->push_back (ArdourCanvas::Duple (0.0, 0.0));
		points->push_back (ArdourCanvas::Duple (6.5, 6.5));
		points->push_back (ArdourCanvas::Duple (0.0, _marker_height));
		points->push_back (ArdourCanvas::Duple (0.0, 0.0));

		_shift = 0;
		_label_offset = _marker_height;
		break;

	case SessionEnd:
	case RangeEnd:
		points->push_back (ArdourCanvas::Duple (6.5, 6.5));
		points->push_back (ArdourCanvas::Duple (_marker_height, 0.0));
		points->push_back (ArdourCanvas::Duple (_marker_height, _marker_height));
		points->push_back (ArdourCanvas::Duple (6.5, 6.5));

		_shift = _marker_height;
		_label_offset = 6.0;
		break;

	case LoopStart:
		points->push_back (ArdourCanvas::Duple (0.0, 0.0));
		points->push_back (ArdourCanvas::Duple (_marker_height, _marker_height));
		points->push_back (ArdourCanvas::Duple (0.0, _marker_height));
		points->push_back (ArdourCanvas::Duple (0.0, 0.0));

		_shift = 0;
		_label_offset = 12.0;
		break;

	case LoopEnd:
		points->push_back (ArdourCanvas::Duple (_marker_height,  0.0));
		points->push_back (ArdourCanvas::Duple (_marker_height, _marker_height));
		points->push_back (ArdourCanvas::Duple (0.0, _marker_height));
		points->push_back (ArdourCanvas::Duple (_marker_height, 0.0));

		_shift = _marker_height;
		_label_offset = 0.0;
		break;

	case  PunchIn:
		points->push_back (ArdourCanvas::Duple (0.0, 0.0));
		points->push_back (ArdourCanvas::Duple (_marker_height, 0.0));
		points->push_back (ArdourCanvas::Duple (0.0, _marker_height));
		points->push_back (ArdourCanvas::Duple (0.0, 0.0));

		_shift = 0;
		_label_offset = _marker_height;
		break;

	case  PunchOut:
		points->push_back (ArdourCanvas::Duple (0.0, 0.0));
		points->push_back (ArdourCanvas::Duple (12.0, 0.0));
		points->push_back (ArdourCanvas::Duple (12.0, 12.0));
		points->push_back (ArdourCanvas::Duple (0.0, 0.0));

		_shift = _marker_height;
		_label_offset = 0.0;
		break;

	}

	mark = new ArdourCanvas::Polygon (group);
	mark->set (*points);
}

void Marker::reparent(ArdourCanvas::Container & parent)
{
	group->reparent (&parent);
	_parent = &parent;
}

void
Marker::set_selected (bool s)
{
	_selected = s;
	setup_line ();
}

void
Marker::set_show_line (bool s)
{
	_line_shown = s;
	setup_line ();
}

void
Marker::setup_line ()
{
	if ((Profile->get_trx() && _type == Mark) || (_shown && ((!Profile->get_trx() && _selected) || _line_shown))) {

		if (_track_canvas_line == 0) {

			_track_canvas_line = new ArdourCanvas::Line (editor.get_hscroll_group());
			_track_canvas_line->Event.connect (sigc::bind (sigc::mem_fun (editor, &PublicEditor::canvas_marker_event), group, this));
		}

		ArdourCanvas::Duple g = group->canvas_origin();
		ArdourCanvas::Duple d = _track_canvas_line->canvas_to_item (ArdourCanvas::Duple (g.x + _shift, 0));

		_track_canvas_line->set_x0 (d.x);
		_track_canvas_line->set_x1 (d.x);
		_track_canvas_line->set_y0 (_marker_height);
                _track_canvas_line->set_y1 (ArdourCanvas::COORD_MAX);
                if (ARDOUR::Profile->get_trx()) {
                        _track_canvas_line->set_outline_color (_color);
                } else {
                        _track_canvas_line->set_outline_color (_selected ? ARDOUR_UI::config()->get_canvasvar_EditPoint() : _color);
                }
		_track_canvas_line->raise_to_top ();
		_track_canvas_line->show ();

	} else {
		if (_track_canvas_line) {
			_track_canvas_line->hide ();
		}
	}
}

void
Marker::canvas_height_set (double h)
{
	_canvas_height = h;
	setup_line ();
}

ArdourCanvas::Item&
Marker::the_item() const
{
	return *group;
}

void
Marker::set_name (const string& new_name)
{
	_name = new_name;

	setup_name_display ();
}

/** @return true if our label is on the left of the mark, otherwise false */
bool
Marker::label_on_left () const
{
	return (_type == SessionEnd || _type == RangeEnd || _type == LoopEnd || _type == PunchOut);
}

void
Marker::setup_name_display ()
{
	double limit = DBL_MAX;

	if (label_on_left ()) {
		limit = _left_label_limit;
	} else {
		limit = _right_label_limit;
	}

	/* Work out how wide the name can be */
	int name_width;
        string text = _name;
        int scene_change_width = 0.0;

        if (_have_scene_change) {

                /* coordinates of rect that will surround "MIDI" */

                ArdourCanvas::Rect r;
                int midi_height;

                pixel_size (X_("MIDI"), name_font, name_width, midi_height);

                r.x0 = 2.0;
                r.x1 = r.x1 + name_width + 7.0;

                if (_scene_change_text == 0) {
                        _scene_change_rect = new ArdourCanvas::Rectangle (group);
                        _scene_change_text = new ArdourCanvas::Text (group);
                        /* move name label over */
                        _label_offset += r.x1;
                }
                 
                /* white with 95% opacity */
                _scene_change_rect->set_outline_color (ArdourCanvas::rgba_to_color (1.0, 1.0, 1.0, 0.95));
                _scene_change_rect->set_fill (false);
                
                _scene_change_text->set_font_description (name_font);
                _scene_change_text->set_color (ArdourCanvas::rgba_to_color (1.0, 1.0, 1.0, 0.95));
                _scene_change_text->set (X_("MIDI"));
                        
                /* 4 pixels left margin, place it in the vertical middle.
                 */
                _scene_change_text->set_position (ArdourCanvas::Duple (4.0, (_marker_height / 2.0) - (name_height / 2.0)));

                r.y0 = _scene_change_text->position().y - 2.0;
                r.y1 = r.y0 + name_height + 4.0;

                _scene_change_rect->set (r);
                scene_change_width = r.x1;
                
        } else {
                if (_scene_change_text) {
                        delete _scene_change_text;
                        delete _scene_change_rect;
                        _scene_change_text = 0;
                        _scene_change_rect = 0;
                }
        }

        if (ARDOUR::Profile->get_trx()) {
                name_width = min ((double) pixel_width (_name, name_font) + (2 * name_padding), limit);
        } else {
                name_width = min ((double) pixel_width (_name, name_font) + 2, limit);
        }
	if (name_width == 0) {
		_name_item->hide ();
	} else {
		_name_item->show ();

                if (ARDOUR::Profile->get_trx()) {
                        _name_item->set_x_position (_label_offset);
                } else {
                        if (label_on_left ()) {
                                _name_item->set_x_position (-name_width);
                        }
                }
			
		_name_item->clamp_width (name_width);
		_name_item->set (_name);
		
                if (ARDOUR::Profile->get_trx()) {
                        _name_background->set_x0 (_name_item->position().x - _label_offset);
                        _name_background->set_x1 (_name_item->position().x - _label_offset + name_width + scene_change_width);
                } else {
                        if (label_on_left ()) {
                                _name_background->set_x0 (_name_item->position().x - 2);
                                _name_background->set_x1 (_name_item->position().x + name_width + _shift + scene_change_width);
                        } else {
                                _name_background->set_x0 (_name_item->position().x - _label_offset + 2);
                                _name_background->set_x1 (_name_item->position().x + name_width + scene_change_width);
                        }
                }
	}

	_name_background->set_y0 (0);
	_name_background->set_y1 (_marker_height + 1.0); 
}

void
Marker::set_position (framepos_t frame)
{
	unit_position = editor.sample_to_pixel (frame) - _shift;
	group->set_x_position (unit_position);
	setup_line ();
	frame_position = frame;
}

void
Marker::reposition ()
{
	set_position (frame_position);
}

void
Marker::show ()
{
	_shown = true;

        group->show ();
	setup_line ();
}

void
Marker::hide ()
{
	_shown = false;

	group->hide ();
	setup_line ();
}

void
Marker::set_color_rgba (uint32_t c)
{
	_color = c;

        if (mark) {
                mark->set_fill_color (_color);
                mark->set_outline_color (_color);
        }

	if (_track_canvas_line && !_selected) {
		_track_canvas_line->set_outline_color (_color);
	}

	_name_background->set_fill (true);
	_name_background->set_fill_color (UINT_RGBA_CHANGE_A (_color, 0x70));
        /* white with 20% opacity */
	_name_background->set_outline_color (ArdourCanvas::rgba_to_color (1.0, 1.0, 1.0, 0.20));

        if (ARDOUR::Profile->get_trx()) {
                _name_background->set_outline_what (ArdourCanvas::Rectangle::What (ArdourCanvas::Rectangle::TOP|
                                                                                   ArdourCanvas::Rectangle::LEFT|
                                                                                   ArdourCanvas::Rectangle::RIGHT));
        }
}

/** Set the number of pixels that are available for a label to the left of the centre of this marker */
void
Marker::set_left_label_limit (double p)
{
	/* Account for the size of the marker */
	_left_label_limit = p - _marker_height;
	if (_left_label_limit < 0) {
		_left_label_limit = 0;
	}

	if (label_on_left ()) {
		setup_name_display ();
	}
}

/** Set the number of pixels that are available for a label to the right of the centre of this marker */
void
Marker::set_right_label_limit (double p)
{
	/* Account for the size of the marker */
	_right_label_limit = p - _marker_height;
	if (_right_label_limit < 0) {
		_right_label_limit = 0;
	}

	if (!label_on_left ()) {
		setup_name_display ();
	}
}

void
Marker::set_has_scene_change (bool yn)
{
        _have_scene_change = yn;
        setup_name_display ();
}
                
/***********************************************************************/

TempoMarker::TempoMarker (PublicEditor& editor, ArdourCanvas::Container& parent, guint32 rgba, const string& text,
			  ARDOUR::TempoSection& temp)
	: Marker (editor, parent, rgba, text, Tempo, 0, false),
	  _tempo (temp)
{
	set_position (_tempo.frame());
	group->Event.connect (sigc::bind (sigc::mem_fun (editor, &PublicEditor::canvas_tempo_marker_event), group, this));
}

TempoMarker::~TempoMarker ()
{
}

/***********************************************************************/

MeterMarker::MeterMarker (PublicEditor& editor, ArdourCanvas::Container& parent, guint32 rgba, const string& text,
			  ARDOUR::MeterSection& m)
	: Marker (editor, parent, rgba, text, Meter, 0, false),
	  _meter (m)
{
	set_position (_meter.frame());
	group->Event.connect (sigc::bind (sigc::mem_fun (editor, &PublicEditor::canvas_meter_marker_event), group, this));
}

MeterMarker::~MeterMarker ()
{
}

