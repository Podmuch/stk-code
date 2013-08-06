#include "network/protocols/game_events_protocol.hpp"

#include "karts/abstract_kart.hpp"
#include "items/item.hpp"
#include "items/item_manager.hpp"
#include "items/powerup.hpp"
#include "modes/world.hpp"
#include "network/network_manager.hpp"

GameEventsProtocol::GameEventsProtocol() : Protocol(NULL, PROTOCOL_GAME_EVENTS)
{
}

GameEventsProtocol::~GameEventsProtocol()
{
}

bool GameEventsProtocol::notifyEvent(Event* event)
{
    if (event->type != EVENT_TYPE_MESSAGE)
        return true;
    NetworkString data = event->data();
    if (data.size() < 5) // for token and type
    {
        Log::warn("GameEventsProtocol", "Too short message.");
        return true;
    }
    if ( (*event->peer)->getClientServerToken() != data.gui32())
    {
        Log::warn("GameEventsProtocol", "Bad token.");
        return true;
    }
    int8_t type = data.gui8(4);
    data.removeFront(5);
    switch (type)
    {
        case 0x01: // item picked
        {
            if (data.size() < 6)
            {
                Log::warn("GameEventsProtocol", "Too short message.");
                return true;
            }
            uint32_t item_id = data.gui32();
            uint8_t powerup_type = data.gui8(4);
            uint8_t kart_race_id = data.gui8(5);
            // now set the kart powerup
            AbstractKart* kart = World::getWorld()->getKart(
                NetworkManager::getInstance()->getGameSetup()->getProfile(kart_race_id)->world_kart_id);
            ItemManager::get()->collectedItem(
                ItemManager::get()->getItem(item_id),
                kart,
                powerup_type);
        }   break;
        default:
            Log::warn("GameEventsProtocol", "Unkown message type.");
            break;
    }
    return true;
}

void GameEventsProtocol::setup()
{
}

void GameEventsProtocol::update()
{
}

void GameEventsProtocol::collectedItem(Item* item, AbstractKart* kart)
{
    NetworkString ns;
    GameSetup* setup = NetworkManager::getInstance()->getGameSetup();
    assert(setup);
    const NetworkPlayerProfile* player_profile = setup->getProfile(kart->getIdent()); // use kart name

    std::vector<STKPeer*> peers = NetworkManager::getInstance()->getPeers();
    for (unsigned int i = 0; i < peers.size(); i++)
    {
        ns.ai32(peers[i]->getClientServerToken());
        // 0x01 : item picked : send item id, powerup type and kart race id
        ns.ai8(0x01).ai32(item->getItemId()).ai8((int)(kart->getPowerup()->getType())).ai8(player_profile->race_id); // send item,
        m_listener->sendMessage(this, peers[i], ns, true); // reliable
    }
}