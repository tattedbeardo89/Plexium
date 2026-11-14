// SPDX-License-Identifier: GPL-3.0-or-later
#include "plex/live_session.h"
#include "plex/guide.h"
#include "plex/home.h"
#include "plex/streaming.h"
#include "runtime/private_json.h"
#include <cstdlib>
#include <iostream>
#include <set>
using namespace plex;
int main(){
 Json config=Json::object();const char *file=getenv("PLEXIUM_TEST_CONFIG");
 if(file&&*file){if(!plexium::read_private_json(file,config,32768)){std::cerr<<"FAIL: private PMS test configuration is unreadable or unsafe\n";return 2;}}
 for(const auto &field:std::vector<std::pair<const char*,const char*>>{{"server_url","PLEXIUM_TEST_PMS_URL"},{"server_token","PLEXIUM_TEST_PMS_TOKEN"},{"machine_id","PLEXIUM_TEST_MACHINE_ID"},{"account_token","PLEXIUM_TEST_ACCOUNT_TOKEN"},{"profile_id","PLEXIUM_TEST_PROFILE_ID"},{"media_id","PLEXIUM_TEST_MEDIA_ID"}})if(const char *value=getenv(field.second);value&&*value)config[field.first]=value;
 auto base=text(config,"server_url"),token=text(config,"server_token"),machine=text(config,"machine_id"),profile=text(config,"profile_id"),media=text(config,"media_id");
 if(base.empty()&&token.empty()&&machine.empty()){std::cout<<"SKIP REAL-PMS: no authorized server configured; no network requests made\n";return 77;}
 if(!valid_base(base,false)||token.empty()||token.size()>4096||!live_identifier(machine)||(!media.empty()&&!stream_id(media))){std::cerr<<"FAIL: incomplete or invalid HTTPS PMS test configuration\n";return 2;}
 bool failed=false;auto check=[&](const char *name,bool passed){std::cout<<(passed?"PASS REAL-PMS: ":"FAIL REAL-PMS: ")<<name<<'\n';failed|=!passed;return passed;};auto skip=[](const char *name){std::cout<<"SKIP REAL-PMS: "<<name<<'\n';};
 Client client(curl_transport(),"plexium-authorized-integration");auto bounded=client.bounded(1500,4000);
 auto account=text(config,"account_token");
 if(!account.empty()){
  if(!check("account authorization",bool(bounded.validate_account(account))))return 1;
  auto users=bounded.home_users(account);if(!check("Plex Home discovery",bool(users)))return 1;
  auto resources=bounded.resources(account);check("authorized server discovery",bool(resources));
  if(!profile.empty()){
   auto profiles=plex::profiles(users.data);auto selected=std::find_if(profiles.begin(),profiles.end(),[&](const auto&p){return p.id==profile;});
   if(!check("dedicated test profile membership",selected!=profiles.end()))return 1;
   auto switched=bounded.switch_home(account,profile,text(config,"pin"));if(!check("dedicated profile authorization",bool(switched)))return 1;
   auto active=text(switched.data,"authenticationToken",text(switched.data,"authToken"));auto discovered=bounded.resources(active);
   if(!check("profile server discovery",bool(discovered)))return 1;
   auto servers=plex::servers(discovered.data);auto selected_server=std::find_if(servers.begin(),servers.end(),[&](const auto&s){return s.id==machine;});
   if(!check("profile server access",selected_server!=servers.end()&&!selected_server->token.empty()))return 1;token=selected_server->token;
  }else skip("profile switch (no dedicated profile configured)");
 }else skip("plex.tv account/Home/discovery (no account token configured)");
 if(!check("server identity",bool(bounded.identity(base,token,machine))))return 1;
 check("library authorization",bool(bounded.sections(base,token)));
 auto channels=live_channels(bounded,base,token);check("DVR discovery and channel lineup",bool(channels));
 if(channels&&channels.data["channels"].size()>0)check("Plex guide retrieval",bool(live_guide(bounded,base,token,channels.data["channels"][0],guide_date(std::time(nullptr)))));else skip("guide retrieval (no accessible DVR channels)");
 check("read-only recording schedule",bool(live_recordings(bounded,base,token)));
 if(!media.empty()){
  auto response=bounded.browse(base,token,"/library/metadata/"+encode(media)+"?includeMarkers=1",0,1);auto items=response?plex::items(response.data):std::vector<Item>{};
  if(!check("dedicated fixture metadata",response&&items.size()==1&&items[0].id==media))return 1;
  auto &item=items[0];check("resume metadata",item.offset>=0&&item.duration>=0);check("audio/subtitle enumeration",item.media.is_array());
  auto plan=negotiate_video(bounded,base,token,item,0,Quality::FHD,true,"plexium-readonly-decision",true);check("compatible playback decision (no playback start)",plan.playback.mode!=Mode::Unsupported);
  if(flag(config,"allow_stream_selection")&&!profile.empty()&&!account.empty()){
   auto tracks=media_tracks(item,0,false),subtitles=media_tracks(item,0,true);TrackSelection selection;for(const auto&t:tracks)if(t.selected)selection.audio=t.id;for(const auto&t:subtitles)if(t.selected)selection.subtitle=t.id;
   check("explicitly authorized current stream selection",bool(select_plex_streams(bounded,base,token,item,0,selection)));
  }else skip("stream selection writes (explicit authorization required)");
  if(flag(config,"allow_watch_write")&&!profile.empty()&&!account.empty())check("explicitly authorized same-position timeline",bool(bounded.timeline(base,token,item,"paused",item.offset,"plexium-authorized-watch-test")));else skip("watch-state writes (explicit authorization required)");
 }else skip("media/negotiation/resume tests (no dedicated media configured)");
 skip("DVR acquisition/keepalive/release (consumer lifecycle remains gated)");
 std::cout<<"REAL-PMS test output contains status labels only; no addresses, tokens or media metadata.\n";return failed?1:0;
}
