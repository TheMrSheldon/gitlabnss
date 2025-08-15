# Installation
1. Move `libnss_gitlab.so` into `/usr/lib/`
2. Add `gitlab` to `/etc/nsswitch.conf`

# With SSH
1. Add these lines to `/etc/ssh/sshd_config`:
```
AuthorizedKeysCommand /bin/fetchgitlabkeys
AuthorizedKeysCommandUser root
```

# Running
On Ubuntu run `systemctl enable gitlabnssd` or `/etc/init.d/gitlabnssd start` to start the service.

# Development
## Nameing

https://www.gnu.org/software/libc/manual/html_mono/libc.html#NSS-Module-Names

`_nss_<service>_<function>`
