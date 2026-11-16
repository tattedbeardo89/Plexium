// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/session.h"
#include <iostream>
#include <sstream>
#include <termios.h>
#include <unistd.h>
using namespace plexium;
struct HostPlayer:PlaybackPort{bool open(const std::string&,int64_t)override{return false;}void stop()override{}};
int main(int argc,char**argv){
 if(argc!=2){std::cerr<<"Usage: plexium-cli PRIVATE_STATE_DIRECTORY\n";return 2;}
 HostPlayer player;Session session(plex::curl_transport(),argv[1],player);auto m=session.dispatch({Action::Restore,{},{}});
 std::cout<<"Plexium host protocol tool. Native video is available only in the PS5 target.\nCommands: link, poll, profile ID, server ID, home, library ID, details ID, children ID, search QUERY, back, signout, quit\n";
 for(;;){
  if(!m.message.empty())std::cout<<m.message<<'\n';if(!m.code.empty())std::cout<<"Open plex.tv/link and enter "<<m.code<<" ("<<m.expires<<"s)\n";
  for(const auto&p:m.profiles)if(m.screen==Screen::Profiles)std::cout<<p.id<<"  "<<p.name<<(p.protected_user?" (PIN)":"")<<'\n';
  for(const auto&s:m.servers)if(m.screen==Screen::Servers)std::cout<<s.id<<"  "<<s.label<<'\n';
  for(const auto&i:m.sections)if(m.screen==Screen::Home)std::cout<<"Library "<<i.id<<"  "<<i.title<<'\n';
  for(const auto&i:m.entries)std::cout<<i.id<<"  "<<i.title<<'\n';
  if(m.screen==Screen::Details)std::cout<<m.detail.title<<"\n"<<m.detail.summary<<'\n';
  std::cout<<"> "<<std::flush;std::string line;if(!std::getline(std::cin,line))break;std::istringstream input(line);std::string verb,id;input>>verb>>id;
  if(verb=="quit")break;Command c{Action::Back,id,{}};
  if(verb=="link")c.action=Action::Link;else if(verb=="poll")c.action=Action::Poll;
  else if(verb=="profile"){
   c.action=Action::Profile;auto p=std::find_if(m.profiles.begin(),m.profiles.end(),[&](auto&p){return p.id==id;});
   if(p!=m.profiles.end()&&p->protected_user){if(!isatty(STDIN_FILENO)){std::cerr<<"PIN entry requires an interactive terminal\n";continue;}
    termios before{};if(tcgetattr(STDIN_FILENO,&before)!=0)return 1;auto hidden=before;hidden.c_lflag&=~ECHO;
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&hidden)!=0)return 1;std::cout<<"PIN: "<<std::flush;std::getline(std::cin,c.text);tcsetattr(STDIN_FILENO,TCSAFLUSH,&before);std::cout<<'\n';}
  }else if(verb=="server")c.action=Action::Server;else if(verb=="home")c.action=Action::Home;
  else if(verb=="library")c.action=Action::Library;else if(verb=="details")c.action=Action::Details;else if(verb=="children")c.action=Action::Children;
  else if(verb=="search"){c.action=Action::Search;c.text=line.substr(std::min(line.size(),size_t(7)));}
  else if(verb=="signout")c.action=Action::SignOut;else if(verb!="back"){std::cout<<"Unknown command\n";continue;}
  m=session.dispatch(c);
 }
}
