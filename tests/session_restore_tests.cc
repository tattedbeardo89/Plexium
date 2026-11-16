// SPDX-License-Identifier: GPL-3.0-or-later
#include "check.h"
#include "app/session.h"
#include <filesystem>
#include <iostream>
using namespace plexium;
struct Player:PlaybackPort{bool open(const std::string&,int64_t)override{return false;}void stop()override{}};
int main(){
 char folder[]="/tmp/plexium-restore-XXXXXX";TEST_REQUIRE(mkdtemp(folder));std::string root=folder;
 TEST_REQUIRE(plex::save_credentials(root+"/device.json","fixture-device",""));
 TEST_REQUIRE(plex::save_credentials(root+"/account.json","fixture-device","account-token"));
 Player player;bool protected_user=false,removed=false,deny=false;std::string account_id="7";int switches=0;
 auto reply=[](plex::Json value){return plex::Reply{200,value.dump(),""};};
 auto transport=[&](const plex::Request&q)->plex::Reply{
  if(q.url=="https://plex.tv/api/v2/user")return reply({{"id",account_id}});
  if(q.url.find("/home/users/1/switch")!=std::string::npos){++switches;TEST_REQUIRE(q.method=="POST");if(deny)return {403,"{}",""};return reply({{"id",1},{"authenticationToken","profile-token"}});}
  if(q.url.find("/home/users")!=std::string::npos)return reply({{"users",removed?plex::Json::array():plex::Json::array({{{"id",1},{"title","Owner"},{"protected",protected_user}}})}});
  if(q.url.find("/api/v2/resources")!=std::string::npos){TEST_REQUIRE(q.headers.at("X-Plex-Token")=="profile-token");return reply(plex::Json::array({{{"name","Fixture PMS"},{"clientIdentifier","machine-1"},{"provides","server"},{"accessToken","server-token"},{"connections",plex::Json::array({{{"uri","https://fixture:32400"},{"local",true}}})}}}));}
  if(q.url.find("/identity")!=std::string::npos)return reply({{"MediaContainer",{{"machineIdentifier","machine-1"}}}});
  TEST_REQUIRE(q.url.rfind("https://fixture:32400/",0)==0);TEST_REQUIRE(q.headers.at("X-Plex-Token")=="server-token");
  return reply({{"MediaContainer",{{"size",0},{"Directory",plex::Json::array()},{"Hub",plex::Json::array()},{"Metadata",plex::Json::array()}}}});
 };
 {Session session(transport,root,player);TEST_REQUIRE(session.dispatch({Action::Restore,{},{}}).screen==Screen::Profiles);TEST_REQUIRE(session.dispatch({Action::Profile,"1",{}}).screen==Screen::Home);}
 std::string owner,profile;TEST_REQUIRE(plex::load_credentials(root+"/selected-profile.json",owner,profile)&&owner=="7"&&profile=="1");
 {Session restart(transport,root,player);auto model=restart.dispatch({Action::Restore,{},{}});TEST_REQUIRE(model.screen==Screen::Home&&model.profile_name=="Owner"&&model.server_name=="Fixture PMS");TEST_REQUIRE(restart.media_context().token=="server-token");TEST_REQUIRE(restart.dispatch({Action::Home,"profiles",{}}).screen==Screen::Profiles);}
 auto before=switches;protected_user=true;
 {Session restart(transport,root,player);TEST_REQUIRE(restart.dispatch({Action::Restore,{},{}}).screen==Screen::Profiles);TEST_REQUIRE(switches==before);auto pin=restart.dispatch({Action::Profile,"1",{}});TEST_REQUIRE(pin.screen==Screen::Pin&&restart.media_context().token.empty()&&pin.navigation.empty());TEST_REQUIRE(switches==before);}
 protected_user=false;account_id="8";
 {Session restart(transport,root,player);TEST_REQUIRE(restart.dispatch({Action::Restore,{},{}}).screen==Screen::Profiles);TEST_REQUIRE(switches==before);}
 account_id="7";removed=true;
 {Session restart(transport,root,player);TEST_REQUIRE(restart.dispatch({Action::Restore,{},{}}).screen==Screen::Profiles);TEST_REQUIRE(switches==before);}
 removed=false;deny=true;
 {Session restart(transport,root,player);auto model=restart.dispatch({Action::Restore,{},{}});TEST_REQUIRE(model.screen==Screen::Profiles&&model.navigation.empty()&&restart.media_context().token.empty());TEST_REQUIRE(!model.message.empty());restart.dispatch({Action::SignOut,{},{}});TEST_REQUIRE(!std::filesystem::exists(root+"/selected-profile.json"));}
 std::filesystem::remove_all(root);std::cout<<"PASS: account-scoped profile/server restore, fresh profile credentials, protected/removed/account-changed rejection, failed switch isolation, explicit picker and logout\n";
}
