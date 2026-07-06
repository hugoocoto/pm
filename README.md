# pm

You might be asking why I wrote another Pacmage Manager. Good question. I don't
know either.

## Usage

- **Deploy the template**: `pm --init` would create a template configuration in
  `~/.config/pm/init.lua`.

- **Update your files**: `pm`. Just like that. Simple. Elegant. And it works.

## Build

There is a Makefile. But to be honest, it's a single main.c, do whatever you
want. Then you may want to move the executable to `.local/bin` or whatever. At
least this is what the bootstraping do. You can change it from the configuration
file.








:wq (nvim prank)
