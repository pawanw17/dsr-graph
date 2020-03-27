// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*!
 * @file cadenaSubscriber.cpp
 * This file contains the implementation of the subscriber functions.
 *
 * This file was generated by the tool fastcdrgen.
 */

#include <fastrtps/participant/Participant.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/subscriber/Subscriber.h>
#include <fastrtps/attributes/SubscriberAttributes.h>
#include <fastrtps/transport/UDPv4TransportDescriptor.h>
#include <fastrtps/Domain.h>

#include "dsrsubscriber.h"

using namespace eprosima::fastrtps;
using namespace eprosima::fastrtps::rtps;

DSRSubscriber::DSRSubscriber() : mp_participant(nullptr), mp_subscriber(nullptr) {}

DSRSubscriber::~DSRSubscriber() 
{
    	//Domain::removeParticipant(mp_participant);
}

bool DSRSubscriber::init(eprosima::fastrtps::Participant *mp_participant_, const char* topicName, const char* topicDataType)
{
    mp_participant = mp_participant_;
    
    // Create Subscriber
    SubscriberAttributes Rparam;
    Rparam.topic.topicKind = NO_KEY;
    Rparam.topic.topicDataType = topicDataType; //Must be registered before the creation of the subscriber
    Rparam.topic.topicName = topicName;
    eprosima::fastrtps::rtps::Locator_t locator;
    IPLocator::setIPv4(locator, 239, 255, 0 , 1);
    locator.port = 7900;
    Rparam.multicastLocatorList.push_back(locator);
    Rparam.qos.m_reliability.kind = eprosima::fastrtps::RELIABLE_RELIABILITY_QOS;
    Rparam.historyMemoryPolicy = eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
    m_listener.participant_ID = mp_participant->getGuid();
    mp_subscriber = Domain::createSubscriber(mp_participant, Rparam, static_cast<SubscriberListener*>(&m_listener));
    if(mp_subscriber == nullptr)
        return false;
    return true;
}




///////////////////////////////////////////
/// Callbacks
///////////////////////////////////////////

void DSRSubscriber::SubListener::onSubscriptionMatched(Subscriber* sub, MatchingInfo& info)
{
    (void)sub;
    if (info.status == MATCHED_MATCHING)
    {
        n_matched++;
        std::cout << "Subscriber matched" << std::endl;
    }
    else
    {
        n_matched--;
        std::cout << "Subscriber unmatched" << std::endl;
    }
}

void DSRSubscriber::SubListener::onNewDataMessage(Subscriber* sub)
{
    // Take data
    DSRDelta st;
    std::cout << "participant_ID " << participant_ID  << std::endl;
    if(sub->takeNextData(&st, &m_info))
    {
        if(m_info.sampleKind == ALIVE)
            if( m_info.sample_identity.writer_guid().is_on_same_process_as(participant_ID) == false)
            {
                ++n_msg;
                std::cout << "Sample received, count=" << n_msg << " " << st.load().size() * 4 << " " << m_info.sample_identity.writer_guid() << std::endl;
            }
    }
    //fps.print(1000, st.load().size() * 4 * 8);
}

void DSRSubscriber::run()
{
    std::cout << "Waiting for Data, press Enter to stop the Subscriber. "<<std::endl;
   while (true);
}
