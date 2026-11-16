# 4K source review and bounded surface preparation

This work does not enable 4K playback, change the 1080p negotiation cap or modify
PS5 decoder ownership. No console or private PMS/ErsatzTV is contacted.

The read-only Emby5 reference's v1.3.1 release notes describe reclaiming idle resident
decoders when a 4K grow fails. In `app/engine/media/src/evo_vdec_native.c`,
`evict_idle_slots` holds its eviction mutex, atomically claims an idle slot, tears down
that particular backend-owned slot and later restores it at boot dimensions. Its
Main10 path initially allocates at 1080p and grows on a 4K access unit; it documents
large resident decoder allocations and padded surface/CTB geometry.

Plexium uses a different VLC/PS5 decoder substrate in `ps5/modules/ps5vdec.c` and
native video ownership in `src/player.cc`. Its shared CPU/queue arena, cache lifetime
and driver completion contracts remain unresolved. An Emby5 idle-slot claim is not
proof that those Plexium arenas or caches may be released. No source, ABI workaround,
eviction/free sequence or driver/kernel call is copied into Plexium.

`src/plex/surface_budget.h` adds overflow-safe NV12/P010 **surface-only** arithmetic
for pressure scenarios. Pitch is measured in luma samples, height must include actual
padding, Main10 samples occupy 16 bits, interleaved UV contributes half the luma plane,
and frame count is bounded. The helper rejects malformed dimensions, multiplication
overflow and budgets below the lower-bound storage requirement. It does not allocate
or release any decoder/GPU resource. Tests cover an eight-frame 1080p pool, a budget
rejection for a sixteen-frame UHD Main10 pool and extreme-size overflow.

Surface storage excludes codec/driver arenas, command queues, decoded-picture buffer
policy, reference frames outside the supplied count, bitstreams, shader intermediates,
output surfaces and presentation retention. It cannot predict successful native decoder
allocation, justify reclaiming an idle arena or establish a total 4K memory budget.
A future implementation needs verified SDK ownership/release contracts and separately
authorized physical acceptance. Stable 1080p behavior and safe retention take priority.
