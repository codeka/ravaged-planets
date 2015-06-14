//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "login_box_window.h"
#include "../../session/session.h"
#include "../../../framework/gui/cegui.h"
#include "../../../framework/gui/tab_navigation.h"
#include "../../../framework/framework.h"
#include "../../../framework/lang.h"
#include "../../../framework/timer.h"
#include "../../../framework/logging.h"

#include <CEGUIEventArgs.h>
#include <CEGUISubscriberSlot.h>
#include <CEGUIWindow.h>
#include <elements/CEGUIEditbox.h>
#include <elements/CEGUIPushButton.h>
#include <elements/CEGUICheckbox.h>

// the login box animates between two different heights, depending on it's state
const int big_height = 132;
const int small_height = 40;

namespace ww {

	login_box_window::login_box_window()
		: fw::gui::window("LoginBox"), _desired_height(big_height), _reset_time(0)
	{
	}

	login_box_window::~login_box_window()
	{
	}

	void login_box_window::initialise()
	{
		fw::gui::window::initialise();

		_username = get_child<CEGUI::Editbox>("LoginBox/Username");
		_navigation->add_widget(_username);
		_defbtn->add_editbox(_username);
		_password = get_child<CEGUI::Editbox>("LoginBox/Password");
		_navigation->add_widget(_password);
		_defbtn->add_editbox(_password);

		CEGUI::Checkbox *chkbx = get_child<CEGUI::Checkbox>("LoginBox/RememberMe");
		_navigation->add_widget(chkbx);

		CEGUI::Window *btn = get_child("LoginBox/Login");
		subscribe(btn, CEGUI::PushButton::EventClicked,
			CEGUI::SubscriberSlot(&login_box_window::login_clicked, this));
		_defbtn->set_button(btn);

		CEGUI::Window *ctrl = get_child("LoginBox/MessageIcon");
		_wait_anim.initialise(ctrl, 24, 24, fw::gui::wait_animation::dots, 8);
		_wait_anim.set_enabled(false);

		get_child("LoginBox/MessageControls")->setVisible(false);
	}

	void login_box_window::show()
	{
		fw::gui::window::show();

		_sig_session_state_changed = session::get_instance()->sig_state_changed.connect(
			boost::bind(&login_box_window::on_session_state_changed, this, _1));
	}

	void login_box_window::hide()
	{
		fw::gui::window::hide();

		_sig_session_state_changed.disconnect();
	}

	void login_box_window::on_session_state_changed(session::session_state new_state)
	{
		CEGUI::Window *msg_ctrls = get_child("LoginBox/MessageControls");
		CEGUI::Window *login_ctrls = get_child("LoginBox/LoginControls");

		CEGUI::Window *msg_text = get_child("LoginBox/MessageText");
		CEGUI::Window *msg_icon = get_child("LoginBox/MessageIcon");

		switch(new_state)
		{
		case session::logging_in:
			msg_ctrls->setVisible(true);
			login_ctrls->setVisible(false);
			fw::gui::set_text(msg_text, fw::text("please-wait.logging-in"));
			msg_icon->setVisible(true);
			_wait_anim.set_enabled(true);
			_desired_height = small_height;
			break;

		case session::logged_in:
			{
				std::string s = (boost::format(fw::text("title.login.logged-in")) % session::get_instance()->get_user_name()).str();
				fw::gui::set_text(msg_text, s);
			}
			msg_icon->setVisible(false);
			_desired_height = small_height;
			break;

		case session::in_error:
			_reset_time = 3.0f;
			fw::gui::set_text(msg_text, fw::text("title.login.failed"));
			_wait_anim.set_enabled(false);
			msg_icon->setProperty("Image", "set:elements image:ErrorIcon");
			break;
		}
	}

	bool login_box_window::login_clicked(CEGUI::EventArgs const &)
	{
		std::string username = _username->getText().c_str();
		std::string password = _password->getText().c_str();

		// if you haven't entered the username or password, don't do anything
		// todo: give some indication of an error...
		if (username == "" || password == "")
			return true;

		ww::session::get_instance()->login(username, password);

		// we just reset the username/password text - the update method will query
		// the session to actually hide things and display progress messages, etc
		_username->setText("");
		_password->setText("");

		return true;
	}

	void login_box_window::update()
	{
		fw::gui::window::update();

		_wait_anim.update();

		float dt = fw::framework::get_instance()->get_timer()->get_frame_time();
		float curr_height = get_window()->getPixelSize().d_height;
		if (curr_height != _desired_height)
		{
			float diff = (_desired_height - curr_height) * 5.0f;

			float new_height = curr_height + (diff * dt);
			get_window()->setHeight(CEGUI::UDim(0.0f, new_height));
		}

		if (_reset_time > -0.5f && _reset_time < 0.0f)
		{
			CEGUI::Window *msg_ctrls = get_child("LoginBox/MessageControls");
			CEGUI::Window *login_ctrls = get_child("LoginBox/LoginControls");

			_reset_time = -1.0f;
			msg_ctrls->setVisible(false);
			login_ctrls->setVisible(true);
			_desired_height = big_height;
		}
		else
		{
			_reset_time -= dt;
		}
	}

}