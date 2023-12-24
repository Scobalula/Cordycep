import os
import shutil

def copy_files(src, dst):
    dst_path = os.path.join(dst, os.path.basename(src))

    if os.path.isfile(src):
        shutil.copy(src, dst_path)
        print(f"Copying file: '{src}' -> '{dst_path}'")

    elif os.path.isdir(src):
        shutil.copytree(src, dst_path, dirs_exist_ok=True)
        print(f"Copying folder: '{src}' -> '{dst_path}'")


def main():
    script_path = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_path)

    src_data_folder = "Data"
    toml_folder = os.path.normpath(os.path.join("Data", "Configs", "Toml"))
    des_data_folder = os.path.normpath(os.path.join("..", "src", "x64", "Debug", "Data"))
    dst_configs_folder = os.path.join(des_data_folder, "Configs")


    if not os.path.exists(dst_configs_folder):
        os.makedirs(dst_configs_folder)

    for item in os.listdir(src_data_folder):
        src_path = os.path.join(src_data_folder, item)
        if os.path.isdir(src_path) and item != "Configs":
            copy_files(src_path, des_data_folder)
    
    print("")

    for item in os.listdir(toml_folder):
        src = os.path.join(toml_folder, item)
        if os.path.isfile(src) and item.endswith(".toml"):
            copy_files(src, dst_configs_folder)

    print("Files copying finished.")

if __name__ == "__main__":
    main()