import shutil
import os
import rsa
import pefile

BASE_DIR = os.path.dirname(__file__)

# Copies files from one directory to another.
def CopyMarv(dir, needle):
    for file in os.listdir(dir):
        if file.endswith(needle):
            final = os.path.join("Temp", dir.replace(BASE_DIR, ""), file)
            os.makedirs(os.path.dirname(final), exist_ok=True)
            shutil.copyfile(os.path.join(dir.replace(BASE_DIR, ""), file), os.path.join("Temp", dir.replace(BASE_DIR, ""), file))

CopyMarv(BASE_DIR, "Cordycep.CLI.exe")
CopyMarv(BASE_DIR, "Greyhound.exe")
CopyMarv(BASE_DIR, ".md")
CopyMarv(BASE_DIR, ".bat")
CopyMarv("Data", ".json")
CopyMarv("OSSLicenses", ".md")

shutil.make_archive("Release", "zip", "Temp")