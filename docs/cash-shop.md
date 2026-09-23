# Cash Shop

Open the shop with the HUD Cash Shop button or the configured Cash Shop key
(default: grave/backtick). Exit or Escape returns to the current channel.

The shop uses the original 800×600 GMS interface in `assets/UI_83.nx`: silver
frames, a character preview at upper left, ten catalog offers, Best Item column,
Cash Inventory, Item Inventory, currency totals and the mushroom Exit button.
It is centered in larger viewports. The existing UI archive remains in use for
other game screens. All assets are read-only.

- Tabs, equipment subcategories, name search, page buttons and mouse wheel browse
  the local Commodity catalog. Wish List holds ten server-saved commodity SNs.
- Double-click clothing to preview it. Turn, Reset, Undress and three backdrops
  affect the preview. Buy Avatar reviews the previewed outfit and its total.
- Buy confirms quantity, duration, price and payment currency. Balances and
  remaining balances are shown for NX Credit, Maple Points and NX Prepaid.
  Packages use the package operation; meso offers use the meso operation.
- Gift asks for the recipient, message and account birthday, using NX Prepaid.
  Coupon and the four-slot inventory/storage expansion buttons use existing
  Cosmic operations. Check Cash refreshes balances; Charge explains that NX
  funding is managed outside the client.
- Double-click items between Cash Inventory and Item Inventory. Mouse wheel over
  either panel pages its contents. Hover for full item names and expiration.
  Unequip cosmetics before depositing them. The in-game equipment window has
  separate Equip/Cash tabs to select each equipped layer.
- The preview speech strip shows status; hover it for the full message. Server
  failures open a notice. Purchases and transfers are serialized. An unconfirmed
  request blocks further transactions and permits Exit after 30 seconds. Outfit
  purchases stop on failure; already-confirmed items remain purchased.

## Boundaries

Only the client changes. Server balances, restrictions, prices, inventory and
expiration remain authoritative. The supplied Commodity/CashPackage data matches
the local Cosmic data. Special server catalog modifiers are parsed but are not
used to invent price or availability overrides.

Relationship rings, character-slot coupons, name changes and world transfers
need their dedicated service workflows and are blocked from ordinary purchases.
Pet items can be purchased and transferred; the fitting preview renders clothing.
New gifts require exiting and reopening once: Cosmic creates the gift items
after its initial locker snapshot, which lacks their final instance metadata.

## Tests

Run the self-contained Cash Shop cases from the repository root:

```bash
./scripts/run_tests.sh -- -R cash_shop
```

These cases check outgoing packet layouts, cash item identity and expiration,
equipment moves, full inventories, pet and consumable decoding, and rejection of
truncated item records. They need no NX assets or running Cosmic server and are
included in the default suite and native-test CI.

Include the local catalog and classic artwork checks with:

```bash
./scripts/run_tests.sh --assets -- -R cash_shop
```

The `--assets` option requires the NX set and native graphics dependencies listed
in the [testing guide](testing.md#tests-using-nx-assets), including `UI_83.nx` for
the classic Cash Shop artwork. Assets are read-only. When local dependencies are
unavailable, use the same filter with the Docker fallback:

```bash
./scripts/docker_run_tests.sh --assets -- -R cash_shop
```

Related equipment and appearance regressions can be run together:

```bash
./scripts/run_tests.sh --assets -- -R 'cash_shop|equip_inventory|face_accessory'
```

The default local asset run writes JUnit results to
`build/tests/native-Debug-assets/reports/junit.xml`. Equipment and appearance
tests also write `equipment.ppm` and `face-accessories.ppm` under their respective
`work/equip_inventory_test/artifacts/` and `work/face_accessory_test/artifacts/`
directories within that build. These images are diagnostic captures, not
automated pixel-comparison baselines. See the
[reporting guide](testing.md#build-directories-and-reports) for other variants.

The current tests exercise client code offline; browser interaction and live
Cosmic transactions are outside their coverage. Validate the WASM build
separately with `./scripts/build_wasm.sh --jobs 4`.

## Cash equipment coverage

The equipment window uses the native tabbed `UIWindow4.img/Equip` artwork from
`UI.nx`. Slot artwork, icon centers and hitboxes share the asset's slot origins,
including independent placement for the four linked ring images. Equip and Cash
tabs address the normal and +100 equipped layers. Close/Escape and title dragging
use the standard floating-window behavior.

Pet/AD tabs and unsupported slots are faded, noninteractive and explain their
availability on hover. Android, Heart, Badge, Pocket, Shoulder, Emblem and other
newer slots still have no corresponding support in this client's equipment model.
The artwork does not enable those systems. The `equip_inventory` regression
checks native geometry, linked origins, hitboxes, separate inventory addresses,
disabled slots and icon drawing. Run it with the commands in [Tests](#tests).

| Category | Current support |
| --- | --- |
| Hats, eye accessories, earrings, tops, overalls, pants, shoes, gloves, capes, shields | Base slot selection and clothing rendering exist. Individual items and extra visual effects have not all been validated. |
| Face accessories | The `face_accessory` regression checks expression-based rendering, brow attachment, mirroring and face/hat layers across the local face-accessory catalog. |
| Cash weapon covers (`170xxxx`) | Incomplete: missing equipment-slot classification, weapon-type-specific artwork selection and application of the remote cash-weapon override. |
| Rings | All four slots have distinct artwork and hitboxes in both tabs. Cash ring slot selection remains incomplete; relationship, name/chat and item-effect rendering are absent. |
| Pet equipment | Not implemented as wearable pet equipment; the Pet tab is visibly unavailable. |
| Mounts and saddles | Slot constants exist, but this does not provide complete item classification, rendering or equipment UI support. |
| Extra equipment effects | The shared item-effect system is not implemented. Base clothing support does not imply support for every effect attached to an item. |

A successful Cash Shop purchase or locker transfer does not establish that an
item can be equipped or that all its appearance/effects are supported.
