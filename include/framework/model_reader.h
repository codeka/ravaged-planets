//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#pragma once

namespace fw {
	class mode;

	// this class is used to read models in from .wwmesh files
	class model_reader
	{
	public:
		shared_ptr<model> read(boost::filesystem::path const &filename);
	};

}