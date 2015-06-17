//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#pragma once

namespace fw {
	class model;

	// this class is used to writer models out to .wwmesh files, ready to be read
	// back in by the model_reader class
	class model_writer
	{
	private:

	public:
		
		// writes the given model out to the given file
		void write(std::string const &filename, model &mdl);
		void write(std::string const &filename, shared_ptr<model> mdl);
	};

}