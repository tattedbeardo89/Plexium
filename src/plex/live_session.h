// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "live_tv.h"
#include <functional>
namespace plex {
enum class LiveState {Idle,Resolving,Acquiring,Buffering,Playing,Stopping,Released,Error,Recovery};
struct LiveChannel {std::string id,dvr,title,number;};
struct LiveLease {std::string session,consumer,playlist;bool owned=false,uncertain=false,compatible=false;};
struct LiveAcquisition {Result result;LiveLease lease;};
struct LiveSnapshot {LiveState state=LiveState::Idle;LiveChannel current,previous;std::string message;bool mini_guide=false;};
// Transport operations established by PMS documentation or the referenced SDK.
// DELETE is SDK-confirmed only; consumer/shared-session semantics remain a gate.
class DvrRequests {
 Client &client;std::string base,token;
 bool authorized()const{return valid_base(base,false)&&!token.empty();}
 public:
 DvrRequests(Client &c,std::string b,std::string t):client(c),base(std::move(b)),token(std::move(t)){}
 Result tune(const LiveChannel &channel){
  if(!authorized()||!live_channel_id(channel.id)||channel.dvr.empty()||channel.dvr.size()>18||channel.dvr.find_first_not_of("0123456789")!=std::string::npos)return {{},"Invalid DVR channel",0};
  auto bounded=client.bounded(800,2000);return bounded.call("POST",base,"/livetv/dvrs/"+encode(channel.dvr)+"/channels/"+encode(channel.id)+"/tune",token);
 }
 Result inspect(const std::string &id){if(!authorized()||!live_identifier(id))return {{},"Invalid live session",0};auto bounded=client.bounded(500,1000);return bounded.call("GET",base,"/livetv/sessions/"+encode(id),token);}
 Result release_owned(const LiveLease &lease){
  if(!authorized()||!lease.owned||!live_identifier(lease.session))return {{},"Live session ownership is not established",0};
  auto bounded=client.bounded(500,1000);return bounded.call("DELETE",base,"/livetv/sessions/"+encode(lease.session),token);
 }
};
// No production implementation may opt in based solely on a runtime setting.
// A verified consumer acquisition/keepalive/release adapter must replace this.
class LiveProtocol {
 public:virtual ~LiveProtocol()=default;
 virtual bool validated()const{return false;}
 virtual LiveAcquisition acquire(const LiveChannel&){return {{{},"Plex DVR consumer acquisition and cleanup are not verified",0},{}};}
 virtual Result heartbeat(const LiveLease&){return {{},"Plex DVR heartbeat is not verified",0};}
 virtual Result release(const LiveLease&){return {{},"Plex DVR consumer release is not verified",0};}
};
class PmsLiveProtocol final:public LiveProtocol {};
inline bool live_playlist(const std::string &base,const std::string &url){
 if(!valid_base(base,false)||url.size()>4096||url.rfind(base+"/",0)!=0)return false;
 const auto path=url.substr(base.size()),route=path.substr(0,path.find('?'));
 if(route.rfind("/livetv/sessions/",0)!=0)return false;
 if(route.size()<5||route.compare(route.size()-5,5,".m3u8")!=0||path.find_first_of("\\\r\n#")!=std::string::npos||route.find('%')!=std::string::npos||route.find("..")!=std::string::npos)return false;
 for(unsigned char c:path)if(c<=32||c>=127)return false;
 return true;
}
struct LivePlayer {
 std::function<bool(const std::string&)> open;
 std::function<bool()> stop;
};
// One worker owns this controller, its protocol, lease and player operations.
// Failed/ambiguous acquisition and failed release never permit another tune.
class LiveTvSession {
 LiveProtocol &protocol;LivePlayer player;std::string base;LiveLease lease;
 double due=0;int failures=0;bool stopped=false,player_active=false,release_pending=false;LiveSnapshot view;
 bool cleanup(){
  view.state=LiveState::Stopping;release_pending=true;
  if(player_active&&!stopped){if(!player.stop||!player.stop()){view.state=LiveState::Recovery;view.message="Live player cleanup is pending; retry Stop.";return false;}stopped=true;player_active=false;}
  if(lease.uncertain){view.state=LiveState::Recovery;view.message="Live acquisition outcome is unknown; tuner ownership must be reconciled before retry.";return false;}
  if(lease.owned){auto r=protocol.release(lease);if(!r){view.state=LiveState::Recovery;view.message="Plex live session release failed; retry Stop before tuning.";return false;}}
  lease={};release_pending=false;view.state=LiveState::Released;view.mini_guide=false;return true;
 }
 public:
 LiveTvSession(LiveProtocol &p,LivePlayer v,std::string b):protocol(p),player(std::move(v)),base(std::move(b)){}
 const LiveSnapshot &snapshot()const{return view;}
 bool owns_resources()const{return player_active||lease.owned||lease.uncertain||view.state==LiveState::Buffering||view.state==LiveState::Playing;}
 bool start(const LiveChannel &channel,double now){
  if(!protocol.validated()){view.message="Live TV playback is disabled in this development build; guide browsing and recordings remain available.";return false;}
  if(!valid_base(base,false)||!live_channel_id(channel.id)||!live_identifier(channel.dvr)){view.message="Invalid authorized live channel.";return false;}
  if(owns_resources()&&!cleanup())return false;
  view.state=LiveState::Resolving;view.message.clear();view.state=LiveState::Acquiring;stopped=false;
  auto acquired=protocol.acquire(channel);lease=std::move(acquired.lease);
  if(!acquired.result||!lease.owned||!lease.compatible||!live_identifier(lease.session)||!live_identifier(lease.consumer)||(!live_playlist(base,lease.playlist)||lease.playlist.rfind(base+"/livetv/sessions/"+lease.session+"/"+lease.consumer+"/",0)!=0)){
   view.message="Plex live acquisition did not provide a verified compatible stream.";
   if(lease.owned||lease.uncertain){if(cleanup())view.state=LiveState::Error;}else view.state=LiveState::Error;return false;
  }
  player_active=true;
  if(!player.open||!player.open(lease.playlist)){view.message="Live stream could not be opened.";if(cleanup())view.state=LiveState::Error;return false;}
  if(view.current.id!=channel.id||view.current.dvr!=channel.dvr){view.previous=view.current;view.current=channel;}
  failures=0;due=now+10;view.state=LiveState::Buffering;return true;
 }
 bool stop(){if(!owns_resources()&&(view.state==LiveState::Released||view.state==LiveState::Idle))return true;return cleanup();}
 bool last(double now){if(view.previous.id.empty())return false;auto channel=view.previous;return start(channel,now);}
 bool step(const std::vector<LiveChannel> &channels,int direction,double now){
  auto at=std::find_if(channels.begin(),channels.end(),[&](const auto&c){return c.id==view.current.id&&c.dvr==view.current.dvr;});
  if(at==channels.end()||channels.empty())return false;
  auto index=(int(at-channels.begin())+(direction<0?-1:1)+int(channels.size()))%int(channels.size());return start(channels[size_t(index)],now);
 }
 void mini_guide(bool on){view.mini_guide=on;}
 void tick(double now,bool playing,bool ended,bool failed){
  if(release_pending){if(now>=due){due=now+10;cleanup();}return;}
  if(!lease.owned||view.state==LiveState::Stopping||view.state==LiveState::Released)return;
  if(failed||ended){view.state=LiveState::Recovery;view.message="Live playback was interrupted. Stop and retune after cleanup.";cleanup();return;}
  if(view.state==LiveState::Buffering&&playing)view.state=LiveState::Playing;
  if(now<due)return;
  auto r=protocol.heartbeat(lease);
  if(r){failures=0;due=now+10;view.message.clear();if(playing)view.state=LiveState::Playing;}
  else{++failures;view.state=LiveState::Recovery;view.message="Plex live connection interrupted; bounded retry pending.";due=now+std::min(60,5*(1<<std::min(failures,3)));if(r.status==401||r.status==403||r.status==404||failures>=3)cleanup();}
 }
 bool shutdown(){return stop();}
};
}
