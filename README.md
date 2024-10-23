# lndir

Cli tool to quickly duplicate a directory structure by:

- Copying the entire directory structure
- Hardlinking every file in the directory structure to the same relative location.

By using hardlinks, no additional space is used, and file duplication is near-instant.

## Use cases

### Safe file reorganising

Files can be safely reorganised and renamed without risk of accidental deletion.

### Application specific uses

A directory of movies can be structured for multiple separate apps (e.g. Jellyfin and Plex) without using extra storage space.

## Compilation

Requires zig >= 0.14

```
zig build -Doptimize=ReleaseSmall
```
