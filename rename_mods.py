import hashlib
import os
import sys

MODS_DIR = "mods"

def sha256_file(filepath):
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while True:
            chunk = f.read(8192)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()

def main():
    if not os.path.isdir(MODS_DIR):
        print(f"错误: 找不到 {MODS_DIR} 目录")
        sys.exit(1)

    renamed = 0
    for name in os.listdir(MODS_DIR):
        if not name.endswith(".dll"):
            continue

        old_path = os.path.join(MODS_DIR, name)
        h = sha256_file(old_path)
        new_name = f"{h}.dll"
        new_path = os.path.join(MODS_DIR, new_name)

        if name == new_name:
            print(f"跳过 {name} (已是哈希名)")
            continue

        if os.path.exists(new_path):
            print(f"警告: {new_name} 已存在，跳过 {name}")
            continue

        os.rename(old_path, new_path)
        print(f"{name} -> {new_name}")
        renamed += 1

    print(f"\n完成! 共重命名 {renamed} 个文件")

if __name__ == "__main__":
    main()