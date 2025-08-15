
# For Users
1. Install from the debian package: `sudo dpkg -i nssgitlab.deb`
2. Configure NSS:
3. Optionally to support SSH login (via public keys added to GitLab):
    Add these lines to `/etc/ssh/sshd_config`
    ```
    AuthorizedKeysCommand /bin/fetchgitlabkeys
    AuthorizedKeysCommandUser root
    ```
4. Modify `/etc/gitlabnss/gitlabnss.conf` to fit your needs. Particularly: set `base_url` to the GitLab API endpoint of your choice and `secret` to a **FILE** that contains the API key. Make sure that the `secret` file can only be read by root (owner: `root` and permissions `0400`). 
5. Start the service: On Ubuntu run `systemctl enable gitlabnssd` or `/etc/init.d/gitlabnssd start` to start the service.

## How it Works
**gitlabnssd** Daemon process that listens to a UNIX file socket (configured in `gitlabnss.conf`; default: `/var/run/gitlabnss.sock`) and provides means of fetching GitLab user information by ID or name. Technically, the consumers of this API (NSS and fetchgitlabkeys) could access the GitLab API directly but the API key then has to be readable by artbitrary users which is a security risk.

**NSS** `TODO`

**fetchgitlabkeys** If you want GitLab users to be able to login using SSH and the public keys configured in GitLab, you can direct the `AuthorizedKeysCommand` to use `fetchgitlabkeys` to load these keys. For reasons explained above, `fetchgitlabkeys` does not access the GitLab API directly but communicates with the daemon using `gitlabnss.sock`.


\dot
digraph G {

  subgraph cluster_0 {
    style=filled;
    color=lightgrey;
    node [style=filled,color=white];
    a0 -> a1 -> a2 -> a3;
    label = "process #1";
  }

  subgraph cluster_1 {
    node [style=filled];
    b0 -> b1 -> b2 -> b3;
    label = "process #2";
    color=blue
  }
  start -> a0;
  start -> b0;
  a1 -> b3;
  b2 -> a3;
  a3 -> a0;
  a3 -> end;
  b3 -> end;

  start [shape=Mdiamond];
  end [shape=Msquare];
}
\enddot


# For Developers
## Installation
1. Move `libnss_gitlab.so` into `/usr/lib/`
2. Add `gitlab` to `/etc/nsswitch.conf`

## With SSH
1. Add these lines to `/etc/ssh/sshd_config`:
```
AuthorizedKeysCommand /bin/fetchgitlabkeys
AuthorizedKeysCommandUser root
```

## Naming
https://www.gnu.org/software/libc/manual/html_mono/libc.html#NSS-Module-Names

`_nss_<service>_<function>`
