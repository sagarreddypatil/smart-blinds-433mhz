import os

def embed_files():
    files = [("www/index.html", "index_html")]

    with open("src/embedded_files.h", "w") as out:
        out.write("#pragma once\n#include <pgmspace.h>\n\n")
        for filepath, varname in files:
            with open(filepath, "r") as f:
                content = f.read()
            escaped = content.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
            out.write(f'const char {varname}[] PROGMEM = "{escaped}";\n\n')

embed_files()