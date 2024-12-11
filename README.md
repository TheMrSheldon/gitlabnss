# Installation
1. Move `libnss_gitlab.so` into `/usr/lib/`
2. Run `find / -name "pam_unix.so" 2>/dev/null` to find where pam modules are placed on your system
3. Move and rename `libnss_gitlab.so` to `<pam-path>/pam_gitlab.so`

## Setting up GitLab Login with ssh:
1. Add `auth    sufficient    pam_gitlab.so` to `/etc/pam.d/sshd`

# Development
## Nameing

https://www.gnu.org/software/libc/manual/html_mono/libc.html#NSS-Module-Names

`_nss_<service>_<function>`
