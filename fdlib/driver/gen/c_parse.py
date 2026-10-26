
import re
from pathlib import Path


IDENT = r"[A-Za-z_]\w*"


def strip_comments(src):
    """Remove C comments while preserving strings and newlines."""
    out = []
    i = 0
    state = "code"

    while i < len(src):
        c = src[i]
        n = src[i + 1] if i + 1 < len(src) else ""

        if state == "code":
            if c == "/" and n == "*":
                out.extend("  ")
                i += 2
                state = "block"
                continue
            if c == "/" and n == "/":
                out.extend("  ")
                i += 2
                state = "line"
                continue
            if c == '"':
                state = "string"
            elif c == "'":
                state = "char"
            out.append(c)

        elif state == "block":
            if c == "*" and n == "/":
                out.extend("  ")
                i += 2
                state = "code"
                continue
            out.append("\n" if c == "\n" else " ")

        elif state == "line":
            if c == "\n":
                out.append("\n")
                state = "code"
            else:
                out.append(" ")

        else:  # string or character literal
            out.append(c)
            if c == "\\" and i + 1 < len(src):
                i += 1
                out.append(src[i])
            elif (state == "string" and c == '"') or (
                state == "char" and c == "'"
            ):
                state = "code"

        i += 1

    return "".join(out)


def matching_brace(src, start):
    """Return the index of the matching closing brace."""
    depth = 0
    for i in range(start, len(src)):
        if src[i] == "{":
            depth += 1
        elif src[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    return -1


def extract_macros(src):
    macros = {}

    # Join backslash-continued preprocessor lines first.
    src = re.sub(r"\\\r?\n", " ", src)

    for line in src.splitlines():
        m = re.match(
            r"^\s*#\s*define\s+(" + IDENT +
            r")(\([^\n]*?\))?[ \t]*(.*)$",
            line,
        )
        if m:
            name, params, value = m.groups()
            key = name + params if params else name
            macros[key] = value.strip()

    return macros


def extract_typedefs(src):
    structs = {}
    enums = {}

    # Deliberately supports typedef struct/enum declarations,
    # including anonymous typedefs and optional struct/enum tags.
    pattern = re.compile(r"\btypedef\s+(struct|enum)\b")

    pos = 0
    while m := pattern.search(src, pos):
        kind = m.group(1)
        brace = src.find("{", m.end())

        # No body, or another declaration appeared first.
        semi = src.find(";", m.end())
        if brace < 0 or (semi >= 0 and semi < brace):
            pos = m.end()
            continue

        end = matching_brace(src, brace)
        if end < 0:
            break

        # The typedef alias follows the closing brace.
        alias_match = re.match(
            r"\s*(" + IDENT + r")\s*;",
            src[end + 1:],
        )
        if not alias_match:
            pos = end + 1
            continue

        alias = alias_match.group(1)
        body = src[brace + 1:end]

        if kind == "struct":
            members = []
            for declaration in body.split(";"):
                declaration = declaration.strip()
                if not declaration:
                    continue

                # Take the last identifier before an optional
                # array suffix or bit-field width.
                declaration = re.sub(
                    r"\[[^\]]*\]", " ", declaration
                )
                declaration = re.sub(
                    r":\s*\d+\s*$", "", declaration
                )
                names = re.findall(IDENT, declaration)
                if names:
                    members.append(names[-1])

            structs[alias] = members

        else:
            members = []
            for item in body.split(","):
                item = item.strip()
                em = re.match(r"(" + IDENT + r")\b", item)
                if em:
                    members.append(em.group(1))

            enums[alias] = members

        pos = end + 1 + alias_match.end()

    return structs, enums


def parse_header(filename):
    src = Path(filename).read_text(encoding="utf-8")
    src = strip_comments(src)

    structs, enums = extract_typedefs(src)

    return {
        "macros": extract_macros(src),
        "structs": structs,
        "enums": enums,
    }
