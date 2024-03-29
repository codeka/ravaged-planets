#pragma once
/*
#include "../../../Framework/gui/window.h"
#include "../../../Framework/gui/default_button.h"
#include "../../../Framework/gui/wait_animation.h"
#include "../../session/session.h"

namespace CEGUI {
	class Editbox;
	class EventArgs;
}

namespace ww {

	// The login box is always visible on the title Screen. It's in the top-right
	// and lets you login/logout. We also handle auto-login, if that's configured.
	class login_box_window : public fw::gui::window
	{
	private:
		CEGUI::Editbox *_username;
		CEGUI::Editbox *_password;
		fw::gui::wait_animation _wait_anim;
		boost::signals::connection sig_session_state_changed_;

		float _reset_time;
		int _desired_height;

		void on_session_state_changed(session::session_state new_state);

		bool login_clicked(CEGUI::EventArgs const &e);
	public:
		login_box_window();
		virtual ~login_box_window();

		virtual void initialize();
		virtual void show();
		virtual void hide();
		virtual void update();
	};

	extern login_box_window *login_box;

}
*/
