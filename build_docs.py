import os
import glob
import subprocess
import sys

# Try to import markdown
try:
    import markdown
except ImportError:
    print("Error: 'markdown' package is not installed.")
    print("Please run: pip install markdown")
    sys.exit(1)

# Configuration
DOCS_ROOT = "docs"
HELP_ROOT = "build"
HHC_PATH = r"C:\Program Files (x86)\HTML Help Workshop\hhc.exe"
LANGUAGES = ["zh", "en"]

# HTML Template
HTML_TEMPLATE = """<!DOCTYPE html>
<html>
<head>
    <meta http-equiv="Content-Type" content="text/html; charset={charset}">
    <title>{title}</title>
    <style>
        body {{ font-family: "Microsoft YaHei", "SimSun", "Segoe UI", sans-serif; }}
        h1 {{ color: #000080; border-bottom: 2px solid #ccc; }}
        h2 {{ color: #000080; margin-top: 20px; }}
        h3 {{ margin-top: 15px; border-left: 4px solid #000080; padding-left: 10px; color: #333; }}
        pre {{ background-color: #f0f0f0; padding: 10px; overflow-x: auto; }}
        code {{ background-color: #f0f0f0; padding: 2px 4px; font-family: Consolas, monospace; }}
        blockquote {{ border-left: 4px solid #007bff; background-color: #f8f9fa; margin: 10px 0; padding: 10px; }}
        table {{ border-collapse: collapse; width: 100%; margin: 15px 0; }}
        th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
        th {{ background-color: #f2f2f2; }}
    </style>
</head>
<body>
{content}
</body>
</html>"""

def get_file_list(lang_dir):
    return [os.path.basename(f) for f in glob.glob(os.path.join(lang_dir, "*.md"))]

def generate_hhc(lang, file_props, output_path):
    # Sort files to put index first, then others
    sorted_files = []
    
    # Priority handling
    priority = ["index.md", "quick_start.md", "benchmark_defs.md", "scoring.md", "technical.md", "troubleshooting.md"]
    
    for p in priority:
        if p in file_props:
            sorted_files.append((p, file_props[p]))
    
    # Add rest
    for f in file_props:
        if f not in priority:
            sorted_files.append((f, file_props[f]))

    content = """<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<HTML>
<HEAD>
<meta name="GENERATOR" content="SigmaZ Build Script">
<!-- Sitemap 1.0 -->
</HEAD><BODY>
<OBJECT type="text/site properties">
    <param name="ImageType" value="Folder">
</OBJECT>
<UL>
"""
    for fname, title in sorted_files:
        html_name = fname.replace(".md", ".html")
        content += f"""    <LI> <OBJECT type="text/sitemap">
        <param name="Name" value="{title}">
        <param name="Local" value="{html_name}">
        </OBJECT>
"""
    content += "</UL>\n</BODY></HTML>"
    
    with open(output_path, "w", encoding=encoding_for_lang(lang)) as f:
        f.write(content)

def generate_hhp(lang, files, output_path, hhc_name):
    lang_id = "0x804" if lang == "zh" else "0x409"
    default_topic = "index.html"
    title = "SigmaZ Help"
    
    content = f"""[OPTIONS]
Compatibility=1.1 or later
Compiled file=..\\SigmaZ_{lang}.chm
Contents file={hhc_name}
Default topic={default_topic}
Display compile progress=No
Language={lang_id}
Title={title}

[FILES]
"""
    for f in files:
        content += f.replace(".md", ".html") + "\n"
        
    with open(output_path, "w", encoding=encoding_for_lang(lang)) as f:
        f.write(content)

def encoding_for_lang(lang):
    return "gb2312" if lang == "zh" else "windows-1252"

def charset_for_lang(lang):
    return "gb2312" if lang == "zh" else "windows-1252"

def process_lang(lang):
    print(f"Processing language: {lang}...")
    
    src_dir = os.path.join(DOCS_ROOT, lang)
    dst_dir = os.path.join(HELP_ROOT, lang)
    
    if not os.path.exists(dst_dir):
        os.makedirs(dst_dir)
        
    md_files = glob.glob(os.path.join(src_dir, "*.md"))
    file_titles = {}
    
    for md_file in md_files:
        fname = os.path.basename(md_file)
        html_fname = fname.replace(".md", ".html")
        
        with open(md_file, "r", encoding="utf-8") as f:
            md_content = f.read()
            
        # Extract title from first h1
        title = "SigmaZ Help"
        for line in md_content.splitlines():
            if line.startswith("# "):
                title = line[2:].strip()
                break
        
        file_titles[fname] = title
        
        # Convert Markdown to HTML
        html_body = markdown.markdown(md_content, extensions=['tables', 'fenced_code'])
        
        # Wrap in template
        full_html = HTML_TEMPLATE.format(
            charset=charset_for_lang(lang),
            title=title,
            content=html_body
        )
        
        # Write HTML (using legacy encoding for CHM compatibility)
        try:
            with open(os.path.join(dst_dir, html_fname), "w", encoding=encoding_for_lang(lang), errors="xmlcharrefreplace") as f:
                f.write(full_html)
        except Exception as e:
            print(f"Error writing {html_fname}: {e}")

    # Generate HHC and HHP
    hhc_name = f"sigmaz_{lang}.hhc"
    hhp_name = f"sigmaz_{lang}.hhp"
    
    generate_hhc(lang, file_titles, os.path.join(dst_dir, hhc_name))
    generate_hhp(lang, file_titles.keys(), os.path.join(dst_dir, hhp_name), hhc_name)
    
    # Compile
    print(f"Compiling CHM for {lang}...")
    hhp_path = os.path.join(dst_dir, hhp_name)
    cmd = [HHC_PATH, hhp_path]
    
    if os.path.exists(HHC_PATH):
        try:
            # HHC returns 1 on success (weirdly), or uses exit codes for errors/warnings
            subprocess.run(cmd, check=False) 
            print(f"Compilation complete for {lang}.")
        except Exception as e:
            print(f"Error running HHC: {e}")
    else:
        print(f"Warning: HHC not found at {HHC_PATH}. Skipping compilation.")

def main():
    if not os.path.exists(DOCS_ROOT):
        print(f"Error: {DOCS_ROOT} directory not found.")
        return

    for lang in LANGUAGES:
        process_lang(lang)

if __name__ == "__main__":
    main()
