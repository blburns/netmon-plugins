# Tools

Utility scripts for operating and validating NetMon Plugins installations.

## Scripts

| Script | Purpose |
|--------|---------|
| `list-plugins.sh` | List all plugins from `plugin_list.txt` |
| `validate-build.sh` | Verify all expected plugin binaries exist in `build/` |
| `run-check.sh` | Run a plugin with optional logging |

## Usage

```bash
# List available plugins
./tools/list-plugins.sh

# Verify local build output
./tools/validate-build.sh

# Run a check with logging
./tools/run-check.sh disk -w 80 -c 90 /
./tools/run-check.sh http -H example.com -p 443 -S
```

## Environment variables

| Variable | Default | Description |
|----------|---------|-------------|
| `PLUGIN_DIR` | `./build` or `/usr/local/libexec/monitoring-plugins` | Plugin binary directory |
| `LOG_FILE` | none | When set, append check output to this file |

## Related

- Build scripts: `scripts/`
- Deployment configs: `deployment/`
- Global config examples: `config/`
