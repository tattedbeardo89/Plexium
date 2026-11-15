// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "session.h"
namespace plexium {
inline size_t json_memory(const plex::Json&j){size_t n=sizeof(j);if(j.is_string())return n+j.get_ref<const std::string&>().capacity();if(j.is_object()){for(auto it=j.begin();it!=j.end();++it)n+=64+it.key().capacity()+json_memory(it.value());}else if(j.is_array())for(const auto&v:j)n+=sizeof(v)+json_memory(v);return n;}
inline size_t item_memory(const plex::Item&i){
 size_t n=sizeof(i)+json_memory(i.media);for(const auto*s:{&i.id,&i.key,&i.type,&i.title,&i.summary,&i.thumb,&i.art,&i.series,&i.content_rating,&i.released,&i.artist,&i.album,&i.parent_id,&i.grandparent_id,&i.section_id,&i.guid,&i.parent_thumb,&i.grandparent_thumb,&i.parent_art,&i.grandparent_art,&i.studio,&i.rating,&i.tagline})n+=s->capacity();
 for(const auto*v:{&i.genres,&i.cast,&i.directors}){n+=v->capacity()*sizeof(std::string);for(const auto&s:*v)n+=s.capacity();}
 n+=i.credits.capacity()*sizeof(plex::Credit)+i.chapters.capacity()*sizeof(plex::Chapter)+i.markers.capacity()*sizeof(plex::Marker);
 for(const auto&c:i.credits)n+=c.name.capacity()+c.role.capacity()+c.thumb.capacity();for(const auto&c:i.chapters)n+=c.title.capacity();for(const auto&m:i.markers)n+=m.type.capacity();return n;
}
inline size_t items_memory(const std::vector<plex::Item>&v){size_t n=(v.capacity()-v.size())*sizeof(plex::Item);for(const auto&i:v)n+=item_memory(i);return n;}
inline size_t hubs_memory(const std::vector<plex::Hub>&v){size_t n=v.capacity()*sizeof(plex::Hub);for(const auto&h:v)n+=h.id.capacity()+h.title.capacity()+items_memory(h.entries);return n;}
inline size_t model_memory(const Model&m){
 size_t n=sizeof(m)+item_memory(m.detail)+items_memory(m.entries)+items_memory(m.sections)+items_memory(m.upcoming)+items_memory(m.recording_rows)+items_memory(m.music_now)+items_memory(m.music_preview)+items_memory(m.queue_rows)+hubs_memory(m.rails)+hubs_memory(m.detail_rails);
 for(const auto*s:{&m.notice_body,&m.message,&m.query,&m.library_title,&m.library_path,&m.library_id,&m.profile_name,&m.profile_avatar,&m.server_name,&m.search_scope,&m.runtime_info,&m.connection_label})n+=s->capacity();
 n+=m.guide.channels.capacity()*sizeof(plex::GuideChannel);for(const auto&c:m.guide.channels){n+=c.id.capacity()+c.number.capacity()+c.title.capacity()+c.thumb.capacity()+c.dvr.capacity()+c.programs.capacity()*sizeof(plex::GuideProgram);for(const auto&p:c.programs)n+=p.title.capacity()+p.series.capacity()+p.summary.capacity();}
 for(const auto&v:m.notice_labels)n+=sizeof(v)+v.capacity();for(const auto&v:m.queue_ids)n+=sizeof(v)+v.capacity();
 n+=m.audio_tracks.capacity()*sizeof(plex::MediaTrack)+m.subtitle_tracks.capacity()*sizeof(plex::MediaTrack);for(const auto*v:{&m.audio_tracks,&m.subtitle_tracks})for(const auto&t:*v)n+=t.id.capacity()+t.language.capacity()+t.codec.capacity()+t.label.capacity()+t.key.capacity();return n;
}
inline Model cache_snapshot(const Model&m){auto result=m;std::vector<plex::Item>().swap(result.music_now);std::vector<plex::Item>().swap(result.music_preview);std::vector<plex::Item>().swap(result.queue_rows);std::vector<std::string>().swap(result.queue_ids);std::string().swap(result.notice_body);std::vector<std::string>().swap(result.notice_labels);return result;}
}
