//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#include "stdafx.h"
#include "entity_factory.h"
#include "orderable_component.h"
#include "../simulation/commands.h"
#include "../simulation/orders.h"
#include "../simulation/simulation_thread.h"

namespace ent {

	// register the orderable component with the entity_factory
	ENT_COMPONENT_REGISTER("orderable", orderable_component);

	orderable_component::orderable_component()
		: _order_pending(false)
	{
	}

	orderable_component::~orderable_component()
	{
	}

	void orderable_component::execute_order(shared_ptr<ww::order> const &order)
	{
		_curr_order = order;
		_order_pending = false;

		_curr_order->begin(_entity);
	}

	void orderable_component::update(float dt)
	{
		if (_curr_order)
		{
			// if we've currently got an order, update it and check whether it's finished
			_curr_order->update(dt);
			if (_curr_order->is_complete())
			{
				_curr_order.reset();
			}
		}

		if (!_curr_order && !_order_pending)
		{
			// if we don't have an order, check whether any has been queued since the last
			// update and execute a command to start it off
			if (_orders.size() > 0)
			{
				shared_ptr<ww::order> next_order = _orders.front();
				_orders.pop();

				shared_ptr<ww::order_command> cmd(ww::create_command<ww::order_command>());
				cmd->order = next_order;
				cmd->entity = shared_ptr<ent::entity>(_entity)->get_id();
				ww::simulation_thread::get_instance()->post_command(cmd);

				// mark that we've got an order pending, it'll take a few frames
				// for it to actually come back to us
				_order_pending = true;
			}
		}
	}

	void orderable_component::issue_order(shared_ptr<ww::order> const &order)
	{
		_orders.push(order);
	}

	int orderable_component::get_order_count() const
	{
		return _orders.size() + (_curr_order ? 1 : 0);
	}

	shared_ptr<ww::order> orderable_component::get_current_order() const
	{
		return _curr_order;
	}

}