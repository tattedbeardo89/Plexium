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
