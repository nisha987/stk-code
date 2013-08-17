//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2013 Glenn De Jonghe
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


#include "online/profile.hpp"

#include "online/profile_manager.hpp"
#include "online/http_manager.hpp"
#include "config/user_config.hpp"
#include "online/current_user.hpp"
#include "utils/log.hpp"
#include "utils/translation.hpp"

#include <sstream>
#include <stdlib.h>
#include <assert.h>

using namespace Online;

namespace Online{



    Profile::RelationInfo::RelationInfo(const irr::core::stringw & date, bool is_online, bool is_pending, bool is_asker)
    {
        m_date = date;
        Log::info("date","%s",m_date.c_str());
        m_is_online = is_online;
        m_is_pending = is_pending;
        m_is_asker = is_asker;
    }


    // ============================================================================
    Profile::Profile( const uint32_t           & userid,
                      const irr::core::stringw & username)
    {
        setState (S_READY);
        m_cache_bit = true;
        m_id = userid;
        m_is_current_user = (m_id == CurrentUser::get()->getUserID());
        m_username = username;
        m_has_fetched_friends = false;
        m_relation_info = NULL;
    }

    Profile::Profile(const XMLNode * xml, ConstructorType type)
    {
        m_relation_info = NULL;
        if(type == C_RELATION_INFO){
            std::string is_online_string("");
            xml->get("online", &is_online_string);
            bool is_online = is_online_string == "yes";
            irr::core::stringw date("");
            xml->get("date", &date);
            std::string is_pending_string("");
            xml->get("is_pending", &is_pending_string);
            bool is_pending = is_pending_string == "yes";
            bool is_asker(false);
            if(is_pending)
            {
                std::string is_asker_string("");
                xml->get("is_asker", &is_asker_string);
                is_asker = is_asker_string == "yes";
            }
            m_relation_info = new RelationInfo(date, is_online, is_pending, is_asker);
            xml = xml->getNode("user");
        }

        xml->get("id", &m_id);
        xml->get("user_name", &m_username);
        m_cache_bit = true;
        m_has_fetched_friends = false;
        m_is_current_user = (m_id == CurrentUser::get()->getUserID());
        setState (S_READY);
    }
    // ============================================================================
    Profile::~Profile()
    {
        delete m_relation_info;
    }

    // ============================================================================
    void Profile::fetchFriends()
    {
        assert(CurrentUser::get()->isRegisteredUser());
        if(m_has_fetched_friends)
            return;
        setState (S_FETCHING);
        requestFriendsList();
    }
    // ============================================================================


    void Profile::friendsListCallback(const XMLNode * input)
    {
        const XMLNode * friends_xml = input->getNode("friends");
        m_friends.clear();
        for (unsigned int i = 0; i < friends_xml->getNumNodes(); i++)
        {
            Profile * profile = new Profile(friends_xml->getNode(i), (m_is_current_user ? C_RELATION_INFO : C_DEFAULT));
            m_friends.push_back(profile->getID());
            ProfileManager::get()->addToCache(profile);
        }
        m_has_fetched_friends = true;
        Profile::setState (Profile::S_READY);
    }


    // ============================================================================

    void Profile::requestFriendsList()
    {
        FriendsListRequest * request = new FriendsListRequest();
        request->setURL((std::string)UserConfigParams::m_server_multiplayer + "client-user.php");
        request->setParameter("action",std::string("get-friends-list"));
        request->setParameter("token", CurrentUser::get()->getToken());
        request->setParameter("userid", CurrentUser::get()->getUserID());
        request->setParameter("visitingid", m_id);
        HTTPManager::get()->addRequest(request);
    }

    void Profile::FriendsListRequest::callback()
    {
        uint32_t user_id(0);
        m_result->get("visitingid", &user_id);
        assert(ProfileManager::get()->getProfileByID(user_id) != NULL);
        ProfileManager::get()->getProfileByID(user_id)->friendsListCallback(m_result);
    }

    // ============================================================================

    const std::vector<uint32_t> & Profile::getFriends()
    {
        assert (m_has_fetched_friends && getState() == S_READY);
        return m_friends;
    }
    // ============================================================================

} // namespace Online
