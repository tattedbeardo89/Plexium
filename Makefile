CXX := g++
CC := gcc
OUT := build$(if $(filter 1,$(SANITIZE)),/san,)
OBJ := build/obj$(if $(filter 1,$(SANITIZE)),-san,)
CXXFLAGS := -std=c++17 -D_FORTIFY_SOURCE=3 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation -Isrc -Ithird_party -Ithird_party/imgui -pthread
LDFLAGS := -pthread
ifeq ($(SANITIZE),1)
 CXXFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
 LDFLAGS += -fsanitize=address,undefined
endif
CORE := $(OBJ)/src/plex/client.o $(OBJ)/third_party/tinyxml2/tinyxml2.o
APP := $(OBJ)/src/app/session.o
IMGUI := $(addprefix $(OBJ)/third_party/imgui/,$(addsuffix .o,imgui imgui_draw imgui_widgets imgui_tables))
SUITES := dvr-integration-tests live-session-tests episode-continuation-tests durable-progress-tests track-selection-tests progress-journal-tests playback-integration-tests  artwork-tests session-restore-tests state-tests guide-tests startup-runtime-tests runtime-tests session-tests protocol-tests auth-tests home-tests home-feed-tests
.PHONY: all test clean
all: $(addprefix $(OUT)/,$(SUITES)) $(OUT)/plexium-render $(OUT)/transport-probe $(OUT)/plexium-cli $(OUT)/plexium-server-check
$(OUT)/artwork-tests: $(CORE) $(OBJ)/src/plex/artwork.o $(OBJ)/tests/artwork_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) $(shell pkg-config --libs gnutls) -o $@
$(OBJ)/src/plex/artwork.o $(OBJ)/tests/artwork_tests.o: CXXFLAGS += -Ithird_party/stb $(shell pkg-config --cflags gnutls)
$(OBJ)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@
$(OBJ)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@
$(OBJ)/qrcodegen.o: third_party/qrcodegen/qrcodegen.c
	@mkdir -p $(dir $@)
	$(CC) -O1 -g -c $< -o $@
$(OUT)/session-restore-tests: $(CORE) $(APP) $(OBJ)/tests/session_restore_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/state-tests: $(CORE) $(OBJ)/tests/state_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/guide-tests: $(CORE) $(OBJ)/tests/guide_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/startup-runtime-tests: $(OBJ)/tests/startup_runtime_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/runtime-tests: $(CORE) $(APP) $(OBJ)/tests/runtime_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/session-tests: $(CORE) $(APP) $(OBJ)/tests/session_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/protocol-tests: $(CORE) $(OBJ)/tests/plex_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/auth-tests: $(CORE) $(OBJ)/qrcodegen.o $(OBJ)/tests/auth_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/home-tests: $(CORE) $(OBJ)/tests/home_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/home-feed-tests: $(CORE) $(OBJ)/tests/home_feed_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/transport-probe: $(CORE) $(OBJ)/src/plex/transport_curl.o $(OBJ)/tests/transport_probe.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) $(shell pkg-config --libs libcurl) -o $@
$(OUT)/plexium-cli: $(CORE) $(APP) $(OBJ)/src/plex/transport_curl.o $(OBJ)/tools/plexium-cli.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) $(shell pkg-config --libs libcurl) -o $@
$(OUT)/plexium-render: $(CORE) $(APP) $(IMGUI) $(OBJ)/src/app/view.o $(OBJ)/tools/render-host.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
$(OBJ)/src/plex/transport_curl.o: CXXFLAGS += $(shell pkg-config --cflags libcurl)
-include $(shell find $(OBJ) -name '*.d' 2>/dev/null)

$(OUT)/playback-integration-tests: $(CORE) $(APP) $(OBJ)/tests/playback_integration_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@

$(OBJ)/tests/host_vlc_tests.o: CXXFLAGS += $(if $(VLC_HOST_CFLAGS),$(VLC_HOST_CFLAGS),$(shell pkg-config --cflags libvlc fontconfig pixman-1)) -fopenmp
$(OUT)/host-vlc-tests: $(CORE) $(APP) $(OBJ)/tests/host_vlc_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) $(if $(VLC_HOST_LIBS),$(VLC_HOST_LIBS),$(shell pkg-config --libs libvlc fontconfig pixman-1)) -l:libfontconfig.so.1 -l:libpixman-1.so.0 -fopenmp -o $@

$(OUT)/track-selection-tests: $(CORE) $(OBJ)/tests/track_selection_tests.o
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/progress-journal-tests: $(CORE) $(OBJ)/tests/progress_journal_tests.o
	$(CXX) $^ $(LDFLAGS) -o $@

$(OUT)/episode-continuation-tests: $(CORE) $(APP) $(OBJ)/tests/episode_continuation_tests.o
	$(CXX) $^ $(LDFLAGS) -o $@
$(OUT)/durable-progress-tests: $(CORE) $(APP) $(OBJ)/tests/durable_progress_tests.o
	$(CXX) $^ $(LDFLAGS) -o $@

$(OUT)/live-session-tests: $(CORE) $(OBJ)/tests/live_session_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@

$(OUT)/plexium-server-check: $(CORE) $(OBJ)/src/plex/transport_curl.o $(OBJ)/tools/plexium-server-check.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) $(shell pkg-config --libs libcurl) -o $@

$(OUT)/dvr-integration-tests: $(CORE) $(APP) $(OBJ)/tests/dvr_integration_tests.o
	@mkdir -p $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@
