import spaudiopy as spa
import numpy as np
import json
import matplotlib.pyplot as plt

t_design_order = 10  # Results in 60 positions
export_path = "_configurationFiles/t_design_10.json"

# Load t-design
positions = spa.grids.load_t_design(10)
spa.plot.hull(spa.decoder.get_hull(*positions.T))

# Convert to spherical coordinates
azi, zen, r = spa.utils.cart2sph(positions[:, 0], positions[:, 1], positions[:, 2])

azi = spa.utils.rad2deg(azi)
zen = spa.utils.rad2deg(zen)

# Convert zenith to elevation
ele = 90 - zen

# Parse as JSON
ls = []
for i in range(len(azi)):
    tmp_dict = {
        "Azimuth": azi[i],
        "Elevation": ele[i],
        "Radius": r[i],
        "IsImaginary": False,
        "Channel": i + 1,
        "Gain": 1.0,
    }

    ls.append(tmp_dict)

out = {
    "Name": "t-design (degree: 10)",
    "Description": "t-design of degree 10, which can be used to decode Ambisonic signals, process them with non-Ambisonic effects, and encode them again.",
    "LoudspeakerLayout": {
        "Name": "t-design (degree: 10)",
        "Description": "t-design of degree 10, which can be used to decode Ambisonic signals, process them with non-Ambisonic effects, and encode them again.",
        "Loudspeakers": ls,
    },
}

# Writing to sample.json
with open(export_path, "w") as outfile:
    outfile.write(json.dumps(out, indent=2))
