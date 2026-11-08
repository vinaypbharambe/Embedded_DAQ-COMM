import os
import subprocess
import threading
from pathlib import Path
from PyQt6.QtCore import QThread, pyqtSignal


class SimulatorProcess(QThread):
    # Signals for communicating with the GUI
    output_line = pyqtSignal(str)
    process_finished = pyqtSignal(int)
    process_error = pyqtSignal(str)

    def __init__(self, mode):
        super().__init__()

        self.mode = mode
        self.process = None

        # Handles Stop requests, including requests
        # received before the process starts.
        self.stop_requested = threading.Event()

    # =====================================
    # RUN SIMULATOR
    # =====================================

    def run(self):
        project_dir = Path(__file__).resolve().parent.parent / "just_emulator"
        executable = project_dir / "embedded_simulator"

        # Check executable exists
        if not executable.is_file():
            self.process_error.emit(
                f"Simulator executable not found: {executable}"
            )
            return

        # Check executable permission
        if not os.access(executable, os.X_OK):
            self.process_error.emit(f"Simulator is not executable: {executable}")
            return

        # Check whether Stop was requested before launch
        if self.stop_requested.is_set():
            return

        # Command to execute
        command = ["stdbuf", "-oL", str(executable), self.mode]

        try:
            # Start C simulator
            self.process = subprocess.Popen(
                command,
                cwd=project_dir,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
            )

            # Handle Stop requests that arrived during launch
            if self.stop_requested.is_set():
                self.process.terminate()

            # Read output continuously
            if self.process.stdout is not None:
                for line in self.process.stdout:
                    if line:
                        self.output_line.emit(line.rstrip("\r\n"))

            # Wait for process completion
            return_code = self.process.wait()
            self.process_finished.emit(return_code)

        except FileNotFoundError as error:
            self.process_error.emit(f"Unable to start simulator: {error}")

        except Exception as error:
            self.process_error.emit(f"Simulator error: {error}")

        finally:
            if self.process is not None:
                if self.process.poll() is None:
                    self.process.kill()
                    self.process.wait()

                if self.process.stdout is not None:
                    self.process.stdout.close()

                self.process = None

    # =====================================
    # STOP SIMULATOR
    # =====================================

    def stop(self):
        # Remember the Stop request
        self.stop_requested.set()

        if self.process is not None:
            if self.process.poll() is None:
                # Request graceful termination
                self.process.terminate()

                try:
                    # Give the process time to exit
                    self.process.wait(timeout=2)

                except subprocess.TimeoutExpired:
                    # Force termination if necessary
                    self.process.kill()
                    self.process.wait()
