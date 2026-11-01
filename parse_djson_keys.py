#!/usr/bin/env python3
import json
import re
import sys
import os
from pathlib import Path

TARGET_CHANNELS = {
    # Powertrain & Core Telemetry
    "speed_kmh": ["speed", "speedkmh", "speedlocal", "filteredspeedlocal"],
    "rpm": ["rpms", "rpm", "engine_rpm"],
    "max_rpm": ["maxrpm", "carsettings_currentdisplayedrpmpercent", "redline"],
    "gear": ["gear", "selectedgear"],
    "turbo_boost": ["turbo", "turboboost"],
    
    # Timing & Deltas
    "current_lap_time": ["currentlaptime", "currentlap"],
    "last_lap_time": ["lastlaptime"],
    "best_lap_time": ["bestlaptime"],
    "delta_to_best": ["deltatosessionbest", "sessionbestlivedeltaseconds", "mdeltabest", "livedelta"],
    "current_lap": ["currentlap"],
    "position": ["position", "leaderboardposition"],
    
    # Driver Electronics & Aids
    "brake_bias": ["brakebias"],
    "tc_level": ["tclevel", "tractioncontrolmap", "vm_tractioncontrolmap"],
    "tc_cut": ["tractioncontrolpowercutmap", "vm_tractioncontrolpowercutmap"],
    "tc_slip": ["tractioncontrolslipanglemap", "vm_tractioncontrolslipanglemap"],
    "abs_level": ["abslevel", "antilockbrakesystemmap", "vm_antilockbrakesystemmap"],
    "engine_map": ["enginemap", "engine_mixture", "vm_engine_mixture"],
    "pit_limiter_on": ["pitlimiteron"],
    
    # Tyres & Brake Temperatures
    "fl.pressure": ["tyrepressurefrontleft", "wheels01.mpressure"],
    "fr.pressure": ["tyrepressurefrontright", "wheels02.mpressure"],
    "rl.pressure": ["tyrepressurerearleft", "wheels03.mpressure"],
    "rr.pressure": ["tyrepressurerearright", "wheels04.mpressure"],
    "fl.brake_c": ["braketemperaturefrontleft"],
    "fr.brake_c": ["braketemperaturefrontright"],
    "rl.brake_c": ["braketematurerearleft", "braketemperaturefrontleft"],
    "rr.brake_c": ["braketemperaturefrontright", "braketematurerearright"],
    "fl.temp_c": ["tyretemperaturefrontleft"],
    "fr.temp_c": ["tyretemperaturefrontright"],
    "rl.temp_c": ["tyretemperaturerearleft"],
    "rr.temp_c": ["tyretemperaturerearright"],
    
    # Car Diagnostics & Status
    "fuel_pct": ["fuelpercent", "fuel_remaining", "fuel"],
    "headlights_on": ["headlights", "mheadlights"],
    "engine_running": ["engineignitionon", "enginestarted"],
}

def clean_expr(expr):
    if not isinstance(expr, str):
        return ""
    return re.sub(r'[\r\n\t]', ' ', expr).strip()

def match_target(expr_or_name):
    if not isinstance(expr_or_name, str):
        return None
    clean = re.sub(r'[^a-zA-Z0-9_]', '', expr_or_name.lower())
    for var_name, keywords in TARGET_CHANNELS.items():
        for kw in keywords:
            if kw in clean:
                return var_name
    return None

def extract_node_bindings(item, screen_idx, parent_name=""):
    if not isinstance(item, dict):
        return []

    results = []
    sid = item.get("Sid")
    name = item.get("Name", "")
    item_type = item.get("$type", "").split(",")[-2].split(".")[-1] if "$type" in item else ""
    full_label = f"{parent_name}/{name}" if parent_name else name

    bindings = item.get("Bindings", {})
    if isinstance(bindings, dict):
        for bind_prop, bind_data in bindings.items():
            if not isinstance(bind_data, dict):
                continue
            formula = bind_data.get("Formula", {})
            if isinstance(formula, dict):
                expr = clean_expr(formula.get("Expression", ""))
                if expr:
                    matched = match_target(expr) or match_target(full_label)
                    results.append({
                        "sid": sid,
                        "screen_idx": screen_idx,
                        "matched_var": matched
                    })

    if "SpeedText" in item_type:
        results.append({"sid": sid, "screen_idx": screen_idx, "matched_var": "speed_kmh"})
    elif "GearText" in item_type:
        results.append({"sid": sid, "screen_idx": screen_idx, "matched_var": "gear"})
    elif "RPMGauge" in item_type:
        results.append({"sid": sid, "screen_idx": screen_idx, "matched_var": "rpm"})

    for container in ["Childrens", "Items"]:
        if container in item and isinstance(item[container], list):
            for child in item[container]:
                results.extend(extract_node_bindings(child, screen_idx, full_label))

    return results

def analyze_single_djson(filepath):
    try:
        with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
            data = json.load(f)
    except Exception:
        return None

    if not isinstance(data, dict):
        return None

    screens = data.get("Screens", [])
    dash_title = data.get("Title", data.get("Metadata", {}).get("Title", os.path.basename(filepath)))
    if not isinstance(dash_title, str):
        dash_title = os.path.basename(filepath)
    
    findings = []
    if isinstance(screens, list) and len(screens) > 0:
        for s_idx, screen in enumerate(screens):
            if not isinstance(screen, dict):
                continue
            s_name = screen.get("Name", f"Screen_{s_idx}")
            items = screen.get("Items", [])
            if isinstance(items, list):
                for it in items:
                    findings.extend(extract_node_bindings(it, s_idx, s_name))
    elif "Items" in data and isinstance(data["Items"], list):
        for it in data["Items"]:
            findings.extend(extract_node_bindings(it, 0, "Widget"))

    found_channels = set()
    key_mapping = {}
    for entry in findings:
        var = entry["matched_var"]
        if var:
            found_channels.add(var)
            if entry["sid"] is not None:
                key = f"{entry['sid']}r{entry['screen_idx']}"
                if key not in key_mapping:
                    key_mapping[key] = var

    return {
        "path": filepath,
        "title": dash_title,
        "score": len(found_channels),
        "key_mapping": key_mapping
    }

def main():
    if len(sys.argv) < 2:
        print("// Error: Provide path to DashTemplates directory or .djson file", file=sys.stderr)
        sys.exit(1)

    target_path = Path(sys.argv[1])
    djson_files = []

    if target_path.is_file() and target_path.suffix.lower() == ".djson":
        djson_files = [target_path]
    elif target_path.is_dir():
        djson_files = list(target_path.rglob("*.djson"))
    else:
        print("// Error: Invalid file or directory", file=sys.stderr)
        sys.exit(1)

    results = []
    for f in djson_files:
        res = analyze_single_djson(str(f))
        if res and res["score"] > 0:
            results.append(res)

    if not results:
        print("// Error: No telemetry channels identified in the provided target", file=sys.stderr)
        sys.exit(1)

    results.sort(key=lambda x: x["score"], reverse=True)
    best = results[0]

    parent_folder = os.path.basename(os.path.dirname(best['path']))
    file_name = os.path.basename(best['path'])

    print(f"// Template: {best['title']} (Score: {best['score']}/{len(TARGET_CHANNELS)})")
    print(f"static const char* TEMPLATE_PATH = \"/Dashtemplates/{parent_folder}/{file_name}\";\n")
    print("if (d.containsKey(\"T\")) {")
    print("    JsonObject t = d[\"T\"];\n")

    for key, var in sorted(best['key_mapping'].items()):
        if "gear" in var:
            print(f"    // Gear ({key})")
            print(f"    if (t.containsKey(\"{key}\")) {{")
            print(f"        const char* g = t[\"{key}\"];")
            print(f"        if (g) strncpy(live_sim_data.gear, g, sizeof(live_sim_data.gear) - 1);")
            print(f"    }}\n")
        elif "time" in var:
            print(f"    // Lap Time ({key})")
            print(f"    if (t.containsKey(\"{key}\")) {{")
            print(f"        const char* s = t[\"{key}\"];")
            print(f"        if (s) strncpy(live_sim_data.{var}, s, sizeof(live_sim_data.{var}) - 1);")
            print(f"    }}\n")
        elif any(k in var for k in ["rpm", "lap", "level", "map", "cut", "slip", "position"]):
            print(f"    // Integer Channel: {var} ({key})")
            print(f"    if (t.containsKey(\"{key}\")) {{")
            print(f"        live_sim_data.{var} = atoi(t[\"{key}\"].as<const char*>());")
            print(f"    }}\n")
        else:
            print(f"    // Float Channel: {var} ({key})")
            print(f"    if (t.containsKey(\"{key}\")) {{")
            print(f"        live_sim_data.{var} = atof(t[\"{key}\"].as<const char*>());")
            print(f"    }}\n")

    print("}")

if __name__ == "__main__":
    main()
