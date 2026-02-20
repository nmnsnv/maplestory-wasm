## WZ to NX (Docker, NoLifeWzToNx)

This workflow runs fully inside Docker:
- clones `https://github.com/ryantpayton/NoLifeWzToNx`
- builds the converter
- converts `wz/*.wz` to `nxFiles/*.nx`

No host-side Go installation is required.

### Run

```bash
./scripts/wz-converter/convert.sh
```

Or directly:

```bash
docker compose -f scripts/wz-converter/docker-compose.yml run --rm wz-converter
```

### Inputs/Outputs

- Input WZ files: `wz/*.wz`
- Cached clone: `.cache/tools/NoLifeWzToNx/`
- Built binary: `.cache/tools/bin/NoLifeWzToNx`
- Converted output: `nxFiles/*.nx`

### Environment knobs

Set with compose overrides or shell env before running:

- `NO_LIFE_WZ_TO_NX_REF` (default: `master`)
- `MODE` (`client` or `server`, default: `client`)
- `LZ4HC` (`1` enables `-h` high-compression mode, default: `0`)
- `SKIP_LIST_WZ` (`1` skips `List.wz`, default: `1`)
