//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "entity_attribute.h"
#include "../../framework/logging.h"

namespace ent {

	entity_attribute::entity_attribute()
	{
	}

	entity_attribute::entity_attribute(std::string name, boost::any value)
		: _name(name), _value(value)
	{
	}

	entity_attribute::entity_attribute(entity_attribute const &copy)
		: _name(copy._name), _value(copy._value)
	{
	}

	entity_attribute::~entity_attribute()
	{
	}

	entity_attribute &entity_attribute::operator =(entity_attribute const &copy)
	{
		_name = copy._name;
		_value = copy._value;
		// note: we don't copy the signal
		return (*this);
	}

	void entity_attribute::set_value(boost::any value)
	{
		if(_value.type() != value.type())
		{
			fw::debug << boost::format("WARN: cannot set value of type %1% to value of type %2%")
				% _value.type().name() % value.type().name() << std::endl;
			return;
		}

		_value = value;
		sig_value_changed(_name, _value);
	}
}
