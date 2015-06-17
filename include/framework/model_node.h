//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#pragma once

#include "scenegraph.h"

namespace fw {
	class model_mesh;
	class graphics;

	// this is a specialization of the scenegraph node used by models. It basically
	// just contains a bit of extra info that we want to keep around to make loading/saving
	// them easier
	class model_node : public sg::node
	{
	protected:
		// renders the model node
		virtual void render(sg::scenegraph *sg);

		// called to set any additional parameters on the given effect
		virtual void setup_effect(shared_ptr<fw::effect> fx);

		// called by clone() to populate the clone
		virtual void populate_clone(shared_ptr<node> clone);
	public:
		model_node();
		virtual ~model_node();

		// the index into the array of meshes that we got our data from
		int mesh_index;

		// the name of this node, used to reference this node in other parts
		// of the model, usually (e.g. in bones)
		std::string node_name;

		// whether or not to render in wireframe
		bool wireframe;

		// the colour we'll use to draw transparents parts of the texture
		fw::colour colour;

		// the transform used to move the model into position in the world
		fw::matrix transform;

		// you can call this after setting mesh_index to set up the node
		void initialise(model &mdl);

		virtual shared_ptr<node> clone();
	};

}