/*
 *    Copyright (C)2018 by YOUR NAME HERE
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
SpecificWorker::SpecificWorker(TuplePrx tprx) : GenericWorker(tprx) {
}

/**
* \brief Default destructor
*/
SpecificWorker::~SpecificWorker() {
    std::cout << "Destroying SpecificWorker" << std::endl;
}

bool SpecificWorker::setParams(RoboCompCommonBehavior::ParameterList params) {
    agent_name = params["agent_name"].value;
    return true;
}

void SpecificWorker::initialize(int period) {
    std::cout << "Initialize worker" << std::endl;

    int argc = 0;
    char *argv[0];

    filter = "^(?!" + agent_name + "$).*$";
    work = false;

    // General topic update
    node = DataStorm::Node(argc, argv);
    topic = std::make_shared < DataStorm::Topic < std::string, RoboCompDSR::OrMap >> (node, "DSR");
    topic->setWriterDefaultConfig({ Ice::nullopt, Ice::nullopt, DataStorm::ClearHistoryPolicy::OnAdd });

    // No filter for this topic
    writer = std::make_shared < DataStorm::SingleKeyWriter < std::string, RoboCompDSR::OrMap >> (*topic.get(), "DSR");

    graph = std::make_shared<CRDT::CRDTNodes>(); // Init nodes

    full_graph = std::thread(&SpecificWorker::serveFullGraphThread, this); // Server sync
    newGraphRequestAndWait();  // Client sync
    read_thread = std::thread(&SpecificWorker::subscribeThread, this); // Reader thread

    timer.start(500);
}

// Subscribe thread
void SpecificWorker::subscribeThread() {
    DataStorm::Topic <std::string, RoboCompDSR::OrMap> topic(node, "DSR");
    topic.setReaderDefaultConfig({ Ice::nullopt, Ice::nullopt, DataStorm::ClearHistoryPolicy::OnAdd });
    auto reader = DataStorm::makeSingleKeyReader(topic, "DSR");
    std::cout << "starting reader" << std::endl;
    reader.waitForWriters(); // If someone is writting...
    while (true) {
        if (work)
            try {
                auto sample = reader.getNextUnread(); // Get sample
                std::cout << "Received: node " << sample.getValue() << " from " << sample.getKey() << std::endl;
//                std::string strId = to_string(sample.getValue().id);
//                auto contx = nodes.context().get(strId); // Get currently context
//                if (!contx.empty()) {
//                    if (contx.back().first <=
//                        sample.getValue().causalContext) { // Check currently context and received context
//                        nodes[strId].add(sample.getValue(), strId);
//                    } else
//                        std::cout << "* ------> Out of time. [CC:" << sample.getValue().causalContext
//                                  << "]. CC_actual: "
//                                  << contx.back().first << ". " << sample.getValue() << endl;
//                } else // Empty context = new value
//                    nodes[strId].add(sample.getValue(), strId);
            }
            catch (const std::exception &ex) { cerr << ex.what() << endl; }
    }
}


void SpecificWorker::compute() {

    if (work) {
        static int cont = 0, laps = 0;
        topic->waitForReaders();
        if (laps < LAPS + agent_name.back()-48) { // Laps = 5 - agent id (just for random)
            try {
                cont++;
                auto test = RoboCompDSR::Node{"foo_" + std::to_string(cont) + "," + std::to_string(laps) + "_from_" + agent_name, cont};
                graph->addNode(cont, test);
                auto test1 = RoboCompDSR::OrMap{graph->id(), graph->map(), graph->context()};
                writer->update(test1);
            }
            catch (const std::exception &ex) { cerr << __FUNCTION__ << " -> " << ex.what() << std::endl; }

            if (cont > NODES) {
                cont = 0;
                laps++;
                cout << "------------" << laps << endl;
            }

        } else {
            std::cout << "Nodes: " << nodes << std::endl;
            sleep(5);
        }
    }
}

void SpecificWorker::newGraphRequestAndWait() {
    std::cout << __FUNCTION__ << " Initiating request for full graph requests" << std::endl;

    DataStorm::Topic <std::string, G> topic_answer(node, "DSR_GRAPH_ANSWER");
    DataStorm::FilteredKeyReader <std::string, G> reader(topic_answer,
                                                         DataStorm::Filter<std::string>("_regex", filter.c_str()));
    topic_answer.setWriterDefaultConfig({ Ice::nullopt, Ice::nullopt, DataStorm::ClearHistoryPolicy::OnAdd });
    topic_answer.setReaderDefaultConfig({ Ice::nullopt, Ice::nullopt, DataStorm::ClearHistoryPolicy::OnAdd });

    DataStorm::Topic <std::string, RoboCompDSR::GraphRequest> topicR(node, "DSR_GRAPH_REQUEST");
    DataStorm::SingleKeyWriter <std::string, RoboCompDSR::GraphRequest> writer(topicR, agent_name,
                                                                               agent_name + " Full Graph Request");
    topicR.setWriterDefaultConfig({ Ice::nullopt, Ice::nullopt, DataStorm::ClearHistoryPolicy::OnAdd });
    topicR.setReaderDefaultConfig({ Ice::nullopt, Ice::nullopt, DataStorm::ClearHistoryPolicy::OnAdd });
    writer.add(RoboCompDSR::GraphRequest{agent_name});


    if (agent_name != "agent0") { //TODO: Check if i am first to arrive
        std::cout << __FUNCTION__ << " Wait for writers " << std::endl;
        auto sample = reader.getNextUnread();
        std::cout << __FUNCTION__ << " Samples received " << std::endl;
//        for (const auto &[k,v] : sample.getValue())
//            std::cout << k << ", "<<v <<std::endl;
        try {
            for (const auto &[k, v] : sample.getValue()) {
                std::string strId = to_string(k);
                nodes[strId].add(v, strId);
            }
        }
        catch (const std::exception &ex) { cerr << ex.what() << endl; }
    } else
        std::cout << __FUNCTION__ << " No writers. Im first." << std::endl;

//    copyIceGraphToDSRGraph(this->ice_graph);
    std::cout << __FUNCTION__ << " Finished uploading full graph" << std::endl;
    work = true;
}


// Thread for initial sync request
void SpecificWorker::serveFullGraphThread() {
    std::cout << __FUNCTION__ << " Entering thread to attend full graph requests" << std::endl;
    // create topic and filtered reader for new graph requests
    DataStorm::Topic <std::string, RoboCompDSR::GraphRequest> topic_graph_request(node, "DSR_GRAPH_REQUEST");
    DataStorm::FilteredKeyReader <std::string, RoboCompDSR::GraphRequest> new_graph_reader(topic_graph_request,
                                                                                           DataStorm::Filter<std::string>(
                                                                                                   "_regex",
                                                                                                   filter.c_str()));
    topic_graph_request.setWriterDefaultConfig({ Ice::nullopt, Ice::nullopt, DataStorm::ClearHistoryPolicy::OnAdd });
    topic_graph_request.setReaderDefaultConfig({ Ice::nullopt, Ice::nullopt, DataStorm::ClearHistoryPolicy::OnAdd });

    auto processSample = [this](auto sample) {
        if (work) {
            work = false;
            std::cout << sample.getValue().from << " asked for full graph" << std::endl;
            sleep(3);
            G ice_graph;
            DataStorm::Topic <std::string, G> topic_answer(node, "DSR_GRAPH_ANSWER");
            DataStorm::SingleKeyWriter <std::string, G> writer(topic_answer, agent_name, agent_name + " Full Graph Answer");
            topic_answer.setWriterDefaultConfig({ Ice::nullopt, Ice::nullopt, DataStorm::ClearHistoryPolicy::OnAdd });
            topic_answer.setReaderDefaultConfig({ Ice::nullopt, Ice::nullopt, DataStorm::ClearHistoryPolicy::OnAdd });

            auto map = nodes.getFullOrMap();
            auto context = map.second.second;
            cout << "-- Sync -- " << endl;
            for (auto node : map.second.first) {
                for (auto node_value : node.second.readAsList()) {
                    auto listContext = context.get();
                    node_value.second.causalContext = listContext.first.front().second;
                    node_value.second.dotCloud = listContext.second.front().second;
                    cout << node_value.second.id << ", " << node_value.second << endl;
                    ice_graph.insert(std::make_pair(node_value.second.id, node_value.second));

                }
            }
            writer.add(ice_graph);
            std::cout << "Full graph written from lambda" << std::endl;
            work = true;
        }
    };
    new_graph_reader.onSamples([processSample](const auto &samples) { for (const auto &s : samples) processSample(s); },
                               processSample);
    node.waitForShutdown();
}
