import sys

from PyQt6.QtWidgets import QApplication
from PyQt6.QtGui import QFont

from dashboard import Dashboard


def main():

    # Initialize PyQt application
    app = QApplication(sys.argv)

    # Application name
    app.setApplicationName("Embedded Data Acquisition Monitor")

    # Default application font
    app.setFont(QFont("Arial", 10))

    # Create dashboard window
    window = Dashboard()

    # Display dashboard
    window.show()

    # Start GUI event loop
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
