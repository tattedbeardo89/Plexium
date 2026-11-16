// SPDX-License-Identifier: GPL-3.0-or-later
#include "session.h"
#include "catalog.h"
#include <charconv>
namespace plexium {
namespace {
struct MusicNavigation {
 Model &model;std::function<void(const Model&)> &publish,saved;
 struct State {Screen screen;plex::Item detail;std::vector<plex::MediaTrack> audio,subtitles;plex::TrackSelection tracks;size_t version;bool during;explicit State(const Model&m):screen(m.screen),detail(m.detail),audio(m.audio_tracks),subtitles(m.subtitle_tracks),tracks(m.tracks),version(m.media_version),during(m.tracks_during_playback){}};
 std::unique_ptr<State> state;
 MusicNavigation(Model&m,std::function<void(const Model&)>&p,bool keep):model(m),publish(p){if(keep){state=std::make_unique<State>(m);saved=std::move(publish);}}
 ~MusicNavigation(){if(!state)return;model.screen=state->screen;model.detail=std::move(state->detail);model.audio_tracks=std::move(state->audio);model.subtitle_tracks=std::move(state->subtitles);model.tracks=std::move(state->tracks);model.media_version=state->version;model.tracks_during_playback=state->during;publish=std::move(saved);}
};
}
__attribute__((noinline)) void Session::report_audio(const std::string &state,int64_t position){
 music_reported_at=now();std::swap(model.detail,playing_audio);remember_position(position);report_progress(state,position);std::swap(model.detail,playing_audio);
}
__attribute__((noinline)) bool Session::stop_audio(int64_t captured){
 if(!audio_active)return true;auto observation=player.observe();auto position=captured>=0?captured:observation.failed?playing_audio.offset:observation.ended&&playing_audio.duration>0?playing_audio.duration:player.position();position=std::clamp<int64_t>(position,0,playing_audio.duration>0?playing_audio.duration:INT64_MAX);
 bool observed=audio_started||!observation.known||(!observation.failed&&(observation.playing||observation.paused||observation.ended));player.stop();auto stopped=player.observe();if(stopped.known&&(stopped.playing||stopped.paused||stopped.buffering)){playing_audio.offset=position;save_music(true);model.message="Music stop is incomplete; ownership was retained. Retry Stop.";return false;}if(observed){playing_audio.offset=position;report_audio("stopped",position);}
 if(!close_stream()){model.message="Music session cleanup failed. Retry Stop before opening another item.";return false;}
 audio_active=false;audio_started=false;saved_music_position=playing_audio.offset;save_music(true);return true;
}
__attribute__((noinline)) bool Session::open_audio(bool background){
 MusicNavigation navigation(model,progress,background);if(reconnect.blocked)return false;
 const auto *queued=audio_queue.current();if(!queued)return false;auto id=queued->id;
 std::vector<plex::Item> entries;
 if(prefetched_uid==audio_queue.uid(audio_queue.cursor)&&prefetched_revision==audio_queue.revision&&now()-prefetched_at<30){entries.push_back(prefetched_audio);}
 else {auto response=client.bounded(800,2000).browse(base,token,"/library/metadata/"+plex::encode(id),0,1);network_result(response);if(response)entries=plex::items(response.data);}
 if(entries.size()!=1||entries[0].id!=id||!audio_metadata(entries[0])){model.message="The selected music track is unavailable.";return false;}
 model.detail=std::move(entries[0]);if(model.detail.section_id.empty())model.detail.section_id=queued->section_id;if(model.detail.key.empty())model.detail.key="/library/metadata/"+id;if(audio_resume>=0){model.detail.offset=audio_resume;audio_resume=-1;}model.media_version=0;load_tracks();
 if(!open_playback())return false;playing_audio=model.detail;music_tracks=model.tracks;audio_active=true;audio_started=false;music_restored=false;music_interrupted=false;audio_queue.entries[audio_queue.order[audio_queue.cursor]]=plex::AudioQueue::compact(playing_audio);save_music(true);return true;
}
__attribute__((noinline)) void Session::music_command(const Command &c){
 // End notifications are observations, never unconditional queue advances. A
 // delayed render-thread notification must not skip the newly opened track.
 const bool natural=c.action==Action::MusicNext&&c.text=="ended";
 if(natural){auto observed=player.observe();if(!audio_active||!observed.ended||observed.failed)return;}
 struct Deferred {Session &owner;bool prior,on;Deferred(Session&s,bool enabled):owner(s),prior(s.defer_music_progress),on(enabled){owner.defer_music_progress=enabled||prior;}~Deferred(){owner.defer_music_progress=prior;if(on&&!prior)owner.flush_progress();}} deferred(*this,natural);
 if(c.action==Action::MusicHome){home();return;}
 if(c.action==Action::MusicOpen){if(audio_active){model.detail=playing_audio;model.media_version=0;load_tracks();model.tracks=music_tracks;model.screen=Screen::Playback;}else queue_command({Action::QueueResume,{},{}});return;}
 if(c.action==Action::MusicShuffle){prefetched_uid=0;prefetched_audio={};if(audio_queue.shuffled)audio_queue.unshuffle();else audio_queue.shuffle(uint32_t(++playback_serial));save_music(true);return;}
 if(c.action==Action::MusicRepeat){audio_queue.repeat=static_cast<plex::Repeat>((int(audio_queue.repeat)+1)%3);prefetched_uid=0;prefetched_audio={};save_music(true);return;}
 if(c.action==Action::QueuePlay){
  auto item=model.detail;if(item.type!="album"&&item.type!="playlist"&&item.type!="artist"&&item.type!="track"){model.message="Choose an artist, album, audio playlist or track.";return;}
  std::vector<plex::Item> tracks;
  if(item.type=="track"){item.media={};tracks.push_back(item);}else{auto bounded=client.bounded(800,1500);auto result=plex::audio_listing(bounded,base,token,item.type=="playlist"?"/playlists/"+plex::encode(item.id)+"/items":"/library/metadata/"+plex::encode(item.id)+(item.type=="artist"?"/allLeaves":"/children"),[]{return true;},true);if(!result){model.message="The complete music queue could not be loaded.";return;}tracks=plex::audio_tracks(result.data,item.type=="album");}
  if(tracks.empty()){model.message="No playable tracks were returned by Plex.";return;}
  if(audio_active&&!stop_audio())return;
  if((model.screen==Screen::Playback||model.screen==Screen::Tracks)&&model.detail.type!="track"){stop_command({Action::Stop,{},{}});if(!stream.session.empty())return;}
  for(auto &track:tracks)if(track.section_id.empty())track.section_id=item.section_id;audio_queue.assign(std::move(tracks));prefetched_uid=0;prefetched_audio={};if(c.id=="shuffle")audio_queue.shuffle(uint32_t(++playback_serial));if(!model.books)audio_resume=0;open_audio();return;
 }
 if(!audio_active&&c.action!=Action::MusicNext&&c.action!=Action::MusicPrevious)return;
 if(audio_active&&c.action==Action::MusicPrevious&&player.position()>3000){player.seek(0);playing_audio.offset=0;report_audio(model.paused?"paused":"playing",0);return;}
 bool background=model.screen!=Screen::Playback&&!(model.screen==Screen::Tracks&&model.tracks_during_playback);bool ended=c.text=="ended";if(!stop_audio())return;
 auto old=audio_queue.cursor;bool next=c.action==Action::MusicPrevious?(audio_queue.cursor==0?true:audio_queue.previous()):audio_queue.advance(ended);
 if(!model.books||audio_queue.repeat==plex::Repeat::One)audio_resume=0;
 if(!next){if(!background){model.screen=Screen::Details;model.detail=playing_audio;}return;}
 if(reconnect.offline&&!reconnect.blocked){playing_audio=*audio_queue.current();saved_music_position=model.books?playing_audio.offset:0;playing_audio.offset=saved_music_position;network_music_resume=preferences.value.reconnect_music;if(!background){model.screen=Screen::Details;model.detail=playing_audio;}save_music(true);model.message="Next track is waiting for the selected server.";return;}
 if(!open_audio(background)){if(reconnect.offline&&!reconnect.blocked){playing_audio=*audio_queue.current();saved_music_position=model.books?playing_audio.offset:0;playing_audio.offset=saved_music_position;network_music_resume=preferences.value.reconnect_music;save_music(true);}else audio_queue.cursor=old;}
}
__attribute__((noinline)) bool Session::music_tick(){
 if(!audio_active)return false;auto observation=player.observe();
 if(observation.failed){if(reconnect.offline){network_music_resume=!player.paused()&&preferences.value.reconnect_music;stop_audio(playing_audio.offset);}model.playback_state=PlaybackState::Error;model.message="Decoder failure: music stopped. Retry this track or choose another.";return true;}
 bool prior_paused=model.paused;playing_audio.offset=std::max<int64_t>(0,player.position());saved_music_position=playing_audio.offset;save_music();model.paused=player.paused();
 if(observation.ended){if(playing_audio.duration>0)playing_audio.offset=playing_audio.duration;music_command({Action::MusicNext,{},"ended"});return true;}
 if(observation.playing||observation.paused){bool first=!audio_started;audio_started=true;model.playback_state=observation.paused?PlaybackState::Paused:PlaybackState::Playing;if(first||prior_paused!=model.paused||now()-music_reported_at>=10)report_audio(observation.paused?"paused":"playing",playing_audio.offset);}else model.playback_state=PlaybackState::Buffering;
 apply_music_tracks();
 if(model.screen==Screen::Playback||(model.screen==Screen::Tracks&&model.tracks_during_playback)){model.detail=playing_audio;model.media_version=0;model.audio_tracks=plex::media_tracks(playing_audio,0,false);model.subtitle_tracks=plex::media_tracks(playing_audio,0,true);model.tracks=music_tracks;}
 prefetch_music();return true;
}
__attribute__((noinline)) void Session::apply_music_tracks(){
 if(!tracks_pending)return;MusicNavigation navigation(model,progress,true);model.detail=playing_audio;model.media_version=0;model.audio_tracks=plex::media_tracks(playing_audio,0,false);model.subtitle_tracks=plex::media_tracks(playing_audio,0,true);model.tracks=music_tracks;apply_tracks();music_tracks=model.tracks;
}
__attribute__((noinline)) bool Session::audio_metadata(const plex::Item&i)const{
 return i.type=="track"&&!i.id.empty()&&(i.section_id.empty()||std::any_of(model.sections.begin(),model.sections.end(),[&](const auto&s){return s.id==i.section_id&&s.type=="artist";}));
}
__attribute__((noinline)) void Session::save_music(bool force){
 if(track_preferences_scope.empty()||(!force&&now()-music_saved_at<15))return;
 if(audio_active)saved_music_position=playing_audio.offset;
 if(!music_store.save(audio_queue,saved_music_position))model.message="Music session could not be saved safely; previous saved state was preserved.";
 music_saved_at=now();
}
__attribute__((noinline)) void Session::bind_music(){
 ++queue_epoch;prefetch_attempt_uid=0;prefetched_uid=0;prefetched_audio={};audio_active=false;audio_started=false;music_interrupted=false;network_music_resume=false;music_restored=false;queue_offset=0;
 if(!music_store.bind(root,track_preferences_scope,audio_queue,saved_music_position)){model.message="Saved music session is corrupt or unsupported; it was not overwritten.";return;}
 restore_validation.clear();restore_validation_at=0;restore_validation_failures=0;
 auto previous=audio_queue.uid(audio_queue.cursor);
 for(size_t n=audio_queue.order.size();n-->0;){const auto&i=audio_queue.entries[audio_queue.order[n]];if(!std::any_of(model.sections.begin(),model.sections.end(),[&](const auto&s){return s.id==i.section_id&&s.type=="artist";}))audio_queue.remove(n,true);}
 if(audio_queue.locate(previous)>=audio_queue.order.size())saved_music_position=0;
 for(size_t n=0;n<audio_queue.order.size();++n)restore_validation.push_back(audio_queue.uid(n));
 // No launch-time audible playback. Resolve current metadata before offering Resume.
 for(unsigned budget=0;audio_queue.current()&&budget<4;++budget){
  auto expected=audio_queue.current()->id;auto r=client.bounded(700,1200).browse(base,token,"/library/metadata/"+plex::encode(expected),0,1);network_result(r);
  if(!r){if(r.status==404){audio_queue.remove(audio_queue.cursor,true);saved_music_position=0;continue;}return;}
  auto items=plex::items(r.data);if(items.size()!=1||items[0].id!=expected||!audio_metadata(items[0])){audio_queue.remove(audio_queue.cursor,true);saved_music_position=0;break;}
  playing_audio=std::move(items[0]);if(playing_audio.section_id.empty())playing_audio.section_id=audio_queue.current()->section_id;playing_audio.offset=std::clamp<int64_t>(saved_music_position,0,playing_audio.duration>0?playing_audio.duration:INT64_MAX);
  audio_queue.entries[audio_queue.order[audio_queue.cursor]]=plex::AudioQueue::compact(playing_audio);music_restored=true;model.message="Saved music queue restored. Open the queue to Resume.";break;
 }
}
__attribute__((noinline)) void Session::validate_music(){
 if(restore_validation_at>=restore_validation.size()||reconnect.offline||reconnect.blocked)return;
 std::set<std::string> requested;auto end=std::min(restore_validation.size(),restore_validation_at+32);
 for(auto n=restore_validation_at;n<end;++n){auto at=audio_queue.locate(restore_validation[n]);if(at<audio_queue.order.size())requested.insert(audio_queue.entries[audio_queue.order[at]].id);}
 if(requested.empty()){restore_validation_at=end;return;}
 std::string ids;for(const auto&id:requested){if(!ids.empty())ids+=",";ids+=id;}
 auto r=client.bounded(600,1200).browse(base,token,"/library/metadata/"+ids,0,32);network_result(r);
 auto container=r.data.find("MediaContainer");
 if(!r||container==r.data.end()||!container->is_object()||!container->contains("Metadata")||!(*container)["Metadata"].is_array()){
  if(!reconnect.offline&&++restore_validation_failures>=3){restore_validation_at=restore_validation.size();model.message="Some saved tracks could not be checked. Each track will be verified before playback.";}return;
 }
 auto values=plex::items(r.data,false);std::map<std::string,plex::Item> accessible;
 for(auto&i:values)if(requested.count(i.id)&&audio_metadata(i))accessible[i.id]=plex::AudioQueue::compact(i);
 for(auto n=restore_validation_at;n<end;++n){auto at=audio_queue.locate(restore_validation[n]);if(at>=audio_queue.order.size())continue;auto&item=audio_queue.entries[audio_queue.order[at]];auto found=accessible.find(item.id);
  if(found!=accessible.end()){auto section=item.section_id;item=found->second;if(item.section_id.empty())item.section_id=section;}
  else {bool current=at==audio_queue.cursor;if(current&&audio_active&&!stop_audio()){restore_validation.clear();restore_validation_at=0;return;}audio_queue.remove(at,true);if(current){playing_audio={};saved_music_position=0;music_restored=false;}}
 }
 restore_validation_at=end;restore_validation_failures=0;save_music(true);
 if(end==restore_validation.size()){restore_validation.clear();restore_validation_at=0;}
}
__attribute__((noinline)) void Session::queue_snapshot(){
 model.queue_rows.clear();model.queue_ids.clear();model.music_available=!audio_queue.order.empty();model.music_restored=music_restored;model.music_paused=!audio_active||model.paused;model.queue_bytes=audio_queue.bytes();
 if(model.screen!=Screen::Queue)return;
 if(queue_offset>=audio_queue.order.size())queue_offset=audio_queue.order.empty()?0:((audio_queue.order.size()-1)/60)*60;model.queue_offset=queue_offset;
 for(size_t n=queue_offset;n<audio_queue.order.size()&&n<queue_offset+60;++n){model.queue_rows.push_back(audio_queue.entries[audio_queue.order[n]]);model.queue_ids.push_back(std::to_string(queue_epoch)+":"+std::to_string(audio_queue.uid(n)));}
}
__attribute__((noinline)) void Session::prefetch_music(){
 if(!audio_active||reconnect.offline||reconnect.blocked||playing_audio.duration<=0||playing_audio.duration-playing_audio.offset>15000)return;
 size_t at=audio_queue.cursor;if(audio_queue.repeat!=plex::Repeat::One){if(at+1<audio_queue.order.size())++at;else if(audio_queue.repeat==plex::Repeat::All)at=0;else return;}
 auto uid=audio_queue.uid(at);if(prefetched_uid==uid&&prefetched_revision==audio_queue.revision&&now()-prefetched_at<30)return;
 if(prefetch_attempt_uid==uid&&prefetch_attempt_revision==audio_queue.revision&&now()-prefetch_attempt_at<10)return;prefetch_attempt_uid=uid;prefetch_attempt_revision=audio_queue.revision;prefetch_attempt_at=now();
 prefetched_uid=0;prefetched_audio={};auto id=audio_queue.entries[audio_queue.order[at]].id;
 auto r=client.bounded(500,1000).browse(base,token,"/library/metadata/"+plex::encode(id),0,1);network_result(r);if(!r)return;
 auto entries=plex::items(r.data);if(entries.size()!=1||entries[0].id!=id||!audio_metadata(entries[0])||entries[0].media.dump().size()>128*1024)return;
 prefetched_audio=std::move(entries[0]);prefetched_uid=uid;prefetched_revision=audio_queue.revision;prefetched_at=now();
}
__attribute__((noinline)) bool Session::music_focus_video(){
 bool interrupted=network_music_resume;network_music_resume=false;if(!audio_active){if(interrupted)music_interrupted=preferences.value.resume_music;return true;}bool resume=!player.paused()&&preferences.value.resume_music;
 if(!stop_audio())return false;music_interrupted=resume;return true;
}
__attribute__((noinline)) void Session::resume_after_video(const Command&c){
 if(c.id!="ended"){music_interrupted=false;return;}
 if(model.autoplay_remaining>0)return;
 if(!music_interrupted||!preferences.value.resume_music||!stream.session.empty()||reconnect.offline||reconnect.blocked)return;
 music_interrupted=false;audio_resume=saved_music_position;open_audio();
}
__attribute__((noinline)) void Session::queue_command(const Command&c){
 if(c.action==Action::Queue){if(model.screen!=Screen::Queue){push_history();queue_offset=audio_queue.cursor/60*60;}else if(c.id=="next")queue_offset+=60;else if(c.id=="previous")queue_offset=queue_offset>=60?queue_offset-60:0;model.screen=Screen::Queue;return;}
 if(c.action==Action::MusicPause){if(audio_active){player.pause();model.paused=player.paused();report_audio(model.paused?"paused":"playing",player.position());}return;}
 if(c.action==Action::QueueResume){if(!audio_queue.current())return;if(audio_active){model.detail=playing_audio;model.screen=Screen::Playback;return;}if(!stream.session.empty())return;audio_resume=saved_music_position;open_audio();return;}
 if(c.action==Action::QueueAdd){restore_validation.clear();
  const auto item=model.detail;std::vector<plex::Item> additions;
  if(item.type=="track")additions.push_back(item);
  else if(item.type=="album"||item.type=="artist"||item.type=="playlist"){
   auto path=item.type=="playlist"?"/playlists/"+plex::encode(item.id)+"/items":"/library/metadata/"+plex::encode(item.id)+(item.type=="artist"?"/allLeaves":"/children");
   auto bounded=client.bounded(600,1200);auto r=plex::audio_listing(bounded,base,token,path,[]{return true;},true);network_result(r);if(!r){model.message="Queue change cancelled: complete listing exceeds the 2000-track budget or is unavailable.";return;}additions=plex::audio_tracks(r.data,item.type=="album");
  }else return;
  if(additions.empty()||audio_queue.entries.size()+additions.size()>plex::AudioQueue::capacity){model.message="Queue limit is 2000 tracks; existing playback was preserved.";return;}
  // Stage edits so a failed admission never leaves a partial album in the queue.
  auto staged=audio_queue;size_t insertion=staged.cursor+1;
  for(auto &i:additions){if(i.section_id.empty())i.section_id=item.section_id;if(!audio_metadata(i)||!staged.add(i)){model.message="Queue byte budget exceeded or track access changed.";return;}if(c.id=="next"&&staged.order.size()>1)staged.move(staged.order.size()-1,std::min(insertion++,staged.order.size()-1));}
  audio_queue=std::move(staged);prefetched_uid=0;prefetched_audio={};save_music(true);model.message="Added "+std::to_string(additions.size())+" track(s) to the queue.";return;
 }
 if(c.action!=Action::QueueEdit)return;
 auto separator=c.id.find(':');if(separator==std::string::npos)return;uint64_t epoch=0;auto epoch_parse=std::from_chars(c.id.data(),c.id.data()+separator,epoch);if(epoch_parse.ec!=std::errc()||epoch_parse.ptr!=c.id.data()+separator||epoch!=queue_epoch){model.message="Queue belongs to a previous profile/server context.";return;}
 uint64_t uid=0;auto parsed=std::from_chars(c.id.data()+separator+1,c.id.data()+c.id.size(),uid);if(parsed.ec!=std::errc()||parsed.ptr!=c.id.data()+c.id.size())return;auto at=audio_queue.locate(uid);
 if(at>=audio_queue.order.size()){model.message="Queue entry changed. Select it again.";return;}
 restore_validation.clear();
 if(c.text=="clear"){if(audio_active&&!stop_audio())return;audio_queue.clear();playing_audio={};saved_music_position=0;music_restored=false;prefetched_uid=0;prefetched_audio={};save_music(true);return;}
 bool current=at==audio_queue.cursor,was_active=audio_active;
 if(c.text=="play"){
  if(current&&audio_active){model.detail=playing_audio;model.screen=Screen::Playback;return;}
  if(audio_active&&!stop_audio())return;if(!stream.session.empty())return;audio_queue.cursor=at;if(!current)saved_music_position=0;audio_resume=current?saved_music_position:0;open_audio(model.screen==Screen::Queue);
 }else if(c.text=="remove"){
  auto following=at+1<audio_queue.order.size()?audio_queue.uid(at+1):audio_queue.repeat==plex::Repeat::All&&audio_queue.order.size()>1?audio_queue.uid(0):0;
  if(current&&audio_active&&!stop_audio())return;audio_queue.remove(at,true);if(current){saved_music_position=0;playing_audio={};if(following)audio_queue.cursor=audio_queue.locate(following);if(was_active&&following&&audio_queue.current()){audio_resume=0;open_audio(model.screen==Screen::Queue);}}
 }else if(c.text=="up"||c.text=="down"||c.text=="next"){
  if(current&&c.text=="next")return;auto target=c.text=="up"?(at?at-1:0):c.text=="down"?std::min(at+1,audio_queue.order.size()-1):std::min(audio_queue.cursor+1,audio_queue.order.size()-1);
  audio_queue.move(at,target);queue_offset=target/60*60;
 }else return;
 prefetched_uid=0;prefetched_audio={};save_music(true);
}

}
