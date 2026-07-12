# pm

This is another package manager that reads a Lua configuration file and
coordinates the download, build, and installation from remote URLs. Packages are
processed in parallel using POSIX threads.

You might be asking why I wrote another Package Manager. Good question. I don't
know either. Update: appart from the rest, this one works.


## Usage

`pm [option]...`

- **Install/Update**: `pm`. Just like that. Simple. Elegant. And it works.

| Command              | Description                                              |
|----------------------|----------------------------------------------------------|
| `pm`                 | Fetch, build, and install all packages from config       |
| `pm --help` / `pm -h`  | Show help message and exit                               |
| `pm --version` / `pm -v` | Show version info and exit                             |
| `pm init` / `pm i`   | Create a template config in `~/.config/pm/init.lua`      |
| `pm threads N` / `pm t N` | Use at most N threads (default: CPU count)          |
| `pm list` / `pm l`   | List all packages defined in the config                  |
| `pm system` / `pm s` | Print system configuration paths and settings            |

## Configuration

The config file lives at `~/.config/pm/init.lua` and defines:

```lua
System = {
    build = { path = os.getenv("HOME") .. "/.local/share/pm/cache/" },
    bin   = { path = os.getenv("HOME") .. "/.local/share/pm/bin/" },
}

Packages = {
    {
        url = "...",
        build = "...",
        artifact = "...",
    },
}
```

It's plain Lua, so you can use `require`, helper functions, and split packages
across multiple files. See the [example](example/config/pm/) directory for
reference.

## Build

```sh
make          # release build
make debug    # debug build with sanitizers
```

Requires `lua`, `libcurl`, and `libcrypto` (OpenSSL). The Makefile installs the
man page to `~/.local/share/man/man1/pm.1`.



:wq (nvim prank)
