# Task: Login Shell And Notices

Status: Not started

## Goal

Close the gap between the current single-screen login flow and MapleStory-Client's broader login shell.

## Upstream References

- `0ef89d0` Added auto login for splash screen removal
- `b6c0c1a` Remove start logo on debug
- `890b91b` Added Terms of Service
- `2c59c58` Updated Terms of Service based on GMS v83
- `10e0d71` UIRegion and login screen adjustments

## Relevant Upstream Files

- `IO/UITypes/UILogo.cpp`
- `IO/UITypes/UILogo.h`
- `IO/UITypes/UITermsOfService.cpp`
- `IO/UITypes/UITermsOfService.h`
- `IO/UITypes/UILoginNotice.cpp`
- `IO/UITypes/UILoginNotice.h`
- `IO/UITypes/UILoginWait.cpp`
- `IO/UITypes/UILoginWait.h`
- `IO/UIStateLogin.cpp`
- `IO/UIStateLogin.h`

## Subtasks

- [ ] Port the startup logo flow if it still belongs in this fork.
- [ ] Port terms-of-service screens if required by the target login flow.
- [ ] Port upstream login notice confirm variants and key handling.
- [ ] Compare `UILoginWait` and `UILogin` behavior against upstream.
- [ ] Expand `UIStateLogin` to support the full multi-window login shell.
- [ ] Decide whether auto-login / splash bypass belongs in this fork.

## Notes

- This task is separate from race/class creation because the login shell can be ported independently.
