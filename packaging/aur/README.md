# Arch package

`PKGBUILD` builds Inkpot on Arch and anything based on it: Manjaro, EndeavourOS,
CachyOS and the rest.

## Installing without the AUR

The file on its own is enough; nothing has to be published first.

```bash
curl -O https://raw.githubusercontent.com/Neklor/Inkpot/main/packaging/aur/PKGBUILD
makepkg -si
```

`makepkg` fetches the release, builds it, and hands the finished package to
pacman. Afterwards Inkpot behaves like any other package: `pacman -Qi inkpot`
shows it, `pacman -R inkpot` removes it cleanly.

## Publishing it on the AUR

Needs an AUR account with an SSH key.

```bash
git clone ssh://aur@aur.archlinux.org/inkpot.git
cd inkpot
cp /path/to/PKGBUILD .
makepkg --printsrcinfo > .SRCINFO      # has to be regenerated on every change
git add PKGBUILD .SRCINFO
git commit -m "Add inkpot 1.1.0"
git push
```

## With every new release

1. Set `pkgver`, put `pkgrel` back to 1.
2. New checksum: `updpkgsums` (from the pacman-contrib package).
3. Rebuild `.SRCINFO`, commit, push.
