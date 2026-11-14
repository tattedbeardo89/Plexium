// SPDX-License-Identifier: GPL-3.0-or-later
#include "check.h"
#include "app/session.h"
#include "fixtures/plex_server.h"
#include <filesystem>
#include <iostream>
using namespace plexium;
struct EpisodeEngine:PlaybackPort{
 int opens=0,stops=0;int64_t at=0;PlaybackObservation observation{true,true,false,false,false,false};std::vector<std::string> events;
 bool open(const std::string&,int64_t position)override{++opens;at=position;events.push_back("open");observation={true,true,false,false,false,false};return true;}
 void stop()override{++stops;events.push_back("stop");}int64_t position()const override{return at;}void seek(int64_t position)override{at=position;}PlaybackObservation observe()const override{return observation;}
};
int main(){
 char folder[]="/tmp/plexium-episodes-XXXXXX";TEST_REQUIRE(mkdtemp(folder));std::string root=folder;TEST_REQUIRE(plex::save_credentials(root+"/account.json","fixture-device","account-token"));
 Fixture fixture;EpisodeEngine engine;double clock=1000;
 auto episode=[&](const char *id,int64_t offset=0,bool watched=false){auto value=fixture.metadata;value["ratingKey"]=id;value["type"]="episode";value["title"]=std::string("Episode ")+id;value["grandparentRatingKey"]="10";value["viewOffset"]=offset;value["viewCount"]=watched?1:0;value["Media"][0]["Part"][0]["key"]=std::string("/library/parts/")+id+"/file.mp4";return value;};
 auto first=episode("1",1000),second=episode("2",0,true),third=episode("3",4000),fourth=episode("4");first["Marker"]=plex::Json::array({{{"type","intro"},{"startTimeOffset",1000},{"endTimeOffset",5000}}});
 auto transport=[&](const plex::Request&q){
  auto reply=[](plex::Json value){return plex::Reply{200,value.dump(),""};};
  if(q.url.find("/allLeaves")!=std::string::npos)return reply({{"MediaContainer",{{"totalSize",4},{"Metadata",plex::Json::array({first,second,third,fourth})}}}});
  for(const auto *id:{"1","3","4"})if(q.url.find(std::string("/library/metadata/")+id+"?")!=std::string::npos)return reply({{"MediaContainer",{{"Metadata",plex::Json::array({std::string(id)=="1"?first:std::string(id)=="3"?third:fourth})}}}});
  if(q.url.find("/library/metadata/10")!=std::string::npos)return reply({{"MediaContainer",{{"Metadata",plex::Json::array()}}}});
  return fixture(q);
 };
 Session session(transport,root,engine,[&]{return clock;});auto send=[&](Action action,std::string id=""){return session.dispatch({action,id,{}});};
 send(Action::Restore);send(Action::Profile,"1");auto model=send(Action::Details,"1");TEST_REQUIRE(model.detail.markers.size()==1);
 send(Action::Play);engine.at=2000;send(Action::Tick);send(Action::SkipMarker);TEST_REQUIRE(engine.at==5000);
 engine.at=90000;engine.observation.ended=true;engine.observation.playing=false;model=send(Action::Tick);TEST_REQUIRE(model.screen==Screen::Details&&model.upcoming.size()==1&&model.upcoming[0].id=="3"&&model.autoplay_remaining==10);
 clock+=4;model=send(Action::Tick);TEST_REQUIRE(model.autoplay_remaining==6);send(Action::CancelNext);clock+=20;send(Action::Tick);TEST_REQUIRE(engine.opens==1);
 model=send(Action::PlayNext);TEST_REQUIRE(model.detail.id=="3"&&engine.at==4000&&engine.opens==2);TEST_REQUIRE(engine.events.size()>=3&&engine.events[engine.events.size()-2]=="stop");
 send(Action::Tick);engine.at=90000;engine.observation.ended=true;engine.observation.playing=false;model=send(Action::Tick);TEST_REQUIRE(model.upcoming.size()==1&&model.upcoming[0].id=="4");
 clock+=11;model=send(Action::Tick);TEST_REQUIRE(model.detail.id=="4"&&engine.opens==3&&model.playback_state==PlaybackState::Opening);
 send(Action::Tick);send(Action::AutoPlay);engine.observation.ended=true;engine.observation.playing=false;model=send(Action::Tick);TEST_REQUIRE(model.autoplay_remaining==0&&model.upcoming.empty());
 // Navigation must not leave an invisible countdown running behind Settings.
 send(Action::AutoPlay);send(Action::Details,"1");send(Action::Play);send(Action::Tick);engine.observation.ended=true;engine.observation.playing=false;
 model=send(Action::Tick);TEST_REQUIRE(model.autoplay_remaining==10);auto opens=engine.opens;
 model=send(Action::Settings);TEST_REQUIRE(model.screen==Screen::Settings&&model.autoplay_remaining==0);clock+=20;send(Action::Tick);TEST_REQUIRE(engine.opens==opens);
 model=send(Action::Back);TEST_REQUIRE(model.screen==Screen::Details&&model.autoplay_remaining==0);send(Action::Tick);TEST_REQUIRE(engine.opens==opens);
 send(Action::SignOut);std::filesystem::remove_all(root);std::cout<<"PASS: Plex episode order, watched skip, intro marker, end detection, countdown/cancel, fresh next resume and prior-player cleanup\n";
}
