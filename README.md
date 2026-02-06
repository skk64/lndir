# lndir

Cli tool to quickly duplicate a directory structure by:

- Copying the entire directory structure
- Hardlinking every file in the directory structure to the same relative location.

By using hardlinks, no additional space is used, and file duplication is near-instant.

## Use cases

### Safe file reorganising

Files can be safely reorganised and renamed without risk of accidental deletion.

### Dotfile management

This acts as a more general version of GNU stow. Create a directory with dotfiles in it, as they would be in your home directory, then run lndir to link them to their correct locations.

## Compilation

Requires liburing, which may not be installed by default, but can be e.g., on Ubuntu with `apt-get install liburing-dev`

To compile, run:
```
make
```
