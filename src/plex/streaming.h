// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "audio.h"
#include "home.h"
namespace plex {
enum class Quality {Original,Automatic,UHD,FHD,HD,SD,Low};
inline const char *quality_name(Quality q) {const char *names[]={"Original","Automatic (local Original / remote 1080p cap)","4K / 40 Mbps cap","1080p / 8 Mbps cap","720p / 4 Mbps cap","480p / 1.5 Mbps cap","360p / 720 kbps cap"};return names[std::clamp(static_cast<int>(q),0,6)];}
// Compiled-engine policy, not a claim of PS5 runtime acceptance.
struct VideoCapabilities {
    bool video(const std::string &codec) const {return codec=="h264" || codec=="hevc";}
    bool audio(const std::string &codec) const {return codec.empty() || codec=="aac" || codec=="mp3" || codec=="ac3" || codec=="eac3" || codec=="flac" || codec=="opus" || codec=="vorbis";}
    bool original(const Json &m) const {return video(text(m,"videoCodec")) && audio(text(m,"audioCodec")) && number(m,"width")<=1920 && number(m,"height")<=1080;}
};
struct StreamPlan {
    Playback playback;
    std::string session,server_mode;
};
inline std::string universal_query(const Item &i,size_t version,const std::string &session,Quality quality,bool local) {
    quality=static_cast<Quality>(std::clamp(static_cast<int>(quality),0,6));
    if(quality==Quality::Automatic)quality=local?Quality::Original:Quality::FHD;
    if(quality==Quality::UHD)quality=Quality::FHD; // 4K output remains gated.
    if(quality==Quality::Original && i.media.is_array() && version<i.media.size() && (number(i.media[version],"width")>1920 || number(i.media[version],"height")>1080))quality=Quality::FHD;
    std::string query="?path="+encode("/library/metadata/"+i.id)+"&mediaIndex="+std::to_string(version)+"&partIndex=0&protocol=hls&session="+encode(session)+"&directPlay=0&directStream=1&fastSeek=1&copyts=1&offset=0&subtitleStreamID=0&subtitles=none&hasMDE=1&X-Plex-Client-Capabilities="+encode("protocols=http-video,http-live-streaming;videoDecoders=h264{profile:high&resolution:1080&level:51},hevc{resolution:1080};audioDecoders=aac{channels:8},mp3,ac3,eac3,flac,opus,vorbis");
    if(quality!=Quality::Original){const char *res[]={"","","3840x2160","1920x1080","1280x720","854x480","640x360"};const char *rate[]={"","","40000","8000","4000","1500","720"};int n=static_cast<int>(quality);query+="&videoResolution="+std::string(res[n])+"&maxVideoBitrate="+rate[n]+"&videoQuality=100";}
    return query;
}
inline StreamPlan negotiate_video(Client &client,const std::string &base,const std::string &token,const Item &item,
                                  size_t version,Quality quality,bool local,const std::string &session,bool force=false) {
    if(item.type!="movie" && item.type!="episode")return {{Mode::Unsupported,"","Not a video item"},"",""};
    auto direct=direct_play(base,token,item,version);
    if(!force && direct.mode==Mode::DirectPlay && VideoCapabilities{}.original(item.media[version]) && (quality==Quality::Original || (quality==Quality::Automatic && local)))return {direct,"","Direct Play requested"};
    if(item.type!="movie" && item.type!="episode")return {{Mode::Unsupported,"","Not a video item"},"",""};
    if(!item.media.is_array() || version>=item.media.size() || !item.media[version].contains("Part") || item.media[version]["Part"].size()!=1)return {{Mode::Unsupported,"","Multipart video is not implemented"},"",""};
    if(!valid_base(base,true) || item.id.empty() || session.empty() || session.size()>128 || session.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_")!=std::string::npos)return {{Mode::Unsupported,"","Invalid playback session"},"",""};
    auto query=universal_query(item,version,session,quality,local);
    auto decision=client.call("GET",base,"/video/:/transcode/universal/decision"+query,token);
    if(!decision)return {{Mode::Unsupported,"",decision.error},"",""};
    auto mc=decision.data.find("MediaContainer");
    if(mc==decision.data.end() || !mc->is_object())return {{Mode::Unsupported,"","No server playback decision"},"",""};
    int64_t code=number(*mc,"generalDecisionCode",number(*mc,"mdeDecisionCode",-1));
    if(code<1000 || code>=2000)return {{Mode::Unsupported,"","Server rejected streaming decision (code "+std::to_string(code)+")"},"",""};
    auto entries=mc->find("Metadata");
    if(entries==mc->end() || !entries->is_array() || entries->empty())return {{Mode::Unsupported,"","Server decision has no media"},"",""};
    const auto &entry=(*entries)[0];if(!text(entry,"ratingKey").empty() && text(entry,"ratingKey")!=item.id)return {{Mode::Unsupported,"","Server decision identity mismatch"},"",""};auto versions=entry.find("Media");
    if(versions==entry.end() || !versions->is_array() || versions->empty())return {{Mode::Unsupported,"","Server decision has no media version"},"",""};
    const auto &media=(*versions)[0];std::string video=text(media,"videoDecision"),audio=text(media,"audioDecision"),codec=text(media,"videoCodec"),output_audio=text(media,"audioCodec");
    auto parts=media.find("Part");
    if(parts==media.end() || !parts->is_array() || parts->size()!=1 || !(*parts)[0].is_object())return {{Mode::Unsupported,"","Server output requires exactly one media part"},"",""};
    {
        if(video.empty())video=text((*parts)[0],"videoDecision");
        if(audio.empty())audio=text((*parts)[0],"audioDecision");
        auto streams=(*parts)[0].find("Stream");
        if(streams!=(*parts)[0].end() && streams->is_array())for(const auto &s:*streams){if(number(s,"streamType")==3 && text(s,"decision")=="burn")return {{Mode::Unsupported,"","Server would burn subtitles; select Original quality to keep client subtitle controls"},"",""};if(number(s,"streamType")==1){video=text(s,"decision",video);codec=text(s,"codec",codec);}if(number(s,"streamType")==2){audio=text(s,"decision",audio);output_audio=text(s,"codec",output_audio);}}
    }
    if(!VideoCapabilities{}.video(codec))return {{Mode::Unsupported,"","Server selected an unsupported video output codec"},"",""};
    if(video!="copy" && video!="transcode")return {{Mode::Unsupported,"","Server did not identify copy/transcode behavior"},"",""};
    if(audio!="copy" && audio!="transcode" && !audio.empty())return {{Mode::Unsupported,"","Unsupported server audio decision"},"",""};
    if(number(media,"width")>1920 || number(media,"height")>1080)return {{Mode::Unsupported,"","Server output exceeds the 1080p policy"},"",""};
    if(!VideoCapabilities{}.audio(output_audio))return {{Mode::Unsupported,"","Unsupported server audio output codec"},"",""};
    auto mode=(video=="copy" && audio!="transcode")?Mode::DirectStream:Mode::Transcode;
    return {{mode,base+"/video/:/transcode/universal/start.m3u8"+query+"&X-Plex-Token="+encode(token),""},session,mode==Mode::DirectStream?"Server decision: Direct Stream":"Server decision: transcode"};
}
// Same session identifier is used for decision, start, keepalive and stop.
inline Result stream_control(Client &client,const std::string &base,const std::string &token,const std::string &session,bool stop) {
    if(session.empty() || session.size()>128 || session.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_")!=std::string::npos)return {{},"Invalid streaming session",0};
    return client.call("GET",base,std::string("/video/:/transcode/universal/")+(stop?"stop":"ping")+"?session="+encode(session),token);
}
inline Playback audio_transcode(const std::string &base,const std::string &token,const Item &item,const std::string &session) {
    if(!audio_type(item) || !valid_base(base,true) || item.id.empty() || !item.media.is_array() || item.media.empty() || !item.media[0].contains("Part") || item.media[0]["Part"].size()!=1)return {Mode::Unsupported,"","Invalid audio transcode request"};
    return {Mode::Transcode,base+"/music/:/transcode/universal/start.m3u8?path="+encode("/library/metadata/"+item.id)+"&session="+encode(session)+"&protocol=hls&directPlay=0&directStream=0&audioCodec=aac&audioBitrate=320&X-Plex-Token="+encode(token),""};
}
inline std::string external_subtitle(const std::string &base,const std::string &token,const Json &stream) {
    auto key=text(stream,"key"),codec=text(stream,"codec");
    if(number(stream,"streamType")!=3 || (codec!="srt" && codec!="webvtt" && codec!="vtt" && codec!="ass" && codec!="ssa"))return "";
    if(!valid_base(base,true) || key.rfind("/library/streams/",0)!=0 || key.find_first_of("?#\r\n")!=std::string::npos || key.find("..")!=std::string::npos)return "";
    return base+key+"?X-Plex-Token="+encode(token);
}
}
