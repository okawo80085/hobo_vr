"""
pyvr.

The most commonly used pyvr commands are:
   calibrate  Calibrate the vr headset's tracking
   track      Start the tracking system
   server     Start the server that communicates with SteamVR

See 'pyvr help <command>' for more information on a specific command.

usage: pyvr <command> [<args>...]

options:
   -h, --help
"""

from docopt import docopt

from virtualreality import __version__


def main():
    """Pyvr entry point."""
    args = docopt(__doc__, version=f"pyvr version {__version__}", options_first=True)

    if args["<command>"] == "calibrate":
        from virtualreality.calibration.manual_color_mask_calibration import main

        main()
    else if args["<command>"] == "calibrate-cam":
        from virtualreality.calibration.camera_calibration import main

        main()
    elif args["<command>"] == "track":
        from virtualreality.trackers.color_tracker import main

        main()
    elif args["<command>"] == "server":
        from virtualreality.server.server import run_til_dead

        run_til_dead()
    elif args["<command>"] in ["help", None]:
        docopt(__doc__, argv="--help")
    else:
        exit(f"{args['<command>']} is not a valid pyvr command. See 'pyvr help'")


if __name__ == "__main__":
    main()
