# Privacy CEF runtime

This component owns the runtime profile contract shared by the CEF browser,
renderer, worker, GPU and audio processes. It does not expose profile contents
to page JavaScript.

The profile master seed is decoded once in the browser process. Site seeds are
derived with HMAC-SHA256 over the top-level registrable domain and are never
written to logs. A protected surface must add its algorithm name, algorithm
version and input digest when deriving operation-specific randomness.

`PrivacyAuditService` is browser-process-only. Other processes must report
events over the dedicated IPC layer instead of opening the JSONL file.

