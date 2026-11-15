# PCM continuity and bounded host soak

This measures actual libVLC decoding through the production `Session` music queue,
not metadata-only state changes. Fixtures are local generated 997 Hz sine tones,
0.65 seconds each, AAC/MP3/FLAC/Vorbis, rendered as S16N 48 kHz stereo. Plex URLs
are explicitly mapped to those local files by a synthetic transport/player adapter.
No real PMS, private network, speaker device or PS5 is involved.

## Measurement contract

`tools/audio-probe.h` records one bounded event per decoded PCM block: generation,
first/last interleaved stereo sample values, presentation timestamp, steady-clock
callback timestamp, frame count, exact-zero frames and contiguous zero runs. It
keeps at most 8192 blocks between drains, with explicit overflow failure. Only the
first 32 transition edges are retained in the report; aggregate timing uses all.

The opt-in `HostVlcPlayer::enable_measurements` sink drains against libVLC's clock.
Stop cancels the drain before joining VLC. This prevents the earlier count-only
adapter from ending a track as soon as decoding runs ahead of presentation. It is
a **paced silent sink**, not an emulated speaker or verified native audio device.
EncounteredError is latched independently of the player's sometimes transient state.

Reports distinguish:

- **Callback delivery gap:** first incoming callback minus last outgoing callback.
  Decoders deliver ahead, so this is not the silence heard by a listener.
- **Scheduled PCM gap:** incoming first PTS minus outgoing last-block end PTS,
  mapped through contemporaneous libVLC/steady-clock pairs. Negative values mean
  overlapping schedules, not proven output mixing or seamless playback.
- **Decoder startup:** first incoming PCM callback minus player open entry.
- **End detection:** Session poll start minus libVLC EndReached event time.
- **Queue advance:** Session Tick duration including durable state and player work.
- **Exact zero frames / longest run / intra-track PTS holes:** PCM observations;
  isolated zero crossings are not audible silence. A PTS hole is over two 48 kHz
  samples between blocks, not a hardware underrun counter.
- **Device underruns / actual output time:** unobservable; no device is opened.

End polling has a bounded 5 ms condition-driven interval and twelve-second failure
deadline. The PMS fixture clock is advanced explicitly for reconnect/backoff; PCM
measurement uses real monotonic time. A 200 ms timeline-response fault is injected
for every natural stopped update, even after the next player opens; the test asserts one delayed stop response per measured transition. This keeps the fault identical in both source paths.
Production moves durable stopped-progress transmission after the next open, before
new playing updates can replace it. Optional health checks also follow open, so
short repeated tracks do not starve authorization checks. No second native player,
decoder, transcoder or output port is created. Manual Stop/release semantics remain.

Quantiles use sorted empirical observations: median order statistic; P95 only for
at least twenty transitions and P99 only for at least one hundred. A short mandatory
run deliberately omits unsupported tail quantiles. CPU contention, codec lookahead,
encoder padding and the host scheduler influence the results. GPU/artwork loading is absent from this PCM fixture; its worker/retention contracts are covered separately, not measured here. No hardware performance
or guaranteed maximum delay is inferred from this machine.

## Commands and budgets

Dependencies: Linux/glibc, libVLC 3 development/runtime codec plugins, FFmpeg with
AAC/libmp3lame/FLAC/libvorbis, Fontconfig, Pixman, C++17 and OpenMP. The normal command
builds the actual adapter and generates authorized synthetic media:

```
bash host/test-audio-continuity.sh
SANITIZE=1 bash host/test-audio-continuity.sh
CONTINUITY_CYCLES=160 CONTINUITY_SECONDS=120 bash host/test-audio-continuity.sh
CONTINUITY_CYCLES=100 CONTINUITY_SECONDS=900 bash host/test-audio-continuity.sh
```

Cycles are bounded to 4–1000, duration to 0–3600 seconds. Both minima apply; the
loop ends only after both are satisfied. The optional manual `soak.yml` workflow
defaults to 900 seconds and does not gate every push. Standard host VLC CI runs
sixteen transitions normally and with ASan/UBSan. JSON and bounded logs are uploaded
with existing host-VLC diagnostics. `CONTINUITY_REPORT` can select a JSON path.

The soak samples `/proc/self/statm`, `/proc/self/fd`, `/proc/self/task` and glibc
`mallinfo2` after a four-codec warmup. It fails on growth over 32 MiB resident or
8 MiB allocator-in-use, more than four additional files or eight additional threads.
ASan uses its live-allocation counter for the same 8 MiB ownership-growth threshold and a separate 320 MiB RSS-growth envelope for the unchanged default freed-memory quarantine; its RSS is not glibc live storage. No quarantine, leak detector, sanitizer or error check is disabled. These are **test regression envelopes**, not production native budgets or exact
allocation ownership proofs. ASan/UBSan independently checks memory misuse; native
failure-state quarantine is separately exercised by lifecycle tests. Retained queue,
cache and pre-resolution accounting is checked against production bounds throughout.

The final phase checks pause/resume, seek, shuffle/repeat, duplicate queue edits,
background navigation, stop/reopen, private restart without autoplay, 32 reconnect
rounds, a deliberately missing local VLC input followed by reconnect, revocation,
profile isolation and shutdown during decoding. Transport status 0 combines DNS/TLS/
connection failures in this fixture; real loopback HTTP/TLS adapters separately test
TLS rejection, timeout and transport handling. This is not a real DNS outage or PMS
restart claim. No callback growth is accepted after synchronous Stop returns.

## Results and remaining boundary

Final source-specific results, sample counts, resources and held packages are linked
from draft PR #2. The baseline comparison builds the original checkpoint's Session,
feature and music translation units against the same current instrumented host
adapter and fixtures; it is a controlled source-path comparison, not a historical
console benchmark or an old native package execution.

True gapless output remains **PARTIAL / unproven**. Native `player.cc` still uses
single-player stop/open and its existing clocked output queue. No speculative decoder
reuse, port duplication or hardware ABI change is introduced. See
[readiness audit](RELEASE-READINESS.md) and [music contracts](MUSIC-RELIABILITY.md).

## Local measured result

Corrected measurements apply the same 200 ms stopped-response delay before and after
player replacement, with an asserted response count equal to the transition count.
The earlier conditional-delay fixture understated queue-command duration and is
superseded. Opening the next player can overlap the server wait with decoding; it
does not remove the server wait or promise a faster worker-command return.

The corrected run passed **160 transitions in 122.226 seconds**, with exactly 160
delayed stop responses, using VLC 3.0.23 on this Linux workspace. Four codecs
alternate; no input sample-rate/channel-layout change is claimed. Other compilation
and checks could contend for CPU.

| Observation (ms) | Median | P95 | P99 | Maximum |
| --- | ---: | ---: | ---: | ---: |
| Scheduled PCM gap | 94.989 | 212.211 | 217.247 | 218.967 |
| Decoder startup | 2.656 | 6.887 | 17.123 | 33.856 |
| End detection | 2.463 | 4.719 | 5.069 | 5.335 |
| Queue-command duration | 210.209 | 217.201 | 221.658 | 229.644 |

The controlled baseline source-path comparison received all 32 delayed stop
responses: scheduled-gap median 297.396 ms, P95 413.982 ms, maximum 417.874 ms;
queue-command median 210.892 ms, P95 217.655 ms. Baseline P99 is omitted because
only 32 observations exist. Decoding starts sooner while the worker still waits
for the same server response. The worker-command duration does not improve by
200 ms; there is no claim that network latency disappeared.

Across 4,912,640 decoded frames, 520 exact-zero frames were isolated zero crossings
(longest run one); there were zero intra-track PTS holes. No unexpected playback
failures or dropped probe blocks occurred. One missing-file error was injected
and recovered deliberately. All 32 reconnect rounds, revocation, restart, profile
isolation and shutdown checks passed. Physical output underruns remain unobservable.

Warmup-to-peak RSS: 48,418,816 → 48,492,544 bytes (+73,728). Glibc in-use storage:
18,226,416 → 18,325,520 (+99,104). Active file observations stayed six and threads
three; after stopped shutdown they were five and one, with allocator in-use
4,501,808 bytes. Stopped RSS 48,521,216 reflects allocator/library retention, not
an ownership proof. Raw JSON includes PCM endpoints, paired monotonic/PTS stamps,
callback-delivery statistics, resource samples and delayed-response counts.

The corrected normal and ASan/UBSan sixteen-transition runs also pass. Clocks are
paired before PCM scanning/lock delay. Final exact-source CI uploads fresh reports
and held packages; existing draft PR #2 links the final source identity/evidence.
