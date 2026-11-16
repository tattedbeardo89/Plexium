# Mixed-format audio and held acceptance candidate

Baseline: `94d0179d9441ff91611f26eb88c7e95cb7451e4e`. This extends the actual Session,
VLC adapter and native player; no private PMS or console is used. Final exact-source
CI, measured results and held artifact identities belong to existing draft PR #2.

## Input, negotiation and PCM evidence

`tools/generate-mixed-audio.py` generates eight authorized files and independently
inspects them with ffprobe: 44.1 kHz AAC stereo, 48 kHz MP3 stereo, 96 kHz FLAC stereo,
48 kHz Vorbis mono, 48 kHz FLAC 5.1, 48 kHz signed-16 PCM WAV, 96 kHz signed-24 PCM WAV
and 44.1 kHz float PCM WAV. Durations vary from 0.45 to 0.8 seconds. A sixteen-slot
sequence covers rate/layout/codec changes in both directions. The separate control
phase uses eight-second AAC and a one-second synthetic video for audio-focus testing.

ffprobe validates the encoded stream's rate, channels/layout, codec and decoded
sample-format description, independently of names and synthetic Plex metadata.
The actual libVLC setup callback observes rate/channels and explicitly negotiates
S16N PCM. Every outgoing/incoming block is checked against the source matrix's
rate/channel expectation. Callback endpoints retain the first two channels (duplicated for mono), not an entire PCM recording. The callback API reports channel count, not a speaker-routing mask; source 5.1 layout is inspected independently and physical routing remains unverified. Mono, stereo and six-channel stride and zero-run scans are
rate/channel aware. This observes actual callback PCM, not just container labels.
VLC's setup format is a negotiation value, not a claim about an internal decoder's
floating-point representation. Source precision conversions are deliberately S16N.

The mixed host mode retains source rate/channels to exercise format changes; the
normal host adapter still uses canonical 48 kHz stereo. The native contract remains
48 kHz S16, stereo or its existing verified-by-source 6/8-channel selection, with
physical acceptance pending. No PS5 API, parallel decoder or new output port is added.
Invalid source rate/channel bounds fail negotiation. New native format acceptance
flushes only copied CPU PCM under its existing mutex, resets queued-frame accounting
and preserves explicit pause state. It never frees decoder/GPU/port quarantine.

## Timing and calibrated fault

The [PCM contract](AUDIO-CONTINUITY.md) remains: sample endpoints, generation, callback
steady-clock stamps, paired VLC PTS/clock, rate-aware frame durations, zero runs and
PTS holes (42 microsecond tolerance). Reports separate first PCM startup, end-event
poll latency, queue-command duration, player stop/release, open call and scheduled
output lead. Stop timing is the actual adapter stop, including its retained single
player; opening does not create another player instance.

Every natural stopped response receives the same 200 ms injected delay, regardless
of whether replacement playback has opened. Delayed-response count must equal
recorded transitions. Exactly one observed end drives each record; there is no
unmeasured warmup Tick. Decode startup, network waiting and scheduled output lead
are distinct. Speaker/device continuity and hardware underruns remain unobservable.
Neither a small scheduled gap nor source metadata establishes true gapless output.
Artwork/GPU loading is absent from this fixture and is audited/tested separately.

## Soak, resources and partial results

Run `CONTINUITY_CYCLES=100 CONTINUITY_SECONDS=900 bash host/test-audio-continuity.sh`
or the existing manual `soak.yml`; normal CI retains sixteen transitions plus controls
normally and under ASan/UBSan. Both duration and cycle minima apply. Intermediate
schema-2 JSON is atomically checkpointed every sixteen completed transitions, with
`status: running`; only successful completion writes `status: passed`. A timeout or
failure leaves actual completed duration/count and logs for inspection, not a false
900-second pass. The final report records actual duration rather than configured time.

The measured phase periodically edits/reverts queue rows and navigates through
Settings/queue/Home without replacing current ownership. Post-matrix actual decode
covers Repeat One, shuffle/repeat controls, duplicates, pause/resume/seek, Music-to-video
focus/return, cold restore without autoplay, repeated simulated outages/navigation,
input failure/recovery, revocation, profile isolation and shutdown. Virtual PMS time
is advanced for retry tests; PCM clocks and the 900-second requirement use real time.
Transport status zero represents DNS/TLS/connection failure classes in simulation;
real loopback production-adapter HTTP/TLS tests separately verify actual TLS rejection
and timeout handling. No real service outage or private-server acceptance is claimed.

Resources include initial, sampled transition peak/current, final stopped RSS,
allocator counters, descriptors and threads. Logical attached media, Session owner,
dispatch-boundary pending requests, queue length, cache and pre-resolution bytes are
reported. LibVLC's internal object-reference counts/device buffers are not observable;
a single attached media item is not claimed as their total. PCM observation storage
is capped at 8192 blocks between drains. Native copied audio retains its four-second
bound. Explicit native retention remains intentional safety state, not a host leak.

Existing budgets remain: normal RSS growth 32 MiB, live allocator growth 8 MiB,
files +4, threads +8 after full format warmup. ASan uses its live allocation counter
and a separate 320 MiB RSS envelope for unchanged quarantine; no suppressions or
quarantine reduction. Cache/queue/prefetch budgets are independently asserted.

## Production corrections and UI

- Native format setup previously retained queued PCM from the old configuration;
  accepted format commits now flush copied PCM/counts while retaining user pause.
  Actual player failure injection covers rejected output and stereo/surround resets.
- Healthy library probes previously discarded fresh permissions. Removed or changed
  section identities now fail closed immediately, stop/clear private views and require
  explicit current-profile/server revalidation; a regression covers HTTP-200 removal.
- Production Music UI shows source codec/rate/channels when supplied by Plex, clearly
  labelled as source information. It does not claim actual native output telemetry.
  Two new production-view frames extend coverage from 38 to 40.

## Candidate readiness

The feature/action audit remains in [RELEASE-READINESS.md](RELEASE-READINESS.md).
Independently tested fixes do not clear original reboot, presentation completion,
audio-close ABI, decoder ownership, hardware 4K or real PMS/DVR blockers. Stage 1 and
full-client packages remain separate held candidates with matching debug symbols,
manifests and checksums. See [offline hardware plan](HARDWARE-ACCEPTANCE-PLAN.md):
**NO-GO for physical installation/launch; no hardware authorization is requested.**
