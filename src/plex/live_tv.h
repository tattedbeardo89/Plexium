// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "home.h"
#include <set>
namespace plex {
inline std::string live_label(std::string value){if(value.size()>256)value.resize(256);for(char &c:value)if(static_cast<unsigned char>(c)<32)c=' ';return value;}
inline bool live_identifier(const std::string &id){return !id.empty() && id.size()<=128 && id.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-")==std::string::npos;}
inline std::string live_provider(const std::string &id) {
    for(const char *kind:{"cloud","xmltv","custom"}) {
        auto prefix=std::string("tv.plex.providers.epg.")+kind+":";
        if(id.rfind(prefix,0)==0){auto suffix=id.substr(prefix.size());if(!suffix.empty() && suffix.size()<=16 && suffix.find_first_not_of("0123456789")==std::string::npos)return "/"+id;}
    }
    return {};
}
inline bool live_date(const std::string &date){
 if(date.size()!=10 || date[4]!='-' || date[7]!='-')return false;
 for(size_t i=0;i<date.size();++i)if(i!=4&&i!=7&&(date[i]<'0'||date[i]>'9'))return false;
 int year=0;for(int i=0;i<4;++i)year=year*10+date[i]-'0';
 int month=(date[5]-'0')*10+date[6]-'0',day=(date[8]-'0')*10+date[9]-'0';
 const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
 return year>=1970&&month>=1&&month<=12&&day>=1&&day<=days[month-1]+(month==2&&year%4==0&&(year%100!=0||year%400==0));
}
inline Result live_channels(Client &client,const std::string &base,const std::string &token) {
    auto dvrs=client.call("GET",base,"/livetv/dvrs",token);if(!dvrs)return dvrs;
    std::set<std::string> authorized;
    auto mc=dvrs.data.find("MediaContainer");
    if(mc!=dvrs.data.end() && mc->is_object())for(const char *name:{"DVR","Dvr"}) {
        auto list=mc->find(name);if(list!=mc->end() && list->is_array())for(const auto &dvr:*list){if(!dvr.contains("key"))continue;auto key=text(dvr,"key",std::to_string(number(dvr,"key")));if(live_identifier(key))authorized.insert(key);}
    }
    if(mc==dvrs.data.end() || !mc->is_object())return {{},"Malformed DVR discovery response",dvrs.status};
    bool shaped=false;
    for(const char *name:{"DVR","Dvr"}){auto list=mc->find(name);if(list!=mc->end()){if(!list->is_array())return {{},"Malformed DVR list",dvrs.status};shaped=true;}}
    if(authorized.empty()){
        if(!shaped && number(*mc,"size",-1)!=0)return {{},"Unsupported DVR discovery response",dvrs.status};
        return {Json{{"channels",Json::array()},{"configured",false}},"",200};
    }
    auto providers=client.call("GET",base,"/media/providers",token);if(!providers)return providers;
    Result lineup_failure;bool failed=false;
    Json channels=Json::array();std::set<std::string> seen;int requests=0;bool truncated=false;mc=providers.data.find("MediaContainer");
    if(mc==providers.data.end() || !mc->is_object())return {{},"Malformed Plex media providers response",providers.status};
    auto provider_list=mc->find("MediaProvider");
    if(provider_list==mc->end() || !provider_list->is_array())return {{},"Unsupported Plex media providers response",providers.status};
    if(mc!=providers.data.end() && mc->is_object()) {
        auto list=mc->find("MediaProvider");int queried=0;
        if(list!=mc->end() && list->is_array())for(const auto &provider:*list) {
            auto id=text(provider,"identifier"),path=live_provider(id);if(path.empty())continue;
            auto dvr=id.substr(id.rfind(':')+1);if(!authorized.count(dvr) || ++queried>4)continue;
            std::string previous;
            for(int start=0;start<1600;++requests,start+=200){
            if(requests>=8){truncated=true;break;}
            auto response=client.call("GET",base,path+"/lineups/dvr/channels?X-Plex-Container-Size=200&X-Plex-Container-Start="+std::to_string(start),token);
            if(!response){failed=true;lineup_failure=response;break;}
            auto container=response.data.find("MediaContainer");
            if(container==response.data.end() || !container->is_object()){
                failed=true;lineup_failure={{},"Malformed Plex channel lineup",response.status};break;
            }
            auto entries=container->find("Channel");
            if(entries==container->end() && number(*container,"size",-1)==0)break;
            if(entries==container->end() || !entries->is_array()){
                failed=true;lineup_failure={{},"Unsupported Plex channel lineup",response.status};break;
            }
            auto first=entries->empty()?std::string():text((*entries)[0],"id",std::to_string(number((*entries)[0],"id",-1)));
            if(start>0&&first==previous){truncated=true;break;}previous=first;
            for(const auto &channel:*entries) {
                if(!channel.contains("id"))continue;
                auto key=text(channel,"id",std::to_string(number(channel,"id"))),grid=text(channel,"gridKey");
                if(!live_identifier(key) || !live_identifier(grid) || channels.size()>=1600||!seen.insert(id+"/"+key).second)continue;
                channels.push_back({{"id",key},{"gridKey",grid},{"provider",id},{"dvr",dvr},{"title",live_label(text(channel,"title",text(channel,"callSign","Channel")))},{"number",text(channel,"vcn")},{"thumb",text(channel,"thumb")}});
            }
            if(entries->empty()||start+int(entries->size())>=number(*container,"totalSize",start+int(entries->size())))break;
            }
        }
    }
    if(channels.empty()&&failed)return lineup_failure;
    return {Json{{"channels",std::move(channels)},{"configured",true},{"truncated",truncated}},"",200};
}
inline Result live_guide(Client &client,const std::string &base,const std::string &token,const Json &channel,const std::string &date,int64_t window_start=0,int64_t window_end=0) {
    auto provider=live_provider(text(channel,"provider")),grid=text(channel,"gridKey");
    if(provider.empty() || !live_identifier(grid) || !live_date(date))return {{},"Invalid authorized guide selection",0};
    if((window_start || window_end) && (window_start<=0 || window_end<=window_start || window_end-window_start>6*3600))return {{},"Invalid guide time window",0};
    auto result=client.call("GET",base,provider+"/grid?channelGridKey="+encode(grid)+"&date="+encode(date),token);if(!result)return result;
    Json programs=Json::array();auto mc=result.data.find("MediaContainer");
    if(mc==result.data.end() || !mc->is_object())return {{},"Malformed Plex guide response",result.status};
    auto metadata=mc->find("Metadata");
    if(metadata==mc->end() && number(*mc,"size",-1)==0)return {Json{{"programs",std::move(programs)}},"",200};
    if(metadata==mc->end() || !metadata->is_array())return {{},"Unsupported Plex guide response",result.status};
    if(mc!=result.data.end() && mc->is_object()) {
        auto metadata=mc->find("Metadata");if(metadata!=mc->end() && metadata->is_array())for(const auto &item:*metadata) {
            auto media=item.find("Media");if(media==item.end() || !media->is_array())continue;
            for(const auto &airing:*media) {
                auto begins=number(airing,"beginsAt"),ends=number(airing,"endsAt");if(begins<=0 || ends<=begins || programs.size()>=100)continue;
                // Apply the viewport before the cap: earlier daily airings
                // must not displace the current page in a large Plex grid.
                if(window_start && (ends<=window_start || begins>=window_end))continue;
                programs.push_back({{"title",live_label(text(item,"title","Program"))},{"series",live_label(text(item,"grandparentTitle"))},{"summary",live_label(text(item,"summary"))},{"beginsAt",begins},{"endsAt",ends}});
            }
        }
    }
    std::sort(programs.begin(),programs.end(),[](const Json &a,const Json &b){return number(a,"beginsAt")<number(b,"beginsAt");});
    return {Json{{"programs",std::move(programs)}},"",200};
}
inline Result live_tune(Client &client,const std::string &base,const std::string &token,const Json &channel,const std::string &consumer) {
    // Documentation establishes tune and playlist endpoints, but not the
    // consumer acquisition/heartbeat/release contract used by this client.
    // Do not allocate a tuner until an authorized real-response adapter proves
    // that complete lifecycle. Never synthesize a playable URL from a UUID.
    (void)client;(void)base;(void)token;(void)channel;(void)consumer;
    return {{},"Personal Live TV playback is pending verified Plex consumer/session lifecycle support",0};
}
// PMS OpenAPI 1.2.3 Subscriptions: list recording rules and scheduled grabs.
inline Result live_recordings(Client &client,const std::string &base,const std::string &token) {
    auto rules=client.call("GET",base,"/media/subscriptions?includeGrabs=1&includeStorage=1",token);
    if(!rules)return rules;
    auto schedule=client.call("GET",base,"/media/subscriptions/scheduled",token);
    if(!schedule)return schedule;
    Json subscriptions=Json::array(),scheduled=Json::array();
    auto mc=rules.data.find("MediaContainer");
    if(mc==rules.data.end() || !mc->is_object())return {{},"Unsupported recording rules response",rules.status};
    auto entries=mc->find("MediaSubscription");
    if(entries!=mc->end() && !entries->is_array())return {{},"Malformed recording rules response",rules.status};
    if(entries!=mc->end())for(const auto &rule:*entries){
        auto key=text(rule,"key",std::to_string(number(rule,"key",-1)));
        if(number(rule,"targetLibrarySectionID")<=0 || number(rule,"type")<1 || number(rule,"type")>4 || key.empty() || key.size()>18 || key.find_first_not_of("0123456789")!=std::string::npos || subscriptions.size()>=200)continue;
        subscriptions.push_back({{"id",key},{"title",live_label(text(rule,"title","Recording rule"))},{"section",number(rule,"targetLibrarySectionID")}});
    }
    mc=schedule.data.find("MediaContainer");
    if(mc==schedule.data.end() || !mc->is_object())return {{},"Unsupported recording schedule response",schedule.status};
    entries=mc->find("MediaGrabOperation");
    if(entries!=mc->end() && !entries->is_array())return {{},"Malformed recording schedule response",schedule.status};
    if(entries!=mc->end())for(const auto &grab:*entries){
        auto metadata=grab.find("Metadata");if(metadata==grab.end() || !metadata->is_object() || scheduled.size()>=200)continue;
        int64_t begins=0;auto media=metadata->find("Media");if(media!=metadata->end() && media->is_array() && !media->empty())begins=number((*media)[0],"beginsAt");
        scheduled.push_back({{"title",live_label(text(*metadata,"title","Scheduled recording"))},{"series",live_label(text(*metadata,"grandparentTitle"))},{"status",live_label(text(grab,"status","Unknown"))},{"beginsAt",begins}});
    }
    return {Json{{"subscriptions",std::move(subscriptions)},{"scheduled",std::move(scheduled)}},"",200};
}
// Read-only SDK-established endpoint; unsupported PMS versions may return 404.
inline Result completed_recordings(Client &client,const std::string &base,const std::string &token){
 return client.browse(base,token,"/livetv/recordings",0,60);
}
inline Result cancel_recording_rule(Client &client,const std::string &base,const std::string &token,const std::string &id) {
    // Re-read using the current profile. Never delete an arbitrary stale/foreign key.
    auto current=live_recordings(client,base,token);if(!current)return current;
    for(const auto &rule:current.data["subscriptions"])if(text(rule,"id")==id)
        return client.call("DELETE",base,"/media/subscriptions/"+encode(id),token);
    return {{},"Recording rule is no longer available to this profile",0};
}

}
