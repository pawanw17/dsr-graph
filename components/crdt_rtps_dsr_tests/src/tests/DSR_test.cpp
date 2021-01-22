//
// Created by juancarlos on 7/5/20.
//

#include "DSR_test.h"


// pick a random id from the list of new ones
int DSR_test::removeID()
{
    std::unique_lock<std::shared_mutex>  lock(mut);
    if(created_nodes.empty())
        return -1;
    auto node_randomizer = std::uniform_int_distribution(0, (int)created_nodes.size() - 1);
    int l = node_randomizer(mt);
    int val = created_nodes.at(l);
    created_nodes.erase(created_nodes.begin() + l);
    return val;
}

// pick a random id from the list of new ones without removing
int DSR_test::getID()
{
    std::unique_lock<std::shared_mutex>  lock(mut);
    if(created_nodes.empty())
        return -1;
    auto node_randomizer = std::uniform_int_distribution(0, (int)created_nodes.size() - 1);
    int l = node_randomizer(mt);
    int val = created_nodes.at(l);
    return val;
}

std::pair<uint64_t, uint64_t> DSR_test::removeEdgeIDs(){
    std::unique_lock<std::shared_mutex>  lock(mut);
    if(created_edges.empty())
        return { -1, -1 };
    auto edge_randomizer = std::uniform_int_distribution(0, (int)created_edges.size()-1);
    int l = edge_randomizer(mt);
    auto val = created_edges.at(l);
    created_edges.erase(created_edges.begin()+l);
    return val;
}


std::pair<uint64_t, uint64_t> DSR_test::getEdgeIDs(){
    std::unique_lock<std::shared_mutex>  lock(mut);
    if(created_edges.empty())
        return { -1, -1 };
    auto edge_randomizer = std::uniform_int_distribution(0, (int)created_edges.size()-1);
    int l = edge_randomizer(mt);
    auto val = created_edges.at(l);
    return val;
}


void DSR_test::addEdgeIDs(uint64_t from, uint64_t to){
    std::unique_lock<std::shared_mutex>  lock(mut);
    created_edges.emplace_back(std::make_pair(from, to));
}