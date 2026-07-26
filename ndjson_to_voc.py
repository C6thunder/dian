"""steelball.ndjson → Pascal VOC 格式（images/ + xml/）"""
import json
import urllib.request
import os
import xml.etree.ElementTree as ET
from xml.dom import minidom

NDJSON  = r"E:\Downloads\steelball.ndjson"
OUT_DIR = r"E:\Downloads\steelball_voc"
IMG_DIR = os.path.join(OUT_DIR, "images")
XML_DIR = os.path.join(OUT_DIR, "xml")
os.makedirs(IMG_DIR, exist_ok=True)
os.makedirs(XML_DIR, exist_ok=True)

MAX_IMAGES = 3000
class_names = {}
count = 0

def make_xml(fname, w, h, boxes):
    """生成 Pascal VOC 格式 XML"""
    root = ET.Element("annotation")
    ET.SubElement(root, "folder").text = "images"
    ET.SubElement(root, "filename").text = fname

    size = ET.SubElement(root, "size")
    ET.SubElement(size, "width").text = str(w)
    ET.SubElement(size, "height").text = str(h)
    ET.SubElement(size, "depth").text = "3"

    for box in boxes:
        cls_id, cx, cy, bw, bh = box[0], box[1], box[2], box[3], box[4]
        x1 = max(0, int((cx - bw/2) * w))
        y1 = max(0, int((cy - bh/2) * h))
        x2 = min(w, int((cx + bw/2) * w))
        y2 = min(h, int((cy + bh/2) * h))

        obj = ET.SubElement(root, "object")
        ET.SubElement(obj, "name").text = class_names.get(str(cls_id), f"cls{cls_id}")
        ET.SubElement(obj, "pose").text = "Unspecified"
        ET.SubElement(obj, "truncated").text = "0"
        ET.SubElement(obj, "difficult").text = "0"
        bndbox = ET.SubElement(obj, "bndbox")
        ET.SubElement(bndbox, "xmin").text = str(x1)
        ET.SubElement(bndbox, "ymin").text = str(y1)
        ET.SubElement(bndbox, "xmax").text = str(x2)
        ET.SubElement(bndbox, "ymax").text = str(y2)

    xml_str = minidom.parseString(ET.tostring(root)).toprettyxml(indent="  ")
    return xml_str


with open(NDJSON, "r", encoding="utf-8") as f:
    for line in f:
        obj = json.loads(line.strip())

        if obj["type"] == "dataset":
            class_names = obj.get("class_names", {})
            print(f"数据集: {obj['name']}, 类别: {class_names}")
            continue

        if obj["type"] != "image":
            continue

        if count >= MAX_IMAGES:
            break

        url    = obj["url"]
        fname  = obj["file"]
        w, h   = obj["width"], obj["height"]
        boxes  = obj.get("annotations", {}).get("boxes", [])

        # 文件名唯一化
        base_name = os.path.splitext(fname)[0]
        img_path  = os.path.join(IMG_DIR, fname)
        xml_path  = os.path.join(XML_DIR, base_name + ".xml")

        # 跳过已存在的
        if os.path.exists(img_path) and os.path.exists(xml_path):
            count += 1
            continue

        # 下载图片
        try:
            urllib.request.urlretrieve(url, img_path)
        except Exception as e:
            print(f"[{count}] 下载失败 {fname}: {e}")
            continue

        # 生成 XML
        xml_str = make_xml(fname, w, h, boxes)
        with open(xml_path, "w", encoding="utf-8") as xf:
            xf.write(xml_str)

        count += 1
        if count % 100 == 0:
            print(f"  已处理 {count} 张...")

print(f"\n完成！共 {count} 张图片")
print(f"  images: {IMG_DIR}")
print(f"  xml:    {XML_DIR}")
