#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <print>

#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Area : public Gtk::DrawingArea
{
	public:
		Area(Glib::RefPtr<Gdk::Pixbuf> i)
		{
		
			image = i;
			set_draw_func( sigc::mem_fun(*this, &Area::on_draw));
		}
		virtual ~Area() = default;

	protected:
		void on_draw (const Cairo::RefPtr< Cairo::Context>& cr, int w, int h)
		{
			if (!image) return;
			Gdk::Cairo::set_source_pixbuf(cr, image, 100, 100);
			
			cr->paint();
		}
		Glib::RefPtr<Gdk::Pixbuf> image;
};

class Window : public Gtk::Window
{
	public:
		Window (Glib::RefPtr<Gdk::Pixbuf> i)
			: area(i)
		{
			set_title("megacute");
			set_default_size(400, 400);
			set_child(area);
		}

	protected:
		Area area;
		//member widget
};

void activated ()
{
}

int main (int argc, char** argv)
{

	auto app = Gtk::Application::create("org.gtkmm.examples.base");

	const std::string str = argc > 1 ? argv[1] : "get_files/thumbnail";
	const std::string key = getenv("HYDRUS_KEY");

	cpr::Response r = cpr::Get( cpr::Url{"http://localhost:45869/" + str},
		cpr::Parameters{{"Hydrus-Client-API-Access-Key", key}, 
				{"file_id", "114169906"}
		},
		cpr::Header{{"Content-Type", "application/json"}});

	std::string raw = r.text;

	auto loader = Gdk::PixbufLoader::create();
	loader->write(reinterpret_cast<const guint8*>(r.text.data()), r.text.size());
	loader->close();

	auto builder = Gtk::Builder::create_from_file("../megacute.xml.ui");
	auto imageWidget = builder->get_widget<Gtk::Image>("image");
	auto window 	 = builder->get_widget<Gtk::Window>("window");

	const auto activate = [&](void)
	{
		app->add_window(*window);
		window->set_visible(true);
		imageWidget->set( loader->get_pixbuf());
	};

	app->signal_activate().connect(activate);
	app->run(argc, argv);
	
}

