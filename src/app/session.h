// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "plex/client.h"
#include "plex/auth.h"
#include "plex/home.h"
#include "plex/home_feed.h"
#include "plex/guide.h"
#include "plex/streaming.h"
#include "plex/progress_outbox.h"
#include "plex/progress_journal.h"
#include "plex/tracks.h"
#include <deque>
#include <thread>
#include "runtime/owned_jobs.h"
#include "runtime/paths.h"
#include <mutex>
#include <condition_variable>
#include <memory>
namespace plexium {
inline constexpr const char *title_id="PPSA85421";
inline constexpr const char *data_path=PLEX_STATE_ROOT;
enum class PlaybackState {Idle,Resolving,Opening,Buffering,Playing,Paused,Seeking,Stopping,Stopped,Error,Released};
struct PlaybackObservation {bool known=false,playing=false,buffering=false,paused=false,ended=false,failed=false;};
enum class Screen { Welcome, Linking, Profiles, Pin, Servers, Home, Library, Details, Settings, Search, Playback, Guide, Tracks };
enum class Action { Restore, Link, Poll, Cancel, Profile, Server, Home, Library, Browse, Page, Details, Children, Search, Play, FreeTV, PlayLive, Stop, Settings, SignOut, Back, Pause, Seek, Audio, Subtitle, Tick, LiveTV, GuidePage, TuneChannel, Version, Quality, Tracks, SelectAudio, SelectSubtitle, AutoPlay, CancelNext, PlayNext, SkipMarker };
struct Command { Action action; std::string id, text; };
struct MediaContext {std::string base,token,scope,account_token;};
struct Destination {std::string id,label;};
struct Model {
 Screen screen=Screen::Welcome;
 std::string message,code,profile_name,profile_avatar,server_name,query;
 std::vector<plex::Profile> profiles;
 std::vector<Destination> servers, navigation;
 std::vector<plex::Item> sections, entries;
 std::vector<plex::Hub> rails;
 plex::Guide guide;
 plex::Item detail;
 std::vector<plex::MediaTrack> audio_tracks,subtitle_tracks;
 plex::TrackSelection tracks;
 std::vector<plex::Item> upcoming;
 int autoplay_remaining=0;bool autoplay=true;bool tracks_during_playback=false;
 std::string library_id,library_title,library_type,library_tab,library_path;
 std::vector<plex::Hub> detail_rails;
 int page_start=0,total=0;
 PlaybackState playback_state=PlaybackState::Idle;
 plex::Mode playback_mode=plex::Mode::Unsupported;
 plex::Quality quality=plex::Quality::Original;
 size_t media_version=0;
 bool books=false,paused=false,buffering=false;
 bool busy=false;
 int64_t expires=0;
};
std::vector<Destination> navigation(const std::vector<plex::Item>&);
std::string poster(const plex::Item&);
class PlaybackPort {
 public: virtual ~PlaybackPort()=default;
 virtual bool open(const std::string&,int64_t)=0;
 virtual void prepare_audio(const plex::MediaTrack*){}
 virtual void stop()=0;
 virtual int64_t position() const {return 0;}
 virtual void pause() {}
 virtual bool paused() const {return false;}
 virtual void seek(int64_t) {}
 virtual PlaybackObservation observe() const {return {};}
 virtual void audio_next() {}
 virtual void subtitle_next() {}
 virtual std::vector<plex::NativeTrack> native_tracks(bool)const{return {};}
 virtual bool select_native_track(bool subtitle,int id){return subtitle&&id<0;}
 virtual bool add_external_subtitle(const std::string&){return false;}
};
// All network/storage/player operations execute on one worker, never the render thread.
class Session {
 plex::Client client;
 std::function<double()> clock;
 std::string root,device,account,account_id,active,base,token,server_id,current_profile;
 std::vector<plex::Server> resources;
 plex::AuthSession auth;
 Model model;
 PlaybackPort &player;
 std::vector<Model> history;
 std::vector<plex::Hub> cached_home;
 double home_loaded=0;
 struct CachedPage {Model value;double loaded=0;};
 std::map<std::string,CachedPage> pages;
 std::map<std::string,std::pair<plex::Item,double>> metadata;
 std::vector<plex::Item> channels;
 std::map<std::string,std::string> live_parts;
 double channels_loaded=0;
 plex::Json personal_channels=plex::Json::array();
 double personal_loaded=0;
 plex::StreamPlan stream;
 std::string stream_base,stream_token;
 plex::ProgressOutbox outbox;
 plex::ProgressJournal journal;
 std::string playback_session,last_ack_session,last_ack_id,last_ack_state,track_preferences_scope;
 int64_t playback_baseline=0,playback_viewed=0,last_ack_position=-1;
 bool tracks_pending=false,external_applied=false;double next_deadline=0;
 std::string audio_language,subtitle_language;bool prefer_subtitles=false;
 void load_tracks();
 void apply_tracks();
 void save_preferences();
 void prepare_next();
 bool play_next();
 bool playback_started=false,local_connection=false;
 uint64_t playback_serial=0;
 double heartbeat_at=0;
 void persist_progress();
 void report_progress(const std::string&,int64_t);
 void flush_progress();
 bool close_stream();
 bool open_playback(bool force=false);
 double now() const;
 void fail(const char*,int=0);
 bool profiles(bool restore_saved=false);
 bool select_profile(const std::string&,const std::string&);
 bool discover();
 bool connect(const std::string&);
 bool home(bool refresh=false);
 bool library(const std::string&,const std::string&);
 bool listing(const std::string&,int start=0);
 bool free_tv(bool refresh=false);
 bool live_tv(int offset,int64_t begins,bool refresh=false);
 void channel_page(int);
 void remember_position(int64_t);
 bool private_root();
 friend class Runtime;
 const Model &dispatch_current(const Command&);
 void clear_context();
 public:
 Session(plex::Transport,const std::string&,PlaybackPort&,std::function<double()> monotonic_clock={});
 Model dispatch(const Command&);
 void shutdown();
 std::function<void(const Model&)> progress;
 MediaContext media_context()const;
};
class Runtime {
 std::unique_ptr<Session> session;
 mutable std::mutex mutex;
 std::condition_variable cv;
 std::deque<std::pair<uint64_t,Command>> queue;
 Model model;
 MediaContext context;
 uint64_t generation=0;
 bool closing=false;
 OwnedJob worker;
 int worker_error=0;
 void run();
 public:
 explicit Runtime(std::unique_ptr<Session>);
 ~Runtime();
 // Owner-thread operation: close the render queue before joining this worker.
 bool close(std::chrono::milliseconds timeout=std::chrono::milliseconds(2000));
 bool ready()const{std::lock_guard<std::mutex> lock(mutex);return !closing&&worker_error==0;}
 int start_error()const{return worker_error;}
 bool send(Command);
 Model snapshot() const;
 // Copy into reusable caller-owned storage; model and credentials share one epoch.
 void snapshot_into(Model&,MediaContext&) const;
 MediaContext media_context() const;
};
}
