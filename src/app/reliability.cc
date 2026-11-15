// SPDX-License-Identifier: GPL-3.0-or-later
#include "session.h"
#include "model_budget.h"
namespace plexium {
__attribute__((noinline)) void Session::push_history(){
 auto snapshot=cache_snapshot(model);if(model_memory(snapshot)>8u*1024*1024)return;
 if(history.size()>=16)history.erase(history.begin());history.push_back(std::move(snapshot));trim_caches();
}
__attribute__((noinline)) void Session::cache_page(const std::string&key){
 auto snapshot=cache_snapshot(model);if(model_memory(snapshot)>4u*1024*1024)return;pages[key]={std::move(snapshot),now()};trim_caches();
}
__attribute__((noinline)) void Session::trim_caches(){
 size_t total=0;for(const auto &p:pages)total+=model_memory(p.second.value)+p.first.capacity();
 while(!pages.empty()&&(total>8u*1024*1024||pages.size()>12)){auto oldest=std::min_element(pages.begin(),pages.end(),[](const auto&a,const auto&b){return a.second.loaded<b.second.loaded;});total-=model_memory(oldest->second.value)+oldest->first.capacity();pages.erase(oldest);++cache_evictions;}
 size_t bytes=0;for(const auto&p:metadata)bytes+=item_memory(p.second.first)+p.first.capacity();
 while(!metadata.empty()&&(bytes>4u*1024*1024||metadata.size()>32)){auto oldest=std::min_element(metadata.begin(),metadata.end(),[](const auto&a,const auto&b){return a.second.second<b.second.second;});bytes-=item_memory(oldest->second.first)+oldest->first.capacity();metadata.erase(oldest);++cache_evictions;}
 size_t back=0;for(const auto&m:history)back+=model_memory(m);
 while(!history.empty()&&(back>8u*1024*1024||history.size()>16)){back-=model_memory(history.front());history.erase(history.begin());++cache_evictions;}
 size_t home=hubs_memory(cached_home);if(home>2u*1024*1024){cached_home.clear();home_loaded=-1;home=0;++cache_evictions;}
 size_t guide=json_memory(personal_channels);if(guide>8u*1024*1024){personal_channels=plex::Json::array();personal_loaded=0;guide=0;++cache_evictions;}
 size_t provider=items_memory(channels);for(const auto&p:live_parts)provider+=p.first.capacity()+p.second.capacity()+64;if(provider>4u*1024*1024){channels.clear();live_parts.clear();channels_loaded=0;provider=0;++cache_evictions;}
 model.cache_bytes=total+bytes+back+home+guide+provider;model.cache_evictions=cache_evictions;model.prefetch_bytes=prefetched_uid?item_memory(prefetched_audio):0;
 if(model.prefetch_bytes>256u*1024){prefetched_uid=0;prefetched_audio={};model.prefetch_bytes=0;}
}
__attribute__((noinline)) void Session::revoke_connection(){
 if(audio_active){saved_music_position=playing_audio.offset;save_music(true);audio_active=audio_started=false;}
 player.stop();playback_started=false;model.playback_state=PlaybackState::Error;authorization_cleared=true;reconnect.blocked=true;connection_verified=false;prefetched_uid=0;prefetched_audio={};network_music_resume=false;
 history.clear();pages.clear();metadata.clear();cached_home.clear();home_loaded=-1;audio_queue.clear();playing_audio={};
 model.navigation.clear();model.servers.clear();model.server_name.clear();model.profile_name.clear();model.profile_avatar.clear();channels.clear();live_parts.clear();personal_channels=plex::Json::array();personal_loaded=0;restore_validation.clear();
 model.sections.clear();model.entries.clear();model.rails.clear();model.detail_rails.clear();model.detail={};model.queue_rows.clear();model.queue_ids.clear();model.music_now.clear();model.music_preview.clear();model.music_active=false;model.music_available=false;model.profiles.clear();model.guide={};model.recording_rows.clear();model.screen=Screen::Profiles;model.message="Plex access expired or was revoked. Select your profile again or sign in.";
}
__attribute__((noinline)) void Session::network_result(const plex::Result&r){
 if(r)return;
 if(r.status==401||r.status==403){reconnect.lost(now(),r.status);connection_verified=false;return;}
 if(r.status==0||r.status==200||r.status==408||r.status==429||r.status>=500){reconnect.lost(now(),r.status);connection_verified=false;prefetched_uid=0;prefetched_audio={};}
}
__attribute__((noinline)) bool Session::connection_guard(const Command&c){
 if(!reconnect.blocked)return false;
 switch(c.action){case Action::Restore:case Action::Link:case Action::Poll:case Action::Profile:case Action::SignOut:case Action::Cancel:case Action::Stop:case Action::Tick:case Action::Notices:case Action::OpenNotice:case Action::ReconnectServer:return false;case Action::Server:if(c.id==server_id)return false;break;case Action::Home:if(c.id=="profiles")return false;break;default:break;}
 model.message="Plex authorization is required before accessing this profile's media.";return true;
}
__attribute__((noinline)) bool Session::reconnect_server(){
 if(base.empty()||server_id.empty()||active.empty()||account.empty()||reconnect.blocked)return false;
 auto bounded=client.bounded(500,1000);
 auto primary=bounded.validate_account(account);network_result(primary);if(!primary)return false;if(plex::profile_id(primary.data)!=account_id){revoke_connection();return false;}
 auto homes=bounded.home_users(account);network_result(homes);if(!homes)return false;
 auto available=plex::profiles(homes.data);auto home_user=std::find_if(available.begin(),available.end(),[&](const auto&p){return p.id==current_profile;});if(home_user==available.end()){revoke_connection();return false;}
 auto discovery=bounded.resources(active);network_result(discovery);if(!discovery)return false;
 auto servers=plex::servers(discovery.data);auto selected=std::find_if(servers.begin(),servers.end(),[&](const auto&s){return s.id==server_id;});if(selected==servers.end()){revoke_connection();return false;}
 auto preferred=std::find_if(selected->connections.begin(),selected->connections.end(),[&](const auto&c){return c.uri==base;});if(preferred!=selected->connections.end())std::rotate(selected->connections.begin(),preferred,preferred+1);if(selected->connections.size()>3)selected->connections.resize(3);
 if(selected->token.empty())selected->token=active;std::string route;
 auto checked=bounded.select(*selected,false,route,nullptr,base);network_result(checked);if(!checked)return false;
 auto sections=plex::items(checked.data,false);
 // Rebind only this machineIdentifier; no call to discover/connect may auto-select another server.
 bool reauthorized=authorization_cleared;model.profile_name=home_user->name;model.profile_avatar=home_user->avatar;model.server_name=selected->name;
 local_connection=std::any_of(selected->connections.begin(),selected->connections.end(),[&](const auto&c){return c.uri==route&&c.local;});base=route;token=selected->token;if(!stream.session.empty()){stream_base=route;stream_token=token;}model.sections=std::move(sections);model.navigation=navigation(model.sections);resources=std::move(servers);model.servers.clear();for(const auto&s:resources)model.servers.push_back({s.id,s.name});
 history.clear();pages.clear();metadata.clear();cached_home.clear();personal_channels=plex::Json::array();personal_loaded=0;model.guide.stale=true;home_loaded=-1;prefetched_uid=0;prefetched_audio={};
 for(size_t n=audio_queue.order.size();n-->0;){const auto&i=audio_queue.entries[audio_queue.order[n]];if(i.section_id.empty()||!audio_metadata(i)){if(n==audio_queue.cursor&&audio_active){player.stop();audio_active=false;playing_audio={};network_music_resume=false;}audio_queue.remove(n,true);}}
 connection_verified=true;reconnect.offline=false;
 if(model.screen==Screen::Library&&!model.library_path.empty()&&!model.library_id.empty()){
  if(std::any_of(model.sections.begin(),model.sections.end(),[&](const auto&s){return s.id==model.library_id;})){model.entries.clear();listing(model.library_path,model.page_start);}else home(true);
 }else if(model.screen==Screen::Library&&model.library_tab=="results"&&!model.query.empty()){
  auto query=model.query,scope=model.search_scope;model.entries.clear();model.rails.clear();if(scope.empty()||std::any_of(model.sections.begin(),model.sections.end(),[&](const auto&s){return s.id==scope;}))search_command({Action::Search,scope,query});else home(true);
 }else if(model.screen==Screen::Library&&!model.library_path.empty()&&!model.detail.section_id.empty()&&std::any_of(model.sections.begin(),model.sections.end(),[&](const auto&s){return s.id==model.detail.section_id;})){
  model.entries.clear();listing(model.library_path,model.page_start);
 }else if(model.screen==Screen::Details&&!model.detail.id.empty()){
  auto id=model.detail.id,type=model.detail.type;auto version=model.media_version;model.detail_rails.clear();if(model.detail.section_id.empty()||std::any_of(model.sections.begin(),model.sections.end(),[&](const auto&s){return s.id==model.detail.section_id;})){details_command({Action::Details,id,type});model.media_version=std::min(version,model.detail.media.empty()?size_t(0):model.detail.media.size()-1);load_tracks();}else {model.detail={};home(true);}
 }else if(model.screen!=Screen::Playback&&model.screen!=Screen::Tracks&&model.screen!=Screen::Queue&&model.screen!=Screen::Settings&&model.screen!=Screen::Search&&model.screen!=Screen::Servers&&model.screen!=Screen::Guide){model.entries.clear();model.rails.clear();model.detail_rails.clear();home(true);}
 if(reconnect.offline||reconnect.blocked)return false;if(reauthorized)bind_music();if(reconnect.offline||reconnect.blocked)return false;authorization_cleared=false;reconnect.reset(now());for(auto&u:outbox.entries){u.blocked=false;u.recovered=true;u.due=now();}flush_progress();
 if(network_music_resume&&!audio_active&&preferences.value.reconnect_music&&audio_queue.current()){network_music_resume=false;audio_resume=saved_music_position;open_audio(true);}
 model.message="Reconnected to the selected Plex server; permissions and pending progress were revalidated.";return true;
}
__attribute__((noinline)) void Session::reliability_tick(){
 if(base.empty()||token.empty()||reconnect.blocked)return;
 if(!connection_verified&&!reconnect.offline)reconnect.lost(now(),0);
 if(reconnect.ready(now())){if(!reconnect_server()&&!reconnect.blocked)reconnect.failed(now());return;}
 if(!reconnect.offline&&now()>=reconnect.probe){reconnect.probe=now()+30;auto r=client.bounded(500,900).identity(base,token,server_id);network_result(r);if(r){auto access=client.bounded(500,900).sections(base,token);network_result(access);}}
}
}
