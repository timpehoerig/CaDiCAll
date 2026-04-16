#!/usr/bin/env bash

# all total
python3 bms.py BMS bms_all --min 450
python3 bms.py custom-runs custom_all --min 26 --max 33
python3 bms.py mc2025 mc_all --max 23

# BMS
python3 bms.py BMS bms_all_closeup --min 482
python3 bms.py BMS bms_t_vs_sr -s -p t sr
python3 bms.py BMS bms_sr_vs_s -s -p sr s
python3 bms.py BMS bms_r_vs_ -s -p r ""

# Custom
python3 bms.py custom-runs custom_no_d_closeup -p r sr "" s t f sf fr sfr --min 25 --max 33
python3 bms.py custom-runs custom_t_vs_r -s -p t r

# mc
python3 bms.py mc2025 mc_all_closeup --min 8 --max 23
python3 bms.py mc2025 mc_t_vs_r -s -p t r