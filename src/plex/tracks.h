// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "home.h"
#include "subtitle_latch.h"
namespace plex {
struct MediaTrack {std::string id,language,title,codec,label,key;int channels=0;bool subtitle=false,selected=false;};
struct NativeTrack {int id=-1;std::string name,language,codec;int channels=0;};
struct TrackSelection {std::string audio,subtitle;bool operator==(const TrackSelection&o)const{return audio==o.audio&&subtitle==o.subtitle;}};
inline bool stream_id(const std::string &id){return !id.empty()&&id.size()<=32&&id.find_first_not_of("0123456789")==std::string::npos;}
inline std::vector<MediaTrack> media_tracks(const Item &item,size_t version,bool subtitles){
 std::vector<MediaTrack> result;if(!item.media.is_array()||version>=item.media.size())return result;
 const auto &media=item.media[version];auto parts=media.find("Part");if(parts==media.end()||!parts->is_array()||parts->size()!=1||!(*parts)[0].is_object())return result;
 auto streams=(*parts)[0].find("Stream");if(streams==(*parts)[0].end()||!streams->is_array())return result;
 for(const auto &s:*streams){
  if(number(s,"streamType")!=(subtitles?3:2))continue;
  MediaTrack track;track.id=text(s,"id",std::to_string(number(s,"id",-1)));if(!stream_id(track.id))continue;
  if(std::any_of(result.begin(),result.end(),[&](const auto&t){return t.id==track.id;}))continue;
  track.subtitle=subtitles;track.language=text(s,"languageCode",text(s,"languageTag"));track.title=text(s,"title");track.codec=text(s,"codec");track.channels=int(std::clamp<int64_t>(number(s,"channels"),0,32));track.selected=flag(s,"selected");track.key=text(s,"key");
  track.label=text(s,"displayTitle",text(s,"language",track.language.empty()?"Unknown language":track.language));
  if(!track.title.empty()&&track.label.find(track.title)==std::string::npos)track.label+=" · "+track.title;
  if(!track.codec.empty())track.label+=" · "+track.codec;
  if(track.channels)track.label+=" · "+std::to_string(track.channels)+" ch";
  if(subtitles)track.label+=track.key.empty()?" · Embedded":" · External";
  if(flag(s,"forced"))track.label+=" · Forced";
  if(flag(s,"hearingImpaired"))track.label+=" · SDH";
  for(char &c:track.label)if(static_cast<unsigned char>(c)<32)c=' ';
  if(track.label.size()>256)track.label.resize(256);
  if(track.language.size()>32||track.language.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_")!=std::string::npos)track.language.clear();if(track.title.size()>256)track.title.resize(256);if(track.key.size()>4096)track.key.clear();
  result.push_back(std::move(track));if(result.size()==96)break;
 }
 return result;
}
inline const MediaTrack *selected_track(const std::vector<MediaTrack>&tracks,const std::string&id){for(const auto &t:tracks)if(t.id==id)return &t;return nullptr;}
// Plex IDs and VLC ES IDs are different namespaces. Only a unique metadata
// match is selectable; never cast a Plex ID or assume an ES ordinal.
inline int native_track_match(const MediaTrack &wanted,const std::vector<NativeTrack>&tracks){
 int id=-1,count=0;
 for(const auto &t:tracks){if(t.id<0)continue;
  if(!wanted.language.empty()&&!subtitle_language_matches(t.language,wanted.language))continue;
  if(!wanted.title.empty()&&t.name.find(wanted.title)==std::string::npos)continue;
  if(wanted.channels&&t.channels&&wanted.channels!=t.channels)continue;
  if(wanted.language.empty()&&wanted.title.empty())continue;
  id=t.id;++count;
 }
 return count==1?id:-1;
}
inline bool text_subtitle(const MediaTrack&t){return t.codec=="srt"||t.codec=="subrip"||t.codec=="webvtt"||t.codec=="vtt"||t.codec=="ass"||t.codec=="ssa";}
}

namespace plex {
inline Result select_plex_streams(Client &client,const std::string &base,const std::string &token,const Item &item,size_t version,const TrackSelection &selection){
 auto audio=media_tracks(item,version,false),subtitles=media_tracks(item,version,true);
 if((!selection.audio.empty()&&!selected_track(audio,selection.audio))||(!selection.subtitle.empty()&&!selected_track(subtitles,selection.subtitle)))return {{},"Selected stream is not authorized for this media",0};
 if(!item.media.is_array()||version>=item.media.size())return {{},"Media version is unavailable",0};
 const auto &media=item.media[version];auto parts=media.find("Part");if(parts==media.end()||!parts->is_array()||parts->size()!=1)return {{},"Media part is unavailable",0};
 auto id=text((*parts)[0],"id",std::to_string(number((*parts)[0],"id",-1)));if(!stream_id(id))return {{},"Plex part identity is missing; server track selection unavailable",0};
 auto query="?allParts=0&subtitleStreamID="+(selection.subtitle.empty()?std::string("0"):encode(selection.subtitle));
 if(!selection.audio.empty())query+="&audioStreamID="+encode(selection.audio);
 return client.call("PUT",base,"/library/parts/"+encode(id)+query,token);
}
}
