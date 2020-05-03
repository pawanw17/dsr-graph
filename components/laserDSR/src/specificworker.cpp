/*
 *    Copyright (C)2020 by YOUR NAME HERE
 *
 *    This file is part of RoboComp
 *
 *    RoboComp is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    RoboComp is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RoboComp.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "specificworker.h"

/**
* \brief Default constructor
*/
SpecificWorker::SpecificWorker(TuplePrx tprx) : GenericWorker(tprx)
{

}

/**
* \brief Default destructor
*/
SpecificWorker::~SpecificWorker()
{
	std::cout << "Destroying SpecificWorker" << std::endl;
	G->write_to_json_file("/home/robocomp/robocomp/components/dsr-graph/etc/"+agent_name+".json");
	G.reset();
}

bool SpecificWorker::setParams(RoboCompCommonBehavior::ParameterList params)
{
	agent_name = params["agent_name"].value;
    agent_id = stoi(params["agent_id"].value);
	read_dsr = params["read_dsr"].value == "true";
    dsr_input_file = params["dsr_input_file"].value;

	return true;
}

void SpecificWorker::initialize(int period)
{
	std::cout << "Initialize worker" << std::endl;

	// create graph
    G = std::make_shared<CRDT::CRDTGraph>(0, agent_name, agent_id); // Init nodes
	// read graph content from file
    if(read_dsr)
    {
        G->read_from_json_file(dsr_input_file);
        G->start_fullgraph_server_thread();     // to receive requests form othe starting agents
        G->start_subscription_thread(true);     // regular subscription to deltas
    }
    else
    {
        G->start_subscription_thread(true);     // regular subscription to deltas
        G->start_fullgraph_request_thread();    // for agents that want to request the graph for other agent
    }
	std::cout<< __FUNCTION__ << "Graph loaded" << std::endl;  

/*************** UNCOMMENT IF NEEDED **********/
	// GraphViewer creation
    graph_viewer = std::make_unique<DSR::GraphViewer>(std::shared_ptr<SpecificWorker>(this));
    setWindowTitle( agent_name.c_str() );

	this->Period = period;
	timer.start(Period);

}

void SpecificWorker::compute()
{
    updateLaser();
}


void SpecificWorker::updateLaser()
{
	RoboCompLaser::TLaserData ldata;
    try
	{
		ldata = laser_proxy->getLaserData();
	}
	catch(const Ice::Exception &e)
	{ std::cout << "Error reading laser " << e << std::endl;	}
    // transform data
    std::vector<float> dists;
    std::transform(ldata.begin(), ldata.end(), std::back_inserter(dists), [](const auto &l) { return l.dist; });
    std::vector<float> angles;
    std::transform(ldata.begin(), ldata.end(), std::back_inserter(angles), [](const auto &l) { return l.angle; });

	// update laser in DSR
	auto node = G->get_node("hokuyo_base");
	if (node.id() != -1)
	{
		auto &nat = node.attrs();
		G->add_attrib(nat, "laser_data_dists", dists);
        G->add_attrib(nat, "laser_data_angles", angles);
		G->add_attrib(nat, "pos_x", nat["pos_x"].value());
		G->add_attrib(nat, "pos_y", nat["pos_y"].value());
		auto r = G->insert_or_assign_node(node);
		if (r)
			std::cout << "Update node robot: "<<node.id()<<" with laser data"<<std::endl;
	}
	else  //node has to be created
	{
		try
		{
			int new_id = dsrgetid_proxy->getID();
            node.type("robot");
			node.id(new_id);
			node.agent_id(agent_id);
			node.name("robot");
			std::map<string, AttribValue> attrs;
			G->add_attrib(attrs, "laser_data_dists", dists);
            G->add_attrib(attrs, "laser_data_angles", angles);
			node.attrs(attrs);
			auto r = G->insert_or_assign_node(node);
			if (r)
				std::cout << "New node robot: "<<node.id()<<std::endl;
		}
		catch(const std::exception& e)
		{
			std::cerr << "Error creating new node robot " << e.what() << '\n';
		}
	}
}

