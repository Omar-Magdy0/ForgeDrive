
import argparse
import importlib
import importlib.util
import re
import sys
from pathlib import Path

from jinja2 import Environment, FileSystemLoader, StrictUndefined, TemplateNotFound


GEN_DIR = Path(__file__).resolve().parent
DRIVER_ROOT = GEN_DIR.parent
SHARED_TEMPLATE_DIR = DRIVER_ROOT / "templates"

# Configuration section -> output template names.

def resolve_template_family(platform_name):
    if platform_name.startswith("stm32f"):
        return "stm32"

    return platform_name

def generate_drivers(config, output_dir, env):
    generated = {}

    # 1. Generate individual peripheral/component drivers
    for driver_name, driver_config in config.items():
        if driver_name == "platform":
            continue

        if not isinstance(driver_config, dict):
            continue

        if not driver_config.get("enabled", True):
            continue

        prefix = f"fd_{driver_name}"

        for extension in (".c", ".h", ".c.j2", ".h.j2", ".cpp", ".hpp", ".cpp.j2", ".hpp.j2"):
            filename = prefix + extension

            try:
                template = env.get_template(filename)
            except TemplateNotFound:
                continue

            generated[filename] = template.render(**config)

    # 2. Generate core/shared files (e.g., fd_drv.c and fd_drv.h)
    core_files = ["fd_drv.c", "fd_drv.cpp", "fd_drv.h", "fd_adc.cpp", "fd_adc.h", "fd_drv.c.j2", "fd_drv.h.j2"]
    # Optional
    if "cortos_port" in config:
        core_files.append("cortos_port.c")
    for filename in core_files:
        try:
            template = env.get_template(filename)
            # Strip the .j2 extension if present so the output file name is correct
            out_filename = filename[:-3] if filename.endswith(".j2") else filename
            generated[out_filename] = template.render(**config)
        except TemplateNotFound:
            continue

    # Write all successfully rendered files.
    for filename, rendered in generated.items():
        output_path =  output_dir / filename
        output_path.write_text(rendered, encoding="utf-8")
        print(f"Generated: {filename}")

    return set(generated)


def remove_stale_files(active_files, output_dir):
    previous_files = set()
    manifest_path = output_dir/".fd_generated_files"
    if manifest_path.is_file():
        previous_files = {
            line.strip()
            for line in manifest_path.read_text(encoding="utf-8").splitlines()
            if line.strip()
        }

    # Delete only files previously recorded as generator-owned.
    for filename in previous_files - active_files:
        # Prevent paths in the manifest from escaping DRIVER_ROOT.
        if Path(filename).name != filename:
            raise ValueError(
                f"Invalid filename in generated-file manifest: {filename}"
            )

        output_path = DRIVER_ROOT / filename

        if output_path.is_file():
            output_path.unlink()
            print(f"Removed stale generated file: {filename}")

    manifest_path.write_text(
        "".join(f"{filename}\n" for filename in sorted(active_files)),
        encoding="utf-8",
    )


def load_config(config_path_str):
    path = Path(config_path_str)

    if not path.suffix and "/" not in config_path_str and "\\" not in config_path_str:
        try:
            mod = importlib.import_module(config_path_str)
            return mod.config
        except ModuleNotFoundError as exc:
            if exc.name != config_path_str:
                raise
        except AttributeError as exc:
            raise AttributeError(
                f"Configuration module '{config_path_str}' "
                "must define a 'config' dictionary."
            ) from exc

    if not path.is_absolute():
        path = (Path.cwd() / path).resolve()

    if not path.is_file():
        raise FileNotFoundError(f"Configuration file not found: {path}")

    spec = importlib.util.spec_from_file_location(
        "fd_custom_driver_config", path
    )
    if spec is None or spec.loader is None:
        raise ImportError(f"Could not load configuration file: {path}")

    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)

    if not hasattr(mod, "config"):
        raise AttributeError(
            f"Configuration file '{path.name}' must define a 'config' dictionary."
        )

    return mod.config


def resolve_platform(platform):

    platform_dir = DRIVER_ROOT / platform
    if not platform_dir.is_dir():
        raise FileNotFoundError(f"Platform directory not found: {platform_dir}")

    return platform, platform_dir

def make_environment(platform_name, platform_dir):
    family_name = resolve_template_family(platform_name)

    search_dirs = [
        platform_dir / "templates",          # Exact platform overrides
        SHARED_TEMPLATE_DIR / platform_name, # Exact platform shared templates
        SHARED_TEMPLATE_DIR / family_name,   # Family-shared templates
        platform_dir,                        # Exact platform fallback
        SHARED_TEMPLATE_DIR,                 # Global fallback
    ]

    search_dirs = [
        str(path) for path in search_dirs
        if path.is_dir()
    ]

    if not search_dirs:
        raise FileNotFoundError("No template directories found.")

    print("Template search paths:")
    for path in search_dirs:
        print(f"  {path}")

    return Environment(
        loader=FileSystemLoader(search_dirs),
        undefined=StrictUndefined,
        keep_trailing_newline=True,
        trim_blocks=True,
        lstrip_blocks=True,
    )


def validate_config(config, platform_dir):
    if not isinstance(config, dict):
        raise ValueError("Configuration must be a dictionary.")

    # TODO: common schema validation and platform-specific validation.
    # Platform validator is expected at platform_dir / "gen" / "validate.py".


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
        config = load_config(args.config_file)
        platform_name, platform_dir = resolve_platform(args.device)
        validate_config(config, platform_dir)

        output_dir = args.output.resolve() # Converts to an absolute path cleanly
        output_dir.mkdir(parents=True, exist_ok=True) # Ensures the folder exists
        env = make_environment(platform_name, platform_dir)
        active_files = generate_drivers(config, output_dir, env)
        remove_stale_files(active_files, output_dir)

    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        raise SystemExit(1)


if __name__ == "__main__":
    main()
