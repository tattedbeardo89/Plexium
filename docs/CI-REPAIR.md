# PR #2 CI repair — 2026-11-12

Both native run `43358805720` and host run `43359262600` stopped in host
compilation: the symlink fixture in `tests/state_tests.cc` ignored `symlink()`'s
return value. Ubuntu's fortified libc declares it warn-unused-result, promoted
to an error by the project's retained `-Werror`. The prior local compiler flags
did not enable that fortification. Native compilation never started.

The fixture now stores and checks the result, reporting the failed operation
and errno. Test checks use `TEST_REQUIRE`, which always evaluates under NDEBUG;
production assertions are unchanged. Related credential, filesystem, child
process, runtime, guide and controller tests have the same always-on checks.
Filesystem setup/cleanup return values are checked. The host build explicitly
enables `_FORTIFY_SOURCE=3`, keeping strict warnings. A separate NDEBUG lane
executes state/auth/session/runtime/protocol and startup-log tests. C startup
and import checks now receive ASan/UBSan flags in the sanitizer workflow too.

`ci/run-check.py` streams command output while retaining a 512 KiB tail and the
first actionable error independently. It preserves command exit status, records
missing executables and bounds memory for unterminated output. Failure collection
creates a summary even if no check started. Uploads tolerate missing diagnostic
files, keeping the original compiler/check failure primary. Three deterministic
Python tests exercise large output, exit status, first error, missing executable
and pre-check failure collection.

Native workflow and host workflow run the same safety checks. Native toolchain
pins, source checksums, TLS, package/signature checks and hardware hold remain.
See subsequent validation records for results; source syntax is not native
compilation, and an eventual Stage 1 build is not full-client/hardware acceptance.

## Validated repair and follow-up implementation

Repair commit `b632fc0bd769478079bfcfba384718b57e5a0a16` passed
[GitHub host CI](https://github.com/tattedbeardo89/Plexium/actions/runs/43368797372).
A subsequent commit includes the following independent core improvements, with
local ordinary/sanitized suites, native-source syntax and entry injection checked
again. Its exact native build result must be read from the workflow, not inferred
from the earlier host success.

- Session restore validates the primary account, fetches current Home users, and
  restores only a remembered **unprotected** profile scoped to that account ID.
  It obtains a new profile token and rediscovers/validates the remembered server's
  machineIdentifier and permitted libraries. Missing/changed/protected profiles,
  account changes and failed switches never reuse a cached authorization token.
  Explicit Switch Profile still opens the picker. PIN entry clears old authorized
  content before prompting; sign-out clears the remembered profile record.
- The personal guide can retain one bounded page for up to 15 minutes during a
  transient DVR/guide outage, labeled `Cached`. A 401/403 discards cached guide
  content, and profile/server transitions clear it. No programming is synthesized.
  Tuning remains disabled until real authorized session lifecycle verification.
- Native shutdown is idempotent and a failed graphics-idle result now produces
  exit status 4 instead of reporting successful exit. Queued player calls close
  before the session worker joins, and application workers finish before player
  and UI shutdown. Device-failure backend/context retention remains intentional.
  Entry tests cover partial init, frame failure, failed shutdown and font fallback.
  Driver/VLC waits and complete native allocation teardown remain unverified.
- A successful pinned native pipeline finalizes
  `Plexium-0.5.0-stage1.<commit>.zip`, matching debug ELF, manifest with source SHA
  and actual UTC build timestamp, checksums, and Stage 1-specific instructions.
  It refuses changed archives/debug symbols, wrong source SHA or full-client
  builds. Synthetic finalizer tests establish those guards only; they are not
  native compilation tests. The installed UI font must match reviewed source.

Local validation: ten C++ suites, six Python diagnostic/finalizer checks, curl and
production GnuTLS HTTP/TLS cases, guide/controller/shared native frames, both
startup modes under host syntax, NDEBUG filesystem/session restoration checks,
and ASan/UBSan including native entry/Vulkan injection passed. Earlier regression
fixture failures were corrected; no warning, assertion or native gate was disabled.

Native Actions now cancel superseded runs on this same development branch so
only the current commit consumes a full toolchain build. Toolchain pins, exact
cache key, dependency fingerprint checks and source digest verification are
unchanged. A cache miss rebuilds the pinned inputs; it never substitutes versions.
Workflow artifacts remain investigation outputs under `ci/HARDWARE-HOLD`.
Physical PS5 recommendation remains **NO-GO** pending native evidence, independent
fault review and a separately authorized controlled hardware gate.
