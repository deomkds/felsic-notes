import sys, os
from PyQt6.QtWidgets import QApplication, QTreeView, QWidget
from PyQt6.QtGui import QFileSystemModel

app = QApplication(sys.argv)
t = QTreeView()
if hasattr(t, "expandRecursively"):
    print("expandRecursively exists!")
else:
    print("missing expand")
