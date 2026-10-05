# ASTRA brand assets

The official ASTRA brand mark is committed here as `astra-mark.png` (the real
asset, stored verbatim — nothing is recoloured, cropped or redrawn). The desktop
header and sidebar load it and draw it beside the `ASTRA` wordmark.

The loader looks for the first of these names in this directory:

- `astra-mark.png`
- `astra-logo.png`
- `astra.png`

PNG and JPEG are supported (loaded with the bundled `stb_image`). If no file is
present the header renders the typographic `ASTRA` wordmark alone, so the terminal
never shows a fabricated or placeholder logo.

The build stages this directory next to the executable (`assets/brand`) and the
install step ships it with the packaged application, so the same asset is used in
the build tree, the packaged app and CI artifacts.

Keep the original source image elsewhere as the canonical copy; the file placed
here is the one the application references at runtime.
