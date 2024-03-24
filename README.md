# ActiveTransportFilter

# This project is currently under dev

ActiveTransportFilter is a driver (WDF) that makes use of the Windows Filtering Platform for packet inspection.

## Interesting bug fixes during dev

### BSOD in flow after starting Opera

Symptoms: BSOD when opera browser is opened, and hundreds of new connections come in. Crash is in ATF in `KMUTEX` lock

<img src="docs/images/interesting_crash_callout_sync.png">

Issue is probably that WFP doesn't like locking in the callout functions, which I used to sync up with the filter.c engine. I need to remove the `KMUTEX` entirely, and freeup the callout immediately.

As a result, I cannot "dynamically" change the config for the filter engine, I need to stop WFP entirely, refresh the config, and restart WFP.

**Update** removing the `KMUTEX` in the callout was the fix.