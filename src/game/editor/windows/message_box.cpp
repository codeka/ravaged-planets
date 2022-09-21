/*
#include "message_box.h"
#include "../../Framework/Framework.h"
#include "../../Framework/gui/cegui.h"

#include <CEGUIWindow.h>
#include <elements/CEGUIPushButton.h>

namespace ed {

	message_box_window *message_box = 0;

	message_box_window::message_box_window()
		: fw::gui::window("MessageBox")
	{
	}

	message_box_window::~message_box_window()
	{
	}

	void message_box_window::initialise()
	{
		window::initialise();

		subscribe("MessageBox/OK", CEGUI::PushButton::EventClicked,
			CEGUI::SubscriberSlot(&message_box_window::ok_clicked, this));
	}

	void message_box_window::show(std::string const &caption, std::string const &message)
	{
		window::show();

		get_child("MessageBox")->setText(caption.c_str());
		get_child("MessageBox/Text")->setText(message.c_str());
	}

	bool message_box_window::ok_clicked(CEGUI::EventArgs const &)
	{
		hide();
		return true;
	}

 }*/
