---
trigger: always_on
---

# Description

This project is supposed to bring MapleStory to the browser by using Web Assembly.

The client is a CPP project called MapleStory-Client (in this project it's known simply as `client`).

Since there are multiple git repositories involved, we've created an integration system that works on patching the `src` directory and keeping the patches in a different repository (our repository).

Everything inside `src` should never be commited or even tracked.
This way we'll be able to build the project consistently among different machines without too much trouble.