# WebAssembly

The optional standalone WebAssembly target requires an installed and activated
Emscripten SDK. Configure with Emscripten's CMake wrapper and disable the
native CLI:

```sh
emcmake cmake -S . -B build-wasm \
  -DAMCORE_BUILD_WASM=ON \
  -DAMCORE_BUILD_CLI=OFF
cmake --build build-wasm --target amcore_wasm
```

The build produces `build-wasm/amcore.wasm`, which exports the calculation
functions, including `amcore_free_space_wavelength_m`,
`amcore_far_field_distance`, `amcore_far_field_line_of_sight_km`,
`amcore_far_field_radio_horizon_km`, and `amcore_far_field_path_loss_db`,
and the near-field functions
`amcore_near_field_separation_distance_m`,
`amcore_near_field_planar_sampling_spacing_m`,
`amcore_near_field_planar_scan_length_m`,
`amcore_near_field_planar_angle_of_view_deg`,
`amcore_near_field_planar_sampling_parameters`,
`amcore_near_field_cylindrical_y_scan_length_m` and
`amcore_near_field_spherical_maximum_radius_extent_m`, without a `main` entry
point. The C functions keep their normal signatures;
result pointers refer to locations in WebAssembly linear memory. When
`AMCORE_BUILD_WASM` is enabled, the module is installed into the library
directory.
