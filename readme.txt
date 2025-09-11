                                       pm
pm package manager.


DESCRIPTION
        pm is a package manager. It solves the problem of handle packages from
        external repositories. Configuration is done using pm.config in the same
        directory as the source file at compilation. This means that the config
        is applied at compilation so pm must be rebuilt every time config file
        changes.


STANDARDS
        c99 for unix like systems


SOURCE CODE
        Source code is avaliable for free. You can download it from github:
        https://github.com/hugoocoto/pm


EXAMPLES
        'pm.config' that installs the latest version of st and vicel:

                PAK(.name = "st",
                    .git = "https://git.suckless.org/st",
                    .branch = "master")

                PAK(.name = "vicel",
                    .git = "https://github.com/hugoocoto/vicel")


AUTOR
        Hugo Coto Flórez (me@hugocoto.com). Github profile:
        https://github.com/hugoocoto

LICENSE
        GNU Public License

