/*
#include "save_map.h"
#include "../editor_screen.h"
#include "../editor_world.h"
#include "../world_writer.h"
#include "../../framework/framework.h"
#include "../../framework/texture.h"
#include "../../framework/gui/cegui.h"
#include "../../framework/misc.h"
#include "../../game/world/world.h"

#include <CEGUIWindow.h>
#include <CEGUIImagesetManager.h>
#include <CEGUIImageset.h>
#include <elements/CEGUIPushButton.h>
#include <elements/CEGUIEditbox.h>
#include <RendererModules/Direct3D9/CEGUIDirect3D9Renderer.h>
#include <RendererModules/Direct3D9/CEGUIDirect3D9Texture.h>

namespace ed {

	save_map_window *save_map = 0;

	save_map_window::save_map_window()
		: window("SaveMap")
	{
	}

	save_map_window::~save_map_window()
	{
	}

	void save_map_window::initialise()
	{
		window::initialise();

		subscribe("SaveMap/Save", CEGUI::PushButton::EventClicked,
			CEGUI::SubscriberSlot(&save_map_window::save_clicked, this));
		subscribe("SaveMap/Cancel", CEGUI::PushButton::EventClicked,
			CEGUI::SubscriberSlot(&save_map_window::cancel_clicked, this));

		_name = get_child<CEGUI::Editbox>("SaveMap/Name");
		_description = get_child<CEGUI::Editbox>("SaveMap/Description");
		_author = get_child<CEGUI::Editbox>("SaveMap/Author");
	}

	// when we go to show, we have to update our controls with what we currently know about
	// the map we're editing.
	void save_map_window::show()
	{
		window::show();

		auto world = dynamic_cast<editor_world *>(ww::world::get_instance());

		_name->setText(world->get_name());
		_description->setText(world->get_description());
		if (world->get_author() == "")
		{
			_author->setText(fw::get_local_username());
		}
		else
		{
			_author->setText(world->get_author());
		}
		update_screenshot();
	}

	// updates the screenshot that we're displaying whenever it changes.
	void save_map_window::update_screenshot()
	{
		auto world = dynamic_cast<editor_world *>(ww::world::get_instance());
		if (world->get_screenshot().get_width() == 0)
			return;

		fw::bitmap const &bmp = world->get_screenshot();

		CEGUI::ImagesetManager &imgset_mgr = CEGUI::ImagesetManager::getSingleton();

		// copy the bitmap into a texture, ready to be used by CEGUI
		shared_ptr<fw::texture> texture(new fw::texture());
		texture->create(bmp.get_width(), bmp.get_height());
		fw::blit(bmp, *texture);

		// create a CEGUI texture from the texture
		CEGUI::Direct3D9Renderer *renderer = dynamic_cast<CEGUI::Direct3D9Renderer *>(CEGUI::System::getSingleton().getRenderer());
		CEGUI::Texture &cegui_texture = renderer->createTexture(texture->get_d3dtexture());

		// next, check if there's an imageset with the name "NewGameScreenshot" and destroy it
		// if there is (we'll create a new one right away)
		if (imgset_mgr.isDefined("SaveMapScreenshot"))
			imgset_mgr.destroy("SaveMapScreenshot");

		// then, create an imageset based on our bitmap
		CEGUI::Imageset &imgset = imgset_mgr.create("SaveMapScreenshot", cegui_texture);
		imgset.defineImage("Screenshot", CEGUI::Point(0, 0),
			CEGUI::Size(static_cast<float>(bmp.get_width()), static_cast<float>(bmp.get_height())),
			CEGUI::Point(0, 0));

		CEGUI::Window *wnd = get_child("SaveMap/Screenshot");
		wnd->setProperty("Image", "set:SaveMapScreenshot image:Screenshot");
	}

	bool save_map_window::save_clicked(CEGUI::EventArgs const &)
	{
		std::string name = _name->getText().c_str();

		auto world = dynamic_cast<editor_world *>(ww::world::get_instance());
		world->set_name(name);
		world->set_description(_description->getText().c_str());
		world->set_author(_author->getText().c_str());

		world_writer writer(world);
		writer.write(name);

		hide();
		return true;
	}

	bool save_map_window::cancel_clicked(CEGUI::EventArgs const &)
	{
		hide();
		return true;
	}
}
*/
