
import argparse
import importlib
import importlib.util
import shutil
import traceback
import json
import re
import sys
from pathlib import Path
from drv import drv
from pprint import pprint
from jinja2 import Environment, FileSystemLoader, StrictUndefined, TemplateNotFound


DRIVER_EXTENSIONS = (".c", ".h", ".cpp")
DRIVER_CORE = ("drv", "adc")
DRIVER_HOST_EXT = ("SILServer", "TCPClient")
MANIFEST_NAME = ".fd_generated_files"
GEN_DIR = Path(__file__).resolve().parent
DRIVER_ROOT = GEN_DIR.parent
SHARED_HEADER_DIR = DRIVER_ROOT

context = {}
context["gen"] = {}
context["gen_sDir"] = []
context["error"] = []
context["warning"] = []
context["generated"] = []

def resolve_drivers():
    for name in context["config"]:
        if name not in context["drivers"]:
            context["drivers"].append(name)
    for driver in context["drivers"]:
        for ext in DRIVER_EXTENSIONS:
            filename = f"fd_{driver}{ext}"
            context["gen"][filename]=None

def generate_drivers():
    ## ERRORS AND WARNINGS
    for warning in context.get("warning", []):
        print(f"WARNING: {warning}")
    if context.get("error"):
        print("\nDriver generation failed:")
        for error in context["error"]:
            print(f"ERROR: {error}")
        return
    temp_dir= context["output_path"]/"temp"
    context["generated"] = set()
    temp_dir.mkdir(parents=True, exist_ok=True)
    try:
        drv(context)
        context["gen_sDir"].extend([
            DRIVER_ROOT / context["device"]["platform"] / context["device"]["family"],
            DRIVER_ROOT / context["device"]["platform"],
            DRIVER_ROOT,
        ])
        gen_file = DRIVER_ROOT / context["device"]["platform"] / "gen" / "gen.py"
        spec = importlib.util.spec_from_file_location(f"{context["device"]["platform"]}_gen", gen_file, )
        if spec is None or spec.loader is None:
            raise ImportError(f"Cannot load platform generator: {gen_file}")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        module.setup(context)
        context["env"] = Environment(
            loader=FileSystemLoader([str(p) for p in context["gen_sDir"]]),
            undefined=StrictUndefined,
            keep_trailing_newline=True,
            trim_blocks=True,
            lstrip_blocks=True,
        )
        print(context["gen"])
        for filename, output_name in context["gen"].items():
            output_path = temp_dir / (output_name or filename)
            output_path.parent.mkdir(parents=True, exist_ok=True)
            try:
                template = context["env"].get_template(filename)
            except TemplateNotFound:
                continue
            content = template.render(**context["config"], device=context["device"])
            output_path.write_text(content, encoding="utf-8")
            context["generated"].add(str(output_path))
    except Exception:
        shutil.rmtree(temp_dir, ignore_errors=True)
        raise
    # if we get here generation succeeded, we can clear anything in output path, and replace them with what is in temp
    output_dir = context["output_path"]
    for item in output_dir.iterdir():
        if item != temp_dir:
            if item.is_dir():
                shutil.rmtree(item)
            else:
                item.unlink()

    for item in temp_dir.iterdir():
        shutil.move(str(item), str(output_dir / item.name))
    shutil.rmtree(temp_dir)
        
def generate_manifest():
    manifest = {
        "device": context["device"],
        "generated_files": sorted(context["generated"]),
        "error": context["error"],
        "warning": context["warning"]
    }
    for i, err in enumerate(context["error"]):
        print(f"ERROR_{i} : {err}\n")
    path = context["output_path"] / MANIFEST_NAME
    path.write_text(
        json.dumps(manifest, indent=4) + "\n",
        encoding="utf-8",
    )

def resolve_device():
    device_lower = context["device"]["name"].lower().strip()
    # Check if it's an STM32 device
    context["drivers"] = []
    context["drivers"].extend(DRIVER_CORE)
    if device_lower.startswith("stm32"):
        match = re.match(r"^stm32([a-z0-9]{2})", device_lower)
        context["device"]["platform"] = f"stm32"
        if match:
            context["device"]["family"] = f"stm32{match.group(1)}"
        else: raise Exception(f"device family not found")

    elif device_lower.startswith("host"):
        # Fallback or generic handling for other microcontrollers/host, etc
        context["device"]["platform"] = device_lower
        context["device"]["family"] = device_lower
        context["drivers"].extend(DRIVER_HOST_EXT)
    else: raise Exception(f"Platform Not supported")

def load_config():
    path = context["config_path"]

    if path.is_file():
        spec = importlib.util.spec_from_file_location("fd_config", path)
        if spec is None or spec.loader is None:
            raise ImportError(f"Cannot load config: {path}")

        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
    else:
        mod = importlib.import_module(context["config_path"])

    config = getattr(mod, "config", None)
    if not isinstance(config, dict):
        raise TypeError("Configuration must define a 'config' dictionary.")

    context["config"] = config



def validate_config():
    # TODO: common schema validation and platform-specific validation.
    # Platform validator is expected at platform_dir / "gen" / "validate.py".
    pass

def main():
    parser = argparse.ArgumentParser(
        description="Generate ForgeDrive driver source files."
    )
    parser.add_argument(
        "config_file",
        nargs="?",
        default="fd_driver_conf",
        help="Configuration file path or importable module name.",
    )
    parser.add_argument(
        "-d", "--device",
        nargs="?",
        default="stm32f4",
        help="device used",
    )
    parser.add_argument(
        "-o", "--output",
        type=Path,
        default=Path("./output"),
        help="Output directory for generated files (relative or absolute path). Defaults to './output'.",
    )
    args = parser.parse_args()

    try:
        context["config_path"] =  Path(args.config_file).resolve()
        context["output_path"] = Path(args.output).resolve()
        context["device"] = { "name": args.device }
        resolve_device()
        load_config()
        resolve_drivers()
        generate_drivers()
        generate_manifest()


    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        traceback.print_exc()
        raise SystemExit(1) from exc


if __name__ == "__main__":
    main()
