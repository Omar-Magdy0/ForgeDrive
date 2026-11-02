import argparse
from pathlib import Path
import sys
import re


context = {
    "files": {},       # Original source files
    "sources": {},     # Preprocessed source
    "tokens": {},      # Tokenized source
    "namespaces": {},  # Extracted declarations
    
    "enums": {},
    "structs": {},
    
    "errors": [],
    "warnings": [],
}

TOKEN_PATTERN = re.compile(
    r"""
    (?P<WHITESPACE>\s+)
  | (?P<IDENT>[A-Za-z_][A-Za-z0-9_]*)
  | (?P<NUMBER>
        (?:0[xX][0-9A-Fa-f]+|[0-9]+)
        (?:[uUlLfF]*)
    )
  | (?P<STRING>"(?:\\.|[^"\\])*")
  | (?P<CHAR>'(?:\\.|[^'\\])*')
  | (?P<SYMBOL>::|&&|\|\||<<|>>|<=|>=|==|!=|\+\+|--|->|\+=|-=|\*=|/=|[{}()\[\];,:=*+\-/%.&|^~!?<>#])
    """,
    re.VERBOSE | re.DOTALL
)

def parse_args():
    parser = argparse.ArgumentParser(
        description="Parse C++ type headers for ForgeDrive Serializer code generation."
    )

    parser.add_argument(
        "-i", "--input",
        dest="inputs",
        nargs="+",
        required=True,
        metavar="FILE",
        help="Input C++ headers."
    )

    parser.add_argument(
        "-o", "--output",
        type=Path,
        required=True,
        metavar="DIR",
        help="Output directory for generated files."
    )

    parser.add_argument(
        "--json-library",
        choices=["cjson", "arduinojson", "nlohmann"],
        default="nlohmann",
        help="Target JSON library (reserved for code generation)."
    )

    return parser.parse_args()


def load_inputs(paths):
    for path in paths:
        path = Path(path).resolve()

        if not path.is_file():
            raise FileNotFoundError(f"Input header not found: {path}")

        if path.suffix not in (".h", ".hpp", ".hh"):
            raise ValueError(f"Unsupported input file extension: {path}")

        if path in context["files"]:
            continue

        context["files"][path] = path.read_text(encoding="utf-8")

    return context["files"]


def preprocess(file, ctx):
    source = ctx["files"][file]

    source = re.sub(
        r"/\*.*?\*/",
        lambda m: "\n" * m.group().count("\n"),
        source,
        flags=re.DOTALL
    )

    source = re.sub(r"//[^\n]*", "", source)

    source = re.sub(
        r"^[ \t]*#.*$",
        "",
        source,
        flags=re.MULTILINE
    )

    ctx["sources"][file] = source


def tokenize(file, ctx):
    source = ctx["sources"][file]
    tokens = []

    pos = 0
    line = 1
    column = 1

    while pos < len(source):
        match = TOKEN_PATTERN.match(source, pos)

        if not match:
            raise SyntaxError(
                f"{file}:{line}:{column}: Invalid character {source[pos]!r}"
            )

        value = match.group()
        kind = match.lastgroup

        if kind != "WHITESPACE":
            tokens.append({
                "kind": kind,
                "value": value,
                "line": line,
                "column": column,
                "file": file,
            })

        lines = value.count("\n")

        if lines:
            line += lines
            column = len(value) - value.rfind("\n")
        else:
            column += len(value)

        pos = match.end()

    ctx["tokens"][file] = tokens




class Parser:
    def __init__(self, file, tokens, ctx):
        self.file = file
        self.tokens = tokens
        self.ctx = ctx
        self.pos = 0

    def peek(self, offset=0):
        index = self.pos + offset
        if index >= len(self.tokens):
            return None
        return self.tokens[index]["value"]

    def consume(self, value=None):
        if self.pos >= len(self.tokens):
            self.error("Unexpected end of file")

        token = self.tokens[self.pos]

        if value is not None and token["value"] != value:
            self.error(f"Expected '{value}', got '{token['value']}'")

        self.pos += 1
        return token

    def error(self, message):
        if self.pos < len(self.tokens):
            token = self.tokens[self.pos]
            line = token["line"]
            column = token["column"]
        else:
            line = self.tokens[-1]["line"] if self.tokens else 1
            column = self.tokens[-1]["column"] if self.tokens else 1

        raise SyntaxError(
            f"{self.file}:{line}:{column}: {message}"
        )

    def parse(self):
        while self.peek() is not None:
            if self.peek() == "enum":
                self.parse_enum()

            elif self.peek() == "struct":
                self.parse_struct()

            else:
                self.error(f"Unsupported top-level declaration: {self.peek()}")

    def parse_enum(self):
        self.consume("enum")

        if self.peek() != "struct":
            self.error("Only 'enum struct' declarations are supported")

        self.consume("struct")

        name = self.consume()
        if name["kind"] != "IDENT":
            self.error("Expected enum name")

        self.consume(":")

        Type = []
        while self.peek() not in ("{", None):
            Type.append(self.consume())

        if not Type:
            self.error("Enum Type is required")

        self.consume("{")

        values = []

        while self.peek() != "}":
            if self.peek() is None:
                self.error("Unterminated enum declaration")

            value = self.consume()

            if value["kind"] != "IDENT":
                self.error("Expected enumerator name")

            entry = {
                "name": value["value"],
                "value": None,
                "line": value["line"],
            }

            if self.peek() == "=":
                self.consume("=")

                expression = []
                while self.peek() not in (",", "}", None):
                    expression.append(self.consume()["value"])

                if not expression:
                    self.error("Expected enum value expression")

                entry["value"] = expression

            values.append(entry)

            if self.peek() == ",":
                self.consume(",")

            elif self.peek() != "}":
                self.error("Expected ',' or '}' after enumerator")

        self.consume("}")
        self.consume(";")

        enum_name = name["value"]

        if enum_name in self.ctx["enums"] or enum_name in self.ctx["structs"]:
            self.error(f"Duplicate type declaration: {enum_name}")

        self.ctx["enums"][enum_name] = {
            "name": enum_name,
            "Type": [t["value"] for t in Type],
            "values": values,
            "file": self.file,
            "line": name["line"],
        }

    def parse_struct(self):
        self.consume("struct")

        name = self.consume()
        if name["kind"] != "IDENT":
            self.error("Expected struct name")

        struct_name = name["value"]

        if struct_name in self.ctx["structs"] or struct_name in self.ctx["enums"]:
            self.error(f"Duplicate type declaration: {struct_name}")

        self.consume("{")

        members = []

        while self.peek() != "}":
            if self.peek() is None:
                self.error("Unterminated struct declaration")

            if self.peek() in ("struct", "class", "enum", "union"):
                self.error("Nested type declarations are not supported")

            if self.peek() in ("public", "private", "protected") and self.peek(1) == ":":
                self.error("Access specifiers are not supported")

            members.append(self.parse_member())

        self.consume("}")
        self.consume(";")

        self.ctx["structs"][struct_name] = {
            "name": struct_name,
            "members": members,
            "file": self.file,
            "line": name["line"],
        }

    def parse_member(self):
        tokens = []
        initializer_brace = False

        while self.peek() is not None:
            value = self.peek()

            if value == ";":
                break

            if value in ("struct", "class", "enum", "union"):
                self.error("Nested type declarations are not supported")

            if value in ("public", "private", "protected") and self.peek(1) == ":":
                self.error("Access specifiers are not supported")

            if value == "{":
                if initializer_brace or not tokens or tokens[-1]["value"] != "=":
                    self.error("Unexpected brace in member declaration")
                initializer_brace = True

            elif value == "}":
                if not initializer_brace:
                    self.error("Unexpected '}' in member declaration")
                initializer_brace = False

            tokens.append(self.consume())

        if self.peek() != ";":
            self.error("Unterminated member declaration")

        if initializer_brace:
            self.error("Unterminated member initializer")

        if not tokens:
            self.error("Empty member declaration")

        self.consume(";")

        return self.interpret_member(tokens)

    def interpret_member(self, tokens):
        # Initial implementation:
        # Keep the declaration tokens intact. Type/name/array extraction
        # will be implemented separately.

        return {
            "declaration": [token["value"] for token in tokens],
            "type": None,
            "spec": None,
            "name": None,
            "line": tokens[0]["line"],
        }



def parse_headers(ctx):
    for file in ctx["files"]:
        preprocess(file, ctx)
        tokenize(file, ctx)

    for file, tokens in ctx["tokens"].items():
        parser = Parser(file, tokens, ctx)
        parser.parse()


def main():
    args = parse_args()

    try:
        files = load_inputs(args.inputs)
    except (OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    args.output.mkdir(parents=True, exist_ok=True)

    print(f"Input files: {len(files)}")
    for path in files:
        print(f"  {path}")

    print(f"Output directory: {args.output.resolve()}")
    print(f"JSON library: {args.json_library}")

    try:
        parse_headers(context)
    except SyntaxError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    print(context["enums"])
    print(context["structs"])
    print(f"Parsed enums: {len(context['enums'])}")
    print(f"Parsed structs: {len(context['structs'])}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
