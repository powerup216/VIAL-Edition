VIA_ENABLE = yes
VIAL_ENABLE = yes

# The two TMAG5273 wheels are exposed as encoders so they can be remapped in
# Vial. keymap.c is the driver: it turns wheel motion into encoder events.
ENCODER_ENABLE = yes
ENCODER_MAP_ENABLE = yes
ENCODER_DRIVER = custom
