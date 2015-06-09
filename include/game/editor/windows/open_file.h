#pragma once

#include <framework/gui/window.h>

namespace ed {
/*
	// a generic "open file" dialog, which lets us open whatever file we want to open.
	class open_file_window : public fw::gui::window
	{
	public:
		typedef boost::function<void (open_file_window *)> file_selected_handler;

	private:
		fw::shell_folder _curr_folder;
		CEGUI::MultiColumnList *_file_list;
		CEGUI::Editbox *_filename;
		CEGUI::Editbox *_path;
		file_selected_handler _file_selected;

		bool ok_clicked(CEGUI::EventArgs const &e);
		bool cancel_clicked(CEGUI::EventArgs const &e);
		bool item_double_clicked(CEGUI::EventArgs const &e);
		bool item_selection_changed(CEGUI::EventArgs const &e);

		void clear_filelist();
		void refresh_filelist();
		void navigate_to_folder(fw::shell_folder const &new_folder);

	public:
		open_file_window();
		~open_file_window();

		virtual void initialise();
		virtual void show();
		virtual void hide();

		std::string get_full_path() const;

		// when the user clicks "OK" after they select a file, the function you
		// pass here will be called.
		void set_file_selected_handler(file_selected_handler fn) { _file_selected = fn; }
	};

	extern open_file_window *open_file;
*/
}
