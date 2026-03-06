# Cosmic MapleStory Server — Network Protocol Reference

**Version:** GMS v83 (OdinMS-derived, Cosmic fork)
**Audience:** Development team implementing a compatible client or server component
**Scope:** Complete wire-level specification for all client↔server communication

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Transport Layer](#2-transport-layer)
3. [Encryption Layer](#3-encryption-layer)
4. [Packet Wire Format](#4-packet-wire-format)
5. [Data Types Reference](#5-data-types-reference)
6. [Connection Handshake](#6-connection-handshake)
7. [Login Server — Receive Opcodes (Client → Server)](#7-login-server--receive-opcodes-client--server)
8. [Login Server — Send Opcodes (Server → Client)](#8-login-server--send-opcodes-server--client)
9. [Channel Server — Receive Opcodes (Client → Server)](#9-channel-server--receive-opcodes-client--server)
10. [Channel Server — Send Opcodes (Server → Client)](#10-channel-server--send-opcodes-server--client)
11. [Shared Packet Structures](#11-shared-packet-structures)
12. [Stat and Buff Mask Reference](#12-stat-and-buff-mask-reference)
13. [Enumeration Values](#13-enumeration-values)

---

## 1. Architecture Overview

The server is split into two distinct logical tiers that a client connects to sequentially:

```
Client
  │
  ├─── TCP connection ──► Login Server (port 8484 by default)
  │                        Handles: authentication, world/channel listing,
  │                                 character list, character creation/deletion.
  │
  └─── TCP connection ──► Channel Server (separate port per channel)
                           Handles: gameplay, map transitions, combat,
                                    inventory, quests, social features.
```

A client connects to the **Login Server** first. After selecting a world and channel and a character, the Login Server sends a `SERVER_IP` packet containing the IP and port of the target **Channel Server**. The client then opens a **new** TCP connection to that Channel Server (the Login Server connection is dropped).

The Channel Server authenticates the returning client via the `PLAYER_LOGGEDIN` packet.

---

## 2. Transport Layer

- **Protocol:** TCP
- **Byte order:** Little-endian throughout (all multi-byte integers are LE)
- **Login Server default port:** 8484
- **Channel Server ports:** Configurable per channel (typically 7575, 7576, … or as set in `config.yaml`)
- **Framework:** Netty (Java NIO, non-blocking)

---

## 3. Encryption Layer

Every packet after the handshake is encrypted with **two layers applied in sequence**.

### 3.1 MapleCustomEncryption (Maple-specific layer)

Applied **before** AES on send, **after** AES on receive. This is a proprietary byte-scrambling algorithm that runs 6 passes over the data. Passes alternate direction (forward / backward) and apply bit-rotations, XOR, and arithmetic transforms. The algorithm is symmetric with inverse operations for decryption.

This layer has no keys — it is a fixed permutation and does not vary per session.

### 3.2 MapleAESOFB (AES-OFB layer)

- **Algorithm:** AES in OFB (Output Feedback) mode, custom implementation
- **Key (fixed, global):**
  ```
  13 00 00 00  08 00 00 00  06 00 00 00  B4 00 00 00
  1B 00 00 00  0F 00 00 00  33 00 00 00  52 00 00 00
  ```
  (256-bit key; only the low bytes of each 4-byte word are significant)
- **IV (Initialization Vector):** 4 bytes, per-session, exchanged in the handshake
- **Block size used in OFB loop:** 1456 bytes (0x5B0), then 1460 bytes (0x5B4) for subsequent blocks

**IV Update:** After each packet is encrypted/decrypted, the IV is updated using a fixed lookup table (`funnyBytes`, 256 entries) with a specific mixing function. Both client and server maintain synchronized IVs independently for each direction.

### 3.3 Encryption Order

**Sending (server → client):**
1. Build plaintext payload (opcode + fields)
2. Apply `MapleCustomEncryption.encryptData(payload)`
3. Apply `MapleAESOFB.crypt(payload)` using the **send IV**
4. Prepend 4-byte encoded header (see §4)
5. Update send IV

**Receiving (client → server):**
1. Read 4-byte header, validate and decode payload length
2. Read `payloadLength` bytes
3. Apply `MapleAESOFB.crypt(payload)` using the **receive IV**
4. Apply `MapleCustomEncryption.decryptData(payload)`
5. Update receive IV
6. Parse plaintext: first 2 bytes = opcode (LE short), remainder = fields

---

## 4. Packet Wire Format

### 4.1 Header (4 bytes)

```
Byte 0  Byte 1  Byte 2  Byte 3
  A       B       C       D
```

Where:
- `A | (B << 8)` = `iiv ^ mapleVersion`
  (`iiv` = `(recvIV[3] & 0xFF) | ((recvIV[2] << 8) & 0xFF00)`)
- `C | (D << 8)` = `(A | (B << 8)) ^ byteswap(payloadLength)`

**Decoding payload length from header:**
```
length = ((header >> 16) ^ (header & 0xFFFF))
length = byteswap16(length)   // swap the two bytes
```

The header is validated by checking that `(Byte0 ^ recvIV[2]) == (mapleVersion >> 8)` and `(Byte1 ^ recvIV[3]) == (mapleVersion & 0xFF)`.

### 4.2 Payload Layout (after decryption)

```
Offset  Size   Description
0       2      Opcode (little-endian unsigned short)
2       N      Fields (opcode-specific)
```

The MapleStory version used by this server is **83**. The `mapleVersion` used in headers is `83` (0x0053).

### 4.3 String Encoding

All strings are Pascal-style: `[int16 length][UTF-8 bytes]`. There is no null terminator.

### 4.4 Fixed-Length String

Used for character names in some packets: exactly 13 bytes, right-padded with `\0`.

---

## 5. Data Types Reference

| Name          | Size    | Description                                       |
|---------------|---------|---------------------------------------------------|
| `byte`        | 1       | Signed 8-bit integer                              |
| `ubyte`       | 1       | Unsigned 8-bit integer                            |
| `short`       | 2       | Signed 16-bit LE integer                          |
| `int`         | 4       | Signed 32-bit LE integer                          |
| `long`        | 8       | Signed 64-bit LE integer                          |
| `bool`        | 1       | 0 = false, 1 = true                               |
| `string`      | 2+N     | `[short length][UTF-8 bytes]`                     |
| `fixedStr13`  | 13      | Fixed 13-byte field, null-padded                  |
| `pos`         | 4       | `[short x][short y]` — map coordinates            |
| `timestamp`   | 8       | Windows FILETIME: 100-nanosecond intervals since 1601-01-01, LE `long` |

**Special timestamp constants:**
| Constant        | Value (hex)          | Meaning                        |
|-----------------|----------------------|--------------------------------|
| `DEFAULT_TIME`  | `02 17 E6 46 BB 05 80 00` | Far future / permanent     |
| `ZERO_TIME`     | `01 4F 37 3B FD E0 40 01` | Epoch / never                |
| `PERMANENT`     | `02 17 E5 7D 90 9B C0 00` | Item never expires           |

For expiration fields: pass `-1` → `DEFAULT_TIME`, `-2` → `ZERO_TIME`, `-3` → `PERMANENT`. Any other value is converted as `(utcMillis * 10000) + 116444736010800000`.

---

## 6. Connection Handshake

The handshake packet is sent by the **server immediately upon TCP connection** — the client does not send anything first.

### 6.1 Hello Packet (Server → Client, unencrypted)

This is the **only** unencrypted packet in the entire session.

```
Field           Type     Value / Notes
payloadSize     short    0x000E (14) — length of remaining fields
mapleVersion    short    83 (0x0053)
patchLen        short    1
patchChar       byte     0x31 ('1')
recvIV          byte[4]  Server's receive IV (client uses this to send)
sendIV          byte[4]  Server's send IV (client uses this to receive)
locale          byte     8 (GMS locale)
```

**IV generation:**
- `sendIV` = `[0x52, 0x30, 0x78, random_byte]`
- `recvIV` = `[0x46, 0x72, 0x7A, random_byte]`

After this packet, all subsequent traffic is encrypted as described in §3.

---

## 7. Login Server — Receive Opcodes (Client → Server)

All opcodes are 2-byte LE shorts at the start of the decrypted payload.

---

### 0x0001 — LOGIN_PASSWORD

**Purpose:** Client submits username and password for authentication.

```
Field       Type     Notes
login       string   Account name
password    string   Password (plaintext; server hashes internally)
[skip]      byte[6]  Padding (zeros from localhost; ignored)
hwid        byte[4]  4-byte hardware ID nibbles
```

**Server responses (LOGIN_STATUS, opcode 0x00):**

| loginok | Packet sent                 |
|---------|-----------------------------|
| 0       | `getAuthSuccess` → full auth success payload |
| 2 / 3   | `getPermBan` — permanent ban |
| 5 (and AUTOMATIC_REGISTER disabled) | `getLoginFailed(5)` — not registered |
| Any other non-zero | `getLoginFailed(loginok)` |

Temp-ban triggers `getTempBan(timestamp, reason)`. Banned IP/MAC triggers `getLoginFailed(3)`.

---

### 0x0002 — GUEST_LOGIN

**Purpose:** Log in as a guest account.

```
(no fields)
```

Server responds with `GUEST_ID_LOGIN` (0x01).

---

### 0x0004 — SERVERLIST_REREQUEST

**Purpose:** Client re-requests the server list (same handler as SERVERLIST_REQUEST).

```
(no fields)
```

---

### 0x0005 — CHARLIST_REQUEST

**Purpose:** Client selects a world and channel, requests character list.

```
Field    Type    Notes
[skip]   byte    Unknown, ignored
world    byte    World ID (0-based)
channel  byte    Channel ID (0-based; server adds 1 internally)
```

Server responds with `CHARLIST` (0x0B) or `SERVERSTATUS` (0x03) if world is full.

---

### 0x0006 — SERVERSTATUS_REQUEST

**Purpose:** Client polls whether the server is full.

```
Field   Type   Notes
world   short  World ID being queried
```

Server responds with `SERVERSTATUS` (0x03).

---

### 0x0007 — ACCEPT_TOS

**Purpose:** Client accepts the Terms of Service.

```
Field    Type   Notes
accepted byte   1 = accepted
```

---

### 0x0008 — SET_GENDER

**Purpose:** Client sets gender for a new account.

```
Field   Type   Notes
gender  byte   0 = Male, 1 = Female
```

Server responds with `GENDER_DONE` (0x04) and `CHECK_PINCODE` (0x06).

---

### 0x0009 — AFTER_LOGIN

**Purpose:** Sent after the login success response; client confirms ready state.

```
Field   Type   Notes
mode    byte   Login continuation mode
```

---

### 0x000A — REGISTER_PIN

**Purpose:** Client registers or submits a PIN code.

```
Field       Type    Notes
request     byte    0 = register new PIN, 1 = verify PIN
[if mode 0]
  pin       string  New PIN (4-digit numeric string)
[if mode 1]
  pin       string  PIN to verify
```

Server responds with `CHECK_PINCODE` (0x06) or `UPDATE_PINCODE` (0x07).

---

### 0x000B — SERVERLIST_REQUEST

**Purpose:** Client requests the list of worlds.

```
(no fields)
```

Server responds with multiple `SERVERLIST` (0x0A) packets (one per world) followed by a terminator.

---

### 0x000C — PLAYER_DC

**Purpose:** Client reports a disconnection event. No server response required.

---

### 0x000D — VIEW_ALL_CHAR

**Purpose:** Client requests to view characters across all worlds.

```
(no fields)
```

Server responds with `VIEW_ALL_CHAR` (0x08).

---

### 0x000E — PICK_ALL_CHAR

**Purpose:** Client selects a character from the View-All-Characters list.

```
Field     Type   Notes
worldId   int    World the character is in
charId    int    Character ID to select
```

---

### 0x0010 — NAME_TRANSFER

**Purpose:** Client initiates a character name transfer.

```
Field      Type    Notes
charId     int     Character ID
newName    string  Desired new name
```

---

### 0x0012 — WORLD_TRANSFER

**Purpose:** Client initiates a world transfer.

```
Field    Type   Notes
charId   int    Character ID
worldId  int    Target world ID
```

---

### 0x0013 — CHAR_SELECT

**Purpose:** Client selects a character to play (no PIC required).

```
Field    Type    Notes
charId   int     Character ID
mac      string  Client MAC address string
hwid     string  Hardware ID string
```

Server responds with `SERVER_IP` (0x0C) after validation.

---

### 0x0014 — PLAYER_LOGGEDIN (Channel Server only)

**Purpose:** After connecting to the Channel Server, client announces itself.

```
Field    Type   Notes
charId   int    Character ID from previous login flow
```

Server loads and spawns the character in-world. This is the first packet sent on a fresh channel connection.

---

### 0x0015 — CHECK_CHAR_NAME

**Purpose:** Client checks if a character name is available.

```
Field   Type    Notes
name    string  Desired character name
```

Server responds with `CHAR_NAME_RESPONSE` (0x0D).

---

### 0x0016 — CREATE_CHAR

**Purpose:** Client creates a new character.

```
Field      Type    Notes
name       string  Character name (3–12 chars)
job        int     Job ID (see Job enum). 0=Explorer, 1000=Cygnus, 2000=Aran
face       int     Face ID (cosmetic)
hair       int     Hair ID (cosmetic)
hairColor  int     Hair color ID
skin       int     Skin tone ID
top        int     Item ID for top equip
bottom     int     Item ID for bottom equip
shoes      int     Item ID for shoes
weapon     int     Item ID for weapon
gender     byte    0 = Male, 1 = Female
```

Server responds with `ADD_NEW_CHAR_ENTRY` (0x0E) on success, or `CHAR_NAME_RESPONSE` with error on failure.

---

### 0x0017 — DELETE_CHAR

**Purpose:** Client deletes a character.

```
Field     Type    Notes
birthday  int     Birthday as YYYYMMDD integer (account verification)
charId    int     Character ID to delete
```

Server responds with `DELETE_CHAR_RESPONSE` (0x0F).

---

### 0x0018 — PONG

**Purpose:** Client response to server's `PING`. Used as keep-alive.

```
(no fields)
```

---

### 0x001C — RELOG

**Purpose:** Client requests to return to character select without disconnecting.

```
(no fields)
```

Server responds with `RELOG_RESPONSE` (0x16).

---

### 0x001D — REGISTER_PIC

**Purpose:** Client registers a Picture (PIC) secondary password.

```
Field    Type    Notes
charId   int     Character ID
pic      string  New PIC (at least 6 chars)
```

---

### 0x001E — CHAR_SELECT_WITH_PIC

**Purpose:** Client selects a character with PIC verification.

```
Field    Type    Notes
pic      string  PIC to verify
charId   int     Character ID
mac      string  Client MAC address
hwid     string  Hardware ID
```

Server responds with `SERVER_IP` (0x0C) on success, `CHECK_SPW_RESULT` (0x1C) on wrong PIC.

---

### 0x001F — VIEW_ALL_PIC_REGISTER

**Purpose:** Register PIC in View-All-Char context.

```
Field    Type    Notes
charId   int     Character ID
pic      string  New PIC
```

---

### 0x0020 — VIEW_ALL_WITH_PIC

**Purpose:** Select character with PIC in View-All-Char context.

```
Field    Type    Notes
pic      string  PIC to verify
worldId  int     World ID
charId   int     Character ID
```

---

## 8. Login Server — Send Opcodes (Server → Client)

---

### 0x0000 — LOGIN_STATUS

**Purpose:** Response to LOGIN_PASSWORD. Reports success or failure.

**Failed login (reason codes):**
```
Field    Type   Notes
reason   byte   Failure code (see table below)
[byte]   byte   0
[int]    int    0
```

| Code | Displayed message                                          |
|------|------------------------------------------------------------|
| 2    | Permanent ban (+ ban reason + timestamp follow)            |
| 3    | ID deleted or blocked                                      |
| 4    | Incorrect password                                         |
| 5    | Not a registered ID                                        |
| 7    | Already logged in                                          |
| 10   | Cannot process so many connections                         |
| 11   | Only users older than 20 may use this channel              |
| 13   | Unable to log on as master at this IP                      |
| 14   | Wrong gateway or personal info                             |
| 16   | Please verify your account via email                       |
| 23   | License agreement                                          |

**Successful login:**
```
Field            Type     Notes
result           int      0 (success)
[short]          short    0
accountId        int      Account ID
gender           byte     0 = Male, 1 = Female
isGM             bool     True if GM account with admin enforcement enabled
adminByte        byte     0x80 if GM, else 0
countryCode      byte     0
accountName      string   Username
[byte]           byte     0
isQuietBan       byte     0
quietBanTS       long     0
creationTS       long     0
[int]            int      1 (suppresses world selection prompt)
pinEnabled       byte     0 = PIN system enabled, 1 = disabled
picStatus        byte     0 = register PIC, 1 = ask for PIC, 2 = PIC disabled
```

**Permanent ban addition:**
```
Field      Type   Notes
[byte]     byte   0
reason     byte   Ban reason code
banUntil   long   Timestamp (PERMANENT constant if permanent)
```

**Temporary ban:**
```
Field      Type   Notes
result     byte   2
[byte]     byte   0
[int]      int    0
reason     byte   Ban reason code
banUntil   long   Timestamp (FILETIME) until which the ban lasts
```

---

### 0x0001 — GUEST_ID_LOGIN

**Purpose:** Response to guest login.

```
Field       Type    Notes
[short]     short   0x0100
guestId     int     Random guest ID (up to 999999)
[long]      long    0
[long]      long    ZERO_TIME constant
[long]      long    Current time as FILETIME
[int]       int     0
url         string  Guest TOS URL
```

---

### 0x0003 — SERVERSTATUS

**Purpose:** Reports how full a world/channel is.

```
Field    Type    Notes
status   short   0 = Normal, 1 = Highly populated, 2 = Full
```

---

### 0x0004 — GENDER_DONE

**Purpose:** Confirms gender was set.

```
Field    Type   Notes
result   byte   1 = success
gender   byte   0 = Male, 1 = Female
```

---

### 0x0006 — CHECK_PINCODE

**Purpose:** PIN system interaction response.

```
Field   Type   Notes
mode    byte   0 = accepted, 1 = register new PIN, 2 = wrong/re-enter, 3 = system error, 4 = enter PIN
```

---

### 0x0007 — UPDATE_PINCODE

**Purpose:** PIN registration confirmation.

```
Field    Type   Notes
result   byte   0 = success
```

---

### 0x0008 — VIEW_ALL_CHAR

**Purpose:** Response to VIEW_ALL_CHAR request.

```
Field          Type   Notes
status         byte   1 = found characters, 5 = none found
totalWorlds    int    Number of worlds with characters
totalChars     int    Total number of characters across all worlds
```

Followed by per-world character data packets.

---

### 0x000A — SERVERLIST

**Purpose:** Describes one world and its channels. Sent once per world; terminated by 0xFF byte.

```
Field           Type    Notes
worldId         byte    World ID (0-based); 0xFF = end of list
serverName      string  World name (e.g. "Scania")
flag            byte    World flag (0 = normal, 1 = hot, 2 = new, etc.)
eventMsg        string  Event notice text shown on world select
expRate         byte    EXP rate modifier (100 = 1×)
[byte]          byte    0
dropRate        byte    Drop rate modifier (100 = 1×)
[byte]          byte    0
[byte]          byte    0
channelCount    byte    Number of channels
[Per channel:]
  channelName   string  e.g. "Scania-1"
  channelLoad   int     Current population (max ~1200)
  worldId       byte    World ID (redundant)
  channelId     byte    Channel ID (0-based)
  isAdult       bool    False for normal channels
[short]         short   0 (end of channel list)
```

---

### 0x000B — CHARLIST

**Purpose:** Sends the character list for a selected world.

```
Field         Type   Notes
status        byte   0 = success; non-zero = error (see LOGIN_STATUS codes)
charCount     byte   Number of characters
[Per char:]   —      See CharEntry structure (§11.1)
picStatus     byte   0 = register PIC, 1 = enter PIC, 2 = PIC disabled
charSlots     int    Total character slots available
```

---

### 0x000C — SERVER_IP

**Purpose:** Redirects client to the Channel Server.

```
Field       Type     Notes
[short]     short    0
ipAddress   byte[4]  Channel server IPv4 address (raw bytes)
port        short    Channel server TCP port
charId      int      Character ID being transferred
[byte[5]]   byte[5]  Zeros (padding)
```

---

### 0x000D — CHAR_NAME_RESPONSE

**Purpose:** Result of CHECK_CHAR_NAME.

```
Field      Type    Notes
name       string  The name that was checked
nameUsed   byte    0 = available, 1 = already taken
```

---

### 0x000E — ADD_NEW_CHAR_ENTRY

**Purpose:** Confirms character creation and sends the new character's data.

```
Field    Type    Notes
result   byte    0 = success
[char]   —       Full CharEntry (§11.1)
```

---

### 0x000F — DELETE_CHAR_RESPONSE

**Purpose:** Result of character deletion.

```
Field    Type   Notes
charId   int    Character ID
state    byte   0x00 = success; see error codes below
```

| State | Meaning                                           |
|-------|---------------------------------------------------|
| 0x06  | System error                                      |
| 0x09  | Unknown error                                     |
| 0x12  | Invalid birthday                                  |
| 0x14  | Incorrect PIC                                     |
| 0x16  | Cannot delete a guild master                      |
| 0x18  | Pending wedding                                   |
| 0x1A  | Pending world transfer                            |
| 0x1D  | Character has a family                            |

---

### 0x0010 — CHANGE_CHANNEL

**Purpose:** Directs client to a new channel server (after CHANGE_CHANNEL request in-game).

```
Field       Type     Notes
success     byte     1 = success
ipAddress   byte[4]  IPv4 address of the new channel
port        short    TCP port of the new channel
```

---

### 0x0011 — PING

**Purpose:** Keep-alive heartbeat from server. Client must respond with PONG (0x18).

```
(no fields)
```

---

### 0x0016 — RELOG_RESPONSE

**Purpose:** Confirms relog (return to char select).

```
Field    Type   Notes
result   byte   1 = success
```

---

### 0x001A — LAST_CONNECTED_WORLD

**Purpose:** Hints the client which world to pre-select.

```
Field    Type   Notes
worldId  int    World ID (most recently played or most populated)
```

---

### 0x001B — RECOMMENDED_WORLD_MESSAGE

**Purpose:** Sends recommended world messages shown on world select.

```
Field       Type    Notes
count       byte    Number of entries
[Per entry:]
  worldId   int     World ID
  message   string  Recommendation text
```

---

### 0x001C — CHECK_SPW_RESULT

**Purpose:** Wrong PIC response.

```
Field    Type   Notes
result   byte   0 = wrong PIC
```

---

## 9. Channel Server — Receive Opcodes (Client → Server)

After the TCP connection to the Channel Server is established, the client sends `PLAYER_LOGGEDIN` first, then the server spawns the character and sends `SET_FIELD`. Normal gameplay packets follow.

---

### 0x0026 — CHANGE_MAP

**Purpose:** Client uses a portal or dies and respawns.

```
Field        Type    Notes
fromDying    byte    1 = dead/respawn, 0 = regular portal use
targetMapId  int     Target map ID (-1 = use portal name)
portalName   string  Name of the portal used (e.g. "sp", "tp")
[byte]       byte    Unknown
useWheel     byte    > 0 if using Wheel of Fortune item
chasing      byte    1 if GM is specifying exact spawn position
[If chasing:]
  posX       int     Spawn X
  posY       int     Spawn Y
```

**Special case:** If `available() == 0` (empty packet), the client is returning from the Cash Shop.

---

### 0x0027 — CHANGE_CHANNEL

**Purpose:** Client requests to switch to another channel.

```
Field     Type   Notes
channel   byte   Target channel ID (0-based)
```

Server responds with `CHANGE_CHANNEL` (0x10).

---

### 0x0028 — ENTER_CASHSHOP

**Purpose:** Client requests to enter the Cash Shop.

```
(no fields)
```

Server sets up Cash Shop state and sends `SET_CASH_SHOP` (0x7F).

---

### 0x0029 — MOVE_PLAYER

**Purpose:** Client reports player movement data.

```
Field              Type      Notes
[skip]             byte[9]   Timestamp and other client data (ignored by server)
movementData       byte[]    Variable-length movement fragment list (see §11.4)
```

The server rebroadcasts movement to all players on the same map via `MOVE_PLAYER` (0xB9).

---

### 0x002A — CANCEL_CHAIR

**Purpose:** Client stops sitting in a chair.

```
(no fields)
```

---

### 0x002B — USE_CHAIR

**Purpose:** Client sits in a map object chair.

```
Field    Type   Notes
itemId   int    Chair item ID
```

---

### 0x002C — CLOSE_RANGE_ATTACK

**Purpose:** Client reports a melee attack.

```
Field              Type    Notes
[Skip]             byte[4] Client timestamp
numTargets         byte    Upper nibble = number of mobs hit; lower nibble = number of hits per mob
[byte]             byte    0x5B (constant)
skillLevel         byte    Level of skill used (0 if basic attack)
skillId            int     Skill ID (only if skillLevel > 0)
display            byte    Display animation ID
direction          byte    Attack direction
stance             byte    Stance/animation
speed              byte    Attack speed
[byte]             byte    0x0A (constant)
projectile         int     0 for melee
[Per target:]
  mobOid           int     Object ID of the mob
  [byte]           byte    0
  [Per hit line:]
    damage         int     Damage dealt (or negative for miss/critical indicators)
```

---

### 0x002D — RANGED_ATTACK

**Purpose:** Client reports a ranged attack (bow, crossbow, gun, throwing stars).

Same structure as `CLOSE_RANGE_ATTACK` but with:
- `projectile` field set to the ammo/projectile item ID
- Appended `int` (0) after target list

---

### 0x002E — MAGIC_ATTACK

**Purpose:** Client reports a magic skill attack.

Same structure as `CLOSE_RANGE_ATTACK` but with:
- Appended `int charge` if the skill uses a charge mechanic (e.g., Thunder Charge)

---

### 0x002F — TOUCH_MONSTER_ATTACK

**Purpose:** Client reports damage dealt by walking into a monster (e.g., Aran combos touching mobs).

```
Field      Type   Notes
mobOid     int    Object ID of the mob touched
damage     int    Damage dealt
```

---

### 0x0030 — TAKE_DAMAGE

**Purpose:** Client reports receiving damage from a monster, map hazard, or game mechanic.

```
Field          Type   Notes
[int]          int    Client timestamp (ignored)
damagefrom     byte   Attack type: -1 = touch dmg, -2 = mob skill, -3 = fall dmg, -4 = poison/DoT field
element        byte   Elemental type of the attack
damage         int    Amount of damage taken
[If damagefrom != -3 and != -4:]
  monsterId    int    Monster template ID
  mobOid       int    Object ID of the attacking monster
  direction    byte   Direction the attack came from
```

Server validates, applies damage to character HP/MP, and rebroadcasts `DAMAGE_PLAYER` (0xC0) to all players on the map.

---

### 0x0031 — GENERAL_CHAT

**Purpose:** Client sends a chat message.

```
Field      Type    Notes
message    string  Chat text (max 127 chars for non-GMs)
show       byte    Display type (0 = normal, non-zero = variation)
```

If message starts with `!` (command prefix), it is routed to the command handler. Server broadcasts `CHATTEXT` (0xA2) to the map.

---

### 0x0032 — CLOSE_CHALKBOARD

**Purpose:** Client closes their chalkboard message.

```
(no fields)
```

---

### 0x0033 — FACE_EXPRESSION

**Purpose:** Client performs a face/emote expression.

```
Field       Type   Notes
expression  int    Expression ID (0–17 standard; higher IDs for cash expressions)
```

Server broadcasts `FACIAL_EXPRESSION` (0xC1) to the map.

---

### 0x0034 — USE_ITEMEFFECT

**Purpose:** Client uses an item visual effect.

```
Field    Type   Notes
itemId   int    Item ID of the effect item
```

---

### 0x0038 — MOB_BANISH_PLAYER

**Purpose:** Server-validated notification that a mob banished the player to another map.

```
Field    Type   Notes
mobOid   int    Object ID of the mob
```

---

### 0x0039 — MONSTER_BOOK_COVER

**Purpose:** Client changes Monster Book cover card.

```
Field    Type   Notes
cardId   int    Monster Book card item ID to set as cover (0 = none)
```

---

### 0x003A — NPC_TALK

**Purpose:** Client initiates conversation with an NPC.

```
Field    Type   Notes
oid      int    Object ID of the NPC on the map
```

Server validates proximity and spawns NPC script dialogue via `NPC_TALK` (0x130).

---

### 0x003C — NPC_TALK_MORE

**Purpose:** Client responds to an NPC dialogue prompt.

```
Field       Type    Notes
lastMsg     byte    Type of the last message shown
response    byte    Selection index (0/1 for yes/no, or menu option)
[optional]  —       Additional data depending on dialogue type
```

---

### 0x003D — NPC_SHOP

**Purpose:** Client interacts with an NPC shop (buy/sell).

```
Field       Type    Notes
operation   byte    0 = buy, 1 = sell, 2 = recharge, 3 = exit
[If buy:]
  slot      short   Shop slot index (0-based)
  unknown   short   Usually 0
  quantity  short   Amount to buy
[If sell:]
  slot      short   Inventory slot
  itemId    int     Item ID
  quantity  short   Amount to sell
[If recharge:]
  slot      short   Inventory slot of the rechargeable item
```

Server responds with `CONFIRM_SHOP_TRANSACTION` (0x132) and `INVENTORY_OPERATION` (0x1D).

---

### 0x003E — STORAGE

**Purpose:** Client interacts with a storage NPC (Trunk).

```
Field       Type    Notes
operation   byte    4 = take item, 5 = store item, 6 = meso, 7 = sort, 8 = exit
[If store:]
  slot      byte    Inventory slot
  type      byte    Inventory type
  quantity  short   Amount
[If take:]
  type      byte    Inventory type
  slot      byte    Storage slot
[If meso:]
  amount    int     Amount to withdraw (negative) or deposit (positive)
```

---

### 0x003F — HIRED_MERCHANT_REQUEST

**Purpose:** Client interacts with their Hired Merchant (setup/manage).

```
Field       Type    Notes
operation   byte    Operation code
```

---

### 0x0041 — DUEY_ACTION

**Purpose:** Client interacts with the Duey delivery NPC.

```
Field       Type    Notes
operation   byte    0x00 = send package, 0x04 = claim package, 0x05 = remove package
[varies by operation]
```

---

### 0x0042 — OWL_ACTION (Owl of Minerva — most-searched)

**Purpose:** Client opens Owl of Minerva or requests search results.

```
Field    Type   Notes
itemId   int    Item ID to search for in player shops
```

Server responds with shop results via `SHOP_SCANNER_RESULT` (0x46).

---

### 0x0043 — OWL_WARP

**Purpose:** Client warps to a player shop via Owl of Minerva result.

```
Field    Type   Notes
oid      int    Object ID of the player shop to visit
```

---

### 0x0045 — ITEM_SORT (Inventory Merge)

**Purpose:** Merges stackable items in an inventory tab.

```
Field   Type   Notes
type    byte   Inventory type (1=Equip, 2=Use, 3=Setup, 4=Etc, 5=Cash)
```

---

### 0x0046 — ITEM_SORT2 (Inventory Sort)

**Purpose:** Sorts inventory items by a defined ordering.

```
Field   Type   Notes
type    byte   Inventory type
```

---

### 0x0047 — ITEM_MOVE

**Purpose:** Client moves, equips, unequips, or drops an item.

```
Field      Type    Notes
[skip]     byte[4] Timestamp (ignored)
invType    byte    Inventory type (1=Equip, 2=Use, 3=Setup, 4=Etc, 5=Cash)
src        short   Source slot (negative = equipped slot)
action     short   Destination slot (0 = drop, negative = equip slot, positive = inventory slot)
quantity   short   Quantity to move/drop
```

**Logic:**
- `src < 0 && action > 0` → Unequip from `src` to inventory slot `action`
- `action < 0` → Equip item at `src` to equip slot `action`
- `action == 0` → Drop `quantity` of item at `src`
- `action > 0 && src > 0` → Move item from `src` to `action`

---

### 0x0048 — USE_ITEM

**Purpose:** Client uses a consumable (potion, food, etc.).

```
Field    Type    Notes
[skip]   byte[4] Timestamp
slot     short   Inventory slot
itemId   int     Item ID (verified server-side)
```

Also handles `USE_RETURN_SCROLL` (0x55) — same structure, routes to the same handler.

---

### 0x0049 — CANCEL_ITEM_EFFECT

**Purpose:** Client cancels an active item visual effect.

```
(no fields)
```

---

### 0x004B — USE_SUMMON_BAG

**Purpose:** Client uses a summon bag item.

```
Field    Type    Notes
slot     short   Inventory slot
itemId   int     Item ID
```

---

### 0x004C — PET_FOOD

**Purpose:** Client feeds their pet.

```
Field    Type    Notes
petSlot  short   Pet slot index (0, 1, or 2)
itemSlot short   Inventory slot of the food item
itemId   int     Item ID of the food
```

---

### 0x004D — USE_MOUNT_FOOD

**Purpose:** Client feeds their mount.

```
Field    Type    Notes
slot     short   Inventory slot of the mount food
itemId   int     Item ID
```

---

### 0x004E — SCRIPTED_ITEM

**Purpose:** Client uses an item that triggers a script.

```
Field    Type    Notes
[skip]   byte[4] Timestamp
slot     short   Inventory slot
itemId   int     Item ID
```

---

### 0x004F — USE_CASH_ITEM

**Purpose:** Client uses a Cash Shop item (megaphones, EXP coupons, etc.).

```
Field    Type    Notes
slot     short   Inventory slot
itemId   int     Item ID
[varies] —       Additional fields depending on item type
```

---

### 0x0051 — USE_CATCH_ITEM

**Purpose:** Client uses a monster-catching item (e.g., monster net).

```
Field    Type    Notes
slot     short   Inventory slot
itemId   int     Item ID of catch item
mobOid   int     Object ID of the monster to catch
```

---

### 0x0052 — USE_SKILL_BOOK

**Purpose:** Client uses a skill book to learn/raise a skill.

```
Field    Type    Notes
slot     short   Inventory slot of the skill book
itemId   int     Item ID of the skill book
```

---

### 0x0054 — USE_TELEPORT_ROCK

**Purpose:** Client uses a Teleport Rock or VIP Rock.

```
Field       Type    Notes
action      byte    0 = teleport to map, 1 = remove map from favorites
rockType    byte    0 = Regular Tele Rock, 1 = VIP Rock
[varies]    —       For teleport: targetCharName (string) or mapId (int)
```

---

### 0x0055 — USE_RETURN_SCROLL

**Purpose:** Uses a return/town scroll. Same handler as `USE_ITEM`.

---

### 0x0056 — USE_UPGRADE_SCROLL

**Purpose:** Client uses a scroll to upgrade equipment.

```
Field     Type    Notes
[skip]    byte[4] Timestamp
scrollSlot short  Inventory slot of the scroll
equipSlot  short  Inventory slot of the equipment to upgrade
[bool]    short   1 if White Scroll is also applied (stacks)
```

Server responds with `SHOW_SCROLL_EFFECT` (0xA7) and `INVENTORY_OPERATION` (0x1D).

---

### 0x0057 — DISTRIBUTE_AP

**Purpose:** Client manually assigns an Ability Point to a stat.

```
Field    Type    Notes
[skip]   byte[4] Timestamp
stat     int     Stat mask value (see §12, Stat enum)
```

---

### 0x0058 — AUTO_DISTRIBUTE_AP

**Purpose:** Client uses auto-assign for AP (distributes to primary/secondary stats automatically).

```
Field    Type    Notes
[skip]   byte[4] Timestamp
job      int     Character's job ID (server verifies)
```

---

### 0x0059 — HEAL_OVER_TIME

**Purpose:** Client reports natural HP/MP regeneration ticks.

```
Field    Type   Notes
[skip]   byte[4] Timestamp
hpGain   short  HP healed
mpGain   short  MP healed
```

Server validates and applies the heal with anti-cheat checks.

---

### 0x005A — DISTRIBUTE_SP

**Purpose:** Client spends a Skill Point to raise a skill.

```
Field     Type    Notes
[skip]    byte[4] Timestamp
skillId   int     Skill ID to raise by 1 level
```

---

### 0x005B — SPECIAL_MOVE

**Purpose:** Client activates a skill (active/toggle/etc.).

```
Field      Type    Notes
skillId    int     Skill ID
skillLevel byte    Skill level
[varies]   —       Additional fields depending on skill (e.g., target OID for some summons)
```

---

### 0x005C — CANCEL_BUFF

**Purpose:** Client cancels an active buff.

```
Field    Type   Notes
skillId  int    Skill ID whose buff to cancel
```

---

### 0x005E — MESO_DROP

**Purpose:** Client drops mesos on the ground.

```
Field     Type    Notes
[skip]    byte[4] Timestamp
amount    int     Amount of mesos to drop
```

---

### 0x005F — GIVE_FAME

**Purpose:** Client gives fame to another character.

```
Field    Type    Notes
charId   int     Target character ID
```

Server responds with `FAME_RESPONSE` (0x26).

---

### 0x0061 — CHAR_INFO_REQUEST

**Purpose:** Client requests detailed character information panel (clicking another player).

```
Field    Type   Notes
[skip]   byte[4] Timestamp
charId   int    Character ID to inspect
```

Server responds with `CHAR_INFO` (0x3D).

---

### 0x0062 — SPAWN_PET

**Purpose:** Client summons or dismisses a pet.

```
Field    Type    Notes
slot     byte    Inventory slot of the pet item
action   bool    True = summon, false = dismiss
```

---

### 0x0064 — CHANGE_MAP_SPECIAL

**Purpose:** Client uses a special named portal (inner-map or script-driven).

```
Field        Type    Notes
[byte]       byte    0
portalName   string  Portal name to enter
```

---

### 0x0065 — USE_INNER_PORTAL

**Purpose:** Client uses an inner (same-map) portal.

```
Field    Type    Notes
fromId   short   Portal ID the player is coming from
toId     short   Portal ID the player is going to
posX     short   Player X position
posY     short   Player Y position
```

---

### 0x0066 — TROCK_ADD_MAP

**Purpose:** Client manages Teleport Rock favorite maps.

```
Field      Type    Notes
operation  byte    0 = add current map, 1 = remove
mapId      int     Map ID to remove (only if operation == 1)
rockType   byte    0 = Regular Rock, 1 = VIP Rock
```

---

### 0x006B — QUEST_ACTION

**Purpose:** Client performs a quest action.

```
Field      Type    Notes
action     byte    0=restore item, 1=start, 2=complete, 3=forfeit, 4=scripted start, 5=scripted complete
questId    short   Quest ID
[If action == 0:]
  [int]    int     Unknown
  itemId   int     Item ID to restore
[If action == 1,2,4,5:]
  npcId    int     NPC ID to perform the action with
  posX     short   Player X (optional; verified within 1000 units)
  posY     short   Player Y (optional)
[If action == 2 and selection available:]
  selection short  Option chosen on quest complete
```

---

### 0x006D — GRENADE_EFFECT

**Purpose:** Client notifies server a grenade/bomb was thrown.

```
Field       Type   Notes
posX        int    X position of the throw
posY        int    Y position of the throw
keyDown     int    Key-hold duration (for trajectory)
skillId     int    Skill ID (e.g., Brawler grenade)
skillLevel  int    Skill level
```

---

### 0x006E — SKILL_MACRO

**Purpose:** Client saves skill macro configuration.

```
Field         Type    Notes
count         byte    Number of macros (max 5)
[Per macro:]
  name        string  Macro name
  silent      byte    1 = silent macro (no shout)
  skillId1    int     Skill slot 1
  skillId2    int     Skill slot 2
  skillId3    int     Skill slot 3
```

---

### 0x0070 — USE_ITEM_REWARD

**Purpose:** Client opens a loot box / item reward package.

```
Field    Type    Notes
[skip]   byte[4] Timestamp
slot     short   Inventory slot
itemId   int     Item ID of the reward box
```

---

### 0x0071 — MAKER_SKILL

**Purpose:** Client uses the Item Creation (Maker) skill.

```
Field      Type    Notes
operation  int     0 = create item, 1 = stimulate (add scrolls), 3 = disassemble
itemId     int     Item ID to create or target
[varies]   —       Ingredient slots depending on recipe
```

---

### 0x0076 — ADMIN_CHAT

**Purpose:** GM sends a server-wide admin message (scrolling ticker or notice).

```
Field    Type    Notes
type     byte    Message type (see SERVERMESSAGE types)
message  string  Message content
```

---

### 0x0077 — MULTI_CHAT

**Purpose:** Client sends a chat in a social context (party/guild/alliance/buddy).

```
Field       Type    Notes
type        byte    0=buddy, 1=party, 2=guild, 3=alliance
[targets]   —       List of target character IDs (for buddy/party)
message     string  Chat text
```

---

### 0x0078 — WHISPER

**Purpose:** Client sends a whisper or finds a player.

```
Field      Type    Notes
type       byte    6 = whisper, 5 = /find
name       string  Target character name
[If whisper:]
  message  string  Message text
```

Server responds with `WHISPER` (0x87).

---

### 0x007A — MESSENGER

**Purpose:** Maple Messenger chat room operations.

```
Field       Type    Notes
operation   byte    Various messenger commands (join, leave, chat, invite, etc.)
[varies]    —       Dependent on operation
```

---

### 0x007B — PLAYER_INTERACTION

**Purpose:** Client interacts with player shops, mini-games, or trades.

```
Field        Type    Notes
operation    byte    Action type (see PlayerInteractionHandler)
[varies]     —       Depends on operation
```

Key operations:
- `0x01` = Open mini-game
- `0x02` = Invite to shop/game
- `0x03` = Accept invite
- `0x04` = Decline invite
- `0x06` = Chat in trade window
- `0x10` = Open player shop
- `0x16` = Add item to trade
- `0x1A` = Confirm trade

---

### 0x007C — PARTY_OPERATION

**Purpose:** Party management actions.

```
Field       Type    Notes
operation   byte    1=create, 2=leave/disband, 3=join, 4=invite, 5=expel, 6=change leader
[If join:]
  partyId   int     Party ID to join
[If invite:]
  name      string  Player name to invite
[If expel:]
  charId    int     Character ID to remove
[If change leader:]
  charId    int     New leader's character ID
```

---

### 0x007D — DENY_PARTY_REQUEST

**Purpose:** Client declines a party invite.

```
Field    Type    Notes
name     string  Inviter's name
```

---

### 0x007E — GUILD_OPERATION

**Purpose:** Guild management actions.

```
Field      Type    Notes
type       byte    See below
[varies]   —       Depends on type
```

Key types:
- `0x02` = Create guild (+ `guildName: string`)
- `0x05` = Invite (+ `name: string`)
- `0x06` = Leave guild
- `0x07` = Expel (+ `charId: int`, `name: string`)
- `0x08` = Set notice (+ `notice: string`)
- `0x0D` = Set rank titles
- `0x0E` = Change member rank (+ `charId: int`, `rank: byte`)
- `0x10` = Change emblem

---

### 0x0082 — BUDDYLIST_MODIFY

**Purpose:** Buddy list operations.

```
Field       Type    Notes
operation   byte    1=add, 2=accept, 3=delete
[If add:]
  name      string  Target character name
[If accept/delete:]
  charId    int     Character ID
```

---

### 0x0083 — NOTE_ACTION

**Purpose:** Send or delete in-game notes (memos).

```
Field       Type    Notes
type        byte    1=send, 2=delete
[If send:]
  name      string  Recipient name
  message   string  Note text
[If delete:]
  count     byte    Number of notes to delete
  [ids]     int[]   Note IDs to delete
```

---

### 0x0085 — USE_DOOR

**Purpose:** Client uses a mystic door (Priest/Bishop skill).

```
Field    Type    Notes
oid      int     Object ID of the door
toTown   bool    True = go to town door, False = go back to dungeon door
```

---

### 0x0087 — CHANGE_KEYMAP

**Purpose:** Client saves keymap configuration.

```
Field      Type    Notes
[skip]     byte[4]
count      int     Number of changed bindings
[Per entry:]
  key      int     Key code
  type     byte    Binding type (0=none, 1=skill, 2=item, 4=special)
  actionId int     Skill ID, item ID, or action ID depending on type
```

---

### 0x008F — ALLIANCE_OPERATION

**Purpose:** Alliance management.

```
Field      Type   Notes
operation  byte   See AllianceOperationHandler
[varies]   —      Depends on operation
```

---

### 0x0091–0x0099 — FAMILY_* Opcodes

Family system operations. The Family system links characters in a senior/junior hierarchy.

| Opcode | Name                       | Key Fields                       |
|--------|----------------------------|----------------------------------|
| 0x91   | OPEN_FAMILY_PEDIGREE       | (none)                           |
| 0x92   | OPEN_FAMILY                | (none)                           |
| 0x93   | ADD_FAMILY                 | `name: string`                   |
| 0x94   | SEPARATE_FAMILY_BY_SENIOR  | `juniorName: string`             |
| 0x95   | SEPARATE_FAMILY_BY_JUNIOR  | (none)                           |
| 0x96   | ACCEPT_FAMILY              | `accept: byte` (1=accept)        |
| 0x97   | USE_FAMILY                 | `entitlement: int`               |
| 0x98   | CHANGE_FAMILY_MESSAGE      | `message: string`                |
| 0x99   | FAMILY_SUMMON_RESPONSE     | `accepted: byte`                 |

---

### 0x009B — BBS_OPERATION

**Purpose:** Guild Bulletin Board System operations.

```
Field        Type    Notes
operation    byte    0=list threads, 1=view thread, 2=post, 3=reply, 4=delete
[varies]     —       Depends on operation
```

---

### 0x009F — NEW_YEAR_CARD_REQUEST

**Purpose:** New Year card system.

```
Field      Type    Notes
type       byte    4=send, 6=receive, 8=delete, etc.
[varies]   —       Depends on type
```

---

### 0x00A1 — CASHSHOP_SURPRISE

**Purpose:** Client opens a Cash Shop Surprise bag.

```
Field    Type    Notes
slot     short   Cash inventory slot of the surprise bag
itemId   int     Item ID of the bag
```

---

### 0x00A3 — ARAN_COMBO_COUNTER

**Purpose:** Aran class reports combo count update (client-side combo tracking).

```
Field    Type   Notes
[skip]   byte[4] Client timestamp
```

---

### 0x00A7 — MOVE_PET

**Purpose:** Client reports pet movement.

```
Field             Type     Notes
petId             long     Pet unique ID
petSlot           byte     Pet slot (0, 1, or 2)
movementData      byte[]   Movement fragment list (see §11.4)
```

---

### 0x00A8 — PET_CHAT

**Purpose:** Pet says something.

```
Field     Type    Notes
petId     long    Pet unique ID
petSlot   byte    Pet slot
act       byte    Chat type
message   string  Message text
```

---

### 0x00A9 — PET_COMMAND

**Purpose:** Pet performs an action command.

```
Field     Type    Notes
petId     long    Pet unique ID
petSlot   byte    Pet slot
command   byte    Command index
```

---

### 0x00AA — PET_LOOT

**Purpose:** Pet picks up an item drop.

```
Field    Type   Notes
petSlot  byte   Pet slot
oid      int    Object ID of the item to pick up
```

---

### 0x00AB — PET_AUTO_POT

**Purpose:** Pet auto-pot settings.

```
Field      Type    Notes
petSlot    byte    Pet slot
type       byte    0=HP, 1=MP
threshold  short   Percentage threshold to trigger auto-pot
itemId     int     Item to use for auto-potting
slot       short   Inventory slot of the item
```

---

### 0x00AC — PET_EXCLUDE_ITEMS

**Purpose:** Configure which items the pet should not pick up.

```
Field       Type    Notes
petSlot     byte    Pet slot
count       short   Number of items in the exclude list
[Per item:] int     Item ID to exclude
```

---

### 0x00AF — MOVE_SUMMON

**Purpose:** Client reports summon movement.

```
Field           Type    Notes
chrId           int     Owner character ID
summonOid       int     Summon object ID
startPos        pos     Starting position
movementData    byte[]  Movement fragment list
```

---

### 0x00B0 — SUMMON_ATTACK

**Purpose:** Client reports damage dealt by a summon.

```
Field         Type    Notes
chrId         int     Owner character ID
summonOid     int     Summon object ID
direction     byte    Attack direction
targetCount   byte    Number of targets
[Per target:]
  mobOid      int     Monster object ID
  [byte]      byte    0x06 (constant)
  damage      int     Damage dealt
```

---

### 0x00B1 — DAMAGE_SUMMON

**Purpose:** Summon takes damage.

```
Field      Type    Notes
summonOid  int     Summon object ID
attackType byte    Type of attack
damage     int     Damage taken
mobOid     int     Attacking monster's OID
```

---

### 0x00B2 — BEHOLDER

**Purpose:** Dark Knight Beholder summon status update.

```
Field    Type   Notes
skill    int    Beholder skill ID being activated
```

---

### 0x00B5 — MOVE_DRAGON

**Purpose:** Evan Dragon mount movement.

```
Field           Type    Notes
dragonOid       int     Dragon object ID
startPos        pos     Starting position
movementData    byte[]  Movement fragments
```

---

### 0x00B7 — CHANGE_QUICKSLOT

**Purpose:** Client saves quick-slot key binding (CP_QuickslotKeyMappedModified).

```
Field       Type     Notes
quickslots  int[8]   Eight quick-slot bindings (skill/item IDs, 0 = empty)
```

---

### 0x00BC — MOVE_LIFE (Monster Movement)

**Purpose:** Client (as monster controller) reports monster movement.

```
Field           Type    Notes
mobOid          int     Monster object ID
moveId          short   Movement sequence number
skillPossible   bool    Can the monster use a skill now?
skill           byte    Skill flag
skillId         byte    Skill type ID
skillLevel      byte    Skill level
pOption         short   Movement option flags
startPos        pos     Monster starting position
movementData    byte[]  Movement fragment list
```

Server validates and responds with `MOVE_MONSTER_RESPONSE` (0xF0).

---

### 0x00BD — AUTO_AGGRO

**Purpose:** Client reports that a monster should start attacking the player.

```
Field    Type   Notes
mobOid   int    Monster object ID to aggro
```

---

### 0x00BF — FIELD_DAMAGE_MOB

**Purpose:** Field hazard (e.g., lava, spike) damages a monster.

```
Field    Type   Notes
mobOid   int    Monster object ID
damage   int    Damage amount
```

---

### 0x00C0 — MOB_DAMAGE_MOB_FRIENDLY

**Purpose:** Friendly mob damages another friendly mob.

```
Field     Type   Notes
mobFrom   int    Attacking mob OID
mobTo     int    Target mob OID
damage    int    Damage dealt
```

---

### 0x00C1 — MONSTER_BOMB

**Purpose:** Monster explodes (suicide bomber mechanic).

```
Field    Type   Notes
mobOid   int    Monster object ID
```

---

### 0x00C2 — MOB_DAMAGE_MOB

**Purpose:** One monster damages another (used in some boss mechanics).

```
Field     Type   Notes
mobFrom   int    Attacking mob OID
mobTo     int    Target mob OID
damage    int    Damage dealt
```

---

### 0x00CA — ITEM_PICKUP

**Purpose:** Client picks up an item from the map.

```
Field    Type    Notes
[skip]   byte[4] Timestamp
oid      int     Object ID of the item on the map
```

---

### 0x00CD — DAMAGE_REACTOR

**Purpose:** Client damages a reactor (interactable map object).

```
Field       Type    Notes
reactorOid  int     Reactor object ID
damage      int     Damage dealt
direction   int     Direction of attack
```

---

### 0x00CE — TOUCHING_REACTOR

**Purpose:** Client touches a reactor (non-combat activation).

```
Field       Type   Notes
reactorOid  int    Reactor object ID
```

---

### 0x00CF — PLAYER_MAP_TRANSFER

**Purpose:** Client notifies server of a map-to-map transfer completion.

```
(no fields)
```

---

### 0x00D3 — SNOWBALL

**Purpose:** Snowball event packet (hitting snowball in seasonal event map).

```
Field       Type   Notes
team        byte   0 or 1 (which team's snowball)
operation   byte   Hit type
damage      int    Damage to snowball
```

---

### 0x00D5 — COCONUT

**Purpose:** Coconut Harvest event interaction.

```
Field       Type   Notes
operation   byte   Action type
coconutId   int    Target coconut object ID
```

---

### 0x00DA — MONSTER_CARNIVAL

**Purpose:** Monster Carnival PQ actions.

```
Field       Type    Notes
operation   byte    0=request start, 1=summon, 2=request buff
[If summon/buff:]
  mobId     int     Monster/skill ID
```

---

### 0x00DC — PARTY_SEARCH_REGISTER

**Purpose:** Register character in the party search system.

```
Field    Type    Notes
joinable byte    1 = willing to join a party
```

---

### 0x00DE — PARTY_SEARCH_START

**Purpose:** Start searching for players to fill party.

```
Field    Type   Notes
minLevel short  Minimum level filter
maxLevel short  Maximum level filter
job      short  Job filter (0 = any)
```

---

### 0x00DF — PARTY_SEARCH_UPDATE

**Purpose:** Update party search preferences.

```
(similar to PARTY_SEARCH_START)
```

---

### 0x00E4 — CHECK_CASH

**Purpose:** Client requests current NX (Cash) balance when entering Cash Shop.

```
(no fields)
```

Server responds with `QUERY_CASH_RESULT` (0x144).

---

### 0x00E5 — CASHSHOP_OPERATION

**Purpose:** Cash Shop purchase/gift/move operations.

```
Field       Type    Notes
operation   byte    See CashOperationHandler
[varies]    —       Depends on operation
```

Key operations:
- `0x03` = Buy item with NX (+ `sn: int`, `quantity: short`)
- `0x04` = Gift item (+ `sn: int`, `recipient: string`, `message: string`)
- `0x06` = Modify wish list
- `0x1D` = Move item to locker
- `0x1E` = Move item from locker to inventory
- `0x21` = Buy item in package

---

### 0x00E6 — COUPON_CODE

**Purpose:** Client enters a coupon code.

```
Field   Type    Notes
code    string  Coupon code string
```

---

### 0x00EC — OPEN_ITEMUI

**Purpose:** Client opens a special item UI (e.g., Giant Wheel of Destiny).

```
Field    Type   Notes
itemId   int    Item ID
```

---

### 0x00EE — USE_ITEMUI

**Purpose:** Client uses the currently open item UI.

```
(no fields — item context is server-tracked)
```

---

### 0x00FD — MTS_OPERATION

**Purpose:** Maple Trading System operations (buy/sell/listings).

```
Field       Type    Notes
operation   byte    See MTSHandler
[varies]    —       Depends on operation
```

---

### 0x0100 — USE_MAPLELIFE

**Purpose:** Client interacts with a MapleLife (player-created NPC).

```
Field    Type   Notes
[skip]   byte[4]
npcId    int    MapleLife NPC ID
```

---

### 0x0104 — USE_HAMMER

**Purpose:** Client uses Vicious Hammer on equipment.

```
Field     Type    Notes
[skip]    byte[4] Timestamp
hammerSlot short  Inventory slot of the Vicious Hammer
equipSlot  short  Inventory slot of the equipment to hammer
```

---

### 0x3713 — CUSTOM_PACKET

**Purpose:** Internal diagnostic/debug packet. Handled by `CustomPacketHandler`.

```
(implementation-defined)
```

---

## 10. Channel Server — Send Opcodes (Server → Client)

---

### 0x001D — INVENTORY_OPERATION

**Purpose:** Updates the client's inventory display.

```
Field       Type    Notes
updateTick  bool    Whether to update client-side tick
count       byte    Number of inventory modifications
[Per mod:]
  mode      byte    0=add, 1=update quantity, 2=move, 3=remove
  invType   byte    Inventory type (1–5)
  slot      short   Target slot (or old slot for mode 2)
  [mode 0:] —       Full item info (§11.2)
  [mode 1:] short   New quantity
  [mode 2:] short   New slot position
[addMovement] byte  (Optional) 1=equip animation, 2=unequip animation
```

---

### 0x001E — INVENTORY_GROW

**Purpose:** Notifies client that an inventory tab grew.

```
Field     Type   Notes
invType   byte   Inventory type (1–5)
newLimit  byte   New slot limit
```

---

### 0x001F — STAT_CHANGED

**Purpose:** Updates one or more character statistics. Also used as an "enable actions" signal.

```
Field        Type   Notes
enableActions bool  True = player can act after this update
updateMask   int    Bitmask of which stats changed (see §12)
[Per stat in mask order:]
  value      —      Size depends on stat (see §12)
```

**Stat mask values** (sorted by value):
| Mask       | Stat  | Type   |
|------------|-------|--------|
| 0x0001     | Skin  | byte   |
| 0x0002     | Face  | int    |
| 0x0004     | Hair  | int    |
| 0x0008     | Pet1  | long   |
| 0x0010     | Pet2  | long   |
| 0x0020     | Pet3  | long   |
| 0x0040     | Level | byte   |
| 0x0080     | Job   | short  |
| 0x0100     | STR   | short  |
| 0x0200     | DEX   | short  |
| 0x0400     | INT   | short  |
| 0x0800     | LUK   | short  |
| 0x1000     | HP    | short  |
| 0x2000     | MaxHP | short  |
| 0x4000     | MP    | short  |
| 0x8000     | MaxMP | short  |
| 0x10000    | AP    | short  |
| 0x20000    | SP    | short (or SP table — see §11.5) |
| 0x40000    | EXP   | int    |
| 0x80000    | Fame  | short  |
| 0x100000   | Mesos | int    |

---

### 0x0020 — GIVE_BUFF

**Purpose:** Applies a buff or debuff to the local character.

```
Field          Type    Notes
firstMask      long    Bitmask for first 64 buff stats
secondMask     long    Bitmask for second 64 buff stats
[Per active stat in mask:]
  value        short   Buff value
  buffSource   int     Skill ID that gave the buff
  duration     int     Duration in milliseconds
[int]          int     0
[byte]         byte    0
[int]          int     Buff value (for homing beacon)
[extra bytes]  —       If MONSTER_RIDING or HOMING_BEACON: skip 3 more bytes
```

**For debuffs (giveDebuff):**
```
firstMask      long
secondMask     long
[Per stat:]
  value        short
  mobSkillType short
  mobSkillLevel short
  duration     int
[short]        short   0
[short]        short   900 (display delay ms)
[byte]         byte    1
```

---

### 0x0021 — CANCEL_BUFF

**Purpose:** Cancels one or more active buffs on the local character.

```
Field        Type   Notes
firstMask    long   Buff stat first mask
secondMask   long   Buff stat second mask
[byte]       byte   1
```

For debuff cancellation, firstMask = 0 and secondMask contains the disease mask.

---

### 0x0024 — UPDATE_SKILLS

**Purpose:** Sends skill level updates to the client.

```
Field       Type    Notes
[byte]      byte    1 (update flag)
count       short   Number of skill entries
[Per skill:]
  skillId   int     Skill ID
  level     int     Current skill level
  expiry    long    Expiration timestamp (FILETIME) or DEFAULT_TIME
  [if 4th job skill:]
  masterLv  int     Master level
```

---

### 0x0026 — FAME_RESPONSE

**Purpose:** Result of GIVE_FAME action.

```
Field        Type    Notes
result       byte    0 = success, others = error
[If success:]
  name       string  Target character name
  mode       byte    0 = gave fame, 1 = defamed
  fame       int     Target's new fame
  change     int     +1 or -1
```

---

### 0x0027 — SHOW_STATUS_INFO

**Purpose:** Shows status information to the local player (EXP, mesos, items, quests).

Multipurpose packet; interpretation varies by first `type` byte:

| Type | Content                                                    |
|------|------------------------------------------------------------|
| 0    | Item gain in HUD (itemId: int, qty: int, 0: int, 0: int)  |
| 1    | Quest update (questId: short, status: byte, progress: string) |
| 2    | Quest info update                                          |
| 3    | EXP gain (white: bool, gain: int, inChat: bool, 0: int, 0: byte, 0: byte, 0: int, [if inChat: 0: byte], 0: byte, partyBonus: int, equipBonus: int, 0: int, 0: int) |
| 4    | Fame gain (amount: int)                                    |
| 5    | Meso gain in chat (amount: int, 0: short)                  |
| 0x0B | Gachapon message (playerName+msg: string, 0: int, town: string, itemInfo) |

---

### 0x0029 — MEMO_RESULT

**Purpose:** Shows note/memo operation result.

---

### 0x0031 — QUEST_CLEAR

**Purpose:** Plays a quest-clear animation/sound.

```
Field     Type   Notes
questId   short  Completed quest ID
```

---

### 0x003D — CHAR_INFO

**Purpose:** Detailed character information popup (from clicking another player).

```
Field              Type    Notes
charId             int
level              byte
job                short   Job ID
str                short   STR stat
dex                short
int_               short
luk                short
hp                 short
maxHp              short
mp                 short
maxMp              short
ap                 short   AP remaining
[SP info]          —       See §11.5
exp                int
fame               short
isMarried          byte    1 if has marriage ring
guildName          string
allianceName       string
[byte]             byte    0 (medal info placeholder)
[Pets: up to 3]
  [petUniqueId]    byte
  petItemId        int
  petName          string
  petLevel         byte
  tameness         short
  fullness         byte
  [short]          short   0
  mountEquipId     int     (currently equipped mount food item)
[byte]             byte    0 (end of pets)
[Mount if equipped:]
  mountId          byte    Mount type ID
  mountLevel       int
  mountExp         int
  mountTiredness   int
[wishlistCount]    byte
[wishlist items]   int[]   SN IDs
monsterBookLevel   int
normalCards        int
specialCards       int
totalCards         int
bookCoverMobId     int
medalItemId        int     Currently equipped medal (or 0)
medalQuestCount    short
medalQuestIds      short[] Completed medal quests (IDs ≥ 29000)
```

---

### 0x003E — PARTY_OPERATION

**Purpose:** Party state updates.

```
Field       Type    Notes
operation   byte    See PartyOperation enum
[varies]    —       Depends on operation
```

Key operations sent to client:
- Create/join/leave/expel notifications include the full party member list
- Each PartyCharacter entry: `name: string`, `jobId: int`, `level: int`, `channel: int`, `mapId: int`, `charId: int`

---

### 0x003F — BUDDYLIST

**Purpose:** Full buddy list update.

```
Field        Type    Notes
operation    byte    7=load list, 9=online notify, 10=offline notify
count        byte    Buddy count (for load)
[Per buddy:]
  charId     int
  name       string  (padded to 13 chars)
  [byte]     byte    0
  channel    int     Channel (-1 if offline)
  [bytes]    byte[3] Zeros
```

---

### 0x0041 — GUILD_OPERATION

**Purpose:** Guild state updates (create, join, leave, expel, emblem change, etc.).

```
Field       Type   Notes
operation   byte   Guild operation type
[varies]    —      Specific data per operation
```

---

### 0x0042 — ALLIANCE_OPERATION

**Purpose:** Alliance state updates.

---

### 0x0043 — SPAWN_PORTAL

**Purpose:** Spawns or removes a mystic door portal at a position.

```
Field      Type   Notes
townMapId  int    Town-side map ID (or 999999999 for removal)
targetId   int    Field-side map ID (or 999999999 for removal)
[If spawning:]
  pos      pos    Portal position (x, y)
```

---

### 0x0044 — SERVERMESSAGE

**Purpose:** Displays a server notice or scrolling message.

```
Field          Type    Notes
type           byte    Message type
[if ticker:]   byte    1 (present only for type=4)
message        string  Message text
[If type==3:]
  channel      byte    Source channel (0-based)
  megaEar      bool    Whether Maple Ear megaphone is used
[If type==6:]
  [int]        int     0
[If type==7:]
  npcId        int     Broadcasting NPC ID
[If type==0x0B:]         Gachapon — see §9 SHOW_STATUS_INFO type 0x0B
```

**Message types:**
| Type | Display                                |
|------|----------------------------------------|
| 0    | `[Notice]` in chat                     |
| 1    | Popup dialog box                       |
| 2    | Megaphone (above-head balloon)         |
| 3    | Super Megaphone (all-channel notice)   |
| 4    | Scrolling ticker at top of screen      |
| 5    | Pink text in chat                      |
| 6    | Light blue text in chat                |
| 7    | NPC broadcasting                       |
| 0x0B | Gachapon item announcement             |

---

### 0x007D — SET_FIELD

**Purpose:** Sent when the character enters the world for the first time, or when warping to a new map.

**First login (channel entry) variant:**
```
Field         Type    Notes
channel       int     Channel index (0-based)
[byte]        byte    1 (first-login flag)
[byte]        byte    1
[short]       short   0
[int * 3]     int[]   Three random seed ints
[char info]   —       Full character info blob (§11.3)
timestamp     long    Current server time (FILETIME)
```

**Map warp variant:**
```
Field        Type    Notes
channel      int     Channel index (0-based)
[int]        int     0
[byte]       byte    0
mapId        int     Target map ID
spawnPoint   byte    Spawn portal index
hp           short   Player's current HP
chasing      bool    If true, spawn at specific coords
[If chasing:]
  posX       int
  posY       int
timestamp    long    Current server time
```

---

### 0x007F — SET_CASH_SHOP

**Purpose:** Sets up the Cash Shop UI. Contains character and account info needed for the CS.

---

### 0x008A — FIELD_EFFECT

**Purpose:** Triggers a map-wide visual/audio effect.

```
Field      Type    Notes
type       byte    0=music, 1=object effect, 3=skill aura
[varies]   —       Depends on type
```

---

### 0x0093 — CLOCK

**Purpose:** Displays a countdown or clock.

```
Field    Type   Notes
type     byte   1=clock (HH:MM:SS), 2=countdown (seconds)
[If 1:]
  hour   byte
  min    byte
  sec    byte
[If 2:]
  secs   int    Countdown duration in seconds
```

---

### 0x0096 — SET_QUEST_CLEAR

**Purpose:** Marks a quest as clear on the minimap timer.

```
Field     Type   Notes
questId   short
```

---

### 0x00A0 — SPAWN_PLAYER

**Purpose:** Announces another character entering the map.

```
Field           Type    Notes
charId          int     Character ID
level           byte
name            string
guildName       string
guildLogoInfo   short+byte+short+byte  (if in guild)
[foreign buffs] —       See §11.6
jobId           short
[char look]     —       See §11.7
chocolates      int     Chocolate count (Heart-Shaped Chocolate cash item qty)
itemEffect      int     Current item visual effect ID
chair           int     Chair item ID (if seated)
pos             pos     Spawn position
stance          byte    Direction facing / stance
[short]         short   0
[byte]          byte    0
[pets]          —       Pet info array (up to 3, terminated by 0x00)
[mount]         —       Level: int, EXP: int, Tiredness: int (or 0x01 0x00×8 if none)
[shop/game box] —       PlayerShop or MiniGame announce box, or 0x00
[chalkboard]    —       byte flag; if 1: string message
[ring looks]    —       Crush rings, friendship rings, marriage ring
[new year]      —       New Year card info
[byte]          byte    0
[byte]          byte    0
team            byte    Team ID (for PvP maps)
```

---

### 0x00A1 — REMOVE_PLAYER_FROM_MAP

**Purpose:** Removes a player from the current map view.

```
Field    Type   Notes
charId   int    Character ID to remove
```

---

### 0x00A2 — CHATTEXT

**Purpose:** Displays a chat message from a character.

```
Field    Type    Notes
charId   int     Speaker's character ID
isGM     bool    True = GM chat (white background)
message  string  Chat text
show     byte    Display variant
```

---

### 0x00A4 — CHALKBOARD

**Purpose:** Shows/hides a character's chalkboard message.

```
Field      Type    Notes
charId     int     Character ID
isEnabled  bool    True = showing, False = cleared
[If enabled:]
  message  string  Chalkboard text
```

---

### 0x00A7 — SHOW_SCROLL_EFFECT

**Purpose:** Plays scroll success/failure animation.

```
Field            Type   Notes
charId           int    Character who scrolled
success          bool   True = scroll succeeded
itemDestroyed    bool   True = item was destroyed (curse)
legendarySpirit  bool   True = Legendary Spirit scroll used
whiteScroll      bool   True = White Scroll was used
```

---

### 0x00A8 — SPAWN_PET

**Purpose:** Spawns a pet for a character.

```
Field        Type    Notes
charId       int
petSlot      byte    0, 1, or 2
name         fixedStr13
petItemId    int
petUniqueId  long
posX         short
posY         short
stance       byte
fh           short
nameTag      bool    True if name tag equipped
chatBubble   bool    True if chat bubble equipped
```

---

### 0x00AF — SPAWN_SPECIAL_MAPOBJECT (Summon)

**Purpose:** Spawns a player's summon on the map.

```
Field           Type   Notes
ownerId         int    Owner character ID
summonOid       int    Summon object ID
skillId         int    Skill that created this summon
[byte]          byte   0x0A (version)
skillLevel      byte
pos             pos
stance          byte   Facing/action
[short]         short  0
movementType    byte   0=stationary, 1=follow, 2/4=tele-follow, 3=bird
canAttack       bool   True if summon attacks
[bool]          bool   True if NOT animated spawn (appears instantly)
```

---

### 0x00B0 — REMOVE_SPECIAL_MAPOBJECT

**Purpose:** Removes a summon.

```
Field      Type   Notes
ownerId    int
summonOid  int
mode       byte   4=animated removal, 1=instant
```

---

### 0x00B1 — MOVE_SUMMON

**Purpose:** Broadcasts summon movement.

```
Field        Type    Notes
ownerId      int
summonOid    int
startPos     pos
[movement]   byte[]  Movement fragments
```

---

### 0x00B2 — SUMMON_ATTACK

**Purpose:** Broadcasts summon attack animation and damage.

```
Field          Type   Notes
ownerId        int
summonOid      int
[byte]         byte   0 (char level placeholder)
direction      byte
targetCount    byte
[Per target:]
  mobOid       int
  [byte]       byte   0x06
  damage       int
```

---

### 0x00B5 — SPAWN_DRAGON

**Purpose:** Spawns Evan's dragon.

```
Field       Type   Notes
ownerId     int
dragonOid   int
posX        short
posY        short
stance      byte
fh          short
dragonType  short
```

---

### 0x00B9 — MOVE_PLAYER

**Purpose:** Rebroadcasts a character's movement to other players.

```
Field        Type   Notes
charId       int
[int]        int    0
[movement]   byte[] Raw movement data (re-broadcast from client packet)
```

---

### 0x00BA — CLOSE_RANGE_ATTACK (broadcast)

**Purpose:** Broadcasts a close-range attack to other players.

Same structure as the server's version of the attack body (§11.8).

---

### 0x00BC — MAGIC_ATTACK (broadcast)

Same as CLOSE_RANGE_ATTACK broadcast, plus optional `charge: int`.

---

### 0x00C0 — DAMAGE_PLAYER

**Purpose:** Shows damage received by a player.

```
Field          Type   Notes
charId         int
skill          byte   Attack type from TAKE_DAMAGE (-1=touch, -2=skill, -3=fall, -4=DoT)
[If skill==-3:]
  [int]        int    0
damage         int    Damage taken
[If skill!=-4:]
  monsterId    int    Monster template ID
  direction    byte
  [pgmr data]  —      Power Guard reflect info (if applicable)
  [short]      short  0 (if no reflect)
  damageCopy   int    Damage repeated
  [If fake>0:]
  fakeDamage   int    Fake (missed) damage number
```

---

### 0x00C1 — FACIAL_EXPRESSION

**Purpose:** Plays an emote on a character.

```
Field        Type   Notes
charId       int
expression   int    Expression ID
```

---

### 0x00C4 — SHOW_CHAIR

**Purpose:** Shows a character sitting in a chair.

```
Field    Type   Notes
charId   int
itemId   int    Chair item ID (0 = unsit)
```

---

### 0x00C5 — UPDATE_CHAR_LOOK

**Purpose:** Updates another character's visual appearance (after equip change).

```
Field       Type   Notes
charId      int
[byte]      byte   1
[char look] —      See §11.7
[rings]     —      Crush, friendship, marriage rings
[int]       int    0
```

---

### 0x00C7 — GIVE_FOREIGN_BUFF

**Purpose:** Applies a visible buff effect to another player (visible to others, not the recipient).

```
Field       Type   Notes
charId      int
firstMask   long
secondMask  long
[Per stat:] short  Buff value
[int]       int    0
[short]     short  0
```

---

### 0x00C8 — CANCEL_FOREIGN_BUFF

**Purpose:** Removes a foreign buff from another player.

```
Field       Type   Notes
charId      int
firstMask   long
secondMask  long
```

---

### 0x00C9 — UPDATE_PARTYMEMBER_HP

**Purpose:** Updates the HP bar display for a party member.

```
Field     Type   Notes
charId    int
hp        int    Current HP
maxHp     int    Maximum HP
```

---

### 0x00CE — SHOW_ITEM_GAIN_INCHAT

**Purpose:** Shows item gain in chat window.

```
Field      Type   Notes
type       byte   3 = standard
subtype    byte   1 = normal
itemId     int
quantity   int
```

---

### 0x00D3 — UPDATE_QUEST_INFO

**Purpose:** Quest timer or HUD quest update.

```
Field     Type   Notes
subtype   byte   6=add timer, 7=remove timer, 8=quest npc update
[If 6:]
  count   short  1
  questId short
  time    int    Seconds remaining
[If 7:]
  count   short  1
  questId short
[If 8:]
  questId short
  npcId   int
  [int]   int    0
```

---

### 0x00EA — COOLDOWN

**Purpose:** Notifies client of a skill cooldown.

```
Field      Type   Notes
skillId    int
cooldown   short  Remaining cooldown in seconds
```

---

### 0x00EC — SPAWN_MONSTER

**Purpose:** Spawns a monster (no controller).

```
Field         Type    Notes
mobOid        int     Object ID
[byte]        byte    5 (if real) or controller indicator
mobId         int     Monster template ID
[statusMask]  int[4]  16 bytes of zeroed status (non-controller spawn)
pos           pos
stance        byte
startFh       short   0 (origin foothold, unused)
fh            short   Current foothold
[spawnEffect] —       See spawn effect encoding
team          byte    Team ID (0xFF for none)
[int]         int     0
```

**Spawn effect encoding** (appended after FH):
- Parent mob present and alive: `byte(-3)`, `int(parentMobOid)`
- No parent, `effect > 0`: `byte(effect)`, `byte(0)`, `short(0)`, [if effect==15: `byte(0)`], then `byte(-2 or -1)`
- No parent, no effect: `byte(-2)` = fade in, `byte(-1)` = instant

---

### 0x00ED — KILL_MONSTER

**Purpose:** Removes a monster with death animation.

```
Field       Type   Notes
mobOid      int
animation   byte   0=instant, 1=fade, 2+=special
[byte]      byte   Same as animation (sent twice)
```

---

### 0x00EE — SPAWN_MONSTER_CONTROL

**Purpose:** Assigns or removes monster control to the client.

```
Field    Type   Notes
mode     byte   0=remove control, 1=normal, 2=aggro
mobOid   int
[If mode > 0:]  Full monster data same as SPAWN_MONSTER
```

---

### 0x00EF — MOVE_MONSTER

**Purpose:** Broadcasts monster movement to all clients on the map.

```
Field           Type   Notes
mobOid          int
[byte]          byte   0
skillPossible   bool
skill           byte
skillId         byte
skillLevel      byte
pOption         short
startPos        pos
[movement]      byte[] Movement fragments
```

---

### 0x00F0 — MOVE_MONSTER_RESPONSE

**Purpose:** Acknowledges a MOVE_LIFE packet from the controlling client.

```
Field       Type   Notes
mobOid      int
moveId      short  Movement sequence number echoed back
useSkills   bool
currentMp   short  Monster's current MP
skillId     byte   Skill to use (0 if none)
skillLevel  byte
```

---

### 0x00F2 — APPLY_MONSTER_STATUS

**Purpose:** Shows a monster receiving a status effect.

```
Field      Type    Notes
mobOid     int
[masks]    int[4]  Status mask (128 bits across 4 ints)
[Per status:]
  value    short   Status value
  sourceId int (or mobSkillType short + level short)
  duration short   -1 (display only)
[reflect]  —       Optional pCounter/mCounter/nCounterProb if reflect is active
```

---

### 0x00F3 — CANCEL_MONSTER_STATUS

**Purpose:** Removes status effects from a monster.

```
Field     Type    Notes
mobOid    int
[masks]   int[4]  Status mask being removed
```

---

### 0x00F6 — DAMAGE_MONSTER

**Purpose:** Displays damage numbers on a monster.

```
Field      Type   Notes
mobOid     int    Object ID
dmgType    byte   0=normal, 1=heal
damage     int    Damage amount
[byte]     byte   0
```

---

### 0x00FA — SHOW_MONSTER_HP

**Purpose:** Displays a monster's HP percentage bar (for bosses).

```
Field      Type   Notes
mobOid     int
hpPercent  byte   0–100
```

---

### 0x0101 — SPAWN_NPC

**Purpose:** Spawns an NPC on the map.

```
Field     Type   Notes
oid       int    NPC object ID
npcId     int    NPC template ID
posX      short
cy        short  Ground Y
facingRight bool  True if facing right
fh        short  Foothold
rx0       short  Hitbox range left
rx1       short  Hitbox range right
[byte]    byte   1
```

---

### 0x0102 — REMOVE_NPC

**Purpose:** Removes an NPC from the map.

```
Field   Type   Notes
oid     int    NPC object ID
```

---

### 0x0103 — SPAWN_NPC_REQUEST_CONTROLLER

**Purpose:** Assigns NPC control to a client (needed for NPC animations).

Same structure as `SPAWN_NPC` with an extra `miniMap: bool` field.

---

### 0x0109 — SPAWN_HIRED_MERCHANT

**Purpose:** Shows a Hired Merchant on the map.

```
Field        Type    Notes
ownerId      int
ownerName    string
posX         short
posY         short
direction    byte
description  string  Shop description
itemId       int     Hired Merchant item ID
[byte]       byte    5 (box type)
objectId     int
shopName     string
[itemClass]  byte    Item ID % 100
[roomInfo]   byte[]  Capacity data
```

---

### 0x010C — DROP_ITEM_FROM_MAPOBJECT

**Purpose:** Spawns a dropped item on the map.

```
Field        Type   Notes
mode         byte   1=regular drop, 2=update existing, 3=spawn from explode
objectId     int    Item's map object ID
isMeso       bool   True if mesos, false if item
itemId       int    Item/meso ID
ownerId      int    Owning character/party ID (-1 = FFA)
dropType     byte   0=timed private, 1=party-timed private, 2=FFA, 3=explosive
pos          pos    Landing position
dropperOid   int    Object ID of the dropper (monster/player)
[If mode != 2:]
  fromPos    pos    Origin position (where it dropped from)
  delay      short  Animation delay
[If item:]
  expiry     long   Item expiration FILETIME
petPickup    byte   0 if pet can pick up
```

---

### 0x010D — REMOVE_ITEM_FROM_MAP

**Purpose:** Removes a dropped item from the map.

```
Field       Type   Notes
animation   byte   0=expire, 1=silent, 2=pickup, 4=explode
objectId    int
[If >= 2:]
  charId    int    Who picked it up
[If pet:]   byte   Pet slot
[If == 4:]  short  Explosion delay
```

---

### 0x0111 — SPAWN_MIST

**Purpose:** Creates a mist/poison cloud.

```
Field       Type   Notes
mistOid     int
isMobMist   bool   True if spawned by a monster, false if by player
sourceId    int    Skill/mob ID
posLeft     int    Bounding box left
posTop      int    Bounding box top
posRight    int    Bounding box right
posBottom   int    Bounding box bottom
[short]     short  Remaining duration (seconds)
```

---

### 0x0112 — REMOVE_MIST

**Purpose:** Removes a mist cloud.

```
Field    Type   Notes
mistOid  int
```

---

### 0x0113 — SPAWN_DOOR

**Purpose:** Displays a mystic door.

```
Field      Type   Notes
launched   bool   True if already deployed
ownerId    int    Door owner's character ID
pos        pos    Door position
```

---

### 0x0115 — REACTOR_HIT

**Purpose:** Plays reactor hit animation.

```
Field       Type   Notes
reactorOid  int
charId      int    Who hit it
damage      int
stance      byte
```

---

### 0x0117 — REACTOR_SPAWN

**Purpose:** Spawns a reactor on the map.

```
Field       Type   Notes
reactorOid  int
reactorId   int    Template ID
state       byte   Current state
posX        short
posY        short
facingRight byte
name        string Reactor name (usually empty)
```

---

### 0x0118 — REACTOR_DESTROY

**Purpose:** Destroys a reactor.

```
Field       Type   Notes
reactorOid  int
state       byte
posX        short
posY        short
```

---

### 0x0121 — MONSTER_CARNIVAL_START

**Purpose:** Starts the Monster Carnival PQ for the character.

```
Field        Type   Notes
team         byte   0 or 1
[int]        int    CP obtained
[Per team: 5 ints] Team member char IDs
[bonus CP]   int    Bonus CP
```

---

### 0x0130 — NPC_TALK

**Purpose:** Displays NPC dialogue to the client.

```
Field       Type    Notes
npcId       int     NPC template ID
msgType     byte    Dialogue type (see below)
[byte]      byte    0 (speaker type, 0=NPC 1=player)
message     string  Dialogue text
[varies]    —       Extra fields depending on msgType
```

**Dialogue types (msgType):**
| Type | Description                         | Extra fields                         |
|------|-------------------------------------|--------------------------------------|
| 0    | Say (with OK button)                | `hasPrev: bool`, `hasNext: bool`     |
| 1    | Say with image                      | `hasPrev, hasNext`                   |
| 2    | Ask Yes/No                          | _(none)_                             |
| 3    | Ask text input                      | `default: string`, `min: short`, `max: short` |
| 4    | Ask number input                    | `default: int`, `min: int`, `max: int` |
| 5    | Ask choice from menu                | _(none — options embedded in string)_ |
| 6    | Ask avatar (cosmetic selection)     | `count: byte`, `itemIds: int[]`      |
| 7    | Ask android                         | Similar to avatar                    |
| 12   | Ask style (char creation-like)      | `count: byte`, `itemIds: int[]`      |
| 14   | Ask accept (with Accept/Decline)    | `hasPrev, hasNext`                   |

---

### 0x0131 — OPEN_NPC_SHOP

**Purpose:** Opens NPC shop UI.

```
Field        Type    Notes
npcId        int     Shop NPC ID
itemCount    short   Number of items in shop
[Per item:]
  itemId     int
  price      int     Meso cost
  pitch      int     Perfection cost (if price==0)
  [int]      int     0
  [int]      int     0
  [If not rechargeable:]
    qty      short   Stack size (1)
    buyable  short   Purchase limit (0=unlimited)
  [If rechargeable:]
    [short]  short   0
    [int]    int     0
    unitPrice short  Floating-point unit price (as short bits)
    slotMax  short   Maximum stack size
```

---

### 0x0132 — CONFIRM_SHOP_TRANSACTION

**Purpose:** Result of shop buy/sell operation.

```
Field    Type   Notes
code     byte   0=success, 1=no stock, 2=no mesos, 3=inventory full, others=errors
```

---

### 0x0135 — STORAGE

**Purpose:** Opens or updates the storage (Trunk) UI.

---

### 0x0139 — MESSENGER

**Purpose:** Maple Messenger system updates.

---

### 0x013A — PLAYER_INTERACTION

**Purpose:** Player shop / trade / mini-game UI updates.

---

### 0x0142 — PARCEL (Duey)

**Purpose:** Duey delivery notification.

---

### 0x0144 — QUERY_CASH_RESULT

**Purpose:** Sends the player's NX Cash balance.

```
Field     Type   Notes
nxCash    int    NX Credit balance
mCash     int    MaplePoints balance
points    int    Prepaid NX balance
```

---

### 0x0145 — CASHSHOP_OPERATION

**Purpose:** Various Cash Shop UI responses (purchase confirmation, inventory list, gift result, etc.).

---

### 0x014F — KEYMAP

**Purpose:** Sends the character's full keymap configuration.

```
Field     Type    Notes
[byte]    byte    0
[89 entries, one per key slot:]
  type    byte    0=none, 1=skill, 2=item, 4=special
  action  int     Skill/item/action ID
```

---

### 0x0150 — AUTO_HP_POT

**Purpose:** Notifies client that the pet automatically used an HP potion.

```
Field    Type   Notes
itemId   int    Potion item ID
```

---

### 0x0151 — AUTO_MP_POT

**Purpose:** Same as AUTO_HP_POT but for MP.

---

### 0x0162 — VICIOUS_HAMMER

**Purpose:** Response to Vicious Hammer use.

```
Field      Type   Notes
result     byte   0=success, 1=cannot be hammered
vicious    int    New vicious count on the item
[int]      int    0
```

---

### 0x0166 — VEGA_SCROLL

**Purpose:** Vega's Spell (special scroll) result.

```
Field    Type   Notes
result   byte   0=success, 1=failed
```

---

## 11. Shared Packet Structures

### 11.1 CharEntry (Character List Entry)

Used in `CHARLIST` and `ADD_NEW_CHAR_ENTRY`.

```
[CharStats (§11.1a)]
[CharLook (§11.7)]
[byte]          byte   0 (if not view-all)
[If not GM:]
  rankEnabled   byte   1 = rank data present
  worldRank     int
  rankChange    int    Positive = moved up, negative = moved down
  jobRank       int
  jobRankChange int
```

### 11.1a CharStats

```
charId       int
name         fixedStr13   13 bytes, null-padded
gender       byte         0=male, 1=female
skinColor    byte         Skin color ID (0=light, 1=tanned, 2=dark, 3=pale, etc.)
face         int          Face cosmetic ID
hair         int          Hair cosmetic ID
pet1UniqueId long         Pet unique ID (0 if no pet)
pet2UniqueId long
pet3UniqueId long
level        byte
job          short        Job ID (see §13)
str          short
dex          short
int_         short
luk          short
hp           short
maxHp        short
mp           short
maxMp        short
ap           short        Remaining ability points
[SP info]    —            See §11.5
exp          int
fame         short
gachaExp     int
mapId        int          Current map ID
spawnPoint   byte         Spawn portal index
[int]        int          0 (padding)
```

### 11.2 Item Info Structure

Used in inventory operations.

```
[If equip slot (pos < 0):]
  position   short        Absolute equip slot position
[Else:]
  position   byte         Inventory slot (1-based)
itemType     byte         1=equip, 2=consumable, 3=setup, 4=etc, 5=cash
itemId       int
isCash       bool         True if a cash item
[If cash:]
  uniqueId   long         Cash ID / pet ID / ring ID

expiration   long         Expiration timestamp (FILETIME)

[If pet (uniqueId > -1 in item):]
  name       fixedStr13
  level      byte
  tameness   short        (closeness)
  fullness   byte
  expiry2    long
  attribute  short
  petSkill   short        0
  remainLife int          18000
  attribute2 short        0
  RETURN

[If equip:]
  upgradeSlots byte
  level        byte
  str          short
  dex          short
  int_         short
  luk          short
  hp           short
  mp           short
  watk         short
  matk         short
  wdef         short
  mdef         short
  acc          short
  avoid        short
  hands        short
  speed        short
  jump         short
  owner        string      Creator/owner name
  flag         short       Item flag bits
  [If NOT cash equip:]
    [byte]     byte        0
    itemLevel  byte        Equip item level
    itemExp    int         EXP towards next item level
    vicious    int         Vicious Hammer uses remaining
    [long]     long        0
  [If cash equip:]
    [byte*10]  byte[10]    0x40 × 10 (cash padding)
  [long]       long        ZERO_TIME timestamp
  [int]        int         -1

[If consumable/etc/setup:]
  quantity   short
  owner      string
  flag       short
  [If rechargeable:]
    [int]    int           2
    [bytes]  byte[4]       0x54 0x00 0x00 0x34
  RETURN
```

### 11.3 Full Character Info Blob

Used in `SET_FIELD` on initial channel entry.

```
[long]           long    -1
[byte]           byte    0
[CharStats]      —       §11.1a
buddyCapacity    byte
linkedName       byte    0 if no linked character; 1 followed by string
mesos            int
[InventoryInfo]  —       §11.3a
[SkillInfo]      —       §11.3b
[QuestInfo]      —       §11.3c
[MiniGameInfo]   —       short 0
[RingInfo]       —       §11.3d
[TeleportInfo]   —       §11.3e
[MonsterBook]    —       §11.3f
[NewYearInfo]    —       §11.3g
[AreaInfo]       —       §11.3h
[short]          short   0
```

#### 11.3a Inventory Info
```
eqpSlots   byte   Equip inventory capacity
useSlots   byte   Use inventory capacity
setupSlots byte   Setup inventory capacity
etcSlots   byte   Etc inventory capacity
cashSlots  byte   Cash inventory capacity
[long]     long   ZERO_TIME
[Equipped items]       (slot, itemInfo) entries, terminated by short 0
[Cash equipped items]  (slot, itemInfo) entries, terminated by short 0
[Equip inventory]      (slot, itemInfo) entries, terminated by int 0
[Use inventory]        (slot, itemInfo) entries, terminated by byte 0
[Setup inventory]      (slot, itemInfo) entries, terminated by byte 0
[Etc inventory]        (slot, itemInfo) entries, terminated by byte 0
[Cash inventory]       (slot, itemInfo) entries, terminated by byte 0 (implicit)
```

#### 11.3b Skill Info
```
[byte]     byte   0
count      short  Number of non-hidden skills
[Per skill:]
  skillId  int
  level    int
  expiry   long   Expiration FILETIME
  [If 4th job:]
  masterLv int
cooldowns  short  Number of active cooldowns
[Per cooldown:]
  skillId  int
  remaining short  Seconds left
```

#### 11.3c Quest Info
```
startedCount  short  Number of started quests
[Per started:]
  questId     short
  progress    string  Quest tracker string (e.g., "000/010")
  [If infoNumber > 0:]
    infoNum   short
    infoData  string
completedCount short
[Per completed:]
  questId     short
  completedAt long    Completion timestamp (FILETIME)
```

#### 11.3d Ring Info
```
[crush rings, friendship rings, marriage ring]
Each ring entry: ringId: int, [int: 0], partnerRingId: int, [int: 0], itemId: int
```

#### 11.3e Teleport Info
```
[5 ints]   Regular Teleport Rock maps (999999999 = empty slot)
[10 ints]  VIP Rock maps (999999999 = empty slot)
```

#### 11.3f Monster Book Info
```
cover      int    Cover mob ID (0 = none)
[byte]     byte   0
cardCount  short
[Per card:]
  mobId    short  mob ID % 10000
  level    byte   Card level (1–5)
```

#### 11.3g New Year Card Info
```
count      short
[Per card:]
  id       int
  senderId int
  senderName string
  senderDiscarded bool
  dateSent  long
  receiverId int
  receiverName string
  receiverDiscarded bool
  receiverReceived  bool
  dateReceived long
  message  string
```

#### 11.3h Area Info
```
count      short
[Per area:]
  areaId   short
  info     string
```

### 11.4 Movement Fragment List

The client sends movement data as a sequence of typed movement commands. The server re-broadcasts them as-is.

```
commandCount  byte
[Per command:]
  type   byte    Movement type
  [data] —       Varies by type
```

Movement types are encoded per the `moveactions.txt` documentation; the server re-broadcasts the raw byte sequence without parsing individual commands.

### 11.5 SP Table (for jobs with multiple SP pools)

Used by Cygnus Knights and some other jobs instead of a single SP value.

```
count     byte   Number of non-zero SP pools
[Per pool:]
  jobId   byte   SP pool index (1-based job advancement level)
  sp      byte   Remaining SP in that pool
```

### 11.6 Foreign Buff Encoding

Sent as part of `SPAWN_PLAYER` to communicate visible buff states of the spawned character.

```
[int]      int    0
[short]    short  0
[byte]     byte   0xFC
[byte]     byte   1
morphBuff  int    2 if morphed, else 0
firstMask  int    Upper 32 bits of visible buff mask
[buff value byte if COMBO or MORPH]
secondMask int    Lower 32 bits
energyBar  int    1 if energy bar full (Buccaneer), else 0
[short]    short  0
[int]      int    0  (padding)
dashSpeed  int    (1 << 24 if DASH active)
[11 bytes] —      Zero padding
[short]    short  0
dashJump   int    (1 << 24 if DASH active)
[9 bytes]  —      Zero padding
[int]      int    0
[short]    short  0
[byte]     byte   0
mountItemId int   If MONSTER_RIDING: mount item ID, else 0
mountSkillId int  If MONSTER_RIDING: mount skill ID, else 0
CHAR_MAGIC  int   Random seed for timing
[8 bytes]  —      Zero padding
CHAR_MAGIC  int
[byte]     byte   0
CHAR_MAGIC  int
[short]    short  0
[9 bytes]  —      Zero padding
CHAR_MAGIC  int
[int]      int    0
[9 bytes]  —      Zero padding
CHAR_MAGIC  int
[short]    short  0
[short]    short  0
```

### 11.7 Character Look

```
gender     byte
skinColor  byte   Skin tone ID
face       int
[bool]     bool   !mega (false if displaying megaphone effect)
hair       int
[equip slots per position: byte key, int itemId, terminated by 0xFF]
[masked equip slots:       byte key, int itemId, terminated by 0xFF]
cashWeapon int    Cash weapon override (slot -111), or 0
petItemId1 int    Pet item ID for slot 0 (or 0)
petItemId2 int
petItemId3 int
```

### 11.8 Attack Body (broadcast format)

```
charId           int
numAttacked      byte   High nibble = mob count, low nibble = hits per mob
[byte]           byte   0x5B
skillLevel       byte
[If level > 0:]
  skillId        int
display          byte
direction        byte
stance           byte
speed            byte
[byte]           byte   0x0A
projectile       int    Ammo item ID (0 for melee/magic)
[Per target mob:]
  mobOid         int
  [byte]         byte   0
  [If Meso Explosion:]
    hitCount     byte
  [Per hit:]
    damage       int
```

---

## 12. Stat and Buff Mask Reference

### Character Stats (Stat enum — used in STAT_CHANGED)

| Mask value | Name   | Notes                             |
|------------|--------|-----------------------------------|
| 0x1        | Skin   | byte                              |
| 0x2        | Face   | int                               |
| 0x4        | Hair   | int                               |
| 0x8        | Pet1   | long                              |
| 0x10       | Pet2   | long                              |
| 0x20       | Pet3   | long                              |
| 0x40       | Level  | byte                              |
| 0x80       | Job    | short                             |
| 0x100      | Str    | short                             |
| 0x200      | Dex    | short                             |
| 0x400      | Int    | short                             |
| 0x800      | Luk    | short                             |
| 0x1000     | HP     | short                             |
| 0x2000     | MaxHP  | short                             |
| 0x4000     | MP     | short                             |
| 0x8000     | MaxMP  | short                             |
| 0x10000    | AP     | short                             |
| 0x20000    | SP     | short or SP table (§11.5)         |
| 0x40000    | Exp    | int                               |
| 0x80000    | Fame   | short                             |
| 0x100000   | Mesos  | int                               |

### Key Buff Stats (BuffStat — used in GIVE_BUFF / CANCEL_BUFF)

Buffs are represented as two 64-bit masks (`firstMask` and `secondMask`). Each `BuffStat` belongs to one of the two halves.

Notable buffs:
| Name              | Effect                              |
|-------------------|-------------------------------------|
| WATK              | Weapon Attack boost                 |
| WDEF              | Weapon Defense boost                |
| MATK              | Magic Attack boost                  |
| MDEF              | Magic Defense boost                 |
| SPEED             | Move speed boost                    |
| JUMP              | Jump boost                          |
| MAGIC_GUARD       | Split damage between HP and MP      |
| DARKSIGHT         | Invisible to monsters               |
| BOOSTER           | Weapon attack speed boost           |
| POWER_GUARD       | Reflect portion of damage back      |
| HYPER_BODY_HP     | Max HP increase %                   |
| HYPER_BODY_MP     | Max MP increase %                   |
| INVINCIBLE        | Temporary invincibility             |
| COMBO             | Crusader combo orb count            |
| SHADOWPARTNER     | Shadow Partner skill active         |
| SOULARROW         | Soul Arrow (no ammo consumption)    |
| MORPH             | Character morph transformation      |
| MONSTER_RIDING    | Mount riding active                 |
| MESOGUARD         | Meso Guard skill (absorbs damage)   |
| HOMING_BEACON     | Marksman skill target lock          |
| DASH              | Pirate Dash skill active            |
| ENERGY_CHARGE     | Knuckler energy bar state           |
| WIND_WALK         | Wind Walk (invisible + speed)       |
| MAGIC_SHIELD      | Magic Shield skill                  |
| AURA              | Warrior Aura (damage reduction)     |
| BODY_PRESSURE     | Aran Body Pressure counter          |
| COMBO_BARRIER     | Aran Combo Barrier (dmg reduction)  |

### Monster Status Bits (MonsterStatus — used in APPLY_MONSTER_STATUS)

Stored as 4 × int32 (128 bits):

| Name             | Effect                               |
|------------------|--------------------------------------|
| WATK             | Monster weapon attack up             |
| WDEF             | Monster weapon defense up            |
| MATK             | Monster magic attack up              |
| MDEF             | Monster magic defense up             |
| ACC              | Accuracy up                          |
| AVOID            | Avoidability up                      |
| SPEED            | Speed up                             |
| STUN             | Stunned (cannot move or attack)      |
| FREEZE           | Frozen                               |
| POISON           | Poisoned (periodic damage)           |
| SEAL             | Cannot use skills                    |
| TAUNT            | Taunted toward player                |
| MIST_ERUPTION    | Activated by mist                    |
| WEAPON_REFLECT   | Reflects physical damage             |
| MAGIC_REFLECT    | Reflects magic damage                |
| NEUTRALISE       | Stunned by Aran Body Pressure        |
| DOOM             | Transformed into snail               |
| SHADOW_WEB       | Slowed movement                      |
| WEAPON_IMMUNITY  | Immune to physical attacks           |
| MAGIC_IMMUNITY   | Immune to magic attacks              |

---

## 13. Enumeration Values

### Job IDs (partial list)

| ID   | Job                     |
|------|-------------------------|
| 0    | Beginner                |
| 100  | Warrior                 |
| 110  | Fighter                 |
| 111  | Crusader                |
| 112  | Hero                    |
| 120  | Page                    |
| 121  | White Knight            |
| 122  | Paladin                 |
| 130  | Spearman                |
| 131  | Dragon Knight           |
| 132  | Dark Knight             |
| 200  | Magician                |
| 210  | F/P Wizard              |
| 211  | F/P Mage                |
| 212  | F/P Arch Mage           |
| 220  | I/L Wizard              |
| 221  | I/L Mage                |
| 222  | I/L Arch Mage           |
| 230  | Cleric                  |
| 231  | Priest                  |
| 232  | Bishop                  |
| 300  | Archer                  |
| 310  | Hunter                  |
| 311  | Ranger                  |
| 312  | Bowmaster               |
| 320  | Crossbowman             |
| 321  | Sniper                  |
| 322  | Marksman                |
| 400  | Rogue                   |
| 410  | Assassin                |
| 411  | Hermit                  |
| 412  | Night Lord              |
| 420  | Bandit                  |
| 421  | Chief Bandit            |
| 422  | Shadower                |
| 430  | Blade Recruit → Master  |
| 500  | Pirate                  |
| 510  | Brawler                 |
| 511  | Marauder                |
| 512  | Buccaneer               |
| 520  | Gunslinger              |
| 521  | Outlaw                  |
| 522  | Corsair                 |
| 900  | GM                      |
| 910  | Super GM                |
| 1000 | Noblesse (Cygnus)       |
| 1100 | Dawn Warrior            |
| 1110 | Dawn Warrior 2          |
| 1111 | Dawn Warrior 3          |
| 1112 | Dawn Warrior 4          |
| 1200 | Blaze Wizard            |
| 1300 | Wind Archer             |
| 1400 | Night Walker            |
| 1500 | Thunder Breaker         |
| 2000 | Legend (Aran)           |
| 2100 | Aran 1                  |
| 2110 | Aran 2                  |
| 2111 | Aran 3                  |
| 2112 | Aran 4                  |

### Inventory Types

| ID | Name   |
|----|--------|
| 1  | Equip  |
| 2  | Use    |
| 3  | Setup  |
| 4  | Etc    |
| 5  | Cash   |

### Drop Types

| ID | Meaning                                    |
|----|--------------------------------------------|
| 0  | Timed private (only owner can pick up)     |
| 1  | Party-timed (party can pick up)            |
| 2  | Free-for-all                               |
| 3  | Explosive / instant FFA                    |

### Quest Action Types

| ID | Meaning                |
|----|------------------------|
| 0  | Restore lost item      |
| 1  | Start quest            |
| 2  | Complete quest         |
| 3  | Forfeit quest          |
| 4  | Scripted quest start   |
| 5  | Scripted quest end     |

### Party Operations (client send)

| ID | Action         |
|----|----------------|
| 1  | Create party   |
| 2  | Leave/disband  |
| 3  | Join party     |
| 4  | Invite player  |
| 5  | Expel member   |
| 6  | Change leader  |

---

*End of Cosmic MapleStory Server Network Protocol Reference*
