import tkinter as tk
import subprocess
import os
from typing import Optional, Union, List
import logging
from sys import platform
from os.path import expanduser
home = expanduser("~")

logging.getLogger(__name__).setLevel(logging.INFO)


info = logging.getLogger(__name__).info


class SubprocessWidget(object):
    # thanks: https://gist.github.com/zed/9294978

    def __init__(self, root: tk.Tk, parent: Union[tk.Tk, tk.Frame], command: List[str], name: Optional[str] = None):
        """Start subprocess, make GUI widgets."""
        self.root = root

        # start subprocess
        self.proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=False)

        # need to put this before createfilehandler
        self.server_frame = tk.Frame(parent)
        self.text = tk.Text(self.server_frame, width=80, bg="black", fg="green")
        self.text.pack(side="left", fill="both")
        self.scroll = tk.Scrollbar(self.server_frame)
        self.scroll.pack(side="right", fill="y")
        self.scroll.config(command=self.text.yview)
        self.text.config(yscrollcommand=self.scroll.set)
        # stop subprocess using a button
        #  if name is None:
        #      name = " ".join(command)
        # tk.Button(self.server_frame, text=f"Stop {name}", command=self.stop).pack()
        self.server_frame.pack(side="top")

        # show subprocess' stdout in GUI
        self.root.createfilehandler(
            self.proc.stdout, tk.READABLE, self.read_output)

    def read_output(self, pipe, mask):
        """Read subprocess' output, pass it to the GUI."""
        data = os.read(pipe.fileno(), 1 << 20)
        print(data)
        if not data:  # clean up
            info("eof")
            self.root.deletefilehandler(self.proc.stdout)
            self.root.after(1, self.stop)  # stop in 5 seconds
            return
        info("got: %r", data)
        if self.text is not None:
            self.text.insert(tk.END, data.decode())

    def stop(self):
        """Stop subprocess and quit GUI."""
        info('stopping')
        self.proc.terminate()  # tell the subprocess to exit

        # kill subprocess if it hasn't exited after a countdown
        def kill_after(countdown):
            if self.proc.poll() is None:  # subprocess hasn't exited yet
                countdown -= 1
                if countdown < 0:  # do kill
                    info('killing')
                    self.proc.kill()  # more likely to kill on *nix
                else:
                    self.root.after(1000, kill_after, countdown)
                    return  # continue countdown in a second

            self.proc.stdout.close()  # close fd
            self.proc.wait()  # wait for the subprocess' exit

        kill_after(countdown=5)

        self.server_frame.destroy()
        self.text.destroy()
        self.scroll.destroy()


class Application(tk.Frame):
    def __init__(self, master=None):
        super().__init__(master)
        self.master = master
        self.pack()

        self.server_frame: Optional[tk.Frame] = None
        self.server_button = None
        self.server_subprocess_gui: Optional[SubprocessWidget] = None
        self.create_server_widgets()

        self.client_frame: Optional[tk.Frame] = None
        self.client_sub_frame = None
        self.client_button = None
        self.client_calibration_label = None
        self.client_calibration_location = None
        self.client_pass_cmd: Optional[subprocess.Popen] = None
        self.client_subprocess_gui: Optional[SubprocessWidget] = None
        self.create_client_widgets()

    def end(self):
        self.client_pass_cmd.terminate()

    ##########
    # CLIENT #
    ##########

    def create_client_widgets(self):
        self.client_frame = tk.Frame()
        self.client_button = tk.Button(self.client_frame)
        self.client_button["text"] = "Start Client"
        self.client_button["command"] = self.start_client
        self.client_button.pack(side="top")

        self.client_sub_frame = tk.Frame(self.client_frame)
        self.client_calibration_location = tk.Entry(self.client_sub_frame)
        self.client_calibration_location.insert(tk.END, home+os.sep+"ranges.pickle")
        self.client_calibration_location.pack(side="right")
        self.client_calibration_label = tk.Label(self.client_sub_frame)
        self.client_calibration_label["text"] = "Calibration Data File:"
        self.client_calibration_label.pack(side="right")
        self.client_sub_frame.pack(side="top")

        self.client_frame.pack(side="top")

    def start_client(self):
        calibration_file = self.client_calibration_location.get()
        calibration_rules = []
        platform_rules = []
        if (platform == "linux" or platform == "linux2") and (os.geteuid() != 0):
            self.client_pass_cmd = subprocess.Popen(["/usr/lib/policykit-1-gnome/polkit-gnome-authentication-agent-1"])
            platform_rules = ["pkexec"]
        if calibration_file:
            calibration_rules = ["--load_calibration", calibration_file]
        self.client_subprocess_gui = SubprocessWidget(self.master, self.client_frame,
                                                      platform_rules + ["pyvr", "track"] + calibration_rules, "Client")
        self.client_button["text"] = "Stop Client"
        self.client_button["command"] = self.stop_client

    def stop_client(self):
        self.client_subprocess_gui.stop()
        self.client_button["text"] = "Start Client"
        self.client_button["command"] = self.start_client

    ##########
    # SERVER #
    ##########

    def create_server_widgets(self):
        self.server_frame = tk.Frame()
        self.server_button = tk.Button(self.server_frame)
        self.server_button["text"] = "Start Server"
        self.server_button["command"] = self.start_server
        self.server_button.pack(side="top")

        self.server_frame.pack(side="top")

    def start_server(self):
        self.server_subprocess_gui = SubprocessWidget(self.master, self.server_frame, ["pyvr", "server"], "Server")
        self.server_button["text"] = "Stop Server"
        self.server_button["command"] = self.stop_server

    def stop_server(self):
        self.server_subprocess_gui.stop()
        self.server_button["text"] = "Start Server"
        self.server_button["command"] = self.start_server


def main():
    root = tk.Tk()
    app = Application(master=root)
    app.mainloop()


if __name__ == '__main__':
    main()
