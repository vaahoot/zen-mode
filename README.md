# zen

A macOS CLI tool that blocks domains of your choice by editing `/etc/hosts`. Supports plain blocking and Pomodoro-style work/rest cycles.

Requires `sudo` because it modifies `/etc/hosts` and flushes the DNS cache.

## Build

```
git clone https://github.com/vaahoot/zen-mode.git zen
cd zen
make
```

Produces `out/zen`.

## Usage

```
sudo ./out/zen -b reddit.com youtube.com         # block until Ctrl-C
sudo ./out/zen -b youtube.com -w 25 -r 5         # 25 min work / 5 min rest cycle
sudo ./out/zen -u youtube.com                    # unblock a specific domain
sudo ./out/zen -U                                # unblock everything zen manages
```

Flags:

- `-b, --block` — domains to block. Runs as a foreground process until interrupted; Ctrl-C unblocks and exits.
- `-u, --unblock` — domains to unblock.
- `-U, --unblock-all` — remove every entry zen added.
- `-w, --work MINUTES` — work phase duration (requires `-r`).
- `-r, --rest MINUTES` — rest phase duration (requires `-w`).
- `-h, --help` — show help.

Both the bare domain and its `www.` variant are blocked. Entries are tagged with a `# ZEN_BLOCK` marker so zen only touches its own lines.

## Important: browser caching

Browsers cache DNS lookups internally and will keep resolving blocked domains until that cache is cleared. To make blocking take effect you must either:

- **start zen before opening the browser**, or
- **restart the browser after starting zen**.

The same applies after a rest phase ends and blocking resumes — if the browser stayed open through the rest window it may have cached the real IP, and will need to be restarted for the next work phase to block effectively.

## How it works

- Zen appends `127.0.0.1 domain # ZEN_BLOCK` entries to `/etc/hosts` to redirect those domains to localhost, then rewrites the file to remove them on unblock.
- macOS's `osascript` is used to send notifications.
- DNS cache is flushed via `dscacheutil -flushcache` and `killall -HUP mDNSResponder` after every hosts mutation.

## License
MIT

## P.S

I am aware that it's easy to get around it, the point of the app is to prevent unconscious doom scrolling sessions, not create an unbreakable wall.
