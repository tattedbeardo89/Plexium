// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
namespace plexium {
struct Reconnect {
 bool offline=false,blocked=false;unsigned attempts=0;double due=0,probe=0;
 void reset(double now){offline=blocked=false;attempts=0;due=0;probe=now+30;}
 void lost(double now,int status){if(status==401||status==403){blocked=true;offline=true;return;}if(!offline){offline=true;attempts=0;due=now+2;}}
 bool ready(double now)const{return offline&&!blocked&&attempts<6&&now>=due;}
 void failed(double now){++attempts;due=now+std::min(60u,2u<<std::min(attempts,5u));}
};
}
