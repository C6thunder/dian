"""steelball.ndjson → Pascal VOC ZIP (<500MB, ≤3000张)"""
import json
import urllib.request
import os
import zipfile
import xml.etree.ElementTree as ET
from xml.dom import minidom
from concurrent.futures import ThreadPoolExecutor, as_completed
import time

NDJSON  = r"E:\Downloads\steelball.ndjson"
WORK    = r"E:\Downloads\steelball_voc"
ZIP_OUT = r"E:\Downloads\steelball_voc.zip"
MAX_IMG = 3000
MAX_SIZE_MB = 500
THREADS = 8

os.makedirs(os.path.join(WORK, "images"), exist_ok=True)
os.makedirs(os.path.join(WORK, "xml"), exist_ok=True)

class_names = {}
image_list = []  # [(url, fname, w, h, boxes)]

# ── 1. 解析 NDJSON ──
print("1. 解析 NDJSON...")
with open(NDJSON, "r", encoding="utf-8") as f:
    for line in f:
        obj = json.loads(line.strip())
        if obj["type"] == "dataset":
            class_names = obj.get("class_names", {})
            continue
        if obj["type"] == "image":
            image_list.append((obj["url"], obj["file"],
                               obj["width"], obj["height"],
                               obj.get("annotations", {}).get("boxes", [])))

print(f"   共 {len(image_list)} 张图片，类别: {class_names}")

# ── 2. 生成 VOC XML ──
def make_xml(fname, w, h, boxes):
    root = ET.Element("annotation")
    ET.SubElement(root, "folder").text = "images"
    ET.SubElement(root, "filename").text = fname
    sz = ET.SubElement(root, "size")
    ET.SubElement(sz, "width").text = str(w)
    ET.SubElement(sz, "height").text = str(h)
    ET.SubElement(sz, "depth").text = "3"
    for box in boxes:
        cls_id, cx, cy, bw, bh = box[0], box[1], box[2], box[3], box[4]
        x1 = max(0, int((cx - bw/2) * w))
        y1 = max(0, int((cy - bh/2) * h))
        x2 = min(w, int((cx + bw/2) * w))
        y2 = min(h, int((cy + bh/2) * h))
        obj = ET.SubElement(root, "object")
        ET.SubElement(obj, "name").text = class_names.get(str(cls_id), "object")
        ET.SubElement(obj, "bndbox")
        bnd = obj.find("bndbox")
        ET.SubElement(bnd, "xmin").text = str(x1)
        ET.SubElement(bnd, "ymin").text = str(y1)
        ET.SubElement(bnd, "xmax").text = str(x2)
        ET.SubElement(bnd, "ymax").text = str(y2)
    return minidom.parseString(ET.tostring(root)).toprettyxml(indent="  ")

# ── 3. 下载图片 + 生成 XML（多线程）──
print("2. 下载图片 + 生成标注...")
done = 0
failed = 0
total_bytes = 0

def download_one(item):
    url, fname, w, h, boxes = item
    base = os.path.splitext(fname)[0]
    img_path = os.path.join(WORK, "images", fname)
    xml_path = os.path.join(WORK, "xml", base + ".xml")

    # 跳过已存在
    if os.path.exists(img_path) and os.path.exists(xml_path):
        return fname, os.path.getsize(img_path), "skip"

    try:
        urllib.request.urlretrieve(url, img_path)
    except Exception as e:
        return fname, 0, str(e)

    sz = os.path.getsize(img_path)
    xml_str = make_xml(fname, w, h, boxes)
    with open(xml_path, "w", encoding="utf-8") as xf:
        xf.write(xml_str)
    return fname, sz, "ok"

with ThreadPoolExecutor(max_workers=THREADS) as pool:
    futures = {pool.submit(download_one, item): item for item in image_list[:MAX_IMG]}
    for fut in as_completed(futures):
        fname, sz, status = fut.result()
        if status == "ok":
            done += 1
            total_bytes += sz
        elif status == "skip":
            done += 1
            total_bytes += sz
        else:
            failed += 1
            print(f"   ✗ {fname}: {status}")
        if (done + failed) % 50 == 0:
            print(f"   {done} 张完成, {failed} 失败, {total_bytes/1024/1024:.0f}MB")

print(f"   完成: {done} 张, 失败: {failed} 张, 大小: {total_bytes/1024/1024:.1f}MB")

# ── 4. 打包 ZIP ──
print("3. 打包 ZIP...")
with zipfile.ZipFile(ZIP_OUT, "w", zipfile.ZIP_DEFLATED) as zf:
    for root, dirs, files in os.walk(WORK):
        for fn in files:
            full = os.path.join(root, fn)
            arc  = os.path.relpath(full, WORK)
            zf.write(full, arc)

zip_mb = os.path.getsize(ZIP_OUT) / 1024 / 1024
print(f"   完成: {ZIP_OUT} ({zip_mb:.1f}MB)")

if zip_mb > MAX_SIZE_MB:
    print(f"   ⚠ 超过 {MAX_SIZE_MB}MB 限制!")
else:
    print(f"   ✓ 在 {MAX_SIZE_MB}MB 以内")
