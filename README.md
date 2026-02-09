# lndir

Cli tool to quickly duplicate a directory structure by:

- Copying the entire directory structure
- Hardlinking every file in the directory structure to the same relative location.

By using hardlinks, no additional space is used, and file duplication is near-instant.

lndir is linux-only, as it uses the ui_uring api. Also, hardlinking files doesn't make sense to do in Windows or MacOS, as they don't really support it.

## Use cases

### Safe file reorganising

Files can be safely reorganised and renamed without any risk of data loss. Just duplicate the directory, e.g. `lndir Documents Documented_backup`, then make changes in Documents. If anything is accidentally deleted/renamed, the original files will all remain in the backup directory.

Note that this doesn't backup the file contents, only the files, filenames, and directory structure. Any file edited in one directory will have the changes be reflected in the other. This property is what makes the next example possible.

### Dotfile management

This acts as a more general version of GNU stow. Create a directory with dotfiles in it that matches the structure of your home directory. Then run `lndir dotfiles/ /home/<your username> to automatically link all of them to their correct locations.

## Compilation

Requires liburing, which may not be installed by default, but can be e.g., on Ubuntu with `apt-get install liburing-dev`

To compile, run:
```
make
```
